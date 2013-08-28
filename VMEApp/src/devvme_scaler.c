/**
  devvme_scaler.c
  kazuro furukawa, dec.14.2006.

 **/
/*NOTUSED*/
static char what[] =
"@(#)devvme_scaler v0.1 support for Second, k.furukawa, dec.2006";

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
#include "devvme_scaler.h"

#include "drv.h"

// @todo deinit driver?

/* Create the dset for devvme_second */
static long init_record();
static long init_ai();
static long read_ai();
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
    NULL,
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

//volatile static long values[N];

static long init_ai(int after)
{
    printf("%s: devvme_scaler (init) called, pass=%d\n", what, after);
    //memset(values, 0, N*sizeof(long));

    drv_init();

    return(0);
}

static int parseRecNumber( char* str ) {
    int addr;
    int ret = sscanf( str,"Value:%d", &addr);

    if( ret == 1 )
        return addr;
    else
        return -1;
}

static long init_record(struct aiRecord *pai)
{

    printf("devvme_scaler (init_record) called:%s\n", pai->inp.text);


    int record = parseRecNumber( pai->inp.text );

    if( record < 0 ) {
        printf("Error initializing Record: Unknown: %s\n", pai->inp.text);
        return 1;
    } else {
        if( record < N ) {
            pai->udf = FALSE;
            printf("Record %d init OK\n", record);
        } else {
            printf("Error: invalid record number %d\n", record);
            return 1;
        }
    }

    return(0);
}


static long read_ai(struct aiRecord *pai)
{
    int record = parseRecNumber(pai->inp.text);

    long val = drv_Get(record);

    //printf("access: %d = %ld\n", record, val);

    pai->udf = FALSE;
    pai->rval = val;

    return 0;
}
