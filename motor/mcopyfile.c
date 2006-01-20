/*
 * Name:    mcopyfile.c
 *
 * Purpose: A simple routine that does a binary copy of a file.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>

#include "motor.h"

extern mx_status_type
motor_copy_file( char *source_filename, char *destination_filename )
{
	static char fname[] = "motor_copy_file()";

	FILE *source_file, *destination_file;
	char buffer[1024];
	int chars_read, chars_written;

	if ( source_filename == NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Source filename pointer is NULL.");
	}

	if ( destination_filename == NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Destination filename pointer is NULL.");
	}

	/* If the source file does not exist, do not attempt to make
	 * a backup.
	 */

	if ( access( source_filename, F_OK ) != 0 )
		return MX_SUCCESSFUL_RESULT;

	/* Try to open the source file. */

	source_file = fopen( source_filename, "rb" );

	if ( source_file == NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
			"The source file '%s' is read protected.",
			source_filename );
	}

	destination_file = fopen( destination_filename, "wb" );

	if ( destination_file == NULL ) {
		fclose( source_file );

		return mx_error( MXE_FILE_IO_ERROR, fname,
	"Destination file '%s' cannot be opened for write access.",
			destination_filename );
	}

	while ( 1 ) {
		chars_read = fread( buffer, 1, sizeof buffer, source_file );

		if ( chars_read <= 0 ) {
			break;		/* exit the while() loop */
		}

		chars_written = fwrite( buffer,
					1, chars_read, destination_file );

		if ( chars_written <= 0 ) {
			fclose( source_file );
			fclose( destination_file );

			return mx_error( MXE_FILE_IO_ERROR, fname,
		"Error occurred while writing to destination file '%s'",
				destination_filename );
		}

		if ( chars_written < chars_read ) {
			fclose( source_file );
			fclose( destination_file );

			return mx_error( MXE_FILE_IO_ERROR, fname,
	"Incomplete write occurred while writing to destination file '%s'",
				destination_filename );
		}
	}

	fclose( source_file );
	fclose( destination_file );

	return MX_SUCCESSFUL_RESULT;
}

#define BACKUP_SUFFIX	".bak"

extern mx_status_type
motor_make_backup_copy( char *filename )
{
	static char fname[] = "motor_make_backup_copy()";

	char backup_filename[512];
	char path_separator;
	char *last_period_in_filename;
	char *last_path_separator_in_filename;
	int length, ptr_diff;
	mx_status_type status;

	if ( filename == NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Source filename pointer is NULL.");
	}

	strncpy( backup_filename, filename,
		sizeof( backup_filename ) - sizeof(BACKUP_SUFFIX) );

	last_period_in_filename = strrchr( backup_filename, '.' );

	if ( last_period_in_filename == NULL ) {
		length = strlen( backup_filename );

		strcpy( backup_filename + length, BACKUP_SUFFIX );
	} else {

#if defined( OS_WIN32) || defined( OS_DOS_EXT_WATCOM)
		path_separator = '\\';
#else
		path_separator = '/';
#endif
		last_path_separator_in_filename
			= strrchr( backup_filename, path_separator );

		if ( last_path_separator_in_filename == NULL ) {
			strcpy( last_period_in_filename, BACKUP_SUFFIX );
		} else {
			ptr_diff = (int) ( last_period_in_filename
					- last_path_separator_in_filename );

			if ( ptr_diff > 0 ) {
				strcpy( last_period_in_filename,
							BACKUP_SUFFIX );
			} else {
				length = strlen( backup_filename );

				strcpy( backup_filename + length,
							BACKUP_SUFFIX );
			}
		}
	}

	status = motor_copy_file( filename, backup_filename );

	return status;
}

