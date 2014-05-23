/*
 * Name:    d_newport_xps.h
 *
 * Purpose: Header file for motors controlled by Newport XPS controllers.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
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

/*---*/

/* Command types for the move thread. */

#define MXT_NEWPORT_XPS_GROUP_MOVE_ABSOLUTE	1
#define MXT_NEWPORT_XPS_GROUP_HOME_SEARCH	2

/*---*/

typedef struct {
	MX_RECORD *record;

	MX_RECORD *newport_xps_record;
	char positioner_name[MXU_NEWPORT_XPS_POSITIONER_NAME_LENGTH+1];

	char group_name[MXU_NEWPORT_XPS_POSITIONER_NAME_LENGTH+1];

	long group_status;
	char group_status_string[MXU_NEWPORT_XPS_GROUP_STATUS_LENGTH+1];

	unsigned long positioner_error;
	unsigned long hardware_status;
	unsigned long driver_status;

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
} MX_NEWPORT_XPS_MOTOR;

MX_API mx_status_type mxd_newport_xps_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_newport_xps_open( MX_RECORD *record );
MX_API mx_status_type mxd_newport_xps_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxd_newport_xps_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_newport_xps_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_xps_get_position( MX_MOTOR *motor );
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

#define MXLV_NEWPORT_XPS_GROUP_STATUS		88001
#define MXLV_NEWPORT_XPS_GROUP_STATUS_STRING	88002
#define MXLV_NEWPORT_XPS_POSITIONER_ERROR	88003
#define MXLV_NEWPORT_XPS_HARDWARE_STATUS	88004
#define MXLV_NEWPORT_XPS_DRIVER_STATUS		88005

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
	{0}, NULL, MXFF_READ_ONLY }

#endif /* __D_NEWPORT_XPS_H__ */
