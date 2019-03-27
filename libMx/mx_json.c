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

/* FIXME: As we do more work with cJSON, we will probably find that the
 * following mx_json_get_key() function is too simplistic.  It may be too
 * tied to the way that the EIGER detector's JSON is structured.
 *
 * At the moment, what we are doing is the following (2019-03-22) (WML).
 *
 * 1.  Check to see if the top level element is a cJSON object.  If it is,
 *     then start with the object's child.  Otherwise, we use the top
 *     level element as is.
 * 2.  Step through the various cJSON objects going from next to next,
 *     looking for one that has a 'string' element whose name matches
 *     the key we are looking for.
 * 3.  If *datatype matches a known MX type, then try to convert one of
 *     the valuestring, valueint, or valuedouble into that datatype and
 *     assign the new value to the 'value' item of the function signature.
 * 4.  If *datatype is <= 0, then we try to guess(?) the MX datatype,
 *     assign that datatype to the 'datatype' item of the function signature
 *     and then progress like in step 3 above.
 */

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_json_get_key( MX_JSON *json,
		char *key_name,
		long key_datatype,
		void *key_value,
		size_t max_key_value_bytes )
{
	static const char fname[] = "mx_json_get_key()";

	cJSON *returned_cjson_ptr = NULL;
	cJSON *first_cjson_element = NULL;
	cJSON *current_cjson_element = NULL;

	size_t element_size;
	mx_bool_type native_longs_are_64bits;

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

	returned_cjson_ptr = json->cjson;

	if ( returned_cjson_ptr == (cJSON *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The cJSON pointer for MX JSON object %p is NULL for key '%s'.",
			json, key_name );
	}

	/* Is the cJSON element 'returned_cjson_ptr' a cJSON object that
	 * points to the first element that we want to look at, or is it
	 * the first element itself?
	 */

	if ( cJSON_IsObject( returned_cjson_ptr ) ) {
		if ( returned_cjson_ptr->child == (struct cJSON *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The top level cJSON object %p for key '%s' "
			"of MX_JSON structure %p has a child pointer that "
			"is NULL.  This means that we have nowhere to look "
			"for the key data that we want, which leaves us "
			"unable to proceed.",
				returned_cjson_ptr, key_name, json );
		}

		first_cjson_element = returned_cjson_ptr->child;
	} else {
		first_cjson_element = returned_cjson_ptr;

		mx_warning( "returned_cjson_ptr %p for MX_JSON structure %p "
			"is not a cJSON object that points to the first "
			"cJSON element that we want to look at.  We are "
			"optimistically continuing in the hope that "
			"returned_cjson_ptr is _itself_ the first element "
			"that we want.", returned_cjson_ptr, json );
	}

	/* Now we can walk through the list of keys looking for 
	 * the key that we want.
	 */

	current_cjson_element = first_cjson_element;

	while (1) {
		if ( strcmp( key_name, current_cjson_element->string ) == 0 ) {
			/* We have found the key that we are looking for.
			 * Now return the key value using the datatype
			 * that our caller requested.
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
					element_size, key_name,
					max_key_value_bytes );
			}

			switch ( key_datatype ) {
			case MXFT_STRING:
				strlcpy( key_value,
					current_cjson_element->valuestring,
					max_key_value_bytes );
				break;
			case MXFT_CHAR:
				*((char *) key_value )
					= current_cjson_element->valueint;
				break;
			case MXFT_UCHAR:
				*((unsigned char *) key_value )
					= current_cjson_element->valueint;
				break;
			case MXFT_SHORT:
				*((short *) key_value )
					= current_cjson_element->valueint;
				break;
			case MXFT_USHORT:
				*((unsigned short *) key_value )
					= current_cjson_element->valueint;
				break;
			case MXFT_BOOL:
				*((mx_bool_type *) key_value )
					= current_cjson_element->valueint;
				break;
			case MXFT_LONG:
				*((long *) key_value )
					= current_cjson_element->valueint;
				break;
			case MXFT_ULONG:
			case MXFT_HEX:
				*((unsigned long *) key_value )
					= current_cjson_element->valueint;
				break;
			case MXFT_FLOAT:
				*((float *) key_value )
					= current_cjson_element->valuedouble;
				break;
			case MXFT_DOUBLE:
				*((double *) key_value )
					= current_cjson_element->valuedouble;
				break;
			case MXFT_INT64:
				*((int64_t *) key_value )
					= current_cjson_element->valueint;
				break;
			case MXFT_UINT64:
				*((uint64_t *) key_value )
					= current_cjson_element->valueint;
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
		} else {
			/* Proceed on to the next key. */

			current_cjson_element = current_cjson_element->next;

			if ( current_cjson_element == (struct cJSON *) NULL ) {
				/* Oops, there _is_ no next element.  We have
				 * not succeeded in finding our key.
				 */

				return mx_error( MXE_NOT_FOUND, fname,
				"JSON key '%s' was not found for "
				"JSON structure %p.", key_name, json );
			}
		}
	}

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
			max_key_value_bytes, sizeof(double_array) );
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
		switch( mx_datatype ) {
		case MXFT_STRING:
			mx_status = mx_json_get_key( json, key_name,
						mx_json_datatype,
						bool_array,
						max_key_value_bytes );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			snprintf( key_value, max_key_value_bytes, "%d",
					bool_array[0] );

			return MX_SUCCESSFUL_RESULT;
			break;
		}
		break;
	case MXFT_LONG:
		switch( mx_datatype ) {
		case MXFT_STRING:
			mx_status = mx_json_get_key( json, key_name,
						mx_json_datatype,
						long_array,
						max_key_value_bytes );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			snprintf( key_value, max_key_value_bytes, "%ld",
					long_array[0] );

			return MX_SUCCESSFUL_RESULT;
			break;
		}
		break;
	case MXFT_ULONG:
		switch( mx_datatype ) {
		case MXFT_STRING:
			mx_status = mx_json_get_key( json, key_name,
						mx_json_datatype,
						ulong_array,
						max_key_value_bytes );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			snprintf( key_value, max_key_value_bytes, "%lu",
					ulong_array[0] );

			return MX_SUCCESSFUL_RESULT;
			break;
		}
		break;
	case MXFT_FLOAT:
		switch( mx_datatype ) {
		case MXFT_STRING:
			mx_status = mx_json_get_key( json, key_name,
						mx_json_datatype,
						float_array,
						max_key_value_bytes );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			snprintf( key_value, max_key_value_bytes, "%g",
					float_array[0] );

			return MX_SUCCESSFUL_RESULT;
			break;
		}
		break;
	}

	return mx_error( MXE_NOT_FOUND, fname,
		"Foooooooooo!  mx_datatype = %ld, mx_json_datatype = %ld",
		mx_datatype, mx_json_datatype );
}

