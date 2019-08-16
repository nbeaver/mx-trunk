/*
 * Name:    mx_scan_mca.c
 *
 * Purpose: MCA support for MX scans.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2009, 2011, 2013, 2015-2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_SCAN_MCA_DEBUG_GET_DIRECTORY_AND_FILENAME	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_key.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_scan.h"
#include "mx_cfn.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "mx_mca.h"

#include "f_sff.h"
#include "f_xafs.h"

#define NUMBER_STRING_LENGTH	40

MX_EXPORT mx_status_type
mx_scan_get_directory_and_filename( MX_SCAN *scan,
				MX_RECORD *input_device,
				long input_device_class,
				char *directory_name,
				size_t max_dirname_length,
				char *filename,
				size_t max_filename_length )
{
	static const char fname[] = "mx_scan_get_directory_and_filename()";

	MX_AREA_DETECTOR *ad;
	char format_name[ MXU_IMAGE_FORMAT_NAME_LENGTH + 1 ];
	char canonical_pathname_copy[ MXU_FILENAME_LENGTH + 1 ];
	char current_directory_name[ MXU_FILENAME_LENGTH + 1 ];
	char *datafile_pathname = NULL;
	char *datafile_dirname_ptr = NULL;
	char *datafile_filename_ptr = NULL;
	char *separator_ptr = NULL;
	char *extension_ptr = NULL;
	long *number_ptr = NULL;
	long measurement_number;
	mx_bool_type datafile_dirname_is_absolute_pathname;
	int i, c;
	char datafile_basename[ MXU_FILENAME_LENGTH + 1 ];
	char suffix_string[NUMBER_STRING_LENGTH+1];
	ptrdiff_t basename_length;
	mx_status_type mx_status;

	if ( input_device != NULL ) {
		input_device_class = input_device->mx_class;
	}

	mx_status = mx_scan_get_pointer_to_measurement_number(
						scan, &number_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	measurement_number = *number_ptr;

	/* Get the current directory name, since we may need it later. */

	mx_status = mx_get_current_directory_name( current_directory_name,
					sizeof(current_directory_name) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get a pointer to the datafile pathname. */

	mx_status = mx_scan_get_pointer_to_datafile_pathname(
						scan, &datafile_pathname );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}

	/* Canonicalize a local copy of the datafile pathname. */

	mx_status = mx_canonicalize_filename( datafile_pathname,
					canonical_pathname_copy,
					sizeof(canonical_pathname_copy) );

#if MX_SCAN_MCA_DEBUG_GET_DIRECTORY_AND_FILENAME
	MX_DEBUG(-2,("%s: datafile_pathname = '%s'", fname, datafile_pathname));
	MX_DEBUG(-2,("%s: canonical_pathname_copy = '%s'",
			fname, canonical_pathname_copy ));
#endif

	/* Split the canonical filename into the directory name and
	 * the filename.
	 */

#if defined(OS_WIN32) || defined(OS_MSDOS)
	separator_ptr = strrchr( canonical_pathname_copy, '\\' );

	if ( separator_ptr == NULL ) {
		separator_ptr = strrchr( canonical_pathname_copy, '/' );
	}
#else
	separator_ptr = strrchr( canonical_pathname_copy, '/' );
#endif

	if ( separator_ptr == NULL ) {
		datafile_dirname_ptr = NULL;
		datafile_filename_ptr = canonical_pathname_copy;
	} else {
		*separator_ptr = '\0';
		datafile_dirname_ptr = canonical_pathname_copy;
		datafile_filename_ptr = separator_ptr + 1;
	}

	/* Is the directory name an absolute pathname? */

	if ( datafile_dirname_ptr == NULL ) {
		datafile_dirname_is_absolute_pathname = FALSE;
	} else {
		datafile_dirname_is_absolute_pathname =
			mx_is_absolute_filename( datafile_dirname_ptr );
	}

	/* Construct the name of the scan subdirectory based on the scan
	 * datafile name.
	 */

	extension_ptr = strrchr( datafile_filename_ptr, '.' );

	if ( extension_ptr == NULL ) {

		/* The scan datafile name has no extension so we just
		 * append the measurement number.
		 */

		basename_length = sizeof(datafile_basename);

		switch( input_device_class ) {
		case MXC_MULTICHANNEL_ANALYZER:
			strlcpy( suffix_string, "mca",
					sizeof(suffix_string) );
			break;
		case MXC_AREA_DETECTOR:
			strlcpy( suffix_string, "ad",
					sizeof(suffix_string) );
			break;
		default:
			strlcpy( suffix_string, "dev",
					sizeof(suffix_string) );
			break;
		}
	} else {
		basename_length = extension_ptr - datafile_filename_ptr + 1;

		if ( basename_length > sizeof(datafile_basename) ) {
			return mx_error( MXE_FUNCTION_FAILED, fname,
"basename_length '%ld' is greater than the maximum length '%ld'.  "
"This shouldn't be possible, so this is a bug if you see this message.",
				(long) basename_length,
				(long) sizeof(datafile_basename) );
		}

		strlcpy( suffix_string, extension_ptr + 1,
				sizeof(suffix_string) );
	}

	strlcpy( datafile_basename, datafile_filename_ptr, basename_length );

