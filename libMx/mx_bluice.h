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
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_BLUICE_H__
#define __MX_BLUICE_H__

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

/* ----- */

#define MXU_BLUICE_NAME_LENGTH		40

#define MX_BLUICE_ARRAY_BLOCK_SIZE	10
#define MX_BLUICE_INITIAL_RECEIVE_BUFFER_LENGTH	1000

/* ----- */

#define MXF_BLUICE_TIMER_UNKNOWN	(-1)
#define MXF_BLUICE_TIMER_CLOCK		1

typedef struct {
	char counter_name[MXU_BLUICE_NAME_LENGTH+1];
	int channel_number;
	char timer_name[MXU_BLUICE_NAME_LENGTH+1];
	int timer_type;

	double value;

	MX_ANALOG_INPUT *mx_analog_input;
	MX_TIMER *mx_timer;
} MX_BLUICE_FOREIGN_ION_CHAMBER;

typedef struct {
	char dhs_name[MXU_BLUICE_NAME_LENGTH+1];
	int is_pseudo;
	double position;		/* scaled */
	double upper_limit;		/* scaled */
	double lower_limit;		/* scaled */
	double scale_factor;					/* real only */
	double speed;			/* steps/second */	/* real only */
	double acceleration_time;	/* seconds */		/* real only */
	double backlash;		/* steps */		/* real only */
	int lower_limit_on;
	int upper_limit_on;
	int motor_lock_on;
	int backlash_on;					/* real only */
	int reverse_on;						/* real only */

	/* The following are non-Blu-Ice fields for the use of MX. */

	MX_MOTOR *mx_motor;
	int move_in_progress;
} MX_BLUICE_FOREIGN_MOTOR;

typedef struct {
	int shutter_status;

	MX_RELAY *mx_relay;
} MX_BLUICE_FOREIGN_SHUTTER;

typedef struct {
	unsigned long string_length;
	char *string_contents;

	MX_VARIABLE *mx_string_variable;
} MX_BLUICE_FOREIGN_STRING;

#define MXT_BLUICE_FOREIGN_UNKNOWN	0
#define MXT_BLUICE_FOREIGN_ION_CHAMBER	1
#define MXT_BLUICE_FOREIGN_MOTOR	2
#define MXT_BLUICE_FOREIGN_SHUTTER	3
#define MXT_BLUICE_FOREIGN_STRING	4

typedef struct {
	int foreign_type;
	char name[MXU_BLUICE_NAME_LENGTH+1];
	char dhs_server_name[MXU_BLUICE_NAME_LENGTH+1];

	union {
		MX_BLUICE_FOREIGN_ION_CHAMBER ion_chamber;
		MX_BLUICE_FOREIGN_MOTOR motor;
		MX_BLUICE_FOREIGN_SHUTTER shutter;
		MX_BLUICE_FOREIGN_STRING string;
	} u;
} MX_BLUICE_FOREIGN_DEVICE;

/* ----- */

typedef struct {
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

	int num_ion_chambers;
	MX_BLUICE_FOREIGN_DEVICE **ion_chamber_array;

	int num_motors;
	MX_BLUICE_FOREIGN_DEVICE **motor_array;

	int num_shutters;
	MX_BLUICE_FOREIGN_DEVICE **shutter_array;

	int num_strings;
	MX_BLUICE_FOREIGN_DEVICE **string_array;
} MX_BLUICE_SERVER;

/* ----- */

MX_API mx_status_type
mx_bluice_send_message( MX_RECORD *bluice_server_record,
			char *text_data,
			char *binary_data,
			long binary_data_length,
			long required_data_length );

MX_API mx_status_type
mx_bluice_receive_message( MX_RECORD *bluice_server_record,
				char *data_buffer,
				long data_buffer_size,
				long *actual_data_length,
				double timeout_in_seconds );

/* ----- */

#define MXT_BLUICE_GTOS	1
#define MXT_BLUICE_STOG	2
#define MXT_BLUICE_HTOS	3
#define MXT_BLUICE_STOH	4
#define MXT_BLUICE_STOC	5

MX_API mx_status_type
mx_bluice_get_message_type( char *message_string, int *message_type );

MX_API mx_status_type
mx_bluice_setup_device_pointer( MX_BLUICE_SERVER *bluice_server,
			char *name,
			MX_BLUICE_FOREIGN_DEVICE ***foreign_device_array_ptr,
			int *num_foreign_devices_ptr,
			size_t foreign_pointer_size,
			size_t foreign_device_size,
			MX_BLUICE_FOREIGN_DEVICE **foreign_device_ptr );

#define mx_bluice_get_device_pointer( b, n, fda, nfd, fd ) \
		mx_bluice_setup_device_pointer( (b), (n), \
		    (MX_BLUICE_FOREIGN_DEVICE ***) &(fda), &(nfd), 0, 0, (fd) )

MX_API mx_status_type
mx_bluice_wait_for_device_pointer_initialization(
			MX_BLUICE_SERVER *bluice_server,
			char *name,
			int bluice_foreign_type,
			MX_BLUICE_FOREIGN_DEVICE ***foreign_device_array_ptr,
			int *num_foreign_devices_ptr,
			MX_BLUICE_FOREIGN_DEVICE **foreign_device_ptr,
			double timeout_in_seconds );

/* ----- */

MX_API mx_status_type
mx_bluice_is_master( MX_BLUICE_SERVER *bluice_server, int *master_flag );

MX_API mx_status_type
mx_bluice_take_master( MX_BLUICE_SERVER *bluice_server, int take_master );

MX_API mx_status_type
mx_bluice_check_for_master( MX_BLUICE_SERVER *bluice_server );

/* ----- */

#define MXF_BLUICE_SEV_UNKNOWN	(-1)
#define MXF_BLUICE_SEV_INFO	1
#define MXF_BLUICE_SEV_WARNING	2
#define MXF_BLUICE_SEV_ERROR	3

#define MXF_BLUICE_LOC_UNKNOWN	(-1)
#define MXF_BLUICE_LOC_SERVER	1

MX_API mx_status_type
mx_bluice_parse_log_message( char *log_message,
				int *severity,
				int *locale,
				char *device_name,
				size_t device_name_length,
				char *message_body,
				size_t message_body_length );

/* ----- */

MX_API mx_status_type
mx_bluice_configure_ion_chamber( MX_BLUICE_SERVER *bluice_server,
				char *configuration_string );

MX_API mx_status_type
mx_bluice_configure_motor( MX_BLUICE_SERVER *bluice_server,
				char *configuration_string );

MX_API mx_status_type
mx_bluice_configure_shutter( MX_BLUICE_SERVER *bluice_server,
				char *configuration_string );

MX_API mx_status_type
mx_bluice_configure_string( MX_BLUICE_SERVER *bluice_server,
				char *configuration_string );

MX_API mx_status_type
mx_bluice_update_motion_status( MX_BLUICE_SERVER *bluice_server,
				char *motion_status_message,
				int move_in_progress );

#endif /* __MX_BLUICE_H__ */

