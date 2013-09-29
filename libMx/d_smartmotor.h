/*
 * Name:    d_smartmotor.h 
 *
 * Purpose: Header file for MX motor driver to control Animatics SmartMotor
 *          servo motor controllers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SMARTMOTOR_H__
#define __D_SMARTMOTOR_H__

#define MXD_SMARTMOTOR_NUM_IO_POINTS		7

#define MXU_SMARTMOTOR_IO_NAME_LENGTH		5

#define MXU_SMARTMOTOR_FIRMWARE_VERSION_LENGTH	20

/* SmartMotor type values. */

#define MXT_SMARTMOTOR_UNKNOWN			0
#define MXT_SMARTMOTOR_SM17			17
#define MXT_SMARTMOTOR_SM23			23
#define MXT_SMARTMOTOR_SM34			34
#define MXT_SMARTMOTOR_SM42			42
#define MXT_SMARTMOTOR_SM56			56

/* Values for the 'smartmotor_flags' field. */

#define MXF_SMARTMOTOR_AUTO_ADDRESS_CONFIG		0x1
#define MXF_SMARTMOTOR_ECHO_ON				0x2

#define MXF_SMARTMOTOR_ENABLE_LIMIT_DURING_HOME_SEARCH	0x1000

/* ===== SmartMotor motor data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	long motor_address;
	unsigned long smartmotor_flags;

	double velocity_scale_factor;
	double acceleration_scale_factor;

	long sample_rate;
	char firmware_version[ MXU_SMARTMOTOR_FIRMWARE_VERSION_LENGTH+1 ];

	mx_bool_type home_search_succeeded;
	mx_bool_type historical_left_limit;
	mx_bool_type historical_right_limit;
} MX_SMARTMOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_smartmotor_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_smartmotor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_smartmotor_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_smartmotor_open( MX_RECORD *record );
MX_API mx_status_type mxd_smartmotor_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_smartmotor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_smartmotor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_smartmotor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_smartmotor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_smartmotor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_smartmotor_raw_home_command( MX_MOTOR *motor );
MX_API mx_status_type mxd_smartmotor_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_smartmotor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_smartmotor_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_smartmotor_get_status( MX_MOTOR *motor );
MX_API mx_status_type mxd_smartmotor_get_extended_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_smartmotor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_smartmotor_motor_function_list;

/* Define some extra functions for the use of this driver. */

MX_API mx_status_type
mxd_smartmotor_command( MX_SMARTMOTOR *smartmotor,
			char *command,
			char *response, size_t response_buffer_length,
			int debug_flag );

MX_API mx_status_type
mxd_smartmotor_check_port_name( MX_RECORD *smartmotor_record, char *port_name );

extern long mxd_smartmotor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_smartmotor_rfield_def_ptr;

#define MXD_SMARTMOTOR_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SMARTMOTOR, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "motor_address", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SMARTMOTOR, motor_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "smartmotor_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SMARTMOTOR, smartmotor_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "sample_rate", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SMARTMOTOR, sample_rate), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "firmware_version", MXFT_STRING, NULL, \
			1, {MXU_SMARTMOTOR_FIRMWARE_VERSION_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SMARTMOTOR, firmware_version), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}

#endif /* __D_SMARTMOTOR_H__ */
