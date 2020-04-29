/*
 * Name:    d_galil_gclib.h
 *
 * Purpose: Header file for motors controlled by Galil gclib controllers.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_GALIL_GCLIB_H__
#define __D_GALIL_GCLIB_H__

/*---*/

typedef struct {
	MX_RECORD *record;

	MX_RECORD *galil_gclib_record;
	char axis_name;
} MX_GALIL_GCLIB_MOTOR;

MX_API mx_status_type mxd_galil_gclib_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_galil_gclib_open( MX_RECORD *record );
MX_API mx_status_type mxd_galil_gclib_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxd_galil_gclib_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_galil_gclib_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_galil_gclib_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_galil_gclib_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_galil_gclib_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_galil_gclib_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_galil_gclib_raw_home_command( MX_MOTOR *motor );
MX_API mx_status_type mxd_galil_gclib_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_galil_gclib_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_galil_gclib_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_galil_gclib_simultaneous_start(
						long num_motor_records,
						MX_RECORD **motor_record_array,
						double *position_array,
						unsigned long flags );
MX_API mx_status_type mxd_galil_gclib_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_galil_gclib_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_galil_gclib_motor_function_list;

extern long mxd_galil_gclib_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_galil_gclib_rfield_def_ptr;

#define MXD_GALIL_GCLIB_MOTOR_STANDARD_FIELDS \
  {-1, -1, "galil_gclib_record", MXFT_RECORD, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_GALIL_GCLIB_MOTOR, galil_gclib_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "axis_name", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GALIL_GCLIB_MOTOR,axis_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_GALIL_GCLIB_H__ */
