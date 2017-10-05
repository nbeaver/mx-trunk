/*
 * Name:    d_aravis_camera.cpp
 *
 * Purpose: MX video input driver for a camera controlled by Aravis.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_ARAVIS_CAMERA_DEBUG_OPEN		TRUE
#define MXD_ARAVIS_CAMERA_DEBUG_CLOSE		TRUE
#define MXD_ARAVIS_CAMERA_DEBUG_ARM		TRUE
#define MXD_ARAVIS_CAMERA_DEBUG_TRIGGER		TRUE
#define MXD_ARAVIS_CAMERA_DEBUG_MX_PARAMETERS	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_bit.h"
#include "mx_process.h"
#include "mx_thread.h"
#include "mx_callback.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "i_aravis.h"
#include "d_aravis_camera.h"

/* Vendor include files */

#include "arv.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_aravis_camera_record_function_list = {
	NULL,
	mxd_aravis_camera_create_record_structures,
	mxd_aravis_camera_finish_record_initialization,
	NULL,
	NULL,
	mxd_aravis_camera_open,
	mxd_aravis_camera_close,
	NULL,
	NULL,
	mxd_aravis_camera_special_processing_setup
};

MX_VIDEO_INPUT_FUNCTION_LIST
mxd_aravis_camera_video_input_function_list = {
	mxd_aravis_camera_arm,
	mxd_aravis_camera_trigger,
	mxd_aravis_camera_stop,
	mxd_aravis_camera_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_aravis_camera_get_extended_status,
	mxd_aravis_camera_get_frame,
	mxd_aravis_camera_get_parameter,
	mxd_aravis_camera_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_aravis_camera_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VIDEO_INPUT_STANDARD_FIELDS,
	MXD_ARAVIS_CAMERA_STANDARD_FIELDS
};

