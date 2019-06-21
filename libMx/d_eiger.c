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

#define MXD_EIGER_DEBUG_PARAMETERS	TRUE

#include <stdio.h>
#include <stdlib.h>

#if defined(__GNUC__)
#  define __USE_XOPEN
#endif

#include "mx_util.h"
#include "mx_time.h"
#include "mx_record.h"
#include "mx_variable.h"
#include "mx_driver.h"
#include "mx_atomic.h"
#include "mx_bit.h"
#include "mx_socket.h"
#include "mx_array.h"
#include "mx_thread.h"
#include "mx_image.h"
#include "mx_module.h"
#include "mx_url.h"
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

MX_AREA_DETECTOR_FUNCTION_LIST mxd_eiger_ad_function_list = {
	mxd_eiger_arm,
	mxd_eiger_trigger,
	NULL,
	mxd_eiger_stop,
	mxd_eiger_abort,
	mxd_eiger_get_last_and_total_frame_numbers,
	mxd_eiger_get_last_and_total_frame_numbers,
	mxd_eiger_get_status,
	NULL,
	NULL,
	NULL,
	mxd_eiger_transfer_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_eiger_get_parameter,
	mxd_eiger_set_parameter,
	NULL
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
						void *socket_handler_ptr,
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
		char *module_name,
		char *key_name,
		char *response,
		size_t max_response_length )
{
	static const char fname[] = "mxd_eiger_get()";

	char command_url[200];
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
	if ( module_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The module_name pointer passed was NULL." );
	}
	if ( key_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The key_name pointer passed was NULL." );
	}
	if ( response == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The response pointer passed was NULL." );
	}

	snprintf( command_url, sizeof(command_url),
		"http://%s/%s/api/%s/%s",
		eiger->hostname, module_name,
		eiger->simplon_version, key_name );

	mx_status = mx_url_get( eiger->url_server_record,
				command_url,
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
			char *module_name,
			char *key_name,
			long mx_datatype,
			long num_dimensions,
			long *dimension,
			void *value )
{
	static const char fname[] = "mxd_eiger_get_value()";

	MX_JSON *json = NULL;
	char response[1024];
	long *dimension_ptr = NULL;
	long dimension_zero[1];
	char value_type_string[40];
	long mx_json_datatype;
	mx_bool_type native_longs_are_64bits;
	size_t scalar_element_size, max_key_value_bytes;
	mx_status_type mx_status;

	if ( dimension == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The dimension pointer passed was NULL for "
		"EIGER detector '%s', module '%s', key '%s'.",
			eiger->record->name, module_name, key_name );
	}
	if ( num_dimensions < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The number of dimensions (%ld) is less than zero for "
		"EIGER detector '%s', module '%s', key '%s'.",
			num_dimensions, eiger->record->name,
			module_name, key_name );
	}
	if ( value == (void *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The value pointer passed for "
		"EIGER detector '%s', module '%s', key '%s' was NULL.",
			eiger->record->name, module_name, key_name );
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

	if ( MX_WORDSIZE >= 64 ) {
		native_longs_are_64bits = TRUE;
	} else {
		native_longs_are_64bits = FALSE;
	}

	scalar_element_size = mx_get_scalar_element_size( mx_datatype,
						native_longs_are_64bits );

	/* FIXME: Need to handle bigger arrays here. */

	if ( mx_datatype == MXFT_STRING ) {
		max_key_value_bytes = dimension_ptr[0];
	} else {
		max_key_value_bytes = scalar_element_size;
	}

	/* Get the JSON key value. */

	mx_status = mxd_eiger_get( ad, eiger,
			module_name, key_name,
			response, sizeof(response) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: EIGER response = '%s'", fname, response ));

	mx_status = mx_json_parse( &json, response);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The 'value_type' key should contain the EIGER datatype of the
	 * 'value' key.  The 'value_type' key itself should be a 'string'.
	 */

	mx_status = mx_json_get_key( json, "value_type", MXFT_STRING,
				value_type_string, sizeof(value_type_string) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 1
	MX_DEBUG(-2,("%s: value_type = '%s'", fname, value_type_string));
#endif
	if ( strcmp( value_type_string, "string" ) == 0 ) {
		mx_json_datatype = MXFT_STRING;
	} else
	if ( strcmp( value_type_string, "bool" ) == 0 ) {
		mx_json_datatype = MXFT_BOOL;
	} else
	if ( strcmp( value_type_string, "int" ) == 0 ) {
		mx_json_datatype = MXFT_LONG;
	} else
	if ( strcmp( value_type_string, "uint" ) == 0 ) {
		mx_json_datatype = MXFT_ULONG;
	} else
	if ( strcmp( value_type_string, "float" ) == 0 ) {
		mx_json_datatype = MXFT_FLOAT;
	} else {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Support for EIGER JSON datatype '%s' is not yet implemented.",
			value_type_string );
	}

	/* Now we can return the actual returned EIGER value for this URL. */

	mx_status = mx_json_get_compatible_key( json, "value",
					mx_datatype, mx_json_datatype,
					value, max_key_value_bytes );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We are done with the original cJSON object, so delete it. */

	cJSON_Delete( json->cjson );

	mx_free( json );

#if 0
	MX_DEBUG(-2,("%s complete.", fname ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxd_eiger_put( MX_AREA_DETECTOR *ad,
		MX_EIGER *eiger,
		char *module_name,
		char *key_name,
		char *key_value,
		char *response,
		size_t max_response_length )
{
	static const char fname[] = "mxd_eiger_put()";

	char command_url[200];
	unsigned long http_status_code;
	mx_status_type mx_status;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}
	if ( eiger == (MX_EIGER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EIGER pointer passed was NULL." );
	}
	if ( module_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The module_name pointer passed was NULL." );
	}
	if ( key_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The key_name pointer passed was NULL." );
	}
	if ( key_value == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The key_value pointer passed was NULL." );
	}

	snprintf( command_url, sizeof(command_url),
		"http://%s/%s/api/%s/%s",
		eiger->hostname, module_name,
		eiger->simplon_version, key_name );

	mx_status = mx_url_put( eiger->url_server_record,
				command_url,
				&http_status_code,
				"application/json",
				key_value, -1,
				response, max_response_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_eiger_put_value( MX_AREA_DETECTOR *ad,
			MX_EIGER *eiger,
			char *module_name,
			char *key_name,
			long mx_datatype,
			long num_dimensions,
			long *dimension,
			void *value,
			char *response,
			size_t max_response_length )
{
	static const char fname[] = "mxd_eiger_put_value()";

	char key_value[MXU_EIGER_KEY_VALUE_LENGTH+1];
	char key_body[500];
	mx_bool_type bool_value;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked for detector '%s'.",
			fname, eiger->record->name));

	/* FIXME: At present, only 1-D strings or one element numbers
	 * are supported.
	 */

	switch( mx_datatype ) {
	case MXFT_STRING:
		snprintf( key_body, sizeof(key_body),
			"\"%s\"", (char *) value );
		break;
	case MXFT_BOOL:
		bool_value = *((mx_bool_type *) value);

		if (bool_value == 0 ) {
			strlcpy( key_body, "0", sizeof(key_body) );
		} else {
			strlcpy( key_body, "1", sizeof(key_body) );
		}
		break;
	case MXFT_CHAR:
		snprintf( key_body, sizeof(key_body),
			"%d", (int) *((char *) value) );
		break;
	case MXFT_UCHAR:
		snprintf( key_body, sizeof(key_body),
			"%u", (unsigned int) *((unsigned char *) value) );
		break;
	case MXFT_SHORT:
		snprintf( key_body, sizeof(key_body),
			"%hd", *((short *) value) );
		break;
	case MXFT_USHORT:
		snprintf( key_body, sizeof(key_body),
			"%hu", *((unsigned short *) value) );
		break;
	case MXFT_LONG:
		snprintf( key_body, sizeof(key_body),
			"%ld", *((long *) value) );
		break;
	case MXFT_ULONG:
		snprintf( key_body, sizeof(key_body),
			"%lu", *((unsigned long *) value) );
		break;
	case MXFT_FLOAT:
		snprintf( key_body, sizeof(key_body),
			"%f", *((float *) value) );
		break;
	case MXFT_DOUBLE:
		snprintf( key_body, sizeof(key_body),
			"%f", *((double *) value) );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"MX datatype (%ld) is not supported for EIGER detector '%s', "
		"module '%s', key '%s'.",
			mx_datatype, eiger->record->name,
			module_name, key_name );
		break;
	}

#if 0
	snprintf( key_value, sizeof(key_value),
		"{\r\n  \"value\" : %s\r\n}\r\n", key_body );
#else
	snprintf( key_value, sizeof(key_value),
		"{ \"value\" : %s }\r\n", key_body );
#endif

	mx_status = mxd_eiger_put( ad, eiger,
			module_name, key_name, key_value,
			response, max_response_length );

	return mx_status;
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

	ad->trigger_mode = MXF_DEV_INTERNAL_TRIGGER;
	ad->initial_correction_flags = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_eiger_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_eiger_open()";

	MX_AREA_DETECTOR *ad = NULL;
	MX_EIGER *eiger = NULL;
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

	/* Get the detector model and type. */

	dimension[0] = sizeof(eiger->description);

	mx_status = mxd_eiger_get_value( ad, eiger,
					"detector", "config/description",
					MXFT_STRING, 1, dimension,
					eiger->description );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the detector software version. */

	dimension[0] = sizeof(eiger->software_version);

	mx_status = mxd_eiger_get_value( ad, eiger,
					"detector", "config/software_version",
					MXFT_STRING, 1, dimension,
					eiger->software_version );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the detector serial number. */

	dimension[0] = sizeof(eiger->detector_number);

	mx_status = mxd_eiger_get_value( ad, eiger,
					"detector", "config/detector_number",
					MXFT_STRING, 1, dimension,
					eiger->detector_number );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the image frame transfer mode. */

	eiger->transfer_mode = 0;

	if ( strcmp( eiger->transfer_mode_name, "monitor" ) == 0 ) {
		mx_status = mx_process_record_field_by_name( ad->record,
						"monitor_enabled",
						NULL, MX_PROCESS_GET, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if 1
		MX_DEBUG(-2,("%s: eiger->monitor_enabled = %d",
			fname, (int) eiger->monitor_enabled));
#endif

		if ( eiger->monitor_enabled ) {
			eiger->transfer_mode = MXF_EIGER_TRANSFER_MONITOR;
		} else {
			eiger->transfer_mode = 0;
		}
	} else
	if ( strcmp( eiger->transfer_mode_name, "stream" ) == 0 ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Stream image transfer mode using ZeroMQ is not yet "
		"implemented for EIGER detector '%s'.", record->name );
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Image transfer mode '%s' is not recognized for "
		"EIGER detector '%s'.",
			eiger->transfer_mode_name, record->name );
	}

#if 1
	MX_DEBUG(-2,("%s: eiger->transfer_mode = %#lx",
		fname, eiger->transfer_mode));
#endif

	/* Get the detector state. */

	dimension[0] = sizeof(response);

	mx_status = mxd_eiger_get_value( ad, eiger,
					"detector", "status/state",
					MXFT_STRING, 1, dimension,
					response );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG_OPEN
	MX_DEBUG(-2,("%s: EIGER '%s' detector state = '%s'.",
				fname, record->name, response));
#endif

	/* Fetch the detector parameters that MX will need. */

	mx_status = mx_area_detector_get_framesize( ad->record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_bits_per_pixel( ad->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_bytes_per_pixel( ad->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_image_format( ad->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_bytes_per_frame( ad->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_resolution( ad->record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->trigger_mode = MXF_DEV_INTERNAL_TRIGGER;

	ad->dictionary = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_eiger_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_eiger_arm()";

	MX_EIGER *eiger = NULL;
	MX_SEQUENCE_PARAMETERS *sp = NULL;
	unsigned long num_frames, arm;
	double exposure_time, exposure_period;
	double photon_energy;
	long dimension[1];
	char trigger_mode[20];
	char arm_response[200];
	int num_items;
	long image_frame_sequence_id;
	mx_status_type mx_status;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif
	/* Set the photon energy. */

	mx_status = mx_get_double_variable( eiger->photon_energy_record,
							&photon_energy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dimension[0] = 1;

	mx_status = mxd_eiger_put_value( ad, eiger,
			"detector", "config/photon_energy",
			MXFT_DOUBLE, 1, dimension,
			(void *) &photon_energy, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the trigger mode and sequence parameters. */

	sp = &(ad->sequence_parameters);

	/* FIXME: Haven't figured out how to make use of
	 * 'inte' trigger mode yet.
	 */

	switch( ad->trigger_mode ) {
	case MXF_DEV_INTERNAL_TRIGGER:
		switch( sp->sequence_type ) {
		case MXT_SQ_ONE_SHOT:
			num_frames = 1;
			exposure_time = sp->parameter_array[0];
			exposure_period = exposure_time;
			strlcpy( trigger_mode, "ints", sizeof(trigger_mode) );
			break;
		case MXT_SQ_MULTIFRAME:
			num_frames = mx_round( sp->parameter_array[0] );
			exposure_time = sp->parameter_array[1];
			exposure_period = sp->parameter_array[2];
			strlcpy( trigger_mode, "ints", sizeof(trigger_mode) );
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"MX sequence type %ld is not supported for internal "
			"trigger mode by area detector '%s'.",
				sp->sequence_type, ad->record->name );
			break;
		}
		break;
	case MXF_DEV_EXTERNAL_TRIGGER:
		switch( sp->sequence_type ) {
		case MXT_SQ_ONE_SHOT:
			num_frames = 1;
			exposure_time = sp->parameter_array[0];
			exposure_period = -1.0;
			strlcpy( trigger_mode, "exts", sizeof(trigger_mode) );
			break;
		case MXT_SQ_MULTIFRAME:
			num_frames = mx_round( sp->parameter_array[0] );
			exposure_time = sp->parameter_array[1];
			exposure_period = sp->parameter_array[2];
			strlcpy( trigger_mode, "exts", sizeof(trigger_mode) );
			break;
		case MXT_SQ_STROBE:
			num_frames = mx_round( sp->parameter_array[0] );
			exposure_time = -1.0;
			exposure_period = -1.0;
			strlcpy( trigger_mode, "exte", sizeof(trigger_mode) );
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"MX sequence type %ld is not supported for external "
			"trigger mode by area detector '%s'.",
				sp->sequence_type, ad->record->name );
			break;
		}
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported trigger mode %ld requested for "
		"area detector '%s'.  The supported modes are internal (1) "
		"and external (2).", ad->trigger_mode, ad->record->name );

		break;
	}

	dimension[0] = strlen(trigger_mode) + 1;

	mx_status = mxd_eiger_put_value( ad, eiger,
			"detector", "config/trigger_mode",
			MXFT_STRING, 1, dimension, trigger_mode, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the frame time. */

	dimension[0] = 1;

	mx_status = mxd_eiger_put_value( ad, eiger,
			"detector", "config/frame_time",
			MXFT_DOUBLE, 1, dimension,
			(void *) &exposure_period, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the exposure time. */

	dimension[0] = 1;

	mx_status = mxd_eiger_put_value( ad, eiger,
			"detector", "config/count_time",
			MXFT_DOUBLE, 1, dimension,
			(void *) &exposure_time, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the number of image frames. */

	dimension[0] = 1;

	mx_status = mxd_eiger_put_value( ad, eiger,
			"detector", "config/nimages",
			MXFT_ULONG, 1, dimension,
			(void *) &num_frames, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Arm the detector. */

	arm = 1;

	dimension[0] = 1;

	mx_status = mxd_eiger_put_value( ad, eiger,
			"detector", "command/arm",
			MXFT_ULONG, 1, dimension, (void *) &arm,
			arm_response, sizeof(arm_response) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The EIGER should have sent us a sequence number at the end
	 * of the HTTP response.
	 */

	num_items = sscanf( arm_response, "{\"sequence id\": %ld}",
						&image_frame_sequence_id );

	if ( num_items != 1 ) {
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"The EIGER detector '%s' did not include the image frame "
		"sequence id in its response '%s' to an 'arm' command.",
			ad->record->name, arm_response );
	}

#if 1
	MX_DEBUG(-2,("%s: image_frame_sequence_id = %ld",
		fname, image_frame_sequence_id));
#endif

	eiger->image_frame_sequence_id = image_frame_sequence_id;

	ad->total_num_frames = 0;
	ad->last_frame_number = -1;

	/* Note: If we are in external trigger mode ("exts"), then the
	 * detector will start taking images once the external trigger
	 * arrives.
	 */

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_eiger_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_eiger_trigger()";

	MX_EIGER *eiger = NULL;
	long trigger;
	long dimension[1];
	mx_status_type mx_status;

	eiger = NULL;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif
	/* If we are in external trigger mode, then explicit EIGER
	 * 'trigger' commands should not be sent.
	 */

	if ( ad->trigger_mode == MXF_DEV_EXTERNAL_TRIGGER ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Otherwise, send the trigger command. */

	dimension[0] = 1;

	trigger = 1;

	mx_status = mxd_eiger_put_value( ad, eiger,
			"detector", "command/trigger",
			MXFT_LONG, 1, dimension, (void *) &trigger, NULL, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_eiger_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_eiger_stop()";

	MX_EIGER *eiger = NULL;
	long cancel;
	long dimension[0];
	mx_status_type mx_status;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	dimension[0] = 1;

	cancel = 1;

	mx_status = mxd_eiger_put_value( ad, eiger,
			"detector", "command/cancel",
			MXFT_LONG, 1, dimension, (void *) &cancel, NULL, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_eiger_abort( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_eiger_abort()";

	MX_EIGER *eiger = NULL;
	long abort;
	long dimension[0];
	mx_status_type mx_status;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	dimension[0] = 1;

	abort = 1;

	mx_status = mxd_eiger_put_value( ad, eiger,
			"detector", "command/abort",
			MXFT_LONG, 1, dimension, (void *) &abort, NULL, 0 );

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
	char status_string[80];
	long dimension[1];
	long status_update;
	mx_status_type mx_status;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG
	MX_DEBUG( 2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	/* Tell the detector to update the detector status. */

	dimension[0] = 1;

	status_update = 1;

	mx_status = mxd_eiger_put_value( ad, eiger,
			"detector", "command/status_update",
			MXFT_LONG, 1, dimension,
			(void *) &status_update, NULL, 0 );

	/* Now get the updated status. */

	dimension[0] = sizeof(status_string);

	mx_status = mxd_eiger_get_value( ad, eiger,
			"detector", "status/state",
			MXFT_STRING, 1, dimension,
			status_string );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 1
	MX_DEBUG(-2,("%s: detector '%s', STATUS = '%s'",
		fname, ad->record->name, status_string));
#endif

	if ( strcmp( status_string, "idle" ) == 0 ) {
		ad->status = 0;
	} else {
		ad->status = 1;
	}

#if 1
	MX_DEBUG(-2,("%s: detector '%s', ad->status = %#lx",
		fname, ad->record->name, ad->status));
#endif

	return MX_SUCCESSFUL_RESULT;
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

	if ( eiger->transfer_mode & MXF_EIGER_TRANSFER_MONITOR ) {
		MX_DEBUG(-2,
	  ("%s: Transferring frame %ld via 'monitor' for EIGER detector '%s'.",
	   	fname, ad->transfer_frame, ad->record->name ));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_eiger_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_eiger_get_parameter()";

	MX_EIGER *eiger = NULL;
	long dimension[1];
	double resolution_in_meters;
	mx_status_type mx_status;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG_PARAMETERS
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

	dimension[0] = 1;

	switch( ad->parameter_type ) {
	case MXLV_AD_MAXIMUM_FRAME_NUMBER:
		break;

	case MXLV_AD_MAXIMUM_FRAMESIZE:
	case MXLV_AD_FRAMESIZE:
		mx_status = mxd_eiger_get_value( ad, eiger,
				"detector", "config/x_pixels_in_detector",
				MXFT_ULONG, 1, dimension,
				(void *) &(ad->framesize[0]) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_eiger_get_value( ad, eiger,
				"detector", "config/y_pixels_in_detector",
				MXFT_ULONG, 1, dimension,
				(void *) &(ad->framesize[1]) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ad->maximum_framesize[0] = ad->framesize[0];
		ad->maximum_framesize[1] = ad->framesize[1];
		break;

	case MXLV_AD_BINSIZE:
		/* Eiger detectors do not do binning as far as I know. */

		ad->binsize[0] = 1;
		ad->binsize[1] = 1;
		break;

	case MXLV_AD_RESOLUTION:
		resolution_in_meters = 0;

		mx_status = mxd_eiger_get_value( ad, eiger,
				"detector", "config/x_pixel_size",
				MXFT_DOUBLE, 1, dimension,
				(void *) &resolution_in_meters );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ad->resolution[0] = 1000.0 * resolution_in_meters;

		mx_status = mxd_eiger_get_value( ad, eiger,
				"detector", "config/y_pixel_size",
				MXFT_DOUBLE, 1, dimension,
				(void *) &resolution_in_meters );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ad->resolution[1] = 1000.0 * resolution_in_meters;
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
		mx_status = mx_process_record_field_by_name( ad->record,
						"bit_depth_image",
						NULL, MX_PROCESS_GET, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ad->bytes_per_pixel =
			mx_round_up( eiger->bit_depth_image / 8L );
		break;

	case MXLV_AD_BITS_PER_PIXEL:
		mx_status = mx_process_record_field_by_name( ad->record,
						"bit_depth_readout",
						NULL, MX_PROCESS_GET, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ad->bits_per_pixel = eiger->bit_depth_readout;
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

#if MXD_EIGER_DEBUG_PARAMETERS
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
		case MXLV_EIGER_BIT_DEPTH_IMAGE:
		case MXLV_EIGER_BIT_DEPTH_READOUT:
		case MXLV_EIGER_DCU_BUFFER_FREE:
		case MXLV_EIGER_ERROR:
		case MXLV_EIGER_HUMIDITY:
		case MXLV_EIGER_KEY_NAME:
		case MXLV_EIGER_KEY_RESPONSE:
		case MXLV_EIGER_KEY_VALUE:
		case MXLV_EIGER_MONITOR_ENABLED:
		case MXLV_EIGER_STATE:
		case MXLV_EIGER_STREAM_ENABLED:
		case MXLV_EIGER_TIME:
		case MXLV_EIGER_TEMPERATURE:
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
				void *socket_handler_ptr,
				int operation )
{
	static const char fname[] = "mxd_eiger_process_function()";

	MX_RECORD *record = NULL;
	MX_RECORD_FIELD *record_field = NULL;
	MX_AREA_DETECTOR *ad = NULL;
	MX_EIGER *eiger = NULL;
	long dimension[1];
	char *ptr_first_slash = NULL;
	char *ptr_second_slash = NULL;
	char *temp_key_name = NULL;
	size_t temp_key_length;
	char local_string_buffer[80];
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	ad = (MX_AREA_DETECTOR *) record->record_class_struct;
	eiger = (MX_EIGER *) record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	dimension[0] = 0;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_EIGER_MONITOR_ENABLED:
			dimension[0] = sizeof(local_string_buffer);

			mx_status = mxd_eiger_get_value( ad, eiger,
						"monitor", "config/mode",
						MXFT_STRING, 1, dimension,
						local_string_buffer );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( strcmp( local_string_buffer, "enabled" ) == 0 ) {
				eiger->monitor_enabled = TRUE;
			} else
			if ( strcmp( local_string_buffer, "disabled" ) == 0 ) {
				eiger->monitor_enabled = FALSE;
			} else {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"The value '%s' returned by EIGER detector "
				"'%s' for 'monitor/config/mode' was not "
				"one of the two expected values: 'enabled' "
				"or 'disabled'.", local_string_buffer,
					eiger->record->name );
			}
			break;
		case MXLV_EIGER_STREAM_ENABLED:
			dimension[0] = sizeof(local_string_buffer);

			mx_status = mxd_eiger_get_value( ad, eiger,
						"stream", "config/mode",
						MXFT_STRING, 1, dimension,
						local_string_buffer );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( strcmp( local_string_buffer, "enabled" ) == 0 ) {
				eiger->stream_enabled = TRUE;
			} else
			if ( strcmp( local_string_buffer, "disabled" ) == 0 ) {
				eiger->stream_enabled = FALSE;
			} else {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"The value '%s' returned by EIGER detector "
				"'%s' for 'stream/config/mode' was not "
				"one of the two expected values: 'enabled' "
				"or 'disabled'.", local_string_buffer,
					eiger->record->name );
			}
			break;
		case MXLV_EIGER_BIT_DEPTH_IMAGE:
			dimension[0] = 1;

			mx_status = mxd_eiger_get_value( ad, eiger,
					"detector", "config/bit_depth_image",
					MXFT_LONG, 1, dimension,
					(void *) &(eiger->bit_depth_image) );
			break;
		case MXLV_EIGER_BIT_DEPTH_READOUT:
			dimension[0] = 1;

			mx_status = mxd_eiger_get_value( ad, eiger,
					"detector", "config/bit_depth_readout",
					MXFT_LONG, 1, dimension,
					(void *) &(eiger->bit_depth_readout) );
			break;
		case MXLV_EIGER_STATE:
			dimension[0] = sizeof(eiger->state);

			mx_status = mxd_eiger_get_value( ad, eiger,
					"detector", "status/state",
					MXFT_STRING, 1, dimension,
					eiger->state );
			break;
		case MXLV_EIGER_ERROR:
			dimension[0] = sizeof(eiger->error);

			mx_status = mxd_eiger_get( ad, eiger,
					"detector", "status/error",
					eiger->error, sizeof(eiger->error) );
			break;
		case MXLV_EIGER_TIME:
			dimension[0] = sizeof(local_string_buffer);

			mx_status = mxd_eiger_get_value( ad, eiger,
					"detector", "status/time",
					MXFT_STRING, 1, dimension,
					local_string_buffer );

			MX_DEBUG(-2,("%s: local_string_buffer = '%s'",
				fname, local_string_buffer));
			break;
		case MXLV_EIGER_TEMPERATURE:
			dimension[0] = 1;

			mx_status = mxd_eiger_get_value( ad, eiger,
				"detector", "status/board_000/th0_temp",
					MXFT_DOUBLE, 1, dimension,
					(void *) &(eiger->temperature) );
			break;
		case MXLV_EIGER_HUMIDITY:
			dimension[0] = 1;

			mx_status = mxd_eiger_get_value( ad, eiger,
				"detector", "status/board_000/th0_humidity",
					MXFT_DOUBLE, 1, dimension,
					(void *) &(eiger->humidity) );
			break;
		case MXLV_EIGER_DCU_BUFFER_FREE:
			dimension[0] = 1;

			mx_status = mxd_eiger_get_value( ad, eiger,
				"detector", "status/builder/dcu_buffer_free",
					MXFT_DOUBLE, 1, dimension,
					(void *) &(eiger->dcu_buffer_free) );
			break;
		case MXLV_EIGER_KEY_NAME:
			break;
		case MXLV_EIGER_KEY_VALUE:
			dimension[0] = sizeof(eiger->key_value);

			mx_status = mxd_eiger_get_value( ad, eiger,
						eiger->module_name,
						eiger->key_name,
						MXFT_STRING, 1, dimension,
						eiger->key_value );
			break;
		case MXLV_EIGER_KEY_RESPONSE:
			dimension[0] = sizeof(eiger->key_response);

			mx_status = mxd_eiger_get( ad, eiger,
						eiger->module_name,
						eiger->key_name,
						eiger->key_response,
						sizeof(eiger->key_response) );
			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_EIGER_MONITOR_ENABLED:
			if ( eiger->monitor_enabled ) {
				strlcpy( eiger->key_value, "enabled",
					sizeof( eiger->key_value ) );
			} else {
				strlcpy( eiger->key_value, "disabled",
					sizeof( eiger->key_value ) );
			}

			dimension[0] = strlen( eiger->key_value ) + 1;

			mx_status = mxd_eiger_put_value( ad, eiger,
						"monitor", "config/mode",
						MXFT_STRING, 1, dimension,
						eiger->key_value, NULL, 0 );
			break;
		case MXLV_EIGER_KEY_NAME:
			/* If the key name has zero slashes in it, then
			 * it is an invalid key name.
			 */

			ptr_first_slash = strchr( eiger->key_name, '/' );

			if ( ptr_first_slash == (char *) NULL ) {
				mx_status = mx_error(MXE_ILLEGAL_ARGUMENT,fname,
				"The requested EIGER key name '%s' for "
				"area detector '%s' does not have any '/' "
				"characters in it, so it cannot "
				"be a valid EIGER key name.",
					eiger->key_name,
					eiger->record->name );

				eiger->module_name[0] = '\0';
				eiger->key_name[0] = '\0';

				return mx_status;
			}

			/* If the key name has only one slash in it, then
			 * it is a valid key name and we leave it alone.
			 */

			ptr_second_slash = strchr( ptr_first_slash, '/' );

			if ( ptr_second_slash == (char *) NULL ) {
				return MX_SUCCESSFUL_RESULT;
			}

			/* If we get here, then the key name has two or more
			 * slashes in it.  We need to move the part before
			 * the first slash into the module name and leave
			 * the rest of the name in eiger->key_name.
			 */

			/* Split the user-supplied string into two strings. */

			*ptr_first_slash = '\0';

			strlcpy( eiger->module_name, eiger->key_name,
					sizeof( eiger->module_name ) );

			temp_key_name = ptr_first_slash + 1;

			temp_key_length = strlen( temp_key_name );

			/* We must use memmove() here and _NOT_ memcpy().*/

			memmove( eiger->key_name, temp_key_name,
					temp_key_length + 1 );
			break;
		case MXLV_EIGER_KEY_VALUE:
			dimension[0] = sizeof(eiger->key_value);

			mx_status = mxd_eiger_put_value( ad, eiger,
						eiger->module_name,
						eiger->key_name,
						MXFT_STRING,
						1, dimension,
						eiger->key_value,
						NULL, 0 );
			break;
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

