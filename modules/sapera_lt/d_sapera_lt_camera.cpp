/*
 * Name:    d_sapera_lt_camera.cpp
 *
 * Purpose: MX video input driver for a DALSA Sapera LT camera.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013-2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SAPERA_LT_CAMERA_DEBUG				FALSE

#define MXD_SAPERA_LT_CAMERA_DEBUG_OPEN				FALSE

#define MXD_SAPERA_LT_CAMERA_DEBUG_RESYNCHRONIZE		TRUE

#define MXD_SAPERA_LT_CAMERA_DEBUG_FRAME_BUFFER_ALLOCATION	FALSE

#define MXD_SAPERA_LT_CAMERA_DEBUG_EXTENDED_EXPOSURE		FALSE

#define MXD_SAPERA_LT_CAMERA_SHOW_FEATURES			FALSE

#define MXD_SAPERA_LT_CAMERA_DEBUG_ARM				FALSE

#define MXD_SAPERA_LT_CAMERA_DEBUG_TRIGGER			FALSE

#define MXD_SAPERA_LT_CAMERA_DEBUG_GET_FRAME			FALSE

#define MXD_SAPERA_LT_CAMERA_DEBUG_GET_FRAME_LOOKUP		FALSE

#define MXD_SAPERA_LT_CAMERA_DEBUG_EXTENDED_STATUS		FALSE

#define MXD_SAPERA_LT_CAMERA_DEBUG_EXTENDED_STATUS_WHEN_BUSY	FALSE

#define MXD_SAPERA_LT_CAMERA_DEBUG_EXTENDED_STATUS_WHEN_CHANGED	FALSE

#define MXD_SAPERA_LT_CAMERA_DEBUG_NUM_FRAMES_LEFT_TO_ACQUIRE	FALSE

#define MXD_SAPERA_LT_CAMERA_DEBUG_ACQUISITION_CALLBACK		FALSE

#define MXD_SAPERA_LT_CAMERA_DEBUG_MX_PARAMETERS		FALSE

#define MXD_SAPERA_LT_CAMERA_SHOW_FRAME_COUNTER			TRUE

#define MXD_SAPERA_LT_CAMERA_BYPASS_BUFFER_OVERRUN_TEST		TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if defined(OS_WIN32)
#  include <io.h>
#endif

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_record.h"
#include "mx_unistd.h"
#include "mx_inttypes.h"
#include "mx_array.h"
#include "mx_bit.h"
#include "mx_memory.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "i_sapera_lt.h"
#include "d_sapera_lt_camera.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_sapera_lt_camera_record_function_list = {
	NULL,
	mxd_sapera_lt_camera_create_record_structures,
	mxd_sapera_lt_camera_finish_record_initialization,
	NULL,
	NULL,
	mxd_sapera_lt_camera_open,
	mxd_sapera_lt_camera_close,
	NULL,
	mxd_sapera_lt_camera_resynchronize,
	mxd_sapera_lt_camera_special_processing_setup
};

MX_VIDEO_INPUT_FUNCTION_LIST
mxd_sapera_lt_camera_video_input_function_list = {
	mxd_sapera_lt_camera_arm,
	mxd_sapera_lt_camera_trigger,
	mxd_sapera_lt_camera_stop,
	mxd_sapera_lt_camera_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_sapera_lt_camera_get_extended_status,
	mxd_sapera_lt_camera_get_frame,
	mxd_sapera_lt_camera_get_parameter,
	mxd_sapera_lt_camera_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_sapera_lt_camera_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VIDEO_INPUT_STANDARD_FIELDS,
	MXD_SAPERA_LT_CAMERA_STANDARD_FIELDS
};

long mxd_sapera_lt_camera_num_record_fields
	= sizeof( mxd_sapera_lt_camera_record_field_defaults )
	/ sizeof( mxd_sapera_lt_camera_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_sapera_lt_camera_rfield_def_ptr
			= &mxd_sapera_lt_camera_record_field_defaults[0];

/*---*/

static mx_status_type mxd_sapera_lt_camera_process_function( void *record_ptr,
							void *record_field_ptr,
							int operation );

static mx_status_type
mxd_sapera_lt_camera_get_pointers( MX_VIDEO_INPUT *vinput,
			MX_SAPERA_LT_CAMERA **sapera_lt_camera,
			MX_SAPERA_LT **sapera_lt,
			const char *calling_fname )
{
	static const char fname[] =
		"mxd_sapera_lt_camera_get_pointers()";

	MX_SAPERA_LT_CAMERA *sapera_lt_camera_ptr;
	MX_RECORD *sapera_lt_record;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	sapera_lt_camera_ptr = (MX_SAPERA_LT_CAMERA *)
				vinput->record->record_type_struct;

	if ( sapera_lt_camera_ptr
		== (MX_SAPERA_LT_CAMERA *) NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_SAPERA_LT_CAMERA pointer for "
			"record '%s' passed by '%s' is NULL.",
			vinput->record->name, calling_fname );
	}

	if ( sapera_lt_camera != (MX_SAPERA_LT_CAMERA **) NULL ) {
		*sapera_lt_camera = sapera_lt_camera_ptr;
	}

	if ( sapera_lt != (MX_SAPERA_LT **) NULL ) {
		sapera_lt_record =
			sapera_lt_camera_ptr->sapera_lt_record;

		if ( sapera_lt_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The sapera_lt_record pointer for record '%s' "
			"is NULL.", vinput->record->name );
		}

		*sapera_lt = (MX_SAPERA_LT *)
					sapera_lt_record->record_type_struct;

		if ( *sapera_lt == (MX_SAPERA_LT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_SAPERA_LT pointer for record '%s' used "
			"by record '%s' is NULL.",
				vinput->record->name,
				sapera_lt_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static void
mxd_sapera_lt_camera_acquisition_callback( SapXferCallbackInfo *info )
{
#if MXD_SAPERA_LT_CAMERA_DEBUG_ACQUISITION_CALLBACK
	static const char fname[] =
		"mxd_sapera_lt_camera_acquisition_callback()";
#endif

	MX_RECORD *record;
	MX_VIDEO_INPUT *vinput;
	MX_SAPERA_LT_CAMERA *sapera_lt_camera;
	long i;
	long raw_frame_buffer_number;
	mx_bool_type old_frame_buffer_was_unsaved;
	mx_bool_type skip_frame;
	struct timespec frame_time, time_offset;

#if MXD_SAPERA_LT_CAMERA_DEBUG_ACQUISITION_CALLBACK
	MX_DEBUG(-2,
	  ("***************************************************************"));
	MX_DEBUG(-2,("***** %s invoked *****", fname ));
	MX_DEBUG(-2,
	  ("***************************************************************"));
#endif

	sapera_lt_camera =
		(MX_SAPERA_LT_CAMERA *) info->GetContext();

	record = sapera_lt_camera->record;

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	/* Immediately compute the time at which the frame was acquired. */

	time_offset = mx_high_resolution_time();

	frame_time = mx_add_high_resolution_times(
			sapera_lt_camera->boot_time,
			time_offset );

	/* Update the raw last frame number for this sequence. */

	sapera_lt_camera->raw_last_frame_number++;

#if MXD_SAPERA_LT_CAMERA_DEBUG_ACQUISITION_CALLBACK
	MX_DEBUG(-2,("%s: sapera_lt_camera->raw_last_frame_number = %ld",
		fname, sapera_lt_camera->raw_last_frame_number));
	MX_DEBUG(-2,("%s: sapera_lt_camera->num_frames_to_skip = %ld",
		fname, sapera_lt_camera->num_frames_to_skip));
#endif

	/*----*/

	if ( sapera_lt_camera->raw_last_frame_number
	    < sapera_lt_camera->num_frames_to_skip )
	{
		skip_frame = TRUE;
	} else {
		skip_frame = FALSE;
	}

	if ( skip_frame ) {

		/* If we are still in the frames that are supposed to
		 * be skipped, then set 'last_frame_number' to indicate
		 * that no 'useable' frames have been acquired yet.
		 */

		vinput->last_frame_number = -1L;

		old_frame_buffer_was_unsaved = FALSE;

#if MXD_SAPERA_LT_CAMERA_DEBUG_ACQUISITION_CALLBACK
		MX_DEBUG(-2,("%s: skipping raw last frame.", fname));
#endif

		sapera_lt_camera->raw_total_num_frames++;

		/* If we are skipping this frame, then we are done now. */

		return;
	} else {
		/* This is a frame that we want to save. */

#if MXD_SAPERA_LT_CAMERA_DEBUG_ACQUISITION_CALLBACK
		MX_DEBUG(-2,("%s: saving raw last frame.", fname));
#endif

		vinput->last_frame_number =
			sapera_lt_camera->raw_last_frame_number
				- sapera_lt_camera->num_frames_to_skip;

		vinput->total_num_frames++;

		/* Save the mapping from user frame number to raw frame number.
		 *
		 * Note that the value of raw_total_num_frames has _not_ _yet_
		 * been incremented by this callback, which is why the
		 * following statement does not have " - 1L" at the end.
		 */

		raw_frame_buffer_number =
			sapera_lt_camera->raw_total_num_frames;

		/*---*/

		i = ( vinput->total_num_frames - 1L )
			% sapera_lt_camera->num_frame_buffers;

		sapera_lt_camera->raw_frame_number_array[i]
					= raw_frame_buffer_number;

#if MXD_SAPERA_LT_CAMERA_DEBUG_ACQUISITION_CALLBACK
		MX_DEBUG(-2,("%s: raw_frame_number_array[%ld] = %ld",
			fname, i, sapera_lt_camera->raw_frame_number_array[i]));
#endif

		sapera_lt_camera->frame_time[i] = frame_time;

		/* Remember whether or not the frame buffer that was just 
		 * overwritten had unsaved data in it.
		 */

		old_frame_buffer_was_unsaved =
			sapera_lt_camera->frame_buffer_is_unsaved[i];

		sapera_lt_camera->frame_buffer_is_unsaved[i] = TRUE;

		/* Update the frame counter. */

		if ( sapera_lt_camera->num_frames_left_to_acquire > 0 ) {
			sapera_lt_camera->num_frames_left_to_acquire--;
		}

		sapera_lt_camera->raw_total_num_frames++;
	}

	/*----*/

#if MXD_SAPERA_LT_CAMERA_SHOW_FRAME_COUNTER
	fprintf( stderr, "CAPTURE: Total num frames = %lu\n",
		(unsigned long) vinput->total_num_frames );
#endif /* MXD_SAPERA_LT_CAMERA_SHOW_FRAME_COUNTER */

#if MXD_SAPERA_LT_CAMERA_DEBUG_ACQUISITION_CALLBACK

	MX_DEBUG(-2,
	("%s: total_num_frames = %lu, raw_total_num_frames = %lu",
		fname, vinput->total_num_frames,
		sapera_lt_camera->raw_total_num_frames ));

	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, raw_last_frame_number = %ld",
		fname, vinput->last_frame_number,
		sapera_lt_camera->raw_last_frame_number ));

	MX_DEBUG(-2,("%s: num_frames_left_to_acquire = %lu",
		fname, sapera_lt_camera->num_frames_left_to_acquire));

	MX_DEBUG(-2,("%s: old_frame_buffer_was_unsaved = %d, "
		"frame_buffer_is_unsaved[%ld] = %d",
		fname, old_frame_buffer_was_unsaved,
		i, sapera_lt_camera->frame_buffer_is_unsaved[i]));

		
#endif /* MXD_SAPERA_LT_CAMERA_DEBUG_ACQUISITION_CALLBACK */

	/* Did we have a buffer overrun? */

	if ( vinput->check_for_buffer_overrun == FALSE ) {
		return;
	}

#if ( MXD_SAPERA_LT_CAMERA_BYPASS_BUFFER_OVERRUN_TEST == FALSE )
	if ( old_frame_buffer_was_unsaved ) {

		/*!!!!! We had a buffer overrun !!!!!*/

		sapera_lt_camera->buffer_overrun_occurred = TRUE;

#if 1
		/* Stop writing out image files, since any new ones
		 * after this point will be overwrites of unsaved frames.
		 *
		 * FIXME: Is this the right thing to do?
		 */

		sapera_lt_camera->num_frames_left_to_acquire = 0;
#endif

		/* Display error messages and abort the sequence. */

		(void) mx_error( MXE_DATA_WAS_LOST, fname,
		"Buffer overrun detected at frame %ld for video input '%s'.",
			vinput->total_num_frames, record->name );

		mx_warning( "Aborting the running sequence for '%s'.",
				record->name );

		(void) mx_video_input_stop( record );
	}
#endif

	return;
}

