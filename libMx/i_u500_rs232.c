/*
 * Name:    i_u500_rs232.c
 *
 * Purpose: MX driver for sending program lines to Aerotech Unidex 500
 *          series motor controllers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2008-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define U500_RS232_DEBUG	FALSE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_U500

#if defined(OS_WIN32)
#include <windows.h>
#endif

#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "i_u500.h"
#include "i_u500_rs232.h"

MX_RECORD_FUNCTION_LIST mxi_u500_rs232_record_function_list = {
	NULL,
	mxi_u500_rs232_create_record_structures,
	mxi_u500_rs232_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_u500_rs232_open,
	mxi_u500_rs232_close
};

MX_RS232_FUNCTION_LIST mxi_u500_rs232_rs232_function_list = {
	mxi_u500_rs232_getchar,
	mxi_u500_rs232_putchar,
	NULL,
	NULL,
	NULL,
	mxi_u500_rs232_putline,
	mxi_u500_rs232_num_input_bytes_available
};

MX_RECORD_FIELD_DEFAULTS mxi_u500_rs232_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_U500_RS232_STANDARD_FIELDS
};

long mxi_u500_rs232_num_record_fields
		= sizeof( mxi_u500_rs232_record_field_defaults )
			/ sizeof( mxi_u500_rs232_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_u500_rs232_rfield_def_ptr
			= &mxi_u500_rs232_record_field_defaults[0];

/* ---- */

static mx_status_type
mxi_u500_rs232_get_pointers( MX_RS232 *rs232,
				MX_U500_RS232 **u500_rs232,
				MX_U500 **u500,
				const char *calling_fname )
{
	static const char fname[] = "mxi_u500_rs232_get_pointers()";

	MX_RECORD *u500_rs232_record, *u500_record;
	MX_U500_RS232 *u500_rs232_ptr;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	u500_rs232_record = rs232->record;

	if ( u500_rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RECORD pointer for the MX_RS232 pointer %p "
			"passed by '%s' is NULL.", rs232, calling_fname );
	}

	u500_rs232_ptr = (MX_U500_RS232 *)u500_rs232_record->record_type_struct;

	if ( u500_rs232_ptr == (MX_U500_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_U500_RS232 pointer for record '%s' is NULL.",
			u500_rs232_record->name );
	}

	if ( u500_rs232 != (MX_U500_RS232 **) NULL ) {
		*u500_rs232 = u500_rs232_ptr;
	}

	if ( u500 != (MX_U500 **) NULL ) {
		u500_record = u500_rs232_ptr->u500_record;

		if ( u500_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The u500_record pointer for record '%s' is NULL.",
				u500_rs232_record->name );
		}

		*u500 = (MX_U500 *) u500_record->record_type_struct;

		if ( (*u500) == (MX_U500 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_U500 pointer for record '%s' is NULL.",
				u500_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_u500_rs232_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_u500_rs232_create_record_structures()";

	MX_RS232 *rs232;
	MX_U500_RS232 *u500_rs232;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_RS232 structure for record '%s'.",
			record->name );
	}

	u500_rs232 = (MX_U500_RS232 *) malloc( sizeof(MX_U500_RS232) );

	if ( u500_rs232 == (MX_U500_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for MX_U500_RS232 structure for record '%s'.",
			record->name );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = u500_rs232;
	record->class_specific_function_list =
					&mxi_u500_rs232_rs232_function_list;
	rs232->record = record;
	u500_rs232->record = record;

	u500_rs232->loopback_port_record = NULL;

	u500_rs232->num_putchars_received = 0;
	u500_rs232->num_write_terminators_seen = 0;

	memset( u500_rs232->putchar_buffer,
		0, sizeof(u500_rs232->putchar_buffer) );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_u500_rs232_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	/* Check to see if the RS-232 parameters are valid. */

	mx_status = mx_rs232_check_port_parameters( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the U500 RS232 device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_u500_rs232_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_u500_rs232_open()";

	MX_RS232 *rs232;
	MX_U500_RS232 *u500_rs232 = NULL;
	MX_U500 *u500 = NULL;
	MX_RECORD *loopback_port_record = NULL;
	MX_RS232 *loopback_port_rs232 = NULL;
	char *ptr;
	int num_items, com_port_number;
	char command[80];
	char response[80];
	size_t bytes_read;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = record->record_class_struct;

	mx_status = mxi_u500_rs232_get_pointers( rs232,
						&u500_rs232, &u500, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if U500_RS232_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	/* Check to see if both port names have been specified. */

	if ( ( strlen(u500_rs232->com_port_name) == 0 )
	  || ( strlen(u500_rs232->loopback_port_name) == 0 ) )
	{
		mx_info( "No com port and loopback port names for record '%s' "
		"have been specified, so reading from this record will not "
		"be possible.", record->name );

		return MX_SUCCESSFUL_RESULT;
	}

	/* Try finding the loopback port record first. */

	loopback_port_record = mx_get_record( record,
					u500_rs232->loopback_port_name );

	if ( loopback_port_record == NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"Record '%s' mentioned by U500 RS-232 record '%s' "
		"does not exist.", u500_rs232->loopback_port_name,
			record->name );
	}

	/* Is the loopback port an RS-232 port? */

	if ( loopback_port_record->mx_class != MXI_RS232 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Loopback port record '%s' used by U500 RS-232 record '%s' "
		"is not an RS-232 record.",
			u500_rs232->loopback_port_name,
			record->name );
	}

	u500_rs232->loopback_port_record = loopback_port_record;

#if U500_RS232_DEBUG
	MX_DEBUG(-2,("%s: Found loopback port '%s' for record '%s'.",
		fname, u500_rs232->loopback_port_record->name, record->name ));
#endif
	/* Get the COM port number for the U500. */

	if ( mx_strncasecmp( u500_rs232->com_port_name, "COM", 3 ) != 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The com port name '%s' specified by U500 RS-232 record '%s' "
		"is not a legal Windows com port name.",
			u500_rs232->com_port_name, record->name );
	}

	ptr = u500_rs232->com_port_name + 3;

	num_items = sscanf( ptr, "%d", &com_port_number );

	if ( num_items != 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Did not find a com port number in the name of Windows "
		"com port '%s' used by U500 RS-232 record '%s'.",
			u500_rs232->com_port_name, record->name );
	}

	/* Tell the U500 to try to connect to the COM port on its side. */

	loopback_port_rs232 = (MX_RS232 *)
				loopback_port_record->record_class_struct;

	if ( loopback_port_rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RS232 pointer for record '%s' is NULL.",
			loopback_port_record->name );
	}

	snprintf( command, sizeof(command),
		"COM INIT %d, %ld, %ld, %c, %ld",
		com_port_number,
		loopback_port_rs232->word_size,
		loopback_port_rs232->stop_bits,
		loopback_port_rs232->parity,
		loopback_port_rs232->speed );

	mx_status = mxi_u500_command( u500, u500_rs232->board_number, command );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if U500_RS232_DEBUG
	MX_DEBUG(-2,("%s: Connected to U500 com port '%s'",
		fname, u500_rs232->com_port_name));
