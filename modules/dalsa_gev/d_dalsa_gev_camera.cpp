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
 * Copyright 2016-2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_DALSA_GEV_CAMERA_DEBUG_OPEN			TRUE
#define MXD_DALSA_GEV_CAMERA_DEBUG_ARM			TRUE
#define MXD_DALSA_GEV_CAMERA_DEBUG_STOP			TRUE
#define MXD_DALSA_GEV_CAMERA_DEBUG_GET_FRAME		TRUE
#define MXD_DALSA_GEV_CAMERA_DEBUG_MX_PARAMETERS	FALSE
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

/* Vendor include files */

#include "cordef.h"
#include "GenApi/GenApi.h"

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

/*---*/

static const char *type_names[] = {
	"Value", "Base", "Integer", "Boolean", "Command", "Float", "String",
	"Register", "Category", "Enumeration", "EnumEntry", "Port"
};

static int num_type_names = 
		sizeof( type_names ) / sizeof(type_names[0]);

static void
dump_feature_hierarchy( const GenApi::CNodePtr &feature_ptr, int indent )
{
#if 0
	static const char fname[] = "dump_feature_hierarchy()";
#endif

	int i = 0;

	for ( i = 0; i < indent; i++ ) {
		fputc( ' ', stderr );
	}

	GenApi::CCategoryPtr category_ptr( feature_ptr );

	if ( category_ptr.IsValid() ) {
		const char *category_name = static_cast<const char *>
					( feature_ptr->GetName() );

		fprintf( stderr, "Category: %s\n", category_name );

		GenApi::FeatureList_t features;
		category_ptr->GetFeatures( features );

		for( GenApi::FeatureList_t::iterator
				it_feature = features.begin() ;
		    it_feature != features.end();
		    it_feature++ )
		{
			dump_feature_hierarchy( (*it_feature), indent + 2 );
		}
	} else {
		const char *feature_name = static_cast<const char *>
			( feature_ptr->GetName() );

		/* Get the type of the feature as a number. */

		int index = static_cast<int>
			( feature_ptr->GetPrincipalInterfaceType() );

		if ( ( index >= 0 ) && ( index < num_type_names ) )
		{
			fprintf( stderr, "%s: %s\n",
				feature_name, type_names[index] );
		} else {
			fprintf( stderr, "%s: Unknown\n", feature_name );
		}
	}
}

/*---*/

static mx_status_type
mxd_dalsa_gev_camera_show_features( MX_DALSA_GEV_CAMERA *dalsa_gev_camera )
{
	static const char fname[] = "mxd_dalsa_gev_camera_show_features()";

	MX_RECORD *record;

	if ( dalsa_gev_camera == (MX_DALSA_GEV_CAMERA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DALSA_GEV_CAMERA pointer passed was NULL." );
	}

	record = dalsa_gev_camera->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_DALSA_GEV_CAMERA = %p is NULL.",
			dalsa_gev_camera );
	}

	/* Get the "Root" node for this camera. */

	GenApi::CNodeMapRef *feature_node_map =
		(GenApi::CNodeMapRef *) dalsa_gev_camera->feature_node_map;

	GenApi::CNodePtr root_ptr = feature_node_map->_GetNode("Root");

	dump_feature_hierarchy( root_ptr, 1 );

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

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

	BOOL read_xml_file  = FALSE;
	BOOL write_xml_file = FALSE;

	unsigned long flags;
	char *serial_number_string;
	unsigned long i;
	short gev_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

#if MXD_DALSA_GEV_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
				&dalsa_gev_camera, &dalsa_gev, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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

	dalsa_gev_camera->camera_object = selected_camera_object;

	/*---------------------------------------------------------------*/

	if ( strlen( dalsa_gev_camera->serial_number ) > 0 ) {
		gev_status = GevOpenCameraBySN( serial_number_string,
					GevExclusiveMode,
					&(dalsa_gev_camera->camera_handle) );
	} else {
		gev_status = GevOpenCamera( selected_camera_object,
					GevExclusiveMode,
					&(dalsa_gev_camera->camera_handle) );
	}

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
#if 0
	case GEV_STATUS_ACCESS_DENIED:
		return mx_error( MXE_PERMISSION_DENIED, fname,
		"GEV_STATUS_ACCESS_DENIED: For some reason we were "
		"not permitted access to the Gev API for camera record '%s'.",
			record->name );
		break;
#endif
	default:
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"A call to GevOpenCamera() for camera record '%s' "
		"failed with status code %hd.",
			record->name, gev_status );
		break;
	}

#if MXD_DALSA_GEV_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,
	("%s: record '%s', selected camera '%s', camera_handle = %#lx",
		fname, record->name, selected_camera_object->serial,
		(unsigned long) dalsa_gev_camera->camera_handle));
