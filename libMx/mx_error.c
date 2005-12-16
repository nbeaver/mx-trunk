/*
 * Name:    mx_error.c
 *
 * Purpose: Functions for displaying error output.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------
 *
 * Copyright 1999-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "mx_util.h"

typedef struct {
	int error_code;
	const char error_message_text[40];
} MX_ERROR_MESSAGE;

static MX_ERROR_MESSAGE error_message_list[] = {
{ MXE_SUCCESS,				"MXE_SUCCESS" },
{ MXE_NULL_ARGUMENT,			"MXE_NULL_ARGUMENT" },
{ MXE_ILLEGAL_ARGUMENT,			"MXE_ILLEGAL_ARGUMENT" },
{ MXE_CORRUPT_DATA_STRUCTURE,		"MXE_CORRUPT_DATA_STRUCTURE" },
{ MXE_UNPARSEABLE_STRING,		"MXE_UNPARSEABLE_STRING" },
{ MXE_END_OF_DATA,			"MXE_END_OF_DATA" },
{ MXE_UNEXPECTED_END_OF_DATA,		"MXE_UNEXPECTED_END_OF_DATA" },
{ MXE_NOT_FOUND,			"MXE_NOT_FOUND" },
{ MXE_TYPE_MISMATCH,			"MXE_TYPE_MISMATCH" },
{ MXE_NOT_YET_IMPLEMENTED,		"MXE_NOT_YET_IMPLEMENTED" },
{ MXE_UNSUPPORTED,			"MXE_UNSUPPORTED" },
{ MXE_OUT_OF_MEMORY,			"MXE_OUT_OF_MEMORY" },
{ MXE_WOULD_EXCEED_LIMIT,		"MXE_WOULD_EXCEED_LIMIT" },
{ MXE_LIMIT_WAS_EXCEEDED,		"MXE_LIMIT_WAS_EXCEEDED" },
{ MXE_INTERFACE_IO_ERROR,		"MXE_INTERFACE_IO_ERROR" },
{ MXE_DEVICE_IO_ERROR,			"MXE_DEVICE_IO_ERROR" },
{ MXE_FILE_IO_ERROR,			"MXE_FILE_IO_ERROR" },
{ MXE_TERMINAL_IO_ERROR,		"MXE_TERMINAL_IO_ERROR" },
{ MXE_IPC_IO_ERROR,			"MXE_IPC_IO_ERROR" },
{ MXE_IPC_CONNECTION_LOST,		"MXE_IPC_CONNECTION_LOST" },
{ MXE_NETWORK_IO_ERROR,			"MXE_NETWORK_IO_ERROR" },
{ MXE_NETWORK_CONNECTION_LOST,		"MXE_NETWORK_CONNECTION_LOST" },
{ MXE_NOT_READY,			"MXE_NOT_READY" },
{ MXE_INTERRUPTED,			"MXE_INTERRUPTED" },
{ MXE_PAUSE_REQUESTED,			"MXE_PAUSE_REQUESTED" },
{ MXE_INTERFACE_ACTION_FAILED,		"MXE_INTERFACE_ACTION_FAILED" },
{ MXE_DEVICE_ACTION_FAILED,		"MXE_DEVICE_ACTION_FAILED" },
{ MXE_FUNCTION_FAILED,			"MXE_FUNCTION_FAILED" },
{ MXE_CONTROLLER_INTERNAL_ERROR,	"MXE_CONTROLLER_INTERNAL_ERROR" },
{ MXE_PERMISSION_DENIED,        	"MXE_PERMISSION_DENIED" },
{ MXE_CLIENT_REQUEST_DENIED,		"MXE_CLIENT_REQUEST_DENIED" },
{ MXE_TRY_AGAIN,			"MXE_TRY_AGAIN" },
{ MXE_TIMED_OUT,			"MXE_TIMED_OUT" },
{ MXE_HARDWARE_CONFIGURATION_ERROR,	"MXE_HARDWARE_CONFIGURATION_ERROR" },
{ MXE_HARDWARE_FAULT,			"MXE_HARDWARE_FAULT" },
{ MXE_RECORD_DISABLED_DUE_TO_FAULT,	"MXE_RECORD_DISABLED_DUE_TO_FAULT" },
{ MXE_RECORD_DISABLED_BY_USER,		"MXE_RECORD_DISABLED_BY_USER" },
{ MXE_INITIALIZATION_ERROR,		"MXE_INITIALIZATION_ERROR" },
{ MXE_READ_ONLY,			"MXE_READ_ONLY" },
{ MXE_SOFTWARE_CONFIGURATION_ERROR,	"MXE_SOFTWARE_CONFIGURATION_ERROR" },
{ MXE_OPERATING_SYSTEM_ERROR,		"MXE_OPERATING_SYSTEM_ERROR" },
{ MXE_UNKNOWN_ERROR,			"MXE_UNKNOWN_ERROR" },
{ MXE_NOT_VALID_FOR_CURRENT_STATE,	"MXE_NOT_VALID_FOR_CURRENT_STATE" },
{ MXE_CONFIGURATION_CONFLICT,		"MXE_CONFIGURATION_CONFLICT" },
{ MXE_NOT_AVAILABLE,			"MXE_NOT_AVAILABLE" },
{ MXE_STOP_REQUESTED,			"MXE_STOP_REQUESTED" },
{ MXE_BAD_HANDLE,			"MXE_BAD_HANDLE" },
{ MXE_OBJECT_ABANDONED,			"MXE_OBJECT_ABANDONED" },
{ MXE_MIGHT_CAUSE_DEADLOCK,		"MXE_MIGHT_CAUSE_DEADLOCK" },
{ MXE_ALREADY_EXISTS,			"MXE_ALREADY_EXISTS" },
{ 0, "" } };

static long num_error_messages = sizeof(error_message_list)
				/ sizeof(error_message_list[0]);

static void (*mx_error_output_function)( char * )
					= mx_error_default_output_function;

#if ( ! USE_STACK_BASED_MX_ERROR )

static char mx_error_message_buffer[MXU_ERROR_MESSAGE_LENGTH + 1];

#endif /* USE_STACK_BASED_MX_ERROR */

