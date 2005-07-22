/*
 * Name:    d_databox_scaler.h
 *
 * Purpose: Header file for MX scaler driver to control the Databox scaler.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_DATABOX_SCALER_H__
#define __D_DATABOX_SCALER_H__

#include "mx_scaler.h"

#define MX_DATABOX_SCALER_MAX_READOUT_LINES	20

/* ===== DATABOX scaler data structure ===== */

typedef struct {
	MX_RECORD *databox_record;
} MX_DATABOX_SCALER;

#define MXD_DATABOX_SCALER_STANDARD_FIELDS \
  {-1, -1, "databox_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DATABOX_SCALER, databox_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_databox_scaler_initialize_type( long type );
MX_API mx_status_type mxd_databox_scaler_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_databox_scaler_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_databox_scaler_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_databox_scaler_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_databox_scaler_read_parms_from_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_databox_scaler_write_parms_to_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_databox_scaler_open( MX_RECORD *record );
MX_API mx_status_type mxd_databox_scaler_close( MX_RECORD *record );

MX_API mx_status_type mxd_databox_scaler_clear( MX_SCALER *scaler );
MX_API mx_status_type mxd_databox_scaler_overflow_set( MX_SCALER *scaler );
MX_API mx_status_type mxd_databox_scaler_read( MX_SCALER *scaler );
MX_API mx_status_type mxd_databox_scaler_is_busy( MX_SCALER *scaler );
MX_API mx_status_type mxd_databox_scaler_start( MX_SCALER *scaler );
MX_API mx_status_type mxd_databox_scaler_stop( MX_SCALER *scaler );
MX_API mx_status_type mxd_databox_scaler_get_parameter( MX_SCALER *scaler );
MX_API mx_status_type mxd_databox_scaler_set_parameter( MX_SCALER *scaler );
MX_API mx_status_type mxd_databox_scaler_set_modes_of_associated_counters(
							MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_databox_scaler_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_databox_scaler_scaler_function_list;

extern long mxd_databox_scaler_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_databox_scaler_rfield_def_ptr;

#endif /* __D_DATABOX_SCALER_H__ */

