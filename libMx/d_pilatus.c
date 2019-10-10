/*
 * Name:    d_pilatus.c
 *
 * Purpose: MX area detector driver for Dectris Pilatus detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015-2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PILATUS_DEBUG				FALSE

#define MXD_PILATUS_DEBUG_UPDATE_DATAFILE_NUMBER	FALSE

#define MXD_PILATUS_DEBUG_DIRECTORY_MAPPING		FALSE

#define MXD_PILATUS_DEBUG_SERIAL			FALSE

#define MXD_PILATUS_DEBUG_EXTENDED_STATUS		FALSE

#define MXD_PILATUS_DEBUG_EXTENDED_STATUS_CHANGE	FALSE

#define MXD_PILATUS_DEBUG_EXTENDED_STATUS_PARSING	FALSE

#define MXD_PILATUS_DEBUG_PARAMETERS			FALSE

#define MXD_PILATUS_DEBUG_GRIMSEL			TRUE

#define MXD_PILATUS_DEBUG_RESPONSE			FALSE

#define MXD_PILATUS_DEBUG_KILL				TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_hrt.h"
#include "mx_process.h"
#include "mx_inttypes.h"
#include "mx_bit.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "d_pilatus.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_pilatus_record_function_list = {
	mxd_pilatus_initialize_driver,
	mxd_pilatus_create_record_structures,
	mx_area_detector_finish_record_initialization,
	NULL,
	NULL,
	mxd_pilatus_open,
	NULL,
	NULL,
	NULL,
	mxd_pilatus_special_processing_setup,
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_pilatus_ad_function_list = {
	mxd_pilatus_arm,
	mxd_pilatus_trigger,
	NULL,
	mxd_pilatus_stop,
	mxd_pilatus_abort,
	NULL,
	NULL,
	NULL,
	mxd_pilatus_get_extended_status,
	mxd_pilatus_readout_frame,
	mxd_pilatus_correct_frame,
	mxd_pilatus_transfer_frame,
	mxd_pilatus_load_frame,
	mxd_pilatus_save_frame,
	mxd_pilatus_copy_frame,
	NULL,
	mxd_pilatus_get_parameter,
	mxd_pilatus_set_parameter,
	mxd_pilatus_measure_correction,
	NULL,
	mxd_pilatus_setup_oscillation,
	mxd_pilatus_trigger_oscillation
};

MX_RECORD_FIELD_DEFAULTS mxd_pilatus_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_PILATUS_STANDARD_FIELDS
};

long mxd_pilatus_num_record_fields
		= sizeof( mxd_pilatus_rf_defaults )
		  / sizeof( mxd_pilatus_rf_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_pilatus_rfield_def_ptr
			= &mxd_pilatus_rf_defaults[0];

static mx_status_type mxd_pilatus_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );

/*---*/

