/*
 * Name:    d_tangent_arm.h 
 *
 * Purpose: Header file for MX motor driver to move a tangent arm or sine arm.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2002, 2007, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_TANGENT_ARM_H__
#define __D_TANGENT_ARM_H__

#include "mx_motor.h"

/* ===== MX tangent arm data structures ===== */

typedef struct {
	MX_MOTOR *motor;	/* Parent motor structure. */

	MX_RECORD *moving_motor_record;
	MX_RECORD *angle_offset_record;
	double arm_length;
	unsigned long tangent_arm_flags;
} MX_TANGENT_ARM;

/* Tangent arm flag values:
 *
 * 0x1 - 'Set position' commands change the angle offset value rather than
 *        the moving motor position.
 */

#define MXF_TAN_CHANGE_ANGLE_OFFSET	0x1

/* Define all of the interface functions. */

MX_API mx_status_type mxd_tangent_arm_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_tangent_arm_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_tangent_arm_print_motor_structure(
					FILE *file, MX_RECORD *record );

MX_API mx_status_type mxd_tangent_arm_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_tangent_arm_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_tangent_arm_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_tangent_arm_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_tangent_arm_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_tangent_arm_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_tangent_arm_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_tangent_arm_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_tangent_arm_raw_home_command( MX_MOTOR *motor );
MX_API mx_status_type mxd_tangent_arm_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_tangent_arm_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_tangent_arm_set_parameter( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_tangent_arm_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_tangent_arm_motor_function_list;

extern long mxd_tangent_arm_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_tangent_arm_rfield_def_ptr;

#define MXD_TANGENT_ARM_STANDARD_FIELDS \
  {-1, -1, "moving_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TANGENT_ARM, moving_motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "angle_offset_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TANGENT_ARM, angle_offset_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "arm_length", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TANGENT_ARM, arm_length), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "tangent_arm_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TANGENT_ARM, tangent_arm_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_TANGENT_ARM_H__ */

