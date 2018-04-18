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
 * Copyright 2016-2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_DALSA_GEV_CAMERA_DEBUG_OPEN				FALSE
#define MXD_DALSA_GEV_CAMERA_DEBUG_ARM				FALSE
#define MXD_DALSA_GEV_CAMERA_DEBUG_STOP				FALSE
#define MXD_DALSA_GEV_CAMERA_DEBUG_GET_FRAME			FALSE
#define MXD_DALSA_GEV_CAMERA_DEBUG_MX_PARAMETERS		FALSE
#define MXD_DALSA_GEV_CAMERA_DEBUG_REGISTER_READ		FALSE
#define MXD_DALSA_GEV_CAMERA_DEBUG_REGISTER_WRITE		FALSE

#define MXD_DALSA_GEV_CAMERA_USE_GEV_INITIALIZE_TRANSFER	FALSE

#define MXD_DALSA_GEV_CAMERA_USE_CUSTOM_GEV_GET_NEXT_IMAGE	TRUE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_bit.h"
#include "mx_process.h"
#include "mx_thread.h"
#include "mx_array.h"
#include "mx_atomic.h"
#include "mx_vm_alloc.h"
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
dump_feature_hierarchy( MX_DALSA_GEV_CAMERA *dalsa_gev_camera,
				const GenApi::CNodePtr &feature_ptr,
				int indent, mx_bool_type show_value )
{
#if 0
	static const char fname[] = "dump_feature_hierarchy()";
#endif

	char saved_feature_name[MXU_DALSA_GEV_CAMERA_FEATURE_NAME_LENGTH+1];
	char feature_value[MXU_DALSA_GEV_CAMERA_FEATURE_VALUE_LENGTH+1];
	int feature_type;
	short gev_status;
	int i;

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
			dump_feature_hierarchy( dalsa_gev_camera,
				(*it_feature), indent + 2, show_value );
		}
	} else {
		const char *feature_name = static_cast<const char *>
			( feature_ptr->GetName() );

		/* Get the type of the feature as a number. */

		int index = static_cast<int>
			( feature_ptr->GetPrincipalInterfaceType() );

		if ( ( index >= 0 ) && ( index < num_type_names ) )
		{
			if ( show_value == FALSE ) {
				fprintf( stderr, "%s: %s\n",
					feature_name, type_names[index] );
			} else {
				strlcpy( saved_feature_name,
					feature_name,
					sizeof(saved_feature_name) );

				gev_status = GevGetFeatureValueAsString(
					dalsa_gev_camera->camera_handle,
					feature_name,
					&feature_type,
					sizeof(feature_value),
					feature_value );

				if ( gev_status != GEVLIB_OK ) {
					fprintf( stderr,
						"%s: %s --> Error %hd\n",
						saved_feature_name,
						type_names[index],
						gev_status );
				} else {
					fprintf( stderr,
						"%s: %s --> '%s'\n",
						saved_feature_name,
						type_names[index],
						feature_value );
				}
			}
		} else {
			fprintf( stderr, "%s: Unknown\n", feature_name );
		}
	}
}

/*---*/

static mx_status_type
mxd_dalsa_gev_camera_show_feature_list( MX_DALSA_GEV_CAMERA *dalsa_gev_camera,
						mx_bool_type show_value )
					
