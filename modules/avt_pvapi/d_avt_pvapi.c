/*
 * Name:    d_avt_pvapi.c
 *
 * Purpose: MX driver for video inputs using the PvAPI C API
 *          for cameras from Allied Vision Technologies.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013-2014, 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_AVT_PVAPI_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_bit.h"
#include "mx_image.h"
#include "mx_video_input.h"

#include "i_avt_pvapi.h"
#include "d_avt_pvapi.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_avt_pvapi_record_function_list = {
	NULL,
	mxd_avt_pvapi_create_record_structures,
	mx_video_input_finish_record_initialization,
	NULL,
	NULL,
	mxd_avt_pvapi_open,
	mxd_avt_pvapi_close
};

MX_VIDEO_INPUT_FUNCTION_LIST mxd_avt_pvapi_video_input_function_list = {
	mxd_avt_pvapi_arm,
	mxd_avt_pvapi_trigger,
	mxd_avt_pvapi_stop,
	mxd_avt_pvapi_abort,
	NULL,
	NULL,
	NULL,
	mxd_avt_pvapi_get_status,
	NULL,
	mxd_avt_pvapi_get_frame,
	mxd_avt_pvapi_get_parameter,
	mxd_avt_pvapi_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_avt_pvapi_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VIDEO_INPUT_STANDARD_FIELDS,
	MXD_AVT_PVAPI_STANDARD_FIELDS
};

long mxd_avt_pvapi_num_record_fields
		= sizeof( mxd_avt_pvapi_record_field_defaults )
			/ sizeof( mxd_avt_pvapi_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_avt_pvapi_rfield_def_ptr
			= &mxd_avt_pvapi_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_avt_pvapi_get_pointers( MX_VIDEO_INPUT *vinput,
			MX_AVT_PVAPI_CAMERA **avt_pvapi_camera,
			MX_AVT_PVAPI **avt_pvapi,
			const char *calling_fname )
{
	static const char fname[] = "mxd_avt_pvapi_get_pointers()";

	MX_AVT_PVAPI_CAMERA *avt_pvapi_camera_ptr;
	MX_RECORD *avt_pvapi_record;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	avt_pvapi_camera_ptr = vinput->record->record_type_struct;

	if ( avt_pvapi_camera_ptr == (MX_AVT_PVAPI_CAMERA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AVT_PVAPI_CAMERA pointer for record '%s' is NULL.",
			vinput->record->name );
	}

	if ( avt_pvapi_camera != (MX_AVT_PVAPI_CAMERA **) NULL ) {
		*avt_pvapi_camera = avt_pvapi_camera_ptr;
	}

	if ( avt_pvapi != (MX_AVT_PVAPI **) NULL ) {
		avt_pvapi_record = avt_pvapi_camera_ptr->avt_pvapi_record;

		if ( avt_pvapi_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The avt_pvapi_record pointer for record '%s' is NULL.",
				vinput->record->name );
		}

		*avt_pvapi = avt_pvapi_record->record_type_struct;

		if ( (*avt_pvapi) == (MX_AVT_PVAPI *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_AVT_PVAPI pointer for record '%s' is NULL.",
				vinput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_avt_pvapi_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_avt_pvapi_create_record_structures()";

	MX_VIDEO_INPUT *vinput;
	MX_AVT_PVAPI_CAMERA *avt_pvapi_camera = NULL;

	vinput = (MX_VIDEO_INPUT *) malloc( sizeof(MX_VIDEO_INPUT) );

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VIDEO_INPUT structure." );
	}

	avt_pvapi_camera = (MX_AVT_PVAPI_CAMERA *)
				malloc( sizeof(MX_AVT_PVAPI_CAMERA));

	if ( avt_pvapi_camera == (MX_AVT_PVAPI_CAMERA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_AVT_PVAPI_CAMERA structure." );
	}

	record->record_class_struct = vinput;
	record->record_type_struct = avt_pvapi_camera;
	record->class_specific_function_list = 
			&mxd_avt_pvapi_video_input_function_list;

	memset( &(vinput->sequence_parameters),
			0, sizeof(vinput->sequence_parameters) );

	vinput->record = record;
	avt_pvapi_camera->record = record;

	vinput->bytes_per_frame = 0;
	vinput->bytes_per_pixel = 0;
	vinput->bits_per_pixel = 0;
	vinput->trigger_mode = 0;

	vinput->pixel_clock_frequency = 0.0;	/* in pixels/second */

	vinput->external_trigger_polarity = MXF_VIN_TRIGGER_NONE;

	vinput->camera_trigger_polarity = MXF_VIN_TRIGGER_NONE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_avt_pvapi_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_avt_pvapi_open()";

	MX_VIDEO_INPUT *vinput;
	MX_AVT_PVAPI_CAMERA *avt_pvapi_camera = NULL;
	MX_AVT_PVAPI *avt_pvapi = NULL;
	char *camera_name;
	unsigned long i, camera_name_length;
