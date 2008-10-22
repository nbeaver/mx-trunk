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
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_U500

#if defined(OS_WIN32)
#include <windows.h>
#endif

#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
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
	NULL,
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

	u500_rs232->pipe_handle = INVALID_HANDLE_VALUE;

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
	int os_major, os_minor, os_update;
	mx_status_type mx_status;

	DWORD open_mode, pipe_mode, max_instances;
	DWORD out_buffer_size, in_buffer_size, default_timeout;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = record->record_class_struct;

	mx_status = mxi_u500_rs232_get_pointers( rs232,
						&u500_rs232, NULL, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	u500_rs232->pipe_handle = INVALID_HANDLE_VALUE;

	/* The 'u500_rs232' driver provides the option of reading text strings
	 * produced using the U500's MESSAGE FILE command by providing a named
	 * pipe for the U500 to write the messages to.
	 *
	 * If the length of the named pipe is zero, then it is assumed that
	 * we do not want to make use of the MESSAGE FILE named pipe feature.
	 */

	if ( strlen(u500_rs232->pipe_name) == 0 )
		return MX_SUCCESSFUL_RESULT;

	mx_status = mx_get_os_version( &os_major, &os_minor, &os_update );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If requested, create the named pipe now.  Only one pipe
	 * with a given name is allowed to exist.
	 *
	 * Note: CreateNamedPipe() will fail on Windows 95/98/ME.
	 */

	open_mode = PIPE_ACCESS_INBOUND;
	pipe_mode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT;
	max_instances = 1;
	out_buffer_size = 0;
	in_buffer_size = 1000;
	default_timeout = 1000;    /* in milliseconds */

	u500_rs232->pipe_handle = CreateNamedPipe( u500_rs232->pipe_name,
							open_mode,
							pipe_mode,
							max_instances,
							out_buffer_size,
							in_buffer_size,
							default_timeout,
							NULL );

	if ( u500_rs232->pipe_handle == INVALID_HANDLE_VALUE ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"Unable to create the named pipe '%s' for record '%s'.  "
		"Win32 error code = %ld, error message = '%s'.",
			u500_rs232->pipe_name, record->name,
			last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_u500_rs232_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_u500_rs232_close()";

	MX_RS232 *rs232;
	MX_U500_RS232 *u500_rs232 = NULL;
	mx_status_type mx_status;

	BOOL close_status;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = record->record_class_struct;

	mx_status = mxi_u500_rs232_get_pointers( rs232,
						&u500_rs232, NULL, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( u500_rs232->pipe_handle == INVALID_HANDLE_VALUE )
		return MX_SUCCESSFUL_RESULT;

	close_status = CloseHandle( u500_rs232->pipe_handle );

	u500_rs232->pipe_handle = INVALID_HANDLE_VALUE;

	if ( close_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"Unable to close named pipe '%s' for record '%s'.  "
		"Win32 error code = %ld, error message = '%s'.",
			u500_rs232->pipe_name, record->name,
			last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_u500_rs232_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_u500_rs232_getchar()";

	MX_U500_RS232 *u500_rs232 = NULL;
	mx_status_type mx_status;

	BOOL read_status;
	DWORD win32_bytes_read;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	mx_status = mxi_u500_rs232_get_pointers( rs232,
						&u500_rs232, NULL, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( u500_rs232->pipe_handle == INVALID_HANDLE_VALUE ) {
		return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
		"The named pipe '%s' is not open for record '%s'.",
			u500_rs232->pipe_name, rs232->record->name );
	}

	read_status = ReadFile( u500_rs232->pipe_handle, c,
				1, &win32_bytes_read, NULL );

	if ( read_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"An attempt to read from named pipe '%s' for record '%s' "
		"failed.  Win32 error code = %ld, error message = '%s'.",
			last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
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

	prefix_length = 3;

	if ( mx_strncasecmp( "MX ", buffer, prefix_length ) != 0 ) {
		mx_status = mxi_u500_command( u500,
				u500_rs232->board_number, buffer );
        } else {
		/* The U500 does not have a programming command that starts
		 * with the string 'MX '.  If the calling routine sends us
		 * a buffer that starts with 'MX ', then we interpret this
		 * as a request to replace the string 'MX ' with the string
		 * 'ME FI(%s,a) ' where %s is the contents of the 'pipe_name'
		 * field of the 'u500_rs232' driver.
		 * 
		 * This is just a convenience that allows the user to write
		 * a U500 script without knowing the name of the U500 pipe.
		 * You do not have to use the 'MX ' command if you do not
		 * want to.
		 */

		snprintf( local_buffer, sizeof(local_buffer),
			"ME FI(%s,A) %s", u500_rs232->pipe_name,
					buffer + prefix_length );

		mx_status = mxi_u500_command( u500,
				u500_rs232->board_number, local_buffer );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_u500_rs232_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] =
		"mxi_u500_rs232_num_input_bytes_available()";

	MX_U500_RS232 *u500_rs232 = NULL;
	mx_status_type mx_status;

	BOOL pipe_status;
	DWORD bytes_read, total_bytes_available;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	TCHAR peek_buffer[1000];

	mx_status = mxi_u500_rs232_get_pointers( rs232,
						&u500_rs232, NULL, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the named pipe is not open, then we merely state that
	 * zero bytes are available.
	 */

	if ( u500_rs232->pipe_handle == INVALID_HANDLE_VALUE ) {
		rs232->num_input_bytes_available = 0;
		return MX_SUCCESSFUL_RESULT;
	}

	pipe_status = PeekNamedPipe( u500_rs232->pipe_handle,
			peek_buffer, sizeof(peek_buffer),
			&bytes_read, &total_bytes_available, NULL );

#if 1
	MX_DEBUG(-2,("%s: bytes_read = %ld, total_bytes_available = %ld",
		fname, (long) bytes_read, (long) total_bytes_available));
#endif

	if ( pipe_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"An attempt to determine the number of bytes available "
		"in named pipe '%s' for record '%s' failed.  "
		"Win32 error code = %ld, error message = '%s'.",
			u500_rs232->pipe_name, rs232->record->name,
			last_error_code, message_buffer );
	}

	rs232->num_input_bytes_available = total_bytes_available;

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_U500 */

