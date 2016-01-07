/*
 * Name:    d_dalsa_gev_camera.c
 *
 * Purpose: MX video input driver for a DALSA GigE-Vision camera
 *          controlled via the Gev API.
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

#define MXD_DALSA_GEV_CAMERA_DEBUG	FALSE

#include <stdio.h>

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
#include "i_dalsa_gev.h"
#include "d_dalsa_gev_camera.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_dalsa_gev_camera_record_function_list = {
	NULL,
	mxd_dalsa_gev_camera_create_record_structures,
	mxd_dalsa_gev_camera_finish_record_initialization,
	NULL,
	NULL,
	mxd_dalsa_gev_camera_open,
	mxd_dalsa_gev_camera_close,
	NULL,
	NULL,
	mxd_dalsa_gev_camera_special_processing_setup
};

MX_VIDEO_INPUT_FUNCTION_LIST
mxd_dalsa_gev_camera_video_input_function_list = {
	mxd_dalsa_gev_camera_arm,
	mxd_dalsa_gev_camera_trigger,
	mxd_dalsa_gev_camera_stop,
	mxd_dalsa_gev_camera_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_dalsa_gev_camera_get_extended_status,
	mxd_dalsa_gev_camera_get_frame,
	mxd_dalsa_gev_camera_get_parameter,
	mxd_dalsa_gev_camera_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_dalsa_gev_camera_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VIDEO_INPUT_STANDARD_FIELDS,
	MXD_DALSA_GEV_CAMERA_STANDARD_FIELDS
};

long mxd_dalsa_gev_camera_num_record_fields
	= sizeof( mxd_dalsa_gev_camera_record_field_defaults )
	/ sizeof( mxd_dalsa_gev_camera_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_dalsa_gev_camera_rfield_def_ptr
			= &mxd_dalsa_gev_camera_record_field_defaults[0];

/*---*/

static mx_status_type mxd_dalsa_gev_camera_process_function( void *record_ptr,
							void *record_field_ptr,
							int operation );
