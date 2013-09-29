/*
 * Name:    d_stp100.h 
 *
 * Purpose: Header file for MX motor driver to control Pontech STP100
 *          stepper motor controllers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2003, 2006, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_STP100_H__
#define __D_STP100_H__

/* ===== Pontech STP100 motor controller data structure ===== */

/*
 * The permitted board numbers are from 1 to 255.
 *
 * The permitted values for digital I/O pins are:
 *
 * 0              - Disable the pin.
 * 3, 5, 6, 8     - The pin is active closed.
 * -3, -5, -6, -8 - The pin is active open.
 *
 * Pins 5 and 6 are normally used for limit switches while either pin 3
 * or pin 8 is used for the home switch.  This is because pins 5 and 6
 * already have pullup resistors.
 *
 * The output of the RP command is 0 = closed and 1 = open.
 *
 */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	long board_number;

	long positive_limit_switch_pin;
	long negative_limit_switch_pin;
	long home_switch_pin;

	double default_speed;
	double default_base_speed;
	double default_acceleration_time;
} MX_STP100_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_stp100_motor_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_stp100_motor_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_stp100_motor_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_stp100_motor_open( MX_RECORD *record );
MX_API mx_status_type mxd_stp100_motor_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_stp100_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_stp100_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_stp100_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_stp100_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_stp100_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_stp100_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_stp100_motor_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_stp100_motor_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_stp100_motor_raw_home_command( MX_MOTOR *motor );
MX_API mx_status_type mxd_stp100_motor_constant_velocity_move( MX_MOTOR *motor);
MX_API mx_status_type mxd_stp100_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_stp100_motor_set_parameter( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_stp100_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_stp100_motor_motor_function_list;

/* Define some extra functions for the use of this driver. */

MX_API mx_status_type mxd_stp100_motor_command( MX_STP100_MOTOR *stp100_motor,
						char *command,
						char *response,
						size_t response_buffer_length,
						int debug_flag );

extern long mxd_stp100_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_stp100_motor_rfield_def_ptr;

#define MXD_STP100_MOTOR_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_STP100_MOTOR, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "board_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_STP100_MOTOR, board_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "positive_limit_switch_pin", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_STP100_MOTOR, positive_limit_switch_pin), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "negative_limit_switch_pin", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_STP100_MOTOR, negative_limit_switch_pin), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "home_switch_pin", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_STP100_MOTOR, home_switch_pin), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "default_speed", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_STP100_MOTOR, default_speed), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "default_base_speed", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_STP100_MOTOR, default_base_speed), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "default_acceleration_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_STP100_MOTOR, \
					default_acceleration_time), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#endif /* __D_STP100_H__ */