{
	static const char fname[] = "mxd_dalsa_gev_camera_show_feature_list()";

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

	dump_feature_hierarchy( dalsa_gev_camera, root_ptr, 1, show_value );

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxd_dalsa_gev_camera_show_feature_range( MX_DALSA_GEV_CAMERA *dalsa_gev_camera )
{
	static const char fname[] = "mxd_dalsa_gev_camera_show_feature_range()";

	GenApi::CNodeMapRef *feature_node_map =
		   (GenApi::CNodeMapRef *) dalsa_gev_camera->feature_node_map;

	GenApi::CNodePtr feature_ptr =
		   feature_node_map->_GetNode( dalsa_gev_camera->feature_name );

	int interface_type = static_cast<int>
			( feature_ptr->GetPrincipalInterfaceType() );

	switch( interface_type ) {
	case GenApi::intfIInteger:
	    {
		GenApi::CIntegerPtr int_feature_ptr =
		  feature_node_map->_GetNode( dalsa_gev_camera->feature_name );

		snprintf( dalsa_gev_camera->feature_range,
			sizeof(dalsa_gev_camera->feature_range),
		    "Min = %" PRId64 ", Max = %" PRId64 ", Step = %" PRId64,
				int_feature_ptr->GetMin(),
				int_feature_ptr->GetMax(),
				int_feature_ptr->GetInc() );
	    }
	    break;
	case GenApi::intfIFloat:
	    {
		GenApi::CFloatPtr float_feature_ptr =
		  feature_node_map->_GetNode( dalsa_gev_camera->feature_name );

		if ( float_feature_ptr->HasInc() ) {
			snprintf( dalsa_gev_camera->feature_range,
				sizeof(dalsa_gev_camera->feature_range),
				"Min = %g, Max = %g, Step = %g",
				float_feature_ptr->GetMin(),
				float_feature_ptr->GetMax(),
				float_feature_ptr->GetInc() );
		} else {
			snprintf( dalsa_gev_camera->feature_range,
				sizeof(dalsa_gev_camera->feature_range),
				"Min = %g, Max = %g",
				float_feature_ptr->GetMin(),
				float_feature_ptr->GetMax() );
		}
	    }
	    break;
	case GenApi::intfIBoolean:
	    {
		strlcpy( dalsa_gev_camera->feature_range,
			"Min = 0, Max = 1",
			sizeof(dalsa_gev_camera->feature_range) );
	    }
	    break;
	case GenApi::intfIString:
	    {
		GenApi::CStringPtr string_feature_ptr =
		  feature_node_map->_GetNode( dalsa_gev_camera->feature_name );

		snprintf( dalsa_gev_camera->feature_range,
			sizeof(dalsa_gev_camera->feature_range),
			"Max Length = %" PRId64,
			string_feature_ptr->GetMaxLength() );
	    }
	    break;
	case GenApi::intfIEnumeration:
	    {
		char enum_temp[80];

		GenApi::CEnumerationPtr enum_feature_ptr =
		  feature_node_map->_GetNode( dalsa_gev_camera->feature_name );

		GenApi::NodeList_t entries;

		enum_feature_ptr->GetEntries( entries );

		int num_entries = entries.size();

		dalsa_gev_camera->feature_range[0] = '\0';

		for ( int i = 0; i < num_entries; i++ ) {

			GenApi::CEnumEntryPtr enum_entry_ptr = entries[i];

			const char *enum_string =
				enum_entry_ptr->GetSymbolic().c_str();

			if ( i == 0 ) {
				snprintf( dalsa_gev_camera->feature_range,
					sizeof(dalsa_gev_camera->feature_range),
					"'%s'", enum_string );
			} else {
				snprintf( enum_temp, sizeof(enum_temp),
						", '%s'", enum_string );

				strlcat( dalsa_gev_camera->feature_range,
				    enum_temp,
				    sizeof(dalsa_gev_camera->feature_range) );
			}
		}
	    }
	    break;
	default:
	    return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Camera '%s' feature '%s' has illegal interface type %d",
			dalsa_gev_camera->record->name,
			dalsa_gev_camera->feature_name,
			interface_type );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

#if MXD_DALSA_GEV_CAMERA_USE_CUSTOM_GEV_GET_NEXT_IMAGE

extern "C" {
	GEV_BUFFER_LIST *GevGetBufferListFromHandle( GEV_CAMERA_HANDLE handle );
}

static GEV_STATUS
mx_gev_get_next_image( GEV_CAMERA_HANDLE handle,
			GEV_BUFFER_OBJECT **image_object_ptr,
			struct timeval *pTimeout )
{
	static const char fname[] = "mx_gev_get_next_image()";

	GEV_STATUS gev_status = GEVLIB_ERROR_INVALID_HANDLE;

	static mx_bool_type valid_address_range = FALSE;

	unsigned long protection_flags;

	if ( handle != NULL ) {
	    gev_status = GEVLIB_ERROR_NULL_PTR;

	    if ( image_object_ptr != NULL ) {

		GEV_BUFFER_LIST *pBufList = GevGetBufferListFromHandle(handle);

		if ( pBufList != NULL ) {

		    if ( valid_address_range == FALSE ) {

			protection_flags = ( R_OK | W_OK );

			(void) mx_vm_get_protection(
						pBufList->pFullBuffers, 1,
						&valid_address_range,
						&protection_flags );

			MX_DEBUG(-2,
			  ("%s: pBufList->pFullBuffers = %p, valid = %d",
			  fname, pBufList->pFullBuffers, valid_address_range));
		    }

		    if ( valid_address_range ) {

			*image_object_ptr = (GEV_BUFFER_OBJECT *)
			    DQueuePendEx( pBufList->pFullBuffers, pTimeout );

			if ( (*image_object_ptr) == NULL ) {
			    gev_status = GEVLIB_ERROR_TIME_OUT;
			} else {
			    gev_status = GEVLIB_OK;
			}
		    } else {
			gev_status = GEVLIB_ERROR_TIME_OUT;
		    }
		}
	    }
	}

	return gev_status;
}

#endif /* MXD_DALSA_GEV_CAMERA_USE_CUSTOM_GEV_GET_NEXT_IMAGE */

/*---*/

static mx_status_type
mxd_dalsa_gev_camera_image_wait_thread_fn( MX_THREAD *thread, void *args )
{
	static const char fname[] =
		"mxd_dalsa_gev_camera_image_wait_thread_fn()";

	MX_RECORD *record = NULL;
	MX_VIDEO_INPUT *vinput = NULL;
	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	long mx_total_num_frames;
#if 0
	unsigned long sleep_ms;
#else
	struct timeval wait_timeval;
#endif
	mx_bool_type image_available;
	mx_status_type mx_status;

	short gev_status;
	GEV_BUFFER_OBJECT *gev_buffer_object = NULL;

	if ( thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}
	if ( args == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	record = (MX_RECORD *) args;

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));

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

	MX_DEBUG(-2,("%s: record = %p", fname, record));
	MX_DEBUG(-2,("%s: dalsa_gev_camera = %p", fname, dalsa_gev_camera));
	MX_DEBUG(-2,("%s: dalsa_gev_camera->camera_handle = %p",
			fname, dalsa_gev_camera->camera_handle));

	while (TRUE) {
		image_available = FALSE;

		wait_timeval.tv_sec = 1;
		wait_timeval.tv_usec = 0;

#if MXD_DALSA_GEV_CAMERA_USE_CUSTOM_GEV_GET_NEXT_IMAGE
		gev_status = mx_gev_get_next_image(
				dalsa_gev_camera->camera_handle,
				&gev_buffer_object, &wait_timeval );
#else
		gev_status = GevGetNextImage(
				dalsa_gev_camera->camera_handle,
				&gev_buffer_object, &wait_timeval );
#endif

#if 0
		MX_DEBUG(-2,("%s: wait thread gev_status = %d",
			fname, gev_status));
#endif

		switch( gev_status ) {
		case GEVLIB_OK:
			image_available = TRUE;
			break;
		case GEVLIB_ERROR_TIME_OUT:
		case GEVLIB_ERROR_NULL_PTR:
			image_available = FALSE;
			break;
		case GEVLIB_ERROR_INVALID_HANDLE:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The handle for Dalsa Gev camera '%s' is invalid.",
				record->name );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized Gevlib error code %d returned "
			"for Dalsa Gev camera '%s'.",
				gev_status, record->name );
			break;
		}

		if ( image_available ) {
		    if ( gev_buffer_object == (GEV_BUFFER_OBJECT *)NULL ) {
			mx_warning("GEV_BUFFER_OBJECT returned was NULL.");
		    } else {
			switch( gev_buffer_object->status ) {
			case GEV_FRAME_STATUS_RECVD:
				mx_total_num_frames = mx_atomic_increment32(
					&(dalsa_gev_camera->total_num_frames) );

				(void) mx_atomic_decrement32(
					&(dalsa_gev_camera->num_frames_left) );
#if 1
				MX_DEBUG(-2,("CAPTURE: Total num frames = %lu",
					(unsigned long) mx_total_num_frames ));
#endif
				break;
			default:
				mx_warning(
				"Gev buffer frame status for '%s' = %#lx",
					dalsa_gev_camera->record->name,
				    (unsigned long) gev_buffer_object->status );
				break;
			}
		    }
		}

		mx_status = mx_thread_check_for_stop_request( thread );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxd_dalsa_gev_camera_image_dead_reckoning_wait_thread_fn( MX_THREAD *thread,
								void *args )
{
	static const char fname[] =
		"mxd_dalsa_gev_camera_image_dead_reckoning_wait_thread_fn()";

	MX_RECORD *record = NULL;
	MX_VIDEO_INPUT *vinput = NULL;
	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	int32_t num_frames_left, milliseconds_per_frame;
	long mx_total_num_frames;
	MX_CLOCK_TICK clock_ticks_per_frame;
	MX_CLOCK_TICK start_tick, current_tick, finish_tick;
	int comparison;

	if ( thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}
	if ( args == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	record = (MX_RECORD *) args;

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));

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

	MX_DEBUG(-2,("%s: record = %p", fname, record));
	MX_DEBUG(-2,("%s: dalsa_gev_camera = %p", fname, dalsa_gev_camera));
	MX_DEBUG(-2,("%s: dalsa_gev_camera->camera_handle = %p",
			fname, dalsa_gev_camera->camera_handle));

	/* WARNING: We call this 'dead reckoning' because the following
	 * is only an estimate of when the new frame will arrive.  If
	 * for some reason the camera is not sending us actual frames,
	 * then this logic will not take that into account.
	 */

	while (TRUE) {
		num_frames_left =
		    mx_atomic_read32( &(dalsa_gev_camera->num_frames_left) );

		if ( num_frames_left <= 0 ) {
			mx_msleep( 100 );
		} else {
			MX_DEBUG(-2,("%s: starting new sequence for %ld frames",
				fname, (long) num_frames_left ));

			milliseconds_per_frame = mx_atomic_read32(
				&(dalsa_gev_camera->milliseconds_per_frame) );

			clock_ticks_per_frame = 
				mx_convert_seconds_to_clock_ticks(
					0.001 * milliseconds_per_frame );

			start_tick = mx_current_clock_tick();

			finish_tick = mx_add_clock_ticks( start_tick,
						clock_ticks_per_frame );

			do {
				do {
					mx_msleep( 100 );

					current_tick = mx_current_clock_tick();

					comparison = mx_compare_clock_ticks(
						current_tick, finish_tick );
				} while( comparison < 0 );

				mx_total_num_frames = mx_atomic_increment32(
					&(dalsa_gev_camera->total_num_frames) );

				num_frames_left = mx_atomic_decrement32(
					&(dalsa_gev_camera->num_frames_left) );
#if 1
				/* Note the following statement may be a lie. */

				MX_DEBUG(-2,("CAPTURE?: Total num frames = %lu",
					(unsigned long) mx_total_num_frames ));
#endif
			} while( num_frames_left > 0 );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxd_dalsa_gev_camera_shadobox_trigger( MX_VIDEO_INPUT *vinput,
					MX_DALSA_GEV_CAMERA *dalsa_gev_camera )
{
	static const char fname[] = "mxd_dalsa_gev_camera_shadobox_trigger()";

	MX_SEQUENCE_PARAMETERS *sp = NULL;
	char synchronization_mode[80];
	long gev_status;

	sp = &(vinput->sequence_parameters);

	if ( vinput->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) {
		MX_DEBUG(-2,("%s: camera '%s' is using internal trigger.",
			fname, vinput->record->name ));

		strlcpy( synchronization_mode, "FreeRunning",
				sizeof(synchronization_mode) );
	} else
	if ( vinput->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER ) {
		MX_DEBUG(-2,("%s: camera '%s' is using external trigger.",
			fname, vinput->record->name ));

		switch( sp->sequence_type ) {
		case MXT_SQ_ONE_SHOT:
		case MXT_SQ_STREAM:
		case MXT_SQ_MULTIFRAME:
			strlcpy( synchronization_mode, "ExtTrigger",
					sizeof(synchronization_mode) );
			break;
		case MXT_SQ_DURATION:
			strlcpy( synchronization_mode, "Snapshot",
					sizeof(synchronization_mode) );
			break;
		}
	}

	gev_status = GevSetFeatureValueAsString(
			dalsa_gev_camera->camera_handle,
			"SynchronizationMode", synchronization_mode );

	if ( gev_status != GEVLIB_OK ) {
		return mxd_dalsa_gev_camera_api_error( gev_status, fname,
					"Set SynchronizationMode value" );
	}

	/* If the camera is configured for streaming, then we must
	 * set the exposure time for that mode.
	 */

	if ( sp->sequence_type == MXT_SQ_STREAM ) {
		UINT32 extended_exposure_microseconds =
			mx_round( 1.0e6 * sp->parameter_array[0] );

		gev_status = GevSetFeatureValue(
				dalsa_gev_camera->camera_handle,
				"ExtendedExposure",
				sizeof(UINT32), 
				&extended_exposure_microseconds );

		if ( gev_status != GEVLIB_OK ) {
			return mxd_dalsa_gev_camera_api_error(
					gev_status, fname,
					"Set SynchronizationMode value" );
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
	long frame_buffer_datatype;
	long dimension_array[2];
	size_t element_size_array[2];

	GEV_CAMERA_INFO *camera_object, *selected_camera_object;

	BOOL read_xml_file  = FALSE;
	BOOL write_xml_file = FALSE;

	unsigned long flags;
	char *serial_number_string;
	unsigned long i;
	short gev_status;
	char *ptr;
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
#if 1
		else
		if (strcmp( serial_number_string + 4, camera_object->serial)==0)
		{
			dalsa_gev_camera->camera_index = i;

			dalsa_gev_camera->camera_object = camera_object;

			selected_camera_object = camera_object;
		}
#endif
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
		MX_DEBUG(-2,
		("%s: calling GevOpenCameraBySN() for camera '%s'",
		 	fname, serial_number_string ));

		gev_status = GevOpenCameraBySN( serial_number_string,
					GevExclusiveMode,
					&(dalsa_gev_camera->camera_handle) );
	} else {
		MX_DEBUG(-2,("%s: calling GevOpenCamera().", fname));

		gev_status = GevOpenCamera( selected_camera_object,
					GevExclusiveMode,
					&(dalsa_gev_camera->camera_handle) );
	}

	switch ( gev_status ) {
	case GEVLIB_OK:
		break;
	case GEVLIB_ERROR_GENERIC:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"GEVLIB_ERROR_GENERIC: An unknown error occurred when "
		"trying to open a connection to camera '%s'.",
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
	case GEVLIB_ERROR_API_NOT_INITIALIZED:
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"GEVLIB_ERROR_API_NOT_INITIALIZED: The Gev API library "
		"has not yet been initialized for camera record '%s'.",
			record->name );
		break;
	case GEVLIB_ERROR_DEVICE_NOT_FOUND:
		return mx_error( MXE_NOT_FOUND, fname,
		"GEVLIB_ERROR_DEVICE_NOT_FOUND: No DALSA GigE-Vision camera "
		"was found with serial number '%s' for camera record '%s'.",
			dalsa_gev_camera->serial_number,
			record->name );
		break;
	case GEVLIB_ERROR_ACCESS_DENIED:
		return mx_error( MXE_PERMISSION_DENIED, fname,
		"GEVLIB_ERROR_ACCESS_DENIED: For some reason we were "
		"not permitted access to the Gev API for camera record '%s'.",
			record->name );
		break;
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
		mx_status = mxd_dalsa_gev_camera_show_feature_list(
						dalsa_gev_camera, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	if ( flags & MXF_DALSA_GEV_CAMERA_SHOW_FEATURE_VALUES ) {
		mx_status = mxd_dalsa_gev_camera_show_feature_list(
						dalsa_gev_camera, TRUE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Identify the camera model. */

	GenApi::CStringPtr device_vendor_ptr =
		feature_node_map->_GetNode( "DeviceVendorName" );

	strlcpy( dalsa_gev_camera->device_vendor_name,
		device_vendor_ptr->GetValue().c_str(),
		sizeof(dalsa_gev_camera->device_vendor_name) );

	GenApi::CStringPtr device_model_ptr =
		feature_node_map->_GetNode( "DeviceModelName" );

	strlcpy( dalsa_gev_camera->device_model_name,
		device_model_ptr->GetValue().c_str(),
		sizeof(dalsa_gev_camera->device_model_name) );

	/*---*/

	dalsa_gev_camera->model_type = 0;

	ptr = strstr( dalsa_gev_camera->device_vendor_name, "Rad-icon" );

	if ( ptr != NULL ) {
		if ( strncmp( "ShadoBox",
			dalsa_gev_camera->device_model_name, 8 ) == 0 )
		{
			dalsa_gev_camera->model_type =
				MXT_DALSA_GEV_CAMERA_SHADOBOX;
		}
	}

	/* Reading the framesize gets a variety of other parameters as well. */

	vinput->parameter_type = MXLV_VIN_FRAMESIZE;

	mx_status = mxd_dalsa_gev_camera_get_parameter( vinput );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vinput->maximum_framesize[0] = vinput->framesize[0];
	vinput->maximum_framesize[1] = vinput->framesize[1];

	/* Allocate memory for the image buffers. */

	switch( vinput->image_format ) {
	case MXT_IMAGE_FORMAT_GREY8:
		frame_buffer_datatype = MXFT_UCHAR;
		break;
	case MXT_IMAGE_FORMAT_GREY16:
		frame_buffer_datatype = MXFT_USHORT;
		break;
	case MXT_IMAGE_FORMAT_GREY32:
		frame_buffer_datatype = MXFT_ULONG;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image format '%s' is not supported for camera '%s'.",
			vinput->image_format_name, record->name );
		break;
	}

	dimension_array[0] = dalsa_gev_camera->num_frame_buffers;
	dimension_array[1] = vinput->framesize[0] * vinput->framesize[1];

	element_size_array[0] = mx_round( vinput->bytes_per_pixel );
	element_size_array[1] = sizeof( void * );
	
	dalsa_gev_camera->frame_buffer_array = (unsigned char **)
		mx_allocate_array( frame_buffer_datatype,
				2, dimension_array, element_size_array );

	if ( dalsa_gev_camera->frame_buffer_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element array "
		"of frame buffers for camera '%s'.",
			dalsa_gev_camera->num_frame_buffers,
			record->name );
	}

	dalsa_gev_camera->frame_buffer_array_size_in_bytes
		= vinput->framesize[0] * vinput->framesize[1]
			* mx_round( vinput->bytes_per_pixel );

	/*---------------------------------------------------------------*/

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
	/* If an area detector driver wants us to skip frames, then it
	 * must set our 'num_frames_to_skip' variable to a non-zero
	 * value in the area detector's open() routine.
	 */

	dalsa_gev_camera->num_frames_to_skip = 0;

	/* Initialize the frame counters. */

	mx_atomic_write32( &(dalsa_gev_camera->total_num_frames_at_start), 0 );
	mx_atomic_write32( &(dalsa_gev_camera->total_num_frames), 0 );
	mx_atomic_write32( &(dalsa_gev_camera->num_frames_left), 0 );
	mx_atomic_write32( &(dalsa_gev_camera->milliseconds_per_frame), 0 );

#if 1
	MX_DEBUG(-2,("%s: record = %p", fname, record));
	MX_DEBUG(-2,("%s: dalsa_gev_camera = %p", fname, dalsa_gev_camera));
	MX_DEBUG(-2,("%s: dalsa_gev_camera->camera_handle = %p",
			fname, dalsa_gev_camera->camera_handle));
#endif

	flags = dalsa_gev_camera->dalsa_gev_camera_flags;

	/* Create a thread to manage the reading of images from Dalsa Gev. */

	if ( flags & MXF_DALSA_GEV_CAMERA_USE_DEAD_RECKONING ) {

		mx_status = mx_thread_create(
				&(dalsa_gev_camera->image_wait_thread),
		    mxd_dalsa_gev_camera_image_dead_reckoning_wait_thread_fn,
				record );
	} else {
		mx_status = mx_thread_create(
				&(dalsa_gev_camera->image_wait_thread),
				mxd_dalsa_gev_camera_image_wait_thread_fn,
				record );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: MARKER X: camera_handle = %p",
		fname, dalsa_gev_camera->camera_handle));

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
	MX_SEQUENCE_PARAMETERS *sp = NULL;
	void *vector_pointer = NULL;
	unsigned char **frame_buffer_array = NULL;
	unsigned long num_frames, num_bytes_in_array;
	double exposure_time, frame_time;
	int32_t total_num_frames_at_start, milliseconds_per_frame;
	long gev_status;
	mx_status_type mx_status;

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
					&dalsa_gev_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DALSA_GEV_CAMERA_DEBUG_ARM
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif
	/* If an imaging sequence was running, then stop it. */

	gev_status = GevAbortImageTransfer( dalsa_gev_camera->camera_handle );

	if ( gev_status != GEVLIB_OK ) {
		return mxd_dalsa_gev_camera_api_error( gev_status, fname,
						"GevAbortImageTransfer()");
	}

	/* Clear out the image frame buffers. */

	frame_buffer_array = dalsa_gev_camera->frame_buffer_array;

#if 0
	mx_show_array_info( frame_buffer_array );
#endif

	mx_status = mx_array_get_num_bytes( frame_buffer_array,
						&num_bytes_in_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vector_pointer = mx_array_get_vector( frame_buffer_array );

	memset( vector_pointer, 0, num_bytes_in_array );

	/* Make sure that the MX image frame buffer is the correct size. */

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
	 * for the upcoming imaging sequence.
	 */

	sp = &(vinput->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_STREAM:
		num_frames = 1;
		exposure_time = sp->parameter_array[0];
		frame_time = exposure_time;
		break;
	case MXT_SQ_MULTIFRAME:
		num_frames = sp->parameter_array[0];
		exposure_time = sp->parameter_array[1];
		frame_time = sp->parameter_array[2];
		break;
	case MXT_SQ_DURATION:
		num_frames = sp->parameter_array[0];
		exposure_time = 1;
		frame_time = exposure_time;
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Sequence type %ld has not yet been implemented for '%s'.",
			sp->sequence_type, vinput->record->name );
		break;
	}

	/* Configure the GenICam AcquisitionMode feature for this sequence. */

	char acquisition_mode[40];

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		strlcpy( acquisition_mode, "SingleFrame",
				sizeof(acquisition_mode) );
		break;
	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_DURATION:
		strlcpy( acquisition_mode, "MultiFrame",
				sizeof(acquisition_mode) );
		break;
	case MXT_SQ_STREAM:
		strlcpy( acquisition_mode, "Continuous",
				sizeof(acquisition_mode) );
		break;
	}

	gev_status = GevSetFeatureValueAsString(
			dalsa_gev_camera->camera_handle,
			"AcquisitionMode", acquisition_mode );

	if ( gev_status != GEVLIB_OK ) {
		return mxd_dalsa_gev_camera_api_error( gev_status, fname,
				"Set AcquisitionMode value" );
	}

	/* Configure external/internal triggering for the detector.
	 * Regrettably, this is different for different models of 
	 * detectors.
	 */

	switch( dalsa_gev_camera->model_type ) {
	case MXT_DALSA_GEV_CAMERA_SHADOBOX:
		mx_status = mxd_dalsa_gev_camera_shadobox_trigger(
						vinput, dalsa_gev_camera );
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Triggering is not yet implemented for the DALSA camera "
		"model '%s' used by video input '%s'.",
			dalsa_gev_camera->device_model_name,
			vinput->record->name );
		break;
	}

	/* Save sequence parameters where the wait thread can find them. */

	total_num_frames_at_start =
		mx_atomic_read32( &(dalsa_gev_camera->total_num_frames) );

	mx_atomic_write32( &(dalsa_gev_camera->total_num_frames_at_start),
				total_num_frames_at_start );

	milliseconds_per_frame = mx_round( 1000.0 * frame_time );

	mx_atomic_write32( &(dalsa_gev_camera->milliseconds_per_frame),
				milliseconds_per_frame );

	/* num_frames_left has to be written last, since the dead reckoning
	 * version of the wait thread uses the change in value to decide
	 * that it has to start estimating frame arrival times.
	 */

	/* We put a memory barrier here to prevent the order of events
	 * from being changed.
	 */

	mx_atomic_memory_barrier();

	mx_atomic_write32( &(dalsa_gev_camera->num_frames_left), num_frames );

	/* Enable image transfer to the image buffers. */

	MX_DEBUG(-2,("%s: '%s' ARMED.", fname, vinput->record->name ));

#if MXD_DALSA_GEV_CAMERA_USE_GEV_INITIALIZE_TRANSFER

	gev_status = GevInitializeTransfer(
			dalsa_gev_camera->camera_handle,
			Asynchronous,
			dalsa_gev_camera->frame_buffer_array_size_in_bytes,
			dalsa_gev_camera->num_frame_buffers,
			dalsa_gev_camera->frame_buffer_array );

	if ( gev_status != GEVLIB_OK ) {
		return mxd_dalsa_gev_camera_api_error( gev_status, fname,
						"GevInitializeTransfer()");
	}

	MX_DEBUG(-2,("%s: '%s' GevInitImageTransfer() called.",
			fname, vinput->record->name ));
#else	/* Not GevInitializeTransfer() */

	gev_status = GevInitImageTransfer( dalsa_gev_camera->camera_handle,
					Asynchronous,
					dalsa_gev_camera->num_frame_buffers,
					dalsa_gev_camera->frame_buffer_array );

	if ( gev_status != GEVLIB_OK ) {
		return mxd_dalsa_gev_camera_api_error( gev_status, fname,
						"GevInitImageTransfer()");
	}

	MX_DEBUG(-2,("%s: '%s' GevInitImageTransfer() called.",
			fname, vinput->record->name ));

#endif	/* Not GevInitializeTransfer() */


	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dalsa_gev_camera_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_dalsa_gev_camera_trigger()";

	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	MX_SEQUENCE_PARAMETERS *sp = NULL;
	UINT32 num_frames_to_acquire;
	GEV_STATUS gev_status;
	mx_status_type mx_status;

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
					&dalsa_gev_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sp = &(vinput->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		num_frames_to_acquire = 1;
		break;
	case MXT_SQ_STREAM:
		num_frames_to_acquire = -1;
		break;
	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_DURATION:
		num_frames_to_acquire = sp->parameter_array[0];
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Sequence type %ld has not yet been implemented for '%s'.",
			sp->sequence_type, vinput->record->name );
		break;
	}

	gev_status = GevStartImageTransfer( dalsa_gev_camera->camera_handle,
						num_frames_to_acquire );

	MXW_UNUSED( gev_status );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dalsa_gev_camera_stop( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_dalsa_gev_camera_stop()";

	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	GEV_STATUS gev_status;
	mx_status_type mx_status;

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
					&dalsa_gev_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DALSA_GEV_CAMERA_DEBUG_STOP
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	gev_status = GevStopImageTransfer( dalsa_gev_camera->camera_handle );

	if ( gev_status != GEVLIB_OK ) {
		return mxd_dalsa_gev_camera_api_error( gev_status,
					fname, "GevStopImageTransfer()");
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dalsa_gev_camera_abort( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_dalsa_gev_camera_abort()";

	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	GEV_STATUS gev_status;
	mx_status_type mx_status;

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
					&dalsa_gev_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DALSA_GEV_CAMERA_DEBUG_STOP
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	gev_status = GevAbortImageTransfer( dalsa_gev_camera->camera_handle );

	if ( gev_status != GEVLIB_OK ) {
		return mxd_dalsa_gev_camera_api_error( gev_status,
					fname, "GevAbortImageTransfer()");
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dalsa_gev_camera_get_extended_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
		"mxd_dalsa_gev_camera_get_extended_status()";

	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	int32_t total_num_frames, total_num_frames_at_start;
	int32_t num_frames_left;
	mx_status_type mx_status;

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
					&dalsa_gev_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vinput->status = 0;

	total_num_frames = 
		mx_atomic_read32( &(dalsa_gev_camera->total_num_frames) );

	total_num_frames_at_start = 
	    mx_atomic_read32( &(dalsa_gev_camera->total_num_frames_at_start));

	vinput->total_num_frames = total_num_frames;

	vinput->last_frame_number =
		total_num_frames - total_num_frames_at_start - 1;

	num_frames_left = mx_atomic_read32(
				&(dalsa_gev_camera->num_frames_left) );

	if ( num_frames_left > 0 ) {
		vinput->status |= MXSF_VIN_IS_BUSY;
	}

	if ( vinput->status & MXSF_VIN_IS_BUSY ) {
		vinput->busy = TRUE;
	} else {
		vinput->busy = FALSE;
	}

	MX_DEBUG(-2,("%s: vinput->total_num_frames = %ld",
			fname, vinput->total_num_frames));
	MX_DEBUG(-2,("%s: vinput->last_frame_number = %ld",
			fname, vinput->last_frame_number));
	MX_DEBUG(-2,("%s: dalsa_gev_camera->num_frames_left = %ld",
			fname, (long) num_frames_left));
	MX_DEBUG(-2,("%s: vinput->status = %#lx",
			fname, vinput->status));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dalsa_gev_camera_get_frame( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_dalsa_gev_camera_get_frame()";

	MX_DALSA_GEV_CAMERA *dalsa_gev_camera = NULL;
	void *mx_data_address = NULL;
	void *dalsa_gev_data_address = NULL;
	unsigned long i;
	mx_status_type mx_status;

	mx_status = mxd_dalsa_gev_camera_get_pointers( vinput,
					&dalsa_gev_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( vinput->frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"No image frames have been acquired yet for camera '%s'.",
			vinput->record->name );
	}

	/* Find the MX video input record's image data buffer. */

	mx_data_address = vinput->frame->image_data;

	if ( mx_data_address == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX data address for the primary image frame buffer "
		"for camera '%s' is NULL.", vinput->record->name );
	}

	/* Find the DALSA GeV image data buffer for the requested image. */

	i = vinput->frame_number;

	dalsa_gev_data_address = dalsa_gev_camera->frame_buffer_array[i];

	/* Now copy the image data from DALSA to the MX frame buffer. */

	memcpy( mx_data_address, dalsa_gev_data_address,
			vinput->frame->image_length );

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
	UINT32 uint32_width, uint32_height;
	UINT32 uint32_x_offset, uint32_y_offset;
	UINT32 uint32_image_format;
	GEV_STATUS gev_status;
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

	case MXLV_VIN_BITS_PER_PIXEL:
	case MXLV_VIN_BYTES_PER_FRAME:
	case MXLV_VIN_BYTES_PER_PIXEL:
	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:
	case MXLV_VIN_FRAMESIZE:
		gev_status = GevGetImageParameters(
				dalsa_gev_camera->camera_handle,
				&uint32_width, &uint32_height,
				&uint32_x_offset, &uint32_y_offset,
				&uint32_image_format );

		if ( gev_status != GEVLIB_OK ) {
			return mxd_dalsa_gev_camera_api_error( gev_status,
					fname, "GevGetImageParameters()");
		}

		vinput->framesize[0] = uint32_width;
		vinput->framesize[1] = uint32_height;

		switch( uint32_image_format ) {
		case fmtMono8:
			vinput->bits_per_pixel = 8;
			break;
		case fmtMono10:
			vinput->bits_per_pixel = 10;
			break;
		case fmtMono12:
			vinput->bits_per_pixel = 12;
			break;
		case fmtMono14:
			vinput->bits_per_pixel = 14;
			break;
		case fmtMono16:
			vinput->bits_per_pixel = 16;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Image format %#lx is not supported for camera '%s'.",
				(unsigned long) uint32_image_format,
				vinput->record->name );
			break;
		}

		if ( vinput->bits_per_pixel <= 8 ) {
			vinput->bytes_per_pixel = 1;
			vinput->image_format = MXT_IMAGE_FORMAT_GREY8;
		} else
		if ( vinput->bits_per_pixel <= 16 ) {
			vinput->bytes_per_pixel = 2;
			vinput->image_format = MXT_IMAGE_FORMAT_GREY16;
		} else
		if ( vinput->bits_per_pixel <= 32 ) {
			vinput->bytes_per_pixel = 4;
			vinput->image_format = MXT_IMAGE_FORMAT_GREY32;
		} else {
			return mx_error( MXE_UNSUPPORTED, fname,
			"%lu bits per pixel is not supported for camera '%s'.",
				vinput->bits_per_pixel, vinput->record->name );
		}

		mx_status = mx_image_get_image_format_name_from_type(
				vinput->image_format, vinput->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		vinput->bytes_per_frame = vinput->framesize[0]
					* vinput->framesize[1]
					* vinput->bytes_per_pixel;

#if MXD_DALSA_GEV_CAMERA_DEBUG_MX_PARAMETERS
		MX_DEBUG(-2,("%s: camera '%s' handle = %p",
			fname, vinput->record->name,
			dalsa_gev_camera->camera_handle));
		MX_DEBUG(-2,("%s: framesize = (%ld,%ld).",
			fname, vinput->framesize[0], vinput->framesize[1]));
		MX_DEBUG(-2,("%s: bits_per_pixel = %lu, bytes_per_pixel = %f",
		    fname, vinput->bits_per_pixel, vinput->bytes_per_pixel));
		MX_DEBUG(-2,("%s: image format = '%s', bytes_per_frame = %lu",
			fname, vinput->image_format_name,
			vinput->bytes_per_frame ));
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
		case MXLV_DALSA_GEV_CAMERA_FEATURE_RANGE:
		case MXLV_DALSA_GEV_CAMERA_GAIN:
		case MXLV_DALSA_GEV_CAMERA_SHOW_FEATURES:
		case MXLV_DALSA_GEV_CAMERA_SHOW_FEATURE_VALUES:
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
		case MXLV_DALSA_GEV_CAMERA_FEATURE_RANGE:
			return mxd_dalsa_gev_camera_show_feature_range(
					dalsa_gev_camera );
			break;
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
		    mxd_dalsa_gev_camera_show_feature_list( dalsa_gev_camera,
									FALSE );
			break;
		case MXLV_DALSA_GEV_CAMERA_SHOW_FEATURE_VALUES:
			mx_status =
		    mxd_dalsa_gev_camera_show_feature_list( dalsa_gev_camera,
									TRUE );
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

