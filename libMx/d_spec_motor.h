/*
 * Name:    d_spec_motor.h 
 *
 * Purpose: Header file for MX motor driver for motors controlled
 *          via a Spec server.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004, 2006, 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SPEC_MOTOR_H__
#define __D_SPEC_MOTOR_H__

typedef struct {
	MX_RECORD *spec_server_record;
	char remote_motor_name[ MXU_RECORD_NAME_LENGTH+1 ];
	unsigned long spec_motor_flags;

	MX_CLOCK_TICK end_of_busy_delay_tick;
} MX_SPEC_MOTOR;

/* Values for spec_motor_flags. */

#define MXF_SPEC_MOTOR_USE_DIAL_UNITS	0x1

/*---*/

#define MX_SPEC_MOTOR_BUSY_DELAY	1.0	/* in seconds */

/* Define all of the interface functions. */

MX_API mx_status_type mxd_spec_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_spec_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_spec_motor_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_spec_motor_open( MX_RECORD *record );

MX_API mx_status_type mxd_spec_motor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_spec_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_spec_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_spec_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_spec_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_spec_motor_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_spec_motor_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_spec_motor_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_spec_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_spec_motor_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_spec_motor_simultaneous_start( long num_motor_records,
						MX_RECORD **motor_record_array,
						double *position_array,
						unsigned long flags );

MX_API mx_status_type mxd_spec_motor_get_pointers( MX_MOTOR *motor,
				MX_SPEC_MOTOR **spec_motor,
				const char *calling_fname );

extern MX_RECORD_FUNCTION_LIST mxd_spec_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_spec_motor_motor_function_list;

extern long mxd_spec_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_spec_motor_rfield_def_ptr;

#define MXD_SPEC_MOTOR_STANDARD_FIELDS \
  {-1, -1, "spec_server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SPEC_MOTOR, spec_server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "remote_motor_name", MXFT_STRING, \
	NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SPEC_MOTOR, remote_motor_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "spec_motor_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SPEC_MOTOR, spec_motor_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_SPEC_MOTOR_H__ */
