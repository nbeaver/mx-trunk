/*
 * Name:    d_dalsa_gev_camera.cpp
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

#if 0
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
#endif

/*---*/

#if 0

static mx_status_type
mxd_dalsa_gev_camera_get_feature_value( MX_DALSA_GEV_CAMERA *dalsa_gev_camera,
					const char *feature_name,
					char *value_buffer,
					size_t value_buffer_length )
{
	static const char fname[] = "mxd_dalsa_gev_camera_get_feature_value()";

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

	if ( dalsa_gev_camera == (MX_DALSA_GEV_CAMERA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DALSA_GEV_CAMERA pointer passed was NULL." );
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

	acq_device = dalsa_gev_camera->acq_device;

	feature = dalsa_gev_camera->feature;

	sapera_status = acq_device->GetFeatureInfo( feature_name, feature );

	if ( sapera_status == 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"A call to GetFeatureInfo( '%s', ... ) for camera '%s' failed.",
			feature_name, dalsa_gev_camera->record->name );
	}

	sapera_status = feature->GetType( &feature_type );

	if ( sapera_status == 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"A call to GetType( '%s', ... ) for camera '%s' failed.",
			feature_name, dalsa_gev_camera->record->name );
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
			dalsa_gev_camera->record->name );
		break;
	}

	if ( sapera_status == 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	    "A call to GetFeatureValue( '%s', ... ) for camera '%s' failed.",
			feature_name, dalsa_gev_camera->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxd_dalsa_gev_camera_show_feature( SapAcqDevice *acq_device,
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

static const char *type_names[] = {
	"Value", "Base", "Integer", "Boolean", "Command", "Float", "String",
	"Register", "Category", "Enumeration", "EnumEntry", "Port"
};

static int num_type_names = 
		sizeof( type_names ) / sizeof(type_names[0]);

static void
dump_feature_hierarchy( const GenApi::CNodePtr &feature_ptr, int indent )
{
	int i = 0;

	for ( i = 0; i < indent; i++ ) {
		fprintf( stderr, "\t" );
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
#if 0
	mx_status_type mx_status;
#endif

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

	fprintf( stderr, "%s invoked for camera '%s'.\n", fname, record->name );

	/* Get the "Root" node for this camera. */

	GenApi::CNodeMapRef *feature_node_map =
static_cast<GenApi::CNodeMapRef*>( GevGetFeatureNodeMap(
					dalsa_gev_camera->camera_handle ) );

	GenApi::CNodePtr root_ptr = feature_node_map->_GetNode("Root");

	dump_feature_hierarchy( root_ptr, 1 );

	return MX_SUCCESSFUL_RESULT;
}

#endif

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
	long i;
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

	/*---------------------------------------------------------------*/

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

	/* mx_breakpoint(); */

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
					dalsa_gev_camera->camera_handle, TRUE );
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

#if 1
	GenApi::CNodeMapRef *feature_node_map = 
    static_cast<GenApi::CNodeMapRef*>( GevGetFeatureNodeMap(
					dalsa_gev_camera->camera_handle ) );

	MXW_UNUSED( feature_node_map );
#endif

	/* FIXME: And then access the features via
	 *        "feature_node_map->GetNode(...)"
	 */

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
#if 0
			mx_status =
			 mxd_dalsa_gev_camera_show_features( dalsa_gev_camera );
#endif
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

