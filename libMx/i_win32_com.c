/*
 * Name:    i_win32_com.c
 *
 * Purpose: MX driver for Microsoft Win32 COM RS-232 devices.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 1999-2007, 2010, 2018, 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_WIN32COM_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_rs232.h"
#include "mx_record.h"

#if defined(OS_WIN32)	/* Include this only on Microsoft Win32 systems. */

/* The header file i_win32_com.h includes <windows.h> for us, so we don't have
 * to do that ourselves.
 */

#include "i_win32_com.h"

MX_RECORD_FUNCTION_LIST mxi_win32com_record_function_list = {
	NULL,
	mxi_win32com_create_record_structures,
	mxi_win32com_finish_record_initialization,
	NULL,
	NULL,
	mxi_win32com_open,
	mxi_win32com_close,
	NULL,
	mxi_win32com_resynchronize
};

MX_RS232_FUNCTION_LIST mxi_win32com_rs232_function_list = {
	mxi_win32com_getchar,
	mxi_win32com_putchar,
	mxi_win32com_read,
	mxi_win32com_write,
	NULL,
	NULL,
	mxi_win32com_num_input_bytes_available,
	mxi_win32com_discard_unread_input,
	mxi_win32com_discard_unwritten_output,
	mxi_win32com_get_signal_state,
	mxi_win32com_set_signal_state,
	mxi_win32com_get_configuration,
	mxi_win32com_set_configuration,
	mxi_win32com_send_break
};

MX_RECORD_FIELD_DEFAULTS mxi_win32com_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_WIN32COM_STANDARD_FIELDS
};

