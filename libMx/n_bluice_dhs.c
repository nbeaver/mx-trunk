/*
 * Name:     n_bluice_dhs.c
 *
 * Purpose:  Client interface to a Blu-Ice DHS server.
 *
 * Author:   William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2008, 2010, 2015-2016, 2018, 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define BLUICE_DHS_DEBUG		TRUE

#define BLUICE_DHS_DEBUG_SHUTDOWN	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_net.h"
#include "mx_bluice.h"
#include "d_bluice_motor.h"
#include "d_bluice_timer.h"
#include "v_bluice_string.h"
#include "n_bluice_dhs.h"

MX_RECORD_FUNCTION_LIST mxn_bluice_dhs_server_record_function_list = {
	NULL,
	mxn_bluice_dhs_server_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxn_bluice_dhs_server_open,
	mxn_bluice_dhs_server_close,
	NULL,
	mxn_bluice_dhs_server_resynchronize
};

MX_RECORD_FIELD_DEFAULTS mxn_bluice_dhs_server_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXN_BLUICE_DHS_SERVER_STANDARD_FIELDS
};

long mxn_bluice_dhs_server_num_record_fields
		= sizeof( mxn_bluice_dhs_server_rf_defaults )
		    / sizeof( mxn_bluice_dhs_server_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxn_bluice_dhs_server_rfield_def_ptr
			= &mxn_bluice_dhs_server_rf_defaults[0];

/*-------------------------------------------------------------------------*/

typedef mx_status_type (MXN_BLUICE_DHS_MSG_HANDLER)( MX_THREAD *,
		MX_RECORD *, MX_BLUICE_SERVER *, MX_BLUICE_DHS_SERVER *);

static MXN_BLUICE_DHS_MSG_HANDLER htos_motor_move_completed;
static MXN_BLUICE_DHS_MSG_HANDLER htos_motor_move_started;
static MXN_BLUICE_DHS_MSG_HANDLER htos_operation_completed;
static MXN_BLUICE_DHS_MSG_HANDLER htos_operation_update;
static MXN_BLUICE_DHS_MSG_HANDLER htos_report_ion_chambers;
static MXN_BLUICE_DHS_MSG_HANDLER htos_report_shutter_state;
static MXN_BLUICE_DHS_MSG_HANDLER htos_send_configuration;
static MXN_BLUICE_DHS_MSG_HANDLER htos_set_string_completed;
static MXN_BLUICE_DHS_MSG_HANDLER htos_update_motor_position;

static struct {
	char message_name[40];
	MXN_BLUICE_DHS_MSG_HANDLER *message_handler;
} dhs_handler_list[] = {
	{"htos_motor_move_completed", htos_motor_move_completed},
	{"htos_motor_move_started", htos_motor_move_started},
	{"htos_operation_completed", htos_operation_completed},
	{"htos_operation_update", htos_operation_update},
	{"htos_report_ion_chambers", htos_report_ion_chambers},
	{"htos_report_shutter_state", htos_report_shutter_state},
	{"htos_send_configuration", htos_send_configuration},
	{"htos_set_string_completed", htos_set_string_completed},
	{"htos_update_motor_position", htos_update_motor_position},
};

static int num_dhs_handlers = sizeof( dhs_handler_list )
				/ sizeof( dhs_handler_list[0] );

/*-------------------------------------------------------------------------*/

/* Once started, mxn_bluice_dhs_monitor_thread() is the only thread
 * that is allowed to receive messages from DHS.
 */