/*---*/

static mx_status_type
mxd_sapera_lt_camera_setup_frame_counters( MX_VIDEO_INPUT *vinput,
			MX_SAPERA_LT_CAMERA *sapera_lt_camera )
{
#if MXD_SAPERA_LT_CAMERA_DEBUG_NUM_FRAMES_LEFT_TO_ACQUIRE
	static const char fname[] =
		"mxd_sapera_lt_camera_setup_frame_counters()";
#endif

	long num_frames_in_sequence;
	mx_status_type mx_status;

	/* Setup the Sapera frame counters. */

	sapera_lt_camera->user_total_num_frames_at_start
					= vinput->total_num_frames;

	sapera_lt_camera->raw_total_num_frames_at_start
				= sapera_lt_camera->raw_total_num_frames;

	mx_status = mx_sequence_get_num_frames( &(vinput->sequence_parameters),
						&num_frames_in_sequence );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sapera_lt_camera->num_frames_left_to_acquire
					= num_frames_in_sequence;

#if MXD_SAPERA_LT_CAMERA_DEBUG_NUM_FRAMES_LEFT_TO_ACQUIRE

	MX_DEBUG(-2,
	("%s: total_num_frames = %lu, num_frames_left_to_acquire = %lu",
		fname, vinput->total_num_frames,
		sapera_lt_camera->num_frames_left_to_acquire ));

#endif /* MXD_SAPERA_LT_CAMERA_DEBUG_NUM_FRAMES_LEFT_TO_ACQUIRE */

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxd_sapera_lt_camera_set_extended_exposure(
			MX_SAPERA_LT_CAMERA *sapera_lt_camera,
			double exposure_time_in_seconds )
{
	static const char fname[] =
		"mxd_sapera_lt_camera_set_extended_exposure()";

	double pixel_data_rate_in_khz = 40000.0;
#if 0
	double clock_cycles_per_line_factor = 676.0;
	double minimum_vertical_blanking_interval_factor = 1320.0;
#else
	double clock_cycles_per_line_factor = 2112.0;
	double minimum_vertical_blanking_interval_factor = 690.0;
#endif

	double exposure_time_in_milliseconds;
	int32_t extended_exposure_value;

	double minimum_exposure_time, maximum_exposure_time;

	BOOL sapera_status;
	mx_bool_type disconnect_and_reconnect = FALSE;

	if ( sapera_lt_camera == (MX_SAPERA_LT_CAMERA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SAPERA_LT_CAMERA pointer passed was NULL." );
	}

	/* You cannot call SetFeatureValue() if the associated transfer
	 * object is connected.  We must test for this.
	 */

	if ( sapera_lt_camera->transfer == NULL ) {
		disconnect_and_reconnect = FALSE;
	} else {
		BOOL is_connected;

		is_connected = sapera_lt_camera->transfer->IsConnected();

		if ( is_connected ) {
			disconnect_and_reconnect = TRUE;
		} else {
			disconnect_and_reconnect = FALSE;
		}
	}

	/* Compute the extended exposure value to send to the camera. */

	exposure_time_in_milliseconds = 1000.0 * exposure_time_in_seconds;

	extended_exposure_value = (int32_t) (
		(pixel_data_rate_in_khz / clock_cycles_per_line_factor)
		* exposure_time_in_milliseconds
			- minimum_vertical_blanking_interval_factor );

#if MXD_SAPERA_LT_CAMERA_DEBUG_EXTENDED_EXPOSURE
	MX_DEBUG(-2,
	("%s: exposure time = %f seconds, extended_exposure_value = %ld",
		fname, exposure_time_in_seconds,
		(long) extended_exposure_value ));
#endif

	if ( extended_exposure_value < 0 ) {
		minimum_exposure_time = 0.001 *
			(clock_cycles_per_line_factor / pixel_data_rate_in_khz)
			* minimum_vertical_blanking_interval_factor;

		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested exposure time of %f seconds for camera '%s' "
		"is less than the minimum exposure time of %f seconds.",
			exposure_time_in_seconds,
			sapera_lt_camera->record->name,
			minimum_exposure_time );
	}

	if ( extended_exposure_value > 65535 ) {
		maximum_exposure_time = 0.001 *
			(clock_cycles_per_line_factor / pixel_data_rate_in_khz)
			* (minimum_vertical_blanking_interval_factor + 65535);

		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested exposure time of %f seconds for camera '%s' "
		"is greater than the maximum exposure time of %f seconds.",
			exposure_time_in_seconds,
			sapera_lt_camera->record->name,
			maximum_exposure_time );
	}

	if ( disconnect_and_reconnect ) {
		sapera_status = sapera_lt_camera->transfer->Disconnect();

		if ( sapera_status == 0 ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to Disconnect() the SapTransfer object "
			"for camera '%s' failed.",
				sapera_lt_camera->record->name );
		}
	}

	/* Set the exposure time in the camera. */

	sapera_status = sapera_lt_camera->acq_device->SetFeatureValue(
						"ExtendedExposure",
						extended_exposure_value );

	if ( sapera_status == 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Setting 'ExtendedExposure' to %lu failed for camera '%s'.",
			(unsigned long) extended_exposure_value,
			sapera_lt_camera->record->name );
	}

	if ( disconnect_and_reconnect ) {
		sapera_status = sapera_lt_camera->transfer->Connect();

		if ( sapera_status == 0 ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to Connect() the SapTransfer object "
			"for camera '%s' failed.",
				sapera_lt_camera->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxd_sapera_lt_camera_get_feature_value( MX_SAPERA_LT_CAMERA *sapera_lt_camera,
					const char *feature_name,
					char *value_buffer,
					size_t value_buffer_length )
{
	static const char fname[] = "mxd_sapera_lt_camera_get_feature_value()";

	SapAcqDevice *acq_device;
	SapFeature *feature;
	SapFeature::Type feature_type;
	SapFeature::AccessMode access_mode;

	INT32 int32_value;
	INT64 int64_value;
	float float_value;
	double double_value;
	BOOL bool_value;
	int enum_value;
	char enum_string[250];
	char string_value[520];
	int exponent;
	char si_units[33];

	BOOL sapera_status;

	if ( sapera_lt_camera == (MX_SAPERA_LT_CAMERA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SAPERA_LT_CAMERA pointer passed was NULL." );
	}
	if ( feature_name == (const char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The feature_name pointer passed was NULL." );
	}
	if ( value_buffer == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The value_buffer pointer passed was NULL." );
	}
	if ( value_buffer_length <= 0 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"There is no space available in the returned value buffer "
		"for Sapera LT feature '%s'.", feature_name );
	}

	acq_device = sapera_lt_camera->acq_device;

	feature = sapera_lt_camera->feature;

	sapera_status = acq_device->GetFeatureInfo( feature_name, feature );

	if ( sapera_status == 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"A call to GetFeatureInfo( '%s', ... ) for camera '%s' failed.",
			feature_name, sapera_lt_camera->record->name );
	}

	sapera_status = feature->GetType( &feature_type );

	if ( sapera_status == 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"A call to GetType( '%s', ... ) for camera '%s' failed.",
			feature_name, sapera_lt_camera->record->name );
	}

	switch( feature_type ) {
	case SapFeature::TypeUndefined:
		strlcpy( value_buffer, "<undefined>", value_buffer_length );
		break;
	case SapFeature::TypeInt32:
		sapera_status = acq_device->GetFeatureValue( feature_name,
								&int32_value );

		snprintf( value_buffer, value_buffer_length,
			"%" PRId32, int32_value );
		break;
	case SapFeature::TypeInt64:
		sapera_status = acq_device->GetFeatureValue( feature_name,
								&int64_value );

		snprintf( value_buffer, value_buffer_length,
			"%" PRId64, int64_value );
		break;
	case SapFeature::TypeFloat:
		sapera_status = acq_device->GetFeatureValue( feature_name,
								&float_value );

		snprintf( value_buffer, value_buffer_length,
			"%f", float_value );
		break;
	case SapFeature::TypeDouble:
		sapera_status = acq_device->GetFeatureValue( feature_name,
								&double_value );

		snprintf( value_buffer, value_buffer_length,
			"%f", double_value );
		break;
	case SapFeature::TypeBool:
		sapera_status = acq_device->GetFeatureValue( feature_name,
								&bool_value );

		snprintf( value_buffer, value_buffer_length,
			"%d", (int) bool_value );
		break;
	case SapFeature::TypeEnum:
		sapera_status = acq_device->GetFeatureValue( feature_name,
								&enum_value );

		sapera_status = feature->GetEnumStringFromValue(
				enum_value, value_buffer, value_buffer_length );
		break;
	case SapFeature::TypeString:
		sapera_status = acq_device->GetFeatureValue( feature_name,
							value_buffer,
							value_buffer_length );
		break;
	case SapFeature::TypeBuffer:
		strlcpy( value_buffer, "<buffer>", value_buffer_length );
		break;
	case SapFeature::TypeLut:
		strlcpy( value_buffer, "<lut>", value_buffer_length );
		break;
	case SapFeature::TypeArray:
		strlcpy( value_buffer, "<array>", value_buffer_length );
		break;
	default:
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Unrecognized feature type %ld for feature '%s' of record '%s'",
			feature_type, feature_name,
			sapera_lt_camera->record->name );
		break;
	}

	if ( sapera_status == 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	    "A call to GetFeatureValue( '%s', ... ) for camera '%s' failed.",
			feature_name, sapera_lt_camera->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxd_sapera_lt_camera_show_feature( SapAcqDevice *acq_device,
					SapFeature *feature, int i )
{
	SapFeature::Type feature_type;
	SapFeature::AccessMode access_mode;
	char feature_name[65];

	INT32  int32_value;
	INT64  int64_value;
	float  float_value;
	double double_value;
	BOOL   bool_value;
	int    enum_value;
	char   enum_string[520];
	char   string_value[520];
	int    exponent;
	char   si_units[33];

	BOOL sapera_status;

	sapera_status = acq_device->GetFeatureInfo( i, feature );

	sapera_status = feature->GetName( feature_name, sizeof(feature_name) );

	fprintf( stderr, "  Feature %d, name = '%s', ", i, feature_name );

	sapera_status = feature->GetAccessMode( &access_mode );

	if ( access_mode == SapFeature::AccessWO ) {
		fprintf( stderr, "WRITE_ONLY\n" );
		return MX_SUCCESSFUL_RESULT;
	}

	fprintf( stderr, "  type = " );

	sapera_status = feature->GetType( &feature_type );

	switch( feature_type ) {
	case SapFeature::TypeUndefined:
		fprintf( stderr, "'undefined'" );
		break;

	case SapFeature::TypeInt32:
		sapera_status = acq_device->GetFeatureValue(
						feature_name, &int32_value );

		fprintf( stderr, "'int32', value = %d", (int) int32_value );
		break;
	case SapFeature::TypeInt64:
		sapera_status = acq_device->GetFeatureValue(
						feature_name, &int64_value );

		fprintf( stderr, "'int64', value = %" PRId64, int64_value );
		break;
	case SapFeature::TypeFloat:
		sapera_status = acq_device->GetFeatureValue(
						feature_name, &float_value );

		fprintf( stderr, "'float', value = %f", float_value );
		break;
	case SapFeature::TypeDouble:
		sapera_status = acq_device->GetFeatureValue(
						feature_name, &double_value );

		fprintf( stderr, "'double', value = %f", double_value);
		break;
	case SapFeature::TypeBool:
		sapera_status = acq_device->GetFeatureValue(
						feature_name, &bool_value );

		fprintf( stderr, "'bool', value = %d", (int) bool_value );
		break;
	case SapFeature::TypeEnum:
		sapera_status = acq_device->GetFeatureValue(
						feature_name, &enum_value );

		sapera_status = feature->GetEnumStringFromValue(
						enum_value, enum_string,
						sizeof(enum_string) );

		fprintf( stderr, "'enum', value = (%d) '%s'",
					enum_value, enum_string );
		break;
	case SapFeature::TypeString:
		sapera_status = acq_device->GetFeatureValue(
						feature_name,
						string_value,
						sizeof(string_value) );

		fprintf( stderr, "'string', value = '%s'", string_value );
		break;
	case SapFeature::TypeBuffer:
		fprintf( stderr, "'buffer'" );
		break;

	case SapFeature::TypeLut:
		fprintf( stderr, "'lut'" );
		break;

	case SapFeature::TypeArray:
		fprintf( stderr, "'array'" );
		break;

	default:
		fprintf( stderr, "'unrecognized feature type %d'",
						feature_type );
		break;
	}

	sapera_status = feature->GetSiToNativeExp10( &exponent );

	if ( exponent != 0 ) {
		fprintf( stderr, " * 10e%d", -exponent );
	}

	sapera_status = feature->GetSiUnit( si_units, sizeof(si_units) );

	if ( strlen( si_units ) > 0 ) {
		fprintf( stderr, " '%s'", si_units );
	}

	fprintf( stderr, "\n" );

	/* If this feature is an Enum, then display the allowed
	 * values of the Enum.
	 */

	if( feature_type == SapFeature::TypeEnum ) {
		int j, enum_count;

		sapera_status = feature->GetEnumCount( &enum_count );

		fprintf( stderr, "    %d enum values = ", enum_count );

		for ( j = 0; j < enum_count; j++ ) {
			BOOL enum_enabled;

			sapera_status =
				    feature->IsEnumEnabled( j, &enum_enabled );

			sapera_status =
				    feature->GetEnumValue( j, &enum_value );

			sapera_status =
				    feature->GetEnumString( j, enum_string,
							sizeof(enum_string) );

			if ( j > 0 ) {
				fprintf( stderr, ", " );
			}
			fprintf( stderr, "%s (%d)", enum_string, enum_value );

			if ( enum_enabled == FALSE ) {
				fprintf( stderr, " (DISABLED)" );
			}
		}
		fprintf( stderr, "\n" );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxd_sapera_lt_camera_show_features( MX_SAPERA_LT_CAMERA *sapera_lt_camera )
{
	static const char fname[] = "mxd_sapera_lt_camera_show_features()";

	MX_RECORD *record;
	SapAcqDevice *acq_device;
	SapFeature *feature;
	BOOL sapera_status;
	int i, feature_count;
	mx_status_type mx_status;

	record = sapera_lt_camera->record;

	acq_device = sapera_lt_camera->acq_device;

	fprintf( stderr, "%s invoked for camera '%s'.\n", fname, record->name );

	sapera_status = acq_device->GetFeatureCount( &feature_count );

	fprintf( stderr, "%d features available.\n", feature_count );

	feature = sapera_lt_camera->feature;

	for ( i = 0; i < feature_count; i++ ) {
		mx_status = mxd_sapera_lt_camera_show_feature( acq_device,
								feature, i );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_sapera_lt_camera_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_sapera_lt_camera_create_record_structures()";

	MX_VIDEO_INPUT *vinput;
	MX_SAPERA_LT_CAMERA *sapera_lt_camera = NULL;

	vinput = (MX_VIDEO_INPUT *) malloc( sizeof(MX_VIDEO_INPUT) );

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VIDEO_INPUT structure." );
	}

	sapera_lt_camera = (MX_SAPERA_LT_CAMERA *)
				malloc( sizeof(MX_SAPERA_LT_CAMERA) );

	if ( sapera_lt_camera == (MX_SAPERA_LT_CAMERA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_SAPERA_LT_CAMERA structure." );
	}

	record->record_class_struct = vinput;
	record->record_type_struct = sapera_lt_camera;
	record->class_specific_function_list = 
			&mxd_sapera_lt_camera_video_input_function_list;

	memset( &(vinput->sequence_parameters),
			0, sizeof(vinput->sequence_parameters) );

	vinput->record = record;
	sapera_lt_camera->record = record;

	sapera_lt_camera->acq_device = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sapera_lt_camera_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_sapera_lt_camera_finish_record_initialization()";

	MX_VIDEO_INPUT *vinput;
	MX_SAPERA_LT_CAMERA *sapera_lt_camera = NULL;
	MX_SAPERA_LT *sapera_lt = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_sapera_lt_camera_get_pointers( vinput,
				&sapera_lt_camera, &sapera_lt, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_CAMERA_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	sapera_lt_camera->transfer = NULL;

	mx_status = mx_video_input_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sapera_lt_camera_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_sapera_lt_camera_open()";

	MX_VIDEO_INPUT *vinput;
	MX_SAPERA_LT_CAMERA *sapera_lt_camera = NULL;
	MX_SAPERA_LT *sapera_lt = NULL;
	BOOL sapera_status;
	long bytes_per_frame;
	unsigned long i;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_sapera_lt_camera_get_pointers( vinput,
				&sapera_lt_camera, &sapera_lt, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	if ( sapera_lt->num_cameras <= 0 ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"Sapera server '%s' does not have any cameras.",
			sapera_lt->server_name );
	} else
	if ( sapera_lt_camera->camera_number
			>= sapera_lt->num_cameras ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested camera number %ld for record '%s' "
		"is outside the allowed range of 0 to %ld for "
		"Sapera server '%s'.",
			sapera_lt_camera->camera_number,
			record->name,
			sapera_lt->num_cameras - 1,
			sapera_lt->server_name );
	}

	if ( sapera_lt_camera->num_frame_buffers < 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The number of allocated frame buffers for Sapera LT "
		"camera '%s' must be greater than or equal to 1.  "
		"Instead, it was set to %ld.",
			record->name,
			sapera_lt_camera->num_frame_buffers );
	}

	/* Get the time that the system booted.  This will be used later
	 * to compute the time when each frame was acquired.
	 */

	mx_status = mx_get_system_boot_time(
			&(sapera_lt_camera->boot_time) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---------------------------------------------------------------*/

	/* Allocate and then Create() the high level SapAcqDevice object.
	 * 
	 * Soon we will call the functions mx_video_input_get_framesize()
	 * and mx_video_input_get_bytes_per_frame().  Those two functions
	 * depend on the presence of an initialized SapAcqDevice object,
	 * so we must create that object before calling the video input
	 * functions below.
	 */

	size_t filename_length = strlen( sapera_lt_camera->config_filename );

	SapLocation location( sapera_lt->server_name,
				sapera_lt_camera->camera_number );

	if ( filename_length == 0 ) {
		sapera_lt_camera->acq_device =
			new SapAcqDevice( location, NULL );
	} else {
		sapera_lt_camera->acq_device = new SapAcqDevice( location,
					sapera_lt_camera->config_filename );
	}

	if ( sapera_lt_camera->acq_device == NULL ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Unable to create a SapAcqDevice object "
		"for camera '%s'", record->name );
	}

	/* -------- */

	sapera_status = sapera_lt_camera->acq_device->Create();

#if MXD_SAPERA_LT_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s: sapera_lt_camera->acq_device->Create() = %d",
		fname, (int) sapera_status));
#endif

	if ( sapera_status == FALSE ) {
		if ( filename_length == 0 ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Unable to create the acquisition device used for "
			"camera '%s'",
				record->name );
		} else {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Unable to read from the configuration file '%s' "
			"used by camera '%s'.",
				sapera_lt_camera->config_filename,
				record->name );
		}
	}

	/* -------- */

	/* Create a SapFeature object now, since we will need it later
	 * in stack frames where we will no longer have access to the
	 * SapLocation object created just above.
	 */

	sapera_lt_camera->feature = new SapFeature( location );

	sapera_status = sapera_lt_camera->feature->Create();

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to Create() a SapFeature object failed "
		"for camera '%s'.",
			record->name );
	}

	/* -------- */

	/* Initialize the exposure time to 1 second. */

	mx_status = mxd_sapera_lt_camera_set_extended_exposure(
					sapera_lt_camera, 1.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
					
	/* -------- */

#if MXD_SAPERA_LT_CAMERA_SHOW_FEATURES
	(void) mxd_sapera_lt_camera_show_features( sapera_lt_camera );
#endif

	/*---------------------------------------------------------------*/

	/* Invoke some video input initialization steps here that we
	 * will need in order to compute the maximum allowed number of
	 * image frames below.
	 */

	mx_status = mx_video_input_get_framesize( record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_frame( record,
							&bytes_per_frame );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s: bytes_per_frame = %ld", fname, bytes_per_frame ));
#endif

	/*---------------------------------------------------------------*/

	/* Allocate the SapBuffer object which contains the raw image
	 * data from all of the frames we take in a sequence.
	 */

#if MXD_SAPERA_LT_CAMERA_DEBUG_FRAME_BUFFER_ALLOCATION
	MX_DEBUG(-2,("%s: allocating %ld frame buffers for camera '%s'.",
		fname, sapera_lt_camera->num_frame_buffers, record->name ));
#endif

	sapera_lt_camera->buffer =
		new SapBufferWithTrash( sapera_lt_camera->num_frame_buffers,
					sapera_lt_camera->acq_device );

	if ( sapera_lt_camera->buffer == NULL ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Unable to create a %ld frame SapBuffer object for "
		"camera '%s'.",
			sapera_lt_camera->num_frame_buffers,
			record->name );
	}

	vinput->num_capture_buffers =
		sapera_lt_camera->num_frame_buffers;

	/*---------------------------------------------------------------*/

	/* Allocate a SapAcqDeviceToBuf object.  This object manages the
	 * transfer of data from the camera to the SapBuffer object
	 * that we created just above.
	 */

	sapera_lt_camera->transfer =
		new SapAcqDeviceToBuf( sapera_lt_camera->acq_device,
				sapera_lt_camera->buffer,
				mxd_sapera_lt_camera_acquisition_callback,
				(void *) sapera_lt_camera );

	if ( sapera_lt_camera->transfer == NULL ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	    "Unable to create a SapAcqToBuf object for camera '%s'.",
			record->name );
	}

	/*---------------------------------------------------------------*/

	/* Create an array of raw frame numbers to translate from user
	 * frame numbers to raw frame numbers.  This mapping allows one
	 * to easily bypass the skipped frame numbers.
	 */

	sapera_lt_camera->raw_frame_number_array = (long *)
		malloc( sapera_lt_camera->num_frame_buffers * sizeof(long) );

	if ( sapera_lt_camera->raw_frame_number_array == (long *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"The attempt to allocate a %lu element translation table "
		"to translate from user frame numbers to raw frame numbers "
		"failed for video input '%s'.",
			sapera_lt_camera->num_frame_buffers,
			record->name );
	}

	for ( i = 0; i < sapera_lt_camera->num_frame_buffers; i++ ) {
		sapera_lt_camera->raw_frame_number_array[i] = -1L;
	}

	/*---------------------------------------------------------------*/

	/* Create an array of 'struct timespec' structures to hold the
	 * wall clock time when each frame was acquired.
	 */

	sapera_lt_camera->frame_time = (struct timespec *)
		calloc( sapera_lt_camera->num_frame_buffers,
			sizeof(struct timespec) );

	if ( sapera_lt_camera->frame_time == (struct timespec *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element "
		"frame time array for area detector '%s'.",
			sapera_lt_camera->num_frame_buffers,
			record->name );
	}

	/* Create the 'unsaved_frame_buffer_index' array.  This array
	 * contains the absolute frame number for the image frame most
	 * recently acquired.
	 */

	sapera_lt_camera->frame_buffer_is_unsaved = (mx_bool_type *)
		calloc( sapera_lt_camera->num_frame_buffers,
			sizeof(mx_bool_type) );

	if ( sapera_lt_camera->frame_buffer_is_unsaved
						== (mx_bool_type *) NULL ) 
	{
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element "
		"'unsaved_frame_buffer_index' array for area detector '%s'.",
			sapera_lt_camera->num_frame_buffers,
			record->name );
	}

	/*---------------------------------------------------------------*/

	/* Create() the buffer object. */

