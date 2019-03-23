/*
 * Name:    d_eiger.c
 *
 * Purpose: MX area detector driver for the Dectris Eiger series detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2018-2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_EIGER_DEBUG			TRUE

#define MXD_EIGER_DEBUG_OPEN		TRUE

#include <stdio.h>
#include <stdlib.h>

#if defined(__GNUC__)
#  define __USE_XOPEN
#endif

#include "mx_util.h"
#include "mx_time.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_atomic.h"
#include "mx_bit.h"
#include "mx_socket.h"
#include "mx_array.h"
#include "mx_thread.h"
#include "mx_image.h"
#include "mx_module.h"
#include "mx_http.h"
#include "mx_json.h"
#include "mx_area_detector.h"
#include "d_eiger.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_eiger_record_function_list = {
	mxd_eiger_initialize_driver,
	mxd_eiger_create_record_structures,
	mx_area_detector_finish_record_initialization,
	NULL,
	NULL,
	mxd_eiger_open,
	NULL,
	NULL,
	NULL,
	mxd_eiger_special_processing_setup,
};

MX_AREA_DETECTOR_FUNCTION_LIST
mxd_eiger_ad_function_list = {
	mxd_eiger_arm,
	mxd_eiger_trigger,
	NULL,
	mxd_eiger_stop,
	NULL,
	mxd_eiger_get_last_and_total_frame_numbers,
	mxd_eiger_get_last_and_total_frame_numbers,
	mxd_eiger_get_status,
	NULL,
	mxd_eiger_readout_frame,
	mxd_eiger_correct_frame,
	mxd_eiger_transfer_frame,
	mxd_eiger_load_frame,
	mxd_eiger_save_frame,
	mxd_eiger_copy_frame,
	NULL,
	mxd_eiger_get_parameter,
	mxd_eiger_set_parameter,
	mxd_eiger_measure_correction
};

MX_RECORD_FIELD_DEFAULTS mxd_eiger_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_EIGER_STANDARD_FIELDS
};

long mxd_eiger_num_record_fields
		= sizeof( mxd_eiger_rf_defaults )
		  / sizeof( mxd_eiger_rf_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_eiger_rfield_def_ptr
			= &mxd_eiger_rf_defaults[0];

static mx_status_type mxd_eiger_process_function( void *record_ptr,
							void *record_field_ptr,
							int operation );

/*---*/