static mx_status_type
mxn_bluice_dhs_monitor_thread( MX_THREAD *thread, void *args )
{
	static const char fname[] = "mxn_bluice_dhs_monitor_thread()";

	MX_RECORD *dhs_server_record;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_DHS_SERVER *bluice_dhs_server;
	MXN_BLUICE_DHS_MSG_HANDLER *message_fn;
	long actual_data_length;
	char message_type_name[80];
	char message_type_format[20];
	int i, num_items;
	unsigned long wait_ms;
	long num_bytes_available;
	mx_bool_type socket_is_open;
	mx_status_type mx_status;

	if ( args == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	dhs_server_record = (MX_RECORD *) args;

#if BLUICE_DHS_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, dhs_server_record->name));
#endif

	bluice_server = (MX_BLUICE_SERVER *)
				dhs_server_record->record_class_struct;

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_SERVER pointer for record '%s' is NULL.",
			dhs_server_record->name );
	}

	bluice_dhs_server = (MX_BLUICE_DHS_SERVER *)
				dhs_server_record->record_type_struct;

	if ( bluice_dhs_server == (MX_BLUICE_DHS_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_DHS_SERVER pointer for record '%s' is NULL.",
			dhs_server_record->name );
	}

	snprintf( message_type_format, sizeof(message_type_format),
			"%%%lus", (unsigned long) sizeof(message_type_name) );

	wait_ms = 100;

	for (;;) {
		socket_is_open = mx_socket_is_open( bluice_server->socket );

		if ( socket_is_open == FALSE ) {
#if BLUICE_DHS_DEBUG
			MX_DEBUG(-2,("%s: DHS '%s' socket is not open.",
				fname, dhs_server_record->name ));
#endif

			mx_msleep(wait_ms);

			/* Go back to the top of the for(;;) loop. */

			continue;
		}

		/* Has the DHS process sent a new message? */

		mx_status = mx_socket_num_input_bytes_available(
					bluice_server->socket,
					&num_bytes_available );

		if ( mx_status.code != MXE_SUCCESS ) {
#if BLUICE_DHS_DEBUG
			MX_DEBUG(-2,("%s: DHS '%s' socket %d had an error.",
				fname, dhs_server_record->name,
				(int) bluice_server->socket->socket_fd));
#endif
			return mx_status;
		}

		if ( num_bytes_available <= 0 ) {
#if BLUICE_DHS_DEBUG
			MX_DEBUG( 2,("%s: No new messages are available from "
				"DHS server '%s' socket %d.",
				fname, dhs_server_record->name,
				(int) bluice_server->socket->socket_fd ));
#endif

			mx_msleep(wait_ms);

			/* Go back to the top of the for(;;) loop. */

			continue;
		}

#if BLUICE_DHS_DEBUG
		MX_DEBUG(-2,("%s: DHS '%s', socket %d, %ld bytes are "
		"available -> calling mx_bluice_receive_message().",
			fname, dhs_server_record->name,
			(int) bluice_server->socket->socket_fd,
			num_bytes_available ));
#endif

		mx_status = mx_bluice_receive_message( dhs_server_record,
					NULL, 0, &actual_data_length, -1 );

#if BLUICE_DHS_DEBUG
		MX_DEBUG(-2,
		("%s: mx_status.code = %ld, mx_status.location = '%s', "
		"mx_status.message = '%s'",
			fname, mx_status.code, mx_status.location,
			mx_status.message));
#endif

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if BLUICE_DHS_DEBUG
		MX_DEBUG(-2,("%s: received '%s' from Blu-Ice server '%s'.",
			fname, bluice_server->receive_buffer,
			dhs_server_record->name ));
#endif

		/* Figure out what kind of message this is. */

		num_items = sscanf( bluice_server->receive_buffer,
				message_type_format,
				message_type_name );

		if ( num_items != 1 ) {
			return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Message type name not found in message '%s' "
			"received from DHS server '%s'.",
				bluice_server->receive_buffer,
				dhs_server_record->name );
		}

		for ( i = 0; i < num_dhs_handlers; i++ ) {
			if ( strcmp( message_type_name, 
				dhs_handler_list[i].message_name ) == 0 )
			{
				break;		/* Exit the for(i) loop. */
			}
		}

		if ( i >= num_dhs_handlers ) {
			(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Message type '%s' received in message '%s' from "
			"DHS server '%s' does not currently have a handler.",
				message_type_name,
				bluice_server->receive_buffer,
				dhs_server_record->name );
		} else {
			message_fn = dhs_handler_list[i].message_handler;

			if ( message_fn == NULL ) {
				MX_DEBUG( 2,("%s: Message type '%s' SKIPPED.",
					fname, message_type_name ));
			} else {
				MX_DEBUG( 2,("%s: Invoking handler for '%s'",
						fname, message_type_name));

				mx_status = (*message_fn)( thread,
						dhs_server_record,
						bluice_server,
						bluice_dhs_server );
			}
		}

		mx_status = mx_thread_check_for_stop_request( thread );
	}

