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
#include "mx_json.h"

MX_EXPORT mx_status_type
mx_json_initialize( void )
{

#if defined(OS_WIN32)
#  error We must redirect cJSON malloc, free to the mx_win32 versions.
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_json_parse( MX_JSON **json, char *json_string )
{
	static const char fname[] = "mx_json_parse()";

	cJSON *cjson = NULL;

	mx_breakpoint();

	if ( json == (MX_JSON **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_JSON pointer passed was NULL." );
	}

	cjson = cJSON_Parse( json_string );

	if ( cjson == (cJSON *) NULL ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The attempt to parse a JSON string failed.  "
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

#if 1
	MX_DEBUG(-2,("%s: cJSON = '%s'", fname, cJSON_Print( cjson ) ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

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

MX_EXPORT mx_status_type
mx_json_get_key( MX_JSON *json,
		long datatype,
		void *value )
{
	static const char fname[] = "mx_json_get_key()";

	if ( json == (MX_JSON *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_JSON pointer passed was NULL." );
	}

	if ( json->cjson != (cJSON *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The cJSON pointer for MX_JSON pointer %p is NULL.", json );
	}

	return MX_SUCCESSFUL_RESULT;
}

