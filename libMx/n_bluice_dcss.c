/*
 * Name:     n_bluice_dcss.c
 *
 * Purpose:  Client interface to a Blu-Ice DCSS server.
 *
 * Author:   William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2005-2006, 2008, 2010-2011, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define BLUICE_DCSS_DEBUG		TRUE

#define BLUICE_DCSS_DEBUG_CONFIG	FALSE

#define BLUICE_DCSS_DEBUG_SHUTDOWN	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_net.h"
#include "mx_bluice.h"
#include "d_bluice_timer.h"
#include "n_bluice_dcss.h"

MX_RECORD_FUNCTION_LIST mxn_bluice_dcss_server_record_function_list = {
	NULL,
	mxn_bluice_dcss_server_create_record_structures,
	NULL,
	NULL,
	mxn_bluice_dcss_server_print_structure,
	mxn_bluice_dcss_server_open,
	mxn_bluice_dcss_server_close,
	NULL,
	mxn_bluice_dcss_server_resynchronize
};

MX_RECORD_FIELD_DEFAULTS mxn_bluice_dcss_server_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXN_BLUICE_DCSS_SERVER_STANDARD_FIELDS
};

long mxn_bluice_dcss_server_num_record_fields
		= sizeof( mxn_bluice_dcss_server_rf_defaults )
		    / sizeof( mxn_bluice_dcss_server_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxn_bluice_dcss_server_rfield_def_ptr
			= &mxn_bluice_dcss_server_rf_defaults[0];

/*-------------------------------------------------------------------------*/

typedef mx_status_type (MXN_BLUICE_DCSS_MSG_HANDLER)( MX_THREAD *,
		MX_RECORD *, MX_BLUICE_SERVER *, MX_BLUICE_DCSS_SERVER *);

static MXN_BLUICE_DCSS_MSG_HANDLER stog_become_master;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_become_slave;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_configure_ion_chamber;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_configure_motor;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_configure_operation;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_configure_shutter;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_configure_string;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_log;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_motor_move_completed;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_motor_move_started;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_operation_completed;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_operation_update;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_report_ion_chambers;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_report_shutter_state;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_set_permission_level;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_set_string_completed;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_start_operation;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_update_motor_position;

static struct {
	char message_name[40];
	MXN_BLUICE_DCSS_MSG_HANDLER *message_handler;
} dcss_handler_list[] = {
	{"stog_become_master", stog_become_master},
	{"stog_become_slave", stog_become_slave},
	{"stog_configure_hardware_host", NULL},
	{"stog_configure_ion_chamber", stog_configure_ion_chamber},
	{"stog_configure_operation", stog_configure_operation},
	{"stog_configure_pseudo_motor", stog_configure_motor},
	{"stog_configure_real_motor", stog_configure_motor},
	{"stog_configure_run", NULL},
	{"stog_configure_runs", NULL},
	{"stog_configure_shutter", stog_configure_shutter},
	{"stog_configure_string", stog_configure_string},
	{"stog_log", stog_log},
	{"stog_motor_move_completed", stog_motor_move_completed},
	{"stog_motor_move_started", stog_motor_move_started},
	{"stog_operation_completed", stog_operation_completed},
	{"stog_operation_update", stog_operation_update},
	{"stog_report_ion_chambers", stog_report_ion_chambers},
	{"stog_report_shutter_state", stog_report_shutter_state},
	{"stog_set_permission_level", stog_set_permission_level},
	{"stog_set_string_completed", stog_set_string_completed},
	{"stog_start_operation", stog_start_operation},
	{"stog_update_client", NULL},
	{"stog_update_client_list", NULL},
	{"stog_update_motor_position", stog_update_motor_position},
};

static int num_dcss_handlers = sizeof( dcss_handler_list )
				/ sizeof( dcss_handler_list[0] );

/*-------------------------------------------------------------------------*/

/* Once started, mxn_bluice_dcss_monitor_thread() is the only thread
 * that is allowed to receive messages from DCSS.
 */

static mx_status_type
mxn_bluice_dcss_monitor_thread( MX_THREAD *thread, void *args )
{
	static const char fname[] = "mxn_bluice_dcss_monitor_thread()";

	MX_RECORD *dcss_server_record;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_DCSS_SERVER *bluice_dcss_server;
	MXN_BLUICE_DCSS_MSG_HANDLER *message_fn;
	long actual_data_length;
	char message_type_name[80];
	char message_type_format[20];
	int i, num_items;
	mx_status_type mx_status;

	if ( args == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	dcss_server_record = (MX_RECORD *) args;

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, dcss_server_record->name));
#endif

	bluice_server = (MX_BLUICE_SERVER *)
				dcss_server_record->record_class_struct;

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_SERVER pointer for record '%s' is NULL.",
			dcss_server_record->name );
	}

	bluice_dcss_server = (MX_BLUICE_DCSS_SERVER *)
				dcss_server_record->record_type_struct;

	if ( bluice_dcss_server == (MX_BLUICE_DCSS_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_DCSS_SERVER pointer for record '%s' is NULL.",
			dcss_server_record->name );
	}

	snprintf( message_type_format, sizeof(message_type_format),
			"%%%lus", (unsigned long) sizeof(message_type_name) );

	for (;;) {
		mx_status = mx_bluice_receive_message( dcss_server_record,
					NULL, 0, &actual_data_length, -1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if 0
		MX_DEBUG(-2,("%s: received '%s' from Blu-Ice server '%s'.",
			fname, bluice_server->receive_buffer,
			dcss_server_record->name ));
#endif

		/* Figure out what kind of message this is. */

		num_items = sscanf( bluice_server->receive_buffer,
				message_type_format,
				message_type_name );

		if ( num_items != 1 ) {
			return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Message type name not found in message '%s' "
			"received from DCSS server '%s'.",
				bluice_server->receive_buffer,
				dcss_server_record->name );
		}

		for ( i = 0; i < num_dcss_handlers; i++ ) {
			if ( strcmp( message_type_name, 
				dcss_handler_list[i].message_name ) == 0 )
			{
				break;		/* Exit the for(i) loop. */
			}
		}

		if ( i >= num_dcss_handlers ) {
			(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Message type '%s' received in message '%s' from "
			"DCSS server '%s' does not currently have a handler.",
				message_type_name,
				bluice_server->receive_buffer,
				dcss_server_record->name );
		} else {
			message_fn = dcss_handler_list[i].message_handler;

			if ( message_fn == NULL ) {
				MX_DEBUG( 2,("%s: Message type '%s' SKIPPED.",
					fname, message_type_name ));
			} else {
				MX_DEBUG( 2,("%s: Invoking handler for '%s'",
						fname, message_type_name));

				mx_status = (*message_fn)( thread,
						dcss_server_record,
						bluice_server,
						bluice_dcss_server );
			}
		}

		mx_status = mx_thread_check_for_stop_request( thread );
	}

#if ( defined(OS_HPUX) && !defined(__ia64) )
	return MX_SUCCESSFUL_RESULT;
#endif

}

static mx_status_type
mxn_bluice_dcss_server_get_session_id(
		MX_BLUICE_DCSS_SERVER *bluice_dcss_server,
		char *user_name,
		size_t username_length,
		char *session_id,
		size_t session_id_length )
{
	static const char fname[] = "mxn_bluice_dcss_server_get_session_id()";

	MX_SOCKET *auth_server_socket;
	FILE *auth_server_fp;
	unsigned long flags;
	unsigned long i, j, num_times_to_loop, remainder, buffer24;
	unsigned char index0, index1, index2, index3;
	size_t plaintext_length;
	static char crlf[] = "\015\012";
	static const char base64_table[] =
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	char authentication_data[MXU_AUTHENTICATION_DATA_LENGTH+1];
	char *ptr, *host_name, *port_number_ptr;
	char *prefix, *session_id_ptr;
	int port_number, status, saved_errno;
	mx_status_type mx_status;

	char line[200];
	char password[40];

	char plaintext[1000];
	char base64_hash[1500];

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s invoked for authentication server '%s'",
		fname, bluice_dcss_server->authentication_data));
