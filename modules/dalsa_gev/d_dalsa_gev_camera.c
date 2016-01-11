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

#define MXD_DALSA_GEV_CAMERA_DEBUG_OPEN			TRUE
#define MXD_DALSA_GEV_CAMERA_DEBUG_ARM			TRUE
#define MXD_DALSA_GEV_CAMERA_DEBUG_STOP			TRUE
#define MXD_DALSA_GEV_CAMERA_DEBUG_GET_FRAME		TRUE
#define MXD_DALSA_GEV_CAMERA_DEBUG_MX_PARAMETERS	TRUE
#define MXD_DALSA_GEV_CAMERA_DEBUG_REGISTER_READ	FALSE
#define MXD_DALSA_GEV_CAMERA_DEBUG_REGISTER_WRITE	TRUE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_bit.h"
#include "mx_process.h"
#include "mx_thread.h"
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

#if 0
static mx_status_type mxd_dalsa_gev_camera_process_function( void *record_ptr,
							void *record_field_ptr,
							int operation );
#endif

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

/*---*/

static mx_status_type
mxd_dalsa_gev_camera_api_error( short gev_status,
				const char *location,
				const char *format, ... )
{
	va_list args;
	char identifier[2500];

	va_start( args, format );
	vsnprintf( identifier, sizeof(identifier), format, args );
	va_end( args );

	switch( gev_status ) {
	case GEVLIB_ERROR_GENERIC:
		return mx_error( MXE_UNKNOWN_ERROR, location,
		"GEVLIB_ERROR_GENERIC in '%s'.", identifier );
		break;
	case GEVLIB_ERROR_NULL_PTR:
		return mx_error( MXE_NULL_ARGUMENT, location,
		"GEVLIB_ERROR_NULL_PTR in '%s'.", identifier );
		break;
	case GEVLIB_ERROR_ARG_INVALID:
		return mx_error( MXE_ILLEGAL_ARGUMENT, location,
		"GEVLIB_ERROR_ARG_INVALID in '%s'.", identifier );
		break;
	case GEVLIB_ERROR_INVALID_HANDLE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, location,
		"GEVLIB_ERROR_INVALID_HANDLE in '%s'.", identifier );
		break;
	case GEVLIB_ERROR_NOT_SUPPORTED:
		return mx_error( MXE_UNSUPPORTED, location,
		"GEVLIB_ERROR_NOT_SUPPORTED in '%s'.", identifier );
		break;
	case GEVLIB_ERROR_TIME_OUT:
		return mx_error( MXE_TIMED_OUT, location,
		"GEVLIB_ERROR_TIME_OUT in '%s'.", identifier );
		break;
	case GEVLIB_ERROR_NOT_IMPLEMENTED:
		return mx_error( MXE_UNSUPPORTED, location,
		"GEVLIB_ERROR_NOT_IMPLEMENTED in '%s'.", identifier );
		break;
	case GEVLIB_ERROR_NO_CAMERA:
		return mx_error( MXE_DEVICE_ACTION_FAILED, location,
		"GEVLIB_ERROR_NO_CAMERA in '%s'.", identifier );
		break;
	case GEVLIB_ERROR_INVALID_PIXEL_FORMAT:
		return mx_error( MXE_ILLEGAL_ARGUMENT, location,
		"GEVLIB_ERROR_INVALID_PIXEL_FORMAT in '%s'.", identifier );
		break;
	case GEVLIB_ERROR_PARAMETER_INVALID:
		return mx_error( MXE_ILLEGAL_ARGUMENT, location,
		"GEVLIB_ERROR_PARAMETER_INVALID in '%s'.", identifier );
		break;
	case GEVLIB_ERROR_SOFTWARE:
		return mx_error( MXE_DEVICE_ACTION_FAILED, location,
		"GEVLIB_ERROR_SOFTWARE in '%s'.", identifier );
		break;
	case GEVLIB_ERROR_API_NOT_INITIALIZED:
		return mx_error( MXE_INITIALIZATION_ERROR, location,
		"GEVLIB_ERROR_API_NOT_INITIALIZED in '%s'.", identifier );
		break;
	case GEVLIB_ERROR_DEVICE_NOT_FOUND:
		return mx_error( MXE_NOT_FOUND, location,
		"GEVLIB_ERROR_DEVICE_NOT_FOUND in '%s'.", identifier );
		break;
	case GEVLIB_ERROR_ACCESS_DENIED:
		return mx_error( MXE_UNKNOWN_ERROR, location,
		"GEVLIB_ERROR_ACCESS_DENIED in '%s'.", identifier );
		break;

	case GEVLIB_ERROR_RESOURCE_NOT_ENABLED:
		return mx_error( MXE_NOT_AVAILABLE, location,
		"GEVLIB_ERROR_RESOURCE_NOT_ENABLED in '%s'.", identifier );
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, location,
		"A call to '%s' failed with Gev API error code %hd.",
			identifier, gev_status );
		break;
	}
}

