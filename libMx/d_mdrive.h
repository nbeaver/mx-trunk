/*
 * Name:    d_mdrive.h 
 *
 * Purpose: Header file for MX motor driver to control Intelligent Motion
 *          Systems MDrive motor controllers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MDRIVE_H__
#define __D_MDRIVE_H__

#define MXD_MDRIVE_NUM_IO_POINTS	5

#define MXF_MDRIVE_UNKNOWN		(-1)

#define MXF_MDRIVE_IN_GENERAL_PURPOSE	0
#define MXF_MDRIVE_IN_HOME		1
#define MXF_MDRIVE_IN_LIMIT_PLUS	2
#define MXF_MDRIVE_IN_LIMIT_MINUS	3
#define MXF_MDRIVE_IN_G0		4
#define MXF_MDRIVE_IN_SOFT_STOP		5
#define MXF_MDRIVE_IN_PAUSE		6
#define MXF_MDRIVE_IN_JOG_PLUS		7
#define MXF_MDRIVE_IN_JOG_MINUS		8

#define MXF_MDRIVE_ANALOG_VOLTAGE	9
#define MXF_MDRIVE_ANALOG_CURRENT	10

#define MXF_MDRIVE_OUT_GENERAL_PURPOSE	16
#define MXF_MDRIVE_OUT_MOVING		17
#define MXF_MDRIVE_OUT_FAULT		18
#define MXF_MDRIVE_OUT_STALL		19
#define MXF_MDRIVE_OUT_VELOCITY_CHANGING 20

#define MXD_MDRIVE_SKIP_ERROR_STATUS	0x8000

/* ===== MDrive motor data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	char axis_name;

	int io_setup[ MXD_MDRIVE_NUM_IO_POINTS ];

	int negative_limit_switch;
	int positive_limit_switch;
	int home_switch;

	int num_write_terminator_chars;
} MX_MDRIVE;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_mdrive_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_mdrive_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_mdrive_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_mdrive_open( MX_RECORD *record );
MX_API mx_status_type mxd_mdrive_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_mdrive_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_mdrive_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_mdrive_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_mdrive_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_mdrive_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_mdrive_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_mdrive_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_mdrive_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_mdrive_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_mdrive_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_mdrive_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_mdrive_motor_function_list;

/* Define some extra functions for the use of this driver. */

MX_API mx_status_type mxd_mdrive_command( MX_MDRIVE *mdrive, char *command,
			char *response, int response_buffer_length,
			int debug_flag );

MX_API mx_status_type
mxd_mdrive_get_error_code( MX_MDRIVE *mdrive,
				long *mx_error_code,
				int *mdrive_error_code,
				const char **mdrive_error_message );

extern mx_length_type mxd_mdrive_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mdrive_rfield_def_ptr;

#define MXD_MDRIVE_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MDRIVE, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "axis_name", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MDRIVE, axis_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_MDRIVE_H__ */
