/*
 * Name:    mx_bluice.h
 *
 * Purpose: Header file for communication with Blu-Ice DCSS and DHS
 *          servers by MX.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------
 *
 * Copyright 2005-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_BLUICE_H__
#define __MX_BLUICE_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#include "mx_socket.h"
#include "mx_mutex.h"
#include "mx_thread.h"
#include "mx_motor.h"
#include "mx_analog_input.h"
#include "mx_timer.h"
#include "mx_relay.h"
#include "mx_variable.h"

#define MX_BLUICE_MSGHDR_TEXT_LENGTH	12
#define MX_BLUICE_MSGHDR_BINARY_LENGTH	13

#define MX_BLUICE_MSGHDR_LENGTH		( MX_BLUICE_MSGHDR_TEXT_LENGTH \
					  + MX_BLUICE_MSGHDR_BINARY_LENGTH + 1 )

#define MX_BLUICE_MSGHDR_TEXT		0
#define MX_BLUICE_MSGHDR_BINARY		MX_BLUICE_MSGHDR_TEXT_LENGTH
#define MX_BLUICE_MSGHDR_NULL		( MX_BLUICE_MSGHDR_LENGTH - 1 )

#define MX_BLUICE_PROTOCOL_1		1
#define MX_BLUICE_PROTOCOL_2		2

#define MX_BLUICE_OLD_MESSAGE_LENGTH	200

/* ----- */

#define MXU_BLUICE_NAME_LENGTH		40
#define MXU_BLUICE_STRING_LENGTH	200

#define MX_BLUICE_ARRAY_BLOCK_SIZE	10
#define MX_BLUICE_INITIAL_RECEIVE_BUFFER_LENGTH	1000

/* ----- */

typedef struct {
	char counter_name[MXU_BLUICE_NAME_LENGTH+1];
	long channel_number;
	char timer_name[MXU_BLUICE_NAME_LENGTH+1];
	char timer_type[MXU_BLUICE_NAME_LENGTH+1];

	double value;

	MX_ANALOG_INPUT *mx_analog_input;
	MX_TIMER *mx_timer;
} MX_BLUICE_FOREIGN_ION_CHAMBER;

typedef struct {
	char dhs_name[MXU_BLUICE_NAME_LENGTH+1];
	mx_bool_type is_pseudo;
	double position;		/* scaled */
	double upper_limit;		/* scaled */
	double lower_limit;		/* scaled */
	double scale_factor;					/* real only */
	double speed;			/* steps/second */	/* real only */
	double acceleration_time;	/* seconds */		/* real only */
	double backlash;		/* steps */		/* real only */
	mx_bool_type lower_limit_on;
	mx_bool_type upper_limit_on;
	mx_bool_type motor_lock_on;
	mx_bool_type backlash_on;				/* real only */
	mx_bool_type reverse_on;				/* real only */

	/* The following are non-Blu-Ice fields for the use of MX. */

	MX_MOTOR *mx_motor;
	mx_bool_type move_in_progress;
} MX_BLUICE_FOREIGN_MOTOR;

/* Note: The 'operation_counter' field in MX_BLUICE_FOREIGN_OPERATION is
 * merely a copy of the operation counter as seen when Blu-Ice messages
 * referring to this operation are sent or received.  The authoritative
 * source for the most recent operation counter is found instead in
 * either the MX_BLUICE_DCSS_SERVER structure or the MX_BLUICE_DHS_MANAGER
 * structure.
 */

/* Values for 'operation_state' */

#define MXSF_BLUICE_OPERATION_NETWORK_ERROR		(-2)
#define MXSF_BLUICE_OPERATION_ERROR			(-1)
#define MXSF_BLUICE_OPERATION_COMPLETED			0
#define MXSF_BLUICE_OPERATION_STARTED			1
#define MXSF_BLUICE_OPERATION_UPDATED			2
#define MXSF_BLUICE_OPERATION_UPDATE_ACKNOWLEDGED	3

typedef struct {
	char *arguments_buffer;
	size_t arguments_length;

	MX_VARIABLE *mx_operation_variable;
	MX_RECORD *mx_record;

	unsigned long client_number;
	unsigned long operation_counter;
	int operation_state;
} MX_BLUICE_FOREIGN_OPERATION;

typedef struct {
	long shutter_status;

	MX_RELAY *mx_relay;
} MX_BLUICE_FOREIGN_SHUTTER;

typedef struct {
	char *string_buffer;
	size_t string_length;
	char status[20];

	MX_VARIABLE *mx_string_variable;
} MX_BLUICE_FOREIGN_STRING;

#define MXT_BLUICE_FOREIGN_UNKNOWN	0
#define MXT_BLUICE_FOREIGN_ION_CHAMBER	1
#define MXT_BLUICE_FOREIGN_MOTOR	2
#define MXT_BLUICE_FOREIGN_OPERATION	3
#define MXT_BLUICE_FOREIGN_SHUTTER	4
#define MXT_BLUICE_FOREIGN_STRING	5

