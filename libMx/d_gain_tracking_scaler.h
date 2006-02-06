/*
 * Name:    d_gain_tracking_scaler.h
 *
 * Purpose: Header file for MX gain tracking scalers.
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 2001-2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_GAIN_TRACKING_SCALER_H__
#define __D_GAIN_TRACKING_SCALER_H__

typedef struct {
	MX_RECORD *real_scaler_record;
	MX_RECORD *amplifier_record;
	double gain_tracking_scale;
} MX_GAIN_TRACKING_SCALER;

MX_API mx_status_type mxd_gain_tracking_scaler_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_gain_tracking_scaler_open( MX_RECORD *record );
MX_API mx_status_type mxd_gain_tracking_scaler_close( MX_RECORD *record );

MX_API mx_status_type mxd_gain_tracking_scaler_clear( MX_SCALER *scaler );
MX_API mx_status_type mxd_gain_tracking_scaler_overflow_set( MX_SCALER *scaler );
MX_API mx_status_type mxd_gain_tracking_scaler_read( MX_SCALER *scaler );
MX_API mx_status_type mxd_gain_tracking_scaler_is_busy( MX_SCALER *scaler );
MX_API mx_status_type mxd_gain_tracking_scaler_start( MX_SCALER *scaler );
MX_API mx_status_type mxd_gain_tracking_scaler_stop( MX_SCALER *scaler );
MX_API mx_status_type mxd_gain_tracking_scaler_get_parameter(
							MX_SCALER *scaler );
MX_API mx_status_type mxd_gain_tracking_scaler_set_parameter(
							MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_gain_tracking_scaler_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_gain_tracking_scaler_scaler_function_list;

extern mx_length_type mxd_gain_tracking_scaler_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_gain_tracking_scaler_rfield_def_ptr;

#define MXD_GAIN_TRACKING_SCALER_STANDARD_FIELDS \
  {-1, -1, "real_scaler_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_GAIN_TRACKING_SCALER, real_scaler_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "amplifier_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_GAIN_TRACKING_SCALER, amplifier_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "gain_tracking_scale", MXFT_DOUBLE, NULL, 0, {0}, \
     MXF_REC_TYPE_STRUCT, \
		offsetof(MX_GAIN_TRACKING_SCALER, gain_tracking_scale), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_GAIN_TRACKING_SCALER_H__ */

