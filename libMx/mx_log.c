/*
 * Name:    mx_log.c
 *
 * Purpose: Logfile support.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2003, 2005-2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_record.h"
#include "mx_variable.h"
#include "mx_log.h"

MX_EXPORT mx_status_type
mx_log_open( MX_RECORD *record_list )
{
	const char fname[] = "mx_log_open()";

	MX_LOG *log_handler;
	MX_LIST_HEAD *list_head;
	MX_RECORD *log_record;
	char log_type[20];
	char log_control_string[200];
	char log_name[ MXU_FILENAME_LENGTH + 1 ];
	char buffer_format[80];
	int status, num_items, saved_errno;
	mx_status_type mx_status;

	/* Try to find the "mx_log_type", "mx_log_control", and "mx_log_name"
	 * records.  Logging will not be started unless they all exist.
	 */

	/* A 1-D string variable record named "mx_log" must exist in the
	 * MX database for logging to begin.
	 */

	log_record = mx_get_record( record_list, MX_LOG_RECORD_NAME );

	if ( log_record == NULL ) {
		/* Logging will not be started. */

		return MX_SUCCESSFUL_RESULT;
	}

	/* Get the log control string from the record. */

	mx_status = mx_get_string_variable_by_name(
						record_list, MX_LOG_RECORD_NAME,
						log_control_string,
						sizeof(log_control_string) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* At the moment, only logging to files is supported.  This is so
	 * that I can defer the writing of a more generic log handler until
	 * later.  I plan to add support for syslog(), the NT event logger,
	 * and so forth at a later date.
	 */

	sprintf( buffer_format, "%%%lds %%%lds", (long) sizeof(log_type),
					(long) sizeof(log_control_string) );

	num_items = sscanf( log_control_string, buffer_format,
				log_type, log_name );

	if ( num_items != 2 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Cannot parse the log control string '%s' in record '%s'",
			log_control_string, MX_LOG_RECORD_NAME );
	}

	if ( strcmp( log_type, "file" ) != 0 ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Only logging to files via log type 'file' is currently supported." );
	}

	/* See if we can write to the log file.  We don't actually
	 * try to open it here.  We will do that later in the 
	 * mx_log_message() function.
	 */

	status = access( log_name, W_OK );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case ENOENT:
			return mx_error( MXE_FILE_IO_ERROR, fname,
		"The MX logfile '%s' does not yet exist.  You must create "
		"an empty file with the name '%s' before logging can begin.",
				log_name, log_name );
		default:
			return mx_error( MXE_PERMISSION_DENIED, fname,
				"Cannot write to the MX logfile '%s'.  "
				"Error code = %d, error text = '%s'.",
				log_name, saved_errno,
				strerror( saved_errno ) );
		}
	}

	/* Get the record list head, so that we may setup an MX_LOG
	 * structure there.
	 */

	list_head = mx_get_record_list_head_struct( record_list );

	if ( list_head == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX list_head structure pointer returned was NULL." );
	}

	/* Allocate memory for an MX_LOG structure. */

	log_handler = (MX_LOG *) malloc( sizeof(MX_LOG) );

	if ( log_handler == (MX_LOG *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an MX_LOG structure." );
	}

	/* Set up the structure and put a pointer to it in the MX_LIST_HEAD
	 * structure.
	 */

	log_handler->log_type = MXLT_FILE;

	strlcpy( log_handler->log_name, log_name, MXU_FILENAME_LENGTH + 1 );

	list_head->log_handler = log_handler;

	/* Finally, add a back pointer from the log structure to the
	 * list head structure.
	 */

	log_handler->list_head = list_head;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_log_close( MX_RECORD *record_list )
{
	const char fname[] = "mx_log_close()";

	MX_LIST_HEAD *list_head;

	/* Find the record list head structure so that we can see if
	 * a log file is currently active.
	 */

	list_head = mx_get_record_list_head_struct( record_list );

	if ( list_head == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX list_head structure pointer returned was NULL." );
	}

	/* The log file is active if the 'log_handler' pointer is not NULL. */

	if ( list_head->log_handler != NULL ) {
		free( list_head->log_handler );
	}

	list_head->log_handler = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_log_message( MX_LOG *log_handler, char *message )
{
	const char fname[] = "mx_log_message()";

	char buffer[ MXU_USERNAME_LENGTH + 40 ];
	FILE *file;
	int fd, status, saved_errno, flags;

	/* At present we only handle logging to a file. */

	/* Open the logfile for exclusive access.  Please note that
	 * this is not likely to work correctly for a file accessed
	 * via NFS.
	 */

	flags = O_WRONLY | O_EXCL | O_APPEND;

#if defined(OS_VXWORKS)
	fd = open( log_handler->log_name, flags, S_IRWXU );
#else
	fd = open( log_handler->log_name, flags );
#endif

	if ( fd < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
			"Unable to open MX log file '%s'.  Error code = %d, "
			"error text = '%s'", log_handler->log_name,
			saved_errno, strerror( saved_errno ) );
	}

	file = fdopen( fd, "a" );

	if ( file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
			"Unable to convert the file descriptor for "
			"the MX log file '%s' to a FILE pointer.  "
			"Error code = %d, error text = '%s'",
			log_handler->log_name,
			saved_errno, strerror( saved_errno ) );
	}

	fprintf( file, "%s  %s\n",
		mx_log_timestamp( log_handler, buffer, sizeof(buffer) ),
		message );

	status = fclose( file );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred while closing the MX log file '%s'."
			"  Error code = %d, error text = '%s'",
			log_handler->log_name,
			saved_errno, strerror( saved_errno ) );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT char *
mx_log_timestamp( MX_LOG *log_handler, char *buffer, size_t max_buffer_length )
{
	time_t time_struct;
	struct tm current_time;
	size_t length, buffer_left;
	char *ptr;

	time( &time_struct );

	(void) localtime_r( &time_struct, &current_time );

	strftime( buffer, max_buffer_length,
				"%a %b %d %H:%M:%S %Y  ", &current_time );

	length = strlen( buffer );

	ptr = buffer + length;

	buffer_left = max_buffer_length - length - 1;

	strlcpy( ptr, log_handler->list_head->username, buffer_left );

	return buffer;
}

