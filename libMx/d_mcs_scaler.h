/*
 * Name:    d_mcs_scaler.h
 *
 * Purpose: Header file for MX scaler driver to control MCS channels
 *          as individual scalers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2002, 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MCS_SCALER_H__
#define __D_MCS_SCALER_H__

#include "mx_scaler.h"

/* ===== MCS scaler data structure ===== */

typedef struct {
	MX_RECORD *mcs_record;
	int scaler_number;
} MX_MCS_SCALER;

#define MXD_MCS_SCALER_STANDARD_FIELDS \
  {-1, -1, "mcs_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCS_SCALER, mcs_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "scaler_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCS_SCALER, scaler_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_mcs_scaler_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_mcs_scaler_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_mcs_scaler_print_structure( FILE *file,
							MX_RECORD *record );

MX_API mx_status_type mxd_mcs_scaler_clear( MX_SCALER *scaler );
MX_API mx_status_type mxd_mcs_scaler_read( MX_SCALER *scaler );
MX_API mx_status_type mxd_mcs_scaler_read_raw( MX_SCALER *scaler );
MX_API mx_status_type mxd_mcs_scaler_is_busy( MX_SCALER *scaler );
MX_API mx_status_type mxd_mcs_scaler_start( MX_SCALER *scaler );
MX_API mx_status_type mxd_mcs_scaler_stop( MX_SCALER *scaler );
MX_API mx_status_type mxd_mcs_scaler_get_parameter( MX_SCALER *scaler );
MX_API mx_status_type mxd_mcs_scaler_set_parameter( MX_SCALER *scaler );
MX_API mx_status_type mxd_mcs_scaler_set_modes_of_associated_counters(
							MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_mcs_scaler_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_mcs_scaler_scaler_function_list;

extern long mxd_mcs_scaler_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mcs_scaler_rfield_def_ptr;

#endif /* __D_MCS_SCALER_H__ */