#endif

	flags = bluice_dcss_server->bluice_dcss_flags;

	/* Get the username. */

	if ( (flags & MXF_BLUICE_DCSS_REQUIRE_USERNAME) == 0 ) {
		mx_username( user_name, username_length );
	} else {
		mx_info_entry_dialog( "Enter Blu-Ice username --> ",
					"Enter Blu-Ice username:",
					TRUE, user_name, username_length );
	}

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s: Blu-Ice username = '%s'", fname, user_name));
#endif

	/* Get the password. */

	mx_info_entry_dialog( "Enter Blu-Ice password --> ",
				"Enter Blu-Ice password:",
				FALSE, password, sizeof(password) );

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s: Blu-Ice password = '%s'", fname, password));
#endif

	/*--------------------------------------------------------------*/

	/* Construct the plaintext of the username:password string. */

	snprintf( plaintext, sizeof(plaintext), "%s:%s", user_name, password );

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s: Blu-Ice plaintext = '%s'", fname, plaintext));
#endif

	/* Null out the hash string. */

	memset( base64_hash, 0, sizeof(base64_hash) );

	/* Construct the base-64 encoded hash from the plaintext.
	 * See http://en.wikipedia.org/wiki/Base64 for the algorithm.
	 */

	plaintext_length = strlen( plaintext );

	num_times_to_loop = plaintext_length / 3;
	remainder         = plaintext_length % 3;

	for ( i = 0, j = 0; i < num_times_to_loop; i++, j++ ) {
		/* Copy three bytes of the plaintext into a 24-bit buffer. */

		buffer24 = 0;

		buffer24 |= ( plaintext[3*i] << 16 );
		buffer24 |= ( plaintext[3*i+1] << 8 );
		buffer24 |= plaintext[3*i+2];

		/* Use the 24-bit buffer as four 6-bit indices into
		 * the base-64 lookup table.
		 */

		index0 = ((unsigned char)( buffer24 >> 18 )) & 0x3f;
		index1 = ((unsigned char)( buffer24 >> 12 )) & 0x3f;
		index2 = ((unsigned char)( buffer24 >>  6 )) & 0x3f;
		index3 = ((unsigned char) buffer24 ) & 0x3f;

		base64_hash[4*j]   = base64_table[index0];
		base64_hash[4*j+1] = base64_table[index1];
		base64_hash[4*j+2] = base64_table[index2];
		base64_hash[4*j+3] = base64_table[index3];
	}

	/* Handle any leftover bytes. */

	if ( remainder != 0 ) {
		buffer24 = 0;

		switch( remainder ) {
		case 2:
			buffer24 |= ( plaintext[3*i+1] << 8 );

			/* Fall through to case 1. */
		case 1:
			buffer24 |= ( plaintext[3*i] << 16 );
			break;
		}

		index0 = ((unsigned char)( buffer24 >> 18 )) & 0x3f;
		index1 = ((unsigned char)( buffer24 >> 12 )) & 0x3f;
		index2 = ((unsigned char)( buffer24 >>  6 )) & 0x3f;

		base64_hash[4*j]   = base64_table[index0];
		base64_hash[4*j+1] = base64_table[index1];

		if ( remainder == 2 ) {
			base64_hash[4*j+2] = base64_table[index2];
		} else {
			base64_hash[4*j+2] = '=';
		}

		base64_hash[4*j+3] = '=';
	}

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s: base64_hash = '%s'", fname, base64_hash));
#endif

	/*--------------------------------------------------------------*/

	/* Get the host name and port number of the authentication server. */

	strlcpy( authentication_data,
		bluice_dcss_server->authentication_data,
		sizeof(authentication_data) );

	ptr = authentication_data;

	host_name = mx_string_token( &ptr, ":" );

	if ( host_name == NULL ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"The host name of the Blu-Ice authentication server "
		"was not specified in the authentication data string '%s'",
			bluice_dcss_server->authentication_data );
	}

	port_number_ptr = mx_string_token( &ptr, ":" );

	if ( port_number_ptr == NULL ) {
		port_number = 17000;
	} else {
		port_number = atoi( port_number_ptr );
	}

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s: host name = '%s', port number = %d",
		fname, host_name, port_number ));
#endif

	/*--------------------------------------------------------------*/

	/* Connect to the authentication server. */

	mx_status = mx_tcp_socket_open_as_client( &auth_server_socket,
			host_name, port_number, 0,
			MX_SOCKET_DEFAULT_BUFFER_SIZE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Convert the socket descriptor to a file pointer. */

	auth_server_fp = fdopen( auth_server_socket->socket_fd, "w+" );

	/***** Send the request *****/

	/* Send the first line of text. */

	snprintf( line, sizeof(line),
 "GET /gateway/servlet/APPLOGIN?userid=%s&passwd=%s&AppName=%s HTTP/1.1%s",
		user_name, base64_hash, bluice_dcss_server->appname, crlf );

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s: Command line 1 = '%s'", fname, line));
#endif

	status = fputs( line, auth_server_fp );

	if ( status == EOF ) {
		saved_errno = errno;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"An error occurred while writing the line '%s' to "
		"Blu-Ice authentication server '%s', port %d.  "
		"Errno = %d, error message = '%s'",
			line, host_name, port_number,
			saved_errno, strerror(saved_errno) );
	}

	/* Send the second line of text. */

	snprintf( line, sizeof(line), "Host: %s:%d%s",
		host_name, port_number, crlf );

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s: Command line 2 = '%s'", fname, line));
#endif

	status = fputs( line, auth_server_fp );

	if ( status == EOF ) {
		saved_errno = errno;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"An error occurred while writing the line '%s' to "
		"Blu-Ice authentication server '%s', port %d.  "
		"Errno = %d, error message = '%s'",
			line, host_name, port_number,
			saved_errno, strerror(saved_errno) );
	}

	/* Terminate the HTTP message with a blank line. */

	status = fputs( crlf, auth_server_fp );

	if ( status == EOF ) {
		saved_errno = errno;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"An error occurred while writing a blank terminator line to "
		"Blu-Ice authentication server '%s', port %d.  "
		"Errno = %d, error message = '%s'",
			host_name, port_number,
			saved_errno, strerror(saved_errno) );
	}

	/*--------------------------------------------------------------*/

	/* Read the response from the authentication server. */

	session_id[0] = '\0';

	for ( i = 0; ; i++ ) {
		mx_fgets( line, sizeof(line), auth_server_fp );

		/* Look for the line that sets a cookie. */

		if ( strncmp( line, "Set-Cookie:", 11 ) == 0 ) {
			/* The format of the set coookie line is:
			 *
			 * Set-Cookie: SMBSessionID=XYZZY;Path=/gateway
			 *
			 * We can extract the session id by copying all
			 * text between the first '=' and the first ';'
			 * characters.
			 */

			 ptr = line;

			 prefix = mx_string_token( &ptr, "=" );

			 if ( prefix == NULL ) {
			 	return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Did not find the '=' character before the "
				"start of the session ID cookie line '%s'.",
					line );
			 }

			 session_id_ptr = mx_string_token( &ptr, ";" );

			 if ( session_id_ptr == NULL ) {
			 	return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Did not find the ';' character after the "
				"session ID cookie '%s'", ptr );
			 }

			 strlcpy(session_id, session_id_ptr, session_id_length);
		}

		if ( feof(auth_server_fp) ) {
			/* We have reached the end of the message,
			 * so break out of the for() loop.
			 */

			 break;
		}

#if BLUICE_DCSS_DEBUG
		MX_DEBUG(-2,("%s: Response line %lu = '%s'",
			fname, i, line));
#endif
	}

	/* Close the connection to the authentication server. */

	status = fclose( auth_server_fp );

	if ( status == EOF ) {
		saved_errno = errno;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"An error occurred while closing the socket for "
		"Blu-Ice authentication server '%s', port %d.  "
		"Errno = %d, error message = '%s'",
			host_name, port_number,
			saved_errno, strerror(saved_errno) );
	}

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s: session id = '%s'", fname, session_id));
#endif

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
stog_become_master( MX_THREAD *thread,
		MX_RECORD *server_record,
		MX_BLUICE_SERVER *bluice_server,
		MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	static const char fname[] = "stog_become_master()";

	long mx_status_code;

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s invoked for message '%s' from server '%s'",
		fname, bluice_server->receive_buffer, server_record->name ));
#endif

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the foreign data mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

	bluice_dcss_server->is_master = TRUE;

	mx_mutex_unlock( bluice_server->foreign_data_mutex );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
stog_become_slave( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	static const char fname[] = "stog_become_slave()";

	long mx_status_code;

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s invoked for message '%s' from server '%s'",
		fname, bluice_server->receive_buffer, server_record->name ));
#endif

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the foreign data mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

	bluice_dcss_server->is_master = FALSE;

	mx_mutex_unlock( bluice_server->foreign_data_mutex );

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/