long mxd_aravis_camera_num_record_fields
	= sizeof( mxd_aravis_camera_record_field_defaults )
	/ sizeof( mxd_aravis_camera_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_aravis_camera_rfield_def_ptr
			= &mxd_aravis_camera_record_field_defaults[0];

/*---*/

static mx_status_type mxd_aravis_camera_process_function( void *record_ptr,
							void *record_field_ptr,
							int operation );

static mx_status_type
mxd_aravis_camera_get_pointers( MX_VIDEO_INPUT *vinput,
			MX_ARAVIS_CAMERA **aravis_camera,
			MX_ARAVIS **aravis,
			const char *calling_fname )
{
	static const char fname[] =
		"mxd_aravis_camera_get_pointers()";

	MX_ARAVIS_CAMERA *aravis_camera_ptr;
	MX_RECORD *aravis_record;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	aravis_camera_ptr = (MX_ARAVIS_CAMERA *)
				vinput->record->record_type_struct;

	if ( aravis_camera_ptr == (MX_ARAVIS_CAMERA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_ARAVIS_CAMERA pointer for "
			"record '%s' passed by '%s' is NULL.",
			vinput->record->name, calling_fname );
	}

	if ( aravis_camera != (MX_ARAVIS_CAMERA **) NULL ) {
		*aravis_camera = aravis_camera_ptr;
	}

	if ( aravis != (MX_ARAVIS **) NULL ) {
		aravis_record = aravis_camera_ptr->aravis_record;

		if ( aravis_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The aravis_record pointer for record '%s' "
			"is NULL.", vinput->record->name );
		}

		*aravis = (MX_ARAVIS *) aravis_record->record_type_struct;

		if ( *aravis == (MX_ARAVIS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_ARAVIS pointer for record '%s' used "
			"by record '%s' is NULL.",
				vinput->record->name,
				aravis_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static void
mxd_aravis_camera_new_buffer_cb( ArvStream *stream, void *data )
{
	static const char fname[] = "mxd_aravis_camera_new_buffer_cb()";

	MX_VIDEO_INPUT *vinput = NULL;
	MX_ARAVIS_CAMERA *aravis_camera = (MX_ARAVIS_CAMERA *) data;
	ArvBuffer *buffer = NULL;

	MX_DEBUG(-2,("%s invoked.", fname));

	vinput = aravis_camera->record->record_class_struct;

#if 0
	buffer = arv_stream_try_pop_buffer( aravis_camera->arv_stream );
#else
	buffer = arv_stream_pop_buffer( aravis_camera->arv_stream );
#endif

	if ( buffer != NULL ) {
#if 0
	    if ( arv_buffer_get_status(buffer) == ARV_BUFFER_STATUS_SUCCESS ) {
#else
	    if ( TRUE ) {
#endif
		mx_info( "CAPTURE: buffer %ld", vinput->total_num_frames );

		vinput->total_num_frames++;
	    }
	    arv_stream_push_buffer( aravis_camera->arv_stream, buffer );
	}
}

static gboolean
mxd_aravis_camera_periodic_task_cb( void *data )
{
	mx_info( "PERIODIC TASK invoked!" );

	return TRUE;
}

static void
mxd_aravis_camera_control_lost_cb( ArvGvDevice *gv_device )
{
	mx_info( "CONTROL LOST!" );
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxd_aravis_camera_mx_callback( MX_CALLBACK_MESSAGE *message )
{
	static const char fname[] = "mxd_aravis_camera_mx_callback()";

	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	mx_status_type mx_status;
	gboolean iteration_status;

	if ( message == (MX_CALLBACK_MESSAGE *) NULL ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"This callback was invoked with a NULL callback message!" );
	}

#if 0
	MX_DEBUG(-2,("%s: callback_message = %p, callback_type = %lu",
		fname, message, message->callback_type));
#endif

	aravis_camera = (MX_ARAVIS_CAMERA *) message->u.function.callback_args;

	if ( aravis_camera == (MX_ARAVIS_CAMERA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ARAVIS_CAMERA pointer for callback message %p is NULL.",
			message );
	}

	/* Restart the virtual timer to arrange for the next callback. */

	mx_status = mx_virtual_timer_start(
			message->u.function.oneshot_timer,
			message->u.function.callback_interval );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	iteration_status = g_main_context_iteration( NULL, FALSE );

#if 0
	MX_DEBUG(-2,("%s: iteration_status = %d",
		fname, (int) iteration_status));
#else
	MXW_UNUSED(iteration_status);
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxd_aravis_camera_mx_callback_destructor( MX_CALLBACK_MESSAGE *message )
{
	static const char fname[] =
		"mxd_aravis_camera_mx_callback_destructor()";

	MX_ARAVIS_CAMERA *aravis_camera = NULL;

	if ( message == (MX_CALLBACK_MESSAGE *) NULL ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"This callback was invoked with a NULL callback message!" );
	}

#if 1
	MX_DEBUG(-2,("%s: callback_message = %p, callback_type = %lu",
		fname, message, message->callback_type));
#endif

	aravis_camera = (MX_ARAVIS_CAMERA *) message->u.function.callback_args;

	if ( aravis_camera == (MX_ARAVIS_CAMERA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ARAVIS_CAMERA pointer for callback message %p is NULL.",
			message );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aravis_camera_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_aravis_camera_create_record_structures()";

	MX_VIDEO_INPUT *vinput;
	MX_ARAVIS_CAMERA *aravis_camera = NULL;

	vinput = (MX_VIDEO_INPUT *) malloc( sizeof(MX_VIDEO_INPUT) );

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VIDEO_INPUT structure." );
	}

	aravis_camera = (MX_ARAVIS_CAMERA *)
				malloc( sizeof(MX_ARAVIS_CAMERA) );

	if ( aravis_camera == (MX_ARAVIS_CAMERA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_ARAVIS_CAMERA structure." );
	}

	record->record_class_struct = vinput;
	record->record_type_struct = aravis_camera;
	record->class_specific_function_list = 
			&mxd_aravis_camera_video_input_function_list;

	memset( &(vinput->sequence_parameters),
			0, sizeof(vinput->sequence_parameters) );

	vinput->record = record;
	aravis_camera->record = record;
	aravis_camera->arv_camera = NULL;
	aravis_camera->arv_stream = NULL;
	aravis_camera->main_loop = NULL;
	aravis_camera->acquisition_in_progress = FALSE;

	aravis_camera->callback_message = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_aravis_camera_finish_record_initialization()";

	MX_VIDEO_INPUT *vinput;
	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	MX_ARAVIS *aravis = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
				&aravis_camera, &aravis, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_aravis_camera_open()";

	MX_VIDEO_INPUT *vinput;
	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	MX_ARAVIS *aravis = NULL;
	gint sensor_width, sensor_height;
	gint height_min, height_max, width_min, width_max;
	gint region_x_offset, region_y_offset, region_width, region_height;
	long i;
	ArvBuffer *arv_buffer = NULL;
	ArvDevice *arv_device = NULL;

	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
				&aravis_camera, &aravis, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---------*/

	if ( strlen( aravis_camera->debug_configuration ) > 0 ) {
		MX_DEBUG(-2,("%s: setting debug configuration to '%s'",
			fname, aravis_camera->debug_configuration));

		arv_debug_enable( aravis_camera->debug_configuration );
	}

	/*---------*/

	aravis_camera->arv_camera = arv_camera_new( aravis_camera->device_id );

	if ( aravis_camera->arv_camera == NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"Aravis camera '%s' was not found.",
			aravis_camera->device_id );
	}

#if MXD_ARAVIS_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s: Record '%s': Aravis camera '%s' found.",
		fname, record->name, aravis_camera->device_id ));
#endif

	/* Figure out what kind of camera this is. */

	strlcpy( aravis_camera->vendor_name,
		arv_camera_get_vendor_name( aravis_camera->arv_camera ),
		sizeof( aravis_camera->vendor_name ) );

	strlcpy( aravis_camera->model_name,
		arv_camera_get_model_name( aravis_camera->arv_camera ),
		sizeof( aravis_camera->model_name ) );

#if MXD_ARAVIS_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s:  vendor = '%s', model = '%s'", fname, 
		aravis_camera->vendor_name, aravis_camera->model_name ));
#endif

#if 1
	/* It does not always work, but ask for the sensor size anyway.*/

	arv_camera_get_sensor_size( aravis_camera->arv_camera,
					&sensor_width, &sensor_height );

	MX_DEBUG(-2,("%s:  sensor size = (%lu,%lu)", fname,
		(unsigned long) sensor_width, (unsigned long) sensor_height ));
#endif

	/* Figure out the maximum framesize that Aravis claims the
	 * camera can do.
	 */

	arv_camera_get_width_bounds( aravis_camera->arv_camera,
					&width_min, &width_max );

	arv_camera_get_height_bounds( aravis_camera->arv_camera,
					&height_min, &height_max );

	/* If requested, try to ask the camera for the current framesize. */

	if ( (vinput->framesize[0] < 0) || (vinput->framesize[1] < 0) ) {
		arv_camera_get_region( aravis_camera->arv_camera,
					&region_x_offset, &region_y_offset,
					&region_width, &region_height );

#if MXD_ARAVIS_CAMERA_DEBUG_OPEN
		MX_DEBUG(-2,
		("%s: initial region: X offset = %ld, Y offset = %ld, "
			"width = %ld, height = %ld", fname,
			(long) region_x_offset, (long) region_y_offset,
			(long) region_width, (long) region_height));
#endif

		if ( (region_x_offset != 0) || (region_y_offset != 0) ) {
			return mx_error( MXE_HARDWARE_CONFIGURATION_ERROR,fname,
			"Aravis camera '%s' is configured with non-zero "
			"X and Y offsets (%ld,%ld) which is incompatible "
			"with MX.  You will need to explicitly set the "
			"framesize in the MX configuration file.",
				record->name,
				(long) region_x_offset,
				(long) region_y_offset );
		}

		if ( (region_width <= 0) || (region_height <= 0) ) {
			return mx_error( MXE_HARDWARE_CONFIGURATION_ERROR,fname,
			"For Aravis camera '%s', one or both of the reported "
			"framesize values (%ld,%ld) is less than "
			"or equal to 0.  You will need to explicitly set the "
			"framesize in the MX configuration file",
				record->name,
				(long) region_width, (long) region_height );
		}

		if ((region_width > width_max) || (region_height > height_max))
		{
			return mx_error( MXE_HARDWARE_CONFIGURATION_ERROR,fname,
			"For Aravis camera '%s', one or both of the reported "
			"framesize values (%ld,%ld) is outside the allowed "
			"range of values (%ld,%ld) reported by the camera.  "
			"You will need to explicitly set the framesize in "
			"the MX configuration file.",
				record->name,
				(long) region_width, (long) region_height,
				(long) width_max, (long) height_max );
		}
	}

	/* If one or both of the reported sensor sizes was 0, then we
	 * set the corresponding vinput->maximum_framesize value to
	 * the matching value from vinput->framesize.
	 */

	if ( sensor_width == 0 ) {
		vinput->maximum_framesize[0] = vinput->framesize[0];
	} else {
		vinput->maximum_framesize[0] = sensor_width;
	}

	if ( sensor_height == 0 ) {
		vinput->maximum_framesize[1] = vinput->framesize[1];
	} else {
		vinput->maximum_framesize[1] = sensor_height;
	}

#if MXD_ARAVIS_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s: setting camera size to (%lu,%lu)",
		fname, vinput->framesize[0], vinput->framesize[1]));