static mx_status_type
mxd_eiger_get_pointers( MX_AREA_DETECTOR *ad,
			MX_EIGER **eiger,
			const char *calling_fname )
{
	static const char fname[] = "mxd_eiger_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (eiger == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_EIGER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*eiger = (MX_EIGER *) ad->record->record_type_struct;

	if ( *eiger == (MX_EIGER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_EIGER pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxd_eiger_get( MX_AREA_DETECTOR *ad,
		MX_EIGER *eiger,
		char *command_url,
		char *response,
		size_t max_response_length )
{
	static const char fname[] = "mxd_eiger_get()";

	unsigned long http_status_code;
	char content_type[80];
	mx_status_type mx_status;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}
	if ( eiger == (MX_EIGER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EIGER pointer passed was NULL." );
	}
	if ( command_url == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command_url pointer passed was NULL." );
	}
	if ( response == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The response pointer passed was NULL." );
	}

	mx_status = mx_http_get( eiger->http, command_url,
				&http_status_code,
				content_type, sizeof(content_type),
				response, max_response_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* FIXME: Define some macros for the HTTP status code values. */

	if ( http_status_code != 200 ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"HTTP status code (%ld) returned for EIGER detector '%s' "
		"for URL '%s'.",
			http_status_code, eiger->record->name, command_url );
	}

	if ( strcmp( content_type, "application/json" ) != 0 ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The HTTP content type '%s' returned for URL '%s' at "
		"EIGER detector '%s' was not 'application/json'.  "
		"The body of the response was '%s'.",
			content_type, command_url, eiger->record->name,
			response );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_eiger_get_value( MX_AREA_DETECTOR *ad,
			MX_EIGER *eiger,
			char *command_url,
			long mx_datatype,
			long num_dimensions,
			long *dimension,
			void *value )
{
	static const char fname[] = "mxd_eiger_get_value()";

	MX_JSON *json = NULL;
	cJSON *returned_cjson_ptr = NULL;
	cJSON *first_cjson_element = NULL;
	cJSON *current_cjson_element = NULL;
	int first_cjson_type = -1;
	char response[1024];
	long *dimension_ptr = NULL;
	long dimension_zero[0];
	char *mx_char_array = NULL;
	short *mx_short_array = NULL;
	long *mx_long_array = NULL;
	float *mx_float_array = NULL;
	double *mx_double_array = NULL;
	int64_t *mx_int64_array = NULL;
	mx_bool_type *mx_bool_array = NULL;
	char *mx_string = NULL;
	long i;
	mx_status_type mx_status;

	if ( dimension == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The dimension pointer passed was NULL for command URL '%s'.",
			command_url );
	}
	if ( num_dimensions < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The number of dimensions (%ld) is less than zero for "
		"command URL '%s'.", num_dimensions, command_url );
	}
	if ( value == (void *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The value pointer passed for command '%s' was NULL.",
			command_url );
	}

	/* Treat a scalar as a 1-dimensional object with only one element. */

	if ( num_dimensions == 0 ) {
		num_dimensions = 1;
		dimension_ptr = dimension_zero;
		dimension_ptr[0] = 1L;
	} else
	if ( num_dimensions == 1 ) {
		dimension_ptr = dimension;
	} else
	if ( num_dimensions > 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Objects with dimension (%ld) greater than 1 are not "
		"currently supported.", num_dimensions );
	}

	mx_status = mxd_eiger_get( ad, eiger, command_url,
			response, sizeof(response) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: EIGER response = '%s'", fname, response ));

	mx_status = mx_json_parse( &json, response);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: json->cjson = %p", fname, json->cjson));

	MX_DEBUG(-2,("%s: cJSON_Print( json->cjson ) = '%s'.",
		fname, cJSON_Print( json->cjson ) ));

	returned_cjson_ptr = json->cjson;

	if ( returned_cjson_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The cJSON pointer for MX_JSON object %p is NULL "
		"for command '%s'.", json, command_url );
	}

	if ( cJSON_IsObject( returned_cjson_ptr ) 
	  && ( returned_cjson_ptr->child != NULL ) )
	{
		first_cjson_element = returned_cjson_ptr->child;

		MX_DEBUG(-2,("%s: CHILD first_cjson_element->type = %d",
			fname, first_cjson_element->type ));
	} else {
		first_cjson_element = returned_cjson_ptr;

		MX_DEBUG(-2,("%s: NOT CHILD first_cjson_element->type = %d",
			fname, first_cjson_element->type ));
	}

	MX_DEBUG(-2,
	("%s: first_cjson_element: name = '%s', type = %#x, value = '%s'",
			fname, first_cjson_element->string,
			first_cjson_element->type,
			first_cjson_element->valuestring ));

	MX_DEBUG(-2,("%s: cJSON_Print(first_cjson_element = %p) is:",
		fname, first_cjson_element ));

#if 1
	mx_breakpoint();
#endif

	cJSON_Print( first_cjson_element );

#if 0
	if ( first_cjson_element->child != NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Multidimensional cJSON objects ( cjson->child != NULL ) "
		"are not currently supported for command '%s'.",
			command_url );
	}
#endif

	first_cjson_type = first_cjson_element->type;

	/* Regrettably, first_cjson_type is a bitmask and the bitmask
	 * positions are not officially documented, so we can't just do
	 * this using a switch statement.
	 */

	if ( cJSON_IsString( first_cjson_element ) ) {
		if ( mx_datatype != MXFT_STRING ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"MX expected a string value, but the cJSON type "
			"(%#x) is not a string for command '%s'.",
				first_cjson_type, command_url );
		}

		mx_string = value;
	} else
	if ( cJSON_IsBool( first_cjson_element ) ) {
		if ( mx_datatype != MXFT_BOOL ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"MX expected a value of type '%s' (%ld), but the "
			"cJSON object is a cJSON boolean (%d) "
			"for command '%s'.",
			    mx_get_datatype_name_from_datatype( mx_datatype ),
			    mx_datatype, first_cjson_type, command_url );
		}

		mx_bool_array = value;
	} else
	if ( cJSON_IsNumber( first_cjson_element ) ) {
		switch( mx_datatype ) {
		case MXFT_CHAR:
		case MXFT_UCHAR:
			mx_char_array = value;
			break;
		case MXFT_SHORT:
		case MXFT_USHORT:
			mx_short_array = value;
			break;
		case MXFT_LONG:
		case MXFT_ULONG:
		case MXFT_HEX:
			mx_long_array = value;
			break;
		case MXFT_FLOAT:
			mx_float_array = value;
			break;
		case MXFT_DOUBLE:
			mx_double_array = value;
			break;
		case MXFT_INT64:
		case MXFT_UINT64:
			mx_int64_array = value;
			break;
		default:
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"MX expected a value of type '%s' (%ld), but the "
			"cJSON object is a cJSON_Number for command '%s'.",
			    mx_get_datatype_name_from_datatype( mx_datatype ),
			    mx_datatype, command_url );
			break;
		}
	} else
	if ( cJSON_IsInvalid( first_cjson_element ) ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"The cJSON object returned for command '%s' "
			"was Invalid.", command_url );
	} else
	if ( cJSON_IsNull( first_cjson_element ) ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"The cJSON object returned for command '%s' "
			"was a cJSON Null.", command_url );
	} else
	if ( cJSON_IsArray( first_cjson_element ) ) {
		return mx_error( MXE_UNSUPPORTED, fname,
			"Arrays of cJSON arrays are not supported "
			"at this time for command '%s'.", command_url );
	} else
	if ( cJSON_IsObject( first_cjson_element ) ) {
#if 0
		return mx_error( MXE_UNSUPPORTED, fname,
			"Arrays of cJSON objects are not supported "
			"at this time for command '%s'.", command_url );
#endif
	} else
	if ( cJSON_IsRaw( first_cjson_element ) ) {
		return mx_error( MXE_UNSUPPORTED, fname,
			"Arrays of cJSON raw objects are not supported "
			"at this time for command '%s'.", command_url );
	} else {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"The cJSON object type (%d) is not a recognized "
			"type for command '%s'.",
				first_cjson_type, command_url );
	}

	if ( mx_datatype == MXFT_STRING ) {

	    /* A string field can be handled directly. */

	    strlcpy( value, mx_string, dimension_ptr[0] );

	} else {
	    current_cjson_element = first_cjson_element;

	    for ( i = 0; i < dimension_ptr[0]; i++ )
	    {
		switch( mx_datatype ) {
		case MXFT_CHAR:
		case MXFT_UCHAR:
			mx_char_array[i] = current_cjson_element->valueint;
			break;
		case MXFT_SHORT:
		case MXFT_USHORT:
			mx_short_array[i] = current_cjson_element->valueint;
			break;
		case MXFT_LONG:
		case MXFT_ULONG:
		case MXFT_HEX:
			mx_long_array[i] = current_cjson_element->valueint;
			break;
		case MXFT_FLOAT:
			mx_float_array[i] = current_cjson_element->valuedouble;
			break;
		case MXFT_DOUBLE:
			mx_double_array[i] = current_cjson_element->valuedouble;
			break;
		case MXFT_INT64:
		case MXFT_UINT64:
			mx_int64_array[i] = current_cjson_element->valueint;
			break;
		case MXFT_BOOL:
			if ( cJSON_IsFalse( current_cjson_element ) ) {
				mx_bool_array[i] = FALSE;
			} else
			if ( cJSON_IsTrue( current_cjson_element ) ) {
				mx_bool_array[i] = TRUE;
			} else {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"The supposed boolean object '%s' was neither "
				"'false' or 'true' for command '%s'.",
					current_cjson_element->valuestring,
					command_url );
			}
			break;
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected datatype (%ld) seen while attempting to "
			"copy a cJSON value '%s' to an MX array "
			"for command '%s'.",
				mx_datatype,
				current_cjson_element->string,
				command_url );
			break;
		}

		current_cjson_element = current_cjson_element->next;
	    }
	}

	/* We are done with the original cJSON object, so delete it. */

	cJSON_Delete( returned_cjson_ptr );

	mx_free( json );

	MX_DEBUG(-2,("%s complete.", fname ));

	return MX_SUCCESSFUL_RESULT;
}

