/*
 * Name:    mx_scan_mca.c
 *
 * Purpose: MCA support for MX scans.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2009, 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_key.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_scan.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "mx_mca.h"

#include "f_sff.h"
#include "f_xafs.h"

#define NUMBER_STRING_LENGTH	40

MX_EXPORT mx_status_type
mx_scan_get_subdirectory_and_filename( MX_SCAN *scan,
				MX_RECORD *input_device,
				long input_device_class,
				char *subdirectory_name,
				size_t max_dirname_length,
				char *filename,
				size_t max_filename_length )
{
	static const char fname[] = "mx_scan_get_subdirectory_and_filename()";

	MX_AREA_DETECTOR *ad;
	char format_name[ MXU_IMAGE_FORMAT_NAME_LENGTH + 1 ];
	char *datafile_filename = NULL;
	char *extension_ptr = NULL;
	long *number_ptr = NULL;
	long measurement_number, format;
	mx_bool_type use_subdirectory;
	int i, c;
	char number_string[NUMBER_STRING_LENGTH+1];
	ptrdiff_t basename_length;
	mx_status_type mx_status;

	use_subdirectory = TRUE;

	if ( input_device != NULL ) {
		input_device_class = input_device->mx_class;
	}

	mx_status = mx_scan_get_pointer_to_measurement_number(
						scan, &number_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	measurement_number = *number_ptr;

	sprintf( number_string, "%03ld", measurement_number );

	/* Construct the name of the scan subdirectory based on the scan
	 * datafile name.
	 */

	mx_status = mx_scan_get_pointer_to_datafile_filename(
						scan, &datafile_filename );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}

	extension_ptr = strrchr( datafile_filename, '.' );

	if ( extension_ptr == NULL ) {

		/* The scan datafile name has no extension so we just
		 * append the measurement number.
		 */

		if ( use_subdirectory ) {
			switch( input_device->mx_class ) {
			case MXC_MULTICHANNEL_ANALYZER:
				snprintf( subdirectory_name, max_dirname_length,
					"%s_mca", datafile_filename );
				break;
			case MXC_AREA_DETECTOR:
				snprintf( subdirectory_name, max_dirname_length,
					"%s_ad", datafile_filename );
				break;
			default:
				snprintf( subdirectory_name, max_dirname_length,
					"%s_dev", datafile_filename );
				break;
			}
		}

		strlcpy( filename, datafile_filename, max_filename_length );
	} else {
		basename_length = extension_ptr - datafile_filename + 1;

		if ( basename_length > max_filename_length ) {
			return mx_error( MXE_FUNCTION_FAILED, fname,
"basename_length '%ld' is greater than the maximum length '%ld'.  "
"This shouldn't be possible, so this is a bug if you see this message.",
				(long) basename_length,
				(long) max_filename_length );
		}

		strlcpy( filename, datafile_filename, basename_length );

		strlcat( filename, "_", max_filename_length );

		extension_ptr++;

		strlcat( filename, extension_ptr, max_filename_length );

		if ( use_subdirectory ) {
			strlcpy( subdirectory_name, filename,
					max_dirname_length );
		}
	}

	if ( input_device_class != MXC_AREA_DETECTOR ) {
		strlcat( filename, ".", max_filename_length );

		strlcat( filename, number_string, max_filename_length );
	} else {
		if ( input_device == (MX_RECORD *) NULL ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"For area detector measurements, a pointer to "
			"the area detector record must be passed as "
			"the input device pointer." );
		}

		ad = input_device->record_class_struct;

		if ( ad == (MX_AREA_DETECTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_AREA_DETECTOR pointer for area detetor '%s' "
			"is NULL.", input_device->name );
		}

		strlcat( filename, "_", max_filename_length );

		strlcat( filename, input_device->name, max_filename_length );

		strlcat( filename, "_", max_filename_length );

		strlcat( filename, number_string, max_filename_length );

		strlcat( filename, ".", max_filename_length );

		if ( ad->datafile_save_format == 0 ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
			"The datafile save format "
			"has not been set for area detector '%s'.",
				ad->record->name );
		}

		mx_status = mx_image_get_file_format_name_from_type(
						ad->datafile_save_format,
						format_name,
						sizeof(format_name) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Convert the format string to lower case. */

		for ( i = 0; i < strlen(format_name); i++ ) {
			c = format_name[i];

			if ( isupper(c) ) {
				format_name[i] = tolower(c);
			}
		}

		strlcat( filename, format_name, max_filename_length );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------*/

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
	long i, j, mca_number;
	unsigned long mca_data_value, max_current_num_channels;
	char mca_filename[MXU_FILENAME_LENGTH + NUMBER_STRING_LENGTH + 1];
	char mca_directory_name[MXU_FILENAME_LENGTH + NUMBER_STRING_LENGTH + 1];
	mx_bool_type use_subdirectory;
	mx_status_type mx_status;

	if ( num_mcas <= 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	use_subdirectory = TRUE;

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

	mx_status = mx_scan_get_subdirectory_and_filename( scan,
						NULL,
						MXC_MULTICHANNEL_ANALYZER,
						mca_directory_name,
						sizeof(mca_directory_name),
						mca_filename,
						sizeof(mca_filename) );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_free( mca_array );
		return mx_status;
	}

	/* Open the MCA datafile. */

	if ( use_subdirectory ) {
		char pathname[MXU_FILENAME_LENGTH + NUMBER_STRING_LENGTH + 1];

		mx_status = mx_verify_directory( mca_directory_name, TRUE );

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

/*--------*/

MX_EXPORT mx_status_type
mx_scan_save_area_detector_image( MX_SCAN *scan,
				MX_RECORD *ad_record )
{
	static const char fname[] = "mx_scan_save_area_detector_image()";

	MX_AREA_DETECTOR *ad;
	char image_directory_name[ MXU_FILENAME_LENGTH + 1 ];
	char image_filename[ MXU_FILENAME_LENGTH + 1 ];
	char image_pathname[MXU_FILENAME_LENGTH + 1];
	mx_bool_type use_subdirectory = TRUE;
	mx_status_type mx_status;

	ad = (MX_AREA_DETECTOR *) ad_record->record_class_struct;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AREA_DETECTOR pointer for area detector '%s' is NULL.",
			ad_record->name );
	}

	if ( ad->transfer_image_during_scan == FALSE ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Construct the name of the area detector image file based on
	 * the scan datafile name.
	 */

	mx_status = mx_scan_get_subdirectory_and_filename( scan,
						ad_record,
						MXC_AREA_DETECTOR,
						image_directory_name,
						sizeof(image_directory_name),
						image_filename,
						sizeof(image_filename) );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( use_subdirectory ) {
		mx_status = mx_verify_directory( image_directory_name, TRUE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		snprintf( image_pathname, sizeof(image_pathname),
			"%s/%s", image_directory_name, image_filename );
	} else {
		strlcpy( image_pathname,
			image_filename, sizeof(image_pathname) );
	}

	/* Write the image to a file. */

	mx_status = mx_image_write_file( ad->image_frame,
					ad->datafile_save_format,
					image_pathname );

	return mx_status;
}

