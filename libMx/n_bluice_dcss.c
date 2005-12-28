/*
 * Name:     n_bluice_dcss.c
 *
 * Purpose:  Client interface to a Blu-Ice DCSS server.
 *
 * Author:   William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define BLUICE_DCSS_DEBUG		FALSE

#define BLUICE_DCSS_DEBUG_SHUTDOWN	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_bluice.h"
#include "d_bluice_timer.h"
#include "n_bluice_dcss.h"

MX_RECORD_FUNCTION_LIST mxn_bluice_dcss_server_record_function_list = {
	NULL,
	mxn_bluice_dcss_server_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
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
static MXN_BLUICE_DCSS_MSG_HANDLER stog_configure_shutter;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_configure_string;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_log;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_motor_move_completed;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_motor_move_started;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_report_ion_chambers;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_report_shutter_state;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_set_permission_level;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_set_string_completed;
static MXN_BLUICE_DCSS_MSG_HANDLER stog_update_motor_position;

static struct {
	char message_name[40];
	MXN_BLUICE_DCSS_MSG_HANDLER *message_handler;
} dcss_handler_list[] = {
	{"stog_become_master", stog_become_master},
	{"stog_become_slave", stog_become_slave},
	{"stog_configure_hardware_host", NULL},
	{"stog_configure_ion_chamber", stog_configure_ion_chamber},
	{"stog_configure_operation", NULL},
	{"stog_configure_pseudo_motor", stog_configure_motor},
	{"stog_configure_real_motor", stog_configure_motor},
	{"stog_configure_run", NULL},
	{"stog_configure_runs", NULL},
	{"stog_configure_shutter", stog_configure_shutter},
	{"stog_configure_string", stog_configure_string},
	{"stog_log", stog_log},
	{"stog_motor_move_completed", stog_motor_move_completed},
	{"stog_motor_move_started", stog_motor_move_started},
	{"stog_report_ion_chambers", stog_report_ion_chambers},
	{"stog_report_shutter_state", stog_report_shutter_state},
	{"stog_set_permission_level", stog_set_permission_level},
	{"stog_set_string_completed", stog_set_string_completed},
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
			"%%%lds", (unsigned long) sizeof(message_type_name) );

	while (1) {
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
				break;		/* Exit the for() loop. */
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

	host_name = mx_string_split( &ptr, ":" );

	if ( host_name == NULL ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"The host name of the Blu-Ice authentication server "
		"was not specified in the authentication data string '%s'",
			bluice_dcss_server->authentication_data );
	}

	port_number_ptr = mx_string_split( &ptr, ":" );

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

#if defined(OS_WIN32)
	/* FIXME: We need to change how mx_socket_getline() is implemented. */

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Line buffered socket I/O is not yet supported correctly by MX "
	"on Win32, so we cannot talk to the authentication server." );
#endif

	/* Connect to the authentication server. */

	mx_status = mx_tcp_socket_open_as_client( &auth_server_socket,
			host_name, port_number, 0,
			MX_SOCKET_DEFAULT_BUFFER_SIZE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* FIXME: mx_socket_getline() throws away trailing characters
	 * after the first set of CR-LF terminators seen.  Temporarily
	 * we deal with this by converting the socket descriptor to
	 * a FILE pointer.
	 */

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
		fgets( line, sizeof(line), auth_server_fp );

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

			 prefix = mx_string_split( &ptr, "=" );

			 if ( prefix == NULL ) {
			 	return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Did not find the '=' character before the "
				"start of the session ID cookie line '%s'.",
					line );
			 }

			 session_id_ptr = mx_string_split( &ptr, ";" );

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
		"server '%s' failed.", bluice_server->record->name ); }

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

static mx_status_type
stog_configure_ion_chamber( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	mx_status_type mx_status;

	mx_status = mx_bluice_configure_ion_chamber( bluice_server,
					bluice_server->receive_buffer );
	return mx_status;
}

static mx_status_type
stog_configure_motor( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	mx_status_type mx_status;

	mx_status = mx_bluice_configure_motor( bluice_server,
					bluice_server->receive_buffer );
	return mx_status;
}

static mx_status_type
stog_configure_shutter( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	mx_status_type mx_status;

	mx_status = mx_bluice_configure_shutter( bluice_server,
					bluice_server->receive_buffer );
	return mx_status;
}

static mx_status_type
stog_configure_string( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	mx_status_type mx_status;

	mx_status = mx_bluice_configure_string( bluice_server,
					bluice_server->receive_buffer );
	return mx_status;
}

