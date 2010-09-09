/*
 * Name:    d_compumotor_linear.h 
 *
 * Purpose: Header file for MX motor driver to perform linear interpolation
 *          moves via a Compumotor 6K controller.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_COMPUMOTOR_LINEAR_H__
#define __D_COMPUMOTOR_LINEAR_H__

#include "mx_motor.h"

/* ===== MX Compumotor linear interpolation motor data structures ===== */

typedef struct {
	long flags;

	long num_motors;
	MX_RECORD **motor_record_array;
	double *real_motor_scale;
	double *real_motor_offset;
	double *motor_move_fraction;

	double *motor_position_array;

	long *index_to_axis_number;
	long axis_number_to_index[MX_MAX_COMPUMOTOR_AXES];

	MX_RECORD *compumotor_interface_record;
	long controller_index;
	long num_axes;
} MX_COMPUMOTOR_LINEAR_MOTOR;

/* Values for the flags variable. */

#define MXF_COMPUMOTOR_SIMULTANEOUS_START	0x1

/* Define all of the interface functions. */

MX_API mx_status_type mxd_compumotor_linear_initialize_type( long type );
MX_API mx_status_type mxd_compumotor_linear_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_linear_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_linear_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_linear_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_linear_open( MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_linear_close( MX_RECORD *record );

MX_API mx_status_type mxd_compumotor_linear_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_linear_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_linear_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_linear_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_linear_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_linear_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_linear_positive_limit_hit(MX_MOTOR *motor);
MX_API mx_status_type mxd_compumotor_linear_negative_limit_hit(MX_MOTOR *motor);
MX_API mx_status_type mxd_compumotor_linear_find_home_position(MX_MOTOR *motor);

extern MX_RECORD_FUNCTION_LIST mxd_compumotor_linear_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_compumotor_linear_motor_function_list;

extern long mxd_compumotor_linear_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_compumotor_linear_rfield_def_ptr;

#define MXD_COMPUMOTOR_LINEAR_STANDARD_FIELDS \
  {-1, -1, "flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR_LINEAR_MOTOR, flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "num_motors", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR_LINEAR_MOTOR, num_motors), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "motor_record_array", MXFT_RECORD, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_COMPUMOTOR_LINEAR_MOTOR, motor_record_array), \
	{sizeof(MX_RECORD *)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)},\
  \
  {-1, -1, "real_motor_scale", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_COMPUMOTOR_LINEAR_MOTOR, real_motor_scale), \
	{sizeof(double)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS)},\
  \
  {-1, -1, "real_motor_offset", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_COMPUMOTOR_LINEAR_MOTOR, real_motor_offset), \
	{sizeof(double)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS)},\
  \
  {-1, -1, "motor_move_fraction", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_COMPUMOTOR_LINEAR_MOTOR, motor_move_fraction), \
	{sizeof(double)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS)}

#endif /* __D_COMPUMOTOR_LINEAR_H__ */
