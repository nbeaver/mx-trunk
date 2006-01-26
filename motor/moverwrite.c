/*
 * Name:    moverwrite.c
 *
 * Purpose: Check to see if any data files may be overwritten.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "motor.h"
#include "mdialog.h"
#include "mx_variable.h"

#define MOVERWRITE_UNEXPECTED_ERROR	3
#define MOVERWRITE_READONLY_FILE_EXISTS	2
#define MOVERWRITE_FILE_EXISTS		1
#define MOVERWRITE_FILE_DOES_NOT_EXIST	0

static int
motor_test_datafile_name( char *datafile_name )
{
	int status, saved_errno;

	/* The following tests are subject to race conditions. */

	status = access( datafile_name, F_OK );

	saved_errno = errno;

	if ( status != 0 ) {
		switch( saved_errno ) {
		case ENOENT:
			/* The file does not exist, so we return success. */

			return MOVERWRITE_FILE_DOES_NOT_EXIST;
			break;
		default:
			fprintf( output,
"scan: Unexpected error with filename '%s'.  Errno = %d, error text = '%s'\n",
				datafile_name, saved_errno,
				strerror( saved_errno ) );
			return MOVERWRITE_UNEXPECTED_ERROR;
			break;
		}
	}

	/* If we got here, the file exists.  Do we have write permission? */

	status = access( datafile_name, W_OK );

	saved_errno = errno;

	if ( status != 0 ) {
		switch( saved_errno ) {
		case EACCES:
			fprintf( output,
"scan: The output datafile named '%s' already exists\n",
				datafile_name );
			fprintf( output,
		"      and is write protected.  Aborting the scan.\n" );
			return MOVERWRITE_READONLY_FILE_EXISTS;
			break;
		default:
			fprintf( output,
"scan: Unexpected write permission error with filename '%s'.  "
"Errno = %d, error text = '%s'\n",
			datafile_name, saved_errno,
			strerror( saved_errno ) );
			return MOVERWRITE_UNEXPECTED_ERROR;
			break;
		}
	}

	/* If we got here, the file exists and is writeable. */

	fprintf( output,
		"scan: The output datafile named '%s' already exists.\n",
		datafile_name );

	return MOVERWRITE_FILE_EXISTS;
}

