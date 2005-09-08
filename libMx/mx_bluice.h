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

typedef struct {
	char name[MXU_BLUICE_NAME_LENGTH+1];
} MX_BLUICE_FOREIGN_DEVICE;

typedef struct {
	char name[MXU_BLUICE_NAME_LENGTH+1];
	char dhs_server_name[MXU_BLUICE_NAME_LENGTH+1];
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
} MX_BLUICE_FOREIGN_MOTOR;

/* ----- */

typedef struct {
	MX_RECORD *record;

	MX_SOCKET *socket;

	MX_MUTEX *mutex;

	char *receive_buffer;
	long receive_buffer_length;
	long num_received_bytes;

	int num_records;
	MX_RECORD *record_array;

	int num_motors;
	MX_BLUICE_FOREIGN_MOTOR *motor_array;
} MX_BLUICE_SERVER;

/* ----- */

MX_API mx_status_type
mx_bluice_send_message( MX_RECORD *bluice_server_record,
			char *text_data,
			char *binary_data,
			long binary_data_length,
			long required_data_length,
			int send_header );

MX_API mx_status_type
mx_bluice_receive_message( MX_RECORD *bluice_server_record,
				char *data_buffer,
				long data_buffer_size,
				long *actual_data_length );

/* ----- */

MX_API mx_status_type
mx_bluice_num_unread_bytes_available( MX_RECORD *bluice_server_record,
					int *num_bytes_available );

MX_API mx_status_type
mx_bluice_discard_unread_bytes( MX_RECORD *bluice_server_record );

/* ----- */

#define MXT_BLUICE_GTOS	1
#define MXT_BLUICE_STOG	2
#define MXT_BLUICE_HTOS	3
#define MXT_BLUICE_STOH	4
#define MXT_BLUICE_STOC	5

MX_API mx_status_type
mx_bluice_get_message_type( char *message_string, int *message_type );

MX_API mx_status_type
mx_bluice_get_device_pointer( char *name,
				void **foreign_device_array,
				int *num_foreign_devices,
				size_t foreign_pointer_size,
				size_t foreign_device_size,
				void **foreign_device );

/* ----- */

MX_API mx_status_type
mx_bluice_configure_motor( MX_BLUICE_SERVER *bluice_server,
				char *configuration_string );

#endif /* __MX_BLUICE_H__ */

