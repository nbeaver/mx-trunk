/*
 * Name:     mx_motor.h
 *
 * Purpose:  Main header file for the MX motor class.
 *
 * Author:   William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_MOTOR_H__
#define __MX_MOTOR_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
#define MX_MOTOR_STEP_FUZZ    (1.0e-9)
#define MX_MOTOR_ANALOG_FUZZ  (1.0e-9)

/* Flag values for mx_motor_move_absolute() and similar routines. */

#define MXF_MTR_NONE               		0x0
#define MXF_MTR_SHOW_MOVE          		0x1
#define MXF_MTR_NOWAIT             		0x2
#define MXF_MTR_IGNORE_KEYBOARD    		0x4
#define MXF_MTR_IGNORE_BACKLASH    		0x8
#define MXF_MTR_GO_TO_BACKLASH_POSITION		0x10
#define MXF_MTR_ONLY_CHECK_LIMITS		0x20
#define MXF_MTR_SCAN_IN_PROGRESS		0x40
#define MXF_MTR_SIMULTANEOUS_START		0x80
#define MXF_MTR_IGNORE_LIMIT_SWITCHES		0x100
#define MXF_MTR_IGNORE_ERRORS			0x200
#define MXF_MTR_IGNORE_PAUSE			0x400

#define MX_MOTOR_NUM_SPEED_CHOICE_PARAMS	3
#define MX_MOTOR_NUM_ACCELERATION_PARAMS	4
#define MX_MOTOR_NUM_SCAN_RANGE_PARAMS		4
#define MX_MOTOR_NUM_POSITION_PARAMS		2

/* Motor record flags. */

#define MXF_MTR_IS_PSEUDOMOTOR				1
#define MXF_MTR_IS_REMOTE_MOTOR				2
#define MXF_MTR_CANNOT_CHANGE_SPEED			4
#define MXF_MTR_CANNOT_QUICK_SCAN			8
#define MXF_MTR_PSEUDOMOTOR_RECURSION_IS_NOT_NECESSARY	16

/* Acceleration types. */

#define MXF_MTR_ACCEL_NONE			0
#define MXF_MTR_ACCEL_RATE			1	/* in units/sec**2 */
#define MXF_MTR_ACCEL_TIME			2	/* in sec */

/* Status bit definitions for the 'status' and 'extended_status' fields. */

#define MXSF_MTR_IS_BUSY			0x1
#define MXSF_MTR_POSITIVE_LIMIT_HIT		0x2
#define MXSF_MTR_NEGATIVE_LIMIT_HIT		0x4
#define MXSF_MTR_HOME_SEARCH_SUCCEEDED		0x8
#define MXSF_MTR_FOLLOWING_ERROR		0x10
#define MXSF_MTR_DRIVE_FAULT			0x20
#define MXSF_MTR_AXIS_DISABLED			0x40
#define MXSF_MTR_OPEN_LOOP			0x80
#define MXSF_MTR_CONTROLLER_DISABLED		0x100
#define MXSF_MTR_SOFT_POSITIVE_LIMIT_HIT	0x200
#define MXSF_MTR_SOFT_NEGATIVE_LIMIT_HIT	0x400
#define MXSF_MTR_SOFTWARE_ERROR			0x800

#define MXSF_MTR_ERROR				0x80000000


#define MXSF_MTR_ERROR_BITMASK	\
  ( MXSF_MTR_POSITIVE_LIMIT_HIT | MXSF_MTR_NEGATIVE_LIMIT_HIT \
  | MXSF_MTR_FOLLOWING_ERROR | MXSF_MTR_DRIVE_FAULT \
  | MXSF_MTR_AXIS_DISABLED | MXSF_MTR_OPEN_LOOP | MXSF_MTR_CONTROLLER_DISABLED \
  | MXSF_MTR_SOFT_POSITIVE_LIMIT_HIT | MXSF_MTR_SOFT_NEGATIVE_LIMIT_HIT \
  | MXSF_MTR_SOFTWARE_ERROR | MXSF_MTR_ERROR )

/* The maximum length of the string used to transmit the 'extended_status'
 * field across the network.
 */