typedef struct {
	long foreign_type;
	char name[MXU_BLUICE_NAME_LENGTH+1];
	char dhs_server_name[MXU_BLUICE_NAME_LENGTH+1];

	struct mx_bluice_server_type *bluice_server;

	union {
		MX_BLUICE_FOREIGN_ION_CHAMBER ion_chamber;
		MX_BLUICE_FOREIGN_MOTOR motor;
		MX_BLUICE_FOREIGN_OPERATION operation;
		MX_BLUICE_FOREIGN_SHUTTER shutter;
		MX_BLUICE_FOREIGN_STRING string;
	} u;
} MX_BLUICE_FOREIGN_DEVICE;

/* ----- */

typedef struct mx_bluice_server_type {
	MX_RECORD *record;

	MX_SOCKET *socket;

	/* socket_send_mutex is used to prevent garbled messages from 
	 * being sent to the Blu-Ice server's socket by multiple threads
	 * trying to send at the same time.
	 *
	 * Received messages do not need a mutex since only the receive
	 * thread is allowed to receive messages from the server.
	 */

	MX_MUTEX *socket_send_mutex;

	/* foreign_data_mutex is used to protect the foreign data arrays
	 * from corruption.
	 */

	MX_MUTEX *foreign_data_mutex;

	char *receive_buffer;
	long receive_buffer_length;
	long num_received_bytes;

	long protocol_version;

	long num_ion_chambers;
	MX_BLUICE_FOREIGN_DEVICE **ion_chamber_array;

	long num_motors;
	MX_BLUICE_FOREIGN_DEVICE **motor_array;

	long num_operations;
	MX_BLUICE_FOREIGN_DEVICE **operation_array;

	long num_shutters;
	MX_BLUICE_FOREIGN_DEVICE **shutter_array;

	long num_strings;
	MX_BLUICE_FOREIGN_DEVICE **string_array;
} MX_BLUICE_SERVER;

/* ----- */

MX_API mx_status_type
mx_bluice_send_message( MX_RECORD *bluice_server_record,
			char *text_data,
			char *binary_data,
			long binary_data_length );

MX_API mx_status_type
mx_bluice_receive_message( MX_RECORD *bluice_server_record,
				char *data_buffer,
				long data_buffer_size,
				long *actual_data_length,
				double timeout_in_seconds );

/* ----- */

MX_API mx_status_type
mx_bluice_setup_device_pointer( MX_BLUICE_SERVER *bluice_server,
			char *name,
			MX_BLUICE_FOREIGN_DEVICE ***foreign_device_array_ptr,
			long *num_foreign_devices_ptr,
			MX_BLUICE_FOREIGN_DEVICE **foreign_device_ptr );

MX_API mx_status_type
mx_bluice_get_device_pointer( MX_BLUICE_SERVER *bluice_server,
			char *name,
			MX_BLUICE_FOREIGN_DEVICE **foreign_device_array,
			long num_foreign_devices,
			MX_BLUICE_FOREIGN_DEVICE **foreign_device_ptr );

MX_API mx_status_type
mx_bluice_wait_for_device_pointer_initialization(
			MX_BLUICE_SERVER *bluice_server,
			char *name,
			long bluice_foreign_type,
			MX_BLUICE_FOREIGN_DEVICE ***foreign_device_array_ptr,
			long *num_foreign_devices_ptr,
			MX_BLUICE_FOREIGN_DEVICE **foreign_device_ptr,
			double timeout_in_seconds );

/* ----- */

MX_API mx_status_type
mx_bluice_is_master( MX_BLUICE_SERVER *bluice_server,
			mx_bool_type *master_flag );

MX_API mx_status_type
mx_bluice_take_master( MX_BLUICE_SERVER *bluice_server,
			mx_bool_type take_master );

MX_API mx_status_type
mx_bluice_check_for_master( MX_BLUICE_SERVER *bluice_server );

/* ----- */

MX_API unsigned long
mx_bluice_get_client_number( MX_BLUICE_SERVER *bluice_server );

MX_API unsigned long
mx_bluice_update_operation_counter( MX_BLUICE_SERVER *bluice_server );

/* ----- */

MX_API int
mx_bluice_get_operation_state( MX_BLUICE_FOREIGN_DEVICE *operation );

MX_API mx_status_type
mx_bluice_set_operation_state( MX_BLUICE_FOREIGN_DEVICE *operation,
				int operation_state );

/* ----- */

MX_API mx_status_type
mx_bluice_update_motion_status( MX_BLUICE_SERVER *bluice_server,
				char *motion_status_message,
				mx_bool_type move_in_progress );

MX_API mx_status_type
mx_bluice_update_operation_status( MX_BLUICE_SERVER *bluice_server,
				char *operation_status_message );

#ifdef __cplusplus
}
#endif

#endif /* __MX_BLUICE_H__ */

