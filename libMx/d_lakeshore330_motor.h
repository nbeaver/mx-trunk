/*
 * Name:    d_lakeshore330_motor.h
 *
 * Purpose: Header file for MX motor driver to control a LakeShore 330
 *          temperature controller as if it were a motor.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2000, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_LAKESHORE330_MOTOR_H__
#define __D_LAKESHORE330_MOTOR_H__

/* ===== LakeShore 330 motor data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_INTERFACE port_interface;
	double busy_deadband;
} MX_LS330_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_ls330_motor_initialize_type( long type );
MX_API mx_status_type mxd_ls330_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_ls330_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_ls330_motor_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_ls330_motor_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_ls330_motor_read_parms_from_hardware(
					MX_RECORD *record);
MX_API mx_status_type mxd_ls330_motor_write_parms_to_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxd_ls330_motor_open( MX_RECORD *record );
MX_API mx_status_type mxd_ls330_motor_close( MX_RECORD *record );
MX_API mx_status_type mxd_ls330_motor_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_ls330_motor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_ls330_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_ls330_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_ls330_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_ls330_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_ls330_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_ls330_motor_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_ls330_motor_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_ls330_motor_find_home_position( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_ls330_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_ls330_motor_motor_function_list;

/* Define an extra function for the use of this driver. */

MX_API mx_status_type mxd_ls330_motor_command(
			MX_LS330_MOTOR *ls330_motor,
			char *command, char *response,
			int response_buffer_length, int debug_flag );

extern mx_length_type mxd_ls330_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_ls330_motor_rfield_def_ptr;

#define MXD_LS330_MOTOR_STANDARD_FIELDS \
  {-1, -1, "port_interface", MXFT_INTERFACE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LS330_MOTOR, port_interface), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "busy_deadband", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LS330_MOTOR, busy_deadband), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \

#endif /* __D_LAKESHORE330_MOTOR_H__ */
