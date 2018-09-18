/*
 * Name:    d_pilatus.c
 *
 * Purpose: MX area detector driver for Dectris Pilatus detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015-2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PILATUS_DEBUG				FALSE

#define MXD_PILATUS_DEBUG_SERIAL			FALSE

#define MXD_PILATUS_DEBUG_EXTENDED_STATUS		FALSE

#define MXD_PILATUS_DEBUG_EXTENDED_STATUS_CHANGE	FALSE

#define MXD_PILATUS_DEBUG_EXTENDED_STATUS_PARSING	FALSE

#define MXD_PILATUS_DEBUG_PARAMETERS			FALSE

#include <stdio.h>
#include <stdlib.h>

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

MX_AREA_DETECTOR_FUNCTION_LIST
mxd_pilatus_ad_function_list = {
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
	char command[100];
	char response[500];
	char *ptr;
	size_t i, length;
	char *buffer_ptr, *line_ptr;
	int num_items;
	unsigned long mask;
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

	ad->trigger_mode = MXT_IMAGE_INTERNAL_TRIGGER;

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
	unsigned long num_frames;
	double exposure_time, exposure_period, delay_time;
	char command[MXU_PILATUS_COMMAND_LENGTH+1];
	char response[MXU_PILATUS_COMMAND_LENGTH+1];
	mx_status_type mx_status;

	pilatus = NULL;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif
	pilatus->old_pilatus_image_counter = 0;

	sp = &(ad->sequence_parameters);

	if ( ad->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) {
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

	/* Set the exposure sequence parameters. */

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		num_frames = 1;
		exposure_time = sp->parameter_array[0];
		exposure_period = -1.0;
		break;
	case MXT_SQ_MULTIFRAME:
		num_frames = mx_round( sp->parameter_array[0] );
		exposure_time = sp->parameter_array[1];
		exposure_period = sp->parameter_array[2];
		break;
	case MXT_SQ_STROBE:
		num_frames = mx_round( sp->parameter_array[0] );
		exposure_time = sp->parameter_array[1];
		exposure_period = -1.0;
		break;
	case MXT_SQ_DURATION:
		num_frames = mx_round( sp->parameter_array[0] );
		exposure_time = 1.0;		/* Not used. */
		exposure_period = -1.0;
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Detector '%s' sequence types other than 'one-shot' "
		"are not yet implemented.", ad->record->name );
		break;
	}

	snprintf( command, sizeof(command), "NImages %lu", num_frames );

	mx_status = mxd_pilatus_command( pilatus, command,
				response, sizeof(response), NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "ExpTime %f", exposure_time );

	mx_status = mxd_pilatus_command( pilatus, command,
				response, sizeof(response), NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the exposure period if not explicitly set above. */

	if ( exposure_period < 0.0 ) {
		if ( pilatus->exposure_period > 0.0 ) {
			exposure_period = pilatus->exposure_period;
		} else
		if ( pilatus->gap_time > 0.0 ) {
			exposure_period = exposure_time + pilatus->gap_time;
		} else {
			exposure_period = exposure_time + 0.0025;
		}
	}

	/* Set the exposure period. */

	snprintf( command, sizeof(command), "ExpPeriod %f", exposure_period );

	mx_status = mxd_pilatus_command( pilatus, command,
				response, sizeof(response), NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute and set the exposure delay. */

	if ( pilatus->delay_time > 0.0 ) {
		if ( pilatus->delay_time > exposure_period ) {
			delay_time = exposure_period;
		} else {
			delay_time = pilatus->delay_time;
		}
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

	if ( ( ad->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER ) == 0 ) {
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

	if ( ( ad->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) == 0 ) {
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
				response, sizeof(response), NULL );

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
					response, sizeof(response), NULL );

	return mx_status;
}

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
		} else {
			mx_warning(
			"%s: Unexpected response '%s' seen for detector '%s'",
				fname, response, ad->record->name );
		}
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
			 * pilatus->detector_server_datafile_directory array.
			 */

			return MX_SUCCESSFUL_RESULT;
		} else {
			mx_status = mxd_pilatus_command( pilatus, "ImgPath",
					response, sizeof(response),
					&pilatus_return_code );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			strlcpy( pilatus->detector_server_datafile_directory,
			    response,
			    sizeof(pilatus->detector_server_datafile_directory) );
		}

		mx_status = mx_change_filename_prefix(
				pilatus->detector_server_datafile_directory,
				pilatus->detector_server_datafile_root,
				pilatus->local_datafile_root,
				ad->datafile_directory,
				sizeof(ad->datafile_directory) );
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
		mx_status = mx_change_filename_prefix(
				ad->datafile_directory,
				pilatus->local_datafile_root,
				pilatus->detector_server_datafile_root,
				pilatus->detector_server_datafile_directory,
			    sizeof(pilatus->detector_server_datafile_directory) );

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

#define BAD_WOLF	TRUE

