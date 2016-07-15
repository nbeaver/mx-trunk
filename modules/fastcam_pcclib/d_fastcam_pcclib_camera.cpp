/*
 * Name:    d_fastcam_pcclib_camera.c
 *
 * Purpose: MX video input driver for a Photron FASTCAM camera
 *          controlled by the PccLib library.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2012, 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_FASTCAM_PCCLIB_CAMERA_DEBUG				TRUE

#define MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_OPEN			TRUE

#define MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_ARM			TRUE

#define MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_TRIGGER			TRUE

#define MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_GET_FRAME		TRUE

#define MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_EXTENDED_STATUS		TRUE

#define MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_MX_PARAMETERS		TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if defined(OS_WIN32)
#  include <io.h>
#endif

#include "mx_util.h"
#include "mx_record.h"
#if 0
#include "mx_unistd.h"
#include "mx_array.h"
#endif
#include "mx_bit.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "i_fastcam_pcclib.h"
#include "d_fastcam_pcclib_camera.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_fastcam_pcclib_camera_record_function_list = {
	NULL,
	mxd_fastcam_pcclib_camera_create_record_structures,
	mxd_fastcam_pcclib_camera_finish_record_initialization,
	NULL,
	NULL,
	mxd_fastcam_pcclib_camera_open,
	mxd_fastcam_pcclib_camera_close
};

MX_VIDEO_INPUT_FUNCTION_LIST
mxd_fastcam_pcclib_camera_video_input_function_list = {
	mxd_fastcam_pcclib_camera_arm,
	mxd_fastcam_pcclib_camera_trigger,
	NULL,
	mxd_fastcam_pcclib_camera_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_fastcam_pcclib_camera_get_extended_status,
	mxd_fastcam_pcclib_camera_get_frame,
	mxd_fastcam_pcclib_camera_get_parameter,
	mxd_fastcam_pcclib_camera_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_fastcam_pcclib_camera_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VIDEO_INPUT_STANDARD_FIELDS,
	MXD_FASTCAM_PCCLIB_CAMERA_STANDARD_FIELDS
};

long mxd_fastcam_pcclib_camera_num_record_fields
	= sizeof( mxd_fastcam_pcclib_camera_record_field_defaults )
	/ sizeof( mxd_fastcam_pcclib_camera_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_fastcam_pcclib_camera_rfield_def_ptr
			= &mxd_fastcam_pcclib_camera_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_fastcam_pcclib_camera_get_pointers( MX_VIDEO_INPUT *vinput,
			MX_FASTCAM_PCCLIB_CAMERA **fastcam_pcclib_camera,
			MX_FASTCAM_PCCLIB **fastcam_pcclib,
			CCameraControl **camera_control,
			const char *calling_fname )
{
	static const char fname[] =
		"mxd_fastcam_pcclib_camera_get_pointers()";

	MX_FASTCAM_PCCLIB_CAMERA *fastcam_pcclib_camera_ptr;
	MX_RECORD *fastcam_pcclib_record;
	CCameraControl *camera_control_ptr;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	fastcam_pcclib_camera_ptr = (MX_FASTCAM_PCCLIB_CAMERA *)
				vinput->record->record_type_struct;

	if ( fastcam_pcclib_camera_ptr
		== (MX_FASTCAM_PCCLIB_CAMERA *) NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_FASTCAM_PCCLIB_CAMERA pointer for "
			"record '%s' passed by '%s' is NULL.",
			vinput->record->name, calling_fname );
	}

	if ( fastcam_pcclib_camera != (MX_FASTCAM_PCCLIB_CAMERA **) NULL ) {
		*fastcam_pcclib_camera = fastcam_pcclib_camera_ptr;
	}

	if ( fastcam_pcclib != (MX_FASTCAM_PCCLIB **) NULL ) {
		fastcam_pcclib_record =
			fastcam_pcclib_camera_ptr->fastcam_pcclib_record;

		if ( fastcam_pcclib_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The fastcam_pcclib_record pointer for record '%s' "
			"is NULL.",
			vinput->record->name, calling_fname );
		}

		*fastcam_pcclib = (MX_FASTCAM_PCCLIB *)
				fastcam_pcclib_record->record_type_struct;

		if ( *fastcam_pcclib == (MX_FASTCAM_PCCLIB *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_FASTCAM_PCCLIB pointer for record '%s' used "
			"by record '%s' is NULL.",
				vinput->record->name,
				fastcam_pcclib_record->name );
		}
	}

	if ( camera_control != (CCameraControl **) NULL ) {
		camera_control_ptr = fastcam_pcclib_camera_ptr->camera_control;

		if ( camera_control_ptr == (CCameraControl *) NULL )  {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The CCameraControl pointer for record '%s' is NULL.",
				vinput->record->name );
		}

		*camera_control = camera_control_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_fastcam_pcclib_camera_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_fastcam_pcclib_camera_create_record_structures()";

	MX_VIDEO_INPUT *vinput;
	MX_FASTCAM_PCCLIB_CAMERA *fastcam_pcclib_camera = NULL;

	vinput = (MX_VIDEO_INPUT *) malloc( sizeof(MX_VIDEO_INPUT) );

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VIDEO_INPUT structure." );
	}

	fastcam_pcclib_camera = (MX_FASTCAM_PCCLIB_CAMERA *)
				malloc( sizeof(MX_FASTCAM_PCCLIB_CAMERA) );

	if ( fastcam_pcclib_camera == (MX_FASTCAM_PCCLIB_CAMERA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_FASTCAM_PCCLIB_CAMERA structure." );
	}

	record->record_class_struct = vinput;
	record->record_type_struct = fastcam_pcclib_camera;
	record->class_specific_function_list = 
			&mxd_fastcam_pcclib_camera_video_input_function_list;

	memset( &(vinput->sequence_parameters),
			0, sizeof(vinput->sequence_parameters) );

	vinput->record = record;
	fastcam_pcclib_camera->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_fastcam_pcclib_camera_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_fastcam_pcclib_camera_finish_record_initialization()";

	MX_VIDEO_INPUT *vinput;
	MX_FASTCAM_PCCLIB_CAMERA *fastcam_pcclib_camera = NULL;
	MX_FASTCAM_PCCLIB *fastcam_pcclib = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_fastcam_pcclib_camera_get_pointers( vinput,
			&fastcam_pcclib_camera, &fastcam_pcclib, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	mx_status = mx_video_input_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_fastcam_pcclib_camera_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_fastcam_pcclib_camera_open()";

	MX_VIDEO_INPUT *vinput;
	MX_FASTCAM_PCCLIB_CAMERA *fastcam_pcclib_camera = NULL;
	MX_FASTCAM_PCCLIB *fastcam_pcclib = NULL;
	int pcclib_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_fastcam_pcclib_camera_get_pointers( vinput,
			&fastcam_pcclib_camera, &fastcam_pcclib, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	/*---------------------------------------------------------------*/

	/* Create a camera control for this camera. */

	CCameraControl *camera_control =
			new CCameraControl(DEVICE_SELECT_AUTO);

	if ( camera_control == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot create a CCameraControl object for camera '%s'.",
			record->name );
	}

	fastcam_pcclib_camera->camera_control = camera_control;

	/* How many cameras are attached to this camera control? */

	int max_cameras = 0;

	pcclib_status = camera_control->GetNumberOfDevice( max_cameras );

	if ( pcclib_status != PCC_ERROR_NOERROR ) {
		(void) mx_error( MXE_DEVICE_IO_ERROR, fname,
		"GetNumberOfDevices returned error %d for camera '%s'.",
			pcclib_status, record->name );
	}

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s: max_cameras = %d", fname, max_cameras));
#endif

	/* Initialize the camera. */

	pcclib_status =
		camera_control->Init( fastcam_pcclib_camera->camera_number );

	if ( pcclib_status != PCC_ERROR_NOERROR ) {
		(void) mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Initializing camera '%s' failed with error code %d",
			record->name, pcclib_status );
	}

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

	vinput->master_clock = MXF_VIN_MASTER_VIDEO_BOARD;

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

	vinput->last_frame_number = -1;
	vinput->total_num_frames = 0;
	vinput->status = 0x0;

	vinput->maximum_frame_number = 0;

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_OPEN
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

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_fastcam_pcclib_camera_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_fastcam_pcclib_camera_close()";

	MX_VIDEO_INPUT *vinput = NULL;
	MX_FASTCAM_PCCLIB_CAMERA *fastcam_pcclib_camera = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_fastcam_pcclib_camera_get_pointers( vinput,
				&fastcam_pcclib_camera, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_fastcam_pcclib_camera_arm( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_fastcam_pcclib_camera_arm()";

	MX_FASTCAM_PCCLIB_CAMERA *fastcam_pcclib_camera = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	CCameraControl *camera_control;
	double exposure_time, frame_time, frames_per_second;
	int num_frames, pcclib_status;
	mx_status_type mx_status;

	mx_status = mxd_fastcam_pcclib_camera_get_pointers( vinput,
			&fastcam_pcclib_camera, NULL, &camera_control, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_ARM
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	/* For this video input, the input frame buffer must be set up
	 * before we can execute the Snap() method.  For that reason,
	 * we do mx_image_alloc() here to make sure that the frame
	 * is set up.
	 */

	mx_status = mx_image_alloc( &(vinput->frame),
					vinput->framesize[0],
					vinput->framesize[1],
					vinput->image_format,
					vinput->byte_order,
					vinput->bytes_per_pixel,
					MXT_IMAGE_HEADER_LENGTH_IN_BYTES,
					vinput->bytes_per_frame, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Figure out the number of frames and the exposure time
	 * for the upcoming imaging sequence.
	 */

	sp = &(vinput->sequence_parameters);

	frame_time = -1;

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		num_frames = 1;
		exposure_time = sp->parameter_array[0];
		break;
	case MXT_SQ_MULTIFRAME:
		num_frames = sp->parameter_array[0];
		exposure_time = sp->parameter_array[1];
		frame_time = sp->parameter_array[2];

		/* sp->parameter_array[2] contains a "frame time"
		 * which is the time interval between the start
		 * of two successive frames.  However, Sapera LT
		 * does not seem to provide a way to use this,
		 * so we just skip it here.
		 */
		break;
	case MXT_SQ_STROBE:
	case MXT_SQ_GATED:
		num_frames = sp->parameter_array[0];
		exposure_time = sp->parameter_array[1];
		break;
	case MXT_SQ_DURATION:
		num_frames = sp->parameter_array[0];
		exposure_time = 1.0;		/* This will be ignored. */
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		    "Sequence type %ld has not yet been implemented for '%s'.",
				sp->sequence_type, vinput->record->name );
		break;
	}
	
#if 0
	if ( num_frames > fastcam_pcclib_camera->max_frames ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The number of frames requested (%d) for this sequence "
		"exceeds the maximum number of frames (%ld) allocated "
		"for detector '%s'.",
			num_frames,
			fastcam_pcclib_camera->max_frames,
			vinput->record->name );
	}
#endif
	
	/* If the video board is in charge of timing, then we must set
	 * the exposure time.
	 */

	if ( vinput->master_clock == MXF_VIN_MASTER_VIDEO_BOARD ) {
	
		int shutter_speed = 
			mx_round( mx_divide_safely( 1.0, exposure_time) );

		pcclib_status = camera_control->SetShutterSpeed(shutter_speed);

		if ( pcclib_status != PCC_ERROR_NOERROR ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to set an exposure time of %g seconds "
			"for camera '%s' failed with error %d.",
				exposure_time, vinput->record->name,
				pcclib_status );
		}

		if ( frame_time > 0.0 ) {
			frames_per_second = 1.0 / frame_time;

			pcclib_status = camera_control->SetRecordRate(
							frames_per_second );

			if ( pcclib_status != PCC_ERROR_NOERROR ) {
				return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to set the frames per second to %g "
			"for camera '%s' failed with error %d.",
				frames_per_second, vinput->record->name,
				pcclib_status );
			}
		}
	}

	/*--------*/

	/* Tell the frame grabber to wait for incoming image frames. */

	/* FIXME: How do we tell the camera how many frames to acquire??? */

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_ARM
	MX_DEBUG(-2,("%s complete for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_fastcam_pcclib_camera_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_fastcam_pcclib_camera_trigger()";

	MX_FASTCAM_PCCLIB_CAMERA *fastcam_pcclib_camera = NULL;
	BOOL sapera_status;
	mx_status_type mx_status;

	mx_status = mxd_fastcam_pcclib_camera_get_pointers( vinput,
				&fastcam_pcclib_camera, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_TRIGGER
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	if ( ( vinput->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) == 0 ) {

		/* If internal triggering is not enabled,
		 * return without doing anything.
		 */

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_TRIGGER
		MX_DEBUG(-2,
		("%s: internal trigger disabled for video input '%s'",
			fname, vinput->record->name));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, we are doing internal triggering. */

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_TRIGGER
	MX_DEBUG(-2,("%s: Sending internal trigger for '%s'.",
		fname, vinput->record->name));
#endif

	/*************************************************************
	 * FIXME: We need to figure out the interactions between the *
	 * Live, Playback, Record, etc. modes.  We will probably be  *
	 * using OnLive() and OnRecord() for this.                   *
	 *************************************************************/

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_fastcam_pcclib_camera_abort( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_fastcam_pcclib_camera_abort()";

	MX_FASTCAM_PCCLIB_CAMERA *fastcam_pcclib_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_fastcam_pcclib_camera_get_pointers( vinput,
				&fastcam_pcclib_camera, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	/* FIXME: Nothing here yet. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_fastcam_pcclib_camera_get_extended_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
		"mxd_fastcam_pcclib_camera_get_extended_status()";

	MX_FASTCAM_PCCLIB_CAMERA *fastcam_pcclib_camera = NULL;
	CCameraControl *camera_control = NULL;
	int pcclib_status;
	mx_status_type mx_status;

	mx_status = mxd_fastcam_pcclib_camera_get_pointers( vinput,
			&fastcam_pcclib_camera, NULL, &camera_control, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vinput->status = 0;

	/* Get the current state of the camera parameters. */

	CAMERA_PARAMS camera_params;

	pcclib_status = camera_control->GetCameraParams( camera_params );

	if ( pcclib_status != PCC_ERROR_NOERROR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to get the camera params for camera '%s' "
		"failed with error %d.",
			vinput->record->name, pcclib_status );
	}

	switch( camera_params.camera_mode ) {
	case CAMERA_MODE_LIVE:
	case CAMERA_MODE_REC:
	case CAMERA_MODE_ENDLESS:
		vinput->status |= MXSF_VIN_IS_BUSY;
		break;
	case CAMERA_MODE_PLAY:
	case CAMERA_MODE_RECREADY:
	case CAMERA_MODE_RECORDED:
		vinput->status &= (~MXSF_VIN_IS_BUSY);
		break;
	default:
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unrecognized camera mode %d reported by GetCameraParams() "
		"for camera '%s'.",
			camera_params.camera_mode,
			vinput->record->name );
		break;
	}

	if ( vinput->status & MXSF_VIN_IS_BUSY ) {
		vinput->busy = TRUE;
	} else {
		vinput->busy = FALSE;
	}

	/* FIXME FIXME FIXME!!! We do not yet know how to determine
	 *                      last_frame_number or total_num_frames!!!
	 */

	vinput->last_frame_number = 0;
	vinput->total_num_frames = 1;

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_EXTENDED_STATUS

	MX_DEBUG(-2,("%s: last_frame_number = %ld, "
			"total_num_frames = %ld, status = %#lx",
			fname, vinput->last_frame_number,
			vinput->total_num_frames,
			vinput->status));

#elif MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_EXTENDED_STATUS_WHEN_BUSY

	if ( vinput->busy ) {
		MX_DEBUG(-2,("%s: last_frame_number = %ld, "
			"total_num_frames = %ld, status = %#lx",
			fname, vinput->last_frame_number,
			vinput->total_num_frames,
			vinput->status));
	}

#endif
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_fastcam_pcclib_camera_get_frame( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_fastcam_pcclib_camera_get_frame()";

	MX_FASTCAM_PCCLIB_CAMERA *fastcam_pcclib_camera = NULL;
	MX_IMAGE_FRAME *frame;
	CCameraControl *camera_control;
	int pcclib_status;
	mx_status_type mx_status;

	mx_status = mxd_fastcam_pcclib_camera_get_pointers( vinput,
			&fastcam_pcclib_camera, NULL, &camera_control, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	frame = vinput->frame;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_GET_FRAME
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif
	BYTE *image_buffer = (BYTE *) vinput->frame->image_data;

	/* FIXME: What is the real difference between using TransferFrame()
	 * and Transfer16BitFrame() (which uses a WORD pointer)?
	 */

	pcclib_status = camera_control->TransferFrame( vinput->frame_number,
								image_buffer );

	if ( pcclib_status != PCC_ERROR_NOERROR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to read frame %ld from camera '%s' failed "
		"with error code %d.", vinput->frame_number,
			vinput->record->name, pcclib_status );
	}

	/* FIXME: GetTimeCodeFromFrame() may be able to get me a timestamp
	 * for the image if I can figure out what an IRIGLIB_TIMECODE is.
	 * Or perhaps use a combination of the methods GetDateOfRecording() 
	 * GetTimeOfRecording().
	 */

	MX_SEQUENCE_PARAMETERS *sp;
	double exposure_time;

	sp = &(vinput->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		exposure_time = sp->parameter_array[0];
		break;
	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_STROBE:
	case MXT_SQ_GATED:
		exposure_time = sp->parameter_array[1];
		break;
	case MXT_SQ_DURATION:
		exposure_time = 1.0;
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Sequence type %ld has not yet been implemented for '%s'.",
			sp->sequence_type, vinput->record->name );
		break;
	}

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_GET_FRAME
	MX_DEBUG(-2,("%s: exposure_time = %f", fname, exposure_time));
#endif

	struct timespec exposure_timespec =
	    mx_convert_seconds_to_high_resolution_time( exposure_time );

	MXIF_EXPOSURE_TIME_SEC( vinput->frame )  = exposure_timespec.tv_sec;

	MXIF_EXPOSURE_TIME_NSEC( vinput->frame ) = exposure_timespec.tv_nsec;

	/*----*/

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_GET_FRAME
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_fastcam_pcclib_camera_get_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
			"mxd_fastcam_pcclib_camera_get_parameter()";

	MX_FASTCAM_PCCLIB_CAMERA *fastcam_pcclib_camera = NULL;
	CCameraControl *camera_control = NULL;
	int pcclib_status;
	mx_status_type mx_status;

	mx_status = mxd_fastcam_pcclib_camera_get_pointers( vinput,
			&fastcam_pcclib_camera, NULL, &camera_control, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_MX_PARAMETERS
	MX_DEBUG(-2,("%s: record '%s', parameter '%s' (%ld)",
		fname, vinput->record->name,
	mx_get_field_label_string( vinput->record, vinput->parameter_type ),
			vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
		DWORD resolution;

		pcclib_status = camera_control->GetResolution( resolution );

		if ( pcclib_status != PCC_ERROR_NOERROR ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to get the resolution of "
			"camera '%s' failed with error code %d.", 
				vinput->record->name, pcclib_status );
		}

		vinput->framesize[0] = ( resolution >> 16 ) & 0xffff;
		vinput->framesize[1] = resolution & 0xffff;

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG
		MX_DEBUG(-2,("%s: camera '%s' framesize = (%ld,%ld).",
			fname, fastcam_pcclib_camera->record->name,
			vinput->framesize[0], vinput->framesize[1]));
#endif

		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:
		/* FIXME: How do we _know_ the video format as opposed to
		 *        unilaterally asserting it?
		 */

		vinput->image_format = MXT_IMAGE_FORMAT_GREY16;

		mx_status = mx_image_get_image_format_name_from_type(
				vinput->image_format, vinput->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_MX_PARAMETERS
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
mxd_fastcam_pcclib_camera_set_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
			"mxd_fastcam_pcclib_camera_set_parameter()";

	MX_FASTCAM_PCCLIB_CAMERA *fastcam_pcclib_camera = NULL;
	CCameraControl *camera_control;
	int pcclib_status;
	mx_status_type mx_status;

	mx_status = mxd_fastcam_pcclib_camera_get_pointers( vinput,
			&fastcam_pcclib_camera, NULL, &camera_control, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_FASTCAM_PCCLIB_CAMERA_DEBUG_MX_PARAMETERS
	MX_DEBUG(-2,("%s: record '%s', parameter '%s' (%ld)",
		fname, vinput->record->name,
	mx_get_field_label_string( vinput->record, vinput->parameter_type ),
			vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
		unsigned long width, height;
		DWORD requested_resolution;
		int i;

		width  = vinput->framesize[0];
		height = vinput->framesize[1];

		requested_resolution  = (width & 0xffff) << 16;
		requested_resolution |= (height & 0xffff);

		/* Is this resolution allowed? */

		DWORD resolution_list[LIST_MAX_NUMBER];

		pcclib_status = camera_control->GetResolutionList(
				resolution_list, LIST_MAX_NUMBER );

		if ( pcclib_status != PCC_ERROR_NOERROR ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to get the allowed framesizes for "
			"camera '%s' failed with error %d.",
				vinput->record->name, pcclib_status );
		}

		for ( i = 0; i < LIST_MAX_NUMBER; i++ ) {
			if ( requested_resolution == resolution_list[i] ) {
				break;		/* Exit the for() loop. */
			}
		}

		if ( i >= LIST_MAX_NUMBER ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested framesize(%lu,%lu) for camera '%s' "
			"is not supported.",
				width, height, vinput->record->name );
		}

		pcclib_status = camera_control->SetResolution(
					requested_resolution );

		if ( pcclib_status != PCC_ERROR_NOERROR ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to set the framesize to (%lu,%lu) for "
			"camera '%s' failed with error %d.",
				width, height,
				vinput->record->name, pcclib_status );
		}
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

