/*
 * Name:    mx_http.c
 *
 * Purpose: Interfaces to HTTP 1.1 connections.
 *
 * Author:  William Lavender
 *
 * This file provides a way to make HTTP 1.1 connections to a
 * web server via one of several different interfaces.  The most
 * full function one is in the 'libcurl' module, but libMx/n_http.c
 * provides a small bundled interface.
 *
 *----------------------------------------------------------------------
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
#include "mx_module.h"
#include "mx_http.h"

#define MX_HTTP_DRIVER_BUILTIN_DEFAULT	"libcurl"

static char
mx_http_default_driver_name[MXU_HTTP_DRIVER_NAME_LENGTH+1]
	= MX_HTTP_DRIVER_BUILTIN_DEFAULT;

/*---*/

MX_EXPORT mx_status_type
mx_http_get_default_driver( char *driver_name_buffer, size_t max_length )
{
	static const char fname[] = "mx_http_get_default_driver()";

	if ( driver_name_buffer == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The driver name buffer pointer passed was NULL." );
	}

	strlcpy( driver_name_buffer, mx_http_default_driver_name, max_length );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT void
mx_http_set_default_driver( char *driver_name )
{
	if ( driver_name == (const char *) NULL ) {
		strlcpy( mx_http_default_driver_name,
			MX_HTTP_DRIVER_BUILTIN_DEFAULT,
			sizeof( mx_http_default_driver_name ) );
	} else {
		strlcpy( mx_http_default_driver_name, driver_name,
			sizeof( mx_http_default_driver_name ) );
	}

	return;
}

MX_EXPORT
mx_status_type
mx_http_create( MX_HTTP **http,
		MX_RECORD *mx_database,
		char *http_driver_name )
{
	static const char fname[] = "mx_http_create()";

	MX_HTTP *http_ptr = NULL;
	MX_RECORD *mx_database_ptr = NULL;
	MX_MODULE *http_driver_module = NULL;
	mx_status_type mx_status;

	if ( http == (MX_HTTP **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HTTP pointer passed was NULL." );
	}

	http_ptr = (MX_HTTP *) calloc( 1, sizeof(MX_HTTP) );

	if ( http_ptr == (MX_HTTP *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an MX_HTTP object." );
	}

	*http = http_ptr;

	if ( http_driver_name == (const char *) NULL ) {
		strlcpy( http_ptr->driver_name,
			mx_http_default_driver_name,
			sizeof( http_ptr->driver_name ) );
	} else
	if ( strlen(http_driver_name) == 0 ) {
		strlcpy( http_ptr->driver_name,
			mx_http_default_driver_name,
			sizeof( http_ptr->driver_name ) );
	} else {
		strlcpy( http_ptr->driver_name, http_driver_name,
			sizeof( http_ptr->driver_name ) );
	}

	MX_DEBUG(-2,("%s: http = %p, http_ptr->driver_name = '%s'",
		fname, http_ptr, http_ptr->driver_name ));

	/* Find the requested HTTP driver by looking for
	 * a module with that name.  It is expected that
	 * the module has already been loaded via the
	 * MX database file or some other method.
	 */

	if ( mx_database == (MX_RECORD *) NULL ) {
		mx_database_ptr = mx_get_database();
	} else {
		mx_database_ptr = mx_database;
	}

	mx_status = mx_get_module( http_driver_name,
			mx_database_ptr, &http_driver_module );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		break;
	case MXE_NOT_FOUND:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The built-in HTTP driver has not yet been implemented." );
		break;
	default:
		return mx_status;
		break;
	}

	/* Call the HTTP driver create function. */

	

	return mx_status;
}

MX_EXPORT mx_status_type
mx_http_get( MX_HTTP *http,
		char *url,
		unsigned long *http_status_code,
		char **received_data_ptr,
		size_t *received_data_length )
{
	static const char fname[] = "mx_http_get()";

#if 1
	MX_DEBUG(-2,("%s: url = '%s'", fname, url));
#endif

	return MX_SUCCESSFUL_RESULT;
}