#if MXD_SAPERA_LT_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s: Before buffer->Create()", fname));
#endif

	sapera_status = sapera_lt_camera->buffer->Create();

#if MXD_SAPERA_LT_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s: sapera_lt_camera->buffer->Create() = %d",
		fname, (int) sapera_status));
#endif

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Unable to create the low-level resources used by the "
		"SapBuffer object of camera '%s'.",
			record->name );
	}

#if ( 1 && MXD_SAPERA_LT_CAMERA_DEBUG_OPEN )
	{
		SapBuffer *buffer;
		int pixel_depth, num_buffers, width, height;

		buffer = sapera_lt_camera->buffer;

		pixel_depth = buffer->GetPixelDepth();
		num_buffers = buffer->GetCount();
		width       = buffer->GetWidth();
		height      = buffer->GetHeight();

		MX_DEBUG(-2,
		("%s: SapBuffer pixel_depth = %d, num_buffers = %d",
			fname, pixel_depth, num_buffers));

		MX_DEBUG(-2,("%s: SapBuffer width = %d, height = %d",
			fname, width, height));
	}
#endif

	/*---------------------------------------------------------------*/

	/* Create() the transfer object. */

#if MXD_SAPERA_LT_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s: Before transfer->Create()", fname));
#endif

	sapera_status = sapera_lt_camera->transfer->Create();

