/*
 * Name:    mx_scan_mca.c
 *
 * Purpose: MCA support for MX scans.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_key.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_scan.h"
#include "mx_mca.h"

#include "f_sff.h"
#include "f_xafs.h"

/* --------------- */

static mx_status_type
mxp_scan_verify_mca_subdirectory( const char *mca_directory_name )
{
	static const char fname[] = "mxp_scan_verify_mca_subdirectory()";

	struct stat stat_buf;
	int os_status, saved_errno;

	if ( mca_directory_name == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The directory name pointer passed was NULL." );
	}

	/* Does a filesystem object with this name already exist? */

	os_status = access( mca_directory_name, F_OK );

	if ( os_status != 0 ) {
		saved_errno = errno;

		if ( saved_errno != ENOENT ) {
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred while testing for the presence of "
			"MCA directory '%s'.  "
			"Errno = %d, error message = '%s'",
				mca_directory_name,
				saved_errno, strerror( saved_errno ) );
		}

		/* The directory does not already exist, so create it. */

		os_status = mkdir( mca_directory_name, 0777 );

		if ( os_status == 0 ) {
			/* Creating the directory succeeded, so we are done! */

			return MX_SUCCESSFUL_RESULT;
		} else {
			/* Creating the directory failed, so report the error.*/

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Creating MCA subdirectory '%s' failed.  "
			"Errno = %d, error message = '%s'",
				mca_directory_name,
				saved_errno, strerror(saved_errno) );
		}
	}

	/*------------*/

	/* A filesystem object with this name already exists, so 
	 * we must find out more about it.
	 */

	os_status = stat( mca_directory_name, &stat_buf );

	if ( os_status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"stat() failed for file '%s'.  "
		"Errno = %d, error message = '%s'",
			mca_directory_name,
			saved_errno, strerror(saved_errno) );
	}

	/* Is the object a directory? */

	if ( S_ISDIR(stat_buf.st_mode) == 0 ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Existing file '%s' is not a directory.",
			mca_directory_name );
	}

	/* Although we just did stat(), determining the access permissions
	 * is more portably done with access().
	 */

	os_status = access( mca_directory_name, R_OK | W_OK | X_OK );

	if ( os_status != 0 ) {
		saved_errno = errno;

		if ( saved_errno == EACCES ) {
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"We do not have read, write, and execute permission "
			"for MCA directory '%s'.", mca_directory_name );
		}

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Checking the access permissions for MCA directory '%s' "
		"failed.  Errno = %d, error message = '%s'",
			mca_directory_name,
			saved_errno, strerror( saved_errno ) );
	}

	/* If we get here, the MCA directory already exists and is useable. */

	return MX_SUCCESSFUL_RESULT;
}

#define NUMBER_STRING_LENGTH	40

