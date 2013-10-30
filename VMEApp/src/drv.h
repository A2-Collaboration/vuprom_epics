#include <sys/types.h>
#include "dbScan.h"

typedef struct {
    u_int32_t   base_addr;
    u_int32_t   scaler;
    int         flag;
    u_int32_t   normval;
    u_int32_t   firmware;
} vu_scaler_addr;

int drv_init();
int drv_deinit();
int drv_isInit();
int drv_start();

u_int32_t* drv_AddRecord(const vu_scaler_addr* addr );

long drv_Get(const u_int32_t addr );

IOSCANPVT* drv_getioinfo();
void drv_disable_iointr();
void drv_enable_iointr();
