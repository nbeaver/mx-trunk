/*
 * Name:    d_scipe_scaler.h
 *
 * Purpose: Header file for MX scaler driver to control SCIPE scalers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2002, 2010, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SCIPE_SCALER_H__
#define __D_SCIPE_SCALER_H__

#include "mx_scaler.h"

/* ===== SCIPE scaler data structure ===== */

typedef struct {
	MX_RECORD *scipe_server_record;

	char scipe_scaler_name[MX_SCIPE_OBJECT_NAME_LENGTH+1];
} MX_SCIPE_SCALER;

#define MXD_SCIPE_SCALER_STANDARD_FIELDS \
  {-1, -1, "scipe_server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_SCALER, scipe_server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "scipe_scaler_name", MXFT_STRING, NULL, \
					1, {MX_SCIPE_OBJECT_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_SCALER, scipe_scaler_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_scipe_scaler_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_scipe_scaler_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_scipe_scaler_open( MX_RECORD *record );
MX_API mx_status_type mxd_scipe_scaler_close( MX_RECORD *record );

MX_API mx_status_type mxd_scipe_scaler_clear( MX_SCALER *scaler );
MX_API mx_status_type mxd_scipe_scaler_overflow_set( MX_SCALER *scaler );
MX_API mx_status_type mxd_scipe_scaler_read( MX_SCALER *scaler );
MX_API mx_status_type mxd_scipe_scaler_is_busy( MX_SCALER *scaler );
MX_API mx_status_type mxd_scipe_scaler_start( MX_SCALER *scaler );
MX_API mx_status_type mxd_scipe_scaler_stop( MX_SCALER *scaler );
MX_API mx_status_type mxd_scipe_scaler_get_parameter( MX_SCALER *scaler );
MX_API mx_status_type mxd_scipe_scaler_set_parameter( MX_SCALER *scaler );
MX_API mx_status_type mxd_scipe_scaler_set_modes_of_associated_counters(
							MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_scipe_scaler_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_scipe_scaler_scaler_function_list;

extern long mxd_scipe_scaler_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_scipe_scaler_rfield_def_ptr;

#endif /* __D_SCIPE_SCALER_H__ */

