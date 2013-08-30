/**
 * @brief A2 VME scaler read out for EPICS
 * @author Oliver Steffen <steffen@kph.uni-mainz.de>
 * @date Aug 30, 2013
 */

static char what[] =
"devvme_scaler v0.1 - A2 VME Access by o.Steffen <steffen@kph.uni-mainz,de>, Aug 30, 2013";

#define DEBUG_ON

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

#include "drv.h"

// @todo deinit driver?

/* Create the dset for devvme_second */
static long init_record();
static long init_ai();
static long read_ai();
static long get_ioint_info();

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


/************************************************************************/
/* vme_scaler Record								*/
/*  INP = "hostname-or-ipaddress:data-number"				*/
/************************************************************************/

/* init_ai for debug */

#define N 64

static long init_ai(int after)
{
    if( after == 0 ) {
        printf("%s: devvme_scaler (init) called, pass=%d\n", what, after);
        drv_init();
    }

    return(0);
}

static int parseRecNumber( char* str ) {
    int addr;
    int ret = sscanf( str,"Addr:%d", &addr);

    if( ret == 1 )
        return addr;
    else
        return -1;
}

static long init_record(struct aiRecord *pai)
{

    printf("Initializing %s...", pai->name);


    int record = parseRecNumber( pai->inp.text );

    if( record < 0 ) {
        printf("Error in INP: \"%s\"\n", pai->inp.text);
        return 1;
    } else {
        if( record < N ) {
            pai->udf = FALSE;
            printf("OK, Addr=%d\n", record);
        } else {
            printf("Error: Addr out of range: %d\n", record);
            return 1;
        }
    }

    return(0);
}


static long read_ai(struct aiRecord *pai)
{
    int record = parseRecNumber(pai->inp.text);

    long val = drv_Get(record);

    pai->udf = FALSE;
    pai->rval = val;

    return 0;
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
            printf("I/O Intr enabled for %s\n", precord->name);
        } else
            puts("Error settinf I/O Intr\n");
        break;
    case 1:
        drv_disable_iointr();
        printf("I/O Intr disabled\n");
        break;
    default:
        printf("Error: unknown command %d\n", cmd);
    }

    return 0;
}