static mx_status_type
mxd_dalsa_gev_camera_get_pointers( MX_VIDEO_INPUT *vinput,
			MX_DALSA_GEV_CAMERA **dalsa_gev_camera,
			MX_DALSA_GEV **dalsa_gev,
			const char *calling_fname )
{
	static const char fname[] =
		"mxd_dalsa_gev_camera_get_pointers()";

	MX_DALSA_GEV_CAMERA *dalsa_gev_camera_ptr;
	MX_RECORD *dalsa_gev_record;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	dalsa_gev_camera_ptr = (MX_DALSA_GEV_CAMERA *)
				vinput->record->record_type_struct;

	if ( dalsa_gev_camera_ptr == (MX_DALSA_GEV_CAMERA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_DALSA_GEV_CAMERA pointer for "
			"record '%s' passed by '%s' is NULL.",
			vinput->record->name, calling_fname );
	}

	if ( dalsa_gev_camera != (MX_DALSA_GEV_CAMERA **) NULL ) {
		*dalsa_gev_camera = dalsa_gev_camera_ptr;
	}

	if ( dalsa_gev != (MX_DALSA_GEV **) NULL ) {
		dalsa_gev_record = dalsa_gev_camera_ptr->dalsa_gev_record;

		if ( dalsa_gev_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The dalsa_gev_record pointer for record '%s' "
			"is NULL.", vinput->record->name );
		}

		*dalsa_gev = (MX_DALSA_GEV *)
					dalsa_gev_record->record_type_struct;

		if ( *dalsa_gev == (MX_DALSA_GEV *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_DALSA_GEV pointer for record '%s' used "
			"by record '%s' is NULL.",
				vinput->record->name,
				dalsa_gev_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_dalsa_gev_camera_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_dalsa_gev_camera_create_record_structures()";

	MX_VIDEO_INPUT *vinput;
	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;

	vinput = (MX_VIDEO_INPUT *) malloc( sizeof(MX_VIDEO_INPUT) );

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VIDEO_INPUT structure." );
	}

	dalsa_gev_camera = (MX_DALSA_GEV_CAMERA *)
				malloc( sizeof(MX_DALSA_GEV_CAMERA) );

	if ( dalsa_gev_camera == (MX_DALSA_GEV_CAMERA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_DALSA_GEV_CAMERA structure." );
	}

	record->record_class_struct = vinput;
	record->record_type_struct = dalsa_gev_camera;
	record->class_specific_function_list = 
			&mxd_dalsa_gev_camera_video_input_function_list;

	memset( &(vinput->sequence_parameters),
			0, sizeof(vinput->sequence_parameters) );

	vinput->record = record;
	dalsa_gev_camera->record = record;

	dalsa_gev_camera->camera_index = -1;
	dalsa_gev_camera->camera_object = NULL;
	dalsa_gev_camera->camera_handle = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dalsa_gev_camera_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_dalsa_gev_camera_finish_record_initialization()";

	MX_VIDEO_INPUT *vinput;
	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	MX_DALSA_GEV *dalsa_gev = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
				&dalsa_gev_camera, &dalsa_gev, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DALSA_GEV_CAMERA_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	mx_status = mx_video_input_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dalsa_gev_camera_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_dalsa_gev_camera_open()";

	MX_VIDEO_INPUT *vinput;
	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	MX_DALSA_GEV *dalsa_gev = NULL;
	GEV_CAMERA_INFO *camera_object, *selected_camera_object;
	char *serial_number_string;
	long i, gev_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
				&dalsa_gev_camera, &dalsa_gev, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DALSA_GEV_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	/* Walk through the list of GEV_CAMERA_INFO objects to find the
	 * camera that has the specified serial number.
	 */

	selected_camera_object = NULL;

	serial_number_string = dalsa_gev_camera->serial_number;

	for ( i = 0; i < dalsa_gev->num_cameras; i++ ) {

		camera_object = &(dalsa_gev->camera_array[i]);

		if (strcmp( serial_number_string, camera_object->serial ) == 0)
		{
			dalsa_gev_camera->camera_index = i;

			dalsa_gev_camera->camera_object = camera_object;

			selected_camera_object = camera_object;
		}
	}

	if ( selected_camera_object == (GEV_CAMERA_INFO *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"A DALSA GigE-Vision camera with serial number '%s' "
		"was not found for camera record '%s'.",
			serial_number_string,
			record->name );
	}

	gev_status = GevOpenCamera( selected_camera_object,
					GevExclusiveMode,
					&(dalsa_gev_camera->camera_handle) );

	switch ( gev_status ) {
	case GEVLIB_OK:
		break;
	case GEVLIB_ERROR_API_NOT_INITIALIZED:
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"GEVLIB_ERROR_API_NOT_INITIALIZED: The Gev API library "
		"has not yet been initialized for camera record '%s'.",
			record->name );
		break;
	case GEVLIB_ERROR_INVALID_HANDLE:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"GEVLIB_ERROR_INVALID_HANDLE: Something is wrong with "
		"the camera handle pointer passed to GevOpenCamera() "
		"for camera record '%s'.",
			record->name );
		break;
	case GEVLIB_ERROR_INSUFFICIENT_MEMORY:
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"GEVLIB_ERROR_INSUFFICIENT_MEMORY: Ran out of memory trying "
		"to allocate Gev API data structures for camera record '%s'.",
			record->name );
		break;
	case GEVLIB_ERROR_NO_CAMERA:
		return mx_error( MXE_NOT_FOUND, fname,
		"GEVLIB_ERROR_NO_CAMERA: No DALSA GigE-Vision camera "
		"was found with serial number '%s' for camera record '%s'.",
			dalsa_gev_camera->serial_number,
			record->name );
		break;
	case GEV_STATUS_ACCESS_DENIED:
		return mx_error( MXE_PERMISSION_DENIED, fname,
		"GEV_STATUS_ACCESS_DENIED: For some reason we were "
		"not permitted access to the Gev API for camera record '%s'.",
			record->name );
		break;
	default:
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"A call to GevOpenCamera() for camera record '%s' "
		"failed with status code %ld.",
			record->name, gev_status );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dalsa_gev_camera_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_dalsa_gev_camera_close()";

	MX_VIDEO_INPUT *vinput = NULL;
	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
					&dalsa_gev_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DALSA_GEV_CAMERA_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dalsa_gev_camera_arm( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_dalsa_gev_camera_arm()";

	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
					&dalsa_gev_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DALSA_GEV_CAMERA_DEBUG_ARM
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dalsa_gev_camera_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_dalsa_gev_camera_trigger()";

	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
					&dalsa_gev_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dalsa_gev_camera_stop( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_dalsa_gev_camera_stop()";

	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
					&dalsa_gev_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DALSA_GEV_CAMERA_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dalsa_gev_camera_abort( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_dalsa_gev_camera_abort()";

	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
					&dalsa_gev_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DALSA_GEV_CAMERA_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dalsa_gev_camera_get_extended_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
		"mxd_dalsa_gev_camera_get_extended_status()";

	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
					&dalsa_gev_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vinput->status = 0;

	if ( vinput->status & MXSF_VIN_IS_BUSY ) {
		vinput->busy = TRUE;
	} else {
		vinput->busy = FALSE;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dalsa_gev_camera_get_frame( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_dalsa_gev_camera_get_frame()";

	MX_DALSA_GEV_CAMERA *dalsa_gev = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
					&dalsa_gev, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*----*/

#if MXD_DALSA_GEV_CAMERA_DEBUG_GET_FRAME
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dalsa_gev_camera_get_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
			"mxd_dalsa_gev_camera_get_parameter()";

	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
					&dalsa_gev_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DALSA_GEV_CAMERA_DEBUG_MX_PARAMETERS
	MX_DEBUG(-2,("%s: record '%s', parameter '%s' (%ld)",
		fname, vinput->record->name,
	mx_get_field_label_string( vinput->record, vinput->parameter_type ),
			vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:

#if MXD_DALSA_GEV_CAMERA_DEBUG
		MX_DEBUG(-2,("%s: camera '%s' framesize = (%ld,%ld).",
			fname, dalsa_gev_camera->record->name,
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

#if MXD_DALSA_GEV_CAMERA_DEBUG_MX_PARAMETERS
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
mxd_dalsa_gev_camera_set_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_dalsa_gev_camera_set_parameter()";

	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
					&dalsa_gev_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DALSA_GEV_CAMERA_DEBUG_MX_PARAMETERS
	MX_DEBUG(-2,("%s: record '%s', parameter '%s' (%ld)",
		fname, vinput->record->name,
	mx_get_field_label_string( vinput->record, vinput->parameter_type ),
			vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_MARK_FRAME_AS_SAVED:

		break;

	case MXLV_VIN_FRAMESIZE:
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
mxd_dalsa_gev_camera_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_DALSA_GEV_CAMERA_SHOW_FEATURES:
			record_field->process_function
				= mxd_dalsa_gev_camera_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_dalsa_gev_camera_process_function( void *record_ptr,
				void *record_field_ptr,
				int operation )
{
	static const char fname[] = "mxd_dalsa_gev_camera_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_VIDEO_INPUT *vinput;
	MX_DALSA_GEV_CAMERA *dalsa_gev_camera;
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

	dalsa_gev_camera = (MX_DALSA_GEV_CAMERA *) record->record_type_struct;

	if ( dalsa_gev_camera == (MX_DALSA_GEV_CAMERA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DALSA_GEV_CAMERA pointer for record '%s' is NULL.",
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
		case MXLV_DALSA_GEV_CAMERA_SHOW_FEATURES:
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