#define MXU_EXTENDED_STATUS_STRING_LENGTH	40

/* The MX_MOTOR structure contains the data that is common to all motor types.*/

typedef struct {
	MX_RECORD *record; /* Pointer to the MX_RECORD structure that points
			    * to this motor.
			    */

	union { long stepper;
		double analog;
	} raw_destination;

	union { long stepper;
		double analog;
	} raw_position;

	union { long stepper;
		double analog;
	} raw_backlash_correction;

	union { long stepper;
		double analog;
	} raw_negative_limit;

	union { long stepper;
		double analog;
	} raw_positive_limit;

	union { long stepper;
		double analog;
	} raw_move_deadband;

	union { long stepper;
		double analog;
	} raw_set_position;

	double value_change_threshold;

	double raw_minimum_speed_limit;
	double raw_maximum_speed_limit;

	double minimum_speed_limit;
	double maximum_speed_limit;

	long subclass;

	unsigned long motor_flags;

	double scale;
	double offset;
	char   units[ MXU_UNITS_NAME_LENGTH + 1 ];

	double destination;
	double old_destination;

	double position;
	double set_position;

	mx_bool_type backlash_move_in_progress;
	mx_bool_type server_backlash_in_progress;
	double backlash_correction;
	double quick_scan_backlash_correction;

	double negative_limit;
	double positive_limit;

	double raw_speed;
	double raw_base_speed;
	double raw_maximum_speed;

	double speed;
	double base_speed;
	double maximum_speed;

	mx_bool_type save_speed;
	mx_bool_type restore_speed;
	mx_bool_type synchronous_motion_mode;

	double raw_saved_speed;
	mx_bool_type saved_synchronous_motion_mode;

	double speed_choice_parameters[ MX_MOTOR_NUM_SPEED_CHOICE_PARAMS ];
	double raw_speed_choice_parameters[ MX_MOTOR_NUM_SPEED_CHOICE_PARAMS ];

	long acceleration_type;

	double raw_acceleration_parameters[ MX_MOTOR_NUM_ACCELERATION_PARAMS ];
	double raw_acceleration_distance;

	double acceleration_distance;
	double acceleration_time;

	long parameter_type;

	mx_bool_type busy;
	mx_bool_type soft_abort;
	mx_bool_type immediate_abort;
	mx_bool_type negative_limit_hit;
	mx_bool_type positive_limit_hit;

	long home_search;
	long constant_velocity_move;

	unsigned long status;
	char extended_status[ MXU_EXTENDED_STATUS_STRING_LENGTH + 1 ];

	/* The following saved field numbers are used by the function
	 * mx_motor_vctest_extended_status().
	 */

	long extended_status_field_number;
	long position_field_number;
	long status_field_number;

	MX_RECORD *real_motor_record;

	double compute_extended_scan_range[ MX_MOTOR_NUM_SCAN_RANGE_PARAMS ];
	double raw_compute_extended_scan_range[MX_MOTOR_NUM_SCAN_RANGE_PARAMS];

	double compute_pseudomotor_position[ MX_MOTOR_NUM_POSITION_PARAMS ];
	double compute_real_position[ MX_MOTOR_NUM_POSITION_PARAMS ];

	double save_start_positions;
	double raw_saved_start_position;

	mx_bool_type use_start_positions;

	mx_bool_type axis_enable;
	mx_bool_type closed_loop;
	mx_bool_type fault_reset;

	double proportional_gain;
	double integral_gain;
	double derivative_gain;
	double velocity_feedforward_gain;
	double acceleration_feedforward_gain;
	double integral_limit;
	double extra_gain;
} MX_MOTOR;

