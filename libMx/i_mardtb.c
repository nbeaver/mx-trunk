/*
 * Name:    i_mardtb.c
 *
 * Purpose: MX interface driver for the MarUSA Desktop Beamline
 *          goniostat controller.  This driver uses the 'tcp232'
 *          RS-232 driver to connect to the controller via ethernet.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_MARDTB_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_ascii.h"
#include "mx_rs232.h"

#include "i_mardtb.h"

MX_RECORD_FUNCTION_LIST mxi_mardtb_record_function_list = {
	NULL,
	mxi_mardtb_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_mardtb_open,
};

MX_RECORD_FIELD_DEFAULTS mxi_mardtb_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_MARDTB_STANDARD_FIELDS
};

long mxi_mardtb_num_record_fields
		= sizeof( mxi_mardtb_record_field_defaults )
			/ sizeof( mxi_mardtb_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_mardtb_rfield_def_ptr
			= &mxi_mardtb_record_field_defaults[0];

/* --- */

static mx_status_type
mxi_mardtb_get_pointers( MX_RECORD *mardtb_record,
			MX_MARDTB **mardtb,
			const char *calling_fname )
{
	static const char fname[] = "mxi_mardtb_get_pointers()";

	if ( mardtb_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mardtb == (MX_MARDTB **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MARDTB pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mardtb = (MX_MARDTB *) mardtb_record->record_type_struct;

	if ( *mardtb == (MX_MARDTB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MARDTB pointer for record '%s' is NULL.",
			mardtb_record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxi_mardtb_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_mardtb_create_record_structures()";

	MX_MARDTB *mardtb;

	mardtb = (MX_MARDTB *) malloc( sizeof(MX_MARDTB) );

	if ( mardtb == (MX_MARDTB *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Can't allocate memory for MX_MARDTB structure." );
	}

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	record->record_type_struct = mardtb;

	mardtb->record = record;

	mardtb->currently_active_record = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_mardtb_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_mardtb_open()";

	MX_MARDTB *mardtb;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warning. */

	mardtb = NULL;

	mx_status = mxi_mardtb_get_pointers( record, &mardtb, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.", fname, record->name));

	/* Discard the signon banner from the remote host. */

	mx_status = mx_rs232_discard_unread_input( mardtb->rs232_record,
							MXI_MARDTB_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We are presumably at the login prompt here, so send the username
	 * to the remote host.
	 */

	mx_status = mx_rs232_putline( mardtb->rs232_record, mardtb->username,
						NULL, MXI_MARDTB_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard login messages. */

	mx_status = mx_rs232_discard_unread_input( mardtb->rs232_record,
							MXI_MARDTB_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the MarDTB clock value. */

	mx_status = mxi_mardtb_raw_read_status_parameter( mardtb,
					MXF_MARDTB_CLOCK_PARAMETER,
					&(mardtb->last_clock_value) );
	return mx_status;
}

/* === Extra functions for the use of this driver. === */

MX_EXPORT mx_status_type
mxi_mardtb_command( MX_MARDTB *mardtb,
			char *command,
			char *response,
			size_t response_buffer_length,
			int debug_flag )
{
	const char fname[] = "mxi_mardtb_command()";

	MX_RECORD *currently_active_record;
	char error_message[80];
	char c;
	int at_start_of_line, carriage_return_seen, non_blank_line_seen;
	int i, line, num_chars, exit_loop;
	mx_status_type mx_status;

	if ( mardtb == (MX_MARDTB *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL MX_MARDTB pointer passed." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL command buffer pointer passed." );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2, ("%s: sending '%s'", fname, command));
	}

	/* See if some Mar DTB record is currently using the connection.
	 * Normally, this will mean a move is in progress.
	 */

	currently_active_record = mardtb->currently_active_record;

	if ( currently_active_record != NULL ) {
		return mx_error( MXE_TRY_AGAIN, fname,
		"The command '%s' could not be sent to Mar DTB record '%s', "
		"since it is currently in use by record '%s'.",
			command, mardtb->record->name,
			currently_active_record->name );
	}

	/* Discard any existing characters in the input buffer. */

	mx_status = mx_rs232_discard_unread_input( mardtb->rs232_record,
							debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the command string. */

	mx_status = mx_rs232_putline( mardtb->rs232_record, command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The Mar Desktop Beamline controller always (?) returns at least
	 * two lines of text.  The first line contains an echoed copy of
	 * the command line we just sent, while the second line is a blank
	 * spacer line.  These two lines are unconditionally thrown away
	 * here.  The line delimiter is assumed to be a <CR> character.
	 *
	 * If we have discarded MX_MARDTB_NUM_CHARS_LIMIT characters
	 * without seeing a <CR>, we assume that something is wrong
	 * and return with an error message.
	 */

	for ( line = 1; line <= 2; line++ ) {
		carriage_return_seen = FALSE;
		num_chars = 0;

		while ( carriage_return_seen == FALSE ) {

			mx_status = mx_rs232_getchar( mardtb->rs232_record,
							&c, MXF_232_WAIT );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( c == MX_CR ) {
				carriage_return_seen = TRUE;
			}

			if ( ++num_chars > MX_MARDTB_NUM_CHARS_LIMIT ) {
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The record '%s' has received over %d characters without "
		"seeing a carriage return character.  Something is wrong "
		"with the connection to the Mar Desktop Beamline controller.",
					mardtb->rs232_record->name,
					MX_MARDTB_NUM_CHARS_LIMIT );
			}
		}
	}

	/* Next there should be a line feed <LF> character in the buffer. */

	mx_status = mx_rs232_getchar( mardtb->rs232_record,
					&c, MXF_232_WAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( c != MX_LF ) {
		mx_warning(
"%s: A '%c' (%#x) character was seen when a line feed was expected.  "
"The character will be ignored in the hope that things will work anyway.",
			fname, c, c );
	}

	/* Next should be a carriage return <CR> or line feed <LF> character.
         * If not, this probably means that an error has occurred.
	 */

	mx_status = mx_rs232_getchar( mardtb->rs232_record,
					&c, MXF_232_WAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ( c != MX_CR ) && ( c != MX_LF ) ) {
		/* Copy the next line's worth of error message to the 
		 * error message buffer.
		 */

		error_message[0] = c;

		for ( i = 1; i < (sizeof(error_message) - 1); i++ ) {
			mx_status = mx_rs232_getchar( mardtb->rs232_record,
							&c, MXF_232_WAIT );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( ( c == MX_CR ) || ( c == MX_LF ) ) {
				error_message[i] = '\0';
				break;		/* Exit the for() loop. */
			}

			error_message[i] = c;
		}

		if ( i >= (sizeof(error_message) - 1) ) {
			error_message[ sizeof(error_message) - 1 ] = '\0';
		} else {
			error_message[i] = '\0';
		}

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The command '%s' sent to MarDTB controller '%s' failed.  "
		"Error message = '%s'.",
			command, mardtb->record->name, error_message );
	}

	/* Return now if no further response is expected. */

	if ( response == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Read the expected response. */

	/* Discard characters until we find a non-blank
	 * response line.  Null characters and linefeed
	 * characters are discarded as well.
	 */

	at_start_of_line = TRUE;
	non_blank_line_seen = FALSE;
	num_chars = 0;

	while( non_blank_line_seen == FALSE ) {

		mx_status = mx_rs232_getchar( mardtb->rs232_record,
							&c, MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( c ) {
		case MX_NUL:
		case MX_LF:
		case MX_CR:
			/* Ignore null, <CR> and <LF> characters. */

			MX_DEBUG( 2,("%s: ignoring (%0#x) character.",
				fname, c));

			break;
		default:
			/* This must be the first character
			 * of the response we are looking for,
			 * so store the character at the
			 * start of the response buffer and
			 * break out of the while() loop.
			 */

			response[0] = c;

			MX_DEBUG( 2,("%s: Start of the response line seen.",
				fname));

			MX_DEBUG( 2,("%s: response[0] = '%c' (%0#x)",
					fname, c, c));

			non_blank_line_seen = TRUE;

			break;
		}

		if ( ++num_chars > MX_MARDTB_NUM_CHARS_LIMIT ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The record '%s' has received over %d characters without "
		"seeing a response line.  Something is wrong with the "
		"connection to the Mar Desktop Beamline controller.",
				mardtb->rs232_record->name,
				MX_MARDTB_NUM_CHARS_LIMIT );
		}
	}

	/* Read the rest of the response characters on this line. */

	num_chars = 0;
	exit_loop = FALSE;

	for ( i = 1; i < response_buffer_length; ) {

		mx_status = mx_rs232_getchar( mardtb->rs232_record,
							&c, MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ++num_chars > MX_MARDTB_NUM_CHARS_LIMIT ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The record '%s' has received over %d characters without "
		"seeing the end of the response line.  Something is wrong "
		"with the connection to the Mar Desktop Beamline controller.",
				mardtb->rs232_record->name,
				MX_MARDTB_NUM_CHARS_LIMIT );
		}

		switch( c ) {
		case MX_NUL:
			/* We still discard null characters. */

			MX_DEBUG( 2,("%s: ignoring (%0#x) character.",
				fname, c));

			break;
		case MX_CR:
		case MX_LF:
			/* This is the end of the response line.  Make sure
			 * the line is null terminated and then exit the loop.
			 */

			MX_DEBUG( 2,("%s: end of response seen.  char = (%0#x)",
				fname, c));

			response[i] = '\0';

			exit_loop = TRUE;
			break;
		default:
			/* Copy this character to the response buffer. */

			response[i] = c;

			MX_DEBUG( 2,("%s: response[%d] = '%c' (%0#x)",
					fname, i, c, c));

			i++;
			break;
		}

		if ( exit_loop ) {
			break;
		}
	}

	if ( ( exit_loop == FALSE ) && ( i >= response_buffer_length ) ) {
		response[response_buffer_length - 1] = '\0';

		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The response to the MarDTB command '%s' exceeded the "
		"length of the response buffer (%ld).  Partial response = '%s'",
			command, (long) response_buffer_length, response );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: response = '%s'", fname, response));
	}

	MX_DEBUG( 2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_mardtb_check_for_move_in_progress( MX_MARDTB *mardtb,
					mx_bool_type *move_in_progress )
{
	static const char fname[] = "mxi_mardtb_check_for_move_in_progress()";

	unsigned long num_input_bytes_available;
	mx_status_type mx_status;

	if ( mardtb == (MX_MARDTB *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MARDTB pointer passed is NULL." );
	}
	if ( move_in_progress == (int *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The move_in_progress pointer passed is NULL." );
	}

	if ( mardtb->currently_active_record == NULL ) {
		MX_DEBUG( 2,("%s: No move was already in progress.", fname));
		MX_DEBUG( 2,("%s: *move_in_progress = %d",
					fname, *move_in_progress));

		*move_in_progress = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* At the end of a move, the Mar Desktop Beamline controller sends out
	 * a new command prompt.  This is the only indication it gives that
	 * the move is complete.  If no new characters are available from the
	 * controller, then the move is still in progress.
	 */

	mx_status = mx_rs232_num_input_bytes_available( mardtb->rs232_record,
						&num_input_bytes_available );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: num_input_bytes_available = %lu",
		fname, num_input_bytes_available));

	if ( num_input_bytes_available > 0 ) {
		MX_DEBUG( 2,("%s: The move by motor '%s' is complete.",
			fname, mardtb->currently_active_record->name));

		*move_in_progress = FALSE;

		mardtb->currently_active_record = NULL;
	} else {
		MX_DEBUG( 2,("%s: The move by motor '%s' is still in progress.",
			fname, mardtb->currently_active_record->name));

		*move_in_progress = TRUE;
	}

	MX_DEBUG( 2,("%s: *move_in_progress = %d", fname, *move_in_progress));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_mardtb_force_status_update( MX_MARDTB *mardtb )
{
	mx_status_type mx_status;

	mx_status = mxi_mardtb_command( mardtb, "scanstat",
					NULL, 0, MXI_MARDTB_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_mardtb_raw_read_status_parameter( MX_MARDTB *mardtb,	
					long parameter_number,
					unsigned long *parameter_value )
{
	static const char fname[] = "mxi_mardtb_raw_read_status_parameter()";

	char command[80];
	char response[80];
	char *hex_value_ptr;
	int move_in_progress, num_items;
	unsigned long use_three_parameter_status_dump;
	mx_status_type mx_status;

	mx_status = mxi_mardtb_check_for_move_in_progress( mardtb,
							&move_in_progress );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( move_in_progress ) {
		return mx_error_quiet( MXE_NOT_READY, fname,
		"MarDTB motor '%s' belonging to MarDTB '%s' "
		"is currently in motion, "
		"so we cannot read the value of parameter %ld",
			mardtb->currently_active_record->name,
			mardtb->record->name,
			parameter_number );
	}

	/* For current (in 2005) versions of the MarDTB software, the
	 * 'status_dump' commands only requires one argument, namely,
	 * the parameter number.  However, for older versions of the
	 * MarDTB software, you had to prefix the parameter number with
	 * the string "1, 1, ".  It is likely that there are few 
	 * remaining installations of the MarDTB that use the old
	 * version of the software.  However, just in case you have
	 * one, the MXF_MARDTB_THREE_PARAMETER_STATUS_DUMP flag is
	 * available to select the older syntax.
	 */

	use_three_parameter_status_dump = mardtb->mardtb_flags &
				MXF_MARDTB_THREE_PARAMETER_STATUS_DUMP;

	if ( use_three_parameter_status_dump ) {
		sprintf( command, "status_dump 1, 1, %ld", parameter_number );
	} else {
		sprintf( command, "status_dump %ld", parameter_number );
	}

	/* Send the command. */

	mx_status = mxi_mardtb_command( mardtb, command,
					response, sizeof(response),
					MXI_MARDTB_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	hex_value_ptr = response + 21;

	num_items = sscanf( hex_value_ptr, "%lx", parameter_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"MarDTB parameter %ld value not found in response '%s' "
		"for MarDTB '%s'.", parameter_number,
			response, mardtb->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_mardtb_read_status_parameter( MX_MARDTB *mardtb,
				long parameter_number,
				unsigned long *parameter_value )
{
	static const char fname[] = "mxi_mardtb_read_status_parameter()";

	unsigned long update_type, clock_value;
	mx_status_type mx_status;

	update_type = mardtb->mardtb_flags & 0x3;

	if ( update_type == 0x3 ) {
		update_type = MXF_MARDTB_FORCE_STATUS_UPDATE;
	}

	if ( update_type != 0 ) {
		mx_warning( "Forcing MarDTB status updates currently "
		"does not work very well and usually ends up crashing "
		"the goniostat controller." );
	}

	MX_DEBUG( 2,("%s: update_type = %#lx", fname, update_type));

	switch ( update_type ) {
	case 0x0:
		/* No update is to be done. */
		break;
	case MXF_MARDTB_FORCE_STATUS_UPDATE:
		mx_status = mxi_mardtb_force_status_update( mardtb );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	case MXF_MARDTB_CONDITIONAL_STATUS_UPDATE:

		/********** WARNING *** WARNING *** WARNING ************
		 * The following code does not actually work.  If you  *
		 * try to use it when MarCCD is running, all that will *
		 * happen is that you will cause the MarDTB operating  *
		 * system to lock up after a very short time.  If      *
		 * anyone has an inspiration as to how to make this    *
		 * actually work, I would be interested in hearing     *
		 * about it.                                           *
		 *******************************************************/

		/* Figure out whether or not the internal MarDTB clock
		 * is being updated in the status parameter table.
		 * Hopefully, we can tell from this whether or not
		 * the MarCCD user interface is running.
		 */

		/* First, read the current value in the parameter table. */

		mx_status = mxi_mardtb_raw_read_status_parameter( mardtb,
						MXF_MARDTB_CLOCK_PARAMETER,
						&clock_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* If this value is the same as the last value read, then
		 * we need to force a status update.
		 */

		MX_DEBUG( 2,("%s: clock_value = %#lx, last_clock_value = %#lx",
			fname, clock_value, mardtb->last_clock_value));

		if ( clock_value == mardtb->last_clock_value ) {
			mx_status = mxi_mardtb_force_status_update( mardtb );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Reread the clock value so that we can store
			 * the current value for next time.
			 */

			mx_status = mxi_mardtb_raw_read_status_parameter(
						mardtb,
						MXF_MARDTB_CLOCK_PARAMETER,
						&clock_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

		}

		mardtb->last_clock_value = clock_value;
		break;
	}

	/* Now call the raw function to get the parameter value we want. */

	mx_status = mxi_mardtb_raw_read_status_parameter( mardtb,
							parameter_number,
							parameter_value );
	return mx_status;
}

