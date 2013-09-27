#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "drv.h"
#include <time.h>
#include <sys/mman.h>
#include <string.h>  // for memcpy()

#define TRUE 1
#define FALSE 0


#include "vmebus.h"

// size of memory range to map
#define RANGE 0x1000

// number of 32 bit values in memory range
#define N     (RANGE / sizeof(u_int32_t))

#define VME_ADDR       0x1000000
#define VME_ADDR_DATA  0x1000000 * 10 + 0x3000

#define MAX_VUPROMS 8
static int n_vuproms =0;

// our measurement thread
static pthread_t pth;

// is the driver initialized?
static int _init = 0;

static IOSCANPVT ioinfo;
static int _iointr = 0;

// sleep time [us]
static unsigned int sleep_time = 1000000;

typedef struct timespec timespec;


float last_sleep = 0.0f;


typedef struct {
    u_int32_t  base_addr;   // Address of vuprom to mmap
    volatile u_int32_t* vme_mem;     // mmaped memory ptr
    u_int32_t  values[N];   // copied values
} vuprom;

int init_vuprom( vuprom* v ) {

    if ((v->vme_mem = (u_int32_t*) vmeext( v->base_addr, RANGE )) == NULL) {
        perror("Error opening device.\n");
        return 0;
    }
    printf("Init vuprom @ %x\n", v->base_addr);
    return 1;
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
    memcpy( &(v->values), (void*)(v->vme_mem), RANGE );
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
 * @brief Calculate the differnce in seconds of two timespec values
 * @param start The first (earlier) time point
 * @param stop The later time point
 * @return difference in seconds
 */
float time_difference( const timespec* start, const timespec* stop ) {
    float time = stop->tv_sec - start->tv_sec + (stop->tv_nsec - start->tv_nsec) * 1E-9;
    return time;
}


/**
 * @brief Thread function that periodically reads the scaler values
 * @param arg ignore. does not take arguments
 * @return nothing.
 */
void *thread_measure( void* arg)
{

    timespec start_measure;
    timespec stop_measure;
    int i;

    while( 1 )
    {
        clock_gettime(CLOCK_MONOTONIC, &start_measure);

        // Clear Counters
        //VMEWrite(VME_ADDR_DATA+0x3800,1);
        //vmemem[512] = 1;

        for( i=0; i<n_vuproms; ++i) {
            start_measurement( &(vu[i]));
        }

        usleep( sleep_time );

        // Save Counters
        //VMEWrite(VME_ADDR+0x3804,1);
        //vmemem[513] = 1;
        for( i=0; i<n_vuproms; ++i) {
            stop_measurement( &(vu[i]));
        }

        clock_gettime(CLOCK_MONOTONIC, &stop_measure);

        //int k;
        //for (k = 0; k<N; k++)
         //      values[k] = VMERead(VME_ADDR+0x3000+k*4);
        // vmemem starts as VME_ADDR_DATA+0x3000
        //memcpy( &values, (void*)vmemem, RANGE );
        for( i=0; i<n_vuproms; ++i) {
            save_values( &(vu[i]));
        }

        last_sleep = time_difference( &start_measure, &stop_measure );

        //printf("threadFunc: sleep was: %f s\n", last_sleep );
        //
        // since we measured the elapsed time we could scale all values to be "per second"...
        // or export the time via epics and d othe calc there?

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

        SetTopBits(VME_ADDR);

        _init=1;
    }

    return 0;
}

int drv_start() {
    int i;

    if( _init == 0 ) {
        perror("Driver not initialized!\n");
        return 1;
    }

    for( i=0; i<n_vuproms; ++i) {
        init_vuprom( &(vu[i]));
    }

    // start measuring thread
    pthread_create(&pth, NULL, thread_measure, NULL);

    return 0;
}

vuprom* AddVuprom( const u_int32_t base_addr ) {

    if( n_vuproms < MAX_VUPROMS ) {

        vu[n_vuproms].base_addr = base_addr;
        printf("New vuprom (%d) added @ %x\n", n_vuproms, base_addr );
        n_vuproms++;
        return &(vu[n_vuproms-1]);

    } else {

        printf("FAILED to add vuprom @ %x: Max number reached (%d)!\n", base_addr, MAX_VUPROMS);
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

u_int32_t* drv_AddRecord( const u_int32_t addr ) {

    const u_int32_t base_addr = 0xFFFFF000 & addr;
    const u_int32_t scaler = 0xFFF & addr;

    vuprom* v = findVuprom( base_addr );

    if( !v ) {

        v = AddVuprom( base_addr );

        if( !v ) {
            return FALSE;
        }
    }

    u_int32_t* val_ptr = &(v->values[scaler]);

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
        printf("ERROR reading scaler @ %x: no vuprom module initialized @ %x!\n", addr, base_addr );
        return 0;
    }

}

float drv_GetLastInterval() {
    return last_sleep;
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
