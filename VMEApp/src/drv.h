#include "dbScan.h"

int drv_init();
int drv_deinit();
int drv_isInit();

long drv_Get( const unsigned int n );

float drv_GetLastInterval();

IOSCANPVT* drv_getioinfo();
void drv_disable_iointr();
void drv_enable_iointr();