#if 0
	tPvAccessFlags access_flag;
#endif
	unsigned long inet_address;
	unsigned long unique_id;
	mx_bool_type is_unique_id;
	unsigned long pvapi_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_avt_pvapi_get_pointers( vinput,
					&avt_pvapi_camera, &avt_pvapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVT_PVAPI_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	camera_name = avt_pvapi_camera->camera_name;

	if ( strlen( camera_name ) == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The camera_name field for camera '%s' is empty.",
			record->name );
	}

	/* If the camera name contains only numbers in the range 0 to 9,
	 * then we assume the string contains the unique_id.  Otherwise,
	 * we assume that it is an IP address or IP name.
	 */

	camera_name_length = strlen( camera_name );

	is_unique_id = TRUE;

	for ( i = 0; i < camera_name_length; i++ ) {
		if ( isdigit( camera_name[i] ) == FALSE ) {
			is_unique_id = FALSE;
		}
	}

	if ( is_unique_id ) {
		unique_id = atol( camera_name );

		MX_DEBUG(-2,
		("%s: Attempting to use camera '%s' unique_id = %lu",
			fname, record->name, unique_id));

		pvapi_status = PvCameraInfoEx( unique_id,
					&( avt_pvapi_camera->camera_info ),
					sizeof(tPvCameraInfoEx) );

	} else {
		/* Assume that the user has specified an IP address or name. */

		MX_DEBUG(-2,
		("%s: Attempting to use camera '%s' IP name or address = '%s'",
			fname, record->name, camera_name));

		mx_status = mx_socket_get_inet_address(
					avt_pvapi_camera->camera_name,
					&inet_address );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		pvapi_status = PvCameraInfoByAddrEx( inet_address,
					&( avt_pvapi_camera->camera_info ),
					NULL,
					sizeof(tPvCameraInfoEx) );

		unique_id = avt_pvapi_camera->camera_info.UniqueId;
	}

	MX_DEBUG(-2,("%s: pvapi_status = %lu", fname, pvapi_status));

	switch( pvapi_status ) {
	case ePvErrSuccess:
		MX_DEBUG(-2,("%s: Camera '%s' successfully found.",
			fname, record->name ));
		break;
	case ePvErrNotFound:
		return mx_error( MXE_NOT_FOUND, fname,
		"Camera '%s' with unique id %lu was not found.",
				record->name, unique_id );
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The attempt to get camera info for camera '%s' "
		"with camera id '%s' failed with error code %lu.",
			record->name, camera_name, pvapi_status );
		break;
	}

	avt_pvapi_camera->unique_id = avt_pvapi_camera->camera_info.UniqueId;

	MX_DEBUG(-2,("%s: Camera '%s' has unique_id = %lu",
		fname, record->name,
		avt_pvapi_camera->unique_id));

	/* Display some information about the camera. */

#if 1
	MX_DEBUG(-2,("%s: Camera '%s':", fname, record->name ));
	MX_DEBUG(-2,("%s:   Version = %lu",
		fname, avt_pvapi_camera->camera_info.StructVer ));
	MX_DEBUG(-2,("%s:   Unique ID = %lu",
		fname, avt_pvapi_camera->camera_info.UniqueId ));
	MX_DEBUG(-2,("%s:   Camera Name = '%s'",
		fname, avt_pvapi_camera->camera_info.CameraName ));
	MX_DEBUG(-2,("%s:   Model Name = '%s'",
		fname, avt_pvapi_camera->camera_info.ModelName ));
	MX_DEBUG(-2,("%s:   Part Number = '%s'",
		fname, avt_pvapi_camera->camera_info.PartNumber ));
	MX_DEBUG(-2,("%s:   Serial Number = '%s'",
		fname, avt_pvapi_camera->camera_info.SerialNumber ));
	MX_DEBUG(-2,("%s:   Firmware Version = '%s'",
		fname, avt_pvapi_camera->camera_info.FirmwareVersion ));
	MX_DEBUG(-2,("%s:   Permitted Access = %lu",
		fname, avt_pvapi_camera->camera_info.PermittedAccess ));
	MX_DEBUG(-2,("%s:   Interface ID = %lu",
		fname, avt_pvapi_camera->camera_info.InterfaceId ));
	MX_DEBUG(-2,("%s:   Interface Type = %u",
		fname, avt_pvapi_camera->camera_info.InterfaceType ));
