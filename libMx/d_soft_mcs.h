/*
 * Name:    d_soft_mcs.h 
 *
 * Purpose: Header file to support software emulated multichannel scalers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2004, 2010, 2016, 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SOFT_MCS_H__
#define __D_SOFT_MCS_H__

#include "mx_mcs.h"

/* ===== MX soft mcs record data structures ===== */

typedef struct {
	double starting_position;
	double delta_position;
	MX_RECORD **soft_scaler_record_array;
	MX_CLOCK_TICK finish_time_in_clock_ticks;
} MX_SOFT_MCS;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_soft_mcs_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_soft_mcs_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_mcs_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_mcs_open( MX_RECORD *record );

MX_API mx_status_type mxd_soft_mcs_arm( MX_MCS *mcs );
MX_API mx_status_type mxd_soft_mcs_stop( MX_MCS *mcs );
MX_API mx_status_type mxd_soft_mcs_clear( MX_MCS *mcs );
MX_API mx_status_type mxd_soft_mcs_busy( MX_MCS *mcs );
MX_API mx_status_type mxd_soft_mcs_read_all( MX_MCS *mcs );
MX_API mx_status_type mxd_soft_mcs_read_scaler( MX_MCS *mcs );
MX_API mx_status_type mxd_soft_mcs_read_measurement( MX_MCS *mcs );
MX_API mx_status_type mxd_soft_mcs_get_parameter( MX_MCS *mcs );
MX_API mx_status_type mxd_soft_mcs_set_parameter( MX_MCS *mcs );

extern MX_RECORD_FUNCTION_LIST mxd_soft_mcs_record_function_list;
extern MX_MCS_FUNCTION_LIST mxd_soft_mcs_mcs_function_list;

extern long mxd_soft_mcs_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_soft_mcs_rfield_def_ptr;

#define MXD_SOFT_MCS_STANDARD_FIELDS \
  {-1, -1, "starting_position", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_MCS, starting_position), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "delta_position", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_MCS, delta_position), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "soft_scaler_record_array", \
				MXFT_RECORD, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_MCS, soft_scaler_record_array), \
	{sizeof(MX_RECORD *)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY|MXFF_VARARGS)}

#endif /* __D_SOFT_MCS_H__ */

