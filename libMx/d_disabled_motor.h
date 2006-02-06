/*
 * Name:    d_disabled_motor.h 
 *
 * Purpose: Header file for MX motor driver for a disabled motor.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_DISABLED_MOTOR_H__
#define __D_DISABLED_MOTOR_H__

#include "mx_motor.h"

typedef struct {
	int dummy;
} MX_DISABLED_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_disabled_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_disabled_motor_finish_record_initialization(
					MX_RECORD *record );

MX_API mx_status_type mxd_disabled_motor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_disabled_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_disabled_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_disabled_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_disabled_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_disabled_motor_immediate_abort( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_disabled_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_disabled_motor_motor_function_list;

extern mx_length_type mxd_disabled_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_disabled_motor_rfield_def_ptr;

#endif /* __D_DISABLED_MOTOR_H__ */

