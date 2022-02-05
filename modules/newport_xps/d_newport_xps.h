/*
 * Name:    d_newport_xps.h
 *
 * Purpose: Header file for motors controlled by Newport XPS controllers.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2014-2015, 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NEWPORT_XPS_H__
#define __D_NEWPORT_XPS_H__

#include "mx_thread.h"
#include "mx_mutex.h"
#include "mx_condition_variable.h"

#define MXU_NEWPORT_XPS_POSITIONER_NAME_LENGTH  80

#define MXU_NEWPORT_XPS_GROUP_STATUS_LENGTH	250

#define MXU_NETWORK_XPS_PCO_CONFIG_NAME_LENGTH  80
#define MXU_NETWORK_XPS_PCO_CONFIG_VALUE_LENGTH 80

/* Flag bits for 'newport_xps_motor_flags'. */

#define MXF_NEWPORT_XPS_MOTOR_DISABLE			0x1
#define MXF_NEWPORT_XPS_MOTOR_APPLY_USER_TRAVEL_LIMITS	0x2

/* Command types for the move thread. */

#define MXT_NEWPORT_XPS_GROUP_MOVE_ABSOLUTE	1
#define MXT_NEWPORT_XPS_GROUP_HOME_SEARCH	2
#define MXT_NEWPORT_XPS_GROUP_SET_POSITION	3

/*---*/

typedef struct {
	MX_RECORD *record;

	MX_RECORD *newport_xps_record;
	char positioner_name[MXU_NEWPORT_XPS_POSITIONER_NAME_LENGTH+1];
	unsigned long newport_xps_motor_flags;

	double socket_send_timeout;
	double socket_receive_timeout;

	char group_name[MXU_NEWPORT_XPS_POSITIONER_NAME_LENGTH+1];

	long group_status;
	char group_status_string[MXU_NEWPORT_XPS_GROUP_STATUS_LENGTH+1];

	unsigned long positioner_error;
	unsigned long hardware_status;
	unsigned long driver_status;

	mx_bool_type home_search_succeeded;

	/* 'array_index' is an index into the motor axis data structures
	 * found in the controller's 'newport_xps' record.
	 */

	long array_index;

	/* We need to keep a list of the motor records that are in the same
	 * group as this motor.  This is used by the 'set_position' driver
	 * method.
	 */

	long num_motors_in_group;
	MX_RECORD **motor_records_in_group;
	double *old_motor_positions_in_group;

	/* During a 'set_position' operation, the motor group that the
	 * motor is in will be killed and then reinitialized.  After this
	 * is done, the driver will wait for 'set_position_sleep_ms' in
	 * milliseconds before going on to reprogram the motor position.
	 */

	double set_position_sleep_ms;

	/* Move commands _block_, so they have to have their own
	 * separate socket and thread in order to avoid having
	 * the entire 'newport_xps' module block during a move.
	 */

	int move_thread_socket_id;

	MX_THREAD *move_thread;
	MX_MUTEX *move_thread_mutex;
	MX_CONDITION_VARIABLE *move_thread_cv;
	mx_bool_type move_in_progress;

	unsigned long command_type;
	double command_destination;

	/* Configuration for the PCO connector. */

	char pco_config_name[ MXU_NETWORK_XPS_PCO_CONFIG_NAME_LENGTH + 1 ];
	char pco_config_value[ MXU_NETWORK_XPS_PCO_CONFIG_VALUE_LENGTH + 1 ];

	double user_travel_limits[2];
	double user_travel_limits_guard_band[2];

	double raw_user_travel_limits[2];
	double raw_user_travel_limits_guard_band[2];
} MX_NEWPORT_XPS_MOTOR;

