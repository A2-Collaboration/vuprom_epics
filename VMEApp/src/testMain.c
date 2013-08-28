#include <stdio.h>
#include <unistd.h>
#include "drv.h"


int main( int argc, char** argv ) {

    drv_init();

    while(1) {
        sleep(5);
        printf("Main:%ld", drv_Get(0) );
    }

    return 0;
}
