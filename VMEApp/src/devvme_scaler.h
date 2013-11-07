/**
 * @file devvme_scaler.h
 * @brief A2 VUPROM VME scaler readout for EPICS
 * @author Oliver Steffen <steffen@kph.uni-mainz.de>
 * @date Oct, 2013
 */

// Interface to EPICS:

/**
 * @brief Initialize a record
 * @param pai ptr to the struct containing the record info
 * @return 0=FAIL, 1=SUCCESS
 */
static long init_record( struct aiRecord *pai );

/**
 * @brief init_ai
 * @return
 */
static long init_ai();

/**
 * @brief read_ai
 * @return
 */
static long read_ai();

/**
 * @brief get_ioint_info
 * @return
 */
static long get_ioint_info();
