#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "drv.h"
#include <time.h>
#include <sys/mman.h>
#include <string.h>  // for memcpy()

#include "vmebus.h"

#define TRUE 1
#define FALSE 0


// size of memory range to map for each vuprom
#define RANGE 0x1000

// number of 32 bit values in memory range
#define N     (RANGE / sizeof(u_int32_t))

#define MAX_VUPROMS 8

// Maximum scaler number to allow.
#define MAX_SCALER_INDEX    1024

static int n_vuproms =0;

static u_int32_t global_top_bits =0;


// is the driver initialized?
static int _init = 0;

typedef struct {
    u_int32_t  base_addr;   // Address of vuprom
    u_int32_t  map_addr;    // mmap address (without top bits)
    volatile u_int32_t* vme_mem;     // mmaped memory ptr
} vuprom;

int vme_read_once( u_int32_t addr, u_int32_t* out ) {
    addr &= 0x1fffffff;        // obere Bits ausmaskieren
    u_int32_t rest = addr % 0x1000;
    addr = (addr / 0x1000) * 0x1000;
    volatile u_int32_t* mem = NULL;

    if ((mem = vmeext(addr, 0x1000)) == NULL) {
        return FALSE;
    }

    *out = mem[rest/4];
    munmap((void*)mem,0x1000);
    return TRUE;
}

int init_vuprom( vuprom* v ) {

    if ((v->vme_mem = (u_int32_t*) vmeext( v->map_addr, RANGE )) == NULL) {
        perror("Error opening device.\n");
        return 0;
    }

    printf("Init vuprom @ %#010x, map addess: %#010x\n", v->base_addr, v->map_addr);
    return TRUE;
}

void deinit_vuprom( vuprom* v ) {
    if( v->vme_mem ) {
        munmap( (void*)(v->vme_mem), RANGE );
        v->vme_mem = NULL;
    }
}

static vuprom vu[MAX_VUPROMS];

unsigned long SetTopBits(unsigned long Myaddr) {
    volatile unsigned long *Mypoi;

    // obere 3 Bits setzen via Register 0xaa000000
    if ((Mypoi = vmebus(0xaa000000, 0x1000)) == NULL) {
        perror("Error opening device.\n");
        return 1;
    }
    *Mypoi = Myaddr & 0xe0000000;
    // obere 3 Bits setzen: fertig

    // free the memory again
    munmap( (void*) Mypoi, 0x1000 );

    return 0;
}

/**
 * @brief Initialize the driver
 * @return always 0
 * @note The driver can only be initialized once. Calling this function again then does nothing.
 */
int drv_init () {

    if( !_init ) {

        bzero( (void*)vu, sizeof(vu) );

        OpenVMEbus();

        _init=1;
    }

    return 0;
}

int drv_start() {


    if( _init == 0 ) {
        perror("Driver not initialized!\n");
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Add a Vuprom struct to the list.
 *    checks if TopBits of the base_address match with previously added vuproms.
 * @param base_addr The base address where the module sits (base addess of the mmap)
 * @return pointer to the new vuprom struct. =NULL if failed
 */
vuprom* AddVuprom( const u_int32_t base_addr ) {

    if( n_vuproms < MAX_VUPROMS ) {

        if( n_vuproms == 0 ) {

            global_top_bits = base_addr & 0xe0000000;
            SetTopBits( global_top_bits );

        } else {

            if ((base_addr & 0xe0000000) != global_top_bits ) {
                printf("ERROR: Top Bits mismatch between previously added vuprom and this one!\n");
                return NULL;
            }
        }

        vu[n_vuproms].base_addr = base_addr;
        vu[n_vuproms].map_addr  = base_addr & 0x1fffffff;      //mask out top bits

        const int ret = init_vuprom( &(vu[n_vuproms]) );
        if( ret != TRUE ) {
            return FALSE;
        }

        printf("New vuprom (%d) added @ %#010x, map addess %#010x\n", n_vuproms, vu[n_vuproms].base_addr, vu[n_vuproms].map_addr );

        n_vuproms++;

        return &(vu[n_vuproms-1]);

    } else {

        printf("FAILED to add vuprom @ %#010x: Max number reached (%d)!\n", base_addr, MAX_VUPROMS);
        return NULL;

    }
}
/**
 * @brief Linear search for active vuprom module with base address
 * @param base_addr the base addess of the module
 * @return ptr to the vuprom struct, or NULL if does not exist
 */
vuprom* findVuprom( const u_int32_t base_addr ) {

    int i;

    for( i=0; i<MAX_VUPROMS; ++i ) {
        if( vu[i].base_addr == base_addr )
            return &vu[i];
    }
    return NULL;
}

/**
 * @brief check if vu_scalser_addr is valid
 * @param addr ptr to the struct to check
 * @return TRUE if ok
 */
int checkAddrValid( const vu_scaler_addr* addr ) {

    if( addr->base_addr & 0x00000fff ) {
        printf("Error: vuprom base address must be 4k aligned (%#010x)\n", addr->base_addr);
        return FALSE;
    }

    if( addr->reg > MAX_SCALER_INDEX ) {
        printf("Error: register index out of range (%d)\n", addr->reg);
        return FALSE;
    }

    return TRUE;
}


/**
 * @brief add a new record to the driver.
 *   checks if the address/scaler tuple is correct, then tries to find an exsiting vuprom struct that matches the base_addr.
 *   Or creates a new one if needed.
 * @param addr ptr to the address/scaler definition
 * @return a pointer to the location where the value for this record will be stored later during operation.
 */
u_int32_t* drv_AddRegister( const vu_scaler_addr* addr ) {

    if( !checkAddrValid(addr) ) {
        return NULL;
    }

    vuprom* v = findVuprom( addr->base_addr );

    if( !v ) {

        v = AddVuprom( addr->base_addr );

        if( !v ) {
            return NULL;
        }
    }

    // set up a pointer into vme mapped memory.
    u_int32_t* val_ptr =  &(v->vme_mem[addr->reg]);

    return val_ptr;
}


/**
 * @brief Shut down driver
 * @return always 0
 */
int drv_deinit() {

    _init = 0;

    return 0;
}

/**
 * @brief Check if the driver is initialized
 * @return 1 = initialized, 0 = not
 */
int drv_isInit() {
    return _init;
}

/**
 * @brief Get a value
 * @param n Index of the value to get
 * @return the value.
 */
long drv_Get( const u_int32_t addr ) {

    const u_int32_t base_addr = 0xFFFFF000 & addr;
    const u_int32_t reg = 0xFFF & addr;

    vuprom* v = findVuprom( base_addr );

    if( v ) {
        return v->vme_mem[reg];
    } else {
        printf("ERROR reading register @ %x: no vuprom module initialized @ %#010x!\n", addr, base_addr );
        return 0;
    }

}
