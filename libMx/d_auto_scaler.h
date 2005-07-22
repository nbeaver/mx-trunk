/*
 * Name:    d_auto_scaler.h
 *
 * Purpose: Header file for autoscaling scalers.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 2001-2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_AUTO_SCALER_H__
#define __D_AUTO_SCALER_H__

#define MXF_AUTOSCALE_SCALER_NORMALIZE	1

typedef struct {
	MX_RECORD *autoscale_record;
	unsigned long autoscale_flags;
	double factor;
} MX_AUTOSCALE_SCALER;

MX_API mx_status_type mxd_autoscale_scaler_initialize_type( long type );
MX_API mx_status_type mxd_autoscale_scaler_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_autoscale_scaler_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_autoscale_scaler_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_autoscale_scaler_read_parms_from_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_autoscale_scaler_write_parms_to_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_autoscale_scaler_open( MX_RECORD *record );
MX_API mx_status_type mxd_autoscale_scaler_close( MX_RECORD *record );

MX_API mx_status_type mxd_autoscale_scaler_clear( MX_SCALER *scaler );
MX_API mx_status_type mxd_autoscale_scaler_overflow_set( MX_SCALER *scaler );
MX_API mx_status_type mxd_autoscale_scaler_read( MX_SCALER *scaler );
MX_API mx_status_type mxd_autoscale_scaler_is_busy( MX_SCALER *scaler );
MX_API mx_status_type mxd_autoscale_scaler_start( MX_SCALER *scaler );
MX_API mx_status_type mxd_autoscale_scaler_stop( MX_SCALER *scaler );
MX_API mx_status_type mxd_autoscale_scaler_get_parameter( MX_SCALER *scaler );
MX_API mx_status_type mxd_autoscale_scaler_set_parameter( MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_autoscale_scaler_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_autoscale_scaler_scaler_function_list;

extern long mxd_autoscale_scaler_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_autoscale_scaler_rfield_def_ptr;

#define MXD_AUTOSCALE_SCALER_STANDARD_FIELDS \
  {-1, -1, "autoscale_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AUTOSCALE_SCALER, autoscale_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "autoscale_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AUTOSCALE_SCALER, autoscale_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "factor", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AUTOSCALE_SCALER, factor), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_AUTO_SCALER_H__ */