int
motor_check_for_datafile_name_collision( MX_SCAN *scan )
{
	char *version_number_ptr;
	char buffer[20];
	char filename[ MXU_FILENAME_LENGTH + 1 ];
	int status, string_length, exit_loop;
	size_t buffer_left;
	long version_number, version_number_length;
	long i, new_version_number, num_existing_files;
	mx_status_type mx_status;

	/* Check to see if this test has been turned off. */

	if ( motor_overwrite_on == TRUE )
		return SUCCESS;

	/* MXDF_NONE scans do not have a datafile. */

	if ( scan->datafile.type == MXDF_NONE )
		return SUCCESS;

	strlcpy( filename, scan->datafile.filename, MXU_FILENAME_LENGTH );

	mx_status = mx_parse_datafile_name( filename, &version_number_ptr,
				&version_number, &version_number_length );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	num_existing_files = 0;

	if ( scan->num_scans <= 0 ) {

		/* CASE: Illegal number of scans */

		fprintf( output,
			"Scan '%s' has an illegal number of scans (%ld).\n",
			scan->record->name, scan->num_scans );

		return FAILURE;

	} else if ( scan->num_scans == 1 ) {

		/* CASE: Single scan */

		status = motor_test_datafile_name( filename );

		switch( status ) {
		case MOVERWRITE_READONLY_FILE_EXISTS:
			return FAILURE;
			break;
		case MOVERWRITE_FILE_EXISTS:
			num_existing_files++;
			break;
		case MOVERWRITE_FILE_DOES_NOT_EXIST:
			break;
		default:
			return FAILURE;
			break;
		}
	} else {
		/* CASE: Multiple scans */

		if ( version_number_ptr == NULL ) {

			/* CASE: Single filename */

			fprintf( output, "\a" );

			fprintf( output,
"The output filename '%s' for this scan does not contain\n",
				filename );
			fprintf( output,
"a version number.  Since this scan is to be run %ld times in a row,\n",
				scan->num_scans );
			fprintf( output,
"this means that the output of later passes will overwrite the output\n"
"from earlier passes.\n" );

			exit_loop = FALSE;

			while( exit_loop == FALSE ) {

				string_length = sizeof(buffer) - 1;

				status = motor_get_string( output,
					" Proceed anyway (Y/N) ? ",
					NULL, &string_length, buffer );

				if ( status != FAILURE ) {
					switch( buffer[0] ) {
					case 'Y':
					case 'y':
						exit_loop = TRUE;
						break;
					case 'N':
					case 'n':
					case 'Q':
					case 'q':
						fprintf( output,
							"\nScan aborted.\n\n");
						return FAILURE;
						break;
					default:
						fprintf( output,
		"\nPlease enter either Y or N or Q to abort the scan.\n\n" );
						break;
					}
				}
			}

			status = motor_test_datafile_name( filename );

			switch( status ) {
			case MOVERWRITE_READONLY_FILE_EXISTS:
				return FAILURE;
				break;
			case MOVERWRITE_FILE_EXISTS:
				num_existing_files++;
				break;
			case MOVERWRITE_FILE_DOES_NOT_EXIST:
				break;
			default:
				return FAILURE;
				break;
			}
		} else {
			/* CASE: Multiple filenames. */

			buffer_left = sizeof(filename)
			    - (size_t) (version_number_ptr - filename);

			for ( i = 0; i < scan->num_scans; i++ ) {

				new_version_number
				    = mx_construct_datafile_version_number(
					version_number + i,
					version_number_length );

				if ( new_version_number < 0 )
					return FAILURE;

				snprintf( version_number_ptr, buffer_left,
					"%0*ld",
					(int) version_number_length,
					new_version_number );

				status = motor_test_datafile_name( filename );

				switch( status ) {
				case MOVERWRITE_READONLY_FILE_EXISTS:
					return FAILURE;
					break;
				case MOVERWRITE_FILE_EXISTS:
					num_existing_files++;
					break;
				case MOVERWRITE_FILE_DOES_NOT_EXIST:
					break;
				default:
					return FAILURE;
					break;
				}
			}
		}
	}

	if ( num_existing_files <= 0 )
		return SUCCESS;

	/* If we got here, one or more files exist and are writeable. */

	fprintf( output, "\a" );

	while( 1 ) {

		string_length = sizeof(buffer) - 1;

		if ( num_existing_files == 1 ) {
			status = motor_get_string( output,
				" Overwrite it (Y/N) ? ",
				NULL, &string_length, buffer );
		} else {
			status = motor_get_string( output,
				" Overwrite them (Y/N) ? ",
				NULL, &string_length, buffer );
		}

		if ( status != FAILURE ) {
			switch( buffer[0] ) {
			case 'Y':
			case 'y':
				/* The user has requested that we overwrite
				 * the datafile, so we return SUCCESS.
				 */
				return SUCCESS;
				break;
			case 'N':
			case 'n':
				if ( num_existing_files == 1 ) {
					fprintf( output,
	"\nThe datafile will not be overwritten.  Aborting the scan.\n\n" );
				} else {
					fprintf( output,
	"\nThe datafiles will not be overwritten.  Aborting the scan.\n\n" );
				}
				return FAILURE;
				break;
			case 'Q':
			case 'q':
				fprintf( output, "\nScan aborted.\n\n" );
				return FAILURE;
				break;
			default:
				fprintf( output,
	"\nPlease enter either Y or N or Q to abort the scan.\n\n" );
				break;
			}
		}
	}
}