#endif

	mx_status = mx_video_input_set_framesize( record,
				vinput->framesize[0], vinput->framesize[1] );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the MX image format, bits per pixel, bytes per pixel,
	 * bytes per frame, etc. by asking for the bytes per frame.
	 */

	mx_status = mx_video_input_get_bytes_per_frame( vinput->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Create the stream that will be used to readout the image frames. */

	aravis_camera->arv_stream = arv_camera_create_stream(
					aravis_camera->arv_camera, NULL, NULL );

	if ( aravis_camera->arv_stream == NULL ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"We were not able to create a video stream for "
		"Aravis camera '%s'.", record->name );
	}

	/* Reserve the requested number of frame buffers. */

	aravis_camera->frame_buffer_array = (ArvBuffer **)
		calloc( aravis_camera->num_frame_buffers, sizeof(ArvBuffer *) );

	if ( aravis_camera->frame_buffer_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element "
		"array of ArvBuffer objects for video input '%s'.",
			aravis_camera->num_frame_buffers, record->name );
	}

	for ( i = 0; i < aravis_camera->num_frame_buffers; i++ ) {
		arv_buffer = arv_buffer_new( vinput->bytes_per_frame, NULL );

		if ( arv_buffer == NULL ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to create stream buffer %ld "
			"for Aravis camera '%s' failed.  "
			"Requested buffer size was %ld.",
				i, record->name, vinput->bytes_per_frame );
		}

		aravis_camera->frame_buffer_array[i] = arv_buffer;

		arv_stream_push_buffer( aravis_camera->arv_stream, arv_buffer );
	}

