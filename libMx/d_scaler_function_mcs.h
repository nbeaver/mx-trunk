/*
 * Name:    d_scaler_function_mcs.h 
 *
 * Purpose: Header file to support MX scaler function
 *          pseudo multichannel scalers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SCALER_FUNCTION_MCS_H__
#define __D_SCALER_FUNCTION_MCS_H__

#include "mx_mcs.h"

/* ===== MX scaler function mcs record data structures ===== */

typedef struct {
	MX_RECORD *scaler_function_record;

	long num_scalers;
	MX_RECORD **scaler_mcs_record_array;
	int *scaler_index_array;

	long num_mcs;
	MX_RECORD **mcs_record_array;
} MX_SCALER_FUNCTION_MCS;

#define MXD_SCALER_FUNCTION_MCS_STANDARD_FIELDS \
  {-1, -1, "scaler_function_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_SCALER_FUNCTION_MCS, scaler_function_record ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_scaler_function_mcs_initialize_type(long record_type);
MX_API mx_status_type mxd_scaler_function_mcs_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_scaler_function_mcs_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_scaler_function_mcs_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_scaler_function_mcs_print_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_scaler_function_mcs_open( MX_RECORD *record );

MX_API mx_status_type mxd_scaler_function_mcs_start( MX_MCS *mcs );
MX_API mx_status_type mxd_scaler_function_mcs_stop( MX_MCS *mcs );
MX_API mx_status_type mxd_scaler_function_mcs_clear( MX_MCS *mcs );
MX_API mx_status_type mxd_scaler_function_mcs_busy( MX_MCS *mcs );
MX_API mx_status_type mxd_scaler_function_mcs_read_all( MX_MCS *mcs );
MX_API mx_status_type mxd_scaler_function_mcs_read_scaler( MX_MCS *mcs );
MX_API mx_status_type mxd_scaler_function_mcs_read_measurement( MX_MCS *mcs );
MX_API mx_status_type mxd_scaler_function_mcs_read_timer( MX_MCS *mcs );
MX_API mx_status_type mxd_scaler_function_mcs_get_parameter( MX_MCS *mcs );
MX_API mx_status_type mxd_scaler_function_mcs_set_parameter( MX_MCS *mcs );

extern MX_RECORD_FUNCTION_LIST mxd_scaler_function_mcs_record_function_list;
extern MX_MCS_FUNCTION_LIST mxd_scaler_function_mcs_mcs_function_list;

extern mx_length_type mxd_scaler_function_mcs_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_scaler_function_mcs_rfield_def_ptr;

#endif /* __D_SCALER_FUNCTION_MCS_H__ */