static mx_status_type
stog_configure_ion_chamber( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	static const char fname[] = "stog_configure_ion_chamber()";

	MX_BLUICE_FOREIGN_DEVICE *foreign_ion_chamber;
	char *ion_chamber_name, *dhs_server_name;
	char *counter_name, *timer_name, *timer_type;
	long channel_number;
	int argc;
	char **argv;
	mx_status_type mx_status;

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_BLUICE_SERVER pointer passed was NULL." );
	}

	mx_string_split( bluice_server->receive_buffer, " ", &argc, &argv );

	if ( argc < 7 ) {
		mx_free(argv);
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The 'stog_configure_ion_chamber' command sent by "
		"Blu-Ice server '%s' was truncated to %d tokens.",
			bluice_server->record->name, argc );
	}

	ion_chamber_name = argv[1];
	dhs_server_name  = argv[2];
	counter_name     = argv[3];
	channel_number   = atoi( argv[4] );
	timer_name       = argv[5];
	timer_type       = argv[6];

	/* Get a pointer to the Blu-Ice foreign ion chamber structure. */

	mx_status = mx_bluice_setup_device_pointer(
					bluice_server,
					ion_chamber_name,
					&(bluice_server->ion_chamber_array),
					&(bluice_server->num_ion_chambers),
					&foreign_ion_chamber );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_free(argv);
		return mx_status;
	}

	foreign_ion_chamber->foreign_type = MXT_BLUICE_FOREIGN_ION_CHAMBER;

	foreign_ion_chamber->u.ion_chamber.mx_analog_input = NULL;
	foreign_ion_chamber->u.ion_chamber.mx_timer = NULL;

	strlcpy( foreign_ion_chamber->dhs_server_name,
			dhs_server_name, MXU_BLUICE_NAME_LENGTH );

	strlcpy( foreign_ion_chamber->u.ion_chamber.counter_name,
			counter_name, MXU_BLUICE_NAME_LENGTH );

	foreign_ion_chamber->u.ion_chamber.channel_number = channel_number;

	strlcpy( foreign_ion_chamber->u.ion_chamber.timer_name,
			timer_name, MXU_BLUICE_NAME_LENGTH );

	strlcpy( foreign_ion_chamber->u.ion_chamber.timer_type,
			timer_type, MXU_BLUICE_NAME_LENGTH );

#if BLUICE_DCSS_DEBUG_CONFIG
	MX_DEBUG(-2,("%s: -------------------------------------------", fname));
	MX_DEBUG(-2,("%s: Foreign ion chamber '%s':",
				fname, foreign_ion_chamber->name));
	MX_DEBUG(-2,("%s: DHS server = '%s'",
				fname, foreign_ion_chamber->dhs_server_name));
	MX_DEBUG(-2,("%s: Counter name = '%s'",
		fname, foreign_ion_chamber->u.ion_chamber.counter_name));
	MX_DEBUG(-2,("%s: Channel number = %ld",
		fname, foreign_ion_chamber->u.ion_chamber.channel_number));
	MX_DEBUG(-2,("%s: Timer name = '%s'",
		fname, foreign_ion_chamber->u.ion_chamber.timer_name));
	MX_DEBUG(-2,("%s: Timer type = '%s'",
		fname, foreign_ion_chamber->u.ion_chamber.timer_type));
	MX_DEBUG(-2,("%s: -------------------------------------------", fname));
#endif

	mx_free(argv);

	return mx_status;
}

/*------------------------------------------------------------------------*/

static mx_status_type
stog_configure_motor( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	static const char fname[] = "stog_configure_motor()";

	MX_BLUICE_FOREIGN_DEVICE *foreign_motor;
	char format_string[100];
	char name[MXU_BLUICE_NAME_LENGTH+1];
	char *config_string;
	long num_items, format_length;
	mx_status_type mx_status;

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_BLUICE_SERVER pointer passed was NULL." );
	}

	config_string = bluice_server->receive_buffer;

	snprintf( format_string, sizeof(format_string),
			"%%*s %%%ds", MXU_BLUICE_NAME_LENGTH );

	num_items = sscanf( config_string, format_string, name );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Device name not found in message '%s' from "
		"Blu-Ice server '%s'.",
			config_string, bluice_server->record->name );
	}

	/* Get a pointer to the Blu-Ice foreign motor structure. */

#if 0
	MX_DEBUG(-2,("%s: MARKER #A &(bluice_server->motor_array) = %p",
		fname, &(bluice_server->motor_array)));

	MX_DEBUG(-2,("%s: MARKER #A bluice_server->motor_array = %p",
		fname, bluice_server->motor_array));
#endif

	mx_status = mx_bluice_setup_device_pointer(
					bluice_server,
					name,
					&(bluice_server->motor_array),
					&(bluice_server->num_motors),
					&foreign_motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	MX_DEBUG(-2,("%s: MARKER #B &(bluice_server->motor_array) = %p",
		fname, &(bluice_server->motor_array)));

	MX_DEBUG(-2,("%s: MARKER #B bluice_server->motor_array = %p",
		fname, bluice_server->motor_array));
#endif

	foreign_motor->foreign_type = MXT_BLUICE_FOREIGN_MOTOR;

	foreign_motor->u.motor.mx_motor = NULL;
	foreign_motor->u.motor.move_in_progress = FALSE;

	/* Now parse the rest of the configuration string. */

	if ( strncmp( config_string, "stog_configure_real_motor", 25 ) == 0 ) {

		foreign_motor->u.motor.is_pseudo = FALSE;

		format_length = snprintf( format_string, sizeof(format_string),
"%%*s %%*s %%%ds %%%ds %%lg %%lg %%lg %%lg %%lg %%lg %%lg %%d %%d %%d %%d %%d",
				MXU_BLUICE_NAME_LENGTH,
				MXU_BLUICE_NAME_LENGTH );

		if ( format_length >= sizeof(format_string) ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The length needed for the format string for "
			"stog_configure_real_motor is %ld bytes, "
			"but the format string buffer is only %ld "
			"bytes.  You must recompile this version of "
			"MX with a longer format string buffer size.",
				format_length + 1,
				(long) sizeof(format_string) );
		}

		num_items = sscanf( config_string, format_string,
			foreign_motor->dhs_server_name,
			foreign_motor->u.motor.dhs_name,
			&(foreign_motor->u.motor.position),
			&(foreign_motor->u.motor.upper_limit),
			&(foreign_motor->u.motor.lower_limit),
			&(foreign_motor->u.motor.scale_factor),
			&(foreign_motor->u.motor.speed),
			&(foreign_motor->u.motor.acceleration_time),
			&(foreign_motor->u.motor.backlash),
			&(foreign_motor->u.motor.lower_limit_on),
			&(foreign_motor->u.motor.upper_limit_on),
			&(foreign_motor->u.motor.motor_lock_on),
			&(foreign_motor->u.motor.backlash_on),
			&(foreign_motor->u.motor.reverse_on) );

		if ( num_items != 14 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
"Message '%s' from Blu-Ice server '%s' did not have the expected format.",
				config_string, bluice_server->record->name );
		}
	} else
	if ( strncmp( config_string, "stog_configure_pseudo_motor", 27 ) == 0 )
	{
		foreign_motor->u.motor.is_pseudo = TRUE;

		foreign_motor->u.motor.scale_factor = 0.0;
		foreign_motor->u.motor.speed = 0.0;
		foreign_motor->u.motor.acceleration_time = 0.0;
		foreign_motor->u.motor.backlash = 0.0;
		foreign_motor->u.motor.backlash_on = FALSE;
		foreign_motor->u.motor.reverse_on = FALSE;

		snprintf( format_string, sizeof(format_string),
		"%%*s %%*s %%%ds %%%ds %%lg %%lg %%lg %%d %%d %%d",
			MXU_BLUICE_NAME_LENGTH,
			MXU_BLUICE_NAME_LENGTH );

		num_items = sscanf( config_string, format_string,
			foreign_motor->dhs_server_name,
			foreign_motor->u.motor.dhs_name,
			&(foreign_motor->u.motor.position),
			&(foreign_motor->u.motor.upper_limit),
			&(foreign_motor->u.motor.lower_limit),
			&(foreign_motor->u.motor.upper_limit_on),
			&(foreign_motor->u.motor.lower_limit_on),
			&(foreign_motor->u.motor.motor_lock_on) );

		if ( num_items != 8 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
"Message '%s' from Blu-Ice server '%s' did not have the expected format.",
				config_string, bluice_server->record->name );
		}
	} else {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Unexpected 'stog' message type in Blu-Ice "
		"message '%s' from DCSS '%s'.", config_string,
				bluice_server->record->name );
	}