#define MXLV_MTR_BUSY					1001
#define MXLV_MTR_DESTINATION				1002
#define MXLV_MTR_POSITION				1003
#define MXLV_MTR_SET_POSITION				1004
#define MXLV_MTR_SOFT_ABORT				1005
#define MXLV_MTR_IMMEDIATE_ABORT			1006
#define MXLV_MTR_NEGATIVE_LIMIT_HIT			1007
#define MXLV_MTR_POSITIVE_LIMIT_HIT			1008
#define MXLV_MTR_HOME_SEARCH				1009
#define MXLV_MTR_CONSTANT_VELOCITY_MOVE			1010
#define MXLV_MTR_SPEED					1011
#define MXLV_MTR_BASE_SPEED				1012
#define MXLV_MTR_MAXIMUM_SPEED				1013
#define MXLV_MTR_ACCELERATION_TYPE			1014
#define MXLV_MTR_RAW_ACCELERATION_PARAMETERS		1015
#define MXLV_MTR_ACCELERATION_TIME			1016
#define MXLV_MTR_ACCELERATION_DISTANCE			1017
#define MXLV_MTR_SPEED_CHOICE_PARAMETERS		1018
#define MXLV_MTR_SAVE_SPEED				1019
#define MXLV_MTR_RESTORE_SPEED				1020
#define MXLV_MTR_SYNCHRONOUS_MOTION_MODE		1021
#define MXLV_MTR_COMPUTE_EXTENDED_SCAN_RANGE		1022
#define MXLV_MTR_COMPUTE_PSEUDOMOTOR_POSITION		1023
#define MXLV_MTR_COMPUTE_REAL_POSITION			1024
#define MXLV_MTR_GET_REAL_MOTOR_FROM_PSEUDOMOTOR	1025
#define MXLV_MTR_GET_STATUS				1026
#define MXLV_MTR_GET_EXTENDED_STATUS			1027
#define MXLV_MTR_SAVE_START_POSITIONS			1028
#define MXLV_MTR_USE_START_POSITIONS			1029

#define MXLV_MTR_VALUE_CHANGED_THRESHOLD			3001

#define MXLV_MTR_AXIS_ENABLE				4001
#define MXLV_MTR_CLOSED_LOOP				4002
#define MXLV_MTR_FAULT_RESET				4003

#define MXLV_MTR_PROPORTIONAL_GAIN			5001
#define MXLV_MTR_INTEGRAL_GAIN				5002
#define MXLV_MTR_DERIVATIVE_GAIN			5003
#define MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN		5004
#define MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN		5005	
#define MXLV_MTR_INTEGRAL_LIMIT				5006
#define MXLV_MTR_EXTRA_GAIN				5007

#define MX_STEPPER_MOTOR_STANDARD_FIELDS \
  {-1, -1, "raw_destination", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, raw_destination.stepper), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "raw_position", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, raw_position.stepper), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_POLL)}, \
  \
  {-1, -1, "raw_backlash_correction", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, \
					raw_backlash_correction.stepper), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "raw_negative_limit", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, raw_negative_limit.stepper),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "raw_positive_limit", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, raw_positive_limit.stepper),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "raw_move_deadband", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, raw_move_deadband.stepper),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "raw_set_position", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, raw_set_position.stepper), \
	{0}, NULL, 0}

#define MX_ANALOG_MOTOR_STANDARD_FIELDS \
  {-1, -1, "raw_destination", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, raw_destination.analog), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "raw_position", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, raw_position.analog), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_POLL)}, \
  \
  {-1, -1, "raw_backlash_correction", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, \
					raw_backlash_correction.analog), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "raw_negative_limit", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, raw_negative_limit.analog),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "raw_positive_limit", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, raw_positive_limit.analog),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "raw_move_deadband", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, raw_move_deadband.analog),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "raw_set_position", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, raw_set_position.analog), \
	{0}, NULL, 0}