#if MXD_ARAVIS_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s: %ld buffers allocated for Aravis stream %p.",
		fname, aravis_camera->num_frame_buffers,
		aravis_camera->arv_stream ));
#endif
	/* Setup the new-buffer signal. */

	g_signal_connect( aravis_camera->arv_stream, "new-buffer",
			G_CALLBACK( mxd_aravis_camera_new_buffer_cb ),
			aravis_camera );

	/* Setup the control-lost signal. */

	arv_device = arv_camera_get_device( aravis_camera->arv_camera );

	g_signal_connect( arv_device, "control-lost",
			G_CALLBACK( mxd_aravis_camera_control_lost_cb ),
			aravis_camera );

	/* Set up a periodic task callback. */

	g_timeout_add_seconds( 0.1,
			mxd_aravis_camera_periodic_task_cb,
			aravis_camera );

	/* Create and save a GMainLoop object that will be used to handle
	 * callbacks for this camera.
	 */

	aravis_camera->main_loop = g_main_loop_new( NULL, FALSE );

	/* FIXME: For now we assume that Aravis cameras are the clock master. */

	vinput->master_clock = MXF_VIN_MASTER_CAMERA;

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

	/* FIXME: The driver does not do anything to handle rollover of
	 * the frame number.
	 */

	vinput->maximum_frame_number = ULONG_MAX;

	vinput->last_frame_number = -1;
	vinput->total_num_frames = 0;
	vinput->status = 0x0;

	/* Setup an MX callback to handle Glib main loop callbacks.  Since
	 * MX servers already have their own main loop found at the end of
	 * the MX server's main() function, we do not use g_main_loop_run()
	 * itself.  Instead, we arrange to periodically invoke the function
	 * g_main_context_iteration(), which runs a single iteration for
	 * the given main loop.
	 */

	mx_status = mx_function_add_callback( record->list_head,
				mxd_aravis_camera_mx_callback,
				mxd_aravis_camera_mx_callback_destructor,
				aravis_camera, 1.0,
				&(aravis_camera->callback_message) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_aravis_camera_close()";

	MX_VIDEO_INPUT *vinput = NULL;
	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
					&aravis_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_ARAVIS_CAMERA_DEBUG_CLOSE
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	g_main_loop_unref( aravis_camera->main_loop );

	arv_camera_stop_acquisition( aravis_camera->arv_camera );

	g_object_unref( aravis_camera->arv_stream );

	g_object_unref( aravis_camera->arv_camera );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_arm( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_aravis_camera_arm()";

	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	MX_SEQUENCE_PARAMETERS *sp = NULL;
	double exposure_time, frame_time, frame_rate;
	unsigned long num_frames;
	ArvAcquisitionMode acquisition_mode;
	mx_status_type mx_status;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
					&aravis_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_ARAVIS_CAMERA_DEBUG_ARM
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	/* Just in case a stream is already running, then we stop it. */

	arv_camera_stop_acquisition( aravis_camera->arv_camera );

	vinput->last_frame_number = -1;

	aravis_camera->total_num_frames_at_start = vinput->total_num_frames;

	/* Make sure that the MX input frame buffer is setup correctly. */

	mx_status = mx_image_alloc( &(vinput->frame),
					vinput->framesize[0],
					vinput->framesize[1],
					vinput->image_format,
					vinput->byte_order,
					vinput->bytes_per_pixel,
					MXT_IMAGE_HEADER_LENGTH_IN_BYTES,
					vinput->bytes_per_frame,
					NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Figure out the number of frames and the exposure time
	 * for the upcoming imaging sequence.  We also figure out
	 * the Aravis acquisition mode.
	 */

	sp = &(vinput->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		num_frames = 1;
		exposure_time = sp->parameter_array[0];
		frame_time = exposure_time;
		acquisition_mode = ARV_ACQUISITION_MODE_SINGLE_FRAME;
		break;
	case MXT_SQ_STREAM:
		num_frames = 1;
		exposure_time = sp->parameter_array[0];
		frame_time = exposure_time;
		acquisition_mode = ARV_ACQUISITION_MODE_CONTINUOUS;
		break;
	case MXT_SQ_MULTIFRAME:
		num_frames = sp->parameter_array[0];
		exposure_time = sp->parameter_array[1];
		frame_time = sp->parameter_array[2];
		acquisition_mode = ARV_ACQUISITION_MODE_CONTINUOUS;
		break;
	case MXT_SQ_DURATION:
		num_frames = sp->parameter_array[0];
		exposure_time = 1;
		frame_time = exposure_time;
		acquisition_mode = ARV_ACQUISITION_MODE_CONTINUOUS;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Sequence type %ld is not supported for video input '%s'.",
			sp->sequence_type, vinput->record->name );
	}

	MXW_UNUSED( num_frames );

	arv_camera_set_acquisition_mode( aravis_camera->arv_camera,
						acquisition_mode );

	frame_rate = mx_divide_safely( 1.0, frame_time );

	arv_camera_set_frame_rate( aravis_camera->arv_camera, frame_rate );

	arv_camera_set_exposure_time( aravis_camera->arv_camera,
					1.0e6 * exposure_time );

	/* Prepare the camera triggering mode. */

	if ( vinput->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER ) {
		aravis_camera->acquisition_in_progress = TRUE;

		arv_camera_set_trigger( aravis_camera->arv_camera, "Line 1" );

		/* Starting acquisition here will cause the camera to
		 * acquire a frame the next time a trigger appears on
		 * 'Line 1'.
		 */

		arv_camera_start_acquisition( aravis_camera->arv_camera );
	} else
	if ( vinput->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) {
		aravis_camera->acquisition_in_progress = FALSE;

		arv_camera_set_trigger( aravis_camera->arv_camera, "Software" );

		/* For internal triggers, we do not start acquisition here. */
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_aravis_camera_trigger()";

	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
					&aravis_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_ARAVIS_CAMERA_DEBUG_TRIGGER
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	/* If we are in internal trigger mode, then send a software trigger. */

	if ( vinput->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) {
		aravis_camera->acquisition_in_progress = TRUE;

		arv_camera_software_trigger( aravis_camera->arv_camera );

		/* Start the acquisition NOW (hopefully). */

		arv_camera_start_acquisition( aravis_camera->arv_camera );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_stop( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_aravis_camera_stop()";

	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
					&aravis_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_ARAVIS_CAMERA_DEBUG_STOP
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif
	aravis_camera->acquisition_in_progress = FALSE;

	arv_camera_stop_acquisition( aravis_camera->arv_camera );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_abort( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_aravis_camera_abort()";

	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
					&aravis_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_ARAVIS_CAMERA_DEBUG_STOP
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif
	aravis_camera->acquisition_in_progress = FALSE;

#if 0
	arv_camera_abort_acquisition( aravis_camera->arv_camera );
#else
	arv_camera_stop_acquisition( aravis_camera->arv_camera );
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_get_extended_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
		"mxd_aravis_camera_get_extended_status()";

	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
					&aravis_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( aravis_camera->acquisition_in_progress ) {
		vinput->status = MXSF_VIN_IS_BUSY;
		vinput->busy = TRUE;
	} else {
		vinput->status = 0;
		vinput->busy = FALSE;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_get_frame( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_aravis_camera_get_frame()";

	MX_ARAVIS_CAMERA *aravis = NULL;
	mx_status_type mx_status;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
					&aravis, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*----*/

#if MXD_ARAVIS_CAMERA_DEBUG_GET_FRAME
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_get_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
			"mxd_aravis_camera_get_parameter()";

	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	ArvPixelFormat arv_pixel_format;
	gint region_x_offset, region_y_offset, region_width, region_height;
	guint payload_size;
	mx_status_type mx_status;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
					&aravis_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_ARAVIS_CAMERA_DEBUG_MX_PARAMETERS
	MX_DEBUG(-2,("%s: record '%s', parameter '%s' (%ld)",
		fname, vinput->record->name,
	mx_get_field_label_string( vinput->record, vinput->parameter_type ),
			vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:

		arv_camera_get_region( aravis_camera->arv_camera,
				&region_x_offset, &region_y_offset,
				&region_width, &region_height );

		vinput->framesize[0] = region_width;
		vinput->framesize[1] = region_height;

#if MXD_ARAVIS_CAMERA_DEBUG_MX_PARAMETERS
		MX_DEBUG(-2,("%s: camera '%s' framesize = (%ld,%ld).",
			fname, aravis_camera->record->name,
			vinput->framesize[0], vinput->framesize[1]));
#endif

		break;

	case MXLV_VIN_BYTES_PER_FRAME:
		/* I want to compare the image size that is reported to me
		 * by Aravis so that I can compare it to my own calculation.
		 */

		payload_size =
			arv_camera_get_payload( aravis_camera->arv_camera );

		/* Fall through to the next case. */

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:
	case MXLV_VIN_BYTES_PER_PIXEL:
	case MXLV_VIN_BITS_PER_PIXEL:
		arv_pixel_format = arv_camera_get_pixel_format(
					aravis_camera->arv_camera );

		switch( arv_pixel_format ) {
		case ARV_PIXEL_FORMAT_MONO_8:
			vinput->image_format = MXT_IMAGE_FORMAT_GREY8;
			vinput->bytes_per_pixel = 1;
			vinput->bits_per_pixel = 8;
			break;
		case ARV_PIXEL_FORMAT_MONO_10:
		case ARV_PIXEL_FORMAT_MONO_12:
		case ARV_PIXEL_FORMAT_MONO_14:
		case ARV_PIXEL_FORMAT_MONO_16:
			vinput->image_format = MXT_IMAGE_FORMAT_GREY16;
			vinput->bytes_per_pixel = 2;
			vinput->bits_per_pixel = 16;
			break;
		default:
			/* FIXME: At present, I am only supporting greyscale
			 * images.  Color will have to wait until some other
			 * time.
			 */

			return mx_error( MXE_UNSUPPORTED, fname,
			"Aravis image format %#lx for camera '%s' "
			"is not currently supported by MX.",
				(unsigned long) arv_pixel_format,
				vinput->record->name );
			break;
		}

		mx_status = mx_image_get_image_format_name_from_type(
				vinput->image_format, vinput->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );

#if MXD_ARAVIS_CAMERA_DEBUG_MX_PARAMETERS
		MX_DEBUG(-2,("%s: video format = %ld, format name = '%s'",
		    fname, vinput->image_format, vinput->image_format_name));
		MX_DEBUG(-2,("%s: bytes_per_pixel = %f, bits_per_pixel = %ld",
		    fname, vinput->bytes_per_pixel, vinput->bits_per_pixel));
#endif

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		vinput->bytes_per_frame = mx_round( vinput->bytes_per_pixel
			* vinput->framesize[0] * vinput->framesize[1] );

#if MXD_ARAVIS_CAMERA_DEBUG_MX_PARAMETERS
		MX_DEBUG(-2,("%s: bytes_per_frame = %ld",
		    fname, vinput->bytes_per_frame));
#endif
		if ( vinput->parameter_type == MXLV_VIN_BYTES_PER_FRAME ) {
			if ( payload_size != vinput->bytes_per_frame ) {
				return mx_error( MXE_CONFIGURATION_CONFLICT,
				fname, "MX calculates that the image size for "
				"Aravis camera '%s' should be %ld.  However, "
				"Aravis says that it should be %ld.  "
				"Something is wrong here.",
					vinput->record->name,
					vinput->bytes_per_frame,
					(long) payload_size );
			}
		}
		break;

	case MXLV_VIN_BYTE_ORDER:
		vinput->byte_order = (long) mx_native_byteorder();
		break;

	case MXLV_VIN_TRIGGER_MODE:
		/* For this, we just return the values set by an earlier
		 * ..._set_parameter() call.
		 */
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
mxd_aravis_camera_set_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_aravis_camera_set_parameter()";

	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	gint region_x_offset, region_y_offset;
	gint region_width, region_height;
	mx_status_type mx_status;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
					&aravis_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_ARAVIS_CAMERA_DEBUG_MX_PARAMETERS
	MX_DEBUG(-2,("%s: record '%s', parameter '%s' (%ld)",
		fname, vinput->record->name,
	mx_get_field_label_string( vinput->record, vinput->parameter_type ),
			vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_MARK_FRAME_AS_SAVED:

		break;

	case MXLV_VIN_FRAMESIZE:
		/* Set the new framesize. */

		arv_camera_set_region( aravis_camera->arv_camera, 0, 0,
				vinput->framesize[0], vinput->framesize[1] );

		/* Verify that the camera is using the new framesize. */

		arv_camera_get_region( aravis_camera->arv_camera,
				&region_x_offset, &region_y_offset,
				&region_width, &region_height );

		if ( (region_x_offset != 0) || (region_y_offset != 0)
		  || (region_width != vinput->framesize[0])
		  || (region_height != vinput->framesize[1]) )
		{
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The requested region parameters (0,0,%lu,%lu) "
			"for video input '%s' did not successfully apply.  "
			"Instead we have (%lu,%lu,%lu,%lu).",
			    vinput->framesize[0], vinput->framesize[1],
			    vinput->record->name,
			    (unsigned long) region_x_offset,
			    (unsigned long) region_y_offset,
			    (unsigned long) region_width,
			    (unsigned long) region_height );
		}
		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:
		break;

	case MXLV_VIN_BYTE_ORDER:
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
mxd_aravis_camera_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case 888:
			record_field->process_function
				= mxd_aravis_camera_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_aravis_camera_process_function( void *record_ptr,
				void *record_field_ptr,
				int operation )
{
	static const char fname[] = "mxd_aravis_camera_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_VIDEO_INPUT *vinput;
	MX_ARAVIS_CAMERA *aravis_camera;
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

	aravis_camera = (MX_ARAVIS_CAMERA *) record->record_type_struct;

	if ( aravis_camera == (MX_ARAVIS_CAMERA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ARAVIS_CAMERA pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		default:
			break;
		}
		break;

	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
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

