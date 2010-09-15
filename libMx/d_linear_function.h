/*
 * Name:    d_linear_function.h 
 *
 * Purpose: Header file for MX motor driver to use a linear function of
 *          multiple MX motors to generate a pseudomotor.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_LINEAR_FUNCTION_H__
#define __D_LINEAR_FUNCTION_H__

#include "mx_motor.h"

/* ===== MX linear function motor data structures ===== */

typedef struct {
	long linear_function_flags;

	long num_records;
	MX_RECORD **record_array;
	double *real_scale;
	double *real_offset;
	double *move_fraction;

	/* Motor arrays */

	long num_motors;
	MX_RECORD **motor_record_array;
	double *motor_position_array;
	double *real_motor_scale;
	double *real_motor_offset;
	double *motor_move_fraction;

	/* Variable arrays */

	long num_variables;
	MX_RECORD **variable_record_array;
	double *variable_value_array;
	double *real_variable_scale;
	double *real_variable_offset;
	double *variable_move_fraction;

} MX_LINEAR_FUNCTION_MOTOR;

/* Values for the linear_function_flags field. */

#define MXF_LINEAR_FUNCTION_MOTOR_SIMULTANEOUS_START	0x1

/* Define all of the interface functions. */

MX_API mx_status_type mxd_linear_function_initialize_driver( MX_DRIVER *driver);
MX_API mx_status_type mxd_linear_function_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_linear_function_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_linear_function_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_linear_function_print_motor_structure(
					FILE *file, MX_RECORD *record );

MX_API mx_status_type mxd_linear_function_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_linear_function_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_linear_function_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_linear_function_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_linear_function_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_linear_function_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_linear_function_positive_limit_hit(MX_MOTOR *motor);
MX_API mx_status_type mxd_linear_function_negative_limit_hit(MX_MOTOR *motor);
MX_API mx_status_type mxd_linear_function_find_home_position(MX_MOTOR *motor);

extern MX_RECORD_FUNCTION_LIST mxd_linear_function_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_linear_function_motor_function_list;

extern long mxd_linear_function_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_linear_function_rfield_def_ptr;

#define MXD_LINEAR_FUNCTION_STANDARD_FIELDS \
  {-1, -1, "linear_function_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_LINEAR_FUNCTION_MOTOR, linear_function_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "num_records", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINEAR_FUNCTION_MOTOR, num_records), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "record_array", MXFT_RECORD, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINEAR_FUNCTION_MOTOR, record_array),\
	{sizeof(MX_RECORD *)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)},\
  \
  {-1, -1, "real_scale", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINEAR_FUNCTION_MOTOR, real_scale), \
	{sizeof(double)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS)},\
  \
  {-1, -1, "real_offset", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINEAR_FUNCTION_MOTOR, real_offset), \
	{sizeof(double)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS)},\
  \
  {-1, -1, "move_fraction", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_LINEAR_FUNCTION_MOTOR, move_fraction), \
	{sizeof(double)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS)}

#endif /* __D_LINEAR_FUNCTION_H__ */