#define MX_MOTOR_STANDARD_FIELDS \
  {MXLV_MTR_VALUE_CHANGED_THRESHOLD, -1, "value_change_threshold", \
  						MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, value_change_threshold),\
	{0}, NULL, 0}, \
  \
  {-1, -1, "raw_minimum_speed_limit", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, raw_minimum_speed_limit),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "raw_maximum_speed_limit", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, raw_maximum_speed_limit),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "minimum_speed_limit", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, minimum_speed_limit),\
	{0}, NULL, 0}, \
  \
  {-1, -1, "maximum_speed_limit", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, maximum_speed_limit),\
	{0}, NULL, 0}, \
  \
  {-1, -1, "subclass", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, subclass), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "motor_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, motor_flags), \
	{0}, NULL, MXFF_NO_NEXT_EVENT_TIME_UPDATE}, \
  \
  {-1, -1, "scale", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, scale), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "offset", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, offset), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "units", MXFT_STRING, NULL, 1, {MXU_UNITS_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, units), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_MTR_DESTINATION, -1, "destination", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, destination), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "old_destination", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, old_destination), \
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_POSITION, -1, "position", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, position), \
	{0}, NULL, MXFF_POLL, \
	0, 0, mx_motor_vctest_extended_status}, \
  \
  {MXLV_MTR_SET_POSITION, -1, "set_position", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, set_position), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "backlash_move_in_progress", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, backlash_move_in_progress), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "server_backlash_in_progress", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, server_backlash_in_progress), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "backlash_correction", MXFT_DOUBLE, NULL, 0, {0},\
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, backlash_correction), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "quick_scan_backlash_correction", MXFT_DOUBLE, NULL, 0, {0},\
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_MOTOR, quick_scan_backlash_correction), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "negative_limit", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, negative_limit),\
	{0}, NULL, 0}, \
  \
  {-1, -1, "positive_limit", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, positive_limit),\
	{0}, NULL, 0}, \
  \
  {-1, -1, "raw_speed", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, raw_speed),\
	{0}, NULL, 0}, \
  \
  {-1, -1, "raw_base_speed", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, raw_base_speed),\
	{0}, NULL, 0}, \
  \
  {-1, -1, "raw_maximum_speed", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, raw_maximum_speed),\
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_SPEED, -1, "speed", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, speed),\
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_BASE_SPEED, -1, "base_speed", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, base_speed),\
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_MAXIMUM_SPEED, -1, "maximum_speed", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, maximum_speed),\
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_SAVE_SPEED, -1, "save_speed", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, save_speed), \
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_RESTORE_SPEED, -1, "restore_speed", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, restore_speed), \
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_SYNCHRONOUS_MOTION_MODE, -1, "synchronous_motion_mode", \
						MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, synchronous_motion_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_SPEED_CHOICE_PARAMETERS, -1, "speed_choice_parameters", \
		MXFT_DOUBLE, NULL, 1, {MX_MOTOR_NUM_SPEED_CHOICE_PARAMS }, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, speed_choice_parameters), \
	{sizeof(double)}, NULL, 0}, \
  \
  {-1, -1, "raw_speed_choice_parameters", \
		MXFT_DOUBLE, NULL, 1, {MX_MOTOR_NUM_SPEED_CHOICE_PARAMS }, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, raw_speed_choice_parameters), \
	{sizeof(double)}, NULL, 0}, \
  \
  {MXLV_MTR_ACCELERATION_TYPE, -1, \
			"acceleration_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, acceleration_type), \
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_RAW_ACCELERATION_PARAMETERS, -1, "raw_acceleration_parameters", \
		MXFT_DOUBLE, NULL, 1, {MX_MOTOR_NUM_ACCELERATION_PARAMS }, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, raw_acceleration_parameters), \
	{sizeof(double)}, NULL, 0}, \
  \
  {MXLV_MTR_ACCELERATION_TIME, -1, "acceleration_time", \
		MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, acceleration_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_ACCELERATION_DISTANCE, -1, "acceleration_distance", \
		MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, acceleration_distance), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "parameter_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, parameter_type), \
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_BUSY, -1, "busy", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, busy), \
	{0}, NULL, MXFF_POLL}, \
  \
  {MXLV_MTR_SOFT_ABORT, -1, "soft_abort", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, soft_abort), \
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_IMMEDIATE_ABORT, -1, "immediate_abort", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, immediate_abort), \
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_NEGATIVE_LIMIT_HIT, -1, "negative_limit_hit", MXFT_BOOL, \
	NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, negative_limit_hit), \
	{0}, NULL, MXFF_POLL}, \
  \
  {MXLV_MTR_POSITIVE_LIMIT_HIT, -1, "positive_limit_hit", MXFT_BOOL, \
	NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, positive_limit_hit), \
	{0}, NULL, MXFF_POLL}, \
  \
  {MXLV_MTR_HOME_SEARCH, -1, "home_search", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, home_search), \
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_CONSTANT_VELOCITY_MOVE, -1, "constant_velocity_move", \
					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, constant_velocity_move), \
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_GET_STATUS, -1, "status", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, status), \
	{0}, NULL, MXFF_POLL, \
	0, 0, mx_motor_vctest_extended_status}, \
  \
  {MXLV_MTR_GET_EXTENDED_STATUS, -1, "extended_status", \
		MXFT_STRING, NULL, 1, {MXU_EXTENDED_STATUS_STRING_LENGTH+1}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, extended_status), \
	{sizeof(char)}, NULL, MXFF_POLL, \
	0, 0, mx_motor_vctest_extended_status}, \
  \
  {MXLV_MTR_COMPUTE_EXTENDED_SCAN_RANGE, -1, "compute_extended_scan_range", \
		MXFT_DOUBLE, NULL, 1, {MX_MOTOR_NUM_SCAN_RANGE_PARAMS}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_MOTOR, compute_extended_scan_range), \
	{sizeof(double)}, NULL, 0}, \
  \
  {-1, -1, "raw_compute_extended_scan_range", \
		MXFT_DOUBLE, NULL, 1, {MX_MOTOR_NUM_SCAN_RANGE_PARAMS}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_MOTOR, raw_compute_extended_scan_range), \
	{sizeof(double)}, NULL, 0}, \
  \
  {MXLV_MTR_COMPUTE_PSEUDOMOTOR_POSITION, -1, "compute_pseudomotor_position", \
		MXFT_DOUBLE, NULL, 1, {MX_MOTOR_NUM_POSITION_PARAMS}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_MOTOR, compute_pseudomotor_position), \
	{sizeof(double)}, NULL, 0}, \
  \
  {MXLV_MTR_COMPUTE_REAL_POSITION, -1, "compute_real_position", \
		MXFT_DOUBLE, NULL, 1, {MX_MOTOR_NUM_POSITION_PARAMS}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_MOTOR, compute_real_position), \
	{sizeof(double)}, NULL, 0}, \
  \
  {MXLV_MTR_SAVE_START_POSITIONS, -1, "save_start_positions", \
	  	MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, save_start_positions), \
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_USE_START_POSITIONS, -1, "use_start_positions", \
	  	MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, use_start_positions), \
	{0}, NULL, 0}, \
  \
  \
  \
  {MXLV_MTR_AXIS_ENABLE, -1, "axis_enable", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, axis_enable), \
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_CLOSED_LOOP, -1, "closed_loop", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, closed_loop), \
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_FAULT_RESET, -1, "fault_reset", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, fault_reset), \
	{0}, NULL, 0}, \
  \
  \
  \
  {MXLV_MTR_PROPORTIONAL_GAIN, -1, "proportional_gain", \
	        MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, proportional_gain), \
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_INTEGRAL_GAIN, -1, "integral_gain", \
	        MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, integral_gain), \
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_DERIVATIVE_GAIN, -1, "derivative_gain", \
	        MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, derivative_gain), \
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN, -1, "velocity_feedforward_gain", \
	        MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR, velocity_feedforward_gain), \
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN, -1, "acceleration_feedforward_gain",\
	        MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR,acceleration_feedforward_gain),\
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_INTEGRAL_LIMIT, -1, "integral_limit", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR,integral_limit),\
	{0}, NULL, 0}, \
  \
  {MXLV_MTR_EXTRA_GAIN, -1, "extra_gain", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MOTOR,extra_gain),\
	{0}, NULL, 0}

