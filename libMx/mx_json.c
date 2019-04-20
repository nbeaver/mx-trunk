/*
 * Name:    mx_json.c
 *
 * Purpose: A thin wrapper around cJSON.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_array.h"
#include "mx_json.h"

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_json_initialize( void )
{

#if defined(OS_WIN32)
	{
		static const char fname[] = "mx_json_initialize()";

		cJSON_Hooks hooks;

		MX_DEBUG(-2,("%s: Initializing cJSON hooks.", fname));

		/* We want cJSON to use the same heap as MX. */

		hooks.malloc_fn = mx_win32_malloc;
		hooks.free_fn = mx_win32_free;

		cJSON_InitHooks( &hooks );

		MX_DEBUG(-2,("%s: cJSON hooks initialized.", fname));
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_json_parse( MX_JSON **json, char *json_string )
{
	static const char fname[] = "mx_json_parse()";

	cJSON *cjson = NULL;

	if ( json == (MX_JSON **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_JSON pointer passed was NULL." );
	}

	cjson = cJSON_Parse( json_string );

	if ( cjson == (cJSON *) NULL ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"The attempt to parse the JSON string failed.  "
		"cJSON error = '%s'.  Original JSON string = '%s'",
			cJSON_GetErrorPtr(), json_string );
	}

	*json = (MX_JSON *) malloc( sizeof(MX_JSON) );

	if ( *json == (MX_JSON *) NULL ) {
		cJSON_Delete( cjson );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an MX_JSON structure." );
	}

	(*json)->cjson = cjson;

#if 0
	MX_DEBUG(-2,
	("%s: cJSON = %p '%s'", fname, cjson, cJSON_Print(cjson) ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_json_delete( MX_JSON *json )
{
	if ( json == (MX_JSON *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	if ( json->cjson != (cJSON *) NULL ) {
		cJSON_Delete( json->cjson );
	}

	mx_free( json );

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_json_find_key( cJSON *starting_cjson_value,
		char *key_name,
		cJSON **found_cjson_value )
{
	static const char fname[] = "mx_json_find_key()";

	cJSON *current_cjson_value = NULL;

	if ( starting_cjson_value == (cJSON *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The cJSON starting pointer passed was NULL." );
	}
	if ( key_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The key_name pointer for starting cJSON pointer %p is NULL.",
			starting_cjson_value );
	}
	if ( found_cjson_value == (cJSON **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The found_cjson_value pointer for "
		"start cJSON pointer %p is NULL.",
			starting_cjson_value );
	}

	*found_cjson_value = NULL;

	/* We walk through the cJSON values until we find a value with
	 * a matching key.  cJSON values that do not have JSON key names
	 * are skipped over.
	 */

	current_cjson_value = starting_cjson_value;

	while (TRUE) {
	    if ( current_cjson_value->string != (char *) NULL ) {
		if ( strcmp( key_name, current_cjson_value->string ) == 0 ) {
			/* If we get here, we found the key. */

			*found_cjson_value = current_cjson_value;

			return MX_SUCCESSFUL_RESULT;
		}
	    }

	    if ( current_cjson_value->next != (cJSON *) NULL ) {
		/* The 'next' value is available. */

		current_cjson_value = current_cjson_value->next;
	    } else {
		/* If there is no 'next' value, but this is a JSON
		 * object value, then we need to check to see if
		 * there is a child value.
		 */
		if ( cJSON_IsObject( current_cjson_value ) ) {
		    if ( current_cjson_value->child != (cJSON *) NULL ) {
			/* Use the 'child' value instead of the 'next' value. */

			current_cjson_value = current_cjson_value->child;
		    } else {
			/* Both 'next' and 'child' are NULL. */

			break;	/* Exit the while() loop. */
		    }
		} else {
		    /* 'next' is NULL and this is not a JSON object value. */

		    break; /* Exit the while() loop. */
		}
	    }

	    if ( current_cjson_value == starting_cjson_value ) {
		break;		/* Exit the while() loop. */
	    }
	}

	/* If we get here, we did not find the key. */

	return mx_error( MXE_NOT_FOUND, fname,
		"JSON key '%s' was not found for starting cJSON value %p.",
			key_name, starting_cjson_value );;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_json_get_key( MX_JSON *json,
		char *key_name,
		long key_datatype,
		void *key_value,
		size_t max_key_value_bytes )
{
	static const char fname[] = "mx_json_get_key()";

	cJSON *found_cjson_value = NULL;
	size_t element_size;
	mx_bool_type native_longs_are_64bits;
	mx_status_type mx_status;

	/* Do lots of sanity checking. */

	if ( key_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The key_name pointer passed was NULL." );
	}
	if ( json == (MX_JSON *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_JSON pointer passed for key '%s' was NULL.",
			key_name );
	}
	if ( key_value == (void *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The key_value pointer passed for key '%s' was NULL.",
			key_name );
	}
	if ( max_key_value_bytes == 0 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The max_key_value_bytes passed for key '%s' is 0, "
		"which leaves no room to copy the key value to.",
			key_name );
	}

	if ( json->cjson == (cJSON *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The cJSON pointer for MX_JSON pointer %p is NULL.", json );
	}

	/* Find the requested key. */

	mx_status = mx_json_find_key( json->cjson,
				key_name, &found_cjson_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 1
	MX_DEBUG(-2,("%s: Found JSON key = '%s'.",
		fname, found_cjson_value->string ));
#endif

	/* We have found the key that we are looking for.  Now return
	 * the key value using the datatype that our caller requested.
	 */

	if ( MX_WORDSIZE >= 64 ) {
		native_longs_are_64bits = TRUE;
	} else {
		native_longs_are_64bits = FALSE;
	}

	element_size = mx_get_scalar_element_size(
					key_datatype, native_longs_are_64bits );

	if ( ( key_datatype != MXFT_STRING )
	  && ( element_size > max_key_value_bytes ) )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The element size (%ld) for JSON key '%s' "
		"is larger than the buffer size (%ld) that "
		"we were given to copy the key value into.",
			(long) element_size, key_name,
			(long) max_key_value_bytes );
	}

	switch ( key_datatype ) {
	case MXFT_STRING:
		strlcpy( key_value, found_cjson_value->valuestring,
					max_key_value_bytes );
		break;
	case MXFT_CHAR:
		*((char *) key_value ) = found_cjson_value->valueint;
		break;
	case MXFT_UCHAR:
		*((unsigned char *) key_value ) = found_cjson_value->valueint;
		break;
	case MXFT_SHORT:
		*((short *) key_value ) = found_cjson_value->valueint;
		break;
	case MXFT_USHORT:
		*((unsigned short *) key_value ) = found_cjson_value->valueint;
		break;
	case MXFT_BOOL:
		*((mx_bool_type *) key_value ) = found_cjson_value->valueint;
		break;
	case MXFT_LONG:
		*((long *) key_value ) = found_cjson_value->valueint;
		break;
	case MXFT_ULONG:
	case MXFT_HEX:
		*((unsigned long *) key_value ) = found_cjson_value->valueint;
		break;
	case MXFT_FLOAT:
		*((float *) key_value ) = found_cjson_value->valuedouble;
		break;
	case MXFT_DOUBLE:
		*((double *) key_value ) = found_cjson_value->valuedouble;
		break;
	case MXFT_INT64:
		*((int64_t *) key_value ) = found_cjson_value->valueint;
		break;
	case MXFT_UINT64:
		*((uint64_t *) key_value ) = found_cjson_value->valueint;
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The requested datatype %ld for JSON key '%s' "
		"does not match any of the datatypes that "
		"are supported by this function.",
			key_datatype, key_name );
		break;
	}

	/* We have found the key, so return to our caller. */

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

/* FIXME: Find a way to get rid of MXU_JSON_MAX_COMPATIBLE_ELEMENTS and
 * make the arrays below that use it adjustable in size in a thread safe
 * way.  Probably means using Thread Local Storage.  Is is worth it?
 */

#define MXU_JSON_MAX_COMPATIBLE_ELEMENTS	100

MX_EXPORT mx_status_type
mx_json_get_compatible_key( MX_JSON *json,
		char *key_name,
		long mx_datatype,
		long mx_json_datatype,
		void *key_value,
		size_t max_key_value_bytes )
{
	static const char fname[] = "mx_json_get_compatible_key()";

	/* FIXME: The following stuff is an attempt to make all of the
	 * datatype arrays share stack space.  It _ASSUMES_ that
	 * sizeof(double) is bigger than the size of the other data types.
	 * We declare the space using the double datatype first to try to
	 * make sure that memory alignment is OK.
	 */

	double double_array[MXU_JSON_MAX_COMPATIBLE_ELEMENTS];
	float *float_array = (float *) double_array;
	long *long_array = (long *) double_array;
	unsigned long *ulong_array = (unsigned long *) double_array;
	mx_bool_type *bool_array = (mx_bool_type *) double_array;

	mx_status_type mx_status;

	if ( mx_datatype != mx_json_datatype ) {
	    if ( max_key_value_bytes > sizeof(double_array) ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"max_key_value_types = %ld is bigger than the maximum = %ld",
			(long) max_key_value_bytes,
			(long) sizeof(double_array) );
	    }
	}

	switch( mx_json_datatype ) {
	case MXFT_STRING:
		switch( mx_datatype ) {
		case MXFT_STRING:
			mx_status = mx_json_get_key( json, key_name,
						mx_json_datatype,
						key_value,
						max_key_value_bytes );
			return mx_status;
			break;
		}
		break;
	case MXFT_BOOL:
		mx_status = mx_json_get_key( json, key_name,
					mx_json_datatype,
					bool_array,
					max_key_value_bytes );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( mx_datatype ) {
		case MXFT_STRING:
			snprintf( key_value, max_key_value_bytes, "%d",
					bool_array[0] );
			break;
		case MXFT_BOOL:
			*((mx_bool_type *) key_value) = bool_array[0];
			break;
		case MXFT_CHAR:
			*((char *) key_value) = bool_array[0];
			break;
		case MXFT_UCHAR:
			*((unsigned char *) key_value) = bool_array[0];
			break;
		case MXFT_SHORT:
			*((short *) key_value) = bool_array[0];
			break;
		case MXFT_USHORT:
			*((unsigned short *) key_value) = bool_array[0];
			break;
		case MXFT_LONG:
			*((long *) key_value) = bool_array[0];
			break;
		case MXFT_ULONG:
		case MXFT_HEX:
			*((unsigned long *) key_value) = bool_array[0];
			break;
		case MXFT_FLOAT:
			*((float *) key_value) = bool_array[0];
			break;
		case MXFT_DOUBLE:
			*((double *) key_value) = bool_array[0];
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported MX datatype %ld for EIGER key '%s'.",
				mx_datatype, key_name );
			break;
		}
		break;
	case MXFT_LONG:
		mx_status = mx_json_get_key( json, key_name,
					mx_json_datatype,
					long_array,
					max_key_value_bytes );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( mx_datatype ) {
		case MXFT_STRING:
			snprintf( key_value, max_key_value_bytes, "%ld",
					long_array[0] );
			break;
		case MXFT_BOOL:
			*((mx_bool_type *) key_value) = long_array[0];
			break;
		case MXFT_CHAR:
			*((char *) key_value) = long_array[0];
			break;
		case MXFT_UCHAR:
			*((unsigned char *) key_value) = long_array[0];
			break;
		case MXFT_SHORT:
			*((short *) key_value) = long_array[0];
			break;
		case MXFT_USHORT:
			*((unsigned short *) key_value) = long_array[0];
			break;
		case MXFT_LONG:
			*((long *) key_value) = long_array[0];
			break;
		case MXFT_ULONG:
		case MXFT_HEX:
			*((unsigned long *) key_value) = long_array[0];
			break;
		case MXFT_FLOAT:
			*((float *) key_value) = long_array[0];
			break;
		case MXFT_DOUBLE:
			*((double *) key_value) = long_array[0];
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported MX datatype %ld for EIGER key '%s'.",
				mx_datatype, key_name );
			break;
		}
		break;
	case MXFT_ULONG:
		mx_status = mx_json_get_key( json, key_name,
					mx_json_datatype,
					ulong_array,
					max_key_value_bytes );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( mx_datatype ) {
		case MXFT_STRING:
			snprintf( key_value, max_key_value_bytes, "%lu",
					ulong_array[0] );
			break;
		case MXFT_BOOL:
			*((mx_bool_type *) key_value) = ulong_array[0];
			break;
		case MXFT_CHAR:
			*((char *) key_value) = ulong_array[0];
			break;
		case MXFT_UCHAR:
			*((unsigned char *) key_value) = ulong_array[0];
			break;
		case MXFT_SHORT:
			*((short *) key_value) = ulong_array[0];
			break;
		case MXFT_USHORT:
			*((unsigned short *) key_value) = ulong_array[0];
			break;
		case MXFT_LONG:
			*((long *) key_value) = ulong_array[0];
			break;
		case MXFT_ULONG:
		case MXFT_HEX:
			*((unsigned long *) key_value) = ulong_array[0];
			break;
		case MXFT_FLOAT:
			*((float *) key_value) = ulong_array[0];
			break;
		case MXFT_DOUBLE:
			*((double *) key_value) = ulong_array[0];
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported MX datatype %ld for EIGER key '%s'.",
				mx_datatype, key_name );
			break;
		}
		break;
	case MXFT_FLOAT:
		mx_status = mx_json_get_key( json, key_name,
					mx_json_datatype,
					float_array,
					max_key_value_bytes );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( mx_datatype ) {
		case MXFT_STRING:
			snprintf( key_value, max_key_value_bytes, "%g",
					float_array[0] );
			break;
		case MXFT_BOOL:
			*((mx_bool_type *) key_value) = float_array[0];
			break;
		case MXFT_CHAR:
			*((char *) key_value) = float_array[0];
			break;
		case MXFT_UCHAR:
			*((unsigned char *) key_value) = float_array[0];
			break;
		case MXFT_SHORT:
			*((short *) key_value) = float_array[0];
			break;
		case MXFT_USHORT:
			*((unsigned short *) key_value) = float_array[0];
			break;
		case MXFT_LONG:
			*((long *) key_value) = float_array[0];
			break;
		case MXFT_ULONG:
		case MXFT_HEX:
			*((unsigned long *) key_value) = float_array[0];
			break;
		case MXFT_FLOAT:
			*((float *) key_value) = float_array[0];
			break;
		case MXFT_DOUBLE:
			*((double *) key_value) = float_array[0];
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported MX datatype %ld for EIGER key '%s'.",
				mx_datatype, key_name );
			break;
		}
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