static mx_status_type
mxd_dalsa_gev_camera_show_string_register( MX_VIDEO_INPUT *vinput,
					GEV_REGISTER *gev_register )
{
	char reg_char_value[MAX_GEVSTRING_LENGTH+1];
	mx_status_type mx_status;

	mx_status = mxd_dalsa_gev_camera_register_read( vinput,
							gev_register, 0,
							MAX_GEVSTRING_LENGTH,
							reg_char_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_info( "Camera '%s': %s = '%s'",
		vinput->record->name,
		gev_register->featureName,
		reg_char_value );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_dalsa_gev_camera_show_int_register( MX_VIDEO_INPUT *vinput,
					GEV_REGISTER *gev_register )
{
	UINT32 reg_int_value;
	mx_status_type mx_status;

	mx_status = mxd_dalsa_gev_camera_register_read( vinput,
							gev_register, 0,
							100 * sizeof(UINT32),
							&reg_int_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_info( "Camera '%s': %s = %lu",
		vinput->record->name,
		gev_register->featureName,
		(unsigned long) reg_int_value );

	return MX_SUCCESSFUL_RESULT;
}

#if 0
static mx_status_type
mxd_dalsa_gev_camera_show_float_register( MX_VIDEO_INPUT *vinput,
					GEV_REGISTER *gev_register )
{
	float reg_float_value;
	mx_status_type mx_status;

	mx_status = mxd_dalsa_gev_camera_register_read( vinput,
							gev_register, 0,
							sizeof(float),
							&reg_float_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_info( "Camera '%s': %s = %f",
		vinput->record->name,
		gev_register->featureName,
		reg_float_value );

	return MX_SUCCESSFUL_RESULT;
}
#endif

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

	mx_status = mx_video_input_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dalsa_gev_camera_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_dalsa_gev_camera_open()";

	MX_VIDEO_INPUT *vinput;
	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	MX_DALSA_GEV *dalsa_gev = NULL;

	GEV_CAMERA_INFO *camera_object, *selected_camera_object;
	GEV_CAMERA_HANDLE camera_handle;
	DALSA_GENICAM_GIGE_REGS *regs = NULL;

	UINT32 gev_width, gev_height, gev_x_offset, gev_y_offset, gev_format;

	char *serial_number_string;
	long i, gev_status;
	unsigned long camera_flags;
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

	camera_flags = dalsa_gev_camera->camera_flags;

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

#if MXD_DALSA_GEV_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,
	("%s: record '%s', selected camera '%s', camera_handle = %#lx",
		fname, record->name, selected_camera_object->serial,
		(unsigned long) dalsa_gev_camera->camera_handle));
#endif

	camera_handle = dalsa_gev_camera->camera_handle;

	/* Figure out the image format, bytes per pixel, etc. from the
	 * value set for 'image_format_name' in the MX database file.
	 */

	mx_status = mx_image_get_image_format_type_from_name(
				vinput->image_format_name,
				&(vinput->image_format) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( vinput->image_format ) {
	case MXT_IMAGE_FORMAT_GREY8:
		vinput->bytes_per_pixel = 1;
		vinput->bits_per_pixel  = 8;
		break;
	case MXT_IMAGE_FORMAT_GREY16:
		vinput->bytes_per_pixel = 2;
		vinput->bits_per_pixel  = 16;
		break;
	case MXT_IMAGE_FORMAT_GREY32:
		vinput->bytes_per_pixel = 4;
		vinput->bits_per_pixel  = 32;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image format %ld is not supported for camera '%s'.",
			vinput->image_format, record->name );
		break;
	}

#if MXD_DALSA_GEV_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s: camera '%s' image_format = %ld, "
	    "bytes_per_pixel = %.2f, bits_per_pixel = %ld", 
		fname, record->name, vinput->image_format,
		vinput->bytes_per_pixel, vinput->bits_per_pixel));
#endif

	/* Read the existing register values for the camera. */

	gev_status = GevGetCameraRegisters( camera_handle,
				&(dalsa_gev_camera->camera_registers),
				sizeof(dalsa_gev_camera->camera_registers) );

	switch( gev_status ) {
	case GEVLIB_OK:
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"A call to GevGetCameraRegisters() for camera '%s' "
		"returned an unexpected status code of %ld.",
			record->name, gev_status );
		break;
	}

	regs = &(dalsa_gev_camera->camera_registers);

	if ( camera_flags & MXF_DALSA_GEV_CAMERA_SHOW_INFO ) {
		(void) mxd_dalsa_gev_camera_show_string_register(
				vinput, &(regs->DeviceVendorName) );

		(void) mxd_dalsa_gev_camera_show_string_register(
				vinput, &(regs->DeviceModelName) );

		(void) mxd_dalsa_gev_camera_show_string_register(
				vinput, &(regs->DeviceVersion) );
	} 

