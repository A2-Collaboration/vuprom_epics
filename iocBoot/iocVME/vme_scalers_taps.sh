#/bin/bash -e

cd /opt/epics/apps/vme/iocBoot/iocVME/

nice -n 10 ../../bin/linux-x86/VME taps_scalers.cmd
