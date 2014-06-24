#include <sys/types.h>
#include "dbScan.h"

typedef struct {
    u_int32_t   base_addr;  // Base address of VUPROM
    u_int32_t   scaler;     // scaler/register number (of 4byte words after base_addr)
} vu_scaler_addr;

int drv_init();
int drv_deinit();
int drv_isInit();
int drv_start();

u_int32_t* drv_AddRegister( const vu_scaler_addr* addr );

long drv_Get(const u_int32_t addr );