#if MX_SCAN_MCA_DEBUG_GET_DIRECTORY_AND_FILENAME
	if ( datafile_dirname_ptr == NULL ) {
		MX_DEBUG(-2,("%s: datafile_dirname_ptr = %p",
		fname, datafile_dirname_ptr));
	} else {
		MX_DEBUG(-2,("%s: datafile_dirname_ptr = '%s'",
		fname, datafile_dirname_ptr));
	}
	MX_DEBUG(-2,("%s: datafile_filename_ptr = '%s'",
		fname, datafile_filename_ptr));
	MX_DEBUG(-2,("%s: datafile_basename = '%s'",
		fname, datafile_basename));
	MX_DEBUG(-2,("%s: suffix_string = '%s'",
		fname, suffix_string));
#endif

	if ( datafile_dirname_is_absolute_pathname ) {
		snprintf( directory_name, max_dirname_length,
		    "%s/%s_%s",
			datafile_dirname_ptr,
			datafile_basename,
			suffix_string );
	} else {
		if ( datafile_dirname_ptr == NULL ) {
			snprintf( directory_name, max_dirname_length,
			"%s/%s_%s", 
			    current_directory_name,
			    datafile_basename,
			    suffix_string );
		} else {
			snprintf( directory_name, max_dirname_length,
			"%s/%s/%s_%s",
			    current_directory_name,
			    datafile_dirname_ptr,
			    datafile_basename,
			    suffix_string );
		}
	}

	switch( input_device_class ) {
	case MXC_MULTICHANNEL_ANALYZER:
		snprintf( filename, max_filename_length,
			"%s.%03ld",
			datafile_filename_ptr, measurement_number );
		break;
	case MXC_AREA_DETECTOR:
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

		snprintf( filename, max_filename_length, "%s_%s_%03ld.%s",
			datafile_filename_ptr,
			input_device->name,
			measurement_number,
			format_name );
		break;
	default:
		if ( input_device != NULL ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Input device '%s' is not an MCA or area detector.",
				input_device->name );
		} else {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		    "Input device class (%ld) is not an MCA or area detector.",
		    		input_device_class );
		}
		break;
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
	char pathname[2*MXU_FILENAME_LENGTH + NUMBER_STRING_LENGTH + 100];
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

	mx_status = mx_scan_get_directory_and_filename( scan,
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

#if MX_SCAN_MCA_DEBUG_GET_DIRECTORY_AND_FILENAME
	MX_DEBUG(-2,("%s: mca_directory_name = '%s'",
		fname, mca_directory_name));
	MX_DEBUG(-2,("%s: mca_filename = '%s'", fname, mca_filename));
#endif

	/* Open the MCA datafile. */

	if ( use_subdirectory ) {

		mx_status = mx_verify_directory( mca_directory_name, TRUE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		snprintf( pathname, sizeof(pathname),
			"%s/%s", mca_directory_name, mca_filename );
	} else {
		strlcpy( pathname, mca_filename, sizeof(pathname) );
	}

#if MX_SCAN_MCA_DEBUG_GET_DIRECTORY_AND_FILENAME
	MX_DEBUG(-2,("%s: pathname = '%s'", fname, pathname ));
#endif

	savefile = fopen( pathname, "w" );

	if ( savefile == NULL ) {
		saved_errno = errno;

		mx_free( mca_array );

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Could not open MCA datafile '%s'.  Reason = '%s'",
			pathname, strerror(saved_errno) );
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
	char image_pathname[2*MXU_FILENAME_LENGTH + 3];
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

	/* If MX controls the area detector via an external control system
	 * and does not have direct access to the data, then there is no
	 * local copy of the image data to save, so we return without
	 * doing anything further.
	 */

	if ( ad->image_data_available == FALSE ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Construct the name of the area detector image file based on
	 * the scan datafile name.
	 */

	mx_status = mx_scan_get_directory_and_filename( scan,
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

	MX_DEBUG(-2,("%s: image_directory_name = '%s'",
		fname, image_directory_name));
	MX_DEBUG(-2,("%s: image_filename = '%s'", fname, image_filename));

	MX_DEBUG(-2,("%s: image_pathname = '%s'", fname, image_pathname ));

	/* Write the image to a file. */

	mx_status = mx_image_write_file( ad->image_frame, NULL,
					ad->datafile_save_format,
					image_pathname );

	return mx_status;
}

