/*
 * Name:    d_soft_motor.h 
 *
 * Purpose: Header file for MX motor driver for a software emulated
 *          stepping motor.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SOFT_MOTOR_H__
#define __D_SOFT_MOTOR_H__

#include "mx_motor.h"

typedef struct {
	MX_DEAD_RECKONING dead_reckoning;

	double default_speed;
	double default_base_speed;
	double default_acceleration;
} MX_SOFT_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_soft_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_soft_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_soft_motor_print_motor_structure(
					FILE *file, MX_RECORD *record );

MX_API mx_status_type mxd_soft_motor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_soft_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_soft_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_soft_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_soft_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_soft_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_soft_motor_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_soft_motor_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_soft_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_soft_motor_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_soft_motor_simultaneous_start( long num_motor_records,
						MX_RECORD **motor_record_array,
						double *position_array,
						unsigned long flags );

extern MX_RECORD_FUNCTION_LIST mxd_soft_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_soft_motor_motor_function_list;

extern long mxd_soft_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_soft_motor_rfield_def_ptr;

#define MXD_SOFT_MOTOR_STANDARD_FIELDS \
  {-1, -1, "default_speed", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_MOTOR, default_speed), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "default_base_speed", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_MOTOR, default_base_speed), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "default_acceleration", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_MOTOR, default_acceleration), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_SOFT_MOTOR_H__ */
