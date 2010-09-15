/*
 * Name:    mx_vfile.c
 *
 * Purpose: Support for file variables in MX database description files.
 *          These variables can be used to read or write named pipes or
 *          Linux /proc, /sys, etc. variables
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2007, 2009-2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_VFILE_DEBUG		FALSE	

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_variable.h"
#include "mx_vfile.h"

MX_RECORD_FUNCTION_LIST mxv_file_variable_record_function_list = {
	mx_variable_initialize_driver,
	mxv_file_variable_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxv_file_variable_open,
	mxv_file_variable_close
};

MX_VARIABLE_FUNCTION_LIST mxv_file_variable_variable_function_list = {
	mxv_file_variable_send_variable,
	mxv_file_variable_receive_variable
};

/********************************************************************/

MX_RECORD_FIELD_DEFAULTS mxv_file_string_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_FILE_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_STRING_VARIABLE_STANDARD_FIELDS
};

long mxv_file_string_variable_num_record_fields
			= sizeof( mxv_file_string_variable_defaults )
			/ sizeof( mxv_file_string_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_file_string_variable_dptr
			= &mxv_file_string_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_file_long_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_FILE_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_LONG_VARIABLE_STANDARD_FIELDS
};

long mxv_file_long_variable_num_record_fields
			= sizeof( mxv_file_long_variable_defaults )
			/ sizeof( mxv_file_long_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_file_long_variable_dptr
			= &mxv_file_long_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_file_ulong_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_FILE_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_ULONG_VARIABLE_STANDARD_FIELDS
};

long mxv_file_ulong_variable_num_record_fields
			= sizeof( mxv_file_ulong_variable_defaults )
			/ sizeof( mxv_file_ulong_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_file_ulong_variable_dptr
			= &mxv_file_ulong_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_file_double_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_FILE_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_DOUBLE_VARIABLE_STANDARD_FIELDS
};

