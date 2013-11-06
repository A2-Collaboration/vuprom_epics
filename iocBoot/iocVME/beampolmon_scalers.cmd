#!../../bin/linux-x86/VME

## You may have to change VME to something else
## everywhere it appears in this file

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/VME.dbd"
VME_registerRecordDeviceDriver pdbbase

## Load record instances
dbLoadRecords("db/vuprom_beampolmon_x01.db")
dbLoadRecords("db/vuprom_beampolmon_x02.db")
dbLoadRecords("db/vuprom_beampolmon_x03.db")
dbLoadRecords("db/vuprom_beampolmon_x04.db")
dbLoadRecords("db/calc_beampolmon.db")

cd ${TOP}/iocBoot/${IOC}
iocInit

# Dump record list
dbl > "/tmp/ioc/vuprom_beampolmon"