#if MXD_SAPERA_LT_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s: sapera_lt_camera->transfer->Create() = %d",
		fname, (int) sapera_status));
#endif

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Unable to create the low-level resources used by the "
		"SapAcqDeviceToBuf object of camera '%s'.",
			record->name );
	}

	/* If an area detector driver wants us to skip frames, then it
	 * must set our 'num_frames_to_skip' variable to a non-zero
	 * value in the area detector's open() routine.
	 */

	sapera_lt_camera->num_frames_to_skip = 0;

	/*---*/

	sapera_lt_camera->old_total_num_frames = -1;
	sapera_lt_camera->old_status = 0xffffffff;

	/*---------------------------------------------------------------*/

	/* Initialize the video parameters. */

	mx_status = mx_video_input_get_framesize( record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_image_format( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_frame( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* For DALSA cameras, the camera is _always_ the clock master. */

	vinput->master_clock = MXF_VIN_MASTER_CAMERA;

	vinput->trigger_mode = MXT_IMAGE_INTERNAL_TRIGGER;

	mx_status = mx_video_input_set_trigger_mode( record,
						MXT_IMAGE_INTERNAL_TRIGGER );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize a bunch of MX driver parameters. */

	vinput->parameter_type = -1;
	vinput->frame_number   = -100;
	vinput->get_frame      = -100;
	vinput->frame          = NULL;
	vinput->frame_buffer   = NULL;
	vinput->byte_order     = (long) mx_native_byteorder();

	/* The allocated Sapera LT frame buffers form a circular ring buffer.
	 * Thus, if you go beyond the last allocated buffer, the Sapera LT
	 * software wraps back to the first buffer.  For that reason, we
	 * set vinput->maximum_frame_number to ULONG_MAX, since the ring
	 * buffer can wrap indefinitely.
	 *
	 * FIXME?: Bad things may happen if vinput->total_num_frames
	 * exceeds LONG_MAX.  However, if you take frames at 30 Hz,
	 * it will take over 27 months of absolutely continuous running
	 * before you reach LONG_MAX.
	 */

	vinput->maximum_frame_number = ULONG_MAX;

	vinput->last_frame_number = -1;
	vinput->total_num_frames = 0;
	vinput->status = 0x0;

#if MXD_SAPERA_LT_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,
	("%s: vinput->framesize[0] = %ld, vinput->framesize[1] = %ld",
		fname, vinput->framesize[0], vinput->framesize[1] ));

	MX_DEBUG(-2,("%s: vinput->image_format_name = '%s'",
		fname, vinput->image_format_name));

	MX_DEBUG(-2,("%s: vinput->image_format = %ld",
		fname, vinput->image_format));

	MX_DEBUG(-2,("%s: vinput->byte_order = %ld",
		fname, vinput->byte_order));

	MX_DEBUG(-2,("%s: vinput->trigger_mode = %ld",
		fname, vinput->trigger_mode));

	MX_DEBUG(-2,("%s: vinput->bits_per_pixel = %ld",
		fname, vinput->bits_per_pixel));

	MX_DEBUG(-2,("%s: vinput->bytes_per_pixel = %g",
		fname, vinput->bytes_per_pixel));

	MX_DEBUG(-2,("%s: vinput->bytes_per_frame = %ld",
		fname, vinput->bytes_per_frame));
#endif

	sapera_lt_camera->user_total_num_frames_at_start
					= vinput->total_num_frames;

	sapera_lt_camera->num_frames_left_to_acquire = 0;

#if MXD_SAPERA_LT_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sapera_lt_camera_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_sapera_lt_camera_close()";

	MX_VIDEO_INPUT *vinput = NULL;
	MX_SAPERA_LT_CAMERA *sapera_lt_camera = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_sapera_lt_camera_get_pointers( vinput,
					&sapera_lt_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_CAMERA_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mxd_sapera_lt_camera_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_sapera_lt_camera_resynchronize()";

	MX_SEQUENCE_PARAMETERS vinput_sp;
	mx_status_type mx_status;

	/* Whether or not resynchronize is successful depends on the
	 * nature of the video capture card's failure.  Some kinds
	 * of failures require restarting the MX server or rebooting
	 * the host computer, which is not something that resynchronize
	 * can handle.
	 */

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

#if MXD_SAPERA_LT_CAMERA_DEBUG_RESYNCHRONIZE
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	/* In some situations, setting the video input card to streaming mode
	 * with internal triggering for a while and then stopping it can put
	 * the video card back into a mode such that we can control it.
	 */

	mx_status = mx_video_input_set_trigger_mode( record,
						MXT_IMAGE_INTERNAL_TRIGGER );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vinput_sp.sequence_type = MXT_SQ_STREAM;
	vinput_sp.num_parameters = 1;
	vinput_sp.parameter_array[0] = 0.1;	/* exposure time in seconds */

	mx_status = mx_video_input_set_sequence_parameters(record, &vinput_sp);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_start( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_stop( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_CAMERA_DEBUG_RESYNCHRONIZE
	MX_DEBUG(-2,("%s complete for record '%s'", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mxd_sapera_lt_camera_arm( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_sapera_lt_camera_arm()";

	MX_SAPERA_LT_CAMERA *sapera_lt_camera = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	double exposure_time, frame_time;
	unsigned long num_frames, raw_num_frames;
	SapAcqDevice *acq_device;
	BOOL sapera_status, is_grabbing;
	mx_status_type mx_status;

	mx_status = mxd_sapera_lt_camera_get_pointers( vinput,
					&sapera_lt_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_CAMERA_DEBUG_ARM
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	mx_status = mxd_sapera_lt_camera_setup_frame_counters(
					vinput, sapera_lt_camera );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sapera_lt_camera->acq_device == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Camera '%s' is not connected.", vinput->record->name );
	}

	/* If the camera is Grabbing, then stop it. */

	is_grabbing = sapera_lt_camera->transfer->IsGrabbing();

	if ( is_grabbing ) {
		mx_status = mxd_sapera_lt_camera_stop( vinput );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Initialize the raw_last_frame_number counter to "no frames" (-1) */

	sapera_lt_camera->raw_last_frame_number = -1L;

	/* Clear the buffer overrun flag. */

	sapera_lt_camera->buffer_overrun_occurred = FALSE;

	/* Clear out the frame_buffer_is_unsaved array. */

	memset( sapera_lt_camera->frame_buffer_is_unsaved, 0,
	    sapera_lt_camera->num_frame_buffers * sizeof(mx_bool_type) );

	/* Clear any existing frame data in the SapBuffer object. */

	sapera_status = sapera_lt_camera->buffer->Clear();

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to clear the capture buffers for "
		"camera '%s' failed.",
			vinput->record->name );
	}

	/* For this video input, the input frame buffer must be set up
	 * before we can execute the Snap() or Grab() methods.  For that
	 * reason, we do mx_image_alloc() here to make sure that the
	 * frame is set up.
	 */

	mx_status = mx_image_alloc( &(vinput->frame),
					vinput->framesize[0],
					vinput->framesize[1],
					vinput->image_format,
					vinput->byte_order,
					vinput->bytes_per_pixel,
					MXT_IMAGE_HEADER_LENGTH_IN_BYTES,
					vinput->bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Figure out the number of frames and the exposure time
	 * for the upcoming imaging sequence.
	 */

	sp = &(vinput->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_STREAM:
		num_frames = 1;
		exposure_time = sp->parameter_array[0];
		break;
	case MXT_SQ_MULTIFRAME:
		num_frames = sp->parameter_array[0];
		exposure_time = sp->parameter_array[1];
		frame_time = sp->parameter_array[2];
		break;
	case MXT_SQ_DURATION:
		num_frames = sp->parameter_array[0];
		exposure_time = 1;
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		    "Sequence type %ld has not yet been implemented for '%s'.",
				sp->sequence_type, vinput->record->name );
		break;
	}
	
	/* Set the SynchronizationMode feature to match the trigger
	 * and sequence settings.
	 */

	acq_device = sapera_lt_camera->acq_device;

	if ( vinput->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER ) {

		if ( sp->sequence_type != MXT_SQ_DURATION ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Only 'duration' mode scans can be run with an "
			"external trigger for camera '%s'.",
				vinput->record->name );
		}

		/* Configure the DALSA camera to be externally triggered. */

		sapera_status = acq_device->SetFeatureValue(
					"SynchronizationMode",
					"ExtTrigger" );

		if ( sapera_status == FALSE ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to set the 'SynchronizationMode' "
			"to 'ExtTrigger' for camera '%s' failed.",
			vinput->record->name );
		}

		/* If we are to be externally triggered, then we start
		 * the Snap() now and then let the external trigger 
		 * start the acquisition.
		 */

		raw_num_frames = num_frames
				+ sapera_lt_camera->num_frames_to_skip;

#if MXD_SAPERA_LT_CAMERA_DEBUG_ARM
		MX_DEBUG(-2,("%s: Snap( %lu )", fname, raw_num_frames));
#endif

		sapera_status = sapera_lt_camera->transfer->Snap(
						raw_num_frames );

		if ( sapera_status == FALSE ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to call Snap() for camera '%s' failed.",
			vinput->record->name );
		}
	} else
	if ( vinput->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) {

		switch( sp->sequence_type ) {
		case MXT_SQ_ONE_SHOT:
		case MXT_SQ_STREAM:
		case MXT_SQ_MULTIFRAME:

			/* Multiframe sequence */

			sapera_status = acq_device->SetFeatureValue(
					"SynchronizationMode",
					"FreeRunning" );

			if ( sapera_status == FALSE ) {
				return mx_error(MXE_DEVICE_ACTION_FAILED, fname,
				"The attempt to set the 'SynchronizationMode' "
				"to 'FreeRunning' for camera '%s' failed.",
				vinput->record->name );
			}
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Only 'one shot', 'stream', and multiframe' mode scans "
			"can be run with an internal trigger for camera '%s'.",
				vinput->record->name );
			break;
		}

		/* We use the "ExtendedExposure" feature to set
		 * the exposure time.  ExtendedExposure can only
		 * be used in FreeRunning mode.
		 */

		mx_status = mxd_sapera_lt_camera_set_extended_exposure(
					sapera_lt_camera, exposure_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;


		/* For internal triggering, the trigger will occur in
		 * the mxd_sapera_lt_camera_trigger() routine.
		 */
	}

#if MXD_SAPERA_LT_CAMERA_DEBUG_ARM
	MX_DEBUG(-2,("%s complete for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sapera_lt_camera_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_sapera_lt_camera_trigger()";

	MX_SAPERA_LT_CAMERA *sapera_lt_camera = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	double exposure_time, frame_time;
	unsigned long num_frames;
	BOOL sapera_status;
	mx_status_type mx_status;

	mx_status = mxd_sapera_lt_camera_get_pointers( vinput,
					&sapera_lt_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sapera_lt_camera->acq_device == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Camera '%s' is not connected.", vinput->record->name );
	}

#if MXD_SAPERA_LT_CAMERA_DEBUG_TRIGGER
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif
	sp = &(vinput->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_STREAM:
		num_frames = 1;
		exposure_time = sp->parameter_array[0];
		break;
	case MXT_SQ_MULTIFRAME:
		num_frames = sp->parameter_array[0];
		exposure_time = sp->parameter_array[1];
		frame_time = sp->parameter_array[2];
		break;
	case MXT_SQ_DURATION:
		num_frames = sp->parameter_array[0];
		exposure_time = 1;
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		    "Sequence type %ld has not yet been implemented for '%s'.",
				sp->sequence_type, vinput->record->name );
		break;
	}

	if ( vinput->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER ) {

		/* For external triggering, this function does nothing. */

		return MX_SUCCESSFUL_RESULT;
	} else
	if ( vinput->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) {

#if MXD_SAPERA_LT_CAMERA_DEBUG_TRIGGER
		MX_DEBUG(-2,
		("%s: internal trigger enabled for video input '%s'",
			fname, vinput->record->name));
#endif
		/* Tell the camera to start sending image frames. */

		if ( sp->sequence_type == MXT_SQ_STREAM ) {
		    sapera_status = sapera_lt_camera->transfer->Grab();

		    if ( sapera_status == FALSE ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to call Grab() for camera '%s' failed.",
				vinput->record->name );
		    }
		} else {
		    sapera_status =
				sapera_lt_camera->transfer->Snap( num_frames );

		    if ( sapera_status == FALSE ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to call Snap() for camera '%s' failed.",
				vinput->record->name );
		    }
		}
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Trigger mode %ld is not supported for camera '%s'.",
			vinput->trigger_mode, vinput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sapera_lt_camera_stop( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_sapera_lt_camera_stop()";

	MX_SAPERA_LT_CAMERA *sapera_lt_camera = NULL;
	MX_SEQUENCE_PARAMETERS *sp = NULL;
	double exposure_time;
	int wait_milliseconds;
	BOOL sapera_status;
	mx_status_type mx_status;

	mx_status = mxd_sapera_lt_camera_get_pointers( vinput,
					&sapera_lt_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sapera_lt_camera->acq_device == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Camera '%s' is not connected.", vinput->record->name );
	}

#if MXD_SAPERA_LT_CAMERA_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif
	sapera_status = sapera_lt_camera->transfer->Freeze();

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to stop camera '%s' failed.",
			vinput->record->name );
	}

	sp = &(vinput->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_STREAM:
		exposure_time = sp->parameter_array[0];
		break;
	case MXT_SQ_MULTIFRAME:
		exposure_time = sp->parameter_array[1];
		break;
	case MXT_SQ_DURATION:
		exposure_time = 1;
		break;
	case 0:
		/* If no sequence has been initialized, then
		 * pretend the exposure time was 1 second.
		 */
		exposure_time = 1;
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Sequence type %ld has not yet been implemented for '%s'.",
			sp->sequence_type, vinput->record->name );
		break;
	}

	/* Wait for 200% of the exposure time for data transfer to finish. */

	wait_milliseconds = (int) ( 2000.0 * exposure_time );

	sapera_status = sapera_lt_camera->transfer->Wait( wait_milliseconds );

#if 0
	if ( sapera_status == FALSE ) {

		(void) mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The running sequence for camera '%s' has not stopped "
		"after a timeout of %d milliseconds.  "
		"Forcing an abort instead.",
			vinput->record->name,
			wait_milliseconds );

		mx_status = mx_video_input_abort( sapera_lt_camera->record );

		sapera_lt_camera->num_frames_left_to_acquire = 0;

		return mx_status;
	}
#endif

	/* Mark the sequence as complete. */

	if ( sapera_lt_camera->num_frames_left_to_acquire > 0 ) {
		sapera_lt_camera->num_frames_left_to_acquire = 0;
	}

#if MXD_SAPERA_LT_CAMERA_DEBUG_NUM_FRAMES_LEFT_TO_ACQUIRE

	MX_DEBUG(-2,
	("%s: total_num_frames = %lu, num_frames_left_to_acquire = %lu",
		fname, vinput->total_num_frames,
		sapera_lt_camera->num_frames_left_to_acquire ));

#endif /* MXD_SAPERA_LT_CAMERA_DEBUG_NUM_FRAMES_LEFT_TO_ACQUIRE */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sapera_lt_camera_abort( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_sapera_lt_camera_abort()";

	MX_SAPERA_LT_CAMERA *sapera_lt_camera = NULL;
	BOOL sapera_status;
	mx_status_type mx_status;

	mx_status = mxd_sapera_lt_camera_get_pointers( vinput,
					&sapera_lt_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sapera_lt_camera->acq_device == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Camera '%s' is not connected.", vinput->record->name );
	}

#if MXD_SAPERA_LT_CAMERA_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif
	sapera_status = sapera_lt_camera->transfer->Abort();

	if ( sapera_lt_camera->num_frames_left_to_acquire > 0 ) {
		sapera_lt_camera->num_frames_left_to_acquire = 0;
	}

#if MXD_SAPERA_LT_CAMERA_DEBUG_NUM_FRAMES_LEFT_TO_ACQUIRE

	MX_DEBUG(-2,
	("%s: total_num_frames = %lu, num_frames_left_to_acquire = %lu",
		fname, vinput->total_num_frames,
		sapera_lt_camera->num_frames_left_to_acquire ));

#endif /* MXD_SAPERA_LT_CAMERA_DEBUG_NUM_FRAMES_LEFT_TO_ACQUIRE */

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to abort camera '%s' failed.",
			vinput->record->name );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

