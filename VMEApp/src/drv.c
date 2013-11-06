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
#define MAX_SCALER_INDEX    256

// At vuprom address 0xf00 (= scaler 960) is a magic number. This is always equal to 0x87654321.
// This way we can check if vme access works and it there is a vuprom at this base address.
#define MAGIC_SCALER 960
#define MAGIC_NUMBER 0x87654321
#define FIRMWARE_SCALER 0x2f00

static int n_vuproms =0;

static u_int32_t global_top_bits =0;

// our measurement thread
static pthread_t pth;

// is the driver initialized?
static int _init = 0;

static IOSCANPVT ioinfo;
static int _iointr = 0;

// sleep time [us]
static unsigned int sleep_time = 1000000;

typedef struct {
    u_int32_t  base_addr;   // Address of vuprom
    u_int32_t  map_addr;    // mmap address (without top bits)
    volatile u_int32_t* vme_mem;     // mmaped memory ptr
    u_int32_t  values[N];   // copied values
    int  max_sclaer_index;
    int  refernece_scaler;
    u_int32_t normval;
    u_int32_t firmware;
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

    const u_int32_t magic_number = v->vme_mem[MAGIC_SCALER];

    u_int32_t firmware = 0;
    if (!vme_read_once( (v->map_addr & 0xFF000000) + FIRMWARE_SCALER, &firmware )) {
        printf("ERROR: Could not read firmware!\n");
    }

    if( magic_number != MAGIC_NUMBER ) {
        printf("Error: vuprom magic number not found. (base_addr: %#010x, scaler %d, value %x, expected %x)\n", v->base_addr, MAGIC_SCALER, magic_number, MAGIC_NUMBER );
        return FALSE;
    }

    if( firmware != v->firmware ) {
        printf("WARNING: vuprom firmware missmatch! (base_addr: %#010x, value %x, expected %x)\n", v->base_addr, firmware, v->firmware );
        return FALSE;
    }

    printf("Init vuprom @ %#010x, map addess: %#010x, max scaler index: %d\n", v->base_addr, v->map_addr, v->max_sclaer_index);
    return TRUE;
}

void deinit_vuprom( vuprom* v ) {
    if( v->vme_mem ) {
        munmap( (void*)(v->vme_mem), RANGE );
        v->vme_mem = NULL;
    }
}

void start_measurement( vuprom* v ) {
    v->vme_mem[512] = 1;
}

void stop_measurement( vuprom* v ) {
    v->vme_mem[513] = 1;
}

void save_values( vuprom* v ) {
    memcpy( &(v->values), (void*)(v->vme_mem), (v->max_sclaer_index+1) * sizeof(u_int32_t) );

    // do normalization if activated
    if( v->refernece_scaler != -1 ) {

        const double factor = (double) v->normval / (double) v->values[v->refernece_scaler];
        //printf("Rescaling vuprom @ %#010x with factor %lf\n", v->base_addr, factor);

        int i;
        for( i=0; i <= v->max_sclaer_index; ++i)
            v->values[i] = (u_int32_t) (v->values[i] * factor);

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
 * @brief Thread function that periodically reads the scaler values
 * @param arg ignore. does not take arguments
 * @return nothing.
 */
void *thread_measure( void* arg)
{

    int i;

    while( 1 )
    {

        // Clear Counters
        for( i=0; i<n_vuproms; ++i) {
            start_measurement( &(vu[i]) );
        }

        usleep( sleep_time );

        // Save Counters
        for( i=0; i<n_vuproms; ++i) {
            stop_measurement( &(vu[i]) );
        }

        // copy values
        for( i=0; i<n_vuproms; ++i) {
            save_values( &(vu[i]) );
        }

        if( _iointr )
            scanIoRequest( ioinfo );

    }

    return NULL;
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

    int i;

    if( _init == 0 ) {
        perror("Driver not initialized!\n");
        return FALSE;
    }

    if( n_vuproms == 0 ) {
        perror("No vuproms added!\n");
        return FALSE;
    }

    SetTopBits( global_top_bits );

    // initialize all vuprom structs
    for( i=0; i<n_vuproms; ++i) {
        const int ret = init_vuprom( &(vu[i]) );
        if( ret != TRUE ) {
            return FALSE;
        }
    }

    // start measuring thread
    pthread_create(&pth, NULL, thread_measure, NULL);

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

        } else {

            if ((base_addr & 0xe0000000) != global_top_bits ) {
                printf("ERROR: Top Bits mismatch between previously added vuprom and this one!\n");
                return NULL;
            }
        }

        vu[n_vuproms].base_addr = base_addr;
        vu[n_vuproms].map_addr  = base_addr & 0x1fffffff;      //mask out top bits
        vu[n_vuproms].refernece_scaler = -1;

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

    if( addr->scaler > MAX_SCALER_INDEX ) {
        printf("Error: scaler index out of range (%d)\n", addr->scaler);
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
u_int32_t* drv_AddRecord( const vu_scaler_addr* addr ) {

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

    u_int32_t* val_ptr = &(v->values[addr->scaler]);

    if( addr->scaler > v->max_sclaer_index )
        v->max_sclaer_index = addr->scaler;

    if( addr->flag == 1 ) {
        if( v->refernece_scaler == -1 ) {
            v->refernece_scaler = addr->scaler;
            v->normval = addr->normval;
            v->firmware = addr->firmware;
            printf("Setting scaler %d as reference for vuprom @ %#010x\n\tNormValue=%d\n\tFirmware=%x\n", addr->scaler, v->base_addr, v->normval,v->firmware);
        } else {
            printf("WARNING: Not setting scaler %d as reference for vuprom @ %#010x. Reference is already scaler %d!\n",addr->scaler,v->base_addr,v->refernece_scaler);
        }
    }

    return val_ptr;
}


/**
 * @brief Shut down driver
 * @return always 0
 */
int drv_deinit() {

    int res = pthread_cancel( pth );

    if( res!=0 ) {
        puts("Error shutting down thread\n");
    }

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
    const u_int32_t scaler = 0xFFF & addr;

    vuprom* v = findVuprom( base_addr );

    if( v ) {
        return v->values[scaler];
    } else {
        printf("ERROR reading scaler @ %x: no vuprom module initialized @ %#010x!\n", addr, base_addr );
        return 0;
    }

}

IOSCANPVT* drv_getioinfo() {
    return &ioinfo;
}

void drv_disable_iointr() {
    _iointr = 0;
}

void drv_enable_iointr() {
    _iointr = 1;
}
