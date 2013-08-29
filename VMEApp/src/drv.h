#include "dbScan.h"

int drv_init();
int drv_deinit();
int drv_isInit();

long drv_Get( const unsigned int n );

float drv_GetLastInterval();

IOSCANPVT* drv_getioinfo();