#endif

	/*---------------------------------------------------------------*/

	flags = dalsa_gev_camera->dalsa_gev_camera_flags;

	if ( flags & MXF_DALSA_GEV_CAMERA_WRITE_XML_FILE ) {
		write_xml_file = TRUE;
	} else {
		write_xml_file = FALSE;
	}

	if ( strlen( dalsa_gev_camera->xml_filename ) == 0 ) {
		read_xml_file  = FALSE;
	} else {
		read_xml_file  = TRUE;
	}

#if MXD_DALSA_GEV_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s: read_xml_file = %d, write_xml_file = %d",
		fname, (int) read_xml_file, (int) write_xml_file ));
#endif

	/*---------------------------------------------------------------*/

	/* Read in the XML data that describes the behavior of the camera. */

	if ( read_xml_file ) {

#if MXD_DALSA_GEV_CAMERA_DEBUG_OPEN
		MX_DEBUG(-2,("%s: reading features from file '%s'.",
			fname, dalsa_gev_camera->xml_filename ));
#endif

		gev_status = GevInitGenICamXMLFeatures_FromFile(
					dalsa_gev_camera->camera_handle,
					dalsa_gev_camera->xml_filename );
	} else {

#if MXD_DALSA_GEV_CAMERA_DEBUG_OPEN
		MX_DEBUG(-2,("%s: reading features from camera.", fname));
#endif

		gev_status = GevInitGenICamXMLFeatures(
					dalsa_gev_camera->camera_handle,
					write_xml_file );
	}

	switch( gev_status ) {
	case GEVLIB_OK:
		break;
	case GEVLIB_ERROR_GENERIC:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"GEVLIB_ERROR_GENERIC - Attempting to read in the XML "
		"data for camera '%s' failed.  gev_status = %hd",
			record->name, gev_status );
		break;
	case GEVLIB_ERROR_INVALID_HANDLE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"GEVLIB_ERROR_INVALID_HANDLE - An invalid handle %#lx was "
		"specified when attempting to read in the XML data for "
		"camera '%s'.  gev_status = %hd",
			(unsigned long) dalsa_gev_camera->camera_handle,
			record->name, gev_status );
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The attempt to read in the XML features "
		"for camera '%s' failed.  gev_status = %hd",
			record->name, gev_status );
		break;
	}

	/* Read in the feature node map. */ 

	GenApi::CNodeMapRef *feature_node_map = 
    static_cast<GenApi::CNodeMapRef*>( GevGetFeatureNodeMap(
					dalsa_gev_camera->camera_handle ) );

	dalsa_gev_camera->feature_node_map = feature_node_map;

	/* Optionally, show the available features. */

	if ( flags & MXF_DALSA_GEV_CAMERA_SHOW_FEATURES ) {
		mx_status = mxd_dalsa_gev_camera_show_features(
						dalsa_gev_camera );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Get camera paraameters from the camera. */

	GenApi::CIntegerPtr int_node = NULL;
	GenApi::CEnumerationPtr enumeration = NULL;
	uint32_t pixel_format_value;
	char pixel_format_string[80];
	int type, num_items;

	int_node = feature_node_map->_GetNode("Width");

	vinput->framesize[0] = int_node->GetValue();

	int_node = feature_node_map->_GetNode("Height");

	vinput->framesize[1] = int_node->GetValue();

	vinput->maximum_framesize[0] = vinput->framesize[0];
	vinput->maximum_framesize[1] = vinput->framesize[1];

	enumeration = feature_node_map->_GetNode("PixelFormat");

	gev_status = GevGetFeatureValueAsString(
			dalsa_gev_camera->camera_handle, "PixelFormat",
			&type, sizeof(pixel_format_string),
			pixel_format_string );

	if ( gev_status != GEVLIB_OK ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to get the pixel format string for "
			"camera '%s' failed with error code %d.",
			record->name, gev_status );
	}

	if ( strncmp( pixel_format_string, "Mono", 4 ) == 0 ) {
		num_items = sscanf( pixel_format_string,
				"Mono%ld", &(vinput->bits_per_pixel) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to find the bits per pixel in the returned "
			"pixel format string '%s' for camera '%s'.",
			pixel_format_string, record->name );
		}
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Pixel format '%s' is not supported for camera '%s'.",
		pixel_format_string, record->name );
	}

	if ( vinput->bits_per_pixel <= 8 ) {
		vinput->image_format = MXT_IMAGE_FORMAT_GREY8;
		vinput->bytes_per_pixel = 1.0;
	} else
	if ( vinput->bits_per_pixel <= 16 ) {
		vinput->image_format = MXT_IMAGE_FORMAT_GREY16;
		vinput->bytes_per_pixel = 2.0;
	} else
	if ( vinput->bits_per_pixel <= 32 ) {
		vinput->image_format = MXT_IMAGE_FORMAT_GREY32;
		vinput->bytes_per_pixel = 4.0;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The pixel format '%s' is not supported for camera '%s'.  "
		"The largest pixel bit depth supported is 32 bits.",
			pixel_format_string, record->name );
	}

	if ( vinput->image_format != MXT_IMAGE_FORMAT_GREY16 ) {
		char image_format_name[80];

		mx_image_get_image_format_name_from_type( vinput->image_format,
				image_format_name, sizeof(image_format_name) );

		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Image format '%s' is not yet implemented for camera '%s'.",
		image_format_name, record->name );
	}

	pixel_format_value = enumeration->GetIntValue();

	vinput->bytes_per_pixel = GetPixelSizeInBytes( pixel_format_value );

	/* Allocate memory for the image buffers. */

	dalsa_gev_camera->frame_buffer_size = 
		mx_round( vinput->bytes_per_pixel
			* vinput->framesize[0]
			* vinput->framesize[1] );
	
	dalsa_gev_camera->frame_buffer_array = (unsigned char **)
		calloc( dalsa_gev_camera->num_frame_buffers,
			sizeof(unsigned char *) );

	if ( dalsa_gev_camera->frame_buffer_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an %lu element array "
		"of frame buffer pointers for camera '%s'.",
			dalsa_gev_camera->num_frame_buffers, record->name );
	}

	for ( i = 0; i < dalsa_gev_camera->num_frame_buffers; i++ ) {

		dalsa_gev_camera->frame_buffer_array[i] = (unsigned char *)
			calloc( dalsa_gev_camera->frame_buffer_size,
					sizeof(char) );

		if ( dalsa_gev_camera->frame_buffer_array[i] == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate frame buffer %lu "
			"for camera '%s'.", i, record->name );
		}
	}


	MX_DEBUG(-2,
	("%s: framesize = (%lu,%lu), pixel_format_string = '%s' (%#x)",
			fname, vinput->framesize[0], vinput->framesize[1],
			pixel_format_string,
			(unsigned int) pixel_format_value));
	MX_DEBUG(-2,("%s: bytes_per_pixel = %f",
			fname, vinput->bytes_per_pixel));

	/*---------------------------------------------------------------*/

