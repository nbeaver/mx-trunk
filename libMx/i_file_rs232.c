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

#define MXI_FILE_RS232_DEBUG		FALSE

#define MXI_FILE_RS232_DEBUG_GETCHAR	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_io.h"
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
	NULL,
	NULL,
	mxi_file_rs232_num_input_bytes_available,
	mxi_file_rs232_discard_unread_input,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_file_rs232_flush
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

	return MX_SUCCESSFUL_RESULT;
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
	int fgetc_value, saved_errno;
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

	if ( file_rs232->input_file == NULL ) {
		/* If the file is not open, we treat it as if it were
		 * an open file that did not have any new bytes available.
		 */

		return mx_error( (MXE_NOT_READY | MXE_QUIET), fname,
		"Input file '%s' is not open for record '%s'.",
			file_rs232->input_filename, rs232->record->name );
	}

	fgetc_value = fgetc( file_rs232->input_file );

	if ( fgetc_value == EOF ) {
		if ( feof( file_rs232->input_file ) ) {
			return mx_error( (MXE_NOT_READY | MXE_QUIET), fname,
			"Failed to read a character from file '%s' "
			"for record '%s'.",
				file_rs232->input_filename,
				rs232->record->name );
		} else
		if ( ferror( file_rs232->input_file ) ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred while trying to read from "
			"file '%s' for record '%s'.  "
			"Errno = %d, error message = '%s'.",
				file_rs232->input_filename,
				rs232->record->name,
				saved_errno, strerror( saved_errno ) );
		} else {
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"fgetc() reported an error for input file '%s' "
			"used by record '%s', but feof() and ferror() "
			"both reported no error.",
				file_rs232->input_filename,
				rs232->record->name );
		}
	}

	*c = (char) fgetc_value;

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_file_rs232_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_file_rs232_putchar()";

	MX_FILE_RS232 *file_rs232;
	int fputc_value, saved_errno;
	mx_status_type mx_status;

#if MXI_FILE_RS232_DEBUG
	MX_DEBUG(-2, ("%s invoked.  c = 0x%x, '%c'", fname, c, c));
#endif

	mx_status = mxi_file_rs232_get_pointers( rs232, &file_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( file_rs232->output_file == NULL ) {
		/* If the file is not open, we treat it as if it were
		 * a file like /dev/null, where bytes disappear without
		 * a trace and just return without doing anything.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	fputc_value = fputc( c, file_rs232->output_file );

	if ( fputc_value == EOF ) {
		if ( ferror( file_rs232->input_file ) ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred while trying to write to "
			"file '%s' for record '%s'.  "
			"Errno = %d, error message = '%s'.",
				file_rs232->input_filename,
				rs232->record->name,
				saved_errno, strerror( saved_errno ) );
		} else {
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"fputc() reported an error for output file '%s' "
			"used by record '%s', but feof() and ferror() "
			"both reported no error.",
				file_rs232->input_filename,
				rs232->record->name );
		}
	}

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
	size_t actual_bytes_read;
	mx_status_type mx_status;

	mx_status = mxi_file_rs232_get_pointers( rs232, &file_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	actual_bytes_read = fread( buffer,
				1, max_bytes_to_read,
				file_rs232->input_file );

	if ( bytes_read != (size_t *) NULL ) {
		*bytes_read = actual_bytes_read;
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
	size_t actual_bytes_written;
	mx_status_type mx_status;

	mx_status = mxi_file_rs232_get_pointers( rs232, &file_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	actual_bytes_written = fwrite( buffer,
				1, max_bytes_to_write,
				file_rs232->output_file );

	if ( bytes_written != NULL ) {
		*bytes_written = actual_bytes_written;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_file_rs232_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] =
			"mxi_file_rs232_num_input_bytes_available()";

	MX_FILE_RS232 *file_rs232;
	int64_t file_size, file_offset, num_bytes_available;
	int saved_errno;
	mx_status_type mx_status;

	mx_status = mxi_file_rs232_get_pointers( rs232, &file_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( file_rs232->input_file == NULL ) {
		return mx_error( MXE_NOT_AVAILABLE, fname,
		"File '%s' for record '%s' is not open, so we cannot "
		"find the number of input bytes available for it.",
			file_rs232->input_filename,
			rs232->record->name );
	}

	file_size = mx_get_file_size( file_rs232->input_filename );

	if ( file_size < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Unable to get the file size of '%s' for record '%s'.  "
		"Errno = %d, error message = '%s'",
			file_rs232->input_filename,
			rs232->record->name,
			saved_errno, strerror(saved_errno) );
	}

	file_offset = mx_get_file_offset( file_rs232->input_file );

	num_bytes_available = file_size - file_offset;

	if ( num_bytes_available > ULONG_MAX ) {
		rs232->num_input_bytes_available = ULONG_MAX;
	} else {
		rs232->num_input_bytes_available =
			(unsigned long) num_bytes_available;
	}

#if MXI_FILE_RS232_DEBUG
	MX_DEBUG(-2,("%s: num_input_bytes_available = %ld",
			fname, rs232->num_input_bytes_available));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_file_rs232_discard_unread_input( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_file_rs232_discard_unread_input()";

	MX_FILE_RS232 *file_rs232;
	mx_status_type mx_status;

	mx_status = mxi_file_rs232_get_pointers( rs232, &file_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( file_rs232->input_file == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	(void) mx_set_file_offset( file_rs232->input_file, 0, SEEK_END );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_file_rs232_flush( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_file_rs232_flush()";

	MX_FILE_RS232 *file_rs232;
	int fflush_status, saved_errno;
	mx_status_type mx_status;

	mx_status = mxi_file_rs232_get_pointers( rs232, &file_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( file_rs232->output_file == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	fflush_status = fflush( file_rs232->output_file );

	if ( fflush_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while writing to output file '%s' "
		"for record '%s'.  Errno = %d, error message = '%s'.",
			file_rs232->output_filename,
			rs232->record->name,
			saved_errno, strerror(saved_errno) );
	}

	return MX_SUCCESSFUL_RESULT;
}

