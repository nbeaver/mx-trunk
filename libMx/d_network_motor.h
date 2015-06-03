/*
 * Name:    d_network_motor.h 
 *
 * Purpose: Header file for MX motor driver for motors controlled
 *          via an MX network server.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2004, 2009-2010, 2013-2015
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NETWORK_MOTOR_H__
#define __D_NETWORK_MOTOR_H__

#include "mx_motor.h"
#include "mx_net.h"

#define MX_VERSION_HAS_MOTOR_GET_STATUS    65000UL

typedef struct {
	MX_RECORD *record;

	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	/* Unconditionally fetching a lot of configuration information
	 * about every network motor can slow down a client that will
	 * only need to talk to a few of them.  As a performance
	 * optimization we use the 'need_to_get_remote_record_information'
	 * flag to tell the driver to fetch the configuration information
	 * the next time it needs to communicate with the remote record.
	 */

	int need_to_get_remote_record_information;

	/*********************************************************
	 * WARNING, WARNING, WARNING, WARNING, WARNING, WARNING  *
	 *                                                       *
	 * The 'remote_driver_type' value is the MX type of the  *
	 * remote record's driver as listed in the _CLIENT_'s    *
	 * MX driver table.  The numerical value of MX driver    *
	 * types are NOT, repeat NOT, guaranteed to be the same  *
	 * in the server's driver table as they are in the       *
	 * client's driver table.  It is even possible for the   *
	 * MX server to implement a driver type that the client  *
	 * DOES NOT KNOW ABOUT.  So don't go assuming that the   *
	 * numerical value of the driver type will be the same   *
	 * in the client as in the server, because sooner or     *
	 * later you will encounter a case where they are        *
	 * different.                                            *
	 *                                                       *
	 * Incidentally, if the server is using a driver type    *
	 * that the client does not recognize, the value of the  *
	 * 'remote_driver_type' field will be set to -1.         *
	 *                                                       *
	 * So, if you are tempted to write application or driver *
	 * code that assumes that driver types are the same in   *
	 * all MX processes on a beamline, then eventually your  *
	 * code will fail.  Remember, you have been warned.      *
	 ********************************************************/

	long remote_driver_type;
	char remote_driver_name[ MXU_DRIVER_NAME_LENGTH+1 ];
	double remote_scale;

	/* On the other hand, you _can_ count on motor flag bit
	 * definitions being the same in both servers and clients.
	 */

	unsigned long remote_motor_flags;

	MX_NETWORK_FIELD acceleration_distance_nf;
	MX_NETWORK_FIELD acceleration_feedforward_gain_nf;
	MX_NETWORK_FIELD acceleration_time_nf;
	MX_NETWORK_FIELD acceleration_type_nf;
	MX_NETWORK_FIELD axis_enable_nf;
	MX_NETWORK_FIELD base_speed_nf;
	MX_NETWORK_FIELD busy_nf;
	MX_NETWORK_FIELD closed_loop_nf;
	MX_NETWORK_FIELD compute_extended_scan_range_nf;
	MX_NETWORK_FIELD compute_pseudomotor_position_nf;
	MX_NETWORK_FIELD compute_real_position_nf;
	MX_NETWORK_FIELD constant_velocity_move_nf;
	MX_NETWORK_FIELD derivative_gain_nf;
	MX_NETWORK_FIELD destination_nf;
	MX_NETWORK_FIELD extended_status_nf;
	MX_NETWORK_FIELD extra_gain_nf;
	MX_NETWORK_FIELD fault_reset_nf;
	MX_NETWORK_FIELD home_search_nf;
	MX_NETWORK_FIELD immediate_abort_nf;
	MX_NETWORK_FIELD integral_gain_nf;
	MX_NETWORK_FIELD integral_limit_nf;
	MX_NETWORK_FIELD limit_switch_as_home_switch_nf;
	MX_NETWORK_FIELD maximum_speed_nf;
	MX_NETWORK_FIELD motor_flags_nf;
	MX_NETWORK_FIELD mx_type_nf;
	MX_NETWORK_FIELD negative_limit_hit_nf;
	MX_NETWORK_FIELD position_nf;
	MX_NETWORK_FIELD positive_limit_hit_nf;
	MX_NETWORK_FIELD proportional_gain_nf;
	MX_NETWORK_FIELD raw_acceleration_parameters_nf;
	MX_NETWORK_FIELD restore_speed_nf;
	MX_NETWORK_FIELD resynchronize_nf;
	MX_NETWORK_FIELD save_speed_nf;
	MX_NETWORK_FIELD save_start_positions_nf;
	MX_NETWORK_FIELD scale_nf;
	MX_NETWORK_FIELD set_position_nf;
	MX_NETWORK_FIELD soft_abort_nf;
	MX_NETWORK_FIELD speed_nf;
	MX_NETWORK_FIELD speed_choice_parameters_nf;
	MX_NETWORK_FIELD status_nf;
	MX_NETWORK_FIELD synchronous_motion_mode_nf;
	MX_NETWORK_FIELD use_start_positions_nf;
	MX_NETWORK_FIELD velocity_feedforward_gain_nf;

} MX_NETWORK_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_network_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_network_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_network_motor_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_network_motor_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_network_motor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_network_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_network_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_network_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_network_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_network_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_network_motor_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_network_motor_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_network_motor_raw_home_command( MX_MOTOR *motor );
MX_API mx_status_type mxd_network_motor_constant_velocity_move(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_network_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_network_motor_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_network_motor_get_status( MX_MOTOR *motor );
MX_API mx_status_type mxd_network_motor_get_extended_status( MX_MOTOR *motor );

MX_API mx_status_type mxd_network_motor_get_pointers( MX_MOTOR *motor,
				MX_NETWORK_MOTOR **network_motor,
				const char *calling_fname );

extern MX_RECORD_FUNCTION_LIST mxd_network_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_network_motor_motor_function_list;

extern long mxd_network_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_network_motor_rfield_def_ptr;

#define MXD_NETWORK_MOTOR_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_MOTOR, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
	NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_MOTOR, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "remote_driver_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_MOTOR, remote_driver_type), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "remote_driver_name", MXFT_STRING, \
	NULL, 1, {MXU_DRIVER_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_MOTOR, remote_driver_name), \
	{sizeof(char)}, NULL, 0}

#endif /* __D_NETWORK_MOTOR_H__ */