MX_EXPORT mx_status_type
mx_error( long error_code, const char *location, char *format, ... )
{
	mx_status_type status_struct;

	va_list args;
	char buffer1[2500];
	char buffer2[2500];
	long i;

	va_start(args, format);
	vsprintf(buffer1, format, args);
	va_end(args);

	/* Fill in the status structure. */

	status_struct.code = error_code;
	status_struct.location = location;

#if USE_STACK_BASED_MX_ERROR

	strncpy( status_struct.message, buffer1, MXU_ERROR_MESSAGE_LENGTH );

#else /* not USE_STACK_BASED_MX_ERROR */

	strncpy( mx_error_message_buffer, buffer1, MXU_ERROR_MESSAGE_LENGTH );

	status_struct.message = &mx_error_message_buffer[0];

#endif /* USE_STACK_BASED_MX_ERROR */

	/* Format the optional error message. */

	if ( mx_error_output_function != NULL ) {

		i = error_code - 1000;

		if ( i >= 0 && i < num_error_messages ) {
			sprintf( buffer2,
				"%s in %s:\n-> %s",
				error_message_list[i].error_message_text,
				location, buffer1 );
		} else {
			sprintf( buffer2,
		"Unrecognized error code %ld occurred in '%s':\n-> %s",
				error_code, location, buffer1 );
		}

		(*mx_error_output_function)( buffer2 );
	}
	return status_struct;
}

/* mx_error_quiet() is just like mx_error() except that it doesn't
 * invoke *mx_error_output_function().
 */

MX_EXPORT mx_status_type
mx_error_quiet( long error_code, const char *location, char *format, ... )
{
	mx_status_type status_struct;

	va_list args;
	char buffer1[2500];

	va_start(args, format);
	vsprintf(buffer1, format, args);
	va_end(args);

	/* Fill in the status structure. */

	status_struct.code = error_code;
	status_struct.location = location;

#if USE_STACK_BASED_MX_ERROR

	strncpy( status_struct.message, buffer1, MXU_ERROR_MESSAGE_LENGTH );

#else /* not USE_STACK_BASED_MX_ERROR */

	strncpy( mx_error_message_buffer, buffer1, MXU_ERROR_MESSAGE_LENGTH );

	status_struct.message = &mx_error_message_buffer[0];

#endif /* USE_STACK_BASED_MX_ERROR */

	return status_struct;
}

MX_EXPORT char *
mx_strerror( long error_code, char *buffer, size_t max_buffer_length )
{
	static char local_buffer[80];
	char *ptr;
	long i;

	if ( buffer != NULL ) {
		ptr = buffer;
	} else {
		ptr = local_buffer;
		max_buffer_length = sizeof( local_buffer );
	}

	i = error_code - 1000;

	if ( i >= 0 && i < num_error_messages ) {
		strlcpy( ptr, error_message_list[i].error_message_text,
					max_buffer_length );
	} else {
		sprintf( ptr, "MXE_(%ld)", error_code );
	}
	return ptr;
}

