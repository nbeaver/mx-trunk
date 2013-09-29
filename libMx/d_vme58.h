/*
 * Name:    d_vme58.h
 *
 * Purpose: Header file for MX drivers for Oregon Microsystems VME58 motor
 *          controllers.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2000, 2002, 2006, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_VME58_H__
#define __D_VME58_H__

#define MX_VME58_MOTOR_MAXIMUM_SPEED		1000000L
#define MX_VME58_MOTOR_MAXIMUM_ACCELERATION	1000000000L

/* Values for the flags variable. */

#define MXF_VME58_MOTOR_IGNORE_DATABASE_SETTINGS	0x0001
#define MXF_VME58_MOTOR_USE_ENCODER			0x0002
#define MXF_VME58_MOTOR_DISABLE_HARDWARE_LIMITS		0x0010
#define MXF_VME58_MOTOR_HOME_HIGH			0x0020

typedef struct {
	MX_RECORD *vme58_record;
	long axis_number;
	unsigned long flags;

	long default_speed;
	long default_base_speed;
	long default_acceleration;
} MX_VME58_MOTOR;

MX_API mx_status_type mxd_vme58_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_vme58_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_vme58_print_structure( FILE *file,
						MX_RECORD *record );
MX_API mx_status_type mxd_vme58_open( MX_RECORD *record );
MX_API mx_status_type mxd_vme58_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_vme58_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_vme58_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_vme58_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_vme58_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_vme58_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_vme58_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_vme58_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_vme58_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_vme58_raw_home_command( MX_MOTOR *motor );
MX_API mx_status_type mxd_vme58_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_vme58_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_vme58_set_parameter( MX_MOTOR *motor );

MX_API mx_status_type mxd_vme58_enable_continuous_mode(
				MX_VME58_MOTOR *vme58_motor,
				MX_VME58 *vme58,
				int enable_flag );

extern MX_RECORD_FUNCTION_LIST mxd_vme58_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_vme58_motor_function_list;

extern long mxd_vme58_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_vme58_rfield_def_ptr;

#define MXD_VME58_MOTOR_STANDARD_FIELDS \
  {-1, -1, "vme58_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_VME58_MOTOR, vme58_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "axis_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME58_MOTOR, axis_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME58_MOTOR, flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "default_speed", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME58_MOTOR, default_speed), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "default_base_speed", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME58_MOTOR, default_base_speed), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "default_acceleration", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME58_MOTOR, default_acceleration), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_VME58_H__ */
