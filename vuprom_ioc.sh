#!/bin/bash

HOSTNAME=$(hostname -s)
case $HOSTNAME in
    vme-taps-trigger)
        CMDFILE="taps_scalers.cmd"
        ;;
    *)
        # silently exit
        echo "Skipping this host, does not have VUPROMs"
        exit 0
esac


# setup the EPICS environment
. /opt/epics/thisEPICS.sh

# launch the screen
cd /opt/epics/apps/regioc/iocBoot/iocVME
CMDFILE=$CMDFILE screen -dm -S VUPROM2 -c ../../screenrc

echo "Screen session with name VUPROM2 started."