static mx_status_type
mxd_pilatus_get_pointers( MX_AREA_DETECTOR *ad,
			MX_PILATUS **pilatus,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pilatus_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pilatus == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PILATUS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pilatus = (MX_PILATUS *) ad->record->record_type_struct;

	if ( *pilatus == (MX_PILATUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_PILATUS pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------*/

/* Debugging code for Pilatus datafile_directory debugging. */

#if MXD_PILATUS_DEBUG_DIRECTORY_MAPPING

MX_RECORD *mxserver_pilatus_record = NULL;
MX_AREA_DETECTOR *mxserver_pilatus_ad = NULL;
MX_PILATUS *mxserver_pilatus = NULL;

void mstest_pilatus_setup( MX_RECORD *mx_record_list )
{
        mxserver_pilatus_record = mx_get_record( mx_record_list, "pilatus" );
        if ( mxserver_pilatus_record == (MX_RECORD *) NULL ) {
            mx_error( MXE_NOT_FOUND,
		"mstest_pilatus_setup", "'pilatus' not found." );
            exit(MXE_NOT_FOUND);
        }
        mxserver_pilatus_ad = mxserver_pilatus_record->record_class_struct;
        mxserver_pilatus = mxserver_pilatus_record->record_type_struct;
}

void mstest_pilatus_show_datafile_directory( const char *fname,
						const char *marker )
{
	MX_DEBUG(-2,
	("%s: MARKER '%s': mxserver_pilatus_ad->datafile_directory = '%s'",
		fname, marker, mxserver_pilatus_ad->datafile_directory ));
	MX_DEBUG(-2,
	("%s: MARKER: mxserver_pilatus->local_datafile_user = '%s'",
		fname, mxserver_pilatus->local_datafile_user ));
	MX_DEBUG(-2,
	("%s: MARKER: mxserver_pilatus->local_datafile_root = '%s'",
		fname, mxserver_pilatus->local_datafile_root ));
	MX_DEBUG(-2,
	("%s: MARKER: mxserver_pilatus->detector_server_datafile_root = '%s'",
		fname, mxserver_pilatus->detector_server_datafile_root ));
	MX_DEBUG(-2,
	("%s: MARKER: mxserver_pilatus->detector_server_datafile_user = '%s'",
		fname, mxserver_pilatus->detector_server_datafile_user ));
	MX_DEBUG(-2,
    ("%s: MARKER: mxserver_pilatus->detector_server_datafile_directory = '%s'",
		fname, mxserver_pilatus->detector_server_datafile_directory ));
}

#else

void mstest_pilatus_setup( MX_RECORD *mx_record_list ) {}

void mstest_pilatus_show_datafile_directory( const char *fname,
						const char *marker ) {}

#endif

/*------------------------------------------------------------------*/

/*---*/

/* The procedure for mxd_pilatus_parse_datafile_name() is based on
 * the naming scheme described in the Pilatus manual at the end of
 * the "Camserver Commands" section.
 */

static mx_status_type
mxd_pilatus_parse_datafile_name( MX_AREA_DETECTOR *ad,
				MX_PILATUS *pilatus,
				char *datafile_name_to_test,
				unsigned long *datafile_number_found_in_test,
				unsigned long *datafile_number_field_length,
				char *datafile_directory_string,
				size_t datafile_directory_string_max_length,
				char *datafile_namestem_string,
				size_t datafile_namestem_string_max_length,
				char *datafile_number_string,
				size_t datafile_number_string_max_length,
				char *datafile_extension_string,
				size_t datafile_extension_string_max_length )
{
	static const char fname[] = "mxd_pilatus_parse_datafile_name()";

	char datafile_name_copy[MXU_FILENAME_LENGTH+1];
	char *last_period_ptr = NULL;
	char *last_underscore_ptr = NULL;
	char *number_string_ptr = NULL;
	char *directory_separator_ptr = NULL;
	unsigned long nominal_number_field_length;
	unsigned long actual_number_field_length;
	unsigned long local_datafile_number_found_in_test;
	int num_items;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}
	if ( pilatus == (MX_PILATUS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The PILATUS pointer passed for detector '%s' was NULL.",
			ad->record->name );
	}
	if ( datafile_name_to_test == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The datafile_name_to_test pointer passed "
		"for detector '%s' was NULL.",
			ad->record->name );
	}

	/* Datafiles produced by the Pilatus detector software should always
	 * be in the form {name}_{number}.{ext} or {name}_{number}.  The
	 * contents of {name} is relatively arbitrary.  {number} must contain
	 * only the integer digits 0 to 9, must be at least 3 digits long,
	 * and may be zero filled at the start of the number field.  If the
	 * .{ext} field is .tif, .edf, or .cbf, then the datafile will have
	 * been created in that format.  If .{ext} is something else or is
	 * absent, then raw format was used.
	 *
	 * We find the datafile number by looking at the part of the string
	 * between the last '_' in the name and the last '.' in the name
	 * (if present).
	 */

	strlcpy( datafile_name_copy,
		datafile_name_to_test,
		sizeof(datafile_name_copy) );

	/*==== Attempt to find any extension at the end. ====*/

	last_period_ptr = strrchr( datafile_name_copy, '.' );

	if ( datafile_extension_string != (char *) NULL ) {
		if ( last_period_ptr == (char *) NULL ) {
			strlcpy( datafile_extension_string, "",
				datafile_extension_string_max_length );
		} else {
			strlcpy( datafile_extension_string,
				last_period_ptr + 1,
				datafile_extension_string_max_length );
		}
	}

	if ( last_period_ptr != (char *) NULL ) {
		*last_period_ptr = '\0';
	}

	/*==== Attempt to find the number field. ====*/

	last_underscore_ptr = strrchr( datafile_name_copy, '_' );

	if ( last_underscore_ptr == (char *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Datafile name '%s' _cannot_ have been produced by the "
		"detector software for Pilatus detector '%s', since it "
		"does not contain an '_' underscore character.  Thus, we "
		"will not attempt to guess a datafile number from it.",
			datafile_name_to_test, ad->record->name );
	}

	/* Figure out how long the space is for the datafile number. */

	number_string_ptr = last_underscore_ptr + 1;

	nominal_number_field_length = strlen( number_string_ptr );

	/* Are any non-numeric characters found in the remaining part
	 * of the filename after the underscore '_'?  If so, then this
	 * cannot be a valid Pilatus datafile number.
	 */

	actual_number_field_length = strspn( number_string_ptr, "0123456789" );

	if ( nominal_number_field_length > actual_number_field_length ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The part of filename '%s' that appears after the "
		"underscore '_' character contains characters that are "
		"not number characters.  Thus, this filename _cannot_ have "
		"been produced by the detector software "
		"for Pilatus detector '%s'.",
			datafile_name_to_test, ad->record->name );
	}

	/*=====*/

	if ( datafile_number_string != (char *) NULL ) {
		strlcpy( datafile_number_string,
			number_string_ptr,
			datafile_number_string_max_length );
	}

	if ( datafile_number_field_length != (unsigned long *) NULL ) {
		*datafile_number_field_length = actual_number_field_length;
	}

	/*=====*/

	num_items = sscanf( number_string_ptr,
			"%lu", &local_datafile_number_found_in_test );

	if ( num_items != 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"A filename number was not found in the field '%s' extracted "
		"from test filename '%s' for area detector '%s'.",
			number_string_ptr, datafile_name_to_test,
			ad->record->name );
	}

	if ( datafile_number_found_in_test != (unsigned long *) NULL ) {
		*datafile_number_found_in_test
			= local_datafile_number_found_in_test;
	}

	*last_underscore_ptr = '\0';

	/*==== Attempt to get the namestem part of this filename ====*/

	directory_separator_ptr = strrchr( datafile_name_copy, '/' );

	if ( datafile_namestem_string != (char *) NULL ) {
		if ( directory_separator_ptr == (char *) NULL ) {
			strlcpy( datafile_namestem_string,
				datafile_name_copy,
				datafile_namestem_string_max_length );
		} else {
			strlcpy( datafile_namestem_string,
				directory_separator_ptr + 1,
				datafile_namestem_string_max_length );
		}
	}

	if ( directory_separator_ptr != (char *) NULL ) {
		*directory_separator_ptr = '\0';
	}

	/*==== Try to copy the directory name (if any) ====*/

	if ( datafile_directory_string != (char *) NULL ) {
		if ( directory_separator_ptr == (char *) NULL ) {
			strlcpy( datafile_directory_string, "",
				datafile_directory_string_max_length );
		} else {
			strlcpy( datafile_directory_string,
				datafile_name_copy,
				datafile_directory_string_max_length );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxd_pilatus_update_datafile_number( MX_AREA_DETECTOR *ad,
					MX_PILATUS *pilatus,
					char *response )
{
	static const char fname[] = "mxd_pilatus_update_datafile_number()";

	char pathname[MXU_FILENAME_LENGTH+1];
	char datafile_directory_string[MXU_FILENAME_LENGTH+1];
	char datafile_namestem_string[MXU_FILENAME_LENGTH+1];
	char datafile_number_string[MXU_FILENAME_LENGTH+1];
	char datafile_extension_string[MXU_FILENAME_LENGTH+1];
	unsigned long datafile_number_found_in_test = 0;
	unsigned long datafile_number_field_length = 0;
	mx_status_type mx_status;

#if MXD_PILATUS_DEBUG_UPDATE_DATAFILE_NUMBER
	MX_DEBUG(-2,("%s: ***** Record '%s', response = '%s' *****",
		fname, ad->record->name, response));
#endif

	if ( strncmp( response, "7 OK ", 5 ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Should not have been called if the response '%s' "
		"did not start with '7 OK ' for detector '%s', but it did.",
			response, ad->record->name );
	}

	strlcpy( pathname, response + strlen("7 OK "), sizeof(pathname) );

#if MXD_PILATUS_DEBUG_UPDATE_DATAFILE_NUMBER
	MX_DEBUG(-2,("%s: pathname = '%s'", fname, pathname));
#endif

	/* Parse the contents of the pathname. */

	mx_status = mxd_pilatus_parse_datafile_name( ad, pilatus, pathname,
					&datafile_number_found_in_test,
					&datafile_number_field_length,
					datafile_directory_string,
					sizeof(datafile_directory_string),
					datafile_namestem_string,
					sizeof(datafile_namestem_string),
					datafile_number_string,
					sizeof(datafile_number_string),
					datafile_extension_string,
					sizeof(datafile_extension_string) );

#if MXD_PILATUS_DEBUG_UPDATE_DATAFILE_NUMBER
	MX_DEBUG(-2,("%s: datafile_number_found_in_test = %lu",
		fname, datafile_number_found_in_test));
	MX_DEBUG(-2,("%s: datafile_number_field_length = %lu",
		fname, datafile_number_field_length));
	MX_DEBUG(-2,("%s: datafile_directory_string = '%s'",
		fname, datafile_directory_string));
	MX_DEBUG(-2,("%s: datafile_namestem_string = '%s'",
		fname, datafile_namestem_string));
	MX_DEBUG(-2,("%s: datafile_number_string = '%s'",
		fname, datafile_number_string));
	MX_DEBUG(-2,("%s: datafile_extension_string = '%s'",
		fname, datafile_extension_string));
#endif

	return mx_status;
}

/*---*/

static mx_status_type
mxd_pilatus_get_target_directory_from_grimsel( MX_PILATUS *pilatus )
{
	static const char fname[] =
		"mxd_pilatus_get_target_directory_from_grimsel()";

	FILE *grimsel_file = NULL;
	char grimsel_filename[] = "/etc/grimsel_dectris.conf";
	int saved_errno;
	char buffer[200];
	int num_items;
	char sscanf_format[40];
	char filename_temp[MXU_FILENAME_LENGTH+1];
	mx_status_type mx_status;

	if ( pilatus == (MX_PILATUS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PILATUS pointer passed was NULL." );
	}

	grimsel_file = fopen( grimsel_filename, "r" );

	if ( grimsel_file == (FILE *) NULL ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case EACCES:
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"Cannot open Grimsel file '%s' for detector '%s'.",
				grimsel_filename, pilatus->record->name );
			break;
		case ENOENT:
			return mx_error( MXE_NOT_FOUND, fname,
			"Grimsel file '%s' was not found for detector '%s'.",
				grimsel_filename, pilatus->record->name );
			break;
		default:
			return mx_error( MXE_FILE_IO_ERROR, fname,
				"An error occurred while trying to open "
				"Grimsel file '%s' for detector '%s'.  "
				"errno = %d, error message = '%s'.",
				grimsel_filename, pilatus->record->name,
				saved_errno, strerror(saved_errno) );
			break;
		}
	}

	/* Loop through the file looking for lines that we care about.*/

	snprintf( sscanf_format, sizeof(sscanf_format),
			"%%*s %%%ds", MXU_FILENAME_LENGTH );

#if 0
	MX_DEBUG(-2,("%s: sscanf_format = '%s'", fname, sscanf_format));
#endif

	while ( 1 ) {
		mx_fgets( buffer, sizeof(buffer), grimsel_file );

		if ( feof(grimsel_file) ) {
			(void) fclose( grimsel_file );
			break;
		}
		if ( ferror(grimsel_file) ) {
			return mx_error( MXE_FILE_IO_ERROR, fname,
				"An unknown file I/O error occurred while "
				"trying to read Grimsel file '%s' for "
				"Pilatus detector '%s'.",
				grimsel_filename, pilatus->record->name );

			(void) fclose( grimsel_file );
			break;
		}

		if (strncmp( buffer, "target", 6 ) == 0) {

			num_items = sscanf( buffer, sscanf_format,
						filename_temp );

			if ( num_items != 1 ) {
				return mx_error(
				MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
					"Did not find a filename in the "
					"line '%s' from Grimsel file '%s' "
					"for detector '%s'.",
						buffer, grimsel_filename,
						pilatus->record->name );
			}

			strlcpy( pilatus->local_datafile_user,
					filename_temp,
					sizeof( pilatus->local_datafile_user ));

#if MXD_PILATUS_DEBUG_GRIMSEL
			MX_DEBUG(-2,("%s: local_datafile_user = '%s'",
				fname, pilatus->local_datafile_user ));
#endif

			/* Check that this directory choice is valid
			 * and set it to the datafile_root if the
			 * choice is _not_ valid.
			 */

			mx_status = mx_process_record_field_by_name(
					pilatus->record, "local_datafile_user",
					NULL, MX_PROCESS_PUT, NULL );
		}
	}

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mxd_pilatus_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_driver( driver,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_pilatus_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_PILATUS *pilatus = NULL;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	pilatus = (MX_PILATUS *) malloc( sizeof(MX_PILATUS) );

	if ( pilatus == (MX_PILATUS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_PILATUS structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = pilatus;
	record->class_specific_function_list = 
			&mxd_pilatus_ad_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	pilatus->record = record;

	ad->trigger_mode = 0;
	ad->initial_correction_flags = 0;

	pilatus->exposure_in_progress = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pilatus_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pilatus_open()";

	MX_AREA_DETECTOR *ad = NULL;
	MX_PILATUS *pilatus = NULL;
	MX_RECORD_FIELD *framesize_field = NULL;
	char command[MXU_FILENAME_LENGTH + 20];
	char response[500];
	char *ptr;
	size_t i, length;
	char *buffer_ptr, *line_ptr;
	int num_items;
	unsigned long mask;
	unsigned long pilatus_flags;
	mx_bool_type debug_serial;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

#if MXD_PILATUS_DEBUG_DIRECTORY_MAPPING
	mstest_pilatus_setup( record->list_head );
#endif

#if MXD_PILATUS_DEBUG_SERIAL
	pilatus->pilatus_flags |= MXF_PILATUS_DEBUG_SERIAL;

	debug_serial = TRUE;

	MX_DEBUG(-2,("%s: Forcing on serial debugging for detector '%s'.",
				fname, record->name ));
#else
	debug_serial = FALSE;
#endif

	(void) mx_rs232_discard_unwritten_output( pilatus->rs232_record,
							debug_serial );

	(void) mx_rs232_discard_unread_input( pilatus->rs232_record,
							debug_serial );

	/* Get the version of the TVX software. */

	mx_status = mxd_pilatus_command( pilatus, "Version",
				response, sizeof(response), NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The TVX version is located after the string 'Code release:'. */

	ptr = strchr( response, ':' );

	if ( ptr == NULL ) {
		strlcpy( pilatus->tvx_version, response,
			sizeof(pilatus->tvx_version) );
	} else {
		ptr++;

		length = strspn( ptr, " " );

		ptr += length;

		strlcpy( pilatus->tvx_version, ptr,
			sizeof(pilatus->tvx_version) );
	}

	/* Send a CamSetup command and parse the responses back from it. */

	mx_status = mxd_pilatus_command( pilatus, "CamSetup",
				response, sizeof(response), NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	buffer_ptr = response;
	line_ptr   = buffer_ptr;

	for ( i = 0; ; i++ ) {
		buffer_ptr = strchr( buffer_ptr, '\n' );

		if ( buffer_ptr != NULL ) {
			*buffer_ptr = '\0';
			buffer_ptr++;

			length = strspn( buffer_ptr, " \t" );
			buffer_ptr += length;
		}

		if ( strncmp( line_ptr, "Controlling PID is: ", 20 ) == 0 ) {
			pilatus->camserver_pid = atof( line_ptr + 20 );
		} else
		if ( strncmp( line_ptr, "Camera name: ", 13 ) == 0 ) {
			strlcpy( pilatus->camera_name, line_ptr + 13,
				sizeof(pilatus->camera_name) );
		}

		if ( buffer_ptr == NULL ) {
			break;		/* Exit the for() loop. */
		}

		line_ptr = buffer_ptr;
	}

	/* Send a Telemetry command and look for the image dimensions in it. */

	mx_status = mxd_pilatus_command( pilatus, "Telemetry",
				response, sizeof(response), NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	buffer_ptr = response;
	line_ptr   = buffer_ptr;

	for ( i = 0; ; i++ ) {
		buffer_ptr = strchr( buffer_ptr, '\n' );

		if ( buffer_ptr != NULL ) {
			*buffer_ptr = '\0';
			buffer_ptr++;

			length = strspn( buffer_ptr, " \t" );
			buffer_ptr += length;
		}

		if ( strncmp( line_ptr, "Image format: ", 14 ) == 0 ) {

			num_items = sscanf( line_ptr,
					"Image format: %lu(w) x %lu(h) pixels",
					&(ad->framesize[0]),
					&(ad->framesize[1]) );

			if ( num_items != 2 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Could not find the area detector resolution "
				"in Telemetry response line '%s' for "
				"detector '%s'.",
					line_ptr, pilatus->record->name );
			}

			ad->maximum_framesize[0] = ad->framesize[0];
			ad->maximum_framesize[1] = ad->framesize[1];
		}

		if ( buffer_ptr == NULL ) {
			break;		/* Exit the for() loop. */
		}

		line_ptr = buffer_ptr;
	}

	/* The framesize of the Pilatus detector is fixed, so we set
	 * the 'framesize' field to be read only.
	 */

	framesize_field = mx_get_record_field( record, "framesize" );

	if ( framesize_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Area detector '%s' somehow does not have a 'framesize' field.",
			record->name );
	}

	framesize_field->flags |= MXFF_READ_ONLY;

	ad->binsize[0] = 1;
	ad->binsize[1] = 1;

	/* Set generic area detector parameters. */

	ad->maximum_frame_number = 0;
	ad->last_frame_number = -1;
	ad->total_num_frames = 0;
	ad->status = 0;

	ad->bytes_per_pixel = 4;
	ad->bits_per_pixel = 32;

	ad->bytes_per_frame =
	  mx_round( ad->bytes_per_pixel * ad->framesize[0] * ad->framesize[1] );

	ad->image_format = MXT_IMAGE_FORMAT_GREY32;

	ad->trigger_mode = MXF_DEV_INTERNAL_TRIGGER;

	mx_status = mx_image_get_image_format_name_from_type(
						ad->image_format,
						ad->image_format_name,
						MXU_IMAGE_FORMAT_NAME_LENGTH );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->byte_order = (long) mx_native_byteorder();

	/* Initialize some Pilatus specific sequence parameters.
	 *
	 * These can be overridden at startup time with 'mxautosave'.
	 */

	pilatus->delay_time = 0.0;
	pilatus->exposure_period = 0.0;
	pilatus->gap_time = 0.0;
	pilatus->exposures_per_frame = 1;

	/* Pilatus external enable mode expects that the exposure time and
	 * period be set to sensible values by the user, so we arbitrarily
	 * set them both to 1.  If the user wants the values for these
	 * variables maintained between runs of the MX server, then the
	 * user needs to include the 'ext_enable_time' and 'ext_enable_period'
	 * variables in 'mxautosave's list of variables to save and restore.
	 */

	pilatus->ext_enable_time = 1.0;
	pilatus->ext_enable_period = 1.0;

	/* If the 'acknowledgement_interval' field is set to a non-zero value,
	 * then we use the 'SetAckInt' command to tell 'camserver' to send us
	 * an acknowledgement every n-th frame.  If the field is 0, then we
	 * send no command at all.
	 */

	if ( pilatus->acknowledgement_interval > 0 ) {
		snprintf( command, sizeof(command),
		"SetAckInt %lu", pilatus->acknowledgement_interval );

		mx_status = mxd_pilatus_command( pilatus, command,
					response, sizeof(response), NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Configure automatic saving or readout of image frames.
	 *
	 * Note: The Pilatus system automatically saves files on its own, so
	 * there should generally not be a reason for MX to do that too.
	 */

	mask = MXF_AD_SAVE_FRAME_AFTER_ACQUISITION
		| MXF_AD_READOUT_FRAME_AFTER_ACQUISITION;

	if ( ad->area_detector_flags & mask ) {
		mx_status =
		  mx_area_detector_setup_datafile_management( record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* If requested, read some detector configuration values from the
	 * Dectris Grimsel file located at /etc/grimsel_dectris.conf.
	 * Of course, this only can work if the MX server is running on
	 * the Dectris PPU computer.
	 */

	pilatus_flags = pilatus->pilatus_flags;
	
	if ( pilatus_flags & MXF_PILATUS_READ_GRIMSEL_DECTRIS_CONF ) {
		mx_status = 
		    mxd_pilatus_get_target_directory_from_grimsel( pilatus );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Initialize detector_server_datafile_user to be the same as
	 * detector_server_datafile_root.  If mxautosave is running,
	 * this field will be updated soon by mxautosave.
	 */

	strlcpy( pilatus->detector_server_datafile_user,
		pilatus->detector_server_datafile_root,
		sizeof( pilatus->detector_server_datafile_user ) );

	/* Do the equivalent initialization for local_datafile_user
	 * by setting it to local_datafile_root.
	 */

	strlcpy( pilatus->local_datafile_user,
		pilatus->local_datafile_root,
		sizeof( pilatus->local_datafile_user ) );

#if ( 1 || MXD_PILATUS_DEBUG )
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pilatus_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_arm()";

	MX_PILATUS *pilatus = NULL;
	MX_SEQUENCE_PARAMETERS *sp = NULL;
	unsigned long num_frames;
	double exposure_time, exposure_period, gap_time, delay_time;
	char command[MXU_PILATUS_COMMAND_LENGTH+1];
	char response[MXU_PILATUS_COMMAND_LENGTH+1];
	int is_subdirectory;
	mx_status_type mx_status;

	pilatus = NULL;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif
	/* It is possible that someone has changed ImgPath behind our back
	 * by directly talking to camserver, so we must update our copy of
	 * the value of detector_server_datafile_directory right now.
	 */

	mx_status = mx_process_record_field_by_name( ad->record,
					"detector_server_datafile_directory",
					NULL, MX_PROCESS_GET, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Is detector_server_datafile_directory within
	 * detector_server_datafile_user?
	 */

	mx_status = mx_is_subdirectory(
			pilatus->detector_server_datafile_user,
			pilatus->detector_server_datafile_directory,
				&is_subdirectory );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( is_subdirectory == FALSE ) {
		return mx_error( MXE_CONFIGURATION_CONFLICT,fname,
			"Requested directory '%s' for "
			"'%s.detector_server_datafile_directory' is not a "
			"subdirectory of '%s' for "
			"'%s.detector_server_datafile_user'.",
				pilatus->detector_server_datafile_directory,
				ad->record->name,
				pilatus->detector_server_datafile_user,
				ad->record->name );
	}

	/* Is datafile_directory within local_datafile_user? */

	mx_status = mx_is_subdirectory( pilatus->local_datafile_user,
					ad->datafile_directory,
					&is_subdirectory );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( is_subdirectory == FALSE ) {
		return mx_error( MXE_CONFIGURATION_CONFLICT,fname,
			"Requested directory '%s' for "
			"'%s.datafile_directory' is not a "
			"subdirectory of '%s' for "
			"'%s.local_datafile_user'.",
				ad->datafile_directory,
				ad->record->name,
				pilatus->local_datafile_user,
				ad->record->name );
	}

	/* The directories are correctly set up, so we can proceed. */

	pilatus->old_pilatus_image_counter = 0;

	sp = &(ad->sequence_parameters);

	if ( ad->trigger_mode & MXF_DEV_INTERNAL_TRIGGER ) {
		switch( sp->sequence_type ) {
		case MXT_SQ_ONE_SHOT:
		case MXT_SQ_MULTIFRAME:
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Only 'one shot' and 'multiframe' sequences are "
			"supported for detector '%s' in internal trigger mode.",
				ad->record->name );
			break;
		}
	}

	/* Set the exposure sequence parameters. 
	 *
	 * Note: The Pilatus requires us to specify the exposure time
	 * and period in order to do some image corrections.
	 *
	 * For the one shot and strobe sequence types, specifying the
	 * exposure period is somewhat meaningless, since a given
	 * trigger pulse only causes one frame to be taken, so we
	 * set it to exposure_time + 0.007, since the manual says
	 * "The minimum time between exposure time and exposure
	 * period must be 7 ms."
	 *
	 * For duration mode images, the user is expected to have set
	 * the times beforehand using the 'ext_enable_time' and the
	 * 'ext_enable_period' MX fields in the 'pilatus' record.
	 */

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		num_frames = 1;
		exposure_time = sp->parameter_array[0];
		exposure_period = exposure_time + pilatus->gap_time;
		break;
	case MXT_SQ_MULTIFRAME:
		num_frames = mx_round( sp->parameter_array[0] );
		exposure_time = sp->parameter_array[1];
		exposure_period = sp->parameter_array[2];
		break;
	case MXT_SQ_STROBE:
		num_frames = mx_round( sp->parameter_array[0] );
		exposure_time = sp->parameter_array[1];
		exposure_period = exposure_time + pilatus->gap_time;
		break;
	case MXT_SQ_DURATION:
		num_frames = mx_round( sp->parameter_array[0] );
		exposure_time = pilatus->ext_enable_time;
		exposure_period = pilatus->ext_enable_period;
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Detector '%s' sequence types other than 'one-shot' "
		"are not yet implemented.", ad->record->name );
		break;
	}

#if 1
	/* FIXME: The minimum gap time should be dependent on
 	 * the model of Pilatus detector.
	 */

	if ( pilatus->gap_time < 0.001 ) {
		pilatus->gap_time = 0.001;

		MX_DEBUG(-2,("%s: resetting pilatus->gap_time to %g",
			fname, pilatus->gap_time));
	}
#endif

	/* Make sure that the difference between the exposure period and
	 * the exposure time is greater than or equal to the minimum
	 * allowed gap time.  The minimum allowed time is different for
	 * different versions of the Pilatus detector.
	 */

	gap_time = exposure_period - exposure_time;

	if ( gap_time < pilatus->gap_time ) {
		exposure_period = exposure_time + pilatus->gap_time;
	}

	/* Set the number of frames. */

	snprintf( command, sizeof(command), "NImages %lu", num_frames );

	mx_status = mxd_pilatus_command( pilatus, command,
				response, sizeof(response), NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the exposure time. */

	snprintf( command, sizeof(command), "ExpTime %f", exposure_time );

	mx_status = mxd_pilatus_command( pilatus, command,
				response, sizeof(response), NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the exposure period. */

	snprintf( command, sizeof(command), "ExpPeriod %f", exposure_period );

	mx_status = mxd_pilatus_command( pilatus, command,
				response, sizeof(response), NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Save the actual exposure period so that it can be requested
	 * by the user.
	 */

	pilatus->exposure_period = exposure_period;

	/* Compute and set the exposure delay. */

	if ( pilatus->delay_time > 0.0 ) {
		delay_time = pilatus->delay_time;
	} else {
		delay_time = 0.0;
	}

	snprintf( command, sizeof(command), "Delay %f", delay_time );

	mx_status = mxd_pilatus_command( pilatus, command,
				response, sizeof(response), NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the number of exposures per frame. */

	snprintf( command, sizeof(command),
		"NExpFrame %lu", pilatus->exposures_per_frame );

	mx_status = mxd_pilatus_command( pilatus, command,
				response, sizeof(response), NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---------------------------------------------------------------*/

	/* Enable the shutter. */

	mx_status = mxd_pilatus_command( pilatus, "ShutterEnable 1",
				response, sizeof(response), NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Create the name of the next area detector image file. */

	mx_status = mx_area_detector_construct_next_datafile_name(
						ad->record, TRUE, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check to see if we currently have a valid datafile name. */

	if ( strlen( ad->datafile_name ) == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"No datafile name has been specified for the area detector "
		"field '%s.datafile_name'.", ad->record->name );
	}

	/* Save the starting value of 'total_num_frames' so that it can be
	 * used later by the extended_status() method.
	 */

	pilatus->old_total_num_frames = ad->total_num_frames;
	pilatus->old_datafile_number = ad->datafile_number;

	/* If we are not in external trigger mode, then we are done here. */

	if ( ( ad->trigger_mode & MXF_DEV_EXTERNAL_TRIGGER ) == 0 ) {
		pilatus->exposure_in_progress = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, we are in external trigger mode, so we must
	 * tell the Pilatus to be ready for the trigger signal.
	 */

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_MULTIFRAME:
		snprintf( command, sizeof(command),
			"ExtTrigger %s", ad->datafile_name );
		break;
	case MXT_SQ_STROBE:
		snprintf( command, sizeof(command),
			"ExtMTrigger %s", ad->datafile_name );
		break;
	case MXT_SQ_DURATION:
		snprintf( command, sizeof(command),
			"ExtEnable %s", ad->datafile_name );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported sequence type requested for detector '%s'.",
			ad->record->name );
		break;
	}

	mx_status = mxd_pilatus_command( pilatus, command,
				response, sizeof(response), NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pilatus->exposure_in_progress = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pilatus_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_trigger()";

	MX_PILATUS *pilatus = NULL;
	char command[MXU_FILENAME_LENGTH + 80];
	char response[80];
	mx_status_type mx_status;

	pilatus = NULL;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif
	/* If we are not in internal trigger mode, we do nothing here. */

	if ( ( ad->trigger_mode & MXF_DEV_INTERNAL_TRIGGER ) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Check to see if we currently have a valid datafile name. */

	if ( strlen( ad->datafile_name ) == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"No datafile name has been specified for the area detector "
		"field '%s.datafile_name'.", ad->record->name );
	}

	/* Start the exposure. */

	snprintf( command, sizeof(command), "Exposure %s", ad->datafile_name );

	mx_status = mxd_pilatus_command( pilatus, command,
				response, sizeof(response), NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pilatus->exposure_in_progress = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

static mx_status_type
mxd_pilatus_kill( MX_AREA_DETECTOR *ad, char *kill_command )
{
	static const char fname[] = "mxd_pilatus_kill()";

	MX_PILATUS *pilatus = NULL;
	MX_RS232 *rs232 = NULL;
	char buffer[80];
	unsigned long image_number;
	long frame_difference;
	unsigned long n, max_attempts;
	unsigned long mx_status_code;
	int num_items;
	size_t num_response_bytes, buffer_len;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pilatus->rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The rs232_record pointer for Pilatus record '%s' is NULL.",
			pilatus->record->name );
	}

	rs232 = pilatus->rs232_record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_RS232 pointer for RS232 record '%s' used by '%s' is NULL.",
			pilatus->rs232_record->name,
			pilatus->record->name );
	}

#if MXD_PILATUS_DEBUG_KILL
	MX_DEBUG(-2,("%s invoked for area detector '%s',  kill_command = '%s'",
		fname, ad->record->name, kill_command ));
#endif
	/* Send the kill command. */

#if 0
	mx_status = mxd_pilatus_command( pilatus, kill_command, NULL, 0, NULL );
#else
	mx_status = mx_rs232_putline( pilatus->rs232_record,
					kill_command, NULL, 0 );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We cannot send any new commands to the Pilatus until it has
 	 * acknowledged the kill.  So we have to wait here until we get
 	 * a kill response or a timeout.
	 */

	max_attempts = 10;

	for ( n = 0; n < max_attempts; n++ ) {

	    memset( buffer, 0, sizeof(buffer) );

	    mx_status = mx_rs232_getline_with_timeout(
				pilatus->rs232_record,
				buffer, sizeof(buffer),
				&num_response_bytes, 0,
				rs232->timeout );

	    mx_status_code = mx_status.code & (~MXE_QUIET);

#if 1
	    MX_DEBUG(-2,("%s: *** n = %lu, mx_status_code = %lu, "
		"num_response_bytes = %ld, response = '%s'",
		fname, n, mx_status_code,
		(long) num_response_bytes, buffer ));
#endif
	    /* --- */

	    if ( mx_status_code == MXE_SUCCESS ) {

		/* Please note that the order of the tests below
 		 * sometimes matters.
 		 */

		buffer_len = strlen( buffer );

		if ( strncmp( buffer, "15 OK", 5 ) == 0 ) {
		    if ( buffer_len <= 6 ) {
			/* No additional parameters given, so we are done. */

			break;	/* Exit the for() loop. */
		    }
		    num_items = sscanf(buffer, "15 OK img %lu", &image_number);

		    if ( num_items == 1 ) {
			/* A new frame has arrived. */

			frame_difference = image_number - ad->last_frame_number;

			if ( frame_difference > 0 ) {
			    ad->total_num_frames += frame_difference;
			}

			/* We do _not_ exit the for() loop here, since we
			 * still need to handle the 13 and 7 error codes
			 * that may appear.
			 */
		    }

		} else
		if ( strcmp( buffer, "13 ERR kill" ) == 0 ) {
		    /* If the Pilatus was not actually running an imaging
 		     * sequence, then we are done here (?)
 		     */

		    if ( pilatus->exposure_in_progress == FALSE ) {
			break;	/* Exit the for() loop. */
		    }

		} else
		if ( strncmp( buffer, "7 ERR", 5 ) == 0 ) {
 		    /* If it _was_ running a sequence, then we should see
 		     * a 7 ERR message followed by a 7 OK message.  We
 		     * accept the 7 ERR message, but do not do anything
 		     * with it, since we really want to see the 7 OK.
 		     */
		} else
		if ( strncmp( buffer, "7 OK", 4 ) == 0 ) {
		    /* We increment total_num_frames and then mark the 
 		     * exposure as over.
 		     */

		    ad->total_num_frames++;

		    pilatus->exposure_in_progress = FALSE;
		    break;	/* Exit the for() loop. */
		}
		else
		if ( strncmp( buffer,
			"1 ERR *** Unrecognized command: ", 32 ) == 0 ) {
		    /* If 'camcmd k' was sent and an imaging sequence was not
 		     * in progress, then we may get a '1 ERR' status message.
 		     */

		    if ( pilatus->exposure_in_progress == FALSE ) {
			break;	/* Exit the for() loop. */
		    }
		} else
		if ( strncmp( buffer, "1 OK", 4 ) == 0 ) {
		    /* Alternately, if 'camcmd k' was sent and an imaging
 		     * sequence was not in progress, then we may get a '1 OK'
 		     * status message.
 		     */

		    if ( pilatus->exposure_in_progress == FALSE ) {
			break;	/* Exit the for() loop. */
		    }
		} else {
		    return mx_error( MXE_UNPARSEABLE_STRING, fname,
		  "Unexpected message seen from detector '%s'.  buffer = '%s'",
			pilatus->record->name, buffer );
		}
	    } else
	    if ( mx_status_code == MXE_TIMED_OUT ) {
		/* If an imaging sequence was not running, then the getline
 		 * will time out since the Pilatus will not send a response
 		 * to the kill command.  If this sequence of events occurs,
 		 * then we assume that all is well and return an MXE_SUCCESS
 		 * status message.
 		 */

		break;		/* Exit the for() loop. */
	    } else {
		return mx_status;
	    }
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mxd_pilatus_stop( MX_AREA_DETECTOR *ad )
{
	mx_status_type mx_status;

	mx_status = mxd_pilatus_kill( ad, "K" );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_abort( MX_AREA_DETECTOR *ad )
{
	mx_status_type mx_status;

	mx_status = mxd_pilatus_kill( ad, "camcmd k" );

	return mx_status;
}

/*----*/

MX_EXPORT mx_status_type
mxd_pilatus_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_get_extended_status()";

	MX_PILATUS *pilatus = NULL;
	char response[80];
	unsigned long num_input_bytes_available, old_status;
	unsigned long pilatus_image_counter;
	long num_frames_in_sequence;
	mx_status_type mx_status;

	static unsigned long saved_total_num_frames = ULONG_MAX;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG_EXTENDED_STATUS
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	mx_status = mx_rs232_num_input_bytes_available( pilatus->rs232_record,
						&num_input_bytes_available );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_input_bytes_available > 0 ) {
		mx_bool_type debug_flag;

		if ( pilatus->pilatus_flags & MXF_PILATUS_DEBUG_SERIAL ) {
			debug_flag = TRUE;
		} else {
			debug_flag = FALSE;
		}

		mx_status = mx_rs232_getline( pilatus->rs232_record,
					response, sizeof(response),
					NULL, debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* We have to go to extreme lengths to extract the information
		 * that we want from the acknowledgements.
		 */

		if ( strncmp( response, "15 OK", 5 ) == 0 ) {
			int num_items;

			num_items = sscanf( response,
					"15 OK img %lu",
					&pilatus_image_counter );

			if ( num_items != 1 ) {
				mx_warning(
				"%s: Unexpected status 15 seen: '%s'",
					fname, response );
			} else {
				pilatus->old_pilatus_image_counter
						= pilatus_image_counter;

				ad->last_frame_number
						= pilatus_image_counter - 1;

				ad->total_num_frames =
					pilatus->old_total_num_frames
						+ pilatus_image_counter;
			}
		} else
		if ( strncmp( response, "7 ERR", 5 ) == 0 ) {
			pilatus->exposure_in_progress = FALSE;
		
			ad->status = MXSF_AD_ERROR;

			ad->last_frame_number =
				pilatus->old_pilatus_image_counter - 1;

			ad->total_num_frames = pilatus->old_total_num_frames
				+ pilatus->old_pilatus_image_counter;
		} else
		if ( strncmp( response, "7 OK", 4 ) == 0 ) {
			/* If we get here, the Pilatus is saying that it
			 * has finished the sequence correctly.  So we
			 * update the frame counters assuming that this
			 * is true.
			 */

			pilatus->exposure_in_progress = FALSE;

			ad->status = 0;

			mx_status = mx_sequence_get_num_frames(
					&(ad->sequence_parameters),
					&num_frames_in_sequence );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			ad->last_frame_number = num_frames_in_sequence - 1;
			ad->total_num_frames = pilatus->old_total_num_frames
						+ num_frames_in_sequence;

			/* Try to update ad->datafile_number from the
			 * filename in the message.
			 */

			mx_status = mxd_pilatus_update_datafile_number(
					ad, pilatus, response );
		} else
		if ( strncmp( response, "1 OK", 4 ) == 0 ) {

			/* FIXME: Sometimes we get a '1 OK' message for	
 			 * some unknown reason, so we ignore it.
 			 */
		} else {
			mx_warning(
			"%s: Unexpected response '%s' seen for detector '%s'",
				fname, response, ad->record->name );
		}

#if 1
		if ( ad->total_num_frames != saved_total_num_frames ) {
			saved_total_num_frames = ad->total_num_frames;

			MX_DEBUG(-2,
			(">>>>> %s: ad->total_num_frames = %lu <<<<<",
				fname, ad->total_num_frames));
		}
#endif
	}

	old_status = ad->status;

	MXW_UNUSED(old_status);

	ad->status = 0;

	if ( pilatus->exposure_in_progress ) {
		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	}

#if MXD_PILATUS_DEBUG_EXTENDED_STATUS
	MX_DEBUG(-2,("%s: ad->status = %#lx", fname, ad->status));
#elif MXD_PILATUS_DEBUG_EXTENDED_STATUS_CHANGE
	if ( old_status != ad->status ) {
		MX_DEBUG(-2,("****** %s: ad->status = %#lx ******",
			fname, ad->status));
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pilatus_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_readout_frame()"; 
	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s', frame %ld.",
		fname, ad->record->name, ad->readout_frame ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_correct_frame()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,
		("%s invoked for area detector '%s', correction_flags=%#lx.",
		fname, ad->record->name, ad->correction_flags ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_transfer_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
			"mxd_pilatus_transfer_frame()";

	MX_PILATUS *pilatus = NULL;
	char local_image_filename[2*MXU_FILENAME_LENGTH+3];
	unsigned long flags;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG_TRANSFER
	MX_DEBUG(-2,("%s invoked for area detector '%s', transfer_frame = %ld.",
		fname, ad->record->name, ad->transfer_frame ));
#endif

	flags = pilatus->pilatus_flags;

	if ( flags & MXF_PILATUS_LOAD_FRAME_AFTER_ACQUISITION ) {
		/* If this flag is set, then we load the most recently
		 * acquired frame from the image file that was saved
		 * by camserver.
		 */

		snprintf( local_image_filename,
			sizeof(local_image_filename),
			"%s/%s",
			ad->datafile_directory,
			ad->datafile_name );

#if MXD_PILATUS_DEBUG_TRANSFER
		MX_DEBUG(-2,("%s: Loading Pilatus image '%s'.\n",
			fname, local_image_filename ));
#endif

		/* For now, we assume that the image file is in TIFF format. */

		mx_status = mx_image_read_tiff_file( &(ad->image_frame),
						NULL, local_image_filename );

		if (mx_status.code != MXE_SUCCESS)
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pilatus_load_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_load_frame()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	pilatus = NULL;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_save_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_save_frame()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_copy_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_copy_frame()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,
	("%s invoked for area detector '%s', source = %#lx, destination = %#lx",
		fname, ad->record->name, ad->copy_frame[0], ad->copy_frame[1]));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_get_parameter()";

	MX_PILATUS *pilatus = NULL;
	char response[MXU_FILENAME_LENGTH + 80];
	unsigned long pilatus_return_code;
	unsigned long disk_blocks_available;
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG_PARAMETERS
	{
		char name_buffer[MXU_FIELD_NAME_LENGTH+1];

		MX_DEBUG(-2,("%s: record '%s', parameter '%s' (%ld)",
			fname, ad->record->name,
			mx_get_parameter_name_from_type(
				ad->record, ad->parameter_type,
				name_buffer, sizeof(name_buffer)),
			ad->parameter_type));
	}
#endif

#if MXD_PILATUS_DEBUG_DIRECTORY_MAPPING
	mstest_pilatus_show_datafile_directory( fname, "before" );
#endif

	switch( ad->parameter_type ) {
	case MXLV_AD_MAXIMUM_FRAME_NUMBER:
		break;

	case MXLV_AD_MAXIMUM_FRAMESIZE:
		break;

	case MXLV_AD_FRAMESIZE:
		break;

	case MXLV_AD_BINSIZE:
		break;

	case MXLV_AD_CORRECTION_FLAGS:
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
#if 0
		MX_DEBUG(-2,("%s: image format = %ld, format name = '%s'",
		    fname, ad->image_format, ad->image_format_name));
#endif
		break;

	case MXLV_AD_BYTE_ORDER:
		break;

	case MXLV_AD_TRIGGER_MODE:
		break;

	case MXLV_AD_EXPOSURE_MODE:
		break;

	case MXLV_AD_BYTES_PER_FRAME:
		break;

	case MXLV_AD_BYTES_PER_PIXEL:
		break;

	case MXLV_AD_BITS_PER_PIXEL:
		break;

	case MXLV_AD_DETECTOR_READOUT_TIME:
		break;

	case MXLV_AD_SEQUENCE_START_DELAY:
		break;

	case MXLV_AD_TOTAL_ACQUISITION_TIME:
		break;

	case MXLV_AD_TOTAL_SEQUENCE_TIME:
		break;

	case MXLV_AD_SEQUENCE_TYPE:

#if 0
		MX_DEBUG(-2,("%s: GET sequence_type = %ld",
			fname, ad->sequence_parameters.sequence_type));
		MX_DEBUG(-2,("%s: sequence type = %ld",
			fname, ad->sequence_parameters.sequence_type));
#endif
		break;

	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
		break;

	case MXLV_AD_ROI:
		break;

	case MXLV_AD_SUBFRAME_SIZE:
		break;

	case MXLV_AD_USE_SCALED_DARK_CURRENT:
		break;

	case MXLV_AD_REGISTER_VALUE:
		break;

	case MXLV_AD_SHUTTER_ENABLE:
		break;

	case MXLV_AD_DISK_SPACE:
		mx_status = mxd_pilatus_command( pilatus, "Df",
					response, sizeof(response),
					&pilatus_return_code );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response,
			"%lu", &disk_blocks_available );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Did not see the number of disk blocks available in "
			"the response '%s' to command 'Df' for detector '%s'.",
				response, ad->record->name );
		}

		ad->disk_space[0] = 0.0;
		ad->disk_space[1] = 1024L * (uint64_t ) disk_blocks_available;
		break;

	case MXLV_AD_DATAFILE_ALLOW_OVERWRITE:
		break;

	case MXLV_AD_DATAFILE_AUTOSELECT_NUMBER:
		break;

	case MXLV_AD_DATAFILE_DIRECTORY:

		if ( pilatus->exposure_in_progress ) {
			/* If an exposure sequence is currently in progress,
			 * then we cannot send an 'ImgPath' command to the PPU.
			 * The best we can do in this circumstances is to 
			 * merely reuse the value that is already in the
			 * ad->datafile_directory field.
			 */

			MX_DEBUG(-2,("%s: EIP datafile_directory = '%s'",
				fname, ad->datafile_directory));

			return MX_SUCCESSFUL_RESULT;
		} else {
			mx_status = mxd_pilatus_command( pilatus, "ImgPath",
					response, sizeof(response),
					&pilatus_return_code );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			strlcpy( pilatus->detector_server_datafile_directory,
			  response,
			  sizeof(pilatus->detector_server_datafile_directory));
		}

#if 0
		MX_DEBUG(-2,
("%s: ***1*** datafile_directory = '%s', detector_server_datafile_directory = '%s'",
			fname, ad->datafile_directory,
			pilatus->detector_server_datafile_directory));
#endif

		mx_status = mx_change_filename_prefix(
				pilatus->detector_server_datafile_directory,
				pilatus->detector_server_datafile_user,
				pilatus->local_datafile_user,
				ad->datafile_directory,
				sizeof(ad->datafile_directory) );

		if ( mx_status.code == MXE_ILLEGAL_ARGUMENT ) {
		  MX_DEBUG(-2,("%s: detector_server_datafile_directory = '%s'",
			fname, pilatus->detector_server_datafile_directory));
		  MX_DEBUG(-2,("%s: detector_server_datafile_user = '%s'",
			fname, pilatus->detector_server_datafile_user));
		  MX_DEBUG(-2,("%s: local_datafile_user = '%s'",
			fname, pilatus->local_datafile_user));
		  MX_DEBUG(-2,("%s: datafile_directory = '%s'",
			fname, ad->datafile_directory));
		}
		break;

	case MXLV_AD_DATAFILE_NAME:
		break;

	case MXLV_AD_DATAFILE_NUMBER:
		break;

	case MXLV_AD_DATAFILE_PATTERN:
		break;

	case MXLV_AD_CORRECTION_MEASUREMENT_TIME:
		break;

	case MXLV_AD_CORRECTION_MEASUREMENT_TYPE:
		break;

	case MXLV_AD_NUM_CORRECTION_MEASUREMENTS:
		break;

	case MXLV_AD_FRAME_FILENAME:
		break;

	default:
		mx_status =
			mx_area_detector_default_get_parameter_handler( ad );
		break;
	}

#if MXD_PILATUS_DEBUG_DIRECTORY_MAPPING
	mstest_pilatus_show_datafile_directory( fname, "after" );
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_set_parameter()";

	MX_PILATUS *pilatus = NULL;
	char command[MXU_FILENAME_LENGTH + 80];
	char response[80];
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG_PARAMETERS
	{
		char name_buffer[MXU_FIELD_NAME_LENGTH+1];

		MX_DEBUG(-2,("%s: record '%s', parameter '%s' (%ld)",
			fname, ad->record->name,
			mx_get_parameter_name_from_type(
				ad->record, ad->parameter_type,
				name_buffer, sizeof(name_buffer)),
			ad->parameter_type));
	}
#endif

#if MXD_PILATUS_DEBUG_DIRECTORY_MAPPING
	mstest_pilatus_show_datafile_directory( fname, "before" );
#endif

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
		break;

	case MXLV_AD_BINSIZE:
		break;

	case MXLV_AD_CORRECTION_FLAGS:
		break;

	case MXLV_AD_TRIGGER_MODE:
		break;

	case MXLV_AD_EXPOSURE_MODE:
		break;

	case MXLV_AD_SEQUENCE_TYPE:
		break;

	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
		break;

	case MXLV_AD_SEQUENCE_ONE_SHOT:
		break;

	case MXLV_AD_SEQUENCE_STREAM:
		break;

	case MXLV_AD_SEQUENCE_MULTIFRAME:
		break;

	case MXLV_AD_SEQUENCE_STROBE:
		break;

	case MXLV_AD_SEQUENCE_DURATION:
		break;

	case MXLV_AD_SEQUENCE_GATED:
		break;

	case MXLV_AD_ROI:
		break;

	case MXLV_AD_SUBFRAME_SIZE:
		break;

	case MXLV_AD_USE_SCALED_DARK_CURRENT:
		break;

	case MXLV_AD_GEOM_CORR_AFTER_FLAT_FIELD:
		break;

	case MXLV_AD_CORRECTION_FRAME_GEOM_CORR_LAST:
		break;

	case MXLV_AD_CORRECTION_FRAME_NO_GEOM_CORR:
		break;

	case MXLV_AD_REGISTER_VALUE:
		break;

	case MXLV_AD_SHUTTER_ENABLE:
		break;

	case MXLV_AD_DATAFILE_ALLOW_OVERWRITE:
		break;

	case MXLV_AD_DATAFILE_AUTOSELECT_NUMBER:
		break;

	case MXLV_AD_DATAFILE_DIRECTORY:
#if 0
		MX_DEBUG(-2,("%s: ***2*** ad->datafile_directory = '%s'",
			fname, ad->datafile_directory));
#endif

		mx_status = mx_change_filename_prefix(
				ad->datafile_directory,
				pilatus->local_datafile_user,
				pilatus->detector_server_datafile_user,
				pilatus->detector_server_datafile_directory,
			  sizeof(pilatus->detector_server_datafile_directory));

		if ( mx_status.code == MXE_ILLEGAL_ARGUMENT ) {
		  MX_DEBUG(-2,("%s: detector_server_datafile_directory = '%s'",
			fname, pilatus->detector_server_datafile_directory));
		  MX_DEBUG(-2,("%s: detector_server_datafile_user = '%s'",
			fname, pilatus->detector_server_datafile_user));
		  MX_DEBUG(-2,("%s: local_datafile_user = '%s'",
			fname, pilatus->local_datafile_user));
		  MX_DEBUG(-2,("%s: datafile_directory = '%s'",
			fname, ad->datafile_directory));
		}

#if MXD_PILATUS_DEBUG_DIRECTORY_MAPPING
		mstest_pilatus_show_datafile_directory( fname, "after" );
#endif

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		snprintf( command, sizeof(command),
		    "ImgPath %s", pilatus->detector_server_datafile_directory );

		mx_status = mxd_pilatus_command( pilatus, command,
					response, sizeof(response), NULL );
		break;

	case MXLV_AD_DATAFILE_NAME:
		break;

	case MXLV_AD_DATAFILE_NUMBER:
		break;

	case MXLV_AD_DATAFILE_PATTERN:
		break;

	case MXLV_AD_CORRECTION_MEASUREMENT_TIME:
		break;

	case MXLV_AD_CORRECTION_MEASUREMENT_TYPE:
		break;

	case MXLV_AD_NUM_CORRECTION_MEASUREMENTS:
		break;

	case MXLV_AD_FRAME_FILENAME:
		break;

	case MXLV_AD_RAW_LOAD_FRAME:
		break;

	case MXLV_AD_RAW_SAVE_FRAME:
		break;

	case MXLV_AD_BYTES_PER_FRAME:
	case MXLV_AD_BYTES_PER_PIXEL:
	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
	case MXLV_AD_BYTE_ORDER:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Changing the parameter '%s' for area detector '%s' "
		"is not supported.", mx_get_field_label_string( ad->record,
			ad->parameter_type ), ad->record->name );
	default:
		mx_status =
			mx_area_detector_default_set_parameter_handler( ad );
		break;
	}

#if MXD_PILATUS_DEBUG_DIRECTORY_MAPPING
	mstest_pilatus_show_datafile_directory( fname, "after" );
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_measure_correction( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_pilatus_measure_correction()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
	MX_DEBUG(-2,("%s: type = %ld, time = %g, num_measurements = %ld",
		fname, ad->correction_measurement_type,
		ad->correction_measurement_time,
		ad->num_correction_measurements ));
#endif

	switch( ad->correction_measurement_type ) {
	case MXFT_AD_DARK_CURRENT_FRAME:
	case MXFT_AD_FLAT_FIELD_FRAME:
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Correction measurement type %ld is not supported "
		"for area detector '%s'.",
			ad->correction_measurement_type, ad->record->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_setup_oscillation( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_pilatus_setup_oscillation()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s', motor '%s', "
	"shutter '%s', trigger '%s', "
	"oscillation distance = %f, oscillation time = %f",
		fname, ad->record->name, ad->oscillation_motor_name,
		ad->shutter_name, ad->oscillation_trigger_name,
		ad->oscillation_distance, ad->exposure_time ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_trigger_oscillation( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_pilatus_trigger_oscillation()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	return mx_status;
}

/*==========================================================================*/

MX_EXPORT mx_status_type
mxd_pilatus_command( MX_PILATUS *pilatus,
			char *command,
			char *response,
			size_t max_response_length,
			unsigned long *pilatus_return_code )
{
	static const char fname[] = "mxd_pilatus_command()";

	MX_AREA_DETECTOR *ad = NULL;
	MX_RS232 *rs232 = NULL;
	char command_buffer[1000];
	char local_response_buffer[1000];
	char *response_ptr;
	size_t response_length;
	char error_status_string[20];
	char *ptr, *return_code_arg, *error_status_arg;
	size_t length, command_length, bytes_to_move;
	unsigned long return_code;
	unsigned long n, max_attempts;
	unsigned long mx_status_code;
	size_t num_response_bytes;
	mx_bool_type debug_flag;
	mx_status_type mx_status;

	if ( pilatus == (MX_PILATUS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PILATUS pointer passed was NULL." );
	}

	/* Initialize internal string buffers to nulls. */

	memset( command_buffer, 0, sizeof(command_buffer) );
	memset( local_response_buffer, 0, sizeof(local_response_buffer) );
	memset( error_status_string, 0, sizeof(error_status_string) );

	/* If our caller is not interested in any responses to the command,
 	 * it indicates this by passing a 'response' pointer that is NULL.
 	 * Even so, we still will need to readout and throw away the response
 	 * that the caller is not interested in.  So we must provide a
 	 * 'local_response_buffer' to read the unwanted response into.
 	 */

	if ( response == (char *) NULL ) {
		response_ptr = local_response_buffer;
		response_length = sizeof(local_response_buffer);
	} else {
		response_ptr = response;
		response_length = max_response_length;
	}

	ad = pilatus->record->record_class_struct;

	rs232 = (MX_RS232 *) pilatus->rs232_record->record_class_struct;

	if ( (pilatus->pilatus_flags) & MXF_PILATUS_DEBUG_SERIAL ) {
		debug_flag = TRUE;
	} else {
		debug_flag = FALSE;
	}

	/* Send the command string, if present. */

	if ( command != (char *) NULL ) {
	    command_length = strlen( command ) + 1;

	    if ( command_length > sizeof(command_buffer) ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The length of the command (%lu bytes) exceeds the "
			"length of the internal command buffer (%lu bytes).  "
			"command = '%s'.",
				(unsigned long) command_length,
				(unsigned long) sizeof(command_buffer),
				command );
	    }

	    /* Copy the command to the internal command buffer. */

	    strlcpy( command_buffer, command, sizeof(command_buffer) );

	    if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'.",
		fname, command_buffer, pilatus->record->name ));
	    }

	    if ( pilatus->pilatus_flags & MXF_PILATUS_APPEND_PAD_TO_COMMANDS ) {

	    	/* FIXME: Sometimes the tail end of commands sent by me several
		 * commands ago unexpectedly show up again in the Pilatus's
		 * command buffer, triggering a 1 ERR *** Unrecognized command:
		 * error.  My working hypothesis is that somewhere in the
		 * Pilatus-provided software there is something that is not
		 * always clearing out an internal buffer.  For now, my
		 * attempted workaround is to append a bunch of space characters
		 * onto the end of all the commands I send to the Pilatus.
		 * Not sure if this will work.
		 */

		strlcat( command_buffer,
			"                                        ",
			sizeof(command_buffer) );
	    }

	    /* Note: For some types of serial interfaces, such as sockets,
	     * the line terminators for a command sometimes may be in a
	     * different packet than the packet that sends the body of the 
	     * command.  It seems that Pilatus does not like this, so we
	     * append the line terminators to the command buffer and then
	     * send the command with the mx_rs232_write() function rather
	     * than the mx_rs232_putline() function.  If we do this, then
	     * the line terminators will probably end up in the same packet
	     * as the command.  But the TCP standard, does _not_ guarantee this.
	     */

	    strlcat( command_buffer, rs232->write_terminator_array,
					sizeof(command_buffer) );

	    /* Send the command. */

	    mx_status = mx_rs232_write( pilatus->rs232_record,
			command_buffer, strlen(command_buffer), NULL, 0 );

	    if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	    /* End of command processing. */
	}

	/* Now read back the response line.
	 *
	 * The response line we want may be preceded by an asynchronous
	 * notification such as '7 OK' (exposure complete).  If we get
	 * an asynchronous notification, then we have to go back and
	 * read another line for the response we actually want.
	 */

	max_attempts = 10;

	for ( n = 0; n < max_attempts; n++ ) {

	    mx_status = mx_rs232_getline_with_timeout( pilatus->rs232_record,
					response_ptr, response_length,
					&num_response_bytes, 0,
					rs232->timeout );

	    mx_status_code = mx_status.code & (~MXE_QUIET);

#if 1
	    MX_DEBUG(-2,("%s: *** n = %lu, mx_status_code = %lu, "
		"num_response_bytes = %ld, response = '%s'",
		fname, n, mx_status_code,
		(long) num_response_bytes, response_ptr));
#endif

	    if ( mx_status_code == MXE_SUCCESS ) {
		if ( strncmp( response_ptr, "7 OK", 4 ) == 0 ) {
			if ( pilatus->exposure_in_progress ) {
				pilatus->exposure_in_progress = FALSE;
				ad->total_num_frames++;
			}
		} else {
			break;	/* Exit the for() loop. */
		}
	    } else
	    if ( mx_status_code == MXE_TIMED_OUT ) {
		if ( num_response_bytes > 0 ) {
			/* Make sure that the response is null terminated
			 * and then break out of the retry loop.
			 */
			response_ptr[num_response_bytes] = 0;

			break;	/* Exit the for() loop. */
		}
		/* If num_response_bytes was 0, then go back and try again. */

	    } else {
		return mx_status;
	    }
	}

	if ( n >= max_attempts ) {
		return mx_error( MXE_TIMED_OUT, fname,
		"Pilatus '%s' response to command '%s' timed out after "
		"%lu retries of %g seconds each for a total of %g seconds.",
			pilatus->record->name, command_buffer,
			max_attempts, rs232->timeout,
			max_attempts * rs232->timeout );
	}

	/*--- Split off the return code and the error status. ---*/

#if MXD_PILATUS_DEBUG_RESPONSE
	MX_DEBUG(-2,("%s: #0 command_buffer = '%s', response_ptr = '%s'",
		fname, command_buffer, response_ptr));
#endif

	/* 1. Skip over any leading spaces. */

	length = strspn( response_ptr, " " );

	return_code_arg = response_ptr + length;

#if MXD_PILATUS_DEBUG_RESPONSE
	MX_DEBUG(-2,("%s: #1 >> return_code_arg = '%s'",
		fname, return_code_arg));
#endif

	/* 2. Find the end of the return code string and null terminate it. */

	length = strcspn( return_code_arg, " " );

	ptr = return_code_arg + length;

	*ptr = '\0';

	ptr++;

#if MXD_PILATUS_DEBUG_RESPONSE
	MX_DEBUG(-2,("%s: #2 >> ptr = '%s'", fname, ptr));
#endif

	/* 3. Parse the return code string. */

	return_code = atol( return_code_arg );

	if ( pilatus_return_code != (unsigned long *) NULL ) {
		*pilatus_return_code = return_code;
	}

#if MXD_PILATUS_DEBUG_RESPONSE
	MX_DEBUG(-2,("%s: #3 >> return_code = %ld", fname, return_code));
#endif

	/* 4. Find the start of the error status argument. */

	length = strspn( ptr, " " );

	error_status_arg = ptr + length;

#if MXD_PILATUS_DEBUG_RESPONSE
	MX_DEBUG(-2,("%s: #4 >> error_status_arg = '%s'",
		fname, error_status_arg));
#endif

	/* 5. Find the end of the error status argument and null terminate it.*/

	length = strcspn( error_status_arg, " " );

	ptr = error_status_arg + length;

	*ptr = '\0';

	ptr++;

#if MXD_PILATUS_DEBUG_RESPONSE
	MX_DEBUG(-2,("%s: #5 >> ptr = '%s'", fname, ptr));
#endif

	strlcpy( error_status_string, error_status_arg,
			sizeof(error_status_string) );

	/* If our caller wants to see the response string from the Pilatus,
 	 * then we arrange for that here.
 	 */

	if ( response != (char *) NULL ) {

	    /* 6. Find the beginning of the remaining text of the response. */

	    length = strspn( ptr, " " );

	    ptr += length;

#if MXD_PILATUS_DEBUG_RESPONSE
	    MX_DEBUG(-2,("%s: #6 >> ptr = '%s'", fname, ptr));
#endif

	    /* 7. Move the remaining text to the beginning of the
 	     * response buffer.
 	     */

	    bytes_to_move = strlen( ptr );

	    if ( bytes_to_move >= (response_length + ( ptr-response )) ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"The string received from Pilatus detector '%s' "
		"was not null terminated.", pilatus->record->name );
	    }

	    memmove( response, ptr, bytes_to_move );

#if MXD_PILATUS_DEBUG_RESPONSE
	    MX_DEBUG(-2,("%s: #7 >> response = '%s'", fname, response));
#endif

	    /* 8.  Null out the rest of the response buffer. */

	    memset( response + bytes_to_move, '\0',
			response_length - bytes_to_move );

#if MXD_PILATUS_DEBUG_RESPONSE
	    MX_DEBUG(-2,("%s: #8 >> response = '%s'", fname, response));
#endif

	    /*---*/

	    if ( strcmp( error_status_string, "OK" ) != 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	    "Command '%s' sent to detector '%s' returned an error (%lu) '%s'.",
			command, pilatus->record->name,
			return_code, response );
	    }

	    /* End of response processing. */
	}

	/*---*/

	if (debug_flag) {
	    if ( response != (char *) NULL ) {
		MX_DEBUG(-2,("%s: received (%lu %s) '%s' from '%s'.",
			fname, return_code, error_status_string,
			response, pilatus->record->name ));
	    } else {
		MX_DEBUG(-2,("%s: received (%lu %s) from '%s'.",
			fname, return_code, error_status_string,
			pilatus->record->name ));
	    }
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================================================================*/

MX_EXPORT mx_status_type
mxd_pilatus_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_PILATUS_COMMAND:
		case MXLV_PILATUS_DELAY_TIME:
		case MXLV_PILATUS_DETECTOR_SERVER_DATAFILE_DIRECTORY:
		case MXLV_PILATUS_DETECTOR_SERVER_DATAFILE_ROOT:
		case MXLV_PILATUS_DETECTOR_SERVER_DATAFILE_USER:
		case MXLV_PILATUS_EXPOSURE_PERIOD:
		case MXLV_PILATUS_EXPOSURES_PER_FRAME:
		case MXLV_PILATUS_GAP_TIME:
		case MXLV_PILATUS_LOCAL_DATAFILE_ROOT:
		case MXLV_PILATUS_LOCAL_DATAFILE_USER:
		case MXLV_PILATUS_NUM_IMAGES:
		case MXLV_PILATUS_REAL_EXPOSURE_TIME:
		case MXLV_PILATUS_RELOAD_GRIMSEL:
		case MXLV_PILATUS_RESPONSE:
		case MXLV_PILATUS_SET_ENERGY:
		case MXLV_PILATUS_SET_THRESHOLD:
		case MXLV_PILATUS_TH:
			record_field->process_function
					= mxd_pilatus_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================================================================*/

static mx_status_type
mxd_pilatus_process_function( void *record_ptr,
			void *record_field_ptr,
			void *socket_handler_ptr,
			int operation )
{
	static const char fname[] = "mxd_pilatus_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_PILATUS *pilatus;
	char command[MXU_PILATUS_COMMAND_LENGTH+1];
	char response[MXU_PILATUS_COMMAND_LENGTH+1];
	int num_items;
	int argc;
	char **argv;
	int is_subdirectory;
	unsigned long pilatus_flags;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	pilatus = (MX_PILATUS *) record->record_type_struct;

	pilatus_flags = pilatus->pilatus_flags;
	
	mx_status = MX_SUCCESSFUL_RESULT;

#if MXD_PILATUS_DEBUG_PARAMETERS
	switch( operation ) {
	case MX_PROCESS_GET:
		MX_DEBUG(-2,("%s: GET: record '%s', parameter '%s' (%ld)",
			fname, record->name, record_field->name,
			record_field->label_value ));
		break;
	case MX_PROCESS_PUT:
		MX_DEBUG(-2,("%s: PUT: record '%s', parameter '%s' (%ld)",
			fname, record->name, record_field->name,
			record_field->label_value ));
		break;
	}
#endif

#if MXD_PILATUS_DEBUG_DIRECTORY_MAPPING
	mstest_pilatus_show_datafile_directory( fname, "before" );
#endif

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_PILATUS_DELAY_TIME:
			mx_status = mxd_pilatus_command( pilatus, "Delay",
					response, sizeof(response), NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response,
					"Delay time set to: %lg",
					&(pilatus->delay_time) );

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Did not find the delay time in response '%s' "
				"from Pilatus detector '%s'.",
					response, record->name );
			}
			break;
		case MXLV_PILATUS_REAL_EXPOSURE_TIME:
			mx_status = mxd_pilatus_command( pilatus, "ExpTime",
					response, sizeof(response), NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response,
					"Exposure time set to: %lg",
					&(pilatus->real_exposure_time) );

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Did not find the exposure time in "
				"response '%s' from Pilatus detector '%s'.",
					response, record->name );
			}
			break;
		case MXLV_PILATUS_EXPOSURE_PERIOD:
			mx_status = mxd_pilatus_command( pilatus, "ExpPeriod",
					response, sizeof(response), NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response,
					"Exposure period set to: %lg",
					&(pilatus->exposure_period) );

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Did not find the exposure period in "
				"response '%s' from Pilatus detector '%s'.",
					response, record->name );
			}
			MX_DEBUG(-2,("%s: exposure period = %g",
				fname, pilatus->exposure_period));
			break;
		case MXLV_PILATUS_GAP_TIME:
			break;
		case MXLV_PILATUS_NUM_IMAGES:
			mx_status = mxd_pilatus_command( pilatus, "NImages",
					response, sizeof(response), NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response,
					"N images set to: %lu",
					&(pilatus->num_images) );

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Did not find the number of images in "
				"response '%s' from Pilatus detector '%s'.",
					response, record->name );
			}
			break;
		case MXLV_PILATUS_EXPOSURES_PER_FRAME:
			mx_status = mxd_pilatus_command( pilatus, "NExpFrame",
					response, sizeof(response), NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response,
					"Exposures per frame set to: %lu",
					&(pilatus->exposures_per_frame) );

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Did not find the exposures per frame in "
				"response '%s' from Pilatus detector '%s'.",
					response, record->name );
			}
			break;
		case MXLV_PILATUS_COMMAND:
		case MXLV_PILATUS_RESPONSE:
		case MXLV_PILATUS_SET_ENERGY:
		case MXLV_PILATUS_SET_THRESHOLD:
			/* We just return whatever was most recently
			 * written to these fields.
			 */
			break;
		case MXLV_PILATUS_DETECTOR_SERVER_DATAFILE_ROOT:
		case MXLV_PILATUS_DETECTOR_SERVER_DATAFILE_USER:
			/* We just report back the current value for 
			 * these variables.
			 */
			break;
		case MXLV_PILATUS_DETECTOR_SERVER_DATAFILE_DIRECTORY:

			if ( pilatus->exposure_in_progress ) {

				/* If an exposure sequence is currently in
				 * progress, then we cannot send an 'ImgPath'
				 * command to the PPU.  The best we can do
				 * in this circumstances is to merely reuse
				 * the value that is already in the
				 * pilatus->detector_server_datafile_directory
				 * array.
				 */

				MX_DEBUG(-2,
			("%s: EIP detector_server_datafile_directory = '%s'",
			fname, pilatus->detector_server_datafile_directory));

				return MX_SUCCESSFUL_RESULT;
			} else {
				mx_status = mxd_pilatus_command( pilatus,
						"ImgPath",
						response, sizeof(response),
						NULL );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				strlcpy(
			  pilatus->detector_server_datafile_directory, response,
			  sizeof(pilatus->detector_server_datafile_directory) );
			}
			break;
		case MXLV_PILATUS_TH:
			snprintf( command, sizeof(command),
				"THread %lu", pilatus->th_channel );

			mx_status = mxd_pilatus_command( pilatus, command,
						response, sizeof(response),
						NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_string_split( response, " ", &argc, &argv );

			pilatus->th[0] = atof( argv[4] );
			pilatus->th[1] = atof( argv[8] );

			mx_free( argv );
			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_PILATUS_DETECTOR_SERVER_DATAFILE_ROOT:
			/* This is the top level directory for image files
			 * on the detector computer.
			 */
			break;
		case MXLV_PILATUS_DETECTOR_SERVER_DATAFILE_USER:
			/* Test: Is datafile_user within datafile_root? */

			mx_status = mx_is_subdirectory(
					pilatus->detector_server_datafile_root,
					pilatus->detector_server_datafile_user,
					&is_subdirectory );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( is_subdirectory == FALSE ) {
			   mx_status = mx_error(
			    MXE_CONFIGURATION_CONFLICT,fname,
			    "Requested directory '%s' for "
			    "'%s.detector_server_datafile_user' is not a "
			    "subdirectory of '%s' for "
			    "'%s.detector_server_datafile_root'.  Record field "
			    "'%s.detector_server_datafile_user' will be "
			    "set to '%s'.",
				pilatus->detector_server_datafile_user,
				record->name,
				pilatus->detector_server_datafile_root,
				record->name,
				record->name,
				pilatus->detector_server_datafile_root);

			   strlcpy( pilatus->detector_server_datafile_user,
			      pilatus->detector_server_datafile_root,
			   sizeof(pilatus->detector_server_datafile_user));
			}
			break;
		case MXLV_PILATUS_DETECTOR_SERVER_DATAFILE_DIRECTORY:

			MX_DEBUG(-2,
		("%s: PUT pilatus->detector_server_datafile_directory = '%s'",
			  fname, pilatus->detector_server_datafile_directory));

			/* Test: Is datafile_directory within datafile_user? */

			mx_status = mx_is_subdirectory(
				pilatus->detector_server_datafile_user,
				pilatus->detector_server_datafile_directory,
					&is_subdirectory );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( is_subdirectory == FALSE ) {
			   mx_status = mx_error(
			    MXE_CONFIGURATION_CONFLICT,fname,
			    "Requested directory '%s' for "
			    "'%s.detector_server_datafile_directory' is not a "
			    "subdirectory of '%s' for "
			    "'%s.detector_server_datafile_user'.  Record field "
			    "'%s.detector_server_datafile_directory' will be "
			    "set to '%s'.",
				pilatus->detector_server_datafile_directory,
				record->name,
				pilatus->detector_server_datafile_user,
				record->name,
				record->name,
				pilatus->detector_server_datafile_user);

			   strlcpy( pilatus->detector_server_datafile_directory,
			      pilatus->detector_server_datafile_user,
			   sizeof(pilatus->detector_server_datafile_directory));
			}

			/* Now we can send the new ImgPath. */

			snprintf( command, sizeof(command), "ImgPath %s",
				pilatus->detector_server_datafile_directory );

			mx_status = mxd_pilatus_command( pilatus, command,
						response, sizeof(response),
						NULL );
			break;
		case MXLV_PILATUS_LOCAL_DATAFILE_ROOT:
			/* This is the top level directory for image files
			 * on the PPU computer which runs the MX server.
			 */
			break;
		case MXLV_PILATUS_LOCAL_DATAFILE_USER:
			mx_status = mx_is_subdirectory(
				pilatus->local_datafile_root,
				pilatus->local_datafile_user,
					&is_subdirectory );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( is_subdirectory == FALSE ) {
			   mx_status = mx_error(
			    MXE_CONFIGURATION_CONFLICT,fname,
			    "Requested directory '%s' for "
			    "'%s.local_datafile_user' is not a "
			    "subdirectory of '%s' for "
			    "'%s.local_datafile_root'.  Record field "
			    "'%s.local_datafile_user' will be "
			    "set to '%s'.",
				pilatus->local_datafile_user,
				record->name,
				pilatus->local_datafile_root,
				record->name,
				record->name,
				pilatus->local_datafile_root);

			   strlcpy( pilatus->local_datafile_user,
			      pilatus->local_datafile_root,
			   sizeof(pilatus->local_datafile_user));
			}
			break;
		case MXLV_PILATUS_RELOAD_GRIMSEL:
			if ( pilatus_flags &
				MXF_PILATUS_READ_GRIMSEL_DECTRIS_CONF )
			{
				mx_status = 
			mxd_pilatus_get_target_directory_from_grimsel(pilatus);

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
			break;
		case MXLV_PILATUS_COMMAND:
			mx_status = mxd_pilatus_command( pilatus,
						pilatus->command,
						pilatus->response,
						MXU_PILATUS_COMMAND_LENGTH,
						NULL );
			break;
		case MXLV_PILATUS_SET_ENERGY:
			snprintf( command, sizeof(command),
			"SetEnergy %f", pilatus->set_energy );

			mx_status = mxd_pilatus_command( pilatus, command,
						response, sizeof(response),
						NULL );
			break;
		case MXLV_PILATUS_SET_THRESHOLD:
			snprintf( command, sizeof(command),
			"SetThreshold %s", pilatus->set_threshold );

			mx_status = mxd_pilatus_command( pilatus, command,
						response, sizeof(response),
						NULL );
			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d for record '%s'.",
			operation, record->name );
		break;
	}

#if MXD_PILATUS_DEBUG_DIRECTORY_MAPPING
	mstest_pilatus_show_datafile_directory( fname, "after" );
#endif

	return mx_status;
}