/*
 * The structure type MX_MOTOR_FUNCTION_LIST contains a list of
 * pointers to functions that vary from motor type to motor type.
 * By abstracting away the individual motor operations in this
 * fashion, it is possible to minimize device dependencies in
 * the main motor source code.
 */

typedef struct {
	mx_status_type ( *motor_is_busy )( MX_MOTOR *motor );
	mx_status_type ( *move_absolute )( MX_MOTOR *motor );
	mx_status_type ( *get_position )( MX_MOTOR *motor );
	mx_status_type ( *set_position )( MX_MOTOR *motor );
	mx_status_type ( *soft_abort )( MX_MOTOR *motor );
	mx_status_type ( *immediate_abort )( MX_MOTOR *motor );
	mx_status_type ( *positive_limit_hit )( MX_MOTOR *motor );
	mx_status_type ( *negative_limit_hit )( MX_MOTOR *motor );
	mx_status_type ( *find_home_position )( MX_MOTOR *motor );
	mx_status_type ( *constant_velocity_move )( MX_MOTOR *motor );
	mx_status_type ( *get_parameter )( MX_MOTOR *motor );
	mx_status_type ( *set_parameter )( MX_MOTOR *motor );
	mx_status_type ( *simultaneous_start )( long num_motor_records,
						MX_RECORD **motor_record_array,
						double *position_array,
						unsigned long flags );
	mx_status_type ( *get_status )( MX_MOTOR *motor );
	mx_status_type ( *get_extended_status )( MX_MOTOR *motor );
} MX_MOTOR_FUNCTION_LIST;

