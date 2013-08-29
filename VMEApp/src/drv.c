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

static volatile u_int32_t* vmemem = NULL;

static pthread_t pth;

// is the driver initialized?
static int _init=0;

// sleep time [us]
static unsigned int sleep_time = 1000000;

typedef struct timespec timespec;

timespec start_measure;
timespec stop_measure;
float last_sleep = 0.0f;


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


/*

unsigned long VMERead(unsigned long Myaddr) {
    unsigned long Myrest, MyOrigAddr, Mypattern;
    volatile unsigned long *Mypoi;
    MyOrigAddr = Myaddr;
    Myaddr &= 0x1fffffff;	// obere Bits ausmaskieren
    Myrest = Myaddr % 0x1000;
    Myaddr = (Myaddr / 0x1000) * 0x1000;

    if ((Mypoi = vmeext(Myaddr, 0x1000)) == NULL) {
        perror("Error opening device.\n");
        return 0;
    }
    Mypoi += (Myrest / 4);  //da 32Bit-Zugriff

    //Lesen
    Mypattern = *Mypoi;

    return Mypattern;
}

unsigned long VMEWrite(unsigned long Myaddr, unsigned long Mypattern) {
    unsigned long MyOrigAddr, Myrest;
    volatile unsigned long *Mypoi;
    MyOrigAddr = Myaddr;
    Myaddr &= 0x1fffffff;	// obere Bits ausmaskieren
    Myrest = Myaddr % 0x1000;
    Myaddr = (Myaddr / 0x1000) * 0x1000;

    if ((Mypoi = vmeext(Myaddr, 0x1000)) == NULL) {
        perror("Error opening device.\n");
        return 0;
    }
    Mypoi += (Myrest / 4);  //da 32Bit-Zugriff

    *Mypoi = Mypattern;  //Doppelter Zugriff beim Schreiben


    return 0;
}
*/

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

void *threadFunc(void * arg)
{

    while( 1 )
    {
        clock_gettime(CLOCK_MONOTONIC, &start_measure);

        // Clear Counters
        //VMEWrite(VME_ADDR_DATA+0x3800,1);
        vmemem[512] = 1;

        usleep( sleep_time );

        // Save Counters
        //VMEWrite(VME_ADDR+0x3804,1);
        vmemem[513] = 1;

        clock_gettime(CLOCK_MONOTONIC, &stop_measure);

        //int k;
        //for (k = 0; k<N; k++)
         //      values[k] = VMERead(VME_ADDR+0x3000+k*4);
        // vmemem starts as VME_ADDR_DATA+0x3000
        memcpy( &values, (void*)vmemem, RANGE );

        last_sleep = time_difference( &start_measure, &stop_measure );

 //       printf("threadFunc: sleep was: %f s\n", last_sleep );

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

        if ((vmemem = (u_int32_t*) vmeext( VME_ADDR_DATA, RANGE )) == NULL) {
            perror("Error opening device.\n");
            return 0;
        }

        pthread_create(&pth, NULL, threadFunc, NULL);
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

    //... reset VME registers

    if( vmemem ) {
        munmap( (void*)vmemem, RANGE );
        vmemem = NULL;
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
long drv_Get( const unsigned int n ) {
    if( n < N ) {
        return values[n];
    } else {
        puts("ERROR! Index out of range!\n");
    }
    return 0;
}

float drv_GetLastInterval() {
    return last_sleep;
}