#endif

	/* Verify that the loopback connection works by trying to send
	 * a message through it.
	 */

	mx_status = mx_rs232_putline( record,
				"MX HELLO", NULL, U500_RS232_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if U500_RS232_DEBUG
	MX_DEBUG(-2,
	("%s: About to read response to 'MX HELLO' command.", fname));
#endif

	mx_status = mx_rs232_getline( record,
				response, sizeof(response),
				&bytes_read,
				U500_RS232_DEBUG );


	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if U500_RS232_DEBUG
	MX_DEBUG(-2,
	("%s: response to 'MX HELLO' command is '%s', bytes_read = %ld.",
		fname, response, bytes_read));
#endif

#if U500_RS232_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_u500_rs232_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_u500_rs232_close()";

	MX_RS232 *rs232;
	MX_U500_RS232 *u500_rs232 = NULL;
	MX_U500 *u500 = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = record->record_class_struct;

	mx_status = mxi_u500_rs232_get_pointers( rs232,
						&u500_rs232, &u500, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_u500_command( u500,
			u500_rs232->board_number, "COM FREE" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_u500_rs232_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_u500_rs232_getchar()";

	MX_U500_RS232 *u500_rs232 = NULL;
	mx_status_type mx_status;

	mx_status = mxi_u500_rs232_get_pointers( rs232,
						&u500_rs232, NULL, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( u500_rs232->loopback_port_record == NULL ) {
		return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
		"The loopback port record '%s' is not open for record '%s'.",
			u500_rs232->loopback_port_name, rs232->record->name );
	}

	mx_status = mx_rs232_getchar_with_timeout(
				u500_rs232->loopback_port_record,
				c, U500_RS232_DEBUG, 5.0 );

#if U500_RS232_DEBUG
	MX_DEBUG(-2,("%s: received %x '%c' from U500 RS-232 record '%s'.",
		fname, *c, *c, rs232->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_u500_rs232_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_u500_rs232_putchar()";

	MX_U500_RS232 *u500_rs232 = NULL;
	char next_term_char;
	long num_term_seen, num_putchars;
	mx_bool_type call_putline;
	mx_status_type mx_status;

	mx_status = mxi_u500_rs232_get_pointers( rs232,
						&u500_rs232, NULL, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if U500_RS232_DEBUG
	MX_DEBUG(-2,("%s: sending %x '%c' to U500 RS-232 record '%s'.",
		fname, c, c, rs232->record->name ));
#endif
	call_putline = FALSE;

	if ( rs232->num_write_terminator_chars <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The number of write terminator characters for U500 RS-232 "
		"record '%s' has an illegal value of %ld.  "
		"The value should be in the range from 1 to 4.",
			rs232->record->name,
			rs232->num_write_terminator_chars );
	}

	/* Check for write terminator characters. */

	num_term_seen = u500_rs232->num_write_terminators_seen;

	if ( num_term_seen >= rs232->num_write_terminator_chars ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"'num_term_seen' (%ld) for record '%s' is greater than "
		"or equal to 'num_write_terminator_chars (%ld).  "
		"This should never happen.",
			num_term_seen,
			rs232->record->name,
			u500_rs232->num_write_terminators_seen );
	}

	next_term_char = rs232->write_terminator_array[num_term_seen];

	if ( c == next_term_char ) {
		num_term_seen++;

		u500_rs232->num_write_terminators_seen = num_term_seen;

		if ( num_term_seen >= rs232->num_write_terminator_chars ) {
			/* All of the write terminator chars have arrived. */

			call_putline = TRUE;
		}
	} else {
		u500_rs232->num_write_terminators_seen = 0;

		num_putchars = u500_rs232->num_putchars_received;

		if ( num_putchars >= sizeof(u500_rs232->putchar_buffer) ) {
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"'num_putchars' (%ld) for record '%s' is greater than "
			"or equal to 'sizeof(putchar_buffer)' (%ld).  "
			"This should never happen.",
				num_putchars,
				rs232->record->name,
				(long) sizeof(u500_rs232->putchar_buffer) );
		}

		u500_rs232->putchar_buffer[ num_putchars ] = c;

		num_putchars++;

		u500_rs232->num_putchars_received = num_putchars;

		if ( num_putchars >= sizeof(u500_rs232->putchar_buffer) ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The number of characters (%ld) written to record '%s' "
			"has now equalled or exceeded the total size of "
			"the putchar buffer (%ld).",
				num_putchars,
				rs232->record->name,
				(long) sizeof(u500_rs232->putchar_buffer) );
		}
	}

	if ( call_putline ) {
		mx_status = mxi_u500_rs232_putline( rs232,
					u500_rs232->putchar_buffer, NULL );

		u500_rs232->num_putchars_received = 0;
		u500_rs232->num_write_terminators_seen = 0;

		memset( u500_rs232->putchar_buffer,
			0, sizeof(u500_rs232->putchar_buffer) );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_u500_rs232_putline( MX_RS232 *rs232,
			char *buffer,
			size_t *bytes_written )
{
	static const char fname[] = "mxi_u500_rs232_putline()";

	MX_U500_RS232 *u500_rs232 = NULL;
	MX_U500 *u500 = NULL;
	char local_buffer[500];
	size_t prefix_length;
	mx_status_type mx_status;

	mx_status = mxi_u500_rs232_get_pointers( rs232,
						&u500_rs232, &u500, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if U500_RS232_DEBUG
	MX_DEBUG(-2,("%s: sending '%s' to U500 RS-232 record '%s'.",
		fname, buffer, rs232->record->name ));
#endif

	prefix_length = 3;

	if ( mx_strncasecmp( "MX ", buffer, prefix_length ) != 0 ) {
		mx_status = mxi_u500_command( u500,
				u500_rs232->board_number, buffer );
        } else {
		/* The U500 does not have a programming command that starts
		 * with the string 'MX '.  If the calling routine sends us
		 * a buffer that starts with 'MX ', then we interpret this
		 * as a request to replace the supplied command with the
		 * string 'COM SEND "%s"' where %s is the text following
		 * the 'MX ' string in the original command.
		 * 
		 * This is just a convenience that allows the user to write
		 * a U500 script without knowing the name of the U500 pipe.
		 * You do not have to use the 'MX ' command if you do not
		 * want to.
		 */

		snprintf( local_buffer, sizeof(local_buffer),
			"COM SEND \"%s\"", buffer + prefix_length );

#if U500_RS232_DEBUG
		MX_DEBUG(-2,
		("%s: MX command translated to '%s' for record '%s'",
			fname, local_buffer, rs232->record->name ));
#endif

		mx_status = mxi_u500_command( u500,
				u500_rs232->board_number, local_buffer );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if U500_RS232_DEBUG
		MX_DEBUG(-2,
		("%s: Successfully sent MX command '%s' for record '%s'",
			fname, local_buffer, rs232->record->name ));
#endif
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_u500_rs232_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] =
		"mxi_u500_rs232_num_input_bytes_available()";

	MX_U500_RS232 *u500_rs232 = NULL;
	unsigned long total_bytes_available;
	mx_status_type mx_status;

	mx_status = mxi_u500_rs232_get_pointers( rs232,
						&u500_rs232, NULL, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the client named pipe is not open, then we merely state that
	 * zero bytes are available.
	 */

	if ( u500_rs232->loopback_port_record == NULL ) {
		rs232->num_input_bytes_available = 0;
		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_rs232_num_input_bytes_available(
				u500_rs232->loopback_port_record,
				&total_bytes_available );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if U500_RS232_DEBUG
	MX_DEBUG(-2,("%s: total_bytes_available = %ld",
		fname, (long) total_bytes_available));
#endif

	rs232->num_input_bytes_available = total_bytes_available;

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_U500 */