MX_EXPORT mx_status_type
mxd_sapera_lt_camera_get_extended_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
		"mxd_sapera_lt_camera_get_extended_status()";

	MX_SAPERA_LT_CAMERA *sapera_lt_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sapera_lt_camera_get_pointers( vinput,
					&sapera_lt_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sapera_lt_camera->acq_device == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Camera '%s' is not connected.", vinput->record->name );
	}

	vinput->status = 0;

	if ( sapera_lt_camera->num_frames_left_to_acquire > 0 ) {
		vinput->status |= MXSF_VIN_IS_BUSY;
	}

#if ( MXD_SAPERA_LT_CAMERA_BYPASS_BUFFER_OVERRUN_TEST == FALSE )
	if ( sapera_lt_camera->buffer_overrun_occurred ) {
		vinput->status |= MXSF_VIN_OVERRUN;
	}
#endif

	/* Computing 'last_frame_number' and 'total_num_frames'
	 * for the MX_VIDEO_INPUT record is handled in the
	 * mxd_sapera_lt_camera_acquisition_callback() function.
	 */

	if ( vinput->status & MXSF_VIN_IS_BUSY ) {
		vinput->busy = TRUE;
	} else {
		vinput->busy = FALSE;
	}

#if MXD_SAPERA_LT_CAMERA_DEBUG_EXTENDED_STATUS

	MX_DEBUG(-2,("%s: last_frame_number = %ld, "
			"total_num_frames = %ld, status = %#lx",
			fname, vinput->last_frame_number,
			vinput->total_num_frames,
			vinput->status));