typedef mx_status_type
	(*MX_MOTOR_MOVE_REPORT_FUNCTION)(unsigned long, long, MX_RECORD **);

/* ============== Public functions and symbols . ================= */

MX_API_PRIVATE mx_status_type mx_motor_finish_record_initialization(
						MX_RECORD *motor_record );

MX_API_PRIVATE mx_status_type mx_motor_get_pointers( MX_RECORD *motor_record,
		MX_MOTOR **motor, MX_MOTOR_FUNCTION_LIST **function_list_ptr,
		const char *calling_fname );

MX_API mx_status_type mx_motor_is_busy( MX_RECORD *motor_record,
					mx_bool_type *busy );

MX_API mx_status_type mx_motor_soft_abort( MX_RECORD *motor_record );

MX_API mx_status_type mx_motor_immediate_abort( MX_RECORD *motor_record );

/* === Move by engineering units functions. === */

#define mx_motor_move_relative( r, rp, flags ) \
		mx_motor_move_relative_with_report( (r), (rp), NULL, (flags) );

#define mx_motor_move_absolute( r, p, flags ) \
		mx_motor_move_absolute_with_report( (r), (p), NULL, (flags) );

#define mx_motor_array_move_absolute( nr, mra, pa, flags ) \
		mx_motor_array_move_absolute_with_report( \
				(nr), (mra), (pa), NULL, (flags) );

#define mx_motor_array_simultaneous_start( nr, mra, pa, flags ) \
		mx_motor_array_move_absolute_with_report( \
				(nr), (mra), (pa), NULL, \
				(flags) | MXF_MTR_SIMULTANEOUS_START );

#define mx_motor_check_position_limits( r, p, flags ) \
	mx_motor_move_absolute_with_report( \
		(r), (p), NULL, (flags) | MXF_MTR_ONLY_CHECK_LIMITS );

MX_API mx_status_type mx_motor_move_relative_with_report(
			MX_RECORD *motor_record,
			double relative_position,
			MX_MOTOR_MOVE_REPORT_FUNCTION move_report_fn,
			unsigned long flags);

MX_API mx_status_type mx_motor_move_absolute_with_report(
			MX_RECORD *motor_record,
			double position,
			MX_MOTOR_MOVE_REPORT_FUNCTION move_report_fn,
			unsigned long flags);

MX_API mx_status_type mx_motor_array_move_absolute_with_report(
			long num_motor_records,
			MX_RECORD **motor_record_array,
			double *position,
			MX_MOTOR_MOVE_REPORT_FUNCTION move_report_fn,
			unsigned long flags );

MX_API mx_status_type mx_motor_array_internal_move_with_report(
			long num_motor_records,
			MX_RECORD **motor_record_array,
			double *position,
			MX_MOTOR_MOVE_REPORT_FUNCTION move_report_fn,
			unsigned long flags );

MX_API mx_status_type mx_wait_for_motor_stop(
			MX_RECORD *motor_record,
			unsigned long flags );

MX_API mx_status_type mx_wait_for_motor_array_stop(
			long num_motor_records,
			MX_RECORD **motor_record_array,
			unsigned long flags );