#if 0
	UINT32 uint32_value;

	/* Set the images offsets to 0, since we are not using ROIs. */

	uint32_value = 0;

	mx_status = mxd_dalsa_gev_camera_register_write(
				vinput, &(regs->OffsetX), 0,
				sizeof(UINT32), &uint32_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	uint32_value = 0;

	mx_status = mxd_dalsa_gev_camera_register_write(
				vinput, &(regs->OffsetY), 0,
				sizeof(UINT32), &uint32_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the image dimensions. */

	uint32_value = vinput->framesize[0];

	mx_status = mxd_dalsa_gev_camera_register_write(
				vinput, &(regs->Width), 0,
				sizeof(UINT32), &uint32_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	uint32_value = vinput->framesize[1];

	mx_status = mxd_dalsa_gev_camera_register_write(
				vinput, &(regs->Height), 0,
				sizeof(UINT32), &uint32_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	gev_status = GevSetImageParameters( camera_handle,
				vinput->framesize[0], 0,
				vinput->framesize[1], 0,
				fmtMono14 );

	MX_DEBUG(-2,("GevSetImageParameters() = %d", (int) gev_status));

#if MXD_DALSA_GEV_CAMERA_DEBUG_OPEN
	/* Display some useful register values. */

	{

		MX_DEBUG(-2,("%s: Camera '%s' registers:",
			fname, record->name ));

		(void) mxd_dalsa_gev_camera_show_int_register(
				vinput, &(regs->Width) );

		(void) mxd_dalsa_gev_camera_show_int_register(
				vinput, &(regs->Height) );

	}
#endif
	gev_status = GevGetImageParameters( camera_handle,
	  &gev_width, &gev_height, &gev_x_offset, &gev_y_offset, &gev_format );

#if MXD_DALSA_GEV_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s: GevGetImageParameters() = %ld", fname, gev_status));
	MX_DEBUG(-2,("%s:    gev_width = %lu, gev_height = %lu",
	    fname, (unsigned long) gev_width, (unsigned long) gev_height));
	MX_DEBUG(-2,("%s:    gev_x_offset = %lu, gev_y_offset = %lu",
	    fname, (unsigned long) gev_x_offset, (unsigned long) gev_y_offset));
	MX_DEBUG(-2,("%s:    gev_format = %#lx",
	    fname, (unsigned long) gev_format));
#endif

	/* */

	if ( (vinput->framesize[0] < 0) || (vinput->framesize[1]) ) {

		/* We need to read the framesize from the camera. */
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dalsa_gev_camera_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_dalsa_gev_camera_close()";

	MX_VIDEO_INPUT *vinput = NULL;
	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	short gev_status;
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

#if MXD_DALSA_GEV_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	gev_status = GevCloseCamera( &(dalsa_gev_camera->camera_handle) );

	switch( gev_status ) {
	case GEVLIB_OK:
		break;
	case GEVLIB_ERROR_INVALID_HANDLE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"GEVLIB_ERROR_INVALID_HANDLE: The camera handle %#lx passed "
		"for camera '%s' is not valid.",
			(unsigned long) dalsa_gev_camera->camera_handle,
			record->name );
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"A call to GevCloseCamera() for camera record '%s' "
		"failed with status code %hd.",
			record->name, gev_status );
		break;
	}

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

#if MXD_DALSA_GEV_CAMERA_DEBUG_STOP
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

#if MXD_DALSA_GEV_CAMERA_DEBUG_STOP
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

#if MXD_DALSA_GEV_CAMERA_DEBUG_MX_PARAMETERS
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
mxd_dalsa_gev_camera_register_read( MX_VIDEO_INPUT *vinput,
					GEV_REGISTER *gev_register,
					int selector,
					size_t data_size,
					void *data_ptr )
{
	static const char fname[] = "mxd_dalsa_gev_camera_register_read()";

	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	short gev_status;
	mx_status_type mx_status;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VIDEO_INPUT pointer passed was NULL." );
	}
	if ( gev_register == (GEV_REGISTER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The GEV_REGISTER pointer passed was NULL." );
	}

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
					&dalsa_gev_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DALSA_GEV_CAMERA_DEBUG_REGISTER_READ
	MX_DEBUG(-2,("%s: GevRegisterRead( '%s', '%s' )",
		fname, vinput->record->name, gev_register->featureName ));
#endif

	gev_status = GevRegisterRead( dalsa_gev_camera->camera_handle,
					gev_register, selector,
					data_size, data_ptr );

	if ( gev_status == GEVLIB_OK ) {
		mx_status = MX_SUCCESSFUL_RESULT;
	} else {
		mx_status = mxd_dalsa_gev_camera_api_error( gev_status, fname,
			"GevRegisterRead( '%s', '%s' )",
			vinput->record->name,
			gev_register->featureName );
	}

	return mx_status;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_dalsa_gev_camera_register_write( MX_VIDEO_INPUT *vinput,
					GEV_REGISTER *gev_register,
					int selector,
					size_t data_size,
					void *data_ptr )
{
	static const char fname[] = "mxd_dalsa_gev_camera_register_write()";

	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	short gev_status;
	mx_status_type mx_status;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VIDEO_INPUT pointer passed was NULL." );
	}
	if ( gev_register == (GEV_REGISTER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The GEV_REGISTER pointer passed was NULL." );
	}

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
					&dalsa_gev_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DALSA_GEV_CAMERA_DEBUG_REGISTER_WRITE
	MX_DEBUG(-2,("%s: GevRegisterwrite( '%s', '%s' )",
		fname, vinput->record->name, gev_register->featureName ));
#endif

	gev_status = GevRegisterWrite( dalsa_gev_camera->camera_handle,
					gev_register, selector,
					data_size, data_ptr );

	if ( gev_status == GEVLIB_OK ) {
		mx_status = MX_SUCCESSFUL_RESULT;
	} else {
		mx_status = mxd_dalsa_gev_camera_api_error( gev_status, fname,
			"GevRegisterWrite( '%s', '%s' )",
			vinput->record->name,
			gev_register->featureName );
	}

	return mx_status;
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
#if 0
		case MXLV_DALSA_GEV_CAMERA_SHOW_FEATURES:
			record_field->process_function
				= mxd_dalsa_gev_camera_process_function;
			break;
#endif
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

#if 0

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

#endif
