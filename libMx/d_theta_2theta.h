/*
 * Name:    d_theta_2theta.h 
 *
 * Purpose: Header file for MX motor driver to control a theta-2 theta
 *          pair of motors as a single pseudomotor with the 2 theta
 *          motor moving at twice the speed of the theta motor.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2010, 2013-2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_THETA_2THETA_H__
#define __D_THETA_2THETA_H__

#include "mx_motor.h"

/* ===== MX theta-2 theta motor data structures ===== */

/* Values for the 'theta_2theta_flags' field. */

#define MXF_THETA_2THETA_SYNCHRONIZE_SPEED	0x1
#define MXF_THETA_2THETA_USE_AVERAGE_POSITION	0x2

typedef struct {
	MX_MOTOR *motor;	/* Parent motor structure. */

	MX_RECORD *theta_motor_record;
	MX_RECORD *two_theta_motor_record;
	unsigned long theta_2theta_flags;
} MX_THETA_2THETA_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_theta_2theta_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_theta_2theta_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_theta_2theta_motor_print_motor_structure(
					FILE *file, MX_RECORD *record );

MX_API mx_status_type mxd_theta_2theta_motor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_theta_2theta_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_theta_2theta_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_theta_2theta_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_theta_2theta_motor_immediate_abort(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_theta_2theta_motor_positive_limit_hit(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_theta_2theta_motor_negative_limit_hit(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_theta_2theta_motor_constant_velocity_move(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_theta_2theta_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_theta_2theta_motor_set_parameter( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_theta_2theta_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_theta_2theta_motor_motor_function_list;

extern long mxd_theta_2theta_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_theta_2theta_motor_rfield_def_ptr;

#define MXD_THETA_2THETA_MOTOR_STANDARD_FIELDS \
  {-1, -1, "theta_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_THETA_2THETA_MOTOR, theta_motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "two_theta_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_THETA_2THETA_MOTOR, two_theta_motor_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "theta_2theta_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_THETA_2THETA_MOTOR, theta_2theta_flags),\
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_THETA_2THETA_H__ */