#if ( defined(OS_HPUX) && !defined(__ia64) )
	return MX_SUCCESSFUL_RESULT;
#endif

}

/*-------------------------------------------------------------------------*/

static mx_status_type
stoh_configure_real_motor( MX_RECORD *motor_record,
			MX_BLUICE_MOTOR *bluice_motor,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DHS_SERVER *bluice_dhs_server )
{
	static const char fname[] = "stoh_configure_real_motor()";

	MX_BLUICE_FOREIGN_DEVICE *fdev;
	char command[3000];
	long mx_status_code;
	mx_status_type mx_status;
	
#if BLUICE_DHS_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'",
		fname, motor_record->name));
#endif

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the foreign data mutex for Blu-Ice "
		"DHS server '%s' on behalf of motor '%s' failed.",
			bluice_server->record->name, motor_record->name );
	}

	fdev = bluice_motor->foreign_device;

	if ( fdev == (MX_BLUICE_FOREIGN_DEVICE *) NULL ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );

		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The MX_BLUICE_FOREIGN_DEVICE pointer for "
		"Blu-Ice motor '%s' has not yet been initialized.",
			motor_record->name );
	}

	snprintf( command, sizeof(command),
	"stoh_configure_real_motor %s %s %s %f %f %f %f %f %f %f 1 1 0 0 0 0",
		motor_record->name,
		bluice_dhs_server->dhs_name,
		bluice_motor->bluice_name,
		fdev->u.motor.position,
		fdev->u.motor.upper_limit,
		fdev->u.motor.lower_limit,
		fdev->u.motor.scale_factor,
		fdev->u.motor.speed,
		fdev->u.motor.acceleration_time,
		fdev->u.motor.backlash );

	mx_mutex_unlock( bluice_server->foreign_data_mutex );

	mx_status = mx_bluice_send_message( bluice_server->record,
						command, NULL, 0 );
	return mx_status;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