MX_API mx_status_type mx_motor_internal_move_absolute( MX_RECORD *motor_record,
							double destination );

MX_API mx_status_type mx_motor_get_position( MX_RECORD *motor_record,
							double *position );
MX_API mx_status_type mx_motor_set_position( MX_RECORD *motor_record, 
							double position );

MX_API mx_status_type mx_motor_positive_limit_hit(
				MX_RECORD *motor_record,
				mx_bool_type *limit_hit );

MX_API mx_status_type mx_motor_negative_limit_hit(
				MX_RECORD *motor_record,
				mx_bool_type *limit_hit );

MX_API mx_status_type mx_motor_find_home_position(
				MX_RECORD *motor_record, long direction );

MX_API mx_status_type mx_motor_zero_position_value( MX_RECORD *motor_record );

MX_API mx_status_type mx_motor_constant_velocity_move(
				MX_RECORD *motor_record, long direction );

MX_API mx_status_type mx_motor_get_status( MX_RECORD *motor_record,
						unsigned long *motor_status );

MX_API mx_status_type mx_motor_get_extended_status( MX_RECORD *motor_record,
						double *motor_position,
						unsigned long *motor_status );

MX_API mx_status_type mx_motor_get_traditional_status( MX_MOTOR *motor );

MX_API void           mx_motor_set_traditional_status( MX_MOTOR *motor );

MX_API mx_status_type mx_motor_get_parameter( MX_RECORD *motor_record,
						long parameter_type );

MX_API mx_status_type mx_motor_set_parameter( MX_RECORD *motor_record,
						long parameter_type );

MX_API mx_status_type mx_motor_default_get_parameter_handler( MX_MOTOR *motor );

MX_API mx_status_type mx_motor_default_set_parameter_handler( MX_MOTOR *motor );

MX_API mx_status_type mx_motor_update_speed_and_acceleration_parameters(
							MX_MOTOR *motor );

MX_API mx_status_type mx_motor_get_speed( MX_RECORD *motor_record,
						double *speed );

MX_API mx_status_type mx_motor_set_speed( MX_RECORD *motor_record,
						double speed );

MX_API mx_status_type mx_motor_get_base_speed( MX_RECORD *motor_record,
						double *base_speed );

MX_API mx_status_type mx_motor_set_base_speed( MX_RECORD *motor_record,
						double base_speed );

MX_API mx_status_type mx_motor_get_maximum_speed( MX_RECORD *motor_record,
						double *maximum_speed );

MX_API mx_status_type mx_motor_set_maximum_speed( MX_RECORD *motor_record,
						double maximum_speed );

MX_API mx_status_type mx_motor_get_raw_speed( MX_RECORD *motor_record,
						double *raw_speed );

MX_API mx_status_type mx_motor_set_raw_speed( MX_RECORD *motor_record,
						double raw_speed );

MX_API mx_status_type mx_motor_save_speed( MX_RECORD *motor_record );

MX_API mx_status_type mx_motor_restore_speed( MX_RECORD *motor_record );

MX_API mx_status_type mx_motor_get_synchronous_motion_mode(
					MX_RECORD *motor_record,
					mx_bool_type *mode );

MX_API mx_status_type mx_motor_set_synchronous_motion_mode(
					MX_RECORD *motor_record,
					mx_bool_type mode );

MX_API mx_status_type mx_motor_set_speed_between_positions(
					MX_RECORD *motor_record,
					double position1, double position2,
					double time_for_move );

MX_API mx_status_type mx_motor_get_acceleration_type( MX_RECORD *motor_record,
						long *acceleration_type );

MX_API mx_status_type mx_motor_get_raw_acceleration_parameters(
				MX_RECORD *motor_record,
				double *raw_acceleration_parameter_array );

MX_API mx_status_type mx_motor_set_raw_acceleration_parameters(
				MX_RECORD *motor_record,
				double *raw_acceleration_parameter_array );

MX_API mx_status_type mx_motor_get_acceleration_time(
				MX_RECORD *motor_record,
				double *acceleration_time );

MX_API mx_status_type mx_motor_get_acceleration_distance(
				MX_RECORD *motor_record,
				double *acceleration_distance );