static mx_status_type
stog_log( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
#if BLUICE_DCSS_DEBUG
	static const char fname[] = "stog_log()";
#endif

	int severity, locale;
	char device_name[MXU_BLUICE_NAME_LENGTH+1];
	char message_body[500];
	mx_status_type mx_status;

	mx_status = mx_bluice_parse_log_message( bluice_server->receive_buffer,
					&severity, &locale,
					device_name, sizeof(device_name),
					message_body, sizeof(message_body) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,
    ("%s: severity = %d, locale = %d, device_name = '%s', message_body = '%s'.",
		fname, severity, locale, device_name, message_body));
#endif

	return MX_SUCCESSFUL_RESULT;
}

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
	char format[80];
	char first_ion_chamber_name[MXU_BLUICE_NAME_LENGTH+1];
	char *ptr, *ion_chamber_name, *token_ptr;
	int num_items;
	long i, num_ion_chambers;
	double measurement_value;
	mx_status_type mx_status;
	long mx_status_code;

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s invoked for message '%s' from server '%s'",
		fname, bluice_server->receive_buffer, server_record->name));
#endif

	/* Find the name of the first ion chamber, so that we can find
	 * the associated MX_BLUICE_TIMER structure.
	 */

	snprintf( format, sizeof(format),
		"%%*s %%*s %%%ds", MXU_BLUICE_NAME_LENGTH );

	num_items = sscanf( bluice_server->receive_buffer, format,
				first_ion_chamber_name );

	if ( num_items != 1 ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Did not find the name of the first ion chamber in "
		"the message '%s' from Blu-Ice server '%s'.",
			bluice_server->receive_buffer,
			bluice_server->record->name );
	}

	/* First, get the ion chamber pointer for the first ion chamber. */

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
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

		return mx_status;
	}

	if ( first_ion_chamber == (MX_BLUICE_FOREIGN_DEVICE *) NULL ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The ion chamber pointer returned for ion chamber '%s' "
		"is NULL.", first_ion_chamber_name );
	}

	/* Get the timer from the first_ion_chamber structure. */

	timer = first_ion_chamber->u.ion_chamber.mx_timer;

	if ( timer == (MX_TIMER *) NULL ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );

		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The MX_TIMER pointer for ion chamber '%s' has not "
		"been initialized.", first_ion_chamber_name );
	}

	if ( timer->record == (MX_RECORD *) NULL ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );

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

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_TIMER pointer for Blu-Ice timer '%s' is NULL.",
			timer->record->name );
	}

	num_ion_chambers = bluice_timer->num_ion_chambers;

	ion_chamber_array = bluice_timer->foreign_ion_chamber_array;

	if ( ion_chamber_array == (MX_BLUICE_FOREIGN_DEVICE **) NULL ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );

		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The array of ion chamber pointers for Blu-Ice timer '%s' "
		"has not been initialized.", timer->record->name );
	}

	/* Mark the current measurement as being over. */

	bluice_timer->measurement_in_progress = FALSE;

	/*** Now we can walk through the message received from the server. ***/

	ptr = bluice_server->receive_buffer;

	/* Skip over the command name. */

	token_ptr = mx_string_split( &ptr, " " );

	if ( token_ptr == NULL ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The supposed stog_report_ion_chambers message from "
		"Blu-Ice server '%s' is actually blank.  "
		"This should never happen.", bluice_server->record->name );
	}

	/* Skip over the measurement time. */

	token_ptr = mx_string_split( &ptr, " " );

	if ( token_ptr == NULL ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Could not find the measurement time string in the message "
		"from Blu-Ice server '%s'.", bluice_server->record->name );
	}

	while (1) {
		/* The next string should be an ion chamber name. */

		ion_chamber_name = mx_string_split( &ptr, " " );

		if ( ion_chamber_name == NULL ) {

			/* We have found the end of the list of ion chambers,
			 * so break out of the while() loop.
			 */

			break;
		}

		/* The next string should be the ion chamber measurement. */

		token_ptr = mx_string_split( &ptr, " " );

		if ( token_ptr == NULL ) {
			mx_mutex_unlock( bluice_server->foreign_data_mutex );

			return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"No measurement was reported for ion chamber '%s' "
			"according to Blu-Ice server '%s'.",
				ion_chamber_name, bluice_server->record->name );
		}

		measurement_value = atof( token_ptr );

		/* Search for the ion chamber in the timer's 
		 * ion chamber array.
		 */

		for ( i = 0; i < num_ion_chambers; i++ ) {
			foreign_ion_chamber = ion_chamber_array[i];

			if ( foreign_ion_chamber == NULL ) {
				mx_warning( "Skipping NULL ion chamber %ld "
				"for Blu-Ice timer '%s'.",
					i, timer->record->name );
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

		if ( i >= num_ion_chambers ) {
			mx_mutex_unlock( bluice_server->foreign_data_mutex );

			return mx_error( MXE_INITIALIZATION_ERROR, fname,
			"Ion chamber '%s' was not found in the "
			"ion chamber array for Blu-Ice timer '%s'.",
				ion_chamber_name, timer->record->name );
		}
	}

	mx_mutex_unlock( bluice_server->foreign_data_mutex );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
stog_report_shutter_state( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	static const char fname[] = "stog_report_shutter_state()"; 

	MX_BLUICE_FOREIGN_DEVICE *foreign_shutter;
	char *ptr, *token_ptr, *shutter_name;
	int shutter_status;
	mx_status_type mx_status;
	long mx_status_code;

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s invoked for message '%s' from server '%s'",
		fname, bluice_server->receive_buffer, server_record->name));