#endif

	/* Now try to open the camera and get a PvAPI handle. */

	pvapi_status = PvCameraOpen( unique_id,
				ePvAccessMaster,
				&(avt_pvapi_camera->camera_handle) );

	switch( pvapi_status ) {
	case ePvErrSuccess:
		MX_DEBUG(-2,("%s: Camera '%s' was successfully opened.",
			fname, record->name));
		break;
	case ePvErrUnplugged:
		return mx_error( MXE_NOT_READY, fname,
			"Camera '%s' is unplugged.", record->name );
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"Unable to open camera '%s' (camera name = '%s').  "
		"PvAPI error code = %lu",
			record->name, camera_name, pvapi_status );
		break;
	}

	/* Initialize a bunch of MX driver parameters. */

	vinput->parameter_type = -1;
	vinput->frame_number   = -100;
	vinput->get_frame      = -100;
	vinput->frame          = NULL;
	vinput->frame_buffer   = NULL;
	vinput->byte_order     = mx_native_byteorder();
	vinput->trigger_mode   = MXF_DEV_INTERNAL_TRIGGER;

	mx_status = mx_video_input_get_image_format( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bits_per_pixel( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_pixel( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_framesize( record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_frame( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVT_PVAPI_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_avt_pvapi_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_avt_pvapi_close()";

	MX_VIDEO_INPUT *vinput;
	MX_AVT_PVAPI_CAMERA *avt_pvapi_camera = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_avt_pvapi_get_pointers( vinput,
					&avt_pvapi_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVT_PVAPI_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_avt_pvapi_arm( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_avt_pvapi_arm()";

	MX_AVT_PVAPI_CAMERA *avt_pvapi_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_avt_pvapi_get_pointers( vinput,
					&avt_pvapi_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVT_PVAPI_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	/* If external triggering is not enabled,
	 * return without doing anything further.
	 */

	if ( ( vinput->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER ) == 0 ) {

#if MXD_AVT_PVAPI_DEBUG
		MX_DEBUG(-2,
		("%s: external trigger disabled for video input '%s'",
			fname, vinput->record->name));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_avt_pvapi_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_avt_pvapi_trigger()";

	MX_AVT_PVAPI_CAMERA *avt_pvapi_camera = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	int num_frames, milliseconds;
	double exposure_time;
	mx_status_type mx_status;

	mx_status = mxd_avt_pvapi_get_pointers( vinput,
					&avt_pvapi_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVT_PVAPI_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif
	if ( ( vinput->trigger_mode & MXF_DEV_INTERNAL_TRIGGER ) == 0 ) {

		/* If internal triggering is not enabled,
		 * return without doing anything
		 */

#if MXD_AVT_PVAPI_DEBUG
		MX_DEBUG(-2,
		("%s: internal trigger disabled for video input '%s'",
			fname, vinput->record->name));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	/*---*/

	sp = &(vinput->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		exposure_time = sp->parameter_array[0];
		num_frames = 1;
		break;
	case MXT_SQ_MULTIFRAME:
		exposure_time = sp->parameter_array[1];
		num_frames = mx_round( sp->parameter_array[0] );
		break;
	default:
		exposure_time = -1;
		num_frames = 0;
		break;
	}

	if ( exposure_time >= 0.0 ) {
		/* Set the exposure time. */

		milliseconds = mx_round( 1000.0 * exposure_time );

#if MXD_AVT_PVAPI_DEBUG
		MX_DEBUG(-2,
	("%s: Setting exposure to %u milliseconds using pdv_set_exposure()",
			fname, milliseconds ));
#endif

	}

#if MXD_AVT_PVAPI_DEBUG
	MX_DEBUG(-2,("%s: About to trigger using pdv_start_images()", fname ));
#endif

	/* Start the actual data acquisition. */


#if MXD_AVT_PVAPI_DEBUG
	MX_DEBUG(-2,("%s: Started taking %d frame(s) using video input '%s'.",
		fname, num_frames, vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_avt_pvapi_stop( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_avt_pvapi_stop()";

	MX_AVT_PVAPI_CAMERA *avt_pvapi_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_avt_pvapi_get_pointers( vinput,
					&avt_pvapi_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVT_PVAPI_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_avt_pvapi_abort( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_avt_pvapi_abort()";

	MX_AVT_PVAPI_CAMERA *avt_pvapi_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_avt_pvapi_get_pointers( vinput,
					&avt_pvapi_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVT_PVAPI_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_avt_pvapi_get_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_avt_pvapi_get_status()";

	MX_AVT_PVAPI_CAMERA *avt_pvapi_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_avt_pvapi_get_pointers( vinput,
					&avt_pvapi_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_avt_pvapi_get_frame( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_avt_pvapi_get_frame()";

	MX_AVT_PVAPI_CAMERA *avt_pvapi_camera = NULL;
	MX_IMAGE_FRAME *frame;
	mx_status_type mx_status;

	mx_status = mxd_avt_pvapi_get_pointers( vinput,
					&avt_pvapi_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	frame = vinput->frame;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

#if MXD_AVT_PVAPI_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

#if MXD_AVT_PVAPI_DEBUG
	MX_DEBUG(-2,("%s: Finished reading out frame.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_avt_pvapi_get_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_avt_pvapi_get_parameter()";

	MX_AVT_PVAPI_CAMERA *avt_pvapi_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_avt_pvapi_get_pointers( vinput,
					&avt_pvapi_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVT_PVAPI_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_BYTE_ORDER:
	case MXLV_VIN_TRIGGER_MODE:
	case MXLV_VIN_SEQUENCE_TYPE:
	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:

		break;

	case MXLV_VIN_BYTES_PER_FRAME:
	case MXLV_VIN_BYTES_PER_PIXEL:
		switch( vinput->image_format ) {
		case MXT_IMAGE_FORMAT_RGB:
			vinput->bytes_per_pixel = 3;
			break;
		case MXT_IMAGE_FORMAT_GREY8:
			vinput->bytes_per_pixel = 1;
			break;
	
		case MXT_IMAGE_FORMAT_GREY16:
			vinput->bytes_per_pixel = 2;
			break;
	
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported image format %ld for video input '%s'.",
				vinput->image_format, vinput->record->name );
		}

		vinput->bytes_per_frame = mx_round( vinput->bytes_per_pixel )
				* vinput->framesize[0] * vinput->framesize[1];
		break;

	case MXLV_VIN_BITS_PER_PIXEL:
		vinput->bits_per_pixel = -1;
		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:

		mx_status = mx_image_get_image_format_name_from_type(
				vinput->image_format, vinput->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;

	case MXLV_VIN_FRAMESIZE:
		break;
	default:
		return mx_video_input_default_get_parameter_handler( vinput );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_avt_pvapi_set_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_avt_pvapi_set_parameter()";

	MX_AVT_PVAPI_CAMERA *avt_pvapi_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_avt_pvapi_get_pointers( vinput, &avt_pvapi_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVT_PVAPI_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_SEQUENCE_TYPE:
	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:

		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:
		(void) mxd_avt_pvapi_get_parameter( vinput );

		return mx_error( MXE_UNSUPPORTED, fname,
		"Changing the video format for video input '%s' via "
		"MX is not supported.", vinput->record->name );
		break;

	case MXLV_VIN_BYTE_ORDER:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the byte order for video input '%s' "
			"is not supported.", vinput->record->name );
		break;

	case MXLV_VIN_FRAMESIZE:

#if MXD_AVT_PVAPI_DEBUG
		MX_DEBUG(-2,("%s: setting '%s' framesize to (%lu, %lu)",
			fname, vinput->record->name,
			vinput->framesize[0], vinput->framesize[1]));
#endif
		break;

	case MXLV_VIN_TRIGGER_MODE:
		break;

	default:
		return mx_video_input_default_set_parameter_handler( vinput );
	}

	return MX_SUCCESSFUL_RESULT;
}

