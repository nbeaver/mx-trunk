/*
 * Name:    i_file_rs232.c
 *
 * Purpose: MX driver for simulating RS-232 ports with either 1 or 2 files.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_FILE_RS232_DEBUG		TRUE

#define MXI_FILE_RS232_DEBUG_GETCHAR	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "i_file_rs232.h"

MX_RECORD_FUNCTION_LIST mxi_file_rs232_record_function_list = {
	NULL,
	mxi_file_rs232_create_record_structures,
	mxi_file_rs232_finish_record_initialization,
	NULL,
	NULL,
	mxi_file_rs232_open,
	mxi_file_rs232_close
};

MX_RS232_FUNCTION_LIST mxi_file_rs232_rs232_function_list = {
	mxi_file_rs232_getchar,
	mxi_file_rs232_putchar,
	mxi_file_rs232_read,
	mxi_file_rs232_write,
	mxi_file_rs232_getline,
	mxi_file_rs232_putline,
	mxi_file_rs232_num_input_bytes_available,
	mxi_file_rs232_discard_unread_input,
	NULL,
	NULL,
	NULL,
	mxi_file_rs232_get_configuration,
	mxi_file_rs232_set_configuration,
	mxi_file_rs232_send_break
};

MX_RECORD_FIELD_DEFAULTS mxi_file_rs232_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_FILE_RS232_STANDARD_FIELDS
};

long mxi_file_rs232_num_record_fields
		= sizeof( mxi_file_rs232_record_field_defaults )
			/ sizeof( mxi_file_rs232_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_file_rs232_rfield_def_ptr
			= &mxi_file_rs232_record_field_defaults[0];

/* ---- */