MX_EXPORT mx_status_type
mxd_pilatus_command( MX_PILATUS *pilatus,
			char *command,
			char *response,
			size_t response_buffer_length,
			unsigned long *pilatus_return_code )
{
	static const char fname[] = "mxd_pilatus_command()";

	MX_AREA_DETECTOR *ad = NULL;
	char error_status_string[20];
	char *ptr, *return_code_arg, *error_status_arg;
	size_t length, bytes_to_move;
	unsigned long return_code;
	mx_bool_type debug_flag;
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

	if ( (pilatus->pilatus_flags) & MXF_PILATUS_DEBUG_SERIAL ) {
		debug_flag = TRUE;
	} else {
		debug_flag = FALSE;
	}

	/* First send the command. */

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'.",
		fname, command, pilatus->record->name ));
	}

#if BAD_WOLF
	{
		/* For some reason, the version of the Pilatus camserver
		 * that we have sometimes appends garbage to the end of
		 * commands that I send to it.  camserver acts as if it
		 * is reading the commands I send to it into a buffer
		 * that it is not zeroing out every time.
		 *
		 * To work around that, I copy the command string into a
		 * larger buffer and fill the rest of that buffer with
		 * space characters ' ' in the hope that any leftover
		 * characters in camserver will be overwritten.
		 */

		size_t initial_command_length;
		char *temp_command_buffer;
		size_t temp_command_buffer_length;
		char *spaces_ptr;
		size_t spaces_length;

		initial_command_length = strlen( command );

		temp_command_buffer_length = initial_command_length + 100;

		temp_command_buffer = malloc( temp_command_buffer_length );

		if ( temp_command_buffer == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"The attempt to allocate a %lu byte temporary "
			"command buffer for detector '%s' to work around "
			"a 'camserver' bug failed.",
				(unsigned long) temp_command_buffer_length,
				pilatus->record->name );
		}

		strlcpy( temp_command_buffer,
			command, temp_command_buffer_length );

		spaces_ptr = temp_command_buffer + initial_command_length;

		spaces_length = temp_command_buffer_length
					- initial_command_length - 1;

		memset( spaces_ptr, ' ', spaces_length );

		spaces_ptr[spaces_length - 1] = '\0';

		mx_status = mx_rs232_putline( pilatus->rs232_record,
					temp_command_buffer, NULL, 0 );

		mx_free( temp_command_buffer );
	}
#else
	mx_status = mx_rs232_putline( pilatus->rs232_record,
					command, NULL, 0 );
#endif

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
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
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
		case MXLV_PILATUS_DETECTOR_SERVER_IMAGE_DIRECTORY:
		case MXLV_PILATUS_DETECTOR_SERVER_IMAGE_ROOT:
		case MXLV_PILATUS_LOCAL_IMAGE_ROOT:
		case MXLV_PILATUS_COMMAND:
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
			int operation )
{
	static const char fname[] = "mxd_pilatus_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_PILATUS *pilatus;
	char command[MXU_PILATUS_COMMAND_LENGTH+1];
	char response[MXU_PILATUS_COMMAND_LENGTH+1];
	int argc;
	char **argv;
	char directory_temp[MXU_FILENAME_LENGTH+1];
	char *ptr = NULL;
	size_t length;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	pilatus = (MX_PILATUS *) record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_PILATUS_COMMAND:
		case MXLV_PILATUS_RESPONSE:
		case MXLV_PILATUS_SET_ENERGY:
		case MXLV_PILATUS_SET_THRESHOLD:
			/* We just return whatever was most recently
			 * written to these fields.
			 */
			break;
		case MXLV_PILATUS_DETECTOR_SERVER_IMAGE_DIRECTORY:

			if ( pilatus->exposure_in_progress ) {

				/* If an exposure sequence is currently in
				 * progress, then we cannot send an 'ImgPath'
				 * command to the PPU.  The best we can do
				 * in this circumstances is to merely reuse
				 * the value that is already in the
				 * pilatus->detector_server_datafile_directory
				 * array.
				 */

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
		case MXLV_PILATUS_DETECTOR_SERVER_IMAGE_DIRECTORY:

			/* See if the new directory is contained within
 			 * the directory pilatus->detector_server_datafile_root.
 			 */

			/* Begin by making a copy of detector_server_datafile_root
			 * with any trailing path separators stripped off.
			 */

			strlcpy( directory_temp,
				pilatus->detector_server_datafile_root,
				sizeof(directory_temp) );

			length = strlen( directory_temp );

			if ( ( directory_temp[length-1] == '/' )
			  || ( directory_temp[length-1] == '\\' ) )
			{
				directory_temp[length-1] = '\0';

				length = strlen( directory_temp );
			}

			/* Is directory_temp found at the beginning of
			 * detector_server_datafile_directory?
			 */

			ptr = strstr( pilatus->detector_server_datafile_directory,
					directory_temp );

			if ( (ptr == NULL)
			  || (ptr != pilatus->detector_server_datafile_directory) )
			{
				return mx_error(
				MXE_CONFIGURATION_CONFLICT, fname,
				"The requested detector server image directory "
				"of '%s' for Pilatus detector '%s' is not "
				"found within the detector server image root "
				"directory of '%s'.",
				    pilatus->detector_server_datafile_directory,
				    pilatus->record->name,
				    pilatus->detector_server_datafile_root );
			}

			/* Now we can send the new ImgPath. */

			snprintf( command, sizeof(command), "ImgPath %s",
				pilatus->detector_server_datafile_directory );

			mx_status = mxd_pilatus_command( pilatus, command,
						response, sizeof(response),
						NULL );
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

	return mx_status;
}

