/*
 * Name:    d_trans_motor.h 
 *
 * Purpose: Header file for MX motor driver to use multiple MX motors to
 *          perform a linear translation.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_TRANS_MOTOR_H__
#define __D_TRANS_MOTOR_H__

#include "mx_motor.h"

/* ===== MX translation motor data structures ===== */

typedef struct {
	long translation_flags;

	long num_motors;
	MX_RECORD **motor_record_array;
	double *position_array;

	double pseudomotor_start_position;
	double *saved_start_position_array;
} MX_TRANSLATION_MOTOR;

/* Values for the translation_flags field. */

#define MXF_TRANSLATION_MOTOR_SIMULTANEOUS_START	0x1

/* Define all of the interface functions. */

MX_API mx_status_type mxd_trans_motor_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_trans_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_trans_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_trans_motor_print_motor_structure(
					FILE *file, MX_RECORD *record );

MX_API mx_status_type mxd_trans_motor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_trans_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_trans_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_trans_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_trans_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_trans_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_trans_motor_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_trans_motor_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_trans_motor_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_trans_motor_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_trans_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_trans_motor_set_parameter( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_trans_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_trans_motor_motor_function_list;

extern long mxd_trans_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_trans_motor_rfield_def_ptr;

#define MXD_TRANS_MOTOR_STANDARD_FIELDS \
  {-1, -1, "tranlation_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TRANSLATION_MOTOR, translation_flags),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "num_motors", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TRANSLATION_MOTOR, num_motors), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "motor_record_array", MXFT_RECORD, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_TRANSLATION_MOTOR, motor_record_array), \
	{sizeof(MX_RECORD *)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)},\
  \
  {-1, -1, "position_array", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TRANSLATION_MOTOR, position_array), \
	{sizeof(double)}, NULL, MXFF_VARARGS}, \
  \
  {-1, -1, "saved_start_position_array", MXFT_DOUBLE, \
	  		NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_TRANSLATION_MOTOR, saved_start_position_array), \
	{sizeof(double)}, NULL, MXFF_VARARGS}

#endif /* __D_TRANS_MOTOR_H__ */
