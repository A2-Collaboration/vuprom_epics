#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "drv.h"
#include <time.h>
#include <sys/mman.h>
#include <string.h>  // for memcpy()



#include "vmebus.h"

// size of memory range to map
#define RANGE 0x1000

// number of 32 bit values in memory range
#define N     (RANGE / sizeof(u_int32_t))

#define VME_ADDR       0x1000000
#define VME_ADDR_DATA  0x1000000 * 10 + 0x3000

#define NumberOfLeftChannels 10
#define NumberOfPairsPerLeftCh 5

// local storage for values
static volatile u_int32_t values[N];

// memory to map VME to
//static volatile u_int32_t* vmemem = NULL;

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


static vuprom vu;


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

    while( 1 )
    {
        clock_gettime(CLOCK_MONOTONIC, &start_measure);

        // Clear Counters
        //VMEWrite(VME_ADDR_DATA+0x3800,1);
        //vmemem[512] = 1;
        start_measurement( &vu );

        usleep( sleep_time );

        // Save Counters
        //VMEWrite(VME_ADDR+0x3804,1);
        //vmemem[513] = 1;
        stop_measurement( &vu );

        clock_gettime(CLOCK_MONOTONIC, &stop_measure);

        //int k;
        //for (k = 0; k<N; k++)
         //      values[k] = VMERead(VME_ADDR+0x3000+k*4);
        // vmemem starts as VME_ADDR_DATA+0x3000
        //memcpy( &values, (void*)vmemem, RANGE );
        save_values( &vu );

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

        OpenVMEbus();

        SetTopBits(VME_ADDR);
/*
        if ((vmemem = (u_int32_t*) vmeext( VME_ADDR_DATA, RANGE )) == NULL) {
            perror("Error opening device.\n");
            return 0;
        }
*/
        vu.base_addr = VME_ADDR_DATA;
        init_vuprom( &vu );

        // start measuring thread
        pthread_create(&pth, NULL, thread_measure, NULL);
        printf("Initialized VME Memory range %x..%x\n",VME_ADDR_DATA,VME_ADDR_DATA+RANGE);

        _init=1;
    }

    return 0;
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

    //... reset VME registers?
/*
    if( vmemem ) {
        munmap( (void*)vmemem, RANGE );
        vmemem = NULL;
    }
*/
    deinit_vuprom( &vu );
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
long drv_Get( const unsigned int n ) {
    if( n < N ) {
        return vu.values[n];
    } else {
        puts("ERROR! Index out of range!\n");
    }
    return 0;
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
