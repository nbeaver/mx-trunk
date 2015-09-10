/*
 * Name:    d_pilatus.c
 *
 * Purpose: MX area detector driver for Dectris Pilatus detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PILATUS_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_hrt.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "d_pilatus.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_pilatus_record_function_list = {
	mxd_pilatus_initialize_driver,
	mxd_pilatus_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_pilatus_open,
};

MX_AREA_DETECTOR_FUNCTION_LIST
mxd_pilatus_ad_function_list = {
	mxd_pilatus_arm,
	mxd_pilatus_trigger,
	NULL,
	mxd_pilatus_stop,
	mxd_pilatus_abort,
	mxd_pilatus_get_last_frame_number,
	mxd_pilatus_get_total_num_frames,
	mxd_pilatus_get_status,
	NULL,
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
	mxd_pilatus_setup_exposure,
	mxd_pilatus_trigger_exposure
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

	*pilatus = (MX_PILATUS *)
				ad->record->record_type_struct;

	if ( *pilatus == (MX_PILATUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_PILATUS pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
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

	pilatus = (MX_PILATUS *)
				malloc( sizeof(MX_PILATUS) );

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

	pilatus->pilatus_debug_flag = FALSE;
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
	char response[500];
	char *ptr;
	size_t i, length;
	char *buffer_ptr, *line_ptr;
	int num_items;
	unsigned long mask;
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

	(void) mx_rs232_discard_unwritten_output( pilatus->rs232_record,
						pilatus->pilatus_debug_flag );

	(void) mx_rs232_discard_unread_input( pilatus->rs232_record,
						pilatus->pilatus_debug_flag );

	/* Get the version of the TVX software. */

	mx_status = mxd_pilatus_command( pilatus, "Version",
				response, sizeof(response),
				NULL, pilatus->pilatus_debug_flag );

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
				response, sizeof(response),
				NULL, pilatus->pilatus_debug_flag );

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
				response, sizeof(response),
				NULL, pilatus->pilatus_debug_flag );

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

	/* Set generic area detector parameters. */

	ad->maximum_frame_number = 0;
	ad->last_frame_number = -1;
	ad->total_num_frames = 0;
	ad->status = 0;

	ad->bytes_per_pixel = 2;
	ad->bits_per_pixel = 16;

	ad->bytes_per_frame =
	  mx_round( ad->bytes_per_pixel * ad->framesize[0] * ad->framesize[1] );

	ad->image_format = MXT_IMAGE_FORMAT_GREY16;

	mx_status = mx_image_get_image_format_name_from_type(
						ad->image_format,
						ad->image_format_name,
						MXU_IMAGE_FORMAT_NAME_LENGTH );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure automatic saving or readout of image frames. */

	mask = MXF_AD_SAVE_FRAME_AFTER_ACQUISITION
		| MXF_AD_READOUT_FRAME_AFTER_ACQUISITION;

	if ( ad->area_detector_flags & mask ) {
		mx_status =
		  mx_area_detector_setup_datafile_management( record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if MXD_PILATUS_DEBUG
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
	double exposure_time;
	char command[80];
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

	sp = &(ad->sequence_parameters);

	/* Set the exposure sequence parameters. */

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		exposure_time = sp->parameter_array[0];
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Detector '%s' sequence types other than 'one-shot' "
		"are not yet implemented.", ad->record->name );
		break;
	}

	snprintf( command, sizeof(command), "ExpTime %f", exposure_time );

	mx_status = mxd_pilatus_command( pilatus, command,
				response, sizeof(response),
				NULL, pilatus->pilatus_debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Enable the shutter. */

	mx_status = mxd_pilatus_command( pilatus, "ShutterEnable 1",
				response, sizeof(response),
				NULL, pilatus->pilatus_debug_flag );

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

	/*---*/

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
	/* Check to see if we currently have a valid datafile name. */

	if ( strlen( ad->datafile_name ) == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"No datafile name has been specified for the area detector "
		"field '%s.datafile_name'.", ad->record->name );
	}

	/* Start the exposure. */

	snprintf( command, sizeof(command), "Exposure %s", ad->datafile_name );

	mx_status = mxd_pilatus_command( pilatus, command,
				response, sizeof(response),
				NULL, pilatus->pilatus_debug_flag );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_stop()";

	MX_PILATUS *pilatus = NULL;
	char response[80];
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	pilatus->exposure_in_progress = FALSE;

	mx_status = mxd_pilatus_command( pilatus, "Stop",
				response, sizeof(response),
				NULL, pilatus->pilatus_debug_flag );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_abort( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_abort()";

	MX_PILATUS *pilatus = NULL;
	char response[80];
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	pilatus->exposure_in_progress = FALSE;

	mx_status = mxd_pilatus_command( pilatus, "K",
				response, sizeof(response),
				NULL, pilatus->pilatus_debug_flag );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_get_last_frame_number( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_pilatus_get_last_frame_number()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s: area detector '%s', last_frame_number = %ld",
		fname, ad->record->name, ad->last_frame_number ));
