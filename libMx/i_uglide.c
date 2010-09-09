/*
 * Name:    i_uglide.c
 *
 * Purpose: MX driver for interface to the BCW u-GLIDE micropositioning
 *          stage from Oceaneering Space Systems.
 *
 *          (The u in u-GLIDE above is really a Greek mu.)
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002, 2005-2008, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_ascii.h"
#include "mx_rs232.h"
#include "i_uglide.h"

MX_RECORD_FUNCTION_LIST mxi_uglide_record_function_list = {
	NULL,
	mxi_uglide_create_record_structures,
	mxi_uglide_finish_record_initialization,
	NULL,
	NULL,
	mxi_uglide_open,
	NULL,
	NULL,
	mxi_uglide_resynchronize
};

MX_RECORD_FIELD_DEFAULTS mxi_uglide_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_UGLIDE_STANDARD_FIELDS
};

long mxi_uglide_num_record_fields
		= sizeof( mxi_uglide_record_field_defaults )
			/ sizeof( mxi_uglide_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_uglide_rfield_def_ptr
			= &mxi_uglide_record_field_defaults[0];

#define MXI_UGLIDE_DEBUG	FALSE

static mx_status_type mxi_uglide_handle_response( MX_UGLIDE *uglide,
							char *command,
							char *response,
							int debug_flag );

static mx_status_type
mxi_uglide_get_pointers( MX_RECORD *record,
			MX_UGLIDE **uglide,
			const char *calling_fname )
{
	static const char fname[] = "mxi_compumotor_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( uglide == (MX_UGLIDE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_UGLIDE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*uglide = (MX_UGLIDE *) record->record_type_struct;

	if ( *uglide == (MX_UGLIDE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_UGLIDE pointer for record '%s' is NULL.",
			record->name );
	}

	if ( (*uglide)->rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The rs232_record pointer for 'uglide' record '%s' is NULL.",
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_uglide_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_uglide_create_record_structures()";

	MX_UGLIDE *uglide = NULL;

	uglide = (MX_UGLIDE *) malloc( sizeof(MX_UGLIDE) );

	if ( uglide == (MX_UGLIDE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_UGLIDE structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	record->record_type_struct = uglide;

	uglide->record = record;

	uglide->x_motor_record = NULL;
	uglide->y_motor_record = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_uglide_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxi_uglide_finish_record_initialization()";

	MX_UGLIDE *uglide = NULL;
	mx_status_type mx_status;

	mx_status = mxi_uglide_get_pointers( record, &uglide, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mx_verify_driver_type( uglide->rs232_record,
			MXR_INTERFACE, MXI_RS232, MXT_ANY ) == FALSE )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"rs232_record '%s' referred to by 'uglide' record '%s' "
		"is not actually an RS-232 record.",
			uglide->rs232_record->name, record->name );
	}

	/* Please note that an addendum to the u-GLIDE manual found on
	 * the included floppy disk says that one must use <CR> as the
	 * line terminator rather than <CR>, <LF> as the manual says.
	 */

	mx_status = mx_rs232_verify_configuration( uglide->rs232_record,
			9600, 8, 'N', 1, 'N', 0x0d0a, 0x0d );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_uglide_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_uglide_open()";

	MX_UGLIDE *uglide = NULL;
	char command[80];
	char response[80];
	char operation[40];
	int mode, bypass_home_search, exit_loop;
	unsigned long i, j, k;
	unsigned long i_command_max_retries, num_input_bytes_available;
	unsigned long getline_max_attempts, getline_sleep_ms;
	unsigned long home_search_max_attempts, home_search_sleep_ms;
	char c;
	long length;
	mx_status_type mx_status;

	mx_status = mxi_uglide_get_pointers( record, &uglide, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2, ("%s invoked for record '%s'.", fname, record->name));

	mx_status = mx_rs232_discard_unwritten_output( uglide->rs232_record,
							MXI_UGLIDE_DEBUG );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We must look at each discarded line individually to see if
	 * a reboot has occurred or a move is in progress.
	 */

	uglide->last_response_code = '\0';

	for (;;) {
		mx_status = mx_rs232_num_input_bytes_available(
			uglide->rs232_record, &num_input_bytes_available );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG( 2,("%s: num_input_bytes_available = %lu",
				fname, num_input_bytes_available ));

		if ( num_input_bytes_available == 0 )
			break;			/* Exit the for(;;) loop. */

		mx_status = mxi_uglide_getline( uglide,
						response, sizeof(response),
						MXI_UGLIDE_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Discard any lines that do not begin with a '#' character. */

		if ( response[0] != '#' ) {
			/* Go back to the top of the for(;;) loop. */

			continue;
		}
	}

	/* See if the controller is active by sending an 'h' command
	 * and looking for the 'ok' response.
	 */

	mx_status = mxi_uglide_command( uglide, "h", MXI_UGLIDE_DEBUG );

	if ( mx_status.code == MXE_TIMED_OUT ) {
		/* If we timed out, this may mean that the controller was
		 * recently power cycled and may be waiting for a command
		 * to do a home search.  So let's tell it to do a home
		 * search on X.
		 */

		MX_DEBUG( 2,("%s: uglide->uglide_flags = %lx",
			fname, uglide->uglide_flags));

		if ( uglide->uglide_flags
			& MXF_UGLIDE_BYPASS_HOME_SEARCH_ON_BOOT )
		{
			bypass_home_search = TRUE;
		} else {
			bypass_home_search = FALSE;
		}

		MX_DEBUG( 2,("%s: bypass_home_search = %d",
			fname, bypass_home_search));

		if ( bypass_home_search ) {
			strcpy( operation, "bypass the" );
			strcpy( command, "\r\rs\rs\rs" );
		} else {
			strcpy( operation, "do a" );
			strcpy( command, "\r\ri" );
		}

		mx_info( "u-GLIDE controller '%s' may have been power cycled.  "
			"Attempting to %s home search.  Please wait...",
				uglide->record->name, operation );

		/* Send the command. */

		mx_status = mx_rs232_putline( uglide->rs232_record, command,
						NULL, MXI_UGLIDE_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( bypass_home_search ) {
			/* Give the bypass a little time to execute. */

			mx_msleep(2000);
		} else {
			/*** Do the home search. ***/

			/* Editorial comment:
			 *
			 * Getting the u-GLIDE controller to successfully
			 * start its power-on home search seems to be quite
			 * error prone and flaky.
			 */

			/* Read back the command line echoed
			 * by the controller.
			 */

			i_command_max_retries = 5;

			for ( i = 0; i < i_command_max_retries; i++ ) {

			    MX_DEBUG( 2,("%s: I command loop, i = %lu",
					fname, i));

			    exit_loop = FALSE;

			    getline_max_attempts = 10;
			    getline_sleep_ms = 1000;

			    for ( j = 0; j < getline_max_attempts; j++ ) {

				MX_DEBUG( 2,("%s: Getline loop, j = %lu",
					fname, j));

				mx_status = mxi_uglide_getline( uglide,
					response, sizeof(response),
					MXI_UGLIDE_DEBUG );

				if ( mx_status.code == MXE_TIMED_OUT ) {
					length = -1;
				} else {
					if ( mx_status.code != MXE_SUCCESS )
						return mx_status;

					MX_DEBUG( 2,
					("%s: Getline response = '%s'",
						fname, response));
	
					length = (long) strlen( response );
				}

				MX_DEBUG( 2,("%s: Getline length = %ld",
					fname, length ));

				if ( length > 0 ) {
					for ( k = 0; k < length; k++ ) {

						c = response[k];

						MX_DEBUG( 2,
				("%s: Getline response[%lu] = %#0x (%c)",
					 		fname, k, c, c ));

						switch( c ) {
						case MX_CR:
						case MX_LF:
							break;
						case 'i':
						case 'j':
							exit_loop = TRUE;
							break;
						default:
							return mx_error(
						MXE_UNPARSEABLE_STRING, fname,
		"Unexpected character %#x in response '%s' to '%s' command.",
							c, response, command );
							break;
						}
						if ( exit_loop )
							break;
					}
				}

				if ( length < 0 )
					break;

				if ( exit_loop )
					break;

				mx_msleep( getline_sleep_ms );
			    }
			    if ( exit_loop )
				    break;

			    /* Try sending the home search command again. */

			    mx_info(
	"Attempting to send another home search command to controller '%s'.",
				uglide->record->name );

			    mx_status = mx_rs232_putline(
					    uglide->rs232_record, "\r\ri",
						NULL, MXI_UGLIDE_DEBUG );

			    if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
			}

			if ( i >= i_command_max_retries ) {
				return mx_error( MXE_TIMED_OUT, fname,
			"Received no response from controller '%s' after "
			"sending the home search command %lu times.  "
			"Is it turned on and connected?",
					uglide->record->name,
					i_command_max_retries + 1 );
			}

			/* Wait a while to see if the home search completes. */

			uglide->last_response_code
				= MXF_UGLIDE_MOVE_IN_PROGRESS;

			home_search_max_attempts = 10;
			home_search_sleep_ms = 1000;

			for ( i = 0; i < home_search_max_attempts; i++ ) {
				/* Attempt to read the motor positions. */

				MX_DEBUG( 2,("%s: Home search loop, i = %lu",
					fname, i));

				mx_status = mxi_uglide_command( uglide, "q",
							MXI_UGLIDE_DEBUG );

				MX_DEBUG( 2,
				("%s: Home search: mx_status.code = %s", fname,
					mx_strerror(mx_status.code, NULL, 0)));

				if ( mx_status.code == MXE_SUCCESS ) {
					break;	/* Exit the while loop. */
				}
				if ( mx_status.code != MXE_NOT_READY )
					return mx_status;

				MX_DEBUG( 2,
				("%s: Home search: sleeping for %lu ms",
					fname, home_search_sleep_ms));

				mx_msleep( home_search_sleep_ms );
			}

			if ( i >= home_search_max_attempts ) {
				return mx_error( MXE_TIMED_OUT, fname,
			"The startup home search for controller '%s' has not "
			"completed after waiting for %lu milliseconds",
					uglide->record->name,
					home_search_max_attempts
						* home_search_sleep_ms );
			}

			mx_info(
		"Power-on home search succeeded for u-GLIDE controller '%s'.",
				uglide->record->name);
		}

		/* Discard any remaining input. */

		mx_status = mx_rs232_discard_unread_input( uglide->rs232_record,
							MXI_UGLIDE_DEBUG );
	}

	/* Set the coordinate system mode. */

	if ( uglide->uglide_flags & MXF_UGLIDE_USE_RELATIVE_MODE ) {
		mode = 1;
	} else {
		mode = 0;
	}

	sprintf( command, "t %d", mode );

	mx_status = mxi_uglide_command( uglide, command, MXI_UGLIDE_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_uglide_resynchronize( MX_RECORD *record )
{
	return mxi_uglide_open( record );
}

MX_EXPORT mx_status_type
mxi_uglide_command( MX_UGLIDE *uglide, char *command, int debug_flag )
{
	static const char fname[] = "mxi_uglide_command()";

	char local_command[80];
	char response[80];
	int retry, first_pass;
	unsigned long num_input_bytes_available;
	mx_status_type mx_status;

	if ( uglide == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_UGLIDE pointer passed was NULL." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'command' buffer pointer passed was NULL.  No command sent.");
	}

	first_pass = TRUE;

	for (;;) {

		/* See if the controller has sent any new information. */

		mx_status = mx_rs232_num_input_bytes_available(
			uglide->rs232_record, &num_input_bytes_available );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( num_input_bytes_available == 0 ) {
			if ( first_pass ) {
				/* See if a move is currently in progress. */

				if ( uglide->last_response_code
					== MXF_UGLIDE_MOVE_IN_PROGRESS )
				{
					return mx_error(
					(MXE_NOT_READY | MXE_QUIET), fname,
		"A previous move for controller '%s' is still in progress.",
						uglide->record->name );
				}
			}

			/* No more responses are available to handle,
			 * so exit the for(;;) loop.
			 */

			break;
		}

		first_pass = FALSE;

		/* Read the response line. */

		mx_status = mxi_uglide_getline( uglide,
					response, sizeof(response), 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Pass the response to the response line handler. */

		mx_status = mxi_uglide_handle_response( uglide,
							command,
							response,
							debug_flag );

		if ( ( mx_status.code != MXE_TRY_AGAIN )
		  && ( mx_status.code != MXE_SUCCESS ) ) {
			return mx_status;
		}
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
			fname, command, uglide->record->name));
	}

	/* An addendum to the manual says that you need to pad commands
	 * sent by the host computer with 15 blank spaces at the front
	 * so that the controller can reliably read the commands.
	 */

	sprintf( local_command, "               %s", command );

	mx_status = mx_rs232_putline( uglide->rs232_record,
					local_command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The u-GLIDE always echoes the command string sent.  We must
	 * read this and then discard it.
	 */

	mx_status = mxi_uglide_getline( uglide,
					response, sizeof(response), 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	do {
		retry = FALSE;

		/* Then, read the response from the controller. */

		mx_status = mxi_uglide_getline( uglide,
					response, sizeof(response), 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG(-2,
			("%s: response = '%s'", fname, response));
		}

		mx_status = mxi_uglide_handle_response( uglide, command,
						response, debug_flag );

		if ( mx_status.code == MXE_TRY_AGAIN ) {
			retry = TRUE;
		} else if ( mx_status.code != MXE_SUCCESS ) {
			return mx_status;
		}

	} while ( retry == TRUE );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_uglide_getline( MX_UGLIDE *uglide,
			char *response,
			size_t max_response_length,
			int debug_flag )
{
	static const char fname[] = "mxi_uglide_getline()";

	unsigned long i, max_retries, milliseconds, num_input_bytes_available;
	mx_status_type mx_status;

	max_retries = 10;
	milliseconds = 100;

	for ( i = 0; i < max_retries; i++ ) {

		mx_status = mx_rs232_num_input_bytes_available(
			uglide->rs232_record, &num_input_bytes_available );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( num_input_bytes_available > 0 )
			break;		/* Exit the for() loop. */

		MX_DEBUG( 2,("%s: i = %lu", fname, i));

		mx_msleep( milliseconds );
	}

	if ( i >= max_retries ) {
		return mx_error( MXE_TIMED_OUT, fname,
	"No response was received from u-GLIDE controller '%s' "
	"on RS-232 port '%s' after waiting for %lu milliseconds.",
				uglide->record->name,
				uglide->rs232_record->name,
				max_retries * milliseconds );
	}

	mx_status = mx_rs232_getline( uglide->rs232_record,
					response, max_response_length,
					NULL, debug_flag );
	return mx_status;
}

static mx_status_type
mxi_uglide_handle_response( MX_UGLIDE *uglide,
				char *command,
				char *response,
				int debug_flag )
{
	static const char fname[] = "mxi_uglide_handle_response()";

	int num_items, mode, x_position, y_position;

	MX_DEBUG( 2,("%s invoked: command = '%s', response = '%s'",
		fname, command, response));

	if ( strcmp( response, "Invalid command" ) == 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"'uglide' controller '%s' said that the command '%s' was an invalid command.",
				uglide->record->name, command );
	}

	/* Ignore other lines that do not start with '#' characters. */

	if ( response[0] != '#' ) {
		return mx_error( (MXE_TRY_AGAIN | MXE_QUIET), fname,
		"Discarding the non-response line '%s' from controller '%s'.  "
		"There may be more text to read.",
			uglide->record->name, response );
	}

	uglide->last_response_code = response[1];

	MX_DEBUG( 2,("%s: uglide->last_response_code = '%c'",
			fname, uglide->last_response_code));

	switch( uglide->last_response_code ) {
	case MXF_UGLIDE_MOVE_COMPLETE:
		return mx_error( (MXE_TRY_AGAIN | MXE_QUIET), fname,
			"A move just completed.  "
			"There should be more response lines available." );

	case MXF_UGLIDE_WARNING_MESSAGE:
		mx_warning(
		"Received warning message '%s' from controller '%s'.",
				response, uglide->record->name );

		return mx_error( (MXE_TRY_AGAIN | MXE_QUIET), fname,
			"Most recent message was a warning.  Try again." );

	case MXF_UGLIDE_OK:
		MX_DEBUG( 2,("%s: Received '%s' response.", fname, response));
		break;

	case MXF_UGLIDE_MOVE_IN_PROGRESS:
		/* We have to wait until the move completes before we can
		 * communicate any further with the controller.
		 */
		break;

	case MXF_UGLIDE_MODE_REPORT:
		num_items = sscanf( response, "#t %d", &mode );

		if ( num_items != 1 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"No coordinate system mode was seen in the response '%s' to command '%s'.",
				response, command );
		}

		if ( ( mode != 0 ) && ( mode != 1 ) ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Illegal coordinate system mode %d seen in response '%s' to command '%s'.",
				mode, response, command );
		}

		if ( mode ) {
			uglide->uglide_flags |= MXF_UGLIDE_USE_RELATIVE_MODE;
		} else {
			uglide->uglide_flags
				&= ( ~ MXF_UGLIDE_USE_RELATIVE_MODE );
		}
		break;

	case MXF_UGLIDE_POSITION_REPORT:
		num_items = sscanf( response, "#x %d y %d",
					&x_position, &y_position );

		if ( num_items != 2 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"No position values were seen in the response '%s' to command '%s'.",
				response, command );
		}
		uglide->x_position = x_position;
		uglide->y_position = y_position;
		break;

	default:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Unrecognize response type seen in response '%s' to command '%s'.",
			response, command );
	}
	return MX_SUCCESSFUL_RESULT;
}

