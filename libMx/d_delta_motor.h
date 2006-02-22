/*
 * Name:    d_delta_motor.h 
 *
 * Purpose: Header file for MX motor driver to move a real motor relative
 *          to a fixed position.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_DELTA_MOTOR_H__
#define __D_DELTA_MOTOR_H__

#include "mx_motor.h"

/* ===== MX delta motor data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *moving_motor_record;
	MX_RECORD *fixed_position_record;

	double saved_fixed_position_value;
} MX_DELTA_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_delta_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_delta_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_delta_motor_print_motor_structure(
					FILE *file, MX_RECORD *record );

MX_API mx_status_type mxd_delta_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_delta_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_delta_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_delta_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_delta_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_delta_motor_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_delta_motor_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_delta_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_delta_motor_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_delta_motor_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_delta_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_delta_motor_motor_function_list;

extern long mxd_delta_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_delta_motor_rfield_def_ptr;

#define MXD_DELTA_MOTOR_STANDARD_FIELDS \
  {-1, -1, "moving_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DELTA_MOTOR, moving_motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "fixed_position_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DELTA_MOTOR, fixed_position_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_DELTA_MOTOR_H__ */

