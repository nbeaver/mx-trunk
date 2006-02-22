/*
 * Name:    d_soft_scaler.h
 *
 * Purpose: Header file for software emulated scaler.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SOFT_SCALER_H__
#define __D_SOFT_SCALER_H__

typedef struct {
	MX_RECORD *motor_record;
	unsigned long num_datapoints;
	char datafile_name[ MXU_FILENAME_LENGTH + 1 ];
	long num_intensity_modifiers;
	MX_RECORD **intensity_modifier_array;

	double *motor_position;
	double *scaler_value;
} MX_SOFT_SCALER;

MX_API mx_status_type mxd_soft_scaler_initialize_type( long type );
MX_API mx_status_type mxd_soft_scaler_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_scaler_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_scaler_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_soft_scaler_open( MX_RECORD *record );
MX_API mx_status_type mxd_soft_scaler_close( MX_RECORD *record );

MX_API mx_status_type mxd_soft_scaler_clear( MX_SCALER *scaler );
MX_API mx_status_type mxd_soft_scaler_overflow_set( MX_SCALER *scaler );
MX_API mx_status_type mxd_soft_scaler_read( MX_SCALER *scaler );
MX_API mx_status_type mxd_soft_scaler_is_busy( MX_SCALER *scaler );
MX_API mx_status_type mxd_soft_scaler_start( MX_SCALER *scaler );
MX_API mx_status_type mxd_soft_scaler_stop( MX_SCALER *scaler );

/* The following is a semi-private function which is also used
 * by the 'soft_mcs' driver.
 */

MX_API double mxd_soft_scaler_get_value_from_position(
						MX_SOFT_SCALER *soft_scaler,
						double position );

extern MX_RECORD_FUNCTION_LIST mxd_soft_scaler_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_soft_scaler_scaler_function_list;

extern long mxd_soft_scaler_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_soft_scaler_rfield_def_ptr;

#define MXD_SOFT_SCALER_STANDARD_FIELDS \
  {-1, -1, "motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_SCALER, motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_datapoints", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_SCALER, num_datapoints), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "datafile_name", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_SCALER, datafile_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_intensity_modifiers", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_SOFT_SCALER, num_intensity_modifiers), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "intensity_modifier_array", MXFT_RECORD, \
					NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_SOFT_SCALER, intensity_modifier_array), \
	{sizeof(MX_RECORD *)}, NULL, \
			(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}

#endif /* __D_SOFT_SCALER_H__ */
