/*
 * Name:    d_gittelsohn_pulser.c
 *
 * Purpose: MX pulse generator driver for Mark Gittelsohn's Arduino-based
 *          pulse generator.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2015-2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_GITTELSOHN_PULSER_DEBUG			FALSE

#define MXD_GITTELSOHN_PULSER_DEBUG_CONF		TRUE

#define MXD_GITTELSOHN_PULSER_DEBUG_RUNNING		TRUE

#define MXD_GITTELSOHN_PULSER_DEBUG_SETUP		TRUE

#define MXD_GITTELSOHN_PULSER_DEBUG_LAST_PULSE_NUMBER	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_hrt.h"
#include "mx_ascii.h"
#include "mx_cfn.h"
#include "mx_rs232.h"
#include "mx_pulse_generator.h"
#include "d_gittelsohn_pulser.h"

/* Flag bits for mxd_gittelsohn_pulser_command(). */

#define MXF_GPC_NO_DISCARD	0x1

/* Initialize the pulse generator driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_gittelsohn_pulser_record_function_list = {
	NULL,
	mxd_gittelsohn_pulser_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_gittelsohn_pulser_open,
	NULL,
	NULL,
	mxd_gittelsohn_pulser_resynchronize
};

MX_PULSE_GENERATOR_FUNCTION_LIST mxd_gittelsohn_pulser_pulser_function_list = {
	mxd_gittelsohn_pulser_is_busy,
	mxd_gittelsohn_pulser_arm,
	mxd_gittelsohn_pulser_trigger,
	mxd_gittelsohn_pulser_stop,
	NULL,
	mxd_gittelsohn_pulser_get_parameter,
	mxd_gittelsohn_pulser_set_parameter,
	mxd_gittelsohn_pulser_setup
};

/* MX digital output pulser data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_gittelsohn_pulser_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PULSE_GENERATOR_STANDARD_FIELDS,
	MXD_GITTELSOHN_PULSER_STANDARD_FIELDS
};

long mxd_gittelsohn_pulser_num_record_fields
		= sizeof( mxd_gittelsohn_pulser_record_field_defaults )
		  / sizeof( mxd_gittelsohn_pulser_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_gittelsohn_pulser_rfield_def_ptr
			= &mxd_gittelsohn_pulser_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_gittelsohn_pulser_get_pointers( MX_PULSE_GENERATOR *pulser,
			MX_GITTELSOHN_PULSER **gittelsohn_pulser,
			const char *calling_fname )
{
	static const char fname[] = "mxd_gittelsohn_pulser_get_pointers()";

	MX_GITTELSOHN_PULSER *gittelsohn_pulser_ptr;

	if ( pulser == (MX_PULSE_GENERATOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PULSE_GENERATOR pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( pulser->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for timer pointer passed by '%s' is NULL.",
			calling_fname );
	}

	gittelsohn_pulser_ptr = (MX_GITTELSOHN_PULSER *)
					pulser->record->record_type_struct;

	if ( gittelsohn_pulser_ptr == (MX_GITTELSOHN_PULSER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_GITTELSOHN_PULSER pointer for pulse generator "
			"record '%s' passed by '%s' is NULL",
				pulser->record->name, calling_fname );
	}

	if ( gittelsohn_pulser != (MX_GITTELSOHN_PULSER **) NULL ) {
		*gittelsohn_pulser = gittelsohn_pulser_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_gittelsohn_pulser_command( MX_GITTELSOHN_PULSER *gittelsohn_pulser,
				char *command,
				char *response,
				size_t max_response_size,
				unsigned long command_flags )
{ 
	static const char fname[] = "mxd_gittelsohn_pulser_command()";

	MX_RECORD *rs232_record;
	unsigned long debug_flag, flags;
	char discard_buffer[200];
	mx_bool_type discard_additional_chars;
	mx_bool_type resync_on_error;
	unsigned long i, max_attempts;
	mx_status_type mx_status, mx_status_2;

	if ( gittelsohn_pulser == (MX_GITTELSOHN_PULSER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The gittelsohn_pulser pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer passed was NULL." );
	}

	if ( command_flags & MXF_GPC_NO_DISCARD ) {
		discard_additional_chars = FALSE;
	} else {
		discard_additional_chars = TRUE;
	}

	rs232_record = gittelsohn_pulser->rs232_record;

	flags = gittelsohn_pulser->gittelsohn_pulser_flags;

	if ( flags & MXF_GITTELSOHN_PULSER_DEBUG ) {
		debug_flag = MXF_232_DEBUG;
	} else {
		debug_flag = 0;
	}

	if ( flags & MXF_GITTELSOHN_PULSER_AUTO_RESYNC_ON_ERROR ) {
		resync_on_error = TRUE;
	} else {
		resync_on_error = FALSE;
	}

	max_attempts = gittelsohn_pulser->max_retries + 1;

	for ( i = 0; i < max_attempts; i++ ) {

		/* Send the command to the Arduino. */

		mx_status = mx_rs232_putline( rs232_record, command,
						NULL, debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* The Arduino immediately sends back a line that just reports
		 * back the command that was sent to the caller.  We just throw
		 * this line away.
		 */

		memset( discard_buffer, 0, sizeof(discard_buffer) );

		mx_status = mx_rs232_getline_with_timeout( rs232_record,
				discard_buffer, sizeof(discard_buffer),
				NULL, debug_flag,
				gittelsohn_pulser->timeout );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
			break;
		case MXE_TIMED_OUT:
			mx_stack_traceback();
			mx_status = mx_error( MXE_TIMED_OUT, fname,
			"Gittelsohn pulser '%s' timed out while waiting for "
			"the echoing of the command '%s'.",
				gittelsohn_pulser->record->name, command );

			if ( resync_on_error ) {
				mx_status_2 = mx_resynchronize_record(
						gittelsohn_pulser->record );

				if ( mx_status_2.code != MXE_SUCCESS ) {
					/* If resynchronize failed, then 
					 * we give up.
					 */
					return mx_status;
				} else {
					/* Otherwise, we retry the command. */
					continue;
				}
			}
			break;
		default:
			return mx_status;
			break;
		}

		/* If requested, read back the _real_ data from the Arduino. */

		if ( response != NULL ) {
			mx_status = mx_rs232_getline_with_timeout( rs232_record,
				response, max_response_size, NULL, debug_flag,
				gittelsohn_pulser->timeout );

			switch( mx_status.code ) {
			case MXE_SUCCESS:
				break;
			case MXE_TIMED_OUT:
				mx_stack_traceback();
				mx_status = mx_error( MXE_TIMED_OUT, fname,
				"Gittelsohn pulser '%s' timed out while "
				"waiting for the response to the command '%s'.",
				gittelsohn_pulser->record->name, command );

				if ( resync_on_error ) {
				    mx_status_2 = mx_resynchronize_record(
						gittelsohn_pulser->record );

				    if ( mx_status_2.code != MXE_SUCCESS ) {
					/* If resynchronize failed, then 
					 * we give up.
					 */
					return mx_status;
				    } else {
					/* Otherwise, we retry the command. */
					continue;
				    }
				}
				break;
			default:
				return mx_status;
				break;
			}
		} else if ( discard_additional_chars  ) {

			/* If there are additional bytes available that we
			 * did not expect, then discard them.
			 */

			mx_status = mx_rs232_discard_unread_input( rs232_record,
								debug_flag );
		}

		/* NOTE: This loop only continues past i == 0 if an error
		 * returned above by MX caused a 'continue' statement to be
		 * executed.  We should only get here if the communication
		 * with the Arduino succeeded!  If so, then we immediately
		 * exit the loop via the 'break' statement right after
		 * this comment.
		 */
		break;

	} /* End of the 'max_attempts' loop. */

	return mx_status;
}

