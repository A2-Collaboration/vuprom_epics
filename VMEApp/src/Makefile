TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================
# Build an IOC support library


LIBRARY_IOC += vme_scaler

DBD += vme_scaler.dbd
vme_scaler_SRCS += devvme_scaler.c
vme_scaler_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================
# Build the IOC application

PROD_IOC = VME
# VME.dbd will be created and installed
DBD += VME.dbd

# VME.dbd will be made up from these files:
VME_DBD += base.dbd

# Include dbd files from all support applications:
VME_DBD += vme_scaler.dbd

# Add all the support libraries needed by this IOC
#VME_LIBS += ccc

# VME_registerRecordDeviceDriver.cpp derives from VME.dbd
VME_SRCS += VME_registerRecordDeviceDriver.cpp

VME_SRCS += vmebus.c drv.c

# Build the main IOC entry point on workstation OSs.
VME_SRCS_DEFAULT += VMEMain.cpp
VME_SRCS_vxWorks += -nil-

#The following adds support from base/src/vxWorks
#VME_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

#add a definition for each support application used by this application
VME_LIBS += vme_scaler

#NOTES:
#      1)It is not possible to build sncExample both as a component of VME
#        and standalone. You must choose only one.
#      2)To build sncExample SNCSEQ must be defined in <top>/configure/RELEASE

#The following builds sncExample as a component of VME
#VME_SRCS += sncExample.stt
#VME_LIBS += seq pv

# Finally link to the EPICS Base libraries
VME_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

STATIC_LDFLAGS += -pthread
