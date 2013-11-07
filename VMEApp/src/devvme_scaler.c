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
#include "aiRecord.h"
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
    DEVSUPFUN	read_ai;
    DEVSUPFUN	special_linconv;
} devvme_scaler = {
	6,
	NULL,
	init_ai,
	init_record,
    get_ioint_info,
	read_ai,
	NULL
};
epicsExportAddress(dset,devvme_scaler);




static long init_ai(int after)
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

static int parseAddress( char* str, vu_scaler_addr* addr) {

    u_int32_t normval=0;
    u_int32_t firmware=0;

    int ret = sscanf( str,"%x:%d R Norm=%d Firmware=%x", &(addr->base_addr), &(addr->scaler), &normval, &firmware);

    if( ret == 2 ) {
        addr->flag = 0;
        return TRUE;
    } else if( (ret == 4) ) {
        addr->flag = 1;
        addr->normval = normval;
        addr->firmware = firmware;
        return TRUE;
    }
    return FALSE;
}

static long init_record(struct aiRecord *pai)
{
    vu_scaler_addr addr;
    const int ret = parseAddress( pai->inp.text, &addr );

    if( !ret ) {
        printf("Invalid: %#010x, scaler %d, for %s\n", addr.base_addr, addr.scaler, pai->name);
        return 1;
    }

    u_int32_t* ptr = drv_AddRecord(&addr);

    if( ptr ) {
        pai->dpvt = (void*) ptr;
        pai->udf = FALSE;
        return 0;
    } else
        return 1;
}


static long read_ai(struct aiRecord *pai)
{
    if( pai->dpvt ) {
        pai->rval = *((u_int32_t*) pai->dpvt);
        pai->udf = FALSE;
        return 0;
    } else
        return 1;
}

static int n=0;

static long get_ioint_info(int cmd,struct dbCommon *precord, IOSCANPVT *ppvt) {

    IOSCANPVT* ioinfo = NULL;

    switch (cmd) {
    case 0:

        ioinfo = drv_getioinfo();

        if( ioinfo ) {
            if (n==0 ) scanIoInit(ioinfo);  //only once! not for every record!
            ++n;
            *ppvt = *ioinfo;
            drv_enable_iointr();
        } else
            puts("Error setting I/O Intr\n");
        break;
    case 1:
        drv_disable_iointr();
        break;
    default:
        printf("Error: unknown command %d\n", cmd);
    }

    return 0;
}