#if BLUICE_DCSS_DEBUG_CONFIG
	MX_DEBUG(-2,("%s: -------------------------------------------", fname));
	MX_DEBUG(-2,("%s: Foreign motor '%s':", fname, foreign_motor->name));
	MX_DEBUG(-2,("%s: is pseudo = %d",
				fname, foreign_motor->u.motor.is_pseudo));
	MX_DEBUG(-2,("%s: DHS server = '%s'",
				fname, foreign_motor->dhs_server_name));
	MX_DEBUG(-2,("%s: DHS name = '%s'",
				fname, foreign_motor->u.motor.dhs_name));
	MX_DEBUG(-2,("%s: position = %g", fname,
				foreign_motor->u.motor.position));
	MX_DEBUG(-2,("%s: upper limit = %g",
				fname, foreign_motor->u.motor.upper_limit));
	MX_DEBUG(-2,("%s: lower limit = %g",
				fname, foreign_motor->u.motor.lower_limit));
	MX_DEBUG(-2,("%s: scale factor = %g",
				fname, foreign_motor->u.motor.scale_factor));
	MX_DEBUG(-2,("%s: speed = %g", fname, foreign_motor->u.motor.speed));
	MX_DEBUG(-2,("%s: acceleration time = %g",
			fname, foreign_motor->u.motor.acceleration_time));
	MX_DEBUG(-2,("%s: backlash = %g", fname,
				foreign_motor->u.motor.backlash));
	MX_DEBUG(-2,("%s: upper limit on = %d",
				fname, foreign_motor->u.motor.upper_limit_on));
	MX_DEBUG(-2,("%s: lower limit on = %d",
				fname, foreign_motor->u.motor.lower_limit_on));
	MX_DEBUG(-2,("%s: motor lock on = %d",
				fname, foreign_motor->u.motor.motor_lock_on));
	MX_DEBUG(-2,("%s: backlash on = %d",
				fname, foreign_motor->u.motor.backlash_on));
	MX_DEBUG(-2,("%s: reverse on = %d",
				fname, foreign_motor->u.motor.reverse_on));
	MX_DEBUG(-2,("%s: -------------------------------------------", fname));
#endif

	return mx_status;
}

/*------------------------------------------------------------------------*/

static mx_status_type
stog_configure_operation( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	static const char fname[] = "stog_configure_operation()";

	MX_BLUICE_FOREIGN_DEVICE *foreign_operation;
	char *operation_name, *dhs_server_name;
	int argc;
	char **argv;
	mx_status_type mx_status;

#if 0
	MX_DEBUG(-2,("%s: message = '%s'",
		fname, bluice_server->receive_buffer));
#endif

	mx_string_split( bluice_server->receive_buffer, " ", &argc, &argv );

	if ( argc < 3 ) {
		mx_free(argv);
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The 'stog_configure_operation' command sent by "
		"Blu-Ice server '%s' was truncated to %d tokens.",
			bluice_server->record->name, argc );
	}

	operation_name  = argv[1];
	dhs_server_name = argv[2];

	/* Get a pointer to the Blu-Ice foreign operation structure. */

	mx_status = mx_bluice_setup_device_pointer(
					bluice_server,
					operation_name,
					&(bluice_server->operation_array),
					&(bluice_server->num_operations),
					&foreign_operation );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_free(argv);
		return mx_status;
	}

	foreign_operation->foreign_type = MXT_BLUICE_FOREIGN_OPERATION;

	foreign_operation->u.operation.mx_operation_variable = NULL;

	strlcpy( foreign_operation->dhs_server_name,
			dhs_server_name, MXU_BLUICE_NAME_LENGTH );

	foreign_operation->u.operation.arguments_buffer = NULL;
	foreign_operation->u.operation.arguments_length = 0;

#if BLUICE_DCSS_DEBUG_CONFIG
	MX_DEBUG(-2,("%s: -------------------------------------------", fname));
	MX_DEBUG(-2,("%s: Foreign operation '%s':",fname,
				foreign_operation->name));
	MX_DEBUG(-2,("%s: DHS server = '%s'",
				fname, foreign_operation->dhs_server_name));
	MX_DEBUG(-2,("%s: -------------------------------------------", fname));
#endif
	mx_free(argv);

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/

static mx_status_type
stog_configure_shutter( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	static const char fname[] = "stog_configure_shutter()";

	MX_BLUICE_FOREIGN_DEVICE *foreign_shutter;
	char *shutter_name, *dhs_server_name, *bluice_shutter_status;
	long shutter_status;
	int argc;
	char **argv;
	mx_status_type mx_status;

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_BLUICE_SERVER pointer passed was NULL." );
	}

	mx_string_split( bluice_server->receive_buffer, " ", &argc, &argv );

	if ( argc < 4 ) {
		mx_free(argv);
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The 'stog_configure_shutter' command sent by "
		"Blu-Ice server '%s' was truncated to %d tokens.",
			bluice_server->record->name, argc );
	}

	shutter_name          = argv[1];
	dhs_server_name       = argv[2];
	bluice_shutter_status = argv[3];

	if ( strcmp( bluice_shutter_status, "closed" ) == 0 ) {
		shutter_status = MXF_RELAY_IS_CLOSED;
	} else
	if ( strcmp( bluice_shutter_status, "open" ) == 0 ) {
		shutter_status = MXF_RELAY_IS_OPEN;
	} else {
		shutter_status = MXF_RELAY_ILLEGAL_STATUS;

		mx_free(argv);
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Unrecognizable shutter status '%s' received for "
		"Blu-Ice shutter '%s'.", bluice_shutter_status,
			bluice_server->record->name );
	}

	/* Get a pointer to the Blu-Ice foreign shutter structure. */

	mx_status = mx_bluice_setup_device_pointer(
					bluice_server,
					shutter_name,
					&(bluice_server->shutter_array),
					&(bluice_server->num_shutters),
					&foreign_shutter );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_free(argv);
		return mx_status;
	}

	foreign_shutter->foreign_type = MXT_BLUICE_FOREIGN_SHUTTER;

	foreign_shutter->u.shutter.mx_relay = NULL;

	strlcpy( foreign_shutter->dhs_server_name,
			dhs_server_name, MXU_BLUICE_NAME_LENGTH );

	foreign_shutter->u.shutter.shutter_status = shutter_status;

#if BLUICE_DCSS_DEBUG_CONFIG
	MX_DEBUG(-2,("%s: -------------------------------------------", fname));
	MX_DEBUG(-2,("%s: Foreign shutter '%s':",fname, foreign_shutter->name));
	MX_DEBUG(-2,("%s: DHS server = '%s'",
				fname, foreign_shutter->dhs_server_name));
	MX_DEBUG(-2,("%s: Shutter status = %ld",
			    fname, foreign_shutter->u.shutter.shutter_status));
	MX_DEBUG(-2,("%s: -------------------------------------------", fname));
#endif

	mx_free(argv);

	return mx_status;
}

/*------------------------------------------------------------------------*/

static mx_status_type
stog_configure_string( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	static const char fname[] = "stog_configure_string()";

	MX_BLUICE_FOREIGN_DEVICE *foreign_string;
	char *config_string, *ptr, *token_ptr;
	char *string_name, *dhs_server_name, *string_buffer;
	size_t string_length;
	mx_status_type mx_status;

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_BLUICE_SERVER pointer passed was NULL." );
	}

	config_string = bluice_server->receive_buffer;

	/* Skip over the command name. */

	ptr = config_string;

	token_ptr = mx_string_token( &ptr, " " );

	if ( token_ptr == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The message '%s' received from Blu-Ice server '%s' "
		"contained only space characters.",
			bluice_server->receive_buffer,
			bluice_server->record->name );
	}

	/* Get the string name. */

	string_name = mx_string_token( &ptr, " " );

	if ( string_name == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"String name not found in message received from "
		"Blu-Ice server '%s'.", bluice_server->record->name );
	}

	/* Get the DHS server name. */

	dhs_server_name = mx_string_token( &ptr, " " );

	if ( dhs_server_name == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"DHS server name not found in message received from "
		"Blu-Ice server '%s' for string '%s'.",
			bluice_server->record->name,
			string_name );
	}

	/* Treat the rest of the receive buffer as the string buffer. */

	string_buffer = ptr;

	if ( string_buffer == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Empty string seen in message received from "
		"Blu-Ice server '%s' for string '%s'.",
			bluice_server->record->name,
			string_name );
	}

	/* Get a pointer to the Blu-Ice foreign string structure. */

	mx_status = mx_bluice_setup_device_pointer(
					bluice_server,
					string_name,
					&(bluice_server->string_array),
					&(bluice_server->num_strings),
					&foreign_string );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	foreign_string->foreign_type = MXT_BLUICE_FOREIGN_STRING;

	foreign_string->u.string.mx_string_variable = NULL;

	strlcpy( foreign_string->dhs_server_name,
			dhs_server_name, MXU_BLUICE_NAME_LENGTH );

	string_length = strlen( string_buffer );

	if ( foreign_string->u.string.string_buffer == (char *) NULL ) {
		foreign_string->u.string.string_buffer = (char *)
						malloc( string_length+1 );
	} else {
		foreign_string->u.string.string_buffer = (char *)
	   realloc( foreign_string->u.string.string_buffer, string_length+1 );
	}

	if ( foreign_string->u.string.string_buffer == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu byte string "
		"for Blu-Ice foreign string '%s'.",
			(unsigned long) string_length, string_name );
	}

	strlcpy( foreign_string->u.string.string_buffer,
			string_buffer, string_length+1 );

