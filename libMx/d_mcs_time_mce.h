/*
 * Name:    d_mcs_time_mce.h
 *
 * Purpose: Header file for MX mce driver to report back the elapsed time as
 *          a multichannel encoder.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002-2003, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MCS_TIME_MCE_H__
#define __D_MCS_TIME_MCE_H__

#include "mx_mce.h"

/* ===== MCS time mce data structure ===== */

typedef struct {
	MX_RECORD *associated_motor_record;

	MX_RECORD *mcs_record;
} MX_MCS_TIME_MCE;

#define MXD_MCS_TIME_MCE_STANDARD_FIELDS \
  {-1, -1, "associated_motor_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, \
                offsetof(MX_MCS_TIME_MCE, associated_motor_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "mcs_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCS_TIME_MCE, mcs_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_mcs_time_mce_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_mcs_time_mce_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_mcs_time_mce_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_mcs_time_mce_read( MX_MCE *mce );
MX_API mx_status_type mxd_mcs_time_mce_get_current_num_values( MX_MCE *mce );

extern MX_RECORD_FUNCTION_LIST mxd_mcs_time_mce_record_function_list;
extern MX_MCE_FUNCTION_LIST mxd_mcs_time_mce_mce_function_list;

extern long mxd_mcs_time_mce_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mcs_time_mce_rfield_def_ptr;

#endif /* __D_MCS_TIME_MCE_H__ */