MX_API mx_status_type mx_motor_set_acceleration_time(
				MX_RECORD *motor_record,
				double acceleration_time );

MX_API mx_status_type mx_motor_send_control_command( MX_RECORD *motor_record,
					int command_type, int command );

MX_API mx_status_type mx_motor_get_gain( MX_RECORD *motor_record,
					int gain_type, double *gain );

MX_API mx_status_type mx_motor_set_gain( MX_RECORD *motor_record,
					int gain_type, double gain );

MX_API mx_status_type mx_motor_home_search_succeeded(
				MX_RECORD *motor_record,
				mx_bool_type *home_search_succeeded );

MX_API mx_status_type mx_motor_compute_extended_scan_range(
				MX_RECORD *motor_record,
				double start_position,
				double end_position,
				double *real_start_position,
				double *real_end_position );

MX_API mx_status_type mx_motor_compute_pseudomotor_position_from_real_position(
				MX_RECORD *motor_record,
				double real_position,
				double *pseudomotor_position,
				int recursion_flag );

MX_API mx_status_type mx_motor_compute_real_position_from_pseudomotor_position(
				MX_RECORD *motor_record,
				double pseudomotor_position,
				double *real_position,
				int recursion_flag );

MX_API mx_status_type mx_motor_get_real_motor_record(
				MX_RECORD *pseudomotor_record,
				MX_RECORD **real_motor_record );

MX_API mx_status_type mx_alternate_motor_can_use_this_motors_mce(
				MX_RECORD *motor_record,
				MX_RECORD *alternate_motor_record,
				int *motor_is_compatible );

MX_API mx_status_type mx_is_motor_position_between_software_limits(
			MX_RECORD *motor_record, double position,
			int *result_flag, int generate_error_message );

MX_API mx_status_type mx_motor_save_start_positions(
			MX_RECORD *motor_record, double start_position );

MX_API mx_status_type mx_motor_use_start_positions( MX_RECORD *motor_record );

/* === Move by steps functions. (MXC_MTR_STEPPER) === */

MX_API mx_status_type mx_motor_move_relative_steps_with_report(
			MX_RECORD *motor_record,
			long motor_steps,
			MX_MOTOR_MOVE_REPORT_FUNCTION move_report_fn,
			unsigned long flags);

MX_API mx_status_type mx_motor_move_absolute_steps_with_report(
			MX_RECORD *motor_record,
			long motor_steps,
			MX_MOTOR_MOVE_REPORT_FUNCTION move_report_fn,
			unsigned long flags);

MX_API mx_status_type mx_motor_get_position_steps(
				MX_RECORD *motor_record, long *motor_steps );
MX_API mx_status_type mx_motor_set_position_steps(
				MX_RECORD *motor_record, long motor_steps );

MX_API mx_status_type mx_motor_steps_to_go(
				MX_RECORD *motor_record, long *steps_to_go );

MX_API mx_status_type
		mx_is_stepper_motor_position_between_software_limits(
			MX_RECORD *motor_record, long step_position,
			int *result_flag, int generate_error_message );

/* === Move analog functions. (MXC_MTR_ANALOG) === */

MX_API mx_status_type mx_motor_move_absolute_analog_with_report(
			MX_RECORD *motor_record,
			double motor_position,
			MX_MOTOR_MOVE_REPORT_FUNCTION move_report_fn,
			unsigned long flags);

MX_API mx_status_type mx_motor_get_position_analog(
				MX_RECORD *motor_record, double *position );
MX_API mx_status_type mx_motor_set_position_analog(
				MX_RECORD *motor_record, double position );

MX_API mx_status_type
		mx_is_analog_motor_position_between_software_limits(
			MX_RECORD *motor_record, double analog_position,
			int *result_flag, int generate_error_message );

/* === */

MX_API_PRIVATE mx_status_type mx_motor_vctest_extended_status(
					MX_RECORD_FIELD *, mx_bool_type * );

extern MX_RECORD_FUNCTION_LIST mx_motor_record_function_list;

#ifdef __cplusplus
}
#endif

#endif /* __MX_MOTOR_H__ */