#if BLUICE_DCSS_DEBUG_CONFIG
	MX_DEBUG(-2,("%s: -------------------------------------------", fname));
	MX_DEBUG(-2,("%s: Foreign string '%s':", fname, foreign_string->name));
	MX_DEBUG(-2,("%s: DHS server = '%s'",
				fname, foreign_string->dhs_server_name));
	MX_DEBUG(-2,("%s: String buffer = '%s'",
			    fname, foreign_string->u.string.string_buffer));
	MX_DEBUG(-2,("%s: -------------------------------------------", fname));
#endif

	return mx_status;
}

/*------------------------------------------------------------------------*/

#define MXF_BLUICE_SEV_UNKNOWN	(-1)
#define MXF_BLUICE_SEV_INFO	1
#define MXF_BLUICE_SEV_WARNING	2
#define MXF_BLUICE_SEV_ERROR	3

#define MXF_BLUICE_LOC_UNKNOWN	(-1)
#define MXF_BLUICE_LOC_SERVER	1

/* ----- */

static mx_status_type
stog_log( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
#if BLUICE_DCSS_DEBUG
	static const char fname[] = "stog_log()";
#endif

	long severity, locale;
	char device_name[MXU_BLUICE_NAME_LENGTH+1];
	char message_body[500];
	char *log_message;
	char *ptr, *token_ptr;

	log_message = bluice_server->receive_buffer;

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s: log_message = '%s'", fname, log_message));
#endif

	ptr = log_message;

	/* Skip over the log message_token. */

	token_ptr = mx_string_token( &ptr, " " );

	if ( token_ptr == NULL ) {
		return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
		"Did not find any non-blank characters in the log message "
		"from the Blu-Ice server." );
	}

	MX_DEBUG( 2,("%s: command = '%s', ptr = '%s'",
		fname, token_ptr, ptr ));

	/* Get the severity level. */

	token_ptr = mx_string_token( &ptr, " " );

	if ( token_ptr == NULL ) {
		return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
		"Did not find the severity level in the log message "
		"from the Blu-Ice server." );
	}

	if ( strcmp( token_ptr, "info" ) == 0 ) {
		severity = MXF_BLUICE_SEV_INFO;
	} else
	if ( strcmp( token_ptr, "warning" ) == 0 ) {
		severity = MXF_BLUICE_SEV_WARNING;
	} else
	if ( strcmp( token_ptr, "error" ) == 0 ) {
		severity = MXF_BLUICE_SEV_ERROR;
	} else {
		severity = MXF_BLUICE_SEV_UNKNOWN;

		mx_warning( "Unrecognized severity level '%s' in the "
			"message from the Blu-Ice server.", token_ptr );
	}

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,
	("%s: severity level = %ld, severity token = '%s', ptr = '%s'",
		fname, severity, token_ptr, ptr ));
#endif

	/* Get the locale. */

	token_ptr = mx_string_token( &ptr, " " );

	if ( token_ptr == NULL ) {
		return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
		"Did not find the locale in the log message "
		"from the Blu-Ice server." );
	}

	if ( strcmp( token_ptr, "server" ) == 0 ) {
		locale = MXF_BLUICE_LOC_SERVER;
	} else {
		locale = MXF_BLUICE_LOC_UNKNOWN;

		mx_warning( "Unrecognized locale '%s' in the "
			"message from the Blu-Ice server.", token_ptr );
	}

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s: locale = %ld, locale token = '%s', ptr = '%s'",
		fname, locale, token_ptr, ptr ));
#endif

	/* Get the device name. */

	token_ptr = mx_string_token( &ptr, " " );

	if ( token_ptr == NULL ) {
		return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
		"Did not find the device name in the log message "
		"from the Blu-Ice server." );
	}

	strlcpy( device_name, token_ptr, sizeof(device_name) );

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s: device_name = '%s', ptr = '%s'",
		fname, device_name, ptr));
#endif

	/* Copy the rest of the string to the message_body argument. */

	strlcpy( message_body, ptr, sizeof(message_body) );

	MX_DEBUG( 2,("%s: message_body = '%s'", fname, message_body));

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,
  ("%s: severity = %ld, locale = %ld, device_name = '%s', message_body = '%s'.",
		fname, severity, locale, device_name, message_body));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/

static mx_status_type
stog_motor_move_completed( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	mx_status_type mx_status;

	mx_status = mx_bluice_update_motion_status( bluice_server,
						bluice_server->receive_buffer,
						FALSE );
	return mx_status;
}

/*------------------------------------------------------------------------*/

static mx_status_type
stog_motor_move_started( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	mx_status_type mx_status;

	mx_status = mx_bluice_update_motion_status( bluice_server,
						bluice_server->receive_buffer,
						TRUE );
	return mx_status;
}

/*------------------------------------------------------------------------*/

static mx_status_type
stog_operation_completed( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	static const char fname[] = "stog_operation_completed()";

	mx_status_type mx_status;

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s invoked for message '%s' from server '%s'",
		fname, bluice_server->receive_buffer, server_record->name));
#endif
	mx_status = mx_bluice_update_operation_status( bluice_server,
						bluice_server->receive_buffer );
	return mx_status;
}

/*------------------------------------------------------------------------*/

static mx_status_type
stog_operation_update( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	static const char fname[] = "stog_operation_update()";

	mx_status_type mx_status;

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s invoked for message '%s' from server '%s'",
		fname, bluice_server->receive_buffer, server_record->name));
#endif
	mx_status = mx_bluice_update_operation_status( bluice_server,
						bluice_server->receive_buffer );
	return mx_status;
}

/*------------------------------------------------------------------------*/

static mx_status_type
stog_report_ion_chambers( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	static const char fname[] = "stog_report_ion_chambers()"; 

	MX_TIMER *timer;
	MX_BLUICE_TIMER *bluice_timer;
	MX_BLUICE_FOREIGN_DEVICE *first_ion_chamber;
	MX_BLUICE_FOREIGN_DEVICE *foreign_ion_chamber;
	MX_BLUICE_FOREIGN_DEVICE **ion_chamber_array;
	char *first_ion_chamber_name;
	char *ion_chamber_name;
	long i, j, num_ion_chambers, num_ion_chamber_readings;
	double measurement_value;
	int argc;
	char **argv;
	mx_status_type mx_status;
	long mx_status_code;

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s invoked for message '%s' from server '%s'",
		fname, bluice_server->receive_buffer, server_record->name));
