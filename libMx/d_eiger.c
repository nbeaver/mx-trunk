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

#define MXD_EIGER_DEBUG			FALSE

#define MXD_EIGER_DEBUG_OPEN		FALSE

#define MXD_EIGER_DEBUG_PARAMETERS	FALSE

#define MXD_EIGER_DEBUG_TRIGGER_THREAD	FALSE

#define MXD_EIGER_DEBUG_RESPONSE	FALSE

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
#include "mx_hrt_debug.h"
#include "mx_atomic.h"
#include "mx_bit.h"
#include "mx_socket.h"
#include "mx_array.h"
#include "mx_thread.h"
#include "mx_mutex.h"
#include "mx_condition_variable.h"
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
	mxd_eiger_resynchronize,
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
		MX_RECORD *url_record,
		char *module_name,
		char *key_name,
		char *response,
		size_t max_response_length )
{
	static const char fname[] = "mxd_eiger_get()";

	char command_url[200];
	unsigned long http_status_code;
	char content_type[80];
	int i, max_attempts;
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

	/* If a url_record is not specified, then use the default one. */

	if ( url_record == (MX_RECORD *) NULL ) {
		url_record = eiger->url_server_record;
	}
	if ( url_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Area detector '%s' does not have a URL server record "
		"specified for this call.", ad->record->name );
	}

	/*----*/

	snprintf( command_url, sizeof(command_url),
		"http://%s/%s/api/%s/%s",
		eiger->hostname, module_name,
		eiger->simplon_version, key_name );

	max_attempts = 3;

	for ( i = 0; i < max_attempts; i++ ) {

	    mx_status = mx_url_get( url_record,
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

	    if ( strcmp( content_type, "application/json" ) == 0 ) {
		/* The HTTP GET succeeded as expected, so return now. */

		return MX_SUCCESSFUL_RESULT;
	    } else
	    if ( strcmp( content_type, "text/plain; charset=utf-8" ) == 0 ) {
		/* The EIGER has probably just sent us an unexpected
		 * empty response message.  Display a warning and go
		 * back and try again.
		 */

		mx_warning( "Received an unexpected non-JSON response from "
		"URL '%s' sent to EIGER detector '%s'.  Response = '%s'.  "
		"Retrying...",
			command_url, ad->record->name, response );

		continue;
	    } else {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The HTTP content type '%s' returned for URL '%s' at "
		"EIGER detector '%s' was not 'application/json'.  "
		"The body of the response was '%s'.",
			content_type, command_url, eiger->record->name,
			response );
	    }
	}

	if ( i >= max_attempts ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"%d attempts to GET the url '%s' for EIGER detector '%s' "
		"have failed.", max_attempts, command_url, ad->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_eiger_get_value( MX_AREA_DETECTOR *ad,
			MX_EIGER *eiger,
			MX_RECORD *url_record,
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

	mx_status = mxd_eiger_get( ad, eiger, url_record,
			module_name, key_name,
			response, sizeof(response) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG_RESPONSE
	MX_DEBUG(-2,("%s: EIGER response = '%s'", fname, response ));
#endif

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

#if MXD_EIGER_DEBUG_RESPONSE
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
		MX_RECORD *url_record,
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

	/* If a url_record is not specified, then use the default one. */

	if ( url_record == (MX_RECORD *) NULL ) {
		url_record = eiger->url_server_record;
	}
	if ( url_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Area detector '%s' does not have a URL server record "
		"specified for this call.", ad->record->name );
	}

	/*----*/

	snprintf( command_url, sizeof(command_url),
		"http://%s/%s/api/%s/%s",
		eiger->hostname, module_name,
		eiger->simplon_version, key_name );

	mx_status = mx_url_put( url_record,
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
			MX_RECORD *url_record,
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

	char format[80];
	char key_value[MXU_EIGER_KEY_VALUE_LENGTH+1];
	char key_body[500];
	mx_bool_type bool_value;
	mx_status_type mx_status;

#if 0
	MX_DEBUG(-2,("%s invoked for detector '%s'.",
			fname, eiger->record->name));
#endif

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

	snprintf( format, sizeof(format),
		"{ \\\"value\\\" : %%%ds )\\\r\\\n",
			(int) sizeof(key_value) - 1 );

	MX_DEBUG(-2,("%s: format = '%s'", fname, format));

	snprintf( key_value, sizeof(key_value), format, key_body );

	MX_DEBUG(-2,("%s: key_value = '%s'", fname, key_value));

	mx_status = mxd_eiger_put( ad, eiger, url_record,
			module_name, key_name, key_value,
			response, max_response_length );

	return mx_status;
}

/*----------------------------------------------------------------*/

static mx_status_type
mxd_eiger_send_command_to_trigger_thread( MX_EIGER *eiger,
					int32_t trigger_command )
{
	static const char fname[] =
		"mxd_eiger_send_command_to_trigger_thread()";

	unsigned long mx_status_code;
	mx_status_type mx_status;

	if ( eiger == (MX_EIGER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EIGER pointer passed was NULL." );
	}

#if MXD_EIGER_DEBUG_TRIGGER_THREAD
	MX_DEBUG(-2,("%s: sending (%d) command to trigger thread.",
		fname, (int) trigger_command ));
#endif

	/* Prepare to tell the 'trigger' thread to start a command. */

	mx_status_code = mx_mutex_lock( eiger->trigger_thread_command_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock the trigger thread command mutex for "
		"area detector '%s' failed.", eiger->record->name );
	}

	eiger->trigger_command = trigger_command;

	/* Notify the trigger thread. */

	mx_status = mx_condition_variable_signal(
			eiger->trigger_thread_command_cv );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We are done sending our notification. */

	mx_status_code = mx_mutex_unlock(
				eiger->trigger_thread_command_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to unlock the trigger thread command mutex for "
		"area detector '%s' failed.",
			eiger->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----------------------------------------------------------------*/

#if 0
static mx_status_type
mxd_eiger_send_status_to_main_thread( MX_EIGER *eiger,
					int32_t trigger_status )
{
	static const char fname[] = "mxd_eiger_send_status_to_main_thread()";

	unsigned long mx_status_code;
	mx_status_type mx_status;

	if ( eiger == (MX_EIGER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EIGER pointer passed was NULL." );
	}

#if MXD_EIGER_DEBUG_TRIGGER_THREAD
	MX_DEBUG(-2,("%s: sending (%d) status to main thread.",
			fname, (int) trigger_status ));
#endif

	mx_status_code = mx_mutex_lock( eiger->trigger_thread_status_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock the trigger thread status mutex for "
		"area detector '%s' failed.",
			eiger->record->name );
	}

	eiger->trigger_status = trigger_status;

	/* Notify the main thread. */

	mx_status = mx_condition_variable_signal(
			eiger->trigger_thread_status_cv );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We are done sending our notification. */

	mx_status_code = mx_mutex_unlock( eiger->trigger_thread_status_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to unlock the trigger thread status mutex for "
		"area detector '%s' failed.",
			eiger->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}
#endif

/*----------------------------------------------------------------*/

/* The trigger thread is used to send trigger commands to the detector.
 * This is because the EIGER sends a response to the trigger command, but
 * not until the imaging sequence is over.
 */

static mx_status_type
mxd_eiger_trigger_thread_fn( MX_THREAD *thread, void *args )
{
	static const char fname[] = "mxd_eiger_trigger_thread_fn()";

	MX_RECORD *eiger_record = NULL;
	MX_AREA_DETECTOR *ad = NULL;
	MX_EIGER *eiger = NULL;
	unsigned long trigger_loop_counter;
	long dimension[1];
	uint32_t trigger;
	char response[800];
	unsigned long mx_status_code;
	mx_status_type mx_status;

	MX_HRT_TIMING trigger_measurement;

	/*----------------------------------------------------------------*/

	/* Initialize the variables to be used by this thread. */

	if ( thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	if ( args == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	eiger_record = (MX_RECORD *) args;

#if MXD_EIGER_DEBUG_TRIGGER_THREAD
	MX_DEBUG(-2,("%s invoked for EIGER detector '%s'.",
		fname, eiger_record->name ));
#endif

	ad = (MX_AREA_DETECTOR *) eiger_record->record_class_struct;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AREA_DETECTOR pointer for detector '%s' is NULL.",
			eiger_record->name );
	}

	eiger = (MX_EIGER *) eiger_record->record_type_struct;

	if ( eiger == (MX_EIGER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EIGER pointer for detector '%s' is NULL.",
			eiger_record->name );
	}

	/*----------------------------------------------------------------*/

	/* Tell the main thread that we have finished initializing ourself. */

	mx_status_code = mx_mutex_lock( eiger->trigger_thread_init_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock the trigger thread initialization mutex "
		"for EIGER detector failed." );
	}

	eiger->trigger_status = MXS_EIGER_STAT_IDLE;

	mx_status = mx_condition_variable_signal(
			eiger->trigger_thread_init_cv );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status_code = mx_mutex_unlock( eiger->trigger_thread_init_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to unlock the trigger thread initialization mutex "
		"for EIGER detector '%s' failed.",
			eiger->record->name );
	}

	/* NOTE: We should not attempt to use the initialization mutex and cv
	 * after this point, since the main thread may have deallocated them.
	 */

	/*  FIXME: Is there a possibility for a race condition here
	 *  between the release of trigger_thread_init_mutex and
	 *  the taking of trigger_thread_command_mutex?
	 */

	/*----------------------------------------------------------------*/

	/* Take ownership of the command mutex. */

	mx_status_code = mx_mutex_lock( eiger->trigger_thread_command_mutex );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock the trigger thread command mutex "
		"for EIGER detector '%s' failed.",
			eiger->record->name );
	}

	/*----------------------------------------------------------------*/

	trigger_loop_counter = 0;

#if MXD_EIGER_DEBUG_TRIGGER_THREAD
	MX_DEBUG(-2,("%s %p [%lu]: Area detector '%s' entering event loop.",
		fname, mx_get_current_thread_pointer(),
		trigger_loop_counter, eiger_record->name ));
#endif

	while ( TRUE ) {
		trigger_loop_counter++;

		/* Wait on the command condition variable. */

		while ( eiger->trigger_command == MXS_EIGER_CMD_NONE ) {

#if MXD_EIGER_DEBUG_TRIGGER_THREAD
			MX_DEBUG(-2,("%s: WAITING FOR COMMAND", fname));
#endif

			mx_status = mx_condition_variable_wait(
					eiger->trigger_thread_command_cv,
					eiger->trigger_thread_command_mutex );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

#if MXD_EIGER_DEBUG_TRIGGER_THREAD
		MX_DEBUG(-2,("%s %p [%lu]: '%s' command = %ld",
			fname, mx_get_current_thread_pointer(),
			trigger_loop_counter, eiger_record->name,
			(long) eiger->trigger_command));
#endif

		switch( eiger->trigger_command ) {
		case MXS_EIGER_CMD_NONE:
			/* No command was requested, so do not do anything. */
			break;
		case MXS_EIGER_CMD_TRIGGER:
			dimension[0] = 1;
			trigger = 1;

			MX_DEBUG(-2,("%s: TRIGGER requested.",fname));

			MX_HRT_START( trigger_measurement );

			mx_status = mxd_eiger_put( ad, eiger,
					eiger->url_trigger_record,
					"detector", "command/trigger",
					"{ \"value\" : 1 }",
					response, sizeof(response) );

			MX_HRT_END( trigger_measurement );
			MX_HRT_RESULTS( trigger_measurement, fname, "trigger" );

			MX_DEBUG(-2,("%s: TRIGGER complete.",fname));

#if 0
			mx_status = mxd_eiger_send_status_to_main_thread(
						eiger, MXS_EIGER_STAT_IDLE );
#endif
			break;
		default:
			(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported command %ld requested "
			"for area detector '%s' trigger thread.",
				(long) eiger->trigger_command,
				eiger_record->name );
			break;
		}

		eiger->trigger_command = MXS_EIGER_CMD_NONE;

#if MXD_EIGER_DEBUG_TRIGGER_THREAD
		MX_DEBUG(-2,("%s %p [%lu]: '%s' status = %ld",
			fname, mx_get_current_thread_pointer(),
			trigger_loop_counter, eiger_record->name,
			(long) eiger->trigger_status));
#endif
	}

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
	long mx_status_code;
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

	mx_status = mxd_eiger_get_value( ad, eiger, NULL,
					"detector", "config/description",
					MXFT_STRING, 1, dimension,
					eiger->description );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the detector software version. */

	dimension[0] = sizeof(eiger->software_version);

	mx_status = mxd_eiger_get_value( ad, eiger, NULL,
					"detector", "config/software_version",
					MXFT_STRING, 1, dimension,
					eiger->software_version );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the detector serial number. */

	dimension[0] = sizeof(eiger->detector_number);

	mx_status = mxd_eiger_get_value( ad, eiger, NULL,
					"detector", "config/detector_number",
					MXFT_STRING, 1, dimension,
					eiger->detector_number );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the image frame transfer mode. */

	eiger->transfer_mode = 0;

	if ( strcmp( eiger->transfer_mode_name, "monitor" ) == 0 ) {
		mx_status = mx_process_record_field_by_name( ad->record,
						"monitor_mode",
						NULL, MX_PROCESS_GET, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if 0
		MX_DEBUG(-2,("%s: eiger->monitor_mode = %d",
			fname, (int) eiger->monitor_mode));
#endif

		if ( eiger->monitor_mode ) {
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

#if 0
	MX_DEBUG(-2,("%s: eiger->transfer_mode = %#lx",
		fname, eiger->transfer_mode));
#endif

	/* Get the detector state. */

	dimension[0] = sizeof(response);

	mx_status = mxd_eiger_get_value( ad, eiger, NULL,
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

	/*---*/

	/* Configure the area detector driver to unconditionally report
	 * the detector as busy for a short time (in seconds) after a start.
	 */

	mx_status = mx_area_detector_set_busy_start_interval( record, 1.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*-----------------------------------------------------------------*/

	/* Trigger commands to the EIGER must be done in a separate thread
	 * since the EIGER does not send a response until the trigger
	 * sequence is complete.
	 */

	/* trigger_thread_init_mutex and trigger_thread_init_cv are used
	 * to verify that the trigger thread has initialized itself.
	 */

	mx_status = mx_mutex_create( &(eiger->trigger_thread_init_mutex) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_condition_variable_create(
				&(eiger->trigger_thread_init_cv) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* trigger_thread_command_mutex and trigger_thread_command_cv are used
	 * to send commands to the trigger thread.
	 */

	mx_status = mx_mutex_create( &(eiger->trigger_thread_command_mutex) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_condition_variable_create(
				&(eiger->trigger_thread_command_cv) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* trigger_thread_status_mutex and trigger_thread_status_cv are used
	 * to check the status of the trogger thread.
	 */

	mx_status = mx_mutex_create( &(eiger->trigger_thread_status_mutex) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_condition_variable_create(
				&(eiger->trigger_thread_status_cv) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the trigger thread status to not initialized. */

	eiger->trigger_command = MXS_EIGER_CMD_NONE;
	eiger->trigger_status  = MXS_EIGER_STAT_NOT_INITIALIZED;

	/* Prepare to wait for the notification that the trigger thread
	 * is initialized.
	 */

	mx_status_code = mx_mutex_lock( eiger->trigger_thread_init_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock the trigger init mutex before creating "
		"the trigger thread failed.  This means that internal "
		"triggering of the detector will be impossible." );
	}

	/* Now create the trigger thread. */

	mx_status = mx_thread_create( &(eiger->trigger_thread),
					mxd_eiger_trigger_thread_fn,
					record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the notification that the trigger thread is initialized. */

	while ( eiger->trigger_status == MXS_EIGER_STAT_NOT_INITIALIZED )
	{
		mx_status = mx_condition_variable_wait(
				eiger->trigger_thread_init_cv,
				eiger->trigger_thread_init_mutex );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* We now know that the trigger thread has finished initializing
	 * itself, so we unlock the initialization mutex.
	 */

	mx_status_code = mx_mutex_unlock( eiger->trigger_thread_init_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock the trigger init mutex before creating "
		"the trigger thread failed.  This means that internal "
		"triggering of the detector will be impossible." );
	}

#if MXD_EIGER_DEBUG_TRIGGER_THREAD
	MX_DEBUG(-2,("%s: Trigger thread is ready for use.", fname));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_eiger_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_eiger_resynchronize()";

	MX_AREA_DETECTOR *ad = NULL;
	MX_EIGER *eiger = NULL;
	long dimension[1];
	long restart, initialize;
	mx_status_type mx_status;

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for detector '%s'", fname, record->name ));

	/* Restart the SIMPLON API service. */

	dimension[0] = 1;
	restart = 1;

	mx_status = mxd_eiger_put_value( ad, eiger, NULL,
					"system", "command/restart",
					MXFT_LONG, 1, dimension,
					(void *) &restart, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now reinitialize the detector. */

	dimension[0] = 1;
	initialize = 1;

	mx_status = mxd_eiger_put_value( ad, eiger, NULL,
					"detector", "command/initialize",
					MXFT_LONG, 1, dimension,
					(void *) &initialize, NULL, 0 );

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
	long sequence_id;
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

	mx_status = mxd_eiger_put_value( ad, eiger, NULL,
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
		case MXT_SQ_STROBE:
			num_frames = mx_round( sp->parameter_array[0] );
			exposure_time = -1.0;
			exposure_period = -1.0;
			strlcpy( trigger_mode, "inte", sizeof(trigger_mode) );
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

	/* We don't know right now when an externally triggered sequence
	 * will be finished, so we initialize it to the largest possible
	 * value.  If we are really using internal triggers 'ints', then
	 * this will get updated in the trigger function.
	 */

	eiger->expected_finish_tick = mx_set_clock_tick_to_maximum();

	/*---*/

	dimension[0] = strlen(trigger_mode) + 1;

	mx_status = mxd_eiger_put_value( ad, eiger, NULL,
			"detector", "config/trigger_mode",
			MXFT_STRING, 1, dimension, trigger_mode, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the frame time. */

	dimension[0] = 1;

	mx_status = mxd_eiger_put_value( ad, eiger, NULL,
			"detector", "config/frame_time",
			MXFT_DOUBLE, 1, dimension,
			(void *) &exposure_period, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the exposure time. */

	dimension[0] = 1;

	mx_status = mxd_eiger_put_value( ad, eiger, NULL,
			"detector", "config/count_time",
			MXFT_DOUBLE, 1, dimension,
			(void *) &exposure_time, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the number of image frames. */

	dimension[0] = 1;

	mx_status = mxd_eiger_put_value( ad, eiger, NULL,
			"detector", "config/nimages",
			MXFT_ULONG, 1, dimension,
			(void *) &num_frames, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Arm the detector. */

	arm = 1;
	dimension[0] = 1;

#if 1
	{
		mx_status = mxd_eiger_get_status( ad );

		MX_DEBUG(-2,("%s: ARM prearm STATE = '%s'",
			fname, eiger->state));
	}
#endif

	mx_status = mxd_eiger_put_value( ad, eiger, NULL,
			"detector", "command/arm",
			MXFT_ULONG, 1, dimension, (void *) &arm,
			arm_response, sizeof(arm_response) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The EIGER should have sent us a sequence number at the end
	 * of the HTTP response.
	 */

	num_items = sscanf( arm_response, "{\"sequence id\": %ld}",
						&sequence_id );

	if ( num_items != 1 ) {
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"The EIGER detector '%s' did not include the image frame "
		"sequence id in its response '%s' to an 'arm' command.",
			ad->record->name, arm_response );
	}

#if 1
	MX_DEBUG(-2,("%s: sequence_id = %ld", fname, sequence_id));
#endif

	eiger->sequence_id = sequence_id;

	ad->total_num_frames = 0;
	ad->last_frame_number = -1;

#if 1
	{
		mx_status = mxd_eiger_get_status( ad );

		MX_DEBUG(-2,("%s: ARM postarm STATE = '%s'",
			fname, eiger->state));
	}
#endif

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
	MX_SEQUENCE_PARAMETERS *sp = NULL;
	double total_sequence_time_in_seconds;
	double sequence_time_cutoff_in_seconds;
	MX_CLOCK_TICK sequence_time_cutoff_in_ticks;
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

	/* Compute a time that the sequence should be finished by. */

	mx_status = mx_area_detector_get_total_sequence_time( ad->record,
					&total_sequence_time_in_seconds );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sp = &(ad->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_MULTIFRAME:
		sequence_time_cutoff_in_seconds = ad->busy_start_interval
				+ ( 1.1 * total_sequence_time_in_seconds );

		sequence_time_cutoff_in_ticks =
	   mx_convert_seconds_to_clock_ticks( sequence_time_cutoff_in_seconds );

		eiger->expected_finish_tick =
			mx_add_clock_ticks( mx_current_clock_tick(),
					sequence_time_cutoff_in_ticks );
		break;
	case MXT_SQ_STROBE:
		/* We don't really know how long a strobe sequence will
		 * _really_ be, so we just set the cutoff time to the
		 * maximum.
		 */

		eiger->expected_finish_tick = mx_set_clock_tick_to_maximum();
		break;
	}
	/* Now send the trigger command. */

	mx_status = mxd_eiger_send_command_to_trigger_thread(
					eiger, MXS_EIGER_CMD_TRIGGER );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_eiger_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_eiger_stop()";

	MX_EIGER *eiger = NULL;
	long disarm;
	long dimension[1];
	mx_status_type mx_status;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	dimension[0] = 1;

	disarm = 1;

	mx_status = mxd_eiger_put_value( ad, eiger, NULL,
			"detector", "command/disarm",
			MXFT_LONG, 1, dimension, (void *) &disarm, NULL, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_eiger_abort( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_eiger_abort()";

	MX_EIGER *eiger = NULL;
	long abort_cmd;
	long dimension[1];
	mx_status_type mx_status;

	mx_status = mxd_eiger_get_pointers( ad, &eiger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EIGER_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	dimension[0] = 1;

	abort_cmd = 1;

	mx_status = mxd_eiger_put_value( ad, eiger, NULL,
			"detector", "command/abort",
			MXFT_LONG, 1, dimension, (void *) &abort_cmd, NULL, 0 );

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
	int comparison;
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

	mx_status = mxd_eiger_put_value( ad, eiger, NULL,
			"detector", "command/status_update",
			MXFT_LONG, 1, dimension,
			(void *) &status_update, NULL, 0 );

	/* Now get the updated status. */

	dimension[0] = sizeof(status_string);

	mx_status = mxd_eiger_get_value( ad, eiger, NULL,
			"detector", "status/state",
			MXFT_STRING, 1, dimension,
			status_string );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 1
	MX_DEBUG(-2,("%s: detector '%s', STATE = '%s'",
		fname, ad->record->name, status_string));
#endif

	/* Parse the status word to see what the current state of
	 * the detector is.
	 */

	strlcpy( eiger->state, status_string, sizeof(eiger->state) );

	if ( strcmp( status_string, "idle" ) == 0 ) {
		ad->status = 0;
	} else
	if ( strcmp( status_string, "acquire" ) == 0 ) {
		ad->status = MXSF_AD_ACQUISITION_IN_PROGRESS;
	} else
	if ( strcmp( status_string, "ready" ) == 0 ) {
		ad->status = MXSF_AD_ARMED;
	} else
	if ( strcmp( status_string, "initialize" ) == 0 ) {
		ad->status = MXSF_AD_CONTROLLER_ACTION_IN_PROGRESS;
	} else
	if ( strcmp( status_string, "configure" ) == 0 ) {
		ad->status = MXSF_AD_CONTROLLER_ACTION_IN_PROGRESS;
	} else
	if ( strcmp( status_string, "test" ) == 0 ) {
		ad->status = MXSF_AD_CONTROLLER_ACTION_IN_PROGRESS;
	} else
	if ( strcmp( status_string, "error" ) == 0 ) {
		ad->status = MXSF_AD_ERROR;
	} else
	if ( strcmp( status_string, "na" ) == 0 ) {
		ad->status = MXSF_AD_ERROR;
	} else {
		mx_warning( "EIGER detector '%s' reports an unrecognized "
				"detector status '%s'.  The recognized "
				"status values are 'idle', 'acquire', 'ready', "
				"'initialize', 'configure', 'test', 'error', "
				"and 'na'", ad->record->name, status_string );

		ad->status = MXSF_AD_ERROR;
	}

	if ( ad->status & MXSF_AD_IS_BUSY ) {
	    if ( mx_clock_tick_is_zero( eiger->expected_finish_tick ) == FALSE )
	    {
		comparison = mx_compare_clock_ticks(
					eiger->expected_finish_tick,
					mx_current_clock_tick() );

		if ( comparison >= 0 ) {
			mx_warning( "Stopping detector '%s' due to the "
				"arrival of the expected finish time.",
				ad->record->name );

			mx_status = mxd_eiger_stop( ad );
		}
	    }
	}

#if 0
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
		mx_status = mxd_eiger_get_value( ad, eiger, NULL,
				"detector", "config/x_pixels_in_detector",
				MXFT_ULONG, 1, dimension,
				(void *) &(ad->framesize[0]) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_eiger_get_value( ad, eiger, NULL,
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

		mx_status = mxd_eiger_get_value( ad, eiger, NULL,
				"detector", "config/x_pixel_size",
				MXFT_DOUBLE, 1, dimension,
				(void *) &resolution_in_meters );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ad->resolution[0] = 1000.0 * resolution_in_meters;

		mx_status = mxd_eiger_get_value( ad, eiger, NULL,
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
		ad->total_sequence_time = 0;
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
		case MXLV_EIGER_MONITOR_MODE:
		case MXLV_EIGER_STATE:
		case MXLV_EIGER_STREAM_MODE:
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
		case MXLV_EIGER_MONITOR_MODE:
			dimension[0] = sizeof(local_string_buffer);

			mx_status = mxd_eiger_get_value( ad, eiger, NULL,
						"monitor", "config/mode",
						MXFT_STRING, 1, dimension,
						local_string_buffer );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( strcmp( local_string_buffer, "enabled" ) == 0 ) {
				eiger->monitor_mode = TRUE;
			} else
			if ( strcmp( local_string_buffer, "disabled" ) == 0 ) {
				eiger->monitor_mode = FALSE;
			} else {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"The value '%s' returned by EIGER detector "
				"'%s' for 'monitor/config/mode' was not "
				"one of the two expected values: 'enabled' "
				"or 'disabled'.", local_string_buffer,
					eiger->record->name );
			}
			break;
		case MXLV_EIGER_STREAM_MODE:
			dimension[0] = sizeof(local_string_buffer);

			mx_status = mxd_eiger_get_value( ad, eiger, NULL,
						"stream", "config/mode",
						MXFT_STRING, 1, dimension,
						local_string_buffer );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( strcmp( local_string_buffer, "enabled" ) == 0 ) {
				eiger->stream_mode = TRUE;
			} else
			if ( strcmp( local_string_buffer, "disabled" ) == 0 ) {
				eiger->stream_mode = FALSE;
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

			mx_status = mxd_eiger_get_value( ad, eiger, NULL,
					"detector", "config/bit_depth_image",
					MXFT_LONG, 1, dimension,
					(void *) &(eiger->bit_depth_image) );
			break;
		case MXLV_EIGER_BIT_DEPTH_READOUT:
			dimension[0] = 1;

			mx_status = mxd_eiger_get_value( ad, eiger, NULL,
					"detector", "config/bit_depth_readout",
					MXFT_LONG, 1, dimension,
					(void *) &(eiger->bit_depth_readout) );
			break;
		case MXLV_EIGER_STATE:
			dimension[0] = sizeof(eiger->state);

			mx_status = mxd_eiger_get_value( ad, eiger, NULL,
					"detector", "status/state",
					MXFT_STRING, 1, dimension,
					eiger->state );
			break;
		case MXLV_EIGER_ERROR:
			dimension[0] = sizeof(eiger->error);

			mx_status = mxd_eiger_get( ad, eiger, NULL,
					"detector", "status/error",
					eiger->error, sizeof(eiger->error) );
			break;
		case MXLV_EIGER_TIME:
			dimension[0] = sizeof(local_string_buffer);

			mx_status = mxd_eiger_get_value( ad, eiger, NULL,
					"detector", "status/time",
					MXFT_STRING, 1, dimension,
					local_string_buffer );

			MX_DEBUG(-2,("%s: local_string_buffer = '%s'",
				fname, local_string_buffer));
			break;
		case MXLV_EIGER_TEMPERATURE:
			dimension[0] = 1;

			mx_status = mxd_eiger_get_value( ad, eiger, NULL,
				"detector", "status/board_000/th0_temp",
					MXFT_DOUBLE, 1, dimension,
					(void *) &(eiger->temperature) );
			break;
		case MXLV_EIGER_HUMIDITY:
			dimension[0] = 1;

			mx_status = mxd_eiger_get_value( ad, eiger, NULL,
				"detector", "status/board_000/th0_humidity",
					MXFT_DOUBLE, 1, dimension,
					(void *) &(eiger->humidity) );
			break;
		case MXLV_EIGER_DCU_BUFFER_FREE:
			dimension[0] = 1;

			mx_status = mxd_eiger_get_value( ad, eiger, NULL,
				"detector", "status/builder/dcu_buffer_free",
					MXFT_DOUBLE, 1, dimension,
					(void *) &(eiger->dcu_buffer_free) );
			break;
		case MXLV_EIGER_KEY_NAME:
			break;
		case MXLV_EIGER_KEY_VALUE:
			dimension[0] = sizeof(eiger->key_value);

			mx_status = mxd_eiger_get_value( ad, eiger, NULL,
						eiger->module_name,
						eiger->key_name,
						MXFT_STRING, 1, dimension,
						eiger->key_value );
			break;
		case MXLV_EIGER_KEY_RESPONSE:
			dimension[0] = sizeof(eiger->key_response);

			mx_status = mxd_eiger_get( ad, eiger, NULL,
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
		case MXLV_EIGER_MONITOR_MODE:
			if ( eiger->monitor_mode ) {
				strlcpy( eiger->key_value, "enabled",
					sizeof( eiger->key_value ) );
			} else {
				strlcpy( eiger->key_value, "disabled",
					sizeof( eiger->key_value ) );
			}

			dimension[0] = strlen( eiger->key_value ) + 1;

			mx_status = mxd_eiger_put_value( ad, eiger, NULL,
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

			mx_status = mxd_eiger_put_value( ad, eiger, NULL,
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