static mx_status_type
mxi_file_rs232_get_pointers( MX_RS232 *rs232,
			MX_FILE_RS232 **file_rs232,
			const char *calling_fname )
{
	static const char fname[] = "mxi_file_rs232_get_pointers()";

	MX_RECORD *file_rs232_record;
	MX_FILE_RS232 *file_rs232_ptr;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	file_rs232_record = rs232->record;

	if ( file_rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The file_rs232_record pointer for the "
			"MX_RS232 pointer passed by '%s' is NULL.",
			calling_fname );
	}

	file_rs232_ptr = (MX_FILE_RS232 *)
				file_rs232_record->record_type_struct;

	if ( file_rs232_ptr == (MX_FILE_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_FILE_RS232 pointer for record '%s' is NULL.",
			rs232->record->name );
	}

	if ( file_rs232 != (MX_FILE_RS232 **) NULL ) {
		*file_rs232 = file_rs232_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mxi_file_rs232_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_file_rs232_create_record_structures()";

	MX_RS232 *rs232;
	MX_FILE_RS232 *file_rs232;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	file_rs232 = (MX_FILE_RS232 *) malloc( sizeof(MX_FILE_RS232) );

	if ( file_rs232 == (MX_FILE_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_FILE_RS232 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = file_rs232;
	record->class_specific_function_list =
			&mxi_file_rs232_rs232_function_list;

	rs232->record = record;
	file_rs232->record = record;

	file_rs232->input_file = NULL;
	file_rs232->output_file = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_file_rs232_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_file_rs232_finish_record_initialization()";

	MX_RS232 *rs232;
	MX_FILE_RS232 *file_rs232;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_file_rs232_get_pointers( rs232, &file_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check to see if the RS-232 parameters are valid. */

	mx_status = mx_rs232_check_port_parameters( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the File RS-232 device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_file_rs232_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_file_rs232_open()";

	MX_RS232 *rs232;
	MX_FILE_RS232 *file_rs232;
	int saved_errno;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_file_rs232_get_pointers( rs232, &file_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_FILE_RS232_DEBUG
	MX_DEBUG(-2,
	  ("%s invoked by record '%s' for input file '%s' and output file '%s'",
		fname, record->name,
		file_rs232->input_filename,
		file_rs232->output_filename));
#endif
	/* If requested, open the input file for binary read. */

	if ( strlen( file_rs232->input_filename ) > 0 ) {
		file_rs232->input_file =
			fopen( file_rs232->input_filename, "rb" );

		if ( file_rs232->input_file == NULL ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"The attempt by record '%s' to open input file '%s' "
			"failed.  Errno = %d, error message = '%s'",
				record->name, file_rs232->input_filename,
				saved_errno, strerror( saved_errno ) );
		}
	}

	/* If requested, open the output file for binary append.
	 * Append is used to ensure that the existing contents of
	 * the file are not overwritten.
	 */

	if ( strlen( file_rs232->output_filename ) > 0 ) {
		file_rs232->output_file =
			fopen( file_rs232->output_filename, "ab" );

		if ( file_rs232->output_file == NULL ) {
			saved_errno = errno;

			(void) fclose( file_rs232->input_file );

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"The attempt by record '%s' to open output file '%s' "
			"failed.  Errno = %d, error message = '%s'",
				record->name, file_rs232->output_filename,
				saved_errno, strerror( saved_errno ) );
		}
	}

	/* Set the standard RS-232 configuration commands. */

	mx_status = mxi_file_rs232_set_configuration( rs232 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_file_rs232_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_file_rs232_close()";

	MX_RS232 *rs232;
	MX_FILE_RS232 *file_rs232;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_file_rs232_get_pointers( rs232, &file_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( file_rs232->input_file != NULL ) {
		(void) fclose( file_rs232->input_file );
	}

	if ( file_rs232->output_file != NULL ) {
		(void) fclose( file_rs232->output_file );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_file_rs232_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_file_rs232_getchar()";

	MX_FILE_RS232 *file_rs232;
	mx_status_type mx_status;

#if MXI_FILE_RS232_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mxi_file_rs232_get_pointers( rs232, &file_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( c == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The character pointer passed was NULL." );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_file_rs232_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_file_rs232_putchar()";

	MX_FILE_RS232 *file_rs232;
	mx_status_type mx_status;

#if MXI_FILE_RS232_DEBUG
	MX_DEBUG(-2, ("%s invoked.  c = 0x%x, '%c'", fname, c, c));
#endif

	mx_status = mxi_file_rs232_get_pointers( rs232, &file_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_file_rs232_read( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_read,
			size_t *bytes_read )
{
	static const char fname[] = "mxi_file_rs232_read()";

	MX_FILE_RS232 *file_rs232;
	mx_status_type mx_status;

	mx_status = mxi_file_rs232_get_pointers( rs232, &file_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_read != (size_t *) NULL ) {
		*bytes_read = max_bytes_to_read;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_file_rs232_write( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_write,
			size_t *bytes_written )
{
	static const char fname[] = "mxi_file_rs232_write()";

	MX_FILE_RS232 *file_rs232;
	mx_status_type mx_status;

	mx_status = mxi_file_rs232_get_pointers( rs232, &file_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_written != NULL ) {
		*bytes_written = max_bytes_to_write;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_file_rs232_getline( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_read,
			size_t *bytes_read )
{
	static const char fname[] = "mxi_file_rs232_getline()";

	MX_FILE_RS232 *file_rs232;
	char *buffer_ptr, *terminators_ptr;
	size_t bytes_left_in_buffer, bytes_read_so_far, bytes_read_by_this_call;
	mx_bool_type do_timeout;
	int tick_comparison;
	MX_CLOCK_TICK timeout_clock_ticks, current_tick, finish_tick;
	long mx_error_code;
	mx_status_type mx_status;

	mx_status = mxi_file_rs232_get_pointers( rs232, &file_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( rs232->timeout < 0.0 ) {
		do_timeout = FALSE;
	} else {
		do_timeout = TRUE;

		timeout_clock_ticks =
		    mx_convert_seconds_to_clock_ticks( rs232->timeout );

		current_tick = mx_current_clock_tick();

		finish_tick = mx_add_clock_ticks( current_tick,
						timeout_clock_ticks );
	}

	buffer_ptr = buffer;
	bytes_left_in_buffer = max_bytes_to_read;
	bytes_read_so_far = 0;

	bytes_read_by_this_call = 0;

	while (1) {
		if ( do_timeout ) {
			current_tick = mx_current_clock_tick();

			tick_comparison = mx_compare_clock_ticks(
					current_tick, finish_tick );

			if ( tick_comparison > 0 ) {
				if ( rs232->rs232_flags &
				    MXF_232_SUPPRESS_TIMEOUT_ERROR_MESSAGES )
				{
					mx_error_code =
						(MXE_TIMED_OUT | MXE_QUIET);
				} else {
					mx_error_code = MXE_TIMED_OUT;
				}

				return mx_error( mx_error_code, fname,
				    "Read from RS-232 port '%s' timed out "
				    "after %g seconds.",
					rs232->record->name, rs232->timeout );
			}
		}

		mx_status = mxi_file_rs232_read( rs232,
						buffer_ptr,
						bytes_left_in_buffer,
						&bytes_read_by_this_call );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		bytes_read_so_far += bytes_read_by_this_call;

		/* See if the read terminators are found in the data
		 * returned by GETN.
		 *
		 * 2012-06-12 (WML):
		 *   It is possible for line terminators to be split
		 *   between two packets returned by getn(), so we
		 *   must not assume that all of the line terminators
		 *   are found in the same packet.  The safest way to
		 *   deal with this is to start scanning for line
		 *   terminators at the start of the buffer every
		 *   time, instead of starting at buffer_ptr.
		 */

		terminators_ptr = mx_rs232_find_terminators( buffer,
					max_bytes_to_read,
					rs232->num_read_terminator_chars,
					rs232->read_terminator_array );

		if ( terminators_ptr != NULL ) {
			*terminators_ptr = '\0';

			if ( bytes_read != NULL ) {
				*bytes_read = bytes_read_so_far
					- rs232->num_read_terminator_chars;
			}

#if MXI_FILE_RS232_DEBUG
			MX_DEBUG(-2,("%s: received '%s' from '%s'.",
				fname, buffer, rs232->record->name));
#endif
			return MX_SUCCESSFUL_RESULT;
		}

		if ( bytes_read_by_this_call >= bytes_left_in_buffer ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The total response beginning with '%s' from '%s'"
			"would exceed the specified buffer size of %lu.",
				buffer, rs232->record->name,
				(unsigned long) max_bytes_to_read );
		}

		buffer_ptr += bytes_read_by_this_call;
		bytes_left_in_buffer -= bytes_read_by_this_call;

		mx_msleep(1);
	}

	MXW_NOT_REACHED( return MX_SUCCESSFUL_RESULT );
}

MX_EXPORT mx_status_type
mxi_file_rs232_putline( MX_RS232 *rs232,
			char *buffer,
			size_t *bytes_written )
{
	static const char fname[] = "mxi_file_rs232_putline()";

	MX_FILE_RS232 *file_rs232;
	size_t bytes_to_write;
	mx_status_type mx_status;

	mx_status = mxi_file_rs232_get_pointers( rs232, &file_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bytes_to_write = strlen( buffer ) + 1;

	if ( bytes_written != NULL ) {
		*bytes_written = bytes_to_write;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_file_rs232_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] =
			"mxi_file_rs232_num_input_bytes_available()";

	MX_FILE_RS232 *file_rs232;
	mx_status_type mx_status;

	mx_status = mxi_file_rs232_get_pointers( rs232, &file_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232->num_input_bytes_available = 0;

#if MXI_FILE_RS232_DEBUG
	MX_DEBUG(-2,("%s: num_input_bytes_available = %ld",
			fname, rs232->num_input_bytes_available));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_file_rs232_discard_unread_input( MX_RS232 *rs232 )
{
	char buffer[2500];
	long total_bytes_to_read, bytes_to_read;
	size_t bytes_read;
	mx_status_type mx_status;

	mx_status = mxi_file_rs232_num_input_bytes_available( rs232 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	total_bytes_to_read = rs232->num_input_bytes_available;

	while ( total_bytes_to_read > 0 ) {
		if ( total_bytes_to_read >= sizeof(buffer) ) {
			bytes_to_read = sizeof(buffer);
		} else {
			bytes_to_read = total_bytes_to_read;
		}

		mx_status = mxi_file_rs232_read( rs232, buffer,
						bytes_to_read,
						&bytes_read );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		total_bytes_to_read -= bytes_read;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_file_rs232_get_configuration( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_file_rs232_get_configuration()";

	MX_FILE_RS232 *file_rs232;
	mx_status_type mx_status;

	mx_status = mxi_file_rs232_get_pointers( rs232, &file_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_file_rs232_set_configuration( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_file_rs232_set_configuration()";

	MX_FILE_RS232 *file_rs232;
	mx_status_type mx_status;

	mx_status = mxi_file_rs232_get_pointers( rs232, &file_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_file_rs232_send_break( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_file_rs232_set_configuration()";

	MX_FILE_RS232 *file_rs232;
	mx_status_type mx_status;

	mx_status = mxi_file_rs232_get_pointers( rs232, &file_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait a while for the break to complete. */

	mx_msleep(200);

	return MX_SUCCESSFUL_RESULT;
}

