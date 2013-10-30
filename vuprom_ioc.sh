#!/bin/bash

HOSTNAME=$(hostname -s)
case $HOSTNAME in
    vme-exptrigger)
        CMDFILE="exptrigger_scalers.cmd"
        ;;    
    vme-beampolmon)
        CMDFILE="beampolmon_scalers.cmd"
        ;;
    vme-taps-trigger)
        CMDFILE="taps-trigger_scalers.cmd"
        ;;
    *)
        # silently exit
        echo "Skipping this host, does not have VUPROMs"
        exit 0
esac


# setup the EPICS environment
. /opt/epics/thisEPICS.sh

# launch the screen
cd /opt/epics/apps/vme/iocBoot/iocVME
CMDFILE=$CMDFILE screen -dm -S VUPROM -c ../../screenrc

echo "Screen session with name VUPROM started."