#endif

	mx_string_split( bluice_server->receive_buffer, " ", &argc, &argv );

	if ( argc < 4 ) {
		mx_free(argv);
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The 'stog_report_ion_chambers' command sent by "
		"Blu-Ice server '%s' was truncated to %d tokens.",
			bluice_server->record->name, argc );
	}

	/* Get the name of the first ion chamber, so that we can find
	 * the associated MX_BLUICE_TIMER structure.
	 */

	first_ion_chamber_name = argv[2];

	num_ion_chamber_readings = (argc/2) - 1;

	/* First, get the ion chamber pointer for the first ion chamber. */

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		mx_free(argv);
		return mx_error( mx_status_code, fname,
		"An attempt to lock the foreign data mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

	mx_status = mx_bluice_get_device_pointer( bluice_server,
					first_ion_chamber_name,
					bluice_server->ion_chamber_array,
					bluice_server->num_ion_chambers,
					&first_ion_chamber );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );
		mx_free(argv);
		return mx_status;
	}

	if ( first_ion_chamber == (MX_BLUICE_FOREIGN_DEVICE *) NULL ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );
		mx_free(argv);
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The ion chamber pointer returned for ion chamber '%s' "
		"is NULL.", first_ion_chamber_name );
	}

	/* Get the timer from the first_ion_chamber structure. */

	timer = first_ion_chamber->u.ion_chamber.mx_timer;

	if ( timer == (MX_TIMER *) NULL ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );
		mx_free(argv);
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The MX_TIMER pointer for ion chamber '%s' has not "
		"been initialized.", first_ion_chamber_name );
	}

	if ( timer->record == (MX_RECORD *) NULL ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );
		mx_free(argv);
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_TIMER %p used by Blu-Ice "
		"ion chamber '%s' is NULL.",
			timer, first_ion_chamber_name );
	}

	/* The MX_BLUICE_TIMER structure contains information about all
	 * of the ion chambers known by this timer.
	 */

	bluice_timer = (MX_BLUICE_TIMER *) timer->record->record_type_struct;

	if ( bluice_timer == (MX_BLUICE_TIMER *) NULL ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );
		mx_free(argv);
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_TIMER pointer for Blu-Ice timer '%s' is NULL.",
			timer->record->name );
	}

	num_ion_chambers = bluice_timer->num_ion_chambers;

	ion_chamber_array = bluice_timer->foreign_ion_chamber_array;

	if ( ion_chamber_array == (MX_BLUICE_FOREIGN_DEVICE **) NULL ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );
		mx_free(argv);
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The array of ion chamber pointers for Blu-Ice timer '%s' "
		"has not been initialized.", timer->record->name );
	}

	if ( num_ion_chambers < num_ion_chamber_readings ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );
		mx_free(argv);
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"The number of ion chamber readings (%ld) is larger than "
		"the total number of ion chambers (%ld) belonging to "
		"Blu-Ice timer '%s'.", num_ion_chamber_readings,
			num_ion_chambers, timer->record->name );
	}

	/* Mark the current measurement as being over. */

	bluice_timer->measurement_in_progress = FALSE;

	/* Now we can read the ion chamber values transmitted by the server. */

	for ( i = 1; i < num_ion_chamber_readings; i++ ) {

		ion_chamber_name = argv[2*i];

		measurement_value = atof( argv[2*i+1] );

		/* Search for the ion chamber in the timer's 
		 * ion chamber array.
		 */

		for ( j = 0; j < num_ion_chambers; j++ ) {
			foreign_ion_chamber = ion_chamber_array[j];

			if ( foreign_ion_chamber == NULL ) {
				mx_warning( "Skipping NULL ion chamber %ld "
				"for Blu-Ice timer '%s'.",
					j, timer->record->name );
			}

			if ( strcmp( foreign_ion_chamber->name,
					ion_chamber_name ) == 0 )
			{
				/* SUCCESS!  We now can save
				 * the measurement value.
				 */

				foreign_ion_chamber->u.ion_chamber.value
							= measurement_value;

				break;	/* Exit the for() loop. */
			}
		}

		if ( j >= num_ion_chambers ) {
			mx_mutex_unlock( bluice_server->foreign_data_mutex );
			mx_free(argv);
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
			"Ion chamber '%s' was not found in the "
			"ion chamber array for Blu-Ice timer '%s'.",
				ion_chamber_name, timer->record->name );
		}
	}

	mx_mutex_unlock( bluice_server->foreign_data_mutex );
	mx_free(argv);

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/

static mx_status_type
stog_report_shutter_state( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	static const char fname[] = "stog_report_shutter_state()"; 

	MX_BLUICE_FOREIGN_DEVICE *foreign_shutter;
	int argc;
	char **argv;
	char *shutter_name, *bluice_shutter_status;
	int shutter_status;
	mx_status_type mx_status;
	long mx_status_code;

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s invoked for message '%s' from server '%s'",
		fname, bluice_server->receive_buffer, server_record->name));
#endif

	mx_string_split( bluice_server->receive_buffer, " ", &argc, &argv );

	if ( argc < 3 ) {
		mx_free(argv);
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The 'stog_report_shutter_state' command sent by "
		"Blu-Ice server '%s' was truncated.",
			bluice_server->record->name );
	}

	shutter_name          = argv[1];
	bluice_shutter_status = argv[2];

	if ( strcmp( bluice_shutter_status, "open" ) == 0 ) {
		shutter_status = MXF_RELAY_IS_OPEN;
	} else
	if ( strcmp( bluice_shutter_status, "closed" ) == 0 ) {
		shutter_status = MXF_RELAY_IS_CLOSED;
	} else {
		shutter_status = MXF_RELAY_ILLEGAL_STATUS;

		mx_warning(
	"Illegal shutter status '%s' returned for Blu-Ice shutter '%s'.",
			bluice_shutter_status, shutter_name );
	}

	/* Update the value in the shutter structure. */

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		mx_free(argv);
		return mx_error( mx_status_code, fname,
		"An attempt to lock the foreign data mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

	mx_status = mx_bluice_get_device_pointer( bluice_server,
						shutter_name,
						bluice_server->shutter_array,
						bluice_server->num_shutters,
						&foreign_shutter );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );
		mx_free(argv);
		return mx_status;
	}

	if ( foreign_shutter == (MX_BLUICE_FOREIGN_DEVICE *) NULL ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );
		mx_free(argv);
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The MX_BLUICE_FOREIGN_DEVICE pointer for DCSS shutter '%s' "
		"has not been initialized.", shutter_name );
	}

	/* Update the shutter status. */

	foreign_shutter->u.shutter.shutter_status = shutter_status;

	mx_mutex_unlock( bluice_server->foreign_data_mutex );
	mx_free(argv);

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/

static mx_status_type
stog_set_permission_level( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
#if BLUICE_DCSS_DEBUG
	static const char fname[] = "stog_set_permission_level()"; 

	MX_DEBUG(-2,("%s invoked for message '%s' from server '%s'",
		fname, bluice_server->receive_buffer, server_record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/

static mx_status_type
stog_set_string_completed( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	static const char fname[] = "stog_set_string_completed()";

	MX_BLUICE_FOREIGN_DEVICE *foreign_string;
	char *config_string, *ptr, *token_ptr;
	char *string_name, *string_buffer, *new_string;
	char *string_status;
	size_t string_length;
	mx_status_type mx_status;

	new_string = NULL;

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_BLUICE_SERVER pointer passed was NULL." );
	}

	config_string = bluice_server->receive_buffer;

	/* Skip over the command name. */

	ptr = config_string;

	token_ptr = mx_string_token( &ptr, " " );

	if ( token_ptr == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The message '%s' received from Blu-Ice server '%s' "
		"contained only space characters.",
			bluice_server->receive_buffer,
			bluice_server->record->name );
	}

	/* Get the string name. */

	string_name = mx_string_token( &ptr, " " );

	if ( string_name == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"String name not found in message received from "
		"Blu-Ice server '%s'.", bluice_server->record->name );
	}

	/* Get the string status. */

	string_status = mx_string_token( &ptr, " " );

	if ( string_status == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"String status not found in message received from "
		"Blu-Ice server '%s'.", bluice_server->record->name );
	}

	/* Treat the rest of the receive buffer as the string buffer. */

	string_buffer = ptr;

	if ( string_buffer == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Empty string seen in message received from "
		"Blu-Ice server '%s' for string '%s'.",
			bluice_server->record->name,
			string_name );
	}

	/* Get a pointer to the Blu-Ice foreign string structure. */

	mx_status = mx_bluice_get_device_pointer( bluice_server,
						string_name,
						bluice_server->string_array,
						bluice_server->num_strings,
						&foreign_string );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	string_length = strlen( string_buffer );

	if ( foreign_string->u.string.string_buffer == (char *) NULL ) {

		new_string = (char *) malloc( string_length+1 );

	} else
	if ( string_length > foreign_string->u.string.string_length ) {

		new_string = (char *) realloc(
		  foreign_string->u.string.string_buffer, string_length+1 );
	}

	if ( new_string == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu byte string "
		"for Blu-Ice foreign string '%s'.",
			(unsigned long) string_length, string_name );
	}

	foreign_string->u.string.string_buffer = new_string;

	foreign_string->u.string.string_length = string_length;

	strlcpy( foreign_string->u.string.string_buffer,
			string_buffer, string_length+1 );

#if 0 && BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s: string '%s' = '%s'",
		fname, foreign_string->name,
		foreign_string->u.string.string_buffer));
#endif
	return mx_status;
}

/*------------------------------------------------------------------------*/

static mx_status_type
stog_start_operation( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	static const char fname[] = "stog_start_operation()";

	mx_status_type mx_status;

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s invoked for message '%s' from server '%s'",
		fname, bluice_server->receive_buffer, server_record->name));
#endif
	mx_status = mx_bluice_update_operation_status( bluice_server,
						bluice_server->receive_buffer );
	return mx_status;
}

/*------------------------------------------------------------------------*/