#elif MXD_SAPERA_LT_CAMERA_DEBUG_EXTENDED_STATUS_WHEN_BUSY

	if ( vinput->busy ) {
		MX_DEBUG(-2,("%s: last_frame_number = %ld, "
			"total_num_frames = %ld, status = %#lx",
			fname, vinput->last_frame_number,
			vinput->total_num_frames,
			vinput->status));
	}

#elif MXD_SAPERA_LT_CAMERA_DEBUG_EXTENDED_STATUS_WHEN_CHANGED

	/* 'old_total_num_frames' is used by the MX debugging macro
	 * MXD_SAPERA_LT_CAMERA_DEBUG_EXTENDED_STATUS_WHEN_CHANGED
	 * so that a comparison of vinput->total_num_frames and
	 * vinput->status with their 'old' versions can determine
	 * whether or not 'extended_status' has changed.
	 */

	if ((vinput->total_num_frames != sapera_lt_camera->old_total_num_frames)
	 || (vinput->status           != sapera_lt_camera->old_status) )
	{
		MX_DEBUG(-2,("%s: last_frame_number = %ld, "
			"total_num_frames = %ld, status = %#lx",
			fname, vinput->last_frame_number,
			vinput->total_num_frames,
			vinput->status));
	}

#endif
	sapera_lt_camera->old_total_num_frames = vinput->total_num_frames;
	sapera_lt_camera->old_status           = vinput->status;

	/* If our callback says that all of the frames we want have been
	 * acquired, check to see if the camera is in Grab mode.  If it
	 * is, then we want to turn that off so that the camera is idle.
	 */

	if ( vinput->busy == FALSE ) {
		BOOL is_grabbing;

		is_grabbing = sapera_lt_camera->transfer->IsGrabbing();

		if ( is_grabbing ) {
			mx_status = mxd_sapera_lt_camera_stop( vinput );
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sapera_lt_camera_get_frame( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_sapera_lt_camera_get_frame()";

	MX_SAPERA_LT_CAMERA *sapera_lt_camera = NULL;
	MX_IMAGE_FRAME *frame;
	unsigned long user_absolute_frame_number;
	unsigned long user_modulo_frame_number;
	unsigned long raw_absolute_frame_number;
	unsigned long raw_modulo_frame_number;
	int buffer_resource_index;
	void *mx_data_address;
	void *sapera_data_address;
	BOOL sapera_status;
	struct timespec acquisition_time;
	mx_status_type mx_status;

	mx_status = mxd_sapera_lt_camera_get_pointers( vinput,
					&sapera_lt_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sapera_lt_camera->acq_device == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Camera '%s' is not connected.", vinput->record->name );
	}

	frame = vinput->frame;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

#if MXD_SAPERA_LT_CAMERA_DEBUG_GET_FRAME
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif
	/* Get the address of the MX image frame buffer. */

	if ( vinput->frame == (	MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"No image frames have been acquired yet "
		"for camera '%s'.",
			vinput->record->name );
	}

	mx_data_address = vinput->frame->image_data;

	/* Start figuring out which Sapera buffer resource index to use. */

	/* Get the raw Sapera frame number from the user frame number. */

	user_absolute_frame_number =
		sapera_lt_camera->user_total_num_frames_at_start
			+ vinput->frame_number;

	user_modulo_frame_number =
	    user_absolute_frame_number % (sapera_lt_camera->num_frame_buffers);

	raw_absolute_frame_number =
    sapera_lt_camera->raw_frame_number_array[ user_modulo_frame_number ];

	if ( raw_absolute_frame_number < 0 ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"An image frame has not yet been acquired for "
		"user absolute frame number %ld of video input '%s'.",
			user_absolute_frame_number, vinput->record->name );
	}

	/* The SapBuffer appears to behave as a ring buffer (no surprise),
	 * so the offset we pass to GetAddress must be the absolute frame
	 * number modulo the allocated number of frame buffers.
	 */

	raw_modulo_frame_number =
	  raw_absolute_frame_number % (sapera_lt_camera->num_frame_buffers);

	buffer_resource_index = (int) raw_modulo_frame_number;

#if MXD_SAPERA_LT_CAMERA_DEBUG_GET_FRAME
	MX_DEBUG(-2,("%s: user_total_num_frames_at_start = %lu, "
		"raw_total_num_frames_at_start = %lu", fname,
			sapera_lt_camera->user_total_num_frames_at_start,
			sapera_lt_camera->raw_total_num_frames_at_start));

	MX_DEBUG(-2,("%s: vinput->frame_number = %ld",
		fname, vinput->frame_number));

	MX_DEBUG(-2,("%s: user_absolute_frame_number = %lu",
		fname, user_absolute_frame_number));

	MX_DEBUG(-2,("%s: user_modulo_frame_number = %lu",
		fname, user_modulo_frame_number));

	MX_DEBUG(-2,("%s: raw_absolute_frame_number = %lu",
		fname, raw_absolute_frame_number));

	MX_DEBUG(-2,("%s: buffer_resource_index = %d",
		fname, buffer_resource_index));
#endif

#if MXD_SAPERA_LT_CAMERA_DEBUG_GET_FRAME_LOOKUP
	MX_DEBUG(-2,("%s: raw_frame_array[ %lu ] => %lu", fname,
	    user_modulo_frame_number,
    sapera_lt_camera->raw_frame_number_array[user_modulo_frame_number]));
#endif
	/* Get the address. */

	sapera_status = sapera_lt_camera->buffer->GetAddress(
							buffer_resource_index,
							&sapera_data_address );

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to get the buffer data address for "
		"camera '%s' failed.",
			vinput->record->name );
	}

	/* Transfer the data from the Sapera buffer to the MX buffer. */

	memcpy( mx_data_address, sapera_data_address,
			vinput->frame->image_length );

	/* Release the Sapera buffer address. */

	sapera_status = sapera_lt_camera->buffer->ReleaseAddress(
							&sapera_data_address );

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to release the buffer data address for "
		"camera '%s' failed.",
			vinput->record->name );
	}

	/* Update the acquisition time in the image header. */

	acquisition_time =
		sapera_lt_camera->frame_time[ buffer_resource_index ];