htos_motor_move_completed( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DHS_SERVER *bluice_dhs_server )
{
	mx_status_type mx_status;

	mx_status = mx_bluice_update_motion_status( bluice_server,
						bluice_server->receive_buffer,
						FALSE );
	return mx_status;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
htos_motor_move_started( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DHS_SERVER *bluice_dhs_server )
{
	mx_status_type mx_status;

	mx_status = mx_bluice_update_motion_status( bluice_server,
						bluice_server->receive_buffer,
						TRUE );
	return mx_status;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
htos_operation_completed( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DHS_SERVER *bluice_dhs_server )
{
	static const char fname[] = "stog_operation_completed()";

	mx_status_type mx_status;

#if BLUICE_DHS_DEBUG
	MX_DEBUG(-2,("%s invoked for message '%s' from server '%s'",
		fname, bluice_server->receive_buffer, server_record->name));
#endif
	mx_status = mx_bluice_update_operation_status( bluice_server,
						bluice_server->receive_buffer );
	return mx_status;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
htos_operation_update( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DHS_SERVER *bluice_dhs_server )
{
	static const char fname[] = "stog_operation_update()";

	mx_status_type mx_status;

#if BLUICE_DHS_DEBUG
	MX_DEBUG(-2,("%s invoked for message '%s' from server '%s'",
		fname, bluice_server->receive_buffer, server_record->name));
#endif
	mx_status = mx_bluice_update_operation_status( bluice_server,
						bluice_server->receive_buffer );
	return mx_status;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
htos_report_ion_chambers( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DHS_SERVER *bluice_dhs_server )
{
	static const char fname[] = "htos_report_ion_chambers()"; 

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

#if BLUICE_DHS_DEBUG
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

	token_ptr = mx_string_token( &ptr, " " );

	if ( token_ptr == NULL ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The supposed htos_report_ion_chambers message from "
		"Blu-Ice server '%s' is actually blank.  "
		"This should never happen.", bluice_server->record->name );
	}

	/* Skip over the measurement time. */

	token_ptr = mx_string_token( &ptr, " " );

	if ( token_ptr == NULL ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Could not find the measurement time string in the message "
		"from Blu-Ice server '%s'.", bluice_server->record->name );
	}

	for (;;) {
		/* The next string should be an ion chamber name. */

		ion_chamber_name = mx_string_token( &ptr, " " );

		if ( ion_chamber_name == NULL ) {

			/* We have found the end of the list of ion chambers,
			 * so break out of the for(;;) loop.
			 */

			break;
		}

		/* The next string should be the ion chamber measurement. */

		token_ptr = mx_string_token( &ptr, " " );

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
htos_report_shutter_state( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DHS_SERVER *bluice_dhs_server )
{
	static const char fname[] = "htos_report_shutter_state()"; 

	MX_BLUICE_FOREIGN_DEVICE *foreign_shutter;
	char *ptr, *token_ptr, *shutter_name;
	int shutter_status;
	mx_status_type mx_status;
	long mx_status_code;

#if BLUICE_DHS_DEBUG
	MX_DEBUG(-2,("%s invoked for message '%s' from server '%s'",
		fname, bluice_server->receive_buffer, server_record->name));
#endif

	/* Skip over the command name. */

	ptr = bluice_server->receive_buffer;

	token_ptr = mx_string_token( &ptr, " " );

	if ( token_ptr == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The message '%s' received from Blu-Ice server '%s' "
		"contained only space characters.",
			bluice_server->receive_buffer, server_record->name );
	}

	/* Get the shutter name. */

	shutter_name = mx_string_token( &ptr, " " );

	if ( shutter_name == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
	"Shutter name not found in message received from Blu-Ice server '%s'.",
			server_record->name );
	}

	/* Get the shutter status. */

	token_ptr = mx_string_token( &ptr, " " );

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
#if 1
		shutter_status = MXF_RELAY_IS_CLOSED;
#else
		shutter_status = MXF_RELAY_ILLEGAL_STATUS;

		mx_warning(
	"Illegal shutter status '%s' returned for Blu-Ice shutter '%s'.",
			token_ptr, shutter_name );
#endif
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
		"The MX_BLUICE_FOREIGN_DEVICE pointer for DHS shutter '%s' "
		"has not been initialized.", shutter_name );
	}

	/* Update the shutter status. */

	foreign_shutter->u.shutter.shutter_status = shutter_status;

	mx_mutex_unlock( bluice_server->foreign_data_mutex );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
htos_send_configuration( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DHS_SERVER *bluice_dhs_server )
{ 
	static const char fname[] = "htos_send_configuration()";

	MX_RECORD *current_record;
	MX_BLUICE_MOTOR *bluice_motor;
	char *ptr, *token_ptr, *device_name;
	mx_status_type mx_status;

	/* Skip over the command name. */

	ptr = bluice_server->receive_buffer;

	token_ptr = mx_string_token( &ptr, " " );

	if ( token_ptr == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The supposed htos_send_configuration message from "
		"Blu-Ice server '%s' is actually blank.  "
		"This should never happen.", bluice_server->record->name );
	}

	/* The next string should be the name of our device. */

	device_name = mx_string_token( &ptr, " " );

	if ( device_name == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"No device name was found in the htos_send_configuration "
		"message from Blu-Ice DHS server '%s'.",
			bluice_server->record->name );
	}

	/* Look for the MX record that corresponds to this Blu-Ice device. */

	current_record = server_record->next_record;

	while( current_record != server_record ) {

		switch( current_record->mx_type ) {
		case MXT_MTR_BLUICE_DHS:
		    bluice_motor = current_record->record_type_struct;

		    if ( (server_record == bluice_motor->bluice_server_record )
		      && (strcmp(device_name, current_record->name) == 0) )
		    {
		    	mx_status = stoh_configure_real_motor(
					current_record, bluice_motor,
					bluice_server, bluice_dhs_server );

			return mx_status;
		    }
		    break;
		}

		current_record = current_record->next_record;
	}

	return mx_error( MXE_NOT_FOUND, fname,
	"Blu-Ice DHS server '%s' requested the configuration of "
	"Blu-Ice device '%s' which does not match any of the "
	"device records in the MX database.",
		server_record->name, device_name );
}