#endif

	if ( pilatus->exposure_in_progress ) {
		ad->last_frame_number = -1;
	} else {
		ad->last_frame_number = 0;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_get_total_num_frames( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_pilatus_get_total_num_frames()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s: area detector '%s', total_num_frames = %ld",
		fname, ad->record->name, ad->total_num_frames ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_get_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_get_status()";

	MX_PILATUS *pilatus = NULL;
	char response[80];
	unsigned long num_input_bytes_available;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG( 2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

#if 0
	mx_status = mxd_pilatus_command( pilatus, "Status",
				response, sizeof(response),
				NULL, pilatus->pilatus_debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#else
	mx_status = mx_rs232_num_input_bytes_available( pilatus->rs232_record,
						&num_input_bytes_available );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_input_bytes_available > 0 ) {
		mx_status = mx_rs232_getline( pilatus->rs232_record,
					response, sizeof(response),
					NULL, pilatus->pilatus_debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( strncmp( response, "7 OK", 4 ) == 0 ) {
			if ( pilatus->exposure_in_progress ) {
				pilatus->exposure_in_progress = FALSE;
				ad->total_num_frames++;
			}
		}
	}
#endif

	ad->status = 0;

	if ( pilatus->exposure_in_progress ) {
		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	}

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s: ad->status = %#lx", fname, ad->status));
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
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s', transfer_frame = %ld.",
		fname, ad->record->name, ad->transfer_frame ));
#endif

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
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
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
#if MXD_PILATUS_DEBUG
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

#if MXD_PILATUS_DEBUG
		MX_DEBUG(-2,("%s: GET sequence_type = %ld",
			fname, ad->sequence_parameters.sequence_type));
#endif

#if MXD_PILATUS_DEBUG
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

	case MXLV_AD_CORRECTION_LOAD_FORMAT:
	case MXLV_AD_CORRECTION_LOAD_FORMAT_NAME:
		break;

	case MXLV_AD_CORRECTION_SAVE_FORMAT:
	case MXLV_AD_CORRECTION_SAVE_FORMAT_NAME:
		break;

	case MXLV_AD_DATAFILE_LOAD_FORMAT:
	case MXLV_AD_DATAFILE_LOAD_FORMAT_NAME:
		break;

	case MXLV_AD_DATAFILE_SAVE_FORMAT:
	case MXLV_AD_DATAFILE_SAVE_FORMAT_NAME:
		break;

	case MXLV_AD_DATAFILE_ALLOW_OVERWRITE:
		break;

	case MXLV_AD_DATAFILE_AUTOSELECT_NUMBER:
		break;

	case MXLV_AD_DATAFILE_DIRECTORY:
		mx_status = mxd_pilatus_command( pilatus, "ImgPath",
					response, sizeof(response),
					&pilatus_return_code,
					pilatus->pilatus_debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		strlcpy( ad->datafile_directory, response,
			sizeof( ad->datafile_directory ) );
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

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
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

	case MXLV_AD_SEQUENCE_CONTINUOUS:
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

	case MXLV_AD_GEOM_CORR_AFTER_FLOOD:
		break;

	case MXLV_AD_CORRECTION_FRAME_GEOM_CORR_LAST:
		break;

	case MXLV_AD_CORRECTION_FRAME_NO_GEOM_CORR:
		break;

	case MXLV_AD_REGISTER_VALUE:
		break;

	case MXLV_AD_SHUTTER_ENABLE:
		break;

	case MXLV_AD_CORRECTION_LOAD_FORMAT:
		break;

	case MXLV_AD_CORRECTION_LOAD_FORMAT_NAME:
		break;

	case MXLV_AD_CORRECTION_SAVE_FORMAT:
		break;

	case MXLV_AD_CORRECTION_SAVE_FORMAT_NAME:
		break;

	case MXLV_AD_DATAFILE_LOAD_FORMAT:
		break;

	case MXLV_AD_DATAFILE_LOAD_FORMAT_NAME:
		break;

	case MXLV_AD_DATAFILE_SAVE_FORMAT:
		break;

	case MXLV_AD_DATAFILE_SAVE_FORMAT_NAME:
		break;

	case MXLV_AD_DATAFILE_ALLOW_OVERWRITE:
		break;

	case MXLV_AD_DATAFILE_AUTOSELECT_NUMBER:
		break;

	case MXLV_AD_DATAFILE_DIRECTORY:
		snprintf( command, sizeof(command),
			"ImgPath %s", ad->datafile_directory );

		mx_status = mxd_pilatus_command( pilatus, command,
					response, sizeof(response),
					NULL, pilatus->pilatus_debug_flag );
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
	case MXFT_AD_FLOOD_FIELD_FRAME:
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
mxd_pilatus_setup_exposure( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_pilatus_setup_exposure()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s', motor '%s', "
	"shutter '%s', trigger '%s', "
	"exposure distance = %f, exposure time = %f",
		fname, ad->record->name, ad->exposure_motor_name,
		ad->shutter_name, ad->exposure_trigger_name,
		ad->exposure_distance, ad->exposure_time ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_trigger_exposure( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_pilatus_trigger_exposure()";

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
			size_t response_buffer_length,
			unsigned long *pilatus_return_code,
			unsigned long debug_flag )
{
	static const char fname[] = "mxd_pilatus_command()";

	MX_AREA_DETECTOR *ad = NULL;
	char error_status_string[20];
	char *ptr, *return_code_arg, *error_status_arg;
	size_t length, bytes_to_move;
	unsigned long return_code;
	mx_status_type mx_status;

	if ( pilatus == (MX_PILATUS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PILATUS pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer passed was NULL." );
	}
	if ( response == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The response pointer passed was NULL." );
	}

	ad = pilatus->record->record_class_struct;

	/* First send the command. */

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'.",
		fname, command, pilatus->record->name ));
	}

	mx_status = mx_rs232_putline( pilatus->rs232_record,
					command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now read back the response line.
	 *
	 * The response line we want may be preceded by an asynchronous
	 * notification such as '7 OK' (exposure complete).  If we get
	 * an asynchronous notification, then we have to go back and
	 * read another line for the response we actually want.
	 */

	while (1) {
		mx_status = mx_rs232_getline( pilatus->rs232_record,
					response, response_buffer_length,
					NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( strncmp( response, "7 OK", 4 ) == 0 ) {
			if ( pilatus->exposure_in_progress ) {
				pilatus->exposure_in_progress = FALSE;
				ad->total_num_frames++;
			}
		} else {
			break;	/* Exit the while() loop. */
		}
	}

	/*--- Split off the return code and the error status. ---*/

	/* 1. Skip over any leading spaces. */

	length = strspn( response, " " );

	return_code_arg = response + length;

	/* 2. Find the end of the return code string and null terminate it. */

	length = strcspn( return_code_arg, " " );

	ptr = return_code_arg + length;

	*ptr = '\0';

	ptr++;

	/* 3. Parse the return code string. */

	return_code = atol( return_code_arg );

	if ( pilatus_return_code != (unsigned long *) NULL ) {
		*pilatus_return_code = return_code;
	}

	/* 4. Find the start of the error status argument. */

	length = strspn( ptr, " " );

	error_status_arg = ptr + length;

	/* 5. Find the end of the error status argument and null terminate it.*/

	length = strcspn( error_status_arg, " " );

	ptr = error_status_arg + length;

	*ptr = '\0';

	ptr++;

	strlcpy( error_status_string, error_status_arg,
			sizeof(error_status_string) );

	/* 6. Find the beginning of the remaining text of the response. */

	length = strspn( ptr, " " );

	ptr += length;

	/* 7. Move the remaining text to the beginning of the response buffer.*/

	bytes_to_move = strlen( ptr );

	if ( bytes_to_move >= (response_buffer_length + ( ptr-response )) ) {
		return mx_error( MXE_UNPARSEABLE_STRING,
		"The string received from Pilatus detector '%s' "
		"was not null terminated.", pilatus->record->name );
	}

	memmove( response, ptr, bytes_to_move );

	response[bytes_to_move] = '\0';

	/*---*/

	if ( strcmp( error_status_string, "OK" ) != 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	    "Command '%s' sent to detector '%s' returned an error (%lu) '%s'.",
			command, pilatus->record->name, return_code, response );
	}

	/*---*/

	if (debug_flag) {
		MX_DEBUG(-2,("%s: received (%lu %s) '%s' from '%s'.",
			fname, return_code, error_status_string,
			response, pilatus->record->name ));
	}

	return MX_SUCCESSFUL_RESULT;
}