long mxi_win32com_num_record_fields
		= sizeof( mxi_win32com_record_field_defaults )
			/ sizeof( mxi_win32com_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_win32com_rfield_def_ptr
			= &mxi_win32com_record_field_defaults[0];

/* ---- */

/* A private function for the use of the driver. */

static mx_status_type
mxi_win32com_get_pointers( MX_RS232 *rs232,
			MX_WIN32COM **win32com,
			const char *calling_fname )
{
	static const char fname[] = "mxi_win32com_get_pointers()";

	MX_RECORD *win32com_record;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( win32com == (MX_WIN32COM **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_WIN32COM pointer passed by '%s' was NULL.",
			calling_fname );
	}

	win32com_record = rs232->record;

	if ( win32com_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The win32com_record pointer for the "
			"MX_RS232 pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*win32com = (MX_WIN32COM *) win32com_record->record_type_struct;

	if ( *win32com == (MX_WIN32COM *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_WIN32COM pointer for record '%s' is NULL.",
			win32com_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_win32com_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_win32com_create_record_structures()";

	MX_RS232 *rs232;
	MX_WIN32COM *win32com;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	win32com = (MX_WIN32COM *) malloc( sizeof(MX_WIN32COM) );

	if ( win32com == (MX_WIN32COM *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_WIN32COM structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = win32com;
	record->class_specific_function_list
				= &mxi_win32com_rs232_function_list;

	rs232->record = record;

	win32com->signal_state_initialized = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_win32com_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type status;

	/* Check to see if the RS-232 parameters are valid. */

	status = mx_rs232_check_port_parameters( record );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Mark the WIN32COM device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_win32com_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_win32com_open()";

	MX_RS232 *rs232;
	MX_WIN32COM *win32com;
	HANDLE com_port_handle;
	mx_status_type status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	status = mxi_win32com_get_pointers( rs232, &win32com, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Check the serial port name for validity. */

	if ( strnicmp( win32com->filename, "com", 3 ) != 0 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The serial device name '%s' is illegal.", win32com->filename );
	}

	/* Initialize the WIN32COM driver on this port.
	 *
	 * Note: The third argument to CreateFile() is the dwShareMode
	 * argument.  If dwShareMode is set to zero (like it is below),
	 * then the file is opened exclusive to the HANDEL com_port_handle.
	 * Thus, it is not necessary to call LockFileEx() to get that
	 * effect.
	 */

	com_port_handle = CreateFile( win32com->filename,
		GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, 0 );

	if ( com_port_handle == INVALID_HANDLE_VALUE ) {
		DWORD last_error_code;
		TCHAR message_buffer[MXU_ERROR_MESSAGE_LENGTH - 120];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		switch( last_error_code ) {
		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			return mx_error( MXE_NOT_FOUND, fname,
			"The requested device '%s' for MX interface '%s' "
			"was not found.",
				win32com->filename, record->name );
			break;
		case ERROR_ACCESS_DENIED:
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"Your account does not have permission to "
			"read and write the device '%s' for MX interface '%s'.",
				win32com->filename, record->name );
			break;
		default:
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"Error opening Win32 COM port '%s'.  "
				"Win32 error code = %ld, error message = '%s'",
				win32com->filename, last_error_code,
				message_buffer );
			break;
		}
	}

	win32com->handle = com_port_handle;

	status = mxi_win32com_set_configuration( rs232 );

	return status;
}

MX_EXPORT mx_status_type
mxi_win32com_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_win32com_close()";

	MX_RS232 *rs232;
	MX_WIN32COM *win32com;
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) (record->record_class_struct);

	status = mxi_win32com_get_pointers( rs232, &win32com, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Close the WIN32COM port. */

	CloseHandle( win32com->handle );

	/* Mark the port as closed. */

	win32com->handle = INVALID_HANDLE_VALUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_win32com_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_win32com_resynchronize()";

	MX_RS232 *rs232 = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_win32com_discard_unread_input( rs232 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_win32com_close( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;


	mx_status = mxi_win32com_open( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_win32com_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_win32com_getchar()";

	MX_WIN32COM *win32com;
	BOOL win32_status;
	DWORD num_chars;
	mx_status_type status;

	status = mxi_win32com_get_pointers( rs232, &win32com, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( rs232->transfer_flags & MXF_232_NOWAIT ) {

		status = mxi_win32com_num_input_bytes_available( rs232 );

		if ( status.code != MXE_SUCCESS )
			return status;

		if ( rs232->num_input_bytes_available == 0 ) {
			return mx_error( (MXE_NOT_READY | MXE_QUIET), fname,
				"Failed to read a character from port '%s'.",
				rs232->record->name );
		}
	}

	win32_status = ReadFile( win32com->handle, c, 1, &num_chars, 0 );

	if ( win32_status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[MXU_ERROR_MESSAGE_LENGTH - 120];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error while trying to read a character from '%s'.  "
			"Win32 error code = %ld, error message = '%s'",
			win32com->filename, last_error_code, message_buffer );
	}

#if MXI_WIN32COM_DEBUG
	MX_DEBUG(-2,("%s: received c = 0x%x, '%c'", fname, *c, *c));
#endif

	/* mxi_win32com_getchar() is often used to test whether there is
	 * input ready on the COM port.  Normally, it is not desireable
	 * to broadcast a message to the world when this fails, so we
	 * add the MXE_QUIET flag to the error code.
	 */

	if ( num_chars != 1 ) {
		return mx_error( (MXE_NOT_READY | MXE_QUIET), fname,
			"Failed to read a character from port '%s'.",
			rs232->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_win32com_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_win32com_putchar()";

	MX_WIN32COM *win32com;
	BOOL win32_status;
	DWORD num_chars;
	mx_status_type status;

#if MXI_WIN32COM_DEBUG
	MX_DEBUG(-2,("%s: sending c = 0x%x, '%c'", fname, c, c));
#endif

	if ( rs232->transfer_flags & MXF_232_NOWAIT ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		  "Non-blocking Win32 COM port I/O not yet implemented.");
	}

	status = mxi_win32com_get_pointers( rs232, &win32com, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	win32_status = WriteFile( win32com->handle, &c, 1, &num_chars, 0 );

	if ( win32_status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[MXU_ERROR_MESSAGE_LENGTH - 120];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Attempt to send a character to port '%s' failed.  "
			"Win32 error code = %ld, error message = '%s'",
			win32com->filename, last_error_code, message_buffer );
	}

	/* Force the character to be transmitted to the device. */

	win32_status = FlushFileBuffers( win32com->handle );

	if ( win32_status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[MXU_ERROR_MESSAGE_LENGTH - 120];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Attempt to flush the output for port '%s' failed.  "
			"Win32 error code = %ld, error message = '%s'",
			win32com->filename, last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_win32com_read( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_read,
			size_t *bytes_read )
{
	static const char fname[] = "mxi_win32com_read()";

	mx_status_type status;
	long i;

	MX_DEBUG( 2,("%s invoked.", fname));

	/* This is slow, but simple.  It currently waits forever until
	 * 'max_chars' are read.  Think about doing something better later.
	 */

	for ( i = 0; i < max_bytes_to_read; i++ ) {
		status = mxi_win32com_getchar(rs232, &buffer[i]);

		if ( status.code != MXE_SUCCESS ) {
			i--;
			continue;
		}
	}

	*bytes_read = max_bytes_to_read;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_win32com_write( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_write,
			size_t *bytes_written )
{
	static const char fname[] = "mxi_win32com_write()";

	mx_status_type status;
	long i;

	MX_DEBUG( 2,("%s invoked.", fname));

	/* This is slow, but simple. */

	for ( i = 0; i < max_bytes_to_write; i++ ) {
		status = mxi_win32com_putchar( rs232, buffer[i]);

		if ( status.code != MXE_SUCCESS ) {
			return status;
		}
	}

	*bytes_written = max_bytes_to_write;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_win32com_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_win32com_num_input_bytes_available()";

	MX_WIN32COM *win32com;
	DWORD error_mask;
	COMSTAT com_status;
	BOOL win32_status;
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked.",fname));

	status = mxi_win32com_get_pointers( rs232, &win32com, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	win32_status = ClearCommError( win32com->handle,
						&error_mask, &com_status );

	if ( win32_status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[MXU_ERROR_MESSAGE_LENGTH - 120];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error while trying to check for input from '%s'.  "
			"Win32 error code = %ld, error message = '%s'",
			win32com->filename, last_error_code, message_buffer );
	}

	rs232->num_input_bytes_available = com_status.cbInQue;

	MX_DEBUG( 2,("%s complete.",fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_win32com_discard_unread_input( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_win32com_discard_unread_input()";

	MX_WIN32COM *win32com;
	BOOL win32_status;
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked.",fname));

	status = mxi_win32com_get_pointers( rs232, &win32com, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	win32_status = PurgeComm( win32com->handle,
					( PURGE_RXABORT | PURGE_RXCLEAR ) );

	if ( win32_status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[MXU_ERROR_MESSAGE_LENGTH - 120];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error while trying to discard unread input for '%s'.  "
			"Win32 error code = %ld, error message = '%s'",
			win32com->filename, last_error_code, message_buffer );
	}

	MX_DEBUG( 2,("%s complete.",fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_win32com_discard_unwritten_output( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_win32com_discard_unwritten_output()";

	MX_WIN32COM *win32com;
	BOOL win32_status;
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked.",fname));

	status = mxi_win32com_get_pointers( rs232, &win32com, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	win32_status = PurgeComm( win32com->handle,
					( PURGE_TXABORT | PURGE_TXCLEAR ) );

	if ( win32_status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[MXU_ERROR_MESSAGE_LENGTH - 120];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error while trying to discard unwritten output for '%s'.  "
			"Win32 error code = %ld, error message = '%s'",
			win32com->filename, last_error_code, message_buffer );
	}

	MX_DEBUG( 2,("%s complete.",fname));

	return MX_SUCCESSFUL_RESULT;
}

#define MXF_WIN32COM_SIGNAL_READ_MASK \
		( MXF_232_CLEAR_TO_SEND | MXF_232_DATA_SET_READY \
		  | MXF_232_DATA_CARRIER_DETECT | MXF_232_RING_INDICATOR )

#define MXF_WIN32COM_SIGNAL_WRITE_MASK \
		( MXF_232_REQUEST_TO_SEND | MXF_232_DATA_TERMINAL_READY )

MX_EXPORT mx_status_type
mxi_win32com_get_signal_state( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_win32com_get_signal_state()";

	MX_WIN32COM *win32com;
	DWORD modem_status;
	BOOL status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, rs232->record->name ));

	win32com = (MX_WIN32COM *) rs232->record->record_type_struct;

	rs232->signal_state &= ~ MXF_WIN32COM_SIGNAL_READ_MASK;

	status = GetCommModemStatus( win32com->handle, &modem_status );

	if ( status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[MXU_ERROR_MESSAGE_LENGTH - 120];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error getting RS-232 signal status from "
			"WIN32COM port '%s'.  "
			"Win32 error code = %ld, error message = '%s'",
			win32com->filename, last_error_code, message_buffer );
	}

	win32com->modem_status = (unsigned long) modem_status;

	if ( modem_status & MS_CTS_ON ) {
		rs232->signal_state |= MXF_232_CLEAR_TO_SEND;
	}
	if ( modem_status & MS_DSR_ON ) {
		rs232->signal_state |= MXF_232_DATA_SET_READY;
	}
	if ( modem_status & MS_RING_ON ) {
		rs232->signal_state |= MXF_232_RING_INDICATOR;
	}
	if ( modem_status & MS_RLSD_ON ) {
		rs232->signal_state |= MXF_232_DATA_CARRIER_DETECT;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_win32com_set_signal_state( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_win32com_set_signal_state()";

	MX_WIN32COM *win32com;
	unsigned long old_rts, new_rts, old_dtr, new_dtr;
	unsigned long saved_read_state;
	int initialized;

	BOOL status;
	DWORD comm_function;
	DWORD last_error_code;
	TCHAR message_buffer[MXU_ERROR_MESSAGE_LENGTH - 120];

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, rs232->record->name ));

	win32com = (MX_WIN32COM *) rs232->record->record_type_struct;

	initialized = win32com->signal_state_initialized;

	/* Preserve the read bits. */

	saved_read_state =
		rs232->old_signal_state & MXF_WIN32COM_SIGNAL_READ_MASK;

	rs232->signal_state |= saved_read_state;

	/* RTS */

	old_rts = rs232->old_signal_state | MXF_232_REQUEST_TO_SEND;

	new_rts = rs232->signal_state | MXF_232_REQUEST_TO_SEND;

	if ( ( new_rts != old_rts ) || ( ! initialized ) ) {
		if ( new_rts == 0 ) {
			comm_function = CLRRTS;
		} else {
			comm_function = SETRTS;
		}

		status = EscapeCommFunction( win32com->handle, comm_function );

		if ( status == 0 ) {

			last_error_code = GetLastError();

			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error changing RTS state for Win32 COM port '%s'.  "
			"Win32 error code = %ld, error message = '%s'",
			win32com->filename, last_error_code, message_buffer );
		}
	}

	/* DTR */

	old_dtr = rs232->old_signal_state | MXF_232_DATA_TERMINAL_READY;

	new_dtr = rs232->signal_state | MXF_232_DATA_TERMINAL_READY;

	if ( ( new_dtr != old_dtr ) || ( ! initialized ) ) {
		if ( new_dtr == 0 ) {
			comm_function = CLRDTR;
		} else {
			comm_function = SETDTR;
		}

		status = EscapeCommFunction( win32com->handle, comm_function );

		if ( status == 0 ) {

			last_error_code = GetLastError();

			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error changing DTR state for Win32 COM port '%s'.  "
			"Win32 error code = %ld, error message = '%s'",
			win32com->filename, last_error_code, message_buffer );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_win32com_get_configuration( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_win32com_get_configuration()";

	MX_WIN32COM *win32com;
	BOOL win32_status;
	DCB dcb;
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked.",fname));

	status = mxi_win32com_get_pointers( rs232, &win32com, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( win32com->handle == INVALID_HANDLE_VALUE ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Win32 COM port device '%s' is not open.",
		rs232->record->name );
	}

	/* Get the current settings of the COM port. */

	win32_status = GetCommState( win32com->handle, &dcb );

	if ( win32_status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[MXU_ERROR_MESSAGE_LENGTH - 120];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Attempt to get current port settings for '%s' failed.  "
		"Win32 error code = %ld, error message = '%s'",
			win32com->filename, last_error_code, message_buffer );
	}

	/* Get the port speed. */

	switch( dcb.BaudRate ) {
	case CBR_256000: rs232->speed = 256000; break;
	case CBR_128000: rs232->speed = 128000; break;
	case CBR_115200: rs232->speed = 115200; break;
	case CBR_57600:  rs232->speed = 57600;  break;
	case CBR_56000:  rs232->speed = 56000;  break;
	case CBR_38400:  rs232->speed = 38400;  break;
	case CBR_19200:  rs232->speed = 19200;  break;
	case CBR_14400:  rs232->speed = 14400;  break;
	case CBR_9600:   rs232->speed = 9600;   break;
	case CBR_4800:   rs232->speed = 4800;   break;
	case CBR_2400:   rs232->speed = 2400;   break;
	case CBR_1200:   rs232->speed = 1200;   break;
	case CBR_600:    rs232->speed = 600;    break;
	case CBR_300:    rs232->speed = 300;    break;
	case CBR_110:    rs232->speed = 110;    break;

	default:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unrecognized Win32 COM speed %lu for COM port '%s'.", 
		(unsigned long) dcb.BaudRate, rs232->record->name);

		break;
	}

	/* Get the word size. */

	rs232->word_size = dcb.ByteSize;

	/* Get the parity. */

	if ( dcb.fParity == FALSE ) {
		rs232->parity = MXF_232_NO_PARITY;
	} else {
		switch( dcb.Parity ) {
		case EVENPARITY:  rs232->parity = MXF_232_EVEN_PARITY;  break;
		case ODDPARITY:   rs232->parity = MXF_232_ODD_PARITY;   break;
		case MARKPARITY:  rs232->parity = MXF_232_MARK_PARITY;  break;
		case SPACEPARITY: rs232->parity = MXF_232_SPACE_PARITY; break;
		case NOPARITY:    rs232->parity = MXF_232_NO_PARITY;    break;

		default:
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Unrecognized Win32 COM parity %lu for COM port '%s'.", 
			(unsigned long) dcb.Parity, rs232->record->name);

			break;
		}
	}

	/* Get the stop bits. */

	switch( dcb.StopBits ) {
	case ONESTOPBIT:    rs232->stop_bits = 1;   break;
	case TWOSTOPBITS:   rs232->stop_bits = 2;   break;

	default:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unrecognized Win32 COM stop bits %lu for COM port '%s'.",
		(unsigned long) dcb.StopBits, rs232->record->name );

		break;
	}

	/* Get the flow control. */

	if ( dcb.fOutxCtsFlow == TRUE ) {
		rs232->flow_control = MXF_232_HARDWARE_FLOW_CONTROL;
	} else
	if ( dcb.fOutxDsrFlow == TRUE ) {
		rs232->flow_control = MXF_232_DTR_DSR_FLOW_CONTROL;
	} else
	if ( dcb.fOutX == TRUE ) {
		rs232->flow_control = MXF_232_SOFTWARE_FLOW_CONTROL;
	} else {
		rs232->flow_control = MXF_232_NO_FLOW_CONTROL;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_win32com_set_configuration( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_win32com_set_configuration()";

	MX_WIN32COM *win32com;
	BOOL win32_status;
	DCB dcb;
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked.",fname));

	status = mxi_win32com_get_pointers( rs232, &win32com, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( win32com->handle == INVALID_HANDLE_VALUE ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Win32 COM port device '%s' is not open.",
		rs232->record->name );
	}

	/* Get the current settings of the COM port. */

	win32_status = GetCommState( win32com->handle, &dcb );

	if ( win32_status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[MXU_ERROR_MESSAGE_LENGTH - 120];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"An attempt to get the current port settings for device '%s' "
		"used by MX interface '%s' failed.  "
		"Win32 error code = %ld, error message = '%s'",
			win32com->filename, rs232->record->name,
		       	last_error_code, message_buffer );
	}

	/* Set the port speed. */

	switch( rs232->speed ) {
	case 256000: dcb.BaudRate = CBR_256000; break;
	case 128000: dcb.BaudRate = CBR_128000; break;
	case 115200: dcb.BaudRate = CBR_115200; break;
	case 57600:  dcb.BaudRate = CBR_57600;  break;
	case 56000:  dcb.BaudRate = CBR_56000;  break;
	case 38400:  dcb.BaudRate = CBR_38400;  break;
	case 19200:  dcb.BaudRate = CBR_19200;  break;
	case 14400:  dcb.BaudRate = CBR_14400;  break;
	case 9600:   dcb.BaudRate = CBR_9600;   break;
	case 4800:   dcb.BaudRate = CBR_4800;   break;
	case 2400:   dcb.BaudRate = CBR_2400;   break;
	case 1200:   dcb.BaudRate = CBR_1200;   break;
	case 600:    dcb.BaudRate = CBR_600;    break;
	case 300:    dcb.BaudRate = CBR_300;    break;
	case 110:    dcb.BaudRate = CBR_110;    break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported Win32 COM speed = %ld for COM port '%s'.", 
		rs232->speed, rs232->record->name);

		break;
	}

	/* Set the word size. */

	if ( rs232->word_size >= 4 && rs232->word_size <= 8 ) {
		dcb.ByteSize = rs232->word_size;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported Win32 COM word size = %ld for COM port '%s'.", 
		rs232->word_size, rs232->record->name);
	}

	/* Set the parity. */

	dcb.fParity = TRUE;

	switch( rs232->parity ) {
	case MXF_232_EVEN_PARITY:  dcb.Parity = EVENPARITY;  break;
	case MXF_232_ODD_PARITY:   dcb.Parity = ODDPARITY;   break;
	case MXF_232_MARK_PARITY:  dcb.Parity = MARKPARITY;  break;
	case MXF_232_SPACE_PARITY: dcb.Parity = SPACEPARITY; break;

	case MXF_232_NO_PARITY:
		dcb.Parity = NOPARITY;
		dcb.fParity = FALSE;
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported Win32 COM parity = '%c' for COM port '%s'.", 
		rs232->parity, rs232->record->name);

		break;
	}

	/* Set the stop bits. */

	switch( rs232->stop_bits ) {
	case 1:  dcb.StopBits = ONESTOPBIT; break;
	case 2:  dcb.StopBits = TWOSTOPBITS; break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported Win32 COM stop bits = %ld for COM port '%s'.", 
		rs232->stop_bits, rs232->record->name);

		break;
	}

	/* Set the flow control parameters. */

	dcb.fInX = FALSE;
	dcb.fOutX = FALSE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;
	dcb.fDtrControl = DTR_CONTROL_ENABLE;

	switch( rs232->flow_control ) {
	case MXF_232_NO_FLOW_CONTROL:
		break;

	case MXF_232_SOFTWARE_FLOW_CONTROL:
		dcb.fInX = TRUE;
		dcb.fOutX = TRUE;
		dcb.XonLim = 50;
		dcb.XoffLim = 200;
		dcb.XonChar = 17;
		dcb.XoffChar = 19;
		break;

	case MXF_232_RTS_CTS_FLOW_CONTROL:
	case MXF_232_HARDWARE_FLOW_CONTROL:
		dcb.fOutxCtsFlow = TRUE;
		dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
		break;

	case MXF_232_DTR_DSR_FLOW_CONTROL:
		dcb.fOutxDsrFlow = TRUE;
		dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported Win32 COM flow control = %c for COM port '%s'.", 
		rs232->flow_control, rs232->record->name);

		break;
	}

	/* Everything is set, so change the configuration of the COM port.*/

	win32_status = SetCommState( win32com->handle, &dcb );

	if ( win32_status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[MXU_ERROR_MESSAGE_LENGTH - 120];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Attempt to set current port settings for '%s' failed.  "
			"Win32 error code = %ld, error message = '%s'",
			win32com->filename, last_error_code, message_buffer );
	}
		
	MX_DEBUG( 2,("%s complete.",fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_win32com_send_break( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_win32com_send_break()";

	MX_WIN32COM *win32com;
	BOOL win32_status;
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked.",fname));

	status = mxi_win32com_get_pointers( rs232, &win32com, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( win32com->handle == INVALID_HANDLE_VALUE ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Win32 COM port device '%s' is not open.",
		rs232->record->name );
	}

	/* Turn the break signal on. */

	win32_status = SetCommBreak( win32com->handle );

	if ( win32_status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[MXU_ERROR_MESSAGE_LENGTH - 120];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Attempt to send a Break signal for '%s' failed.  "
			"Win32 error code = %ld, error message = '%s'",
			win32com->filename, last_error_code, message_buffer );
	}

	/* Leave the break signal on for about 0.5 seconds. */

	Sleep(500);

	/* Turn the break signal off. */
		
	win32_status = ClearCommBreak( win32com->handle );

	if ( win32_status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[MXU_ERROR_MESSAGE_LENGTH - 120];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Attempt to stop a Break signal for '%s' failed.  "
			"Win32 error code = %ld, error message = '%s'",
			win32com->filename, last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* OS_WIN32 */