static mx_status_type
htos_set_string_completed( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DHS_SERVER *bluice_dhs_server )
{ 
	static const char fname[] = "htos_set_string_completed()";

	MX_RECORD *current_record;
	MX_BLUICE_STRING *bluice_string;
	MX_BLUICE_FOREIGN_DEVICE *fdev;
	char *ptr, *token_ptr, *string_name, *string_status;
	char *fstring_ptr;

	/* Skip over the command name. */

	ptr = bluice_server->receive_buffer;

	token_ptr = mx_string_token( &ptr, " " );

	if ( token_ptr == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The supposed htos_set_string_completed message from "
		"Blu-Ice server '%s' is actually blank.  "
		"This should never happen.", bluice_server->record->name );
	}

	/* The next string should be the name of our string. */

	string_name = mx_string_token( &ptr, " " );

	if ( string_name == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"No string name was found in the htos_set_string_completed "
		"message from Blu-Ice DHS server '%s'.",
			bluice_server->record->name );
	}

	/* Get the string status. */

	string_status = mx_string_token( &ptr, " " );

	if ( string_status == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"String status not found in message received from "
		"Blu-Ice server '%s'.", bluice_server->record->name );
	}

	/* Look for the MX record that corresponds to this Blu-Ice string. */

	current_record = server_record->next_record;

	while( current_record != server_record ) {

		switch( current_record->mx_type ) {
		case MXV_BLUICE_DHS_STRING:
		    bluice_string = current_record->record_type_struct;

		    if ( (server_record == bluice_string->bluice_server_record )
		      && (strcmp(string_name, bluice_string->bluice_name) == 0))
		    {
			fdev = bluice_string->foreign_device;

			fstring_ptr = fdev->u.string.string_buffer;

			strlcpy( fstring_ptr, ptr, MXU_BLUICE_STRING_LENGTH );
		    }
		    break;
		}

		current_record = current_record->next_record;
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
htos_update_motor_position( MX_THREAD *thread,
			MX_RECORD *server_record,
			MX_BLUICE_SERVER *bluice_server,
			MX_BLUICE_DHS_SERVER *bluice_dhs_server )
{ 
	mx_status_type mx_status;

	mx_status = mx_bluice_update_motion_status( bluice_server,
						bluice_server->receive_buffer,
						TRUE );
	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxn_bluice_dhs_server_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxn_bluice_dhs_server_create_record_structures()";

	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_DHS_SERVER *bluice_dhs_server;
	mx_status_type mx_status;

	/* Allocate memory for the necessary structures. */

	bluice_server = (MX_BLUICE_SERVER *)
				malloc( sizeof(MX_BLUICE_SERVER) );

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_BLUICE_SERVER structure." );
	}

	bluice_dhs_server = (MX_BLUICE_DHS_SERVER *)
				malloc( sizeof(MX_BLUICE_DHS_SERVER) );

	if ( bluice_dhs_server == (MX_BLUICE_DHS_SERVER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_BLUICE_DHS_SERVER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = bluice_server;;
	record->record_type_struct = bluice_dhs_server;
	record->class_specific_function_list = NULL;

	bluice_server->record = record;
	bluice_server->socket = NULL;

	bluice_dhs_server->record = record;
	bluice_dhs_server->dhs_monitor_thread = NULL;

	mx_status = mx_mutex_create( &(bluice_server->socket_send_mutex) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mutex_create( &(bluice_server->foreign_data_mutex) );

	return mx_status;
}

#define CLIENT_TYPE_RESPONSE_LENGTH \
	((2*MXU_HOSTNAME_LENGTH) + MXU_USERNAME_LENGTH + 100)

MX_EXPORT mx_status_type
mxn_bluice_dhs_server_open( MX_RECORD *record )
{
	static const char fname[] = "mxn_bluice_dhs_server_open()";

	MX_LIST_HEAD *list_head;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_DHS_SERVER *bluice_dhs_server;
	unsigned long mask;
	char thread_name[50];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

#if BLUICE_DHS_DEBUG
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

	bluice_dhs_server = (MX_BLUICE_DHS_SERVER *)
					record->record_type_struct;

	if ( bluice_dhs_server == (MX_BLUICE_DHS_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_BLUICE_DHS_SERVER pointer for record '%s' is NULL.",
			record->name );
	}

	/* Enable Blu-Ice network debugging_if requested. */

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

#if 0
	/* Listen for connections from the DHS on the server port. */

	mx_status = mx_tcp_socket_open_as_server( &(bluice_server->socket),
				bluice_dhs_server->port_number,
				MXF_SOCKET_DISABLE_NAGLE_ALGORITHM,
				MX_SOCKET_DEFAULT_BUFFER_SIZE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	/* At this point we need to create a thread that monitors
	 * messages sent by the DHS server.  From this point on,
	 * only mxn_bluice_dhs_monitor_thread() is allowed to
	 * receive messages from the DHS server.
	 */

	snprintf( thread_name, sizeof(thread_name), "MON %s", record->name );

	mx_status = mx_thread_create(
			&(bluice_dhs_server->dhs_monitor_thread),
			thread_name,
			mxn_bluice_dhs_monitor_thread,
			record );

	return mx_status;
}

#define MX_BLUICE_DHS_CLOSE_WAIT_TIME	(5.0)		/* in seconds */

MX_EXPORT mx_status_type
mxn_bluice_dhs_server_close( MX_RECORD *record )
{
	static const char fname[] = "mxn_bluice_dhs_server_close()";

	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_DHS_SERVER *bluice_dhs_server;
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

	bluice_dhs_server = 
		(MX_BLUICE_DHS_SERVER *) record->record_type_struct;

	if ( bluice_dhs_server == (MX_BLUICE_DHS_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_DHS_SERVER pointer for record '%s' is NULL.",
			record->name );
	}

	/* First, tell the monitor thread to exit. */

#if BLUICE_DHS_DEBUG_SHUTDOWN
	MX_DEBUG(-2,("%s: Stopping DHS monitor thread.", fname));
#endif

	mx_status = mx_thread_stop( bluice_dhs_server->dhs_monitor_thread );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_thread_wait( bluice_dhs_server->dhs_monitor_thread,
					&thread_exit_status,
					MX_BLUICE_DHS_CLOSE_WAIT_TIME );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if BLUICE_DHS_DEBUG_SHUTDOWN
	MX_DEBUG(-2,("%s: DHS monitor thread for record '%s' "
		"stopped with exit status = %ld",
		fname, record->name, thread_exit_status ));
#endif

	/* Lock the bluice_server mutexes so that other threads cannot
	 * use the data structures while we are destroying them.  Get
	 * copies of the pointers to the mutexes so that we can continue
	 * to hold them while tearing down the bluice_server structure.
	 */

#if BLUICE_DHS_DEBUG_SHUTDOWN
	MX_DEBUG(-2,("%s: Locking the socket send mutex.", fname));
#endif

	mx_status_code = mx_mutex_lock( bluice_server->socket_send_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the socket send mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

#if BLUICE_DHS_DEBUG_SHUTDOWN
	MX_DEBUG(-2,("%s: Locking the foreign data mutex.", fname));
#endif

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the foreign data mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

#if BLUICE_DHS_DEBUG_SHUTDOWN
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

#if BLUICE_DHS_DEBUG_SHUTDOWN
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

	return MX_SUCCESSFUL_RESULT;

#endif /* Disabled for now. */
}

MX_EXPORT mx_status_type
mxn_bluice_dhs_server_resynchronize( MX_RECORD *record )
{
#if 1
	static const char fname[] = "mxn_bluice_dhs_server_resynchronize()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Reconnection to Blu-Ice server '%s' is not currently supported.  "
	"The only way to reconnect is to exit your application program "
	"and then start your application again.", record->name );
#else
	mx_status_type mx_status;

	mx_status = mxn_bluice_dhs_server_close( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxn_bluice_dhs_server_open( record );

	return mx_status;
#endif
}