#if MXD_SAPERA_LT_CAMERA_DEBUG_GET_FRAME
	MX_DEBUG(-4,("%s: acquisition_time = (%lu,%lu)", fname,
		acquisition_time.tv_sec, acquisition_time.tv_nsec));
#endif
	MXIF_TIMESTAMP_SEC( vinput->frame )  = acquisition_time.tv_sec;
	MXIF_TIMESTAMP_NSEC( vinput->frame ) = acquisition_time.tv_nsec;

	/* Update the exposure time in the image header. */

	MX_SEQUENCE_PARAMETERS *sp;
	double exposure_time;

	sp = &(vinput->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_STREAM:
		exposure_time = sp->parameter_array[0];
		break;
	case MXT_SQ_MULTIFRAME:
		exposure_time = sp->parameter_array[1];
		break;
	case MXT_SQ_DURATION:
		exposure_time = 1;
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Sequence type %ld has not yet been implemented for '%s'.",
			sp->sequence_type, vinput->record->name );
		break;
	}

#if MXD_SAPERA_LT_CAMERA_DEBUG_GET_FRAME
	MX_DEBUG(-2,("%s: exposure_time = %f", fname, exposure_time));
#endif

	struct timespec exposure_timespec =
	    mx_convert_seconds_to_high_resolution_time( exposure_time );

	MXIF_EXPOSURE_TIME_SEC( vinput->frame )  = exposure_timespec.tv_sec;

	MXIF_EXPOSURE_TIME_NSEC( vinput->frame ) = exposure_timespec.tv_nsec;

	/*----*/

#if MXD_SAPERA_LT_CAMERA_DEBUG_GET_FRAME
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sapera_lt_camera_get_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
			"mxd_sapera_lt_camera_get_parameter()";

	MX_SAPERA_LT_CAMERA *sapera_lt_camera = NULL;
	SapAcqDevice *acq_device;
	UINT64 pixels_per_line, lines_per_frame;
	BOOL sapera_status;
	mx_status_type mx_status;

	mx_status = mxd_sapera_lt_camera_get_pointers( vinput,
					&sapera_lt_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sapera_lt_camera->acq_device == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Camera '%s' is not connected.", vinput->record->name );
	}