MX_API mx_status_type mxd_newport_xps_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_newport_xps_open( MX_RECORD *record );
MX_API mx_status_type mxd_newport_xps_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxd_newport_xps_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_newport_xps_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_xps_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_xps_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_xps_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_xps_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_xps_raw_home_command( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_xps_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_xps_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_xps_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_xps_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_newport_xps_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_newport_xps_motor_function_list;

extern long mxd_newport_xps_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_newport_xps_rfield_def_ptr;

#define MXLV_NEWPORT_XPS_MOTOR_SOCKET_SEND_TIMEOUT		88001
#define MXLV_NEWPORT_XPS_MOTOR_SOCKET_RECEIVE_TIMEOUT		88002
#define MXLV_NEWPORT_XPS_GROUP_STATUS				88003
#define MXLV_NEWPORT_XPS_GROUP_STATUS_STRING			88004
#define MXLV_NEWPORT_XPS_POSITIONER_ERROR			88005
#define MXLV_NEWPORT_XPS_HARDWARE_STATUS			88006
#define MXLV_NEWPORT_XPS_DRIVER_STATUS				88007
#define MXLV_NEWPORT_XPS_MOVE_THREAD_SOCKET_ID			88008
#define MXLV_NEWPORT_XPS_PCO_CONFIG_NAME			88009
#define MXLV_NEWPORT_XPS_PCO_CONFIG_VALUE			88010
#define MXLV_NEWPORT_XPS_USER_TRAVEL_LIMITS			88011
#define MXLV_NEWPORT_XPS_RAW_USER_TRAVEL_LIMITS			88012
#define MXLV_NEWPORT_XPS_USER_TRAVEL_LIMITS_GUARD_BAND		88013
#define MXLV_NEWPORT_XPS_RAW_USER_TRAVEL_LIMITS_GUARD_BAND	88014

#define MXD_NEWPORT_XPS_MOTOR_STANDARD_FIELDS \
  {-1, -1, "newport_xps_record", MXFT_RECORD, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_NEWPORT_XPS_MOTOR, newport_xps_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY) }, \
  \
  {-1, -1, "positioner_name", MXFT_STRING, NULL, \
				1, {MXU_NEWPORT_XPS_POSITIONER_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS_MOTOR, positioner_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY) }, \
  \
  {-1, -1, "newport_xps_motor_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_NEWPORT_XPS_MOTOR, newport_xps_motor_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY) }, \
  \
  {MXLV_NEWPORT_XPS_MOTOR_SOCKET_SEND_TIMEOUT, -1, "socket_send_timeout", \
					MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_NEWPORT_XPS_MOTOR, socket_send_timeout), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {MXLV_NEWPORT_XPS_MOTOR_SOCKET_RECEIVE_TIMEOUT, -1, "socket_receive_timeout",\
					MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_NEWPORT_XPS_MOTOR, socket_receive_timeout),\
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "group_name", MXFT_STRING, NULL, \
				1, {MXU_NEWPORT_XPS_POSITIONER_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS_MOTOR, group_name), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_NEWPORT_XPS_GROUP_STATUS, -1, "group_status", \
			MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS_MOTOR, group_status), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_NEWPORT_XPS_GROUP_STATUS_STRING, -1, "group_status_string",\
		MXFT_STRING, NULL, 1, {MXU_NEWPORT_XPS_GROUP_STATUS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_NEWPORT_XPS_MOTOR, group_status_string), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_NEWPORT_XPS_POSITIONER_ERROR, -1, "positioner_error", \
						MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS_MOTOR, positioner_error), \
	{0}, NULL, 0 }, \
  \
  {MXLV_NEWPORT_XPS_HARDWARE_STATUS, -1, "hardware_status", \
						MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS_MOTOR, hardware_status), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_NEWPORT_XPS_DRIVER_STATUS, -1, "driver_status", \
						MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS_MOTOR, driver_status), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "home_search_succeeded", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_NEWPORT_XPS_MOTOR, home_search_succeeded), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "set_position_sleep_ms", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_NEWPORT_XPS_MOTOR, set_position_sleep_ms), \
	{0}, NULL, 0 }, \
  \
  {MXLV_NEWPORT_XPS_MOVE_THREAD_SOCKET_ID, -1, "move_thread_socket_id", \
						MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_NEWPORT_XPS_MOTOR, move_thread_socket_id), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_NEWPORT_XPS_PCO_CONFIG_NAME, -1, "pco_config_name", MXFT_STRING, NULL,\
				1, {MXU_NETWORK_XPS_PCO_CONFIG_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_NEWPORT_XPS_MOTOR, pco_config_name), \
	{sizeof(char)}, NULL, 0 }, \
  \
  {MXLV_NEWPORT_XPS_PCO_CONFIG_VALUE, -1, "pco_config_value", \
	    MXFT_STRING, NULL, 1, {MXU_NETWORK_XPS_PCO_CONFIG_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS_MOTOR, pco_config_value),\
	{sizeof(char)}, NULL, 0 }, \
  \
  {MXLV_NEWPORT_XPS_USER_TRAVEL_LIMITS, -1, "user_travel_limits", \
						MXFT_DOUBLE, NULL, 1, {2}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_NEWPORT_XPS_MOTOR, user_travel_limits), \
	{sizeof(double)}, NULL, 0 }, \
  \
  {MXLV_NEWPORT_XPS_RAW_USER_TRAVEL_LIMITS, -1, "raw_user_travel_limits", \
						MXFT_DOUBLE, NULL, 1, {2}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_NEWPORT_XPS_MOTOR, raw_user_travel_limits), \
	{sizeof(double)}, NULL, 0 }

#endif /* __D_NEWPORT_XPS_H__ */
