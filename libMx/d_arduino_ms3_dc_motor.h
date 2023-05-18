/*
 * Name:    d_arduino_ms3_dc_motor.h 
 *
 * Purpose: Header file for MX motor driver to use the Arduino Motor Shield R3
 *          to control one of the two available DC motor channels.
 *
 *          The following pinouts are normally used for the two channels
 *
 *          Function     Channel A    Channel B
 *          -----------------------------------
 *          Direction       D12          D13
 *
 *          PWM             D3           D11
 *
 *          Brake           D9           D8
 *
 *          Current         A0           A1
 *          Sensing
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2023 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ARDUINO_MS3_DC_MOTOR_H__
#define __D_ARDUINO_MS3_DC_MOTOR_H__

/* ===== ARDUINO_MS3_DC_MOTOR data structure ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *direction_pin_record;
	MX_RECORD *pwm_pin_record;
	MX_RECORD *brake_pin_record;
	MX_RECORD *current_pin_record;
	char encoder_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_RECORD *encoder_record;
	mx_bool_type move_in_progress;
} MX_ARDUINO_MS3_DC_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_arduino_ms3_dc_motor_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_arduino_ms3_dc_motor_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_arduino_ms3_dc_motor_open( MX_RECORD *record );

MX_API mx_status_type mxd_arduino_ms3_dc_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_arduino_ms3_dc_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_arduino_ms3_dc_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_arduino_ms3_dc_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_arduino_ms3_dc_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_arduino_ms3_dc_motor_immediate_abort(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_arduino_ms3_dc_motor_constant_velocity_move(
							MX_MOTOR *motor);
MX_API mx_status_type mxd_arduino_ms3_dc_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_arduino_ms3_dc_motor_set_parameter( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_arduino_ms3_dc_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_arduino_ms3_dc_motor_motor_function_list;

/* Define some extra functions for the use of this driver. */

extern long mxd_arduino_ms3_dc_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_arduino_ms3_dc_motor_rfield_def_ptr;

#define MXD_ARDUINO_MS3_DC_MOTOR_STANDARD_FIELDS \
  {-1, -1, "direction_pin_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
  		offsetof(MX_ARDUINO_MS3_DC_MOTOR, direction_pin_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "pwm_pin_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_ARDUINO_MS3_DC_MOTOR, pwm_pin_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "brake_pin_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_ARDUINO_MS3_DC_MOTOR, brake_pin_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "current_pin_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
	  	offsetof(MX_ARDUINO_MS3_DC_MOTOR, current_pin_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "encoder_record_name", MXFT_STRING, NULL, 1, \
	  					{MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
	  	offsetof(MX_ARDUINO_MS3_DC_MOTOR, encoder_record_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_ARDUINO_MS3_DC_MOTOR_H__ */