#endif

	/* Skip over the command name. */

	ptr = bluice_server->receive_buffer;

	token_ptr = mx_string_split( &ptr, " " );

	if ( token_ptr == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The message '%s' received from Blu-Ice server '%s' "
		"contained only space characters.",
			bluice_server->receive_buffer, server_record->name );
	}

	/* Get the shutter name. */

	shutter_name = mx_string_split( &ptr, " " );

	if ( shutter_name == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
	"Shutter name not found in message received from Blu-Ice server '%s'.",
			server_record->name );
	}

	/* Get the shutter status. */

	token_ptr = mx_string_split( &ptr, " " );

	if ( token_ptr == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Did not find the shutter status in a message received from "
		"Blu-Ice server '%s'.", server_record->name );
	}

	if ( strcmp( token_ptr, "open" ) == 0 ) {
		shutter_status = MXF_RELAY_IS_OPEN;
	} else
	if ( strcmp( token_ptr, "closed" ) == 0 ) {
		shutter_status = MXF_RELAY_IS_CLOSED;
	} else {
		shutter_status = MXF_RELAY_ILLEGAL_STATUS;

		mx_warning(
	"Illegal shutter status '%s' returned for Blu-Ice shutter '%s'.",
			token_ptr, shutter_name );
	}

	/* Update the value in the shutter structure. */

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
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

		return mx_status;
	}

	if ( foreign_shutter == (MX_BLUICE_FOREIGN_DEVICE *) NULL ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );

		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The MX_BLUICE_FOREIGN_DEVICE pointer for DCSS shutter '%s' "
		"has not been initialized.", shutter_name );
	}

	/* Update the shutter status. */

	foreign_shutter->u.shutter.shutter_status = shutter_status;

	mx_mutex_unlock( bluice_server->foreign_data_mutex );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
stog_set_string_completed( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DCSS_SERVER *bluice_dcss_server )
{
	mx_status_type mx_status;

	mx_status = mx_bluice_configure_string( bluice_server,
						bluice_server->receive_buffer );

	return mx_status;
}

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

	mx_status = mx_mutex_create( &(bluice_server->socket_send_mutex) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mutex_create( &(bluice_server->foreign_data_mutex) );

	return mx_status;
}

#define CLIENT_TYPE_RESPONSE_LENGTH \
	((2*MXU_HOSTNAME_LENGTH) + MXU_USERNAME_LENGTH + 100)

MX_EXPORT mx_status_type
mxn_bluice_dcss_server_open( MX_RECORD *record )
{
	static const char fname[] = "mxn_bluice_dcss_server_open()";

	MX_LIST_HEAD *list_head;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_DCSS_SERVER *bluice_dcss_server;
	char user_name[MXU_USERNAME_LENGTH+1];
	char host_name[MXU_HOSTNAME_LENGTH+1];
	char display_name[MXU_HOSTNAME_LENGTH+10];
	char session_id[MXU_AUTHENTICATION_DATA_LENGTH+1];
	char client_type_response[CLIENT_TYPE_RESPONSE_LENGTH+1];
	char *display_name_ptr;
	int i, num_retries, num_bytes_available, num_items, need_authentication;
	unsigned long wait_ms, flags;
	long actual_data_length;
	mx_status_type mx_status;

#if BLUICE_DCSS_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
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

		mx_username( user_name, MXU_USERNAME_LENGTH );

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
				user_name,
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
		user_name, session_id, host_name, display_name );

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

	/* Send back the client type response. */

	mx_status = mx_bluice_send_message( record,
					client_type_response, NULL, 0, 200 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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
		    user_name, record->name, bluice_server->receive_buffer );
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
	MX_DEBUG(-2,("%s: DCSS monitor thread stopped with exit status = %ld",
		fname, thread_exit_status ));
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