/*========================================================================*/

MX_EXPORT mx_status_type
mxd_eiger_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_driver( driver,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_eiger_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_eiger_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_EIGER *eiger = NULL;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	eiger = (MX_EIGER *) malloc( sizeof(MX_EIGER) );

	if ( eiger == (MX_EIGER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_EIGER structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = eiger;
	record->class_specific_function_list = 
			&mxd_eiger_ad_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	eiger->record = record;

	ad->trigger_mode = 0;
	ad->initial_correction_flags = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_eiger_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_eiger_open()";

	MX_AREA_DETECTOR *ad = NULL;
	MX_EIGER *eiger = NULL;
	char command_url[200];
	char response[1000];
	long dimension[1];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

#if MXD_EIGER_DEBUG_OPEN
	MX_DEBUG( 2,("%s invoked for area detector '%s'.",
		fname, record->name ));
#endif

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Create an EIGER HTTP handler. */

	mx_status = mx_http_create( &(eiger->http), record, "libcurl" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG_OPEN
	MX_DEBUG(-2,("%s: Getting the detector state.", fname));
#endif

	/* Get the detector state. */

	snprintf( command_url, sizeof(command_url),
		"http://%s/detector/api/%s/status/state",
		eiger->hostname, eiger->simplon_version );

	dimension[0] = sizeof(response);

	mx_status = mxd_eiger_get_value( ad, eiger, command_url,
					MXFT_STRING, 1, dimension,
					response );

	if ( mx_status.code != MXE_SUCCESS );

#if MXD_EIGER_DEBUG_OPEN
	MX_DEBUG(-2,("%s: EIGER '%s' response = '%s'.",
				fname, record->name, response));

	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_eiger_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_eiger_arm()";

	MX_EIGER *eiger = NULL;
	MX_SEQUENCE_PARAMETERS *sp = NULL;
	mx_status_type mx_status;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sp = &(ad->sequence_parameters);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_eiger_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_eiger_trigger()";

	MX_EIGER *eiger = NULL;
	mx_status_type mx_status;

	eiger = NULL;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_eiger_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_eiger_stop()";

	MX_EIGER *eiger = NULL;
	mx_status_type mx_status;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_eiger_get_last_and_total_frame_numbers( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_eiger_get_last_and_total_frame_numbers()";

	MX_EIGER *eiger = NULL;
	mx_status_type mx_status;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG
	MX_DEBUG( 2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_eiger_get_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_eiger_get_status()";

	MX_EIGER *eiger = NULL;
	mx_status_type mx_status;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG
	MX_DEBUG( 2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_eiger_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_eiger_readout_frame()"; 
	MX_EIGER *eiger = NULL;
	mx_status_type mx_status;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s', frame %ld.",
		fname, ad->record->name, ad->readout_frame ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_eiger_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_eiger_correct_frame()";

	MX_EIGER *eiger = NULL;
	mx_status_type mx_status;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG
	MX_DEBUG(-2,
		("%s invoked for area detector '%s', correction_flags=%#lx.",
		fname, ad->record->name, ad->correction_flags ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_eiger_transfer_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
			"mxd_eiger_transfer_frame()";

	MX_EIGER *eiger = NULL;
	mx_status_type mx_status;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s', transfer_frame = %ld.",
		fname, ad->record->name, ad->transfer_frame ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_eiger_load_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_eiger_load_frame()";

	MX_EIGER *eiger = NULL;
	mx_status_type mx_status;

	eiger = NULL;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_eiger_save_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_eiger_save_frame()";

	MX_EIGER *eiger = NULL;
	mx_status_type mx_status;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_eiger_copy_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_eiger_copy_frame()";

	MX_EIGER *eiger = NULL;
	mx_status_type mx_status;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for area detector '%s', source = %#lx, destination = %#lx",
		fname, ad->record->name, ad->copy_frame[0], ad->copy_frame[1]));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_eiger_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_eiger_get_parameter()";

	MX_EIGER *eiger = NULL;
	mx_status_type mx_status;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG
	{
		char parameter_name_buffer[80];

		MX_DEBUG(-2,("%s: record '%s', parameter '%s'",
			fname, ad->record->name,
			mx_get_parameter_name_from_type( ad->record,
					ad->parameter_type,
					parameter_name_buffer,
					sizeof(parameter_name_buffer) ) ));
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
#if MXD_EIGER_DEBUG
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

#if MXD_EIGER_DEBUG
		MX_DEBUG(-2,("%s: GET sequence_type = %ld",
			fname, ad->sequence_parameters.sequence_type));
#endif

#if MXD_EIGER_DEBUG
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

	case MXLV_AD_DATAFILE_ALLOW_OVERWRITE:
		break;

	case MXLV_AD_DATAFILE_AUTOSELECT_NUMBER:
		break;

	case MXLV_AD_DATAFILE_DIRECTORY:
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
mxd_eiger_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_eiger_set_parameter()";

	MX_EIGER *eiger = NULL;
	mx_status_type mx_status;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG
	{
		char parameter_name_buffer[80];

		MX_DEBUG(-2,("%s: record '%s', parameter '%s'",
			fname, ad->record->name,
			mx_get_parameter_name_from_type( ad->record,
					ad->parameter_type,
					parameter_name_buffer,
					sizeof(parameter_name_buffer) ) ));
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
mxd_eiger_measure_correction( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_eiger_measure_correction()";

	MX_EIGER *eiger = NULL;
	mx_status_type mx_status;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG
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

/*==========================================================================*/

#if 0
MX_EXPORT mx_status_type
mxd_eiger_command( MX_EIGER *eiger,
			char *command,
			char *response,
			size_t response_buffer_length,
			mx_bool_type suppress_output )

{
	static const char fname[] = "mxd_eiger_command()";

	mx_status_type mx_status;

	if ( eiger == (MX_EIGER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EIGER pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer passed was NULL." );
	}

	mx_status =  mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"%s is not yet implemented.", fname );

	return mx_status;
}
#endif

/*==========================================================================*/

MX_EXPORT mx_status_type
mxd_eiger_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field = NULL;
	MX_RECORD_FIELD *record_field_array = NULL;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case 0:
			record_field->process_function
					= mxd_eiger_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================================================================*/

static mx_status_type
mxd_eiger_process_function( void *record_ptr,
				void *record_field_ptr,
				int operation )
{
	static const char fname[] = "mxd_eiger_process_function()";

	MX_RECORD *record = NULL;
	MX_RECORD_FIELD *record_field = NULL;
	MX_EIGER *eiger = NULL;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	eiger = (MX_EIGER *) record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		default:
			MX_DEBUG( 1,(
			    "%s: *** unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		default:
			MX_DEBUG( 1,(
			    "%s: *** unknown MX_PROCESS_PUT label value = %ld",
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

