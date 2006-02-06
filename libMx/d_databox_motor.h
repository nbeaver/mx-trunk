/*
 * Name:    d_databox_motor.h
 *
 * Purpose: Header file for MX drivers for motors attached to the Radix
 *          Instruments Databox.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_DATABOX_MOTOR_H__
#define __D_DATABOX_MOTOR_H__

typedef struct {
	MX_RECORD *databox_record;
	char axis_name;

	double steps_per_degree;
} MX_DATABOX_MOTOR;

MX_API mx_status_type mxd_databox_motor_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_databox_motor_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_databox_motor_print_structure( FILE *file,
						MX_RECORD *record );
MX_API mx_status_type mxd_databox_motor_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_databox_motor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_databox_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_databox_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_databox_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_databox_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_databox_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_databox_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_databox_motor_set_parameter( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_databox_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_databox_motor_motor_function_list;

extern mx_length_type mxd_databox_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_databox_motor_rfield_def_ptr;

#define MXD_DATABOX_MOTOR_STANDARD_FIELDS \
  {-1, -1, "databox_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_DATABOX_MOTOR, databox_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "axis_name", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DATABOX_MOTOR, axis_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "steps_per_degree", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DATABOX_MOTOR, steps_per_degree), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_DATABOX_MOTOR_H__ */

