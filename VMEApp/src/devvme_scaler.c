/**
 * @brief A2 VME scaler read out for EPICS
 * @author Oliver Steffen <steffen@kph.uni-mainz.de>
 * @date Aug 30, 2013
 */

static char what[] =
"A2 VUPROM VME Scalers - O.Steffen <steffen@kph.uni-mainz.de>, 27. Sep 2013";

#define DEBUG_OFF

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "alarm.h"
#include "cvtTable.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "recGbl.h"
#include "recSup.h"
#include "devSup.h"
#include "longinRecord.h"
#include "link.h"
#include "epicsExport.h"
#include "dbScan.h"
#include "devvme_scaler.h"
#include <sys/types.h>

#include "drv.h"

// export our functions to EPICS:
struct {
    long		number;
    DEVSUPFUN	report;
    DEVSUPFUN	init;
    DEVSUPFUN	init_record;
    DEVSUPFUN	get_ioint_info;
    DEVSUPFUN	read_longin;
} vuprom_register = {
    6,
    NULL,
    init,
    init_register,
    NULL,
    read_register
};
epicsExportAddress(dset,vuprom_register);


static long init(int after)
{
    if( after == 0 ) {
        printf("%s\n", what);
        drv_init();
    } else if ( after == 1 ) {
        const int ret = drv_start();
        if( ret != TRUE ) {
            printf("ERROR: Failed to initialize VUPROM VME driver.");
            exit(1);
        }
    }

    return 0;
}

static int parseAddress2( char* str, vu_scaler_addr* addr) {

    int ret = sscanf( str,"register %x:%d", &(addr->base_addr), &(addr->scaler));

    if( ret == 2 ) {
        addr->flag = 2;
        addr->firmware = 0;
        return TRUE;
    }
    return FALSE;
}

static long init_register(struct longinRecord *pai)
{
    vu_scaler_addr addr;
    const int ret = parseAddress2( pai->inp.text, &addr );

    if( !ret ) {
        printf("Invalid: %#010x, register %d, for %s\n", addr.base_addr, addr.scaler, pai->name);
        return 1;
    }

    u_int32_t* ptr = drv_AddRegister(&addr);

    if( ptr ) {
        pai->dpvt = (void*) ptr;
        pai->udf = FALSE;
        return 0;
    } else
        return 1;
}

static long read_register(struct longinRecord *pai)
{
    if( pai->dpvt ) {
        pai->val = *((u_int32_t*) pai->dpvt);
        pai->udf = FALSE;
        printf("Read register\n");
        return 0;
    } else
        return 1;
}
