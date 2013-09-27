#include <sys/types.h>
#include "dbScan.h"

int drv_init();
int drv_deinit();
int drv_isInit();
int drv_start();

u_int32_t* drv_AddRecord( const u_int32_t addr );

long drv_Get(const u_int32_t addr );

float drv_GetLastInterval();

IOSCANPVT* drv_getioinfo();
void drv_disable_iointr();
void drv_enable_iointr();
