/*
 * Name:    d_table_motor.h 
 *
 * Purpose: Header file for MX table pseudomotors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2006, 2010, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_TABLE_MOTOR_H__
#define __D_TABLE_MOTOR_H__

#include "mx_motor.h"

typedef struct {
	MX_RECORD *table_record;
	long axis_id;
} MX_TABLE_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_table_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_table_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_table_motor_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_table_motor_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_table_motor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_table_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_table_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_table_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_table_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_table_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_table_motor_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_table_motor_negative_limit_hit( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_table_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_table_motor_motor_function_list;

extern long mxd_table_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_table_motor_rfield_def_ptr;

#define MXD_TABLE_MOTOR_STANDARD_FIELDS \
  {-1, -1, "table_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TABLE_MOTOR, table_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "axis_id", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TABLE_MOTOR, axis_id), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_TABLE_MOTOR_H__ */