MX_EXPORT mx_status_type
mx_scan_save_mca_measurements( MX_SCAN *scan, long num_mcas )
{
	static const char fname[] = "mx_scan_save_mca_measurements()";

	FILE *savefile;
	MX_MCA **mca_array;
	MX_MCA *mca;
	MX_RECORD *input_device;
	MX_SCAN *parent_scan;
	int os_status, saved_errno, exit_i_loop;
	long *number_ptr = NULL;
	long i, j, measurement_number, mca_number;
	unsigned long mca_data_value, max_current_num_channels;
	char *datafile_filename = NULL;
	char *extension_ptr = NULL;
	char number_string[NUMBER_STRING_LENGTH + 1];
	char mca_filename[MXU_FILENAME_LENGTH + NUMBER_STRING_LENGTH + 1];
	char mca_directory_name[MXU_FILENAME_LENGTH + NUMBER_STRING_LENGTH + 1];
	mx_bool_type use_subdirectory;
	ptrdiff_t basename_length;
	mx_status_type mx_status;

	if ( num_mcas <= 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	use_subdirectory = TRUE;

	mx_status = mx_scan_get_pointer_to_measurement_number(
						scan, &number_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	measurement_number = *number_ptr;

	sprintf( number_string, "%03ld", measurement_number );

	/* mx_mca_read() will already have been invoked during the
	 * measurement process for each MCA, so all we have to do
	 * here is get pointers to the MCA structures and use the
	 * values that are already there.
	 */

	mca_array = (MX_MCA **) malloc( num_mcas * sizeof(MX_MCA *) );

	if ( mca_array == (MX_MCA **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate a %ld element array of MX_MCA pointers.",
			num_mcas );
	}

	mca_number = 0;

	for ( i = 0; i < scan->num_input_devices; i++ ) {

		input_device = (scan->input_device_array)[i];

		if ( input_device->mx_class == MXC_MULTICHANNEL_ANALYZER ) {

			mca_array[ mca_number ] = (MX_MCA *)
					input_device->record_class_struct;

			if ( mca_array[ mca_number ] == (MX_MCA *) NULL ) {
				mx_free( mca_array );

				return mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname,
					"MX_MCA pointer for MCA record '%s' "
					"is NULL.", input_device->name );
			}
			mca_number++;
		}
	}

	/* Construct the name of the MCA datafile based on the scan
	 * datafile name.
	 */

	mx_status = mx_scan_get_pointer_to_datafile_filename(
						scan, &datafile_filename );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_free( mca_array );
		return mx_status;
	}

	extension_ptr = strrchr( datafile_filename, '.' );

	if ( extension_ptr == NULL ) {

		/* The scan datafile name has no extension so we just
		 * append the measurement number.
		 */

		if ( use_subdirectory ) {
			snprintf(mca_directory_name, sizeof(mca_directory_name),
			"%s_mca", datafile_filename );
		}

		strlcpy( mca_filename,
			datafile_filename, sizeof(mca_filename) );

		strlcat( mca_filename, ".", sizeof(mca_filename) );

		strlcat( mca_filename,
			number_string, sizeof(mca_filename) );
	} else {
		basename_length = extension_ptr - datafile_filename + 1;

		if ( basename_length > sizeof(mca_filename) ) {
			mx_free( mca_array );

			return mx_error( MXE_FUNCTION_FAILED, fname,
"basename_length '%ld' is greater than sizeof(mca_filename) '%ld'.  "
"This shouldn't be possible, so this is a bug if you see this message.",
				(long) basename_length,
				(long) sizeof(mca_filename) );
		}

		strlcpy( mca_filename,
			datafile_filename, basename_length );

		strlcat( mca_filename, "_", sizeof(mca_filename) );

		extension_ptr++;

		strlcat( mca_filename, extension_ptr, sizeof(mca_filename) );

		if ( use_subdirectory ) {
			strlcpy( mca_directory_name, mca_filename,
				sizeof(mca_directory_name) );
		}

		strlcat( mca_filename, ".", sizeof(mca_filename) );

		strlcat( mca_filename, number_string, sizeof(mca_filename) );
	}

	/* Open the MCA datafile. */

	if ( use_subdirectory ) {
		char pathname[MXU_FILENAME_LENGTH + NUMBER_STRING_LENGTH + 1];

		mx_status = mxp_scan_verify_mca_subdirectory(
						mca_directory_name );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		snprintf( pathname, sizeof(pathname),
			"%s/%s", mca_directory_name, mca_filename );

		savefile = fopen( pathname, "w" );
	} else {
		savefile = fopen( mca_filename, "w" );
	}

	if ( savefile == NULL ) {
		saved_errno = errno;

		mx_free( mca_array );

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Could not open MCA datafile '%s'.  Reason = '%s'",
			mca_filename, strerror(saved_errno) );
	}

	/* If requested, write an MCA file header. */

	switch( scan->datafile.type ) {
	case MXDF_SFF:
		mx_status = mxdf_sff_write_header( &(scan->datafile),
					savefile, MX_SFF_MCA_HEADER );

		break;
	case MXDF_XAFS:
		mx_status = mxdf_xafs_write_header( &(scan->datafile),
					savefile, TRUE );

		break;
	case MXDF_CHILD:
		mx_status = mx_scan_find_parent_scan( scan, &parent_scan );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( parent_scan->datafile.type ) {
		case MXDF_SFF:
			mx_status = mxdf_sff_write_header( &(scan->datafile),
					savefile, MX_SFF_MCA_HEADER );

			break;
		case MXDF_XAFS:
			mx_status = mxdf_xafs_write_header( &(scan->datafile),
					savefile, TRUE );

			break;
		}
		break;
	default:
		mx_status = MX_SUCCESSFUL_RESULT;
		break;
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find the largest value of 'mca->current_num_channels' over
	 * all of the MCAs.
	 */

	max_current_num_channels = 0;

	for ( i = 0; i < num_mcas; i++ ) {
		mca = mca_array[i];

		if ( mca->current_num_channels > max_current_num_channels )
			max_current_num_channels = mca->current_num_channels;
	}

	/* Loop until all of the MCAs have run out of channels. */

	exit_i_loop = FALSE;

	for ( i = 0; i < max_current_num_channels; i++ ) {

		for ( j = 0; j < num_mcas; j++ ) {

			mca = mca_array[j];

			if ( mca->current_num_channels <= i ) {
				mca_data_value = 0;
			} else {
				mca_data_value = mca->channel_array[i];
			}

			fprintf( savefile, "%10lu  ", mca_data_value );

			if ( feof(savefile) || ferror(savefile) ) {
				saved_errno = errno;

				(void) mx_error( MXE_FILE_IO_ERROR, fname,
				"Error writing to MCA datafile '%s' for "
				"channel %ld of MCA '%s'.  Reason = '%s'",
					mca_filename, i, mca->record->name,
					strerror(saved_errno));

				/* Force the for( i ) loop to exit. */

				exit_i_loop = TRUE;

				break;	/* Exit the for( j ) loop. */
			}
		}
		if ( exit_i_loop ) {
			break;	/* Exit the for( i ) loop. */
		}

		fprintf( savefile, "\n" );

		if ( feof(savefile) || ferror(savefile) ) {
			saved_errno = errno;

			(void) mx_error( MXE_FILE_IO_ERROR, fname,
			"Error writing line terminator to MCA datafile '%s' "
			"for channel %ld'.  Reason = '%s'",
				mca_filename, i,
				strerror(saved_errno));

			break;	/* Exit the for( i ) loop. */
		}
	}

	mx_free( mca_array );

	os_status = fclose( savefile );

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
			"Unable to close MCA datafile '%s'.  Reason = '%s'",
				mca_filename, strerror(saved_errno) );
	}

	return MX_SUCCESSFUL_RESULT;
}

