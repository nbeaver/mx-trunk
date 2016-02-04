/*
 * Name:    d_soft_mce.h
 *
 * Purpose: Header file to support software emulated multichannel encoders.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SOFT_MCE_H__
#define __D_SOFT_MCE_H__

#include "mx_mce.h"

/* ===== soft mce data structure ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *motor_record;

	int32_t monitor_command;
	int32_t monitor_status;

	MX_THREAD *monitor_thread;
	MX_CONDITION_VARIABLE *monitor_thread_cv;
	MX_MUTEX *monitor_thread_mutex;
} MX_SOFT_MCE;

/* Values for 'monitor_command' and 'monitor_status' */

#define MXS_SOFT_MCE_NOT_INITIALIZED	(-1)
#define MXS_SOFT_MCE_IDLE		0
#define MXS_SOFT_MCE_START		1
#define MXS_SOFT_MCE_STOP		2
#define MXS_SOFT_MCE_CLEAR		3
#define MXS_SOFT_MCE_ACQUIRING		4

#define MXD_SOFT_MCE_STANDARD_FIELDS \
  {-1, -1, "motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_MCE, motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_soft_mce_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_soft_mce_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_mce_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_mce_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_soft_mce_open( MX_RECORD *record );

MX_API mx_status_type mxd_soft_mce_read( MX_MCE *mce );
MX_API mx_status_type mxd_soft_mce_get_current_num_values( MX_MCE *mce );
MX_API mx_status_type mxd_soft_mce_get_last_measurement_number( MX_MCE *mce );
MX_API mx_status_type mxd_soft_mce_get_status( MX_MCE *mce );
MX_API mx_status_type mxd_soft_mce_start( MX_MCE *mce );
MX_API mx_status_type mxd_soft_mce_stop( MX_MCE *mce );
MX_API mx_status_type mxd_soft_mce_clear( MX_MCE *mce );
MX_API mx_status_type mxd_soft_mce_read_measurement( MX_MCE *mce );
MX_API mx_status_type mxd_soft_mce_get_parameter( MX_MCE *mce );
MX_API mx_status_type mxd_soft_mce_set_parameter( MX_MCE *mce );

extern MX_RECORD_FUNCTION_LIST mxd_soft_mce_record_function_list;
extern MX_MCE_FUNCTION_LIST mxd_soft_mce_mce_function_list;

extern long mxd_soft_mce_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_soft_mce_rfield_def_ptr;

#endif /* __D_SOFT_MCE_H__ */

