/*
 * Name:    d_am9513_motor.h
 *
 * Purpose: Header file for MX motor driver to use Am9513 counters
 *          as motor indexer/controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_AM9513_MOTOR_H__
#define __D_AM9513_MOTOR_H__

#include "i_am9513.h"

/* ============ Motor channels ============ */

typedef struct {
	long num_counters;
	MX_INTERFACE *am9513_interface_array;
	MX_RECORD *direction_record;
	long direction_bit;
	double clock_frequency;
	double step_frequency;
	long busy_retries;

	long last_destination;
	long last_direction;
	long busy_retries_left;
} MX_AM9513_MOTOR;

MX_API mx_status_type mxd_am9513_motor_initialize_type( long type );
MX_API mx_status_type mxd_am9513_motor_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_am9513_motor_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_am9513_motor_print_structure( FILE *file,
						MX_RECORD *record );
MX_API mx_status_type mxd_am9513_motor_open( MX_RECORD *record );
MX_API mx_status_type mxd_am9513_motor_close( MX_RECORD *record );
MX_API mx_status_type mxd_am9513_motor_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_am9513_motor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_am9513_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_am9513_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_am9513_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_am9513_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_am9513_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_am9513_motor_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_am9513_motor_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_am9513_motor_find_home_position( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_am9513_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_am9513_motor_motor_function_list;

extern long mxd_am9513_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_am9513_motor_rfield_def_ptr;

#define MXD_AM9513_MOTOR_STANDARD_FIELDS \
  {-1, -1, "num_counters", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AM9513_MOTOR, num_counters), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "am9513_interface_array", MXFT_INTERFACE, NULL, \
	1, {MXU_VARARGS_LENGTH}, MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AM9513_MOTOR, am9513_interface_array), \
	{sizeof(MX_INTERFACE)}, \
		NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}, \
  \
  {-1, -1, "direction_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AM9513_MOTOR, direction_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "direction_bit", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AM9513_MOTOR, direction_bit), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "clock_frequency", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AM9513_MOTOR, clock_frequency), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "step_frequency", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AM9513_MOTOR, step_frequency), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "busy_retries", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AM9513_MOTOR, busy_retries), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#endif /* __D_AM9513_MOTOR_H__ */