long mxv_file_double_variable_num_record_fields
			= sizeof( mxv_file_double_variable_defaults )
			/ sizeof( mxv_file_double_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_file_double_variable_dptr
			= &mxv_file_double_variable_defaults[0];

/********************************************************************/

static mx_status_type
mxv_file_variable_get_pointers( MX_VARIABLE *variable,
				MX_FILE_VARIABLE **file_variable,
				MX_RECORD_FIELD **local_value_field,
				void **local_value_pointer,
				const char *calling_fname )
{
	static const char fname[] = "mxv_file_variable_get_pointers()";

	MX_RECORD_FIELD *local_value_field_ptr;
	mx_status_type mx_status;

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( variable->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for MX_VARIABLE structure %p passed by '%s' is NULL.",
			variable, calling_fname );
	}

	if ( (variable->record->mx_superclass != MXR_VARIABLE)
	  || (variable->record->mx_class != MXV_FILE) ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The record '%s' passed by '%s' is not a file variable.",
			variable->record->name, calling_fname );
	}

	if ( file_variable != (MX_FILE_VARIABLE **) NULL ) {

		*file_variable = (MX_FILE_VARIABLE *)
				variable->record->record_class_struct;

		if ( *file_variable == (MX_FILE_VARIABLE *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_FILE_VARIABLE pointer for variable '%s' passed by '%s' is NULL.",
				variable->record->name, calling_fname );
		}
	}

	mx_status = mx_find_record_field( variable->record, "value",
						&local_value_field_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( local_value_field_ptr == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RECORD_FIELD pointer not found for variable '%s' "
		"passed by '%s'",
			variable->record->name, calling_fname );
	}

	if ( local_value_field != (MX_RECORD_FIELD **) NULL ) {
		*local_value_field = local_value_field_ptr;
	}

	if ( local_value_pointer != NULL ) {
		*local_value_pointer = mx_get_field_value_pointer(
							local_value_field_ptr );

		if ( *local_value_pointer == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Value pointer not found for record field '%s.%s' "
			"passed by '%s'", variable->record->name,
				local_value_field_ptr->name,
				calling_fname );
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxvp_file_variable_open_file( MX_FILE_VARIABLE *file_variable )
{
	static const char fname[] = "mxvp_file_variable_open_file()";

	int saved_errno;
	unsigned long flags;
	char mode[5];

	flags = file_variable->file_flags;

	if ( flags & MXF_VFILE_READ ) {
		if ( flags & MXF_VFILE_WRITE ) {
			strlcpy( mode, "w+", sizeof(mode) );
		} else {
			strlcpy( mode, "r", sizeof(mode) );
		}
	} else
	if ( flags & MXF_VFILE_WRITE ) {
		strlcpy( mode, "w", sizeof(mode) );
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Neither the read bit (0x1) or the write bit (0x2) is set "
		"in the 'file_flags' field (%#lx) for record '%s'",
			flags, file_variable->record->name );
	}

	file_variable->file = fopen( file_variable->filename, mode );

	if ( file_variable->file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while opening file '%s' for record '%s'.  "
		"Errno = %d, error message = '%s'.",
				file_variable->filename,
				file_variable->record->name,
				saved_errno, strerror(saved_errno) );
	}

	setvbuf( file_variable->file, (char *)NULL, _IOLBF, BUFSIZ );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxvp_file_variable_close_file( MX_FILE_VARIABLE *file_variable )
{
	static const char fname[] = "mxvp_file_variable_close_file()";

	int os_status, saved_errno;

	if ( file_variable->file != NULL ) {
		os_status = fclose( file_variable->file );

		if ( os_status != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while closing file '%s' for record '%s'.  "
		"Errno = %d, error message = '%s'.",
				file_variable->filename,
				file_variable->record->name,
				saved_errno, strerror(saved_errno) );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxvp_file_variable_read( MX_FILE_VARIABLE *file_variable,
			char *buffer, size_t buffer_length )
{
	static const char fname[] = "mxvp_file_variable_read()";

	char *ptr, *err_ptr;

#if MX_VFILE_DEBUG
	MX_DEBUG(-2,("%s: buffer_length = %ld", fname, (long)buffer_length));
#endif

	/* Read until we get a newline. */

	buffer[0] = '\0';

	ptr = buffer;

	while (1) {
		ptr = buffer + strlen(buffer);

		err_ptr = fgets( ptr, sizeof(buffer), file_variable->file );

		if ( err_ptr == NULL ) {
			if ( feof(file_variable->file) ) {
				clearerr(file_variable->file);

				return mx_error(
				MXE_END_OF_DATA | MXE_QUIET, fname,
			    "End of file for file '%s' used by variable '%s'.",
					file_variable->filename,
					file_variable->record->name );
			} else
			if ( ferror(file_variable->file) ) {
				clearerr(file_variable->file);

				return mx_error( MXE_FILE_IO_ERROR, fname,
				"An error occurred while reading file '%s' "
				"used by variable '%s'.",
					file_variable->filename,
					file_variable->record->name );
			} else {
				clearerr(file_variable->file);

				return mx_error( MXE_FILE_IO_ERROR, fname,
				"An unrecognized error occurred for file '%s' "
				"used by variable '%s'.",
					file_variable->filename,
					file_variable->record->name );
			}
		}

		/* Break out of the loop if we have received a newline. */

		if ( strchr(buffer, '\n' ) != NULL ) {
			break;		/* Exit the while(1) loop. */
		}
	}

#if MX_VFILE_DEBUG
	MX_DEBUG(-2,("%s: buffer = '%s'", fname, buffer));
#endif

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxvp_file_variable_write( MX_FILE_VARIABLE *file_variable, char *buffer )
{
	static const char fname[] = "mxvp_file_variable_write()";

	int os_status, saved_errno;

	os_status = fputs( buffer, file_variable->file );

	if ( os_status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Error writing string description '%s' to file variable '%s'.",
			buffer, file_variable->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_file_variable_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_file_variable_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_FILE_VARIABLE *file_variable;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCAN structure." );
	}

	file_variable = (MX_FILE_VARIABLE *)
				malloc( sizeof(MX_FILE_VARIABLE) );

	if ( file_variable == (MX_FILE_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_FILE_VARIABLE structure." );
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;
	file_variable->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = file_variable;
	record->record_type_struct = NULL;

	record->superclass_specific_function_list =
				&mxv_file_variable_variable_function_list;
	record->class_specific_function_list = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_file_variable_open( MX_RECORD *record )
{
	static const char fname[] = "mxv_file_variable_open()";

	MX_VARIABLE *variable;
	MX_FILE_VARIABLE *file_variable;
	unsigned long flags;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	variable = (MX_VARIABLE *) record->record_superclass_struct;

	mx_status = mxv_file_variable_get_pointers( variable,
						&file_variable,
						NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	flags = file_variable->file_flags;

	if ( (flags & MXF_VFILE_TEMP_OPEN) == 0 ) {
		mx_status = mxvp_file_variable_open_file( file_variable );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_file_variable_close( MX_RECORD *record )
{
	static const char fname[] = "mxv_file_variable_close()";

	MX_VARIABLE *variable;
	MX_FILE_VARIABLE *file_variable;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	variable = (MX_VARIABLE *) record->record_superclass_struct;

	mx_status = mxv_file_variable_get_pointers( variable,
						&file_variable,
						NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxvp_file_variable_close_file( file_variable );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_file_variable_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_file_variable_send_variable()";

	MX_FILE_VARIABLE *file_variable;
	MX_RECORD_FIELD *local_value_field;
	void *local_value_ptr;
	unsigned long flags;
	char write_buffer[500];
	mx_status_type mx_status;

	mx_status = mxv_file_variable_get_pointers( variable,
						&file_variable,
						&local_value_field,
						&local_value_ptr,
						fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	flags = file_variable->file_flags;

	if ( (flags & MXF_VFILE_WRITE) == 0 ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
		"File variable '%s' is not open for write access.",
			file_variable->filename );
	}

	if ( flags & MXF_VFILE_TEMP_OPEN ) {
		mx_status = mxvp_file_variable_open_file( file_variable );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_create_description_from_field( local_value_field->record,
							local_value_field,
							write_buffer,
							sizeof(write_buffer) );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_VFILE_DEBUG
	MX_DEBUG(-2,("%s: write_buffer = '%s'", fname, write_buffer));
#endif

	mx_status = mxvp_file_variable_write( file_variable, write_buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( flags & MXF_VFILE_TEMP_OPEN ) {
		mx_status = mxvp_file_variable_close_file( file_variable );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_file_variable_receive_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_file_variable_receive_variable()";

	MX_FILE_VARIABLE *file_variable;
	MX_RECORD_FIELD *local_value_field;
	void *local_value_ptr;
	unsigned long flags;
	long row;
	long row_offset, column_offset;
	long field_buffer_length, read_buffer_sizeof;
	char *column_ptr;
	char field_buffer[500], read_buffer[100];
	mx_status_type mx_status;

	mx_status = mxv_file_variable_get_pointers( variable,
						&file_variable,
						&local_value_field,
						&local_value_ptr,
						fname );


	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	flags = file_variable->file_flags;

	if ( (flags & MXF_VFILE_READ) == 0 ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
		"File variable '%s' is not open for read access.",
			file_variable->filename );
	}

	if ( flags & MXF_VFILE_TEMP_OPEN ) {
		mx_status = mxvp_file_variable_open_file( file_variable );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	row_offset    = file_variable->row_offset;
	column_offset = file_variable->column_offset;

	field_buffer[0] = '\0';

	read_buffer_sizeof = sizeof(read_buffer);

	for ( row = 0; ; row++ ) {
		mx_status = mxvp_file_variable_read( file_variable,
					read_buffer, read_buffer_sizeof );

		if ( mx_status.code == MXE_SUCCESS ) {
			/* Do nothing. */
		} else
		if ( mx_status.code == MXE_END_OF_DATA ) {
			break;		/* Exit the for() loop. */
		} else {
			return mx_status;
		}

#if MX_VFILE_DEBUG
		MX_DEBUG(-2,("%s: read_buffer = '%s'",
			fname, read_buffer));
#endif

		if ( row < row_offset ) {
			continue;    /* Go back to the top of the for() loop. */
		}

		switch( local_value_field->datatype ) {
		case MXFT_STRING:
			if ( column_offset >= read_buffer_sizeof ) {
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"Column offset %ld exceeds the maximum allowed "
				"column offset of %ld for variable '%s'.",
					column_offset,
					read_buffer_sizeof - 1L,
					file_variable->record->name );
			} else
			if ( column_offset >= 0 ) {
				column_ptr = read_buffer + column_offset;
			} else {
				column_ptr = read_buffer;
			}
			break;

		case MXFT_LONG:
		case MXFT_ULONG:
		case MXFT_DOUBLE:
			/* For these datatypes, the column number is
			 * interpreted as the number of tokens to skip
			 * rather than the number of bytes to skip.
			 */

			if ( column_offset <= 0 ) {
				column_ptr = read_buffer;
			} else {
				column_ptr = mx_skip_string_fields(
						read_buffer, column_offset );
			}
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported data type of %ld requested "
			"for file variable '%s'.",
				local_value_field->datatype,
				file_variable->record->name );
			break;
		}

		if ( flags & MXF_VFILE_CONCATENATE ) {
			/* All of the text on an input line is copied
			 * into one MX token by surrounding the text
			 * with double quotes (").
			 */

			strlcpy( field_buffer, "\"", sizeof(field_buffer) );
			strlcat( field_buffer, column_ptr,
					sizeof(field_buffer) );

			field_buffer_length = (long) strlen(field_buffer);

			if ( field_buffer[field_buffer_length - 1] == '\n' ) {
				field_buffer[field_buffer_length - 1] = '\0';
			}

			strlcat( field_buffer, "\"\n", sizeof(field_buffer) );
		} else {
			strlcat( field_buffer, column_ptr,
					sizeof(field_buffer) );
		}
	}

#if MX_VFILE_DEBUG
	MX_DEBUG(-2,("%s: field_buffer = '%s'", fname, field_buffer));
#endif

	mx_status = mx_create_field_from_description( local_value_field->record,
							local_value_field,
							NULL, field_buffer );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( flags & MXF_VFILE_TEMP_OPEN ) {
		mx_status = mxvp_file_variable_close_file( file_variable );
	}

	return mx_status;
}