static mx_status_type
mxd_gittelsohn_pulser_set_arduino_parameters( MX_PULSE_GENERATOR *pulser,
				MX_GITTELSOHN_PULSER *gittelsohn_pulser )
{
#if 0
	static const char fname[] =
		"mxd_gittelsohn_pulser_set_arduino_parameters()";
#endif

	char command[200];
	char response[200];
	long on_ms, off_ms;
	long num_pulses;
	mx_status_type mx_status;

	/*----------------------------------------------------------------*/

	/* The Gittelsohn Arduino pulser only does pulses. */

	pulser->function_mode = MXF_PGN_PULSE;

	/* The Gittelsohn Arduino pulser does not implement a pulse delay. */

	pulser->pulse_delay = 0;

	/*----------------------------------------------------------------*/

	/* Set the number of pulses. */

	/* If the version of the Arduino code is 2.6 or before, then
	 * we must tell the Arduino to send one extra pulse, since
	 * older versions of the firmware do not send an extra pulse
	 * to clear out accumulated counts from the RDI area detector.
	 */

	if ( gittelsohn_pulser->firmware_version <= 2.6 ) {
		num_pulses = pulser->num_pulses + 1;
	} else {
		num_pulses = pulser->num_pulses;
	}

	snprintf( command, sizeof(command), "cycles %ld", num_pulses );

	mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
				command, response, sizeof(response), 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*----------------------------------------------------------------*/

	/* Unconditionally set 'on_ms' and 'off_ms' in terms of
	 * the values of 'pulse_width' and 'pulse_period' at the
	 * time of this call.
	 */

	on_ms = mx_round( 1000.0 * pulser->pulse_width );

	off_ms = mx_round( 1000.0 *
			( pulser->pulse_period - pulser->pulse_width ) );

	if ( off_ms < 0 ) {
		pulser->pulse_period = pulser->pulse_width;

		off_ms = 0;
	}

	snprintf( command, sizeof(command), "onms %ld", on_ms );

	mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
				command, response, sizeof(response), 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "offms %ld", off_ms );

	mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
				command, response, sizeof(response), 0 );

	return mx_status;
}
				

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_gittelsohn_pulser_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_gittelsohn_pulser_create_record_structures()";

	MX_PULSE_GENERATOR *pulser;
	MX_GITTELSOHN_PULSER *gittelsohn_pulser;

	/* Allocate memory for the necessary structures. */

	pulser = (MX_PULSE_GENERATOR *) malloc( sizeof(MX_PULSE_GENERATOR) );

	if ( pulser == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PULSE_GENERATOR structure." );
	}

	gittelsohn_pulser = (MX_GITTELSOHN_PULSER *)
				malloc( sizeof(MX_GITTELSOHN_PULSER) );

	if ( gittelsohn_pulser == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GITTELSOHN_PULSER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = pulser;
	record->record_type_struct = gittelsohn_pulser;
	record->class_specific_function_list
			= &mxd_gittelsohn_pulser_pulser_function_list;

	pulser->record = record;
	gittelsohn_pulser->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_gittelsohn_pulser_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_gittelsohn_pulser_open()";

	MX_PULSE_GENERATOR *pulser;
	MX_GITTELSOHN_PULSER *gittelsohn_pulser = NULL;
	char response[255];
	int num_items, i, max_attempts;
	unsigned long debug_flag, flags;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	pulser = (MX_PULSE_GENERATOR *) record->record_class_struct;

	mx_status = mxd_gittelsohn_pulser_get_pointers( pulser,
						&gittelsohn_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	memset( &(gittelsohn_pulser->last_internal_start_time),
			0, sizeof(struct timespec) );

	pulser->trigger_mode = MXF_PGN_INTERNAL_TRIGGER;

	MX_DEBUG(-2,("%s: pulser '%s' trigger_mode = %ld",
		fname, record->name, pulser->trigger_mode));

	/* Make sure that the RS232 port is configured correctly. */

	mx_status = mx_rs232_set_configuration( gittelsohn_pulser->rs232_record,
				57600, 8, 'N', 1, 'N', 0x0d0a, 0x0a,
				gittelsohn_pulser->timeout );

	if (mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

	mx_status = mx_rs232_discard_unwritten_output(
					gittelsohn_pulser->rs232_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input(
					gittelsohn_pulser->rs232_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

        flags = gittelsohn_pulser->gittelsohn_pulser_flags;

	if ( flags & MXF_GITTELSOHN_PULSER_DEBUG ) {
		debug_flag = MXF_232_DEBUG;
	} else {
		debug_flag = 0;
	}

	/* Attempt to synchronize communication with the Arduino controller. */

	max_attempts = 5;

	for ( i = 0; i < max_attempts; i++ ) {

		if ( i > 0 ) {
			mx_warning(
			"Initial connection to pulser '%s' failed: retry %d",
				record->name, i );
		}

		/* Print out the Gittelsohn pulser configuration. */

		mx_status = mx_rs232_putline( gittelsohn_pulser->rs232_record,
						"conf", NULL, debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* The first time we send a command to the Arduino, we must
		 * put in a 1 second delay before trying to read the response.
		 * Otherwise, we time out.  Subsequent commands do not seem
		 * to have this problem.
		 */

		mx_msleep(1000);

		/* The first response line should say 'command : conf'. */

		mx_status = mx_rs232_getline_with_timeout(
						gittelsohn_pulser->rs232_record,
						response, sizeof(response),
						NULL, debug_flag,
						gittelsohn_pulser->timeout );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
			break;
		case MXE_TIMED_OUT:
			mx_stack_traceback();
			return mx_error( MXE_TIMED_OUT, fname,
			"Gittelsohn pulser '%s' timed out while waiting for "
			"the first line of the response to the command 'conf'.",
				gittelsohn_pulser->record->name );
			break;
		default:
			return mx_status;
			break;
		}

#if MXD_GITTELSOHN_PULSER_DEBUG_CONF
		MX_DEBUG(-2,("%s: pulser '%s', response line #1 = '%s'",
			fname,  record->name, response));
#endif

		if ( strcmp( response, "command : conf" ) == 0 ) {
			/* We have found the response line that we
			 * expect to see, so break out of the loop.
			 */

			break;
		}

		/* Tell the user that the initial connection to the
		 * Arduino appears to have failed somehow.
		 */

		mx_warning( "Pulser '%s': "
			"The first line of response to a 'conf' command sent "
			"to Arduino pulser '%s' was not 'command : conf'.  "
			"Instead, it was '%s'.",
				record->name, record->name, response );

		/* If we have reached the maximum number of attempts,
		 * then return an error to the user.
		 */

		if ( i >= (max_attempts - 1) ) {
			return mx_error( MXE_PROTOCOL_ERROR, fname,
			"Unable to successfully communicate with "
			"Arduino pulser '%s' after %d attempts.",
				record->name, max_attempts );
		}

		/* Attempt to retry the connection. */

		mx_status = mx_rs232_discard_unwritten_output(
				gittelsohn_pulser->rs232_record, debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_rs232_discard_unread_input(
				gittelsohn_pulser->rs232_record, debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Go back to the top of the loop to try again. */
	}

	/* The second response line should contain the firmware version.*/

	mx_status = mx_rs232_getline_with_timeout(
				gittelsohn_pulser->rs232_record,
				response, sizeof(response),
				NULL, debug_flag,
				gittelsohn_pulser->timeout );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		break;
	case MXE_TIMED_OUT:
		mx_stack_traceback();
		return mx_error( MXE_TIMED_OUT, fname,
		"Gittelsohn pulser '%s' timed out while waiting for the "
		"second line of the response to the command 'conf'.",
			gittelsohn_pulser->record->name );
		break;
	default:
		return mx_status;
		break;
	}

#if MXD_GITTELSOHN_PULSER_DEBUG_CONF
	MX_DEBUG(-2,("%s: pulser '%s', response line #2 = '%s'",
		fname,  record->name, response));
#endif

	num_items = sscanf( response, "{ app : rd%lg",
				&(gittelsohn_pulser->firmware_version) );

	if ( num_items != 1 ) {
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"The second line of response to a 'conf' command sent to "
		"Arduino pulser '%s' did not begin with '{ app : rd'.  "
		"Instead, it was '%s'.", record->name, response );
	}

	/* Unconditionally stop the pulse generator. */

	mx_status = mx_pulse_generator_stop( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the pulse generator settings to known values. */

	mx_status = mxd_gittelsohn_pulser_set_arduino_parameters( pulser,
							gittelsohn_pulser );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gittelsohn_pulser_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_gittelsohn_pulser_resynchronize()";

	MX_PULSE_GENERATOR *pulser = NULL;
	MX_GITTELSOHN_PULSER *gittelsohn_pulser = NULL;
	FILE *log_file = NULL;
	char timestamp_buffer[100];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	pulser = (MX_PULSE_GENERATOR *) record->record_class_struct;

	mx_status = mxd_gittelsohn_pulser_get_pointers( pulser,
						&gittelsohn_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	log_file = mx_cfn_fopen( MX_CFN_LOGFILE, "pulser_timeout.log", "a" );

	mx_warning("Attempting to restore communication with pulser '%s'.",
						record->name );

	if ( log_file ) {
		fprintf( log_file,"%s: Timeout occurred for '%s'.\n",
		    mx_timestamp( timestamp_buffer, sizeof(timestamp_buffer)),
			record->name );
	}

	mx_status = mx_resynchronize_record( gittelsohn_pulser->rs232_record );

	if ( log_file ) {
		fprintf( log_file,"%s: Resynchronization status = %lu.\n",
		    mx_timestamp( timestamp_buffer, sizeof(timestamp_buffer)),
			mx_status.code );

		fclose( log_file );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gittelsohn_pulser_is_busy( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_gittelsohn_pulser_is_busy()";

	MX_GITTELSOHN_PULSER *gittelsohn_pulser = NULL;
	mx_status_type mx_status;

	mx_status = mxd_gittelsohn_pulser_get_pointers( pulser,
						&gittelsohn_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the number of pulses generated so far to the total number of
	 * pulses expected to be generated.
	 */

	mx_status = mx_pulse_generator_get_num_pulses( pulser->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_get_last_pulse_number( pulser->record,
									NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_GITTELSOHN_PULSER_DEBUG_RUNNING
	MX_DEBUG(-2,("%s: num_pulses = %lu, last_pulse_number = %ld",
		fname, pulser->num_pulses, pulser->last_pulse_number));
#endif

	if ( pulser->last_pulse_number < 0 ) {
		pulser->busy = FALSE;
	} else
	if ( pulser->last_pulse_number < pulser->num_pulses ) {
		pulser->busy = TRUE;
	} else {
		pulser->busy = FALSE;
	}

#if MXD_GITTELSOHN_PULSER_DEBUG_RUNNING
	MX_DEBUG(-2,("%s: pulser '%s', busy = %d",
		fname, pulser->record->name, (int) pulser->busy));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gittelsohn_pulser_arm( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_gittelsohn_pulser_arm()";

	MX_GITTELSOHN_PULSER *gittelsohn_pulser = NULL;
	long on_milliseconds, off_milliseconds;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_gittelsohn_pulser_get_pointers( pulser,
						&gittelsohn_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_GITTELSOHN_PULSER_DEBUG_RUNNING
	MX_DEBUG(-2,("%s: Pulse generator '%s' armed, "
		"pulse_width = %f, pulse_period = %f, num_pulses = %ld",
			fname, pulser->record->name,
			pulser->pulse_width,
			pulser->pulse_period,
			pulser->num_pulses));
#endif
	pulser->busy = FALSE;

	if ( pulser->pulse_width < 0.0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Pulser '%s' is configured for a negative pulse width %g.",
			pulser->record->name, pulser->pulse_width );
	}
	if ( pulser->pulse_period < 0.0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Pulser '%s' is configured for a negative pulse period %g.",
			pulser->record->name, pulser->pulse_period );
	}

	if ( pulser->pulse_period < pulser->pulse_width ) {
		mx_warning( "The pulse period %g for pulser '%s' is less than "
			"the pulse width %g.  The pulse period will be "
			"increased to match the pulse width.",
				pulser->pulse_period,
				pulser->record->name,
				pulser->pulse_width );

		pulser->pulse_period = pulser->pulse_width;
	}

	/* Configure the pulse generator parameters */

	on_milliseconds = mx_round( 1000.0 * pulser->pulse_width );

	off_milliseconds =
	    mx_round( 1000.0 * (pulser->pulse_period - pulser->pulse_width) );

	snprintf( command, sizeof(command), "onms %ld", on_milliseconds );

	mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
						command, NULL, 0, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "offms %ld", off_milliseconds );

	mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
						command, NULL, 0, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "cycles %lu", pulser->num_pulses );

	mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
						command, NULL, 0, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If we are not in external trigger mode, then we are done. */

	mx_status = mx_pulse_generator_get_trigger_mode( pulser->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( (pulser->trigger_mode & MXF_PGN_EXTERNAL_TRIGGER) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* We are in external trigger mode. */

	/* Start the pulse generator. */

	mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
						"start", NULL, 0, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the pulse generator as busy. */

	pulser->busy = TRUE;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gittelsohn_pulser_trigger( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_gittelsohn_pulser_arm()";

	MX_GITTELSOHN_PULSER *gittelsohn_pulser = NULL;
	mx_status_type mx_status;

	mx_status = mxd_gittelsohn_pulser_get_pointers( pulser,
						&gittelsohn_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_GITTELSOHN_PULSER_DEBUG_RUNNING
	MX_DEBUG(-2,("%s: Pulse generator '%s' triggered, "
			"trigger_mode = %#lx",
			fname, pulser->record->name,
			pulser->trigger_mode ));
#endif

	if ( (pulser->trigger_mode & MXF_PGN_INTERNAL_TRIGGER) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* We are in internal trigger mode. */

	/* Start the pulse generator. */

	mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
						"start", NULL, 0, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Save the start time. */

	gittelsohn_pulser->last_internal_start_time = mx_high_resolution_time();

	/* Mark the pulse generator as busy. */

	pulser->busy = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_gittelsohn_pulser_stop( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_gittelsohn_pulser_stop()";

	MX_GITTELSOHN_PULSER *gittelsohn_pulser = NULL;
	mx_status_type mx_status;

	mx_status = mxd_gittelsohn_pulser_get_pointers( pulser,
						&gittelsohn_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_GITTELSOHN_PULSER_DEBUG_RUNNING
	MX_DEBUG(-2,("%s: Stopping pulse generator '%s'.",
		fname, pulser->record->name ));
#endif
	mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
						"stop", NULL, 0, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pulser->busy = FALSE;

	memset( &(gittelsohn_pulser->last_internal_start_time),
			0, sizeof(struct timespec) );

	return MX_SUCCESSFUL_RESULT;
}

/*-----*/

static mx_status_type
mxd_gittelsohn_pulser_internal_get_last_pulse_number(
				MX_PULSE_GENERATOR *pulser,
				MX_GITTELSOHN_PULSER *gittelsohn_pulser )
{
#if 0
	static const char fname[] =
		"mxd_gittelsohn_pulser_internal_get_last_pulse_number()";
#endif

	struct timespec last_internal_start_time;
	struct timespec current_hrt_time;
	struct timespec hrt_time_difference;
	double difference_seconds, pulses_since_start_as_double;
	long pulses_since_start;

	last_internal_start_time = gittelsohn_pulser->last_internal_start_time;

	/* If the pulser has never been started, then we can return now. */

	if ( ( last_internal_start_time.tv_sec == 0 )
	  && ( last_internal_start_time.tv_nsec == 0 ) )
	{
		pulser->last_pulse_number = -1L;

		return MX_SUCCESSFUL_RESULT;
	}

	/* The pulser has been started at least once. */

	current_hrt_time = mx_high_resolution_time();

	hrt_time_difference =
		mx_subtract_high_resolution_times( current_hrt_time,
						last_internal_start_time );

	difference_seconds =
	    mx_convert_high_resolution_time_to_seconds( hrt_time_difference );

	pulses_since_start_as_double = mx_divide_safely( difference_seconds,
							pulser->pulse_period );

	pulses_since_start = mx_round_down( pulses_since_start_as_double );

	if ( pulses_since_start < 0L ) {
		pulser->last_pulse_number = -1L;
	} else
	if ( pulses_since_start > pulser->num_pulses ) {
		pulser->last_pulse_number = pulser->num_pulses - 1L;
	} else {
		pulser->last_pulse_number = pulses_since_start - 1L;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----*/

static mx_status_type
mxd_gittelsohn_pulser_external_get_last_pulse_number(
				MX_PULSE_GENERATOR *pulser,
				MX_GITTELSOHN_PULSER *gittelsohn_pulser )
{
	static const char fname[] =
		"mxd_gittelsohn_pulser_external_get_last_pulse_number()";

	char response[200];
	int i, argc, fn_status, saved_errno;
	char **argv;
	char c;
	unsigned long flags;
	mx_status_type mx_status;

	flags = gittelsohn_pulser->gittelsohn_pulser_flags;

	/* FIXME: At present (2015-05-05), the Gittelsohn pulser
	 * sends only \n at the end of a 'count' response instead
	 * of \r\n, which is what all of the other commands do.
	 * Because of that, we must handle the response to a
	 * 'count' command specially.
	 */

	mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
					"count", NULL, 0, 0x3 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	memset( response, 0, sizeof(response) );

	/* Read until we get a line feed (0xa). */

	for ( i = 0; i < sizeof(response); i++ ) {
		mx_status = mx_rs232_getchar_with_timeout(
			gittelsohn_pulser->rs232_record, &c,
			0x0, 0.25 );

		if ( mx_status.code == MXE_SUCCESS ) {
			if ( c == MX_LF ) {
				response[i] = '\0';

				break;	/* Exit the for() loop. */
			}

			response[i] = c;
		} else
		if ( mx_status.code == MXE_TIMED_OUT ) {
			MX_DEBUG(-2,("%s: TIMED_OUT", fname));
			response[i] = '\0';

			break;	/* Exit the for() loop. */
		} else {
			return mx_status;
		}
	}


	if ( flags & MXF_GITTELSOHN_PULSER_DEBUG ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
			fname, response,
			gittelsohn_pulser->rs232_record->name ));
	}

	fn_status = mx_string_split( response, " ", &argc, &argv );

	if ( fn_status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FUNCTION_FAILED, fname,
		"The attempt to split the response '%s' "
		"to command '%s' for record '%s' "
		"using mx_string_split() failed with an errno value "
		"of %d.  Error message = '%s'",
			response, "count", pulser->record->name,
			saved_errno, strerror(saved_errno) );
	}

	if ( argc < 4 ) {
		mx_free(argv);

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The response '%s' to the 'count' command did not "
		"have at least 4 tokens in it for record '%s'.",
			response, pulser->record->name );
	} else {
		pulser->last_pulse_number = atol( argv[3] );

		mx_free(argv);

#if MXD_GITTELSOHN_PULSER_DEBUG_LAST_PULSE_NUMBER

		MX_DEBUG(-2,("%s: '%s' last_pulse_number = %ld",
			fname, pulser->record->name,
			pulser->last_pulse_number ));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----*/

MX_EXPORT mx_status_type
mxd_gittelsohn_pulser_get_parameter( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_gittelsohn_pulser_get_parameter()";

	MX_GITTELSOHN_PULSER *gittelsohn_pulser = NULL;
	MX_PULSE_GENERATOR_FUNCTION_LIST *pulser_flist = NULL;
	char response[200];
	int argc, fn_status, saved_errno;
	char **argv;
	double on_ms, off_ms;
	long saved_parameter_type;
	unsigned long flags;
	mx_status_type mx_status;

	mx_status = mxd_gittelsohn_pulser_get_pointers( pulser,
						&gittelsohn_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulser->record, NULL,
						&pulser_flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	flags = gittelsohn_pulser->gittelsohn_pulser_flags;

	MXW_UNUSED( flags );

#if MXD_GITTELSOHN_PULSER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulser->record->name,
		mx_get_field_label_string( pulser->record,
					pulser->parameter_type ),
		pulser->parameter_type));
#endif

	switch( pulser->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:
		mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
				"cycles", response, sizeof(response), 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		fn_status = mx_string_split( response, " ", &argc, &argv );

		if ( fn_status < 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FUNCTION_FAILED, fname,
			"The attempt to split the response '%s' "
			"to command '%s' for record '%s' "
			"using mx_string_split() failed with an errno value "
			"of %d.  Error message = '%s'",
				response, "cycles", pulser->record->name,
				saved_errno, strerror(saved_errno) );
		}

		if ( argc < 4 ) {
			mx_free(argv);

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The response '%s' to the 'cycles' command did not "
			"have at least 4 tokens in it for record '%s'.",
				response, pulser->record->name );
		} else {
			pulser->num_pulses = atol( argv[3] );

			if ( gittelsohn_pulser->firmware_version <= 2.6 ) {
				pulser->num_pulses--;
			}

			mx_free(argv);

#if MXD_GITTELSOHN_PULSER_DEBUG
			MX_DEBUG(-2,("%s: '%s' pulser->num_pulses = %ld",
				fname, pulser->record->name,
				pulser->num_pulses));
#endif
			return MX_SUCCESSFUL_RESULT;
		}
		break;

	case MXLV_PGN_PULSE_WIDTH:
		mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
				"onms", response, sizeof(response), 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		fn_status = mx_string_split( response, " ", &argc, &argv );

		if ( fn_status < 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FUNCTION_FAILED, fname,
			"The attempt to split the response '%s' "
			"to command '%s' for record '%s' "
			"using mx_string_split() failed with an errno value "
			"of %d.  Error message = '%s'",
				response, "onms", pulser->record->name,
				saved_errno, strerror(saved_errno) );
		}

		if ( argc < 4 ) {
			mx_free(argv);

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The response '%s' to the 'onms' command did not "
			"have at least 4 tokens in it for record '%s'.",
				response, pulser->record->name );
		} else {
			on_ms = atof( argv[3] );

			mx_free(argv);

			pulser->pulse_width = 0.001 * on_ms;

			return MX_SUCCESSFUL_RESULT;
		}
		break;

	case MXLV_PGN_PULSE_PERIOD:
		/*--- First, get the on time by getting the pulse width. ---*/

		saved_parameter_type = pulser->parameter_type;

		pulser->parameter_type = MXLV_PGN_PULSE_WIDTH;

		mx_status = pulser_flist->get_parameter( pulser );

		pulser->parameter_type = saved_parameter_type;

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/*--- Next, get the off time. ---*/

		mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
				"offms", response, sizeof(response), 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		fn_status = mx_string_split( response, " ", &argc, &argv );

		if ( fn_status < 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FUNCTION_FAILED, fname,
			"The attempt to split the response '%s' "
			"to command '%s' for record '%s' "
			"using mx_string_split() failed with an errno value "
			"of %d.  Error message = '%s'",
				response, "offms", pulser->record->name,
				saved_errno, strerror(saved_errno) );
		}

		if ( argc < 4 ) {
			mx_free(argv);

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The response '%s' to the 'offms' command did not "
			"have at least 4 tokens in it for record '%s'.",
				response, pulser->record->name );
		} else {
			off_ms = atof( argv[3] );

			mx_free(argv);

			/* The pulse period is the sum of 'on_ms' and 'off_ms'
			 * expressed in seconds.
			 */

			pulser->pulse_period = pulser->pulse_width
						+ 0.001 * off_ms;
		}
		break;

	case MXLV_PGN_PULSE_DELAY:
		/* The Gittelsohn Arduino pulser does not implement
		 * a pulse delay.
		 */

		pulser->pulse_delay = 0;
		break;

	case MXLV_PGN_FUNCTION_MODE:
		/* The Gittelsohn Arduino pulser only does pulses. */

		pulser->function_mode = MXF_PGN_PULSE;
		break;

	case MXLV_PGN_LAST_PULSE_NUMBER:

		if ( pulser->trigger_mode & MXF_PGN_INTERNAL_TRIGGER ) {
			mx_status =
			  mxd_gittelsohn_pulser_internal_get_last_pulse_number(
				pulser, gittelsohn_pulser );
		} else
		if ( pulser->trigger_mode & MXF_PGN_EXTERNAL_TRIGGER ) {
			mx_status =
			  mxd_gittelsohn_pulser_external_get_last_pulse_number(
				pulser, gittelsohn_pulser );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXD_GITTELSOHN_PULSER_DEBUG_LAST_PULSE_NUMBER
		MX_DEBUG(-2,("%s: '%s' last_pulse_number = %ld",
			fname, pulser->record->name,
			pulser->last_pulse_number ));
#endif
		break;

	default:
		return
		    mx_pulse_generator_default_get_parameter_handler( pulser );

		break;
	}

#if MXD_GITTELSOHN_PULSER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gittelsohn_pulser_set_parameter( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_gittelsohn_pulser_set_parameter()";

	MX_GITTELSOHN_PULSER *gittelsohn_pulser = NULL;
	mx_status_type mx_status;

	mx_status = mxd_gittelsohn_pulser_get_pointers( pulser,
						&gittelsohn_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_GITTELSOHN_PULSER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulser->record->name,
		mx_get_field_label_string( pulser->record,
					pulser->parameter_type ),
		pulser->parameter_type));
#endif

	switch( pulser->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:

#if MXD_GITTELSOHN_PULSER_DEBUG
			MX_DEBUG(-2,("%s: '%s' pulser->num_pulses = %ld",
				fname, pulser->record->name,
				pulser->num_pulses));
#endif

		break;

	case MXLV_PGN_PULSE_WIDTH:
	case MXLV_PGN_PULSE_PERIOD:

#if MXD_GITTELSOHN_PULSER_DEBUG
		MX_DEBUG(-2,("%s: '%s' pulser->pulse_width = %g",
			fname, pulser->record->name, pulser->pulse_width ));

		MX_DEBUG(-2,("%s: '%s' pulser->pulse_period = %g",
			fname, pulser->record->name, pulser->pulse_period ));
#endif

		/* Just save the most recently requested values for use
		 * by the 'start' command.
		 */

		break;

	case MXLV_PGN_PULSE_DELAY:
		break;

	case MXLV_PGN_FUNCTION_MODE:
		break;

	default:
		return
		    mx_pulse_generator_default_set_parameter_handler( pulser );

		break;
	}

#if MXD_GITTELSOHN_PULSER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gittelsohn_pulser_setup( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_gittelsohn_pulser_setup()";

	MX_GITTELSOHN_PULSER *gittelsohn_pulser = NULL;
	mx_status_type mx_status;

	mx_status = mxd_gittelsohn_pulser_get_pointers( pulser,
						&gittelsohn_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	pulser->pulse_period  = pulser->setup[MXSUP_PGN_PULSE_PERIOD];
	pulser->pulse_width   = pulser->setup[MXSUP_PGN_PULSE_WIDTH];
	pulser->num_pulses    = mx_round( pulser->setup[MXSUP_PGN_NUM_PULSES] );
	pulser->pulse_delay   = pulser->setup[MXSUP_PGN_PULSE_DELAY];
	pulser->function_mode = 
			mx_round( pulser->setup[MXSUP_PGN_FUNCTION_MODE] );
	pulser->trigger_mode = 
			mx_round( pulser->setup[MXSUP_PGN_TRIGGER_MODE] );
#else
	mx_status = mx_pulse_generator_update_settings_from_setup( pulser );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

#if MXD_GITTELSOHN_PULSER_DEBUG_SETUP
	MX_DEBUG(-2,("%s: pulser '%s', period = %f, width = %f, "
		"num_pulses = %ld, delay = %f, "
		"function mode = %ld, trigger_mode = %ld",
		fname, pulser->record->name,
		pulser->pulse_period,
		pulser->pulse_width,
		pulser->num_pulses,
		pulser->pulse_delay,
		pulser->function_mode,
		pulser->trigger_mode ));
#endif

	mx_status = mxd_gittelsohn_pulser_set_arduino_parameters( pulser,
							gittelsohn_pulser );

	return mx_status;
}