#if 0
	GevBufferCyclingMode cycling_mode = SynchronousNextEmpty;
#else
	GevBufferCyclingMode cycling_mode = Asynchronous;
#endif

#if 0
	{
	    if ( dalsa_gev_camera->frame_buffer_array == NULL ) {
		MX_DEBUG(-2,("%s: frame_buffer_array = NULL", fname));
	    } else {
		MX_DEBUG(-2,("%s: frame_buffer_array = %p",
			fname, dalsa_gev_camera->frame_buffer_array));

		MX_DEBUG(-2,("%s: num_frame_buffers = %lu",
			fname, dalsa_gev_camera->num_frame_buffers));

		for ( i = 0; i < dalsa_gev_camera->num_frame_buffers; i++ ) {

		    MX_DEBUG(-2,("%s: frame_buffer_array[%lu] = %p",
			fname, i, dalsa_gev_camera->frame_buffer_array[i] ));
		}
	    }
	}
#endif

	gev_status = GevInitImageTransfer( dalsa_gev_camera->camera_handle,
					cycling_mode,
					dalsa_gev_camera->num_frame_buffers,
					dalsa_gev_camera->frame_buffer_array );

	if ( gev_status != GEVLIB_OK ) {
		return mxd_dalsa_gev_camera_api_error( gev_status, fname,
						"GevInitImageTransfer()");
	}

	/*---------------------------------------------------------------*/

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

	case MXLV_VIN_BYTES_PER_PIXEL:
	case MXLV_VIN_BITS_PER_PIXEL:
	case MXLV_VIN_BYTES_PER_FRAME:
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
		case MXLV_DALSA_GEV_CAMERA_FEATURE_NAME:
		case MXLV_DALSA_GEV_CAMERA_FEATURE_VALUE:
		case MXLV_DALSA_GEV_CAMERA_GAIN:
		case MXLV_DALSA_GEV_CAMERA_SHOW_FEATURES:
		case MXLV_DALSA_GEV_CAMERA_TEMPERATURE:
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
	short gev_status;
	int feature_type;

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
		case MXLV_DALSA_GEV_CAMERA_FEATURE_VALUE:
			gev_status = GevGetFeatureValueAsString(
					dalsa_gev_camera->camera_handle,
					dalsa_gev_camera->feature_name,
					&feature_type,
					sizeof(dalsa_gev_camera->feature_value),
					dalsa_gev_camera->feature_value );

			if ( gev_status != GEVLIB_OK ) {
				mx_status =
					mxd_dalsa_gev_camera_api_error(
						gev_status, fname,
						"GevGetFeatureValueAsString()");
			}
			break;
		default:
			break;
		}
		break;

	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_DALSA_GEV_CAMERA_SHOW_FEATURES:
			mx_status =
			 mxd_dalsa_gev_camera_show_features( dalsa_gev_camera );

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