#if MXD_SAPERA_LT_CAMERA_DEBUG_MX_PARAMETERS
	MX_DEBUG(-2,("%s: record '%s', parameter '%s' (%ld)",
		fname, vinput->record->name,
	mx_get_field_label_string( vinput->record, vinput->parameter_type ),
			vinput->parameter_type));
#endif

	acq_device = sapera_lt_camera->acq_device;

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
		sapera_status = acq_device->GetFeatureValue( "Width",
							&pixels_per_line );

		sapera_status = acq_device->GetFeatureValue( "Height",
							&lines_per_frame );

		vinput->framesize[0] = pixels_per_line;
		vinput->framesize[1] = lines_per_frame;

#if MXD_SAPERA_LT_CAMERA_DEBUG
		MX_DEBUG(-2,("%s: camera '%s' framesize = (%ld,%ld).",
			fname, sapera_lt_camera->record->name,
			vinput->framesize[0], vinput->framesize[1]));
#endif

		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:
		/* FIXME: The 'PixelFormat' enum may have the value we want,
		 *        but for now, we hard code the response.
		 */

		vinput->image_format = MXT_IMAGE_FORMAT_GREY16;
		vinput->bytes_per_pixel = 2;
		vinput->bits_per_pixel = 16;

		mx_status = mx_image_get_image_format_name_from_type(
				vinput->image_format, vinput->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );

#if MXD_SAPERA_LT_CAMERA_DEBUG_MX_PARAMETERS
		MX_DEBUG(-2,("%s: video format = %ld, format name = '%s'",
		    fname, vinput->image_format, vinput->image_format_name));
#endif
		break;

	case MXLV_VIN_BYTE_ORDER:
		vinput->byte_order = (long) mx_native_byteorder();
		break;

	case MXLV_VIN_TRIGGER_MODE:
		/* For this, we just return the values set by an earlier
		 * ..._set_parameter() call.
		 */
		break;

	case MXLV_VIN_BYTES_PER_FRAME:
	case MXLV_VIN_BYTES_PER_PIXEL:
	case MXLV_VIN_BITS_PER_PIXEL:
		vinput->bytes_per_pixel = 2;
		vinput->bits_per_pixel = 16;

		vinput->bytes_per_frame = mx_round( vinput->bytes_per_pixel
			* vinput->framesize[0] * vinput->framesize[1] );
		break;

	case MXLV_VIN_BUSY:
		vinput->busy = 0;
		break;

	case MXLV_VIN_STATUS:
		vinput->status = 0;
		break;

	case MXLV_VIN_SEQUENCE_TYPE:
		break;

	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:
		break;

	default:
		mx_status =
			mx_video_input_default_get_parameter_handler( vinput );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sapera_lt_camera_set_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
			"mxd_sapera_lt_camera_set_parameter()";

	MX_SAPERA_LT_CAMERA *sapera_lt_camera = NULL;
	unsigned long i, absolute_frame_number, num_frame_buffers;
	mx_status_type mx_status;

	mx_status = mxd_sapera_lt_camera_get_pointers( vinput,
					&sapera_lt_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sapera_lt_camera->acq_device == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Camera '%s' is not connected.", vinput->record->name );
	}

#if MXD_SAPERA_LT_CAMERA_DEBUG_MX_PARAMETERS
	MX_DEBUG(-2,("%s: record '%s', parameter '%s' (%ld)",
		fname, vinput->record->name,
	mx_get_field_label_string( vinput->record, vinput->parameter_type ),
			vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_MARK_FRAME_AS_SAVED:

#if 0
		MX_DEBUG(-2,("%s: mark_frame_as_saved = %lu",
			fname, vinput->mark_frame_as_saved));
#endif

		if ( sapera_lt_camera->num_frame_buffers <= 0 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The sapera_lt_camera '%s' must have at least "
			"1 frame buffer, but the driver says it has (%ld).",
				vinput->record->name,
				sapera_lt_camera->num_frame_buffers );
		}
		if ( sapera_lt_camera->frame_buffer_is_unsaved == NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The frame_buffer_is_unsaved pointer for "
			"area detector '%s' is NULL.",
				vinput->record->name );
		}

		absolute_frame_number = vinput->mark_frame_as_saved;

		num_frame_buffers = sapera_lt_camera->num_frame_buffers;

		i = absolute_frame_number % num_frame_buffers;

		sapera_lt_camera->frame_buffer_is_unsaved[i] = FALSE;
		break;

	case MXLV_VIN_FRAMESIZE:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Changing the framesize is not yet implemented for '%s'.",
			vinput->record->name );
		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the image format is not supported for "
			"video input '%s'.", vinput->record->name );
		break;

	case MXLV_VIN_BYTE_ORDER:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the byte order for video input '%s' "
			"is not supported.", vinput->record->name );
		break;

	case MXLV_VIN_TRIGGER_MODE:
		break;

	case MXLV_VIN_BYTES_PER_FRAME:
		break;

	case MXLV_VIN_BYTES_PER_PIXEL:
		break;

	case MXLV_VIN_BITS_PER_PIXEL:
		break;

	case MXLV_VIN_SEQUENCE_TYPE:
		break;

	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:
		break;

	default:
		mx_status =
			mx_video_input_default_set_parameter_handler( vinput );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_sapera_lt_camera_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_SAPERA_LT_CAMERA_FEATURE_NAME:
		case MXLV_SAPERA_LT_CAMERA_FEATURE_VALUE:
		case MXLV_SAPERA_LT_CAMERA_GAIN:
		case MXLV_SAPERA_LT_CAMERA_SHOW_FEATURES:
		case MXLV_SAPERA_LT_CAMERA_TEMPERATURE:
			record_field->process_function
				= mxd_sapera_lt_camera_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_sapera_lt_camera_process_function( void *record_ptr,
				void *record_field_ptr,
				int operation )
{
	static const char fname[] = "mxd_sapera_lt_camera_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_VIDEO_INPUT *vinput;
	MX_SAPERA_LT_CAMERA *sapera_lt_camera;
	SapAcqDevice *acq_device;
	BOOL sapera_status;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	record_field = (MX_RECORD_FIELD *) record_field_ptr;

	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VIDEO_INPUT pointer for record '%s' is NULL.",
			record->name );
	}

	sapera_lt_camera = (MX_SAPERA_LT_CAMERA *) record->record_type_struct;

	if ( sapera_lt_camera == (MX_SAPERA_LT_CAMERA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SAPERA_LT_CAMERA pointer for record '%s' is NULL.",
			record->name );
	}

	if ( sapera_lt_camera->acq_device == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Camera '%s' is not connected.", vinput->record->name );
	}

	mx_status = MX_SUCCESSFUL_RESULT;

	acq_device = sapera_lt_camera->acq_device;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_SAPERA_LT_CAMERA_GAIN:
			sapera_status = acq_device->GetFeatureValue(
						"Gain_Float",
						&(sapera_lt_camera->gain) );

			if ( sapera_status == FALSE ) {
				return mx_error(MXE_DEVICE_ACTION_FAILED, fname,
				"The attempt to get the gain for "
				"camera '%s' failed.",
					vinput->record->name );
			}
			break;
		case MXLV_SAPERA_LT_CAMERA_TEMPERATURE:
#if 0
			sapera_status = acq_device->SetFeatureValue(
						"ForceReadTemperature", 1 );

			if ( sapera_status == FALSE ) {
				return mx_error(MXE_DEVICE_ACTION_FAILED, fname,
				"The attempt to force a read of the "
				"temperature of camera '%s' failed.",
					vinput->record->name );
			}
#endif

			sapera_status = acq_device->GetFeatureValue(
						"TemperatureAbs",
					&(sapera_lt_camera->temperature) );

			if ( sapera_status == FALSE ) {
				return mx_error(MXE_DEVICE_ACTION_FAILED, fname,
				"The attempt to read the temperature in "
				"Celsius of camera '%s' failed.",
					vinput->record->name );
			}
		case MXLV_SAPERA_LT_CAMERA_FEATURE_VALUE:
			mx_status = mxd_sapera_lt_camera_get_feature_value(
					sapera_lt_camera,
					sapera_lt_camera->feature_name,
					sapera_lt_camera->feature_value,
				    sizeof(sapera_lt_camera->feature_value));
			break;
		default:
			break;
		}
		break;

	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_SAPERA_LT_CAMERA_GAIN:
			sapera_status = acq_device->SetFeatureValue(
						"Gain_Float",
						sapera_lt_camera->gain );

			if ( sapera_status == FALSE ) {
				return mx_error(MXE_DEVICE_ACTION_FAILED, fname,
				"The attempt to set the gain for "
				"camera '%s' failed.",
					vinput->record->name );
			}
			break;
		case MXLV_SAPERA_LT_CAMERA_SHOW_FEATURES:
			mx_status =
		    mxd_sapera_lt_camera_show_features( sapera_lt_camera );

			break;
		default:
			break;
		}
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unknown operation code = %d for record '%s'.",
			operation, record->name );
	}


	return mx_status;
}