MX_EXPORT const char *
mx_status_code_string( long mx_status_code_value )
{
	static const char unknown_error_message[]
				= "MXE_(Unknown Error Message)";
	const char *ptr;
	long i;

	i = mx_status_code_value - 1000;

	if ( i >= 0 && i < num_error_messages ) {
		ptr = error_message_list[i].error_message_text;
	} else {
		ptr = unknown_error_message;
	}
	return ptr;
}

MX_EXPORT mx_status_type
mx_successful_result( void )
{
	mx_status_type status;

	status.code = MXE_SUCCESS;
	status.location = NULL;

#if USE_STACK_BASED_MX_ERROR

	strcpy( status.message, "" );

#else /* not USE_STACK_BASED_MX_ERROR */

	status.message = NULL;

#endif /* USE_STACK_BASED_MX_ERROR */

	return status;
}
	
MX_EXPORT void
mx_set_error_output_function( void (*output_function)(char *) )
{
	if ( output_function == NULL ) {
		mx_error_output_function = mx_error_default_output_function;
	} else {
		mx_error_output_function = output_function;
	}
	return;
}

MX_EXPORT void
mx_error_default_output_function( char *string )
{
	fprintf( stderr, "%s\n", string );

	return;
}

#if defined( OS_WIN32 )

#include <windows.h>

#ifndef MX_CR
#define MX_CR  0x0d
#define MX_LF  0x0a
#endif

MX_EXPORT long
mx_win32_error_message( long error_code, char *buffer, size_t buffer_length )
{
	long num_chars, null_index;

	num_chars = FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error_code,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) buffer,
			buffer_length-1,
			NULL );

	if ( num_chars >= buffer_length ) {
		null_index = buffer_length - 1;
	} else {
		null_index = num_chars;
	}

	/* Drop the end of line characters if present. */

	if ( (buffer[null_index-2] == MX_CR)
	  && (buffer[null_index-1] == MX_LF) )
	{
		null_index--;
		null_index--;

	} else if ( buffer[null_index-1] == MX_CR ) {
		null_index--;
	}

	buffer[ null_index ] = '\0';

	return num_chars;
}

#endif /* OS_WIN32 */

MX_EXPORT long
mx_errno_to_mx_status_code( int errno_value ) {
	long mx_status_code;

	switch( errno_value ) {
	case EPERM:
	case EACCES:
	case EEXIST:
	case EXDEV:
#ifdef ETXTBSY
	case ETXTBSY:
#endif
		mx_status_code = MXE_PERMISSION_DENIED;
		break;
	case ENOENT:
	case ESRCH:
	case ENXIO:
	case ENODEV:
		mx_status_code = MXE_NOT_FOUND;
		break;
	case EINTR:
		mx_status_code = MXE_INTERRUPTED;
		break;
	case EIO:
		mx_status_code = MXE_INTERFACE_IO_ERROR;
		break;
	case E2BIG:
	case ECHILD:
	case EDOM:
	case ERANGE:
		mx_status_code = MXE_WOULD_EXCEED_LIMIT;
		break;
	case ENOEXEC:
		mx_status_code = MXE_INITIALIZATION_ERROR;
		break;
	case EBADF:
		mx_status_code = MXE_FILE_IO_ERROR;
		break;
	case EAGAIN:
		mx_status_code = MXE_TRY_AGAIN;
		break;
	case ENOMEM:
		mx_status_code = MXE_OUT_OF_MEMORY;
		break;
	case EFAULT:
	case ENOTDIR:
	case EISDIR:
	case EINVAL:
	case ESPIPE:
		mx_status_code = MXE_ILLEGAL_ARGUMENT;
		break;
#ifdef ENOTBLK
	case ENOTBLK:
#endif
	case ENOTTY:
		mx_status_code = MXE_UNSUPPORTED;
		break;
	case EBUSY:
		mx_status_code = MXE_TIMED_OUT;
		break;
	case ENFILE:
	case EMFILE:
	case EFBIG:
	case ENOSPC:
	case EMLINK:
		mx_status_code = MXE_LIMIT_WAS_EXCEEDED;
		break;
	case EROFS:
		mx_status_code = MXE_READ_ONLY;
		break;
	case EPIPE:
		mx_status_code = MXE_IPC_IO_ERROR;
		break;
	default:
		mx_status_code = MXE_OPERATING_SYSTEM_ERROR;
		break;
	}

	return mx_status_code;
}