static mx_status_type
stog_update_motor_position( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{ 
	mx_status_type mx_status;

	mx_status = mx_bluice_update_motion_status( bluice_server,
						bluice_server->receive_buffer,
						TRUE );
	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxn_bluice_dcss_server_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxn_bluice_dcss_server_create_record_structures()";

	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_DCSS_SERVER *bluice_dcss_server;
	mx_status_type mx_status;

	/* Allocate memory for the necessary structures. */

	bluice_server = (MX_BLUICE_SERVER *)
				malloc( sizeof(MX_BLUICE_SERVER) );

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_BLUICE_SERVER structure." );
	}

	bluice_dcss_server = (MX_BLUICE_DCSS_SERVER *)
				malloc( sizeof(MX_BLUICE_DCSS_SERVER) );

	if ( bluice_dcss_server == (MX_BLUICE_DCSS_SERVER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_BLUICE_DCSS_SERVER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = bluice_server;;
	record->record_type_struct = bluice_dcss_server;
	record->class_specific_function_list = NULL;

	bluice_server->record = record;
	bluice_server->socket = NULL;

	bluice_dcss_server->record = record;
	bluice_dcss_server->dcss_monitor_thread = NULL;
	bluice_dcss_server->client_number = 0;
	bluice_dcss_server->is_authenticated = FALSE;
	bluice_dcss_server->is_master = FALSE;
	bluice_dcss_server->operation_counter = 0;

	mx_status = mx_mutex_create( &(bluice_server->socket_send_mutex) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mutex_create( &(bluice_server->foreign_data_mutex) );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxn_bluice_dcss_server_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxn_bluice_dcss_server_print_structure()";

	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *device;
	long i, mx_status_code;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	bluice_server = (MX_BLUICE_SERVER *) record->record_class_struct;

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_SERVER pointer for record '%s' is NULL.",
			record->name );
	}

	fprintf( file, "\nBlu-Ice devices for DCSS server record '%s':\n",
		record->name );

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the foreign data mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

	fprintf( file, "\n  %ld ion chambers:\n",
		bluice_server->num_ion_chambers );

	for ( i = 0; i < bluice_server->num_ion_chambers; i++ ) {
		device = bluice_server->ion_chamber_array[i];

		fprintf( file, "    %s  %s\n",
			device->name, device->dhs_server_name );
	}

	fprintf( file, "\n  %ld motors:\n", bluice_server->num_motors );

	for ( i = 0; i < bluice_server->num_motors; i++ ) {
		device = bluice_server->motor_array[i];

		fprintf( file, "    %s  %s\n",
			device->name, device->dhs_server_name );
	}

	fprintf( file, "\n  %ld operations:\n", bluice_server->num_operations );

	for ( i = 0; i < bluice_server->num_operations; i++ ) {
		device = bluice_server->operation_array[i];

		fprintf( file, "    %s  %s\n",
			device->name, device->dhs_server_name );
	}

	fprintf( file, "\n  %ld shutters:\n", bluice_server->num_shutters );

	for ( i = 0; i < bluice_server->num_shutters; i++ ) {
		device = bluice_server->shutter_array[i];

		fprintf( file, "    %s  %s\n",
			device->name, device->dhs_server_name );
	}

	fprintf( file, "\n  %ld strings:\n", bluice_server->num_strings );

	for ( i = 0; i < bluice_server->num_strings; i++ ) {
		device = bluice_server->string_array[i];

		fprintf( file, "    %s  %s\n",
			device->name, device->dhs_server_name );
	}

	fprintf( file, "\n" );

	mx_mutex_unlock( bluice_server->foreign_data_mutex );

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#define CLIENT_TYPE_RESPONSE_LENGTH \
	((2*MXU_HOSTNAME_LENGTH) + MXU_USERNAME_LENGTH + 100)

MX_EXPORT mx_status_type
mxn_bluice_dcss_server_open( MX_RECORD *record )
{
	static const char fname[] = "mxn_bluice_dcss_server_open()";

	MX_LIST_HEAD *list_head;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_DCSS_SERVER *bluice_dcss_server;
	char host_name[MXU_HOSTNAME_LENGTH+1];
	char display_name[MXU_HOSTNAME_LENGTH+10];
	char session_id[MXU_AUTHENTICATION_DATA_LENGTH+1];
	char client_type_response[CLIENT_TYPE_RESPONSE_LENGTH+1];
	char *display_name_ptr;
	int i, num_retries, num_items, need_authentication;
	long num_bytes_available;
	unsigned long wait_ms, flags, mask;
	long actual_data_length;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, record->name));
#endif

	mx_status = MX_SUCCESSFUL_RESULT;

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for record '%s' is NULL.",
			record->name );
	}

	bluice_server = (MX_BLUICE_SERVER *) record->record_class_struct;

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_BLUICE_SERVER pointer for record '%s' is NULL.",
			record->name );
	}

	bluice_dcss_server = (MX_BLUICE_DCSS_SERVER *)
					record->record_type_struct;

	if ( bluice_dcss_server == (MX_BLUICE_DCSS_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_BLUICE_DCSS_SERVER pointer for record '%s' is NULL.",
			record->name );
	}

	/* Enable Blu-Ice network debugging if requested. */

	mask = MXF_NETDBG_SUMMARY | MXF_NETDBG_VERBOSE;

	if ( list_head->network_debug_flags & mask ) {
		mx_bluice_enable_network_debugging( TRUE );
	}

	/* Allocate memory for the receive buffer. */

	bluice_server->receive_buffer =
		(char *) malloc( MX_BLUICE_INITIAL_RECEIVE_BUFFER_LENGTH );

	if ( bluice_server->receive_buffer == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for a Blu-Ice message receive "
		"buffer of length %lu.",
		    (unsigned long) MX_BLUICE_INITIAL_RECEIVE_BUFFER_LENGTH  );
	}

	bluice_server->receive_buffer_length
		= MX_BLUICE_INITIAL_RECEIVE_BUFFER_LENGTH;

	/* Gather information for the response for the initial
	 * 'stoc_send_client_type' message.
	 */

	mx_status = mx_gethostname( host_name, MXU_HOSTNAME_LENGTH );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	display_name_ptr = getenv("DISPLAY");

	if ( display_name_ptr == NULL ) {
		strlcpy( display_name, ":0", sizeof(display_name) );
	} else {
		strlcpy( display_name, display_name_ptr, sizeof(display_name) );
	}

	/* Do we need to authenticate the user? */

	flags = bluice_dcss_server->bluice_dcss_flags;

	if ( flags & MXF_BLUICE_DCSS_REQUIRE_USERNAME ) {
		need_authentication = TRUE;
	} else
	if ( flags & MXF_BLUICE_DCSS_REQUIRE_PASSWORD ) {
		need_authentication = TRUE;
	} else {
		need_authentication = FALSE;
	}

	if ( need_authentication == FALSE ) {

		/* If no authentication is needed, then the
		 * 'authentication_data' field will be sent
		 * to DCSS as the session id.
		 */

		mx_username( bluice_dcss_server->bluice_username,
				MXU_USERNAME_LENGTH );

		strlcpy( session_id,
			bluice_dcss_server->authentication_data,
			sizeof(session_id) );
	} else {
		/* If authentication is needed, then the
		 * 'authentication_data' field must contain
		 * the hostname and port number of the 
		 * authentication server using the format
		 * 
		 *    hostname:portnumber
		 */

		 /* Connect to the authentication server to get
		  * a session ID.
		  */

		mx_status = mxn_bluice_dcss_server_get_session_id(
				bluice_dcss_server,
				bluice_dcss_server->bluice_username,
				MXU_USERNAME_LENGTH,
				session_id,
				sizeof(session_id) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Construct the 'htos_client_is_gui' response from the
	 * information we have gathered.
	 */

	snprintf( client_type_response, sizeof(client_type_response),
		"htos_client_is_gui %s %s %s %s",
		bluice_dcss_server->bluice_username,
		session_id, host_name, display_name );

	/* Now we are ready to connect to the DCSS. */

	mx_status = mx_tcp_socket_open_as_client( &(bluice_server->socket),
				bluice_dcss_server->hostname,
				bluice_dcss_server->port_number,
				MXF_SOCKET_DISABLE_NAGLE_ALGORITHM,
				MX_SOCKET_DEFAULT_BUFFER_SIZE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The DCSS should send us a 'stoc_send_client_type' message
	 * almost immediately.  Wait for this to happen.
	 */

	wait_ms = 100;
	num_retries = 100;

	for ( i = 0; i < num_retries; i++ ) {
		mx_status = mx_socket_num_input_bytes_available(
						bluice_server->socket,
						&num_bytes_available );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( num_bytes_available > 0 )
			break;			/* Exit the for() loop. */

		mx_msleep( wait_ms );
	}

	if ( i >= num_retries ) {
		return mx_error( MXE_TIMED_OUT, fname,
			"Timed out waiting for the initial message from "
			"Blu-Ice DCSS server '%s' after %g seconds.",
				record->name,
				0.001 * (double) ( num_retries * wait_ms ) );
	}

	/* The first message has arrived.  Read it in to the
	 * internal receive buffer.
	 */

	mx_status = mx_bluice_receive_message( record, NULL, 0,
						&actual_data_length, -1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Is the message a 'stoc_send_client_type' message? */

	if (strcmp(bluice_server->receive_buffer,"stoc_send_client_type") != 0)
	{
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Did not receive the 'stoc_send_client_type' message from "
		"Blu-Ice DCSS server '%s' that we were expecting.  "
		"Instead, we received '%s'.  Perhaps the server you have "
		"specified is not a DCSS server?",
			record->name, bluice_server->receive_buffer );
	}

	/* Send back the client type response.  This message must use
	 * Blu-Ice protocol version 1.
	 */

	bluice_server->protocol_version = MX_BLUICE_PROTOCOL_1;

	mx_status = mx_bluice_send_message( record,
					client_type_response, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* FIXME: For now we hard code the DCS protocol for 
	 * subsequent messages to 2.
	 */

	bluice_server->protocol_version = MX_BLUICE_PROTOCOL_2;

	/* If DCSS recognizes the username we sent, it will then send
	 * us a success message.
	 */

	wait_ms = 100;
	num_retries = 100;

	for ( i = 0; i < num_retries; i++ ) {
		mx_status = mx_socket_num_input_bytes_available(
						bluice_server->socket,
						&num_bytes_available );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( num_bytes_available > 0 )
			break;			/* Exit the for() loop. */

		mx_msleep( wait_ms );
	}

	if ( i >= num_retries ) {
		return mx_error( MXE_TIMED_OUT, fname,
			"Timed out waiting for login status from "
			"Blu-Ice DCSS server '%s' after %g seconds.",
				record->name,
				0.001 * (double) ( num_retries * wait_ms ) );
	}

	/* The status message has arrived.  Read it in to the
	 * internal receive buffer.
	 */

	mx_status = mx_bluice_receive_message( record, NULL, 0,
						&actual_data_length, -1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Is the message a 'stoc_login_complete' message? */

	if ( strncmp( bluice_server->receive_buffer,
			"stog_login_complete", 19 ) != 0 )
	{
		return mx_error( MXE_PERMISSION_DENIED, fname,
		"Blu-Ice DCSS login was not successful for user '%s' "
		"at server '%s'.  Response from server = '%s'",
		    bluice_dcss_server->bluice_username,
		    record->name, bluice_server->receive_buffer );
	}

	num_items = sscanf( bluice_server->receive_buffer,
			"stog_login_complete %lu",
			&(bluice_dcss_server->client_number) );

	if ( num_items != 1 ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Did not find the client number in the message '%s' "
		"from DCSS server '%s'.",
			bluice_server->receive_buffer, record->name );
	}

	bluice_dcss_server->is_authenticated = TRUE;

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s: DCSS login successful for client %lu.",
		fname, bluice_dcss_server->client_number));
#endif

	/* At this point we need to create a thread that monitors
	 * messages sent by the DCSS server.  From this point on,
	 * only mxn_bluice_dcss_monitor_thread() is allowed to
	 * receive messages from the DCSS server.
	 */

	mx_status = mx_thread_create(
			&(bluice_dcss_server->dcss_monitor_thread),
			mxn_bluice_dcss_monitor_thread,
			record );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

#define MX_BLUICE_DCSS_CLOSE_WAIT_TIME	(5.0)		/* in seconds */

MX_EXPORT mx_status_type
mxn_bluice_dcss_server_close( MX_RECORD *record )
{
	static const char fname[] = "mxn_bluice_dcss_server_close()";

	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_DCSS_SERVER *bluice_dcss_server;
	MX_SOCKET *server_socket;
	long thread_exit_status;
	mx_status_type mx_status;
	long mx_status_code;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	bluice_server = (MX_BLUICE_SERVER *) record->record_class_struct;

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_SERVER pointer for record '%s' is NULL.",
			record->name );
	}

	bluice_dcss_server = 
		(MX_BLUICE_DCSS_SERVER *) record->record_type_struct;

	if ( bluice_dcss_server == (MX_BLUICE_DCSS_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_DCSS_SERVER pointer for record '%s' is NULL.",
			record->name );
	}

	/* First, tell the monitor thread to exit. */

#if BLUICE_DCSS_DEBUG_SHUTDOWN
	MX_DEBUG(-2,("%s: Stopping DCSS monitor thread.", fname));
#endif

	mx_status = mx_thread_stop( bluice_dcss_server->dcss_monitor_thread );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_thread_wait( bluice_dcss_server->dcss_monitor_thread,
					&thread_exit_status,
					MX_BLUICE_DCSS_CLOSE_WAIT_TIME );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if BLUICE_DCSS_DEBUG_SHUTDOWN
	MX_DEBUG(-2,("%s: DCSS monitor thread for record '%s' "
		"stopped with exit status = %ld",
		fname, record->name, thread_exit_status ));
#endif

	/* Lock the bluice_server mutexes so that other threads cannot
	 * use the data structures while we are destroying them.  Get
	 * copies of the pointers to the mutexes so that we can continue
	 * to hold them while tearing down the bluice_server structure.
	 */

#if BLUICE_DCSS_DEBUG_SHUTDOWN
	MX_DEBUG(-2,("%s: Locking the socket send mutex.", fname));
#endif

	mx_status_code = mx_mutex_lock( bluice_server->socket_send_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the socket send mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

#if BLUICE_DCSS_DEBUG_SHUTDOWN
	MX_DEBUG(-2,("%s: Locking the foreign data mutex.", fname));
#endif

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the foreign data mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

#if BLUICE_DCSS_DEBUG_SHUTDOWN
	MX_DEBUG(-2,("%s: Shutting down the server socket.", fname));
#endif

	server_socket = bluice_server->socket;

	if ( server_socket != NULL ) {
		mx_status = mx_socket_close( server_socket );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	bluice_server->socket = NULL;

	/* FIXME: Successfully destroying all of the interrelated Blu-Ice
	 * data structures in a thread-safe manner is difficult.  For example,
	 * the MX_BLUICE_SERVER has a foreign data mutex that could be used
	 * to prevent access.  But there is a window for a race condition
	 * between the time another thread gets a pointer to the
	 * MX_BLUICE_SERVER structure and the time that thread locks the
	 * foreign data mutex in that structure.  During that window of
	 * opportunity, it would be possible for MX_BLUICE_SERVER structure
	 * to be free()-ed by the thread running this close() routine.
	 * If that happened, then the other thread would core dump when
	 * it tried to access the mutex through the free()-ed 
	 * MX_BLUICE_SERVER pointer.
	 *
	 * For now, all that is important is that we make sure the monitor
	 * thread is shut down before destroying the socket so that the
	 * user does not get an error message from the socket routines.
	 * This means that a lost connection to the Blu-Ice server currently
	 * can only be dealt with by exiting the application program and then
	 * reconnecting.
	 */

	/* FIXME: Return with the mutexes still locked.  We assume for now
	 * that the program is shutting down.
	 */

	return mx_status;

#if 0 /* Disabled for now. */

	/* FIXME: If we did attempt to free all of the data structures,
	 * it would resemble stuff like the following code.
	 */

#if BLUICE_DCSS_DEBUG_SHUTDOWN
	MX_DEBUG(-2,("%s: Destroying ion chamber data structures.", fname));
#endif

	if ( bluice_server->ion_chamber_array != NULL ) {
	    for ( i = 0; i < bluice_server->num_ion_chambers; i++ ) {
		foreign_ion_chamber = bluice_server->ion_chamber_array[i];

		    if ( foreign_ion_chamber != NULL ) {

			/* Reach through to the timer record and delete the
			 * entry in its array of foreign ion chambers.  This
			 * is thread-safe since the 'bluice_timer' record is
			 * expected to lock the Blu-Ice foreign data mutex
			 * before using these data structures.
			 */

			mx_timer = foreign_ion_chamber->mx_timer;

			bluice_timer = (MX_BLUICE_TIMER *)
					mx_timer->record->record_type_struct;

			for ( j = 0; j < bluice_timer->num_ion_chambers; j++ ) {
			    if ( foreign_ion_chamber ==
			    	bluice_timer->foreign_ion_chamber_array[j] )
			    {
			    	bluice_timer->foreign_ion_chamber_array[j]
					= NULL;

				break;    /* Exit the for(j) loop. */
			    }
			}
		    }
		}
		mx_free( foreign_ion_chamber );
	    }
	    mx_free( bluice_server->ion_chamber_array );
	}

#endif /* Disabled for now. */

}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxn_bluice_dcss_server_resynchronize( MX_RECORD *record )
{
#if 1
	static const char fname[] = "mxn_bluice_dcss_server_resynchronize()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Reconnection to Blu-Ice server '%s' is not currently supported.  "
	"The only way to reconnect is to exit your application program "
	"and then start your application again.", record->name );
#else
	mx_status_type mx_status;

	mx_status = mxn_bluice_dcss_server_close( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxn_bluice_dcss_server_open( record );

	return mx_status;
#endif
}

