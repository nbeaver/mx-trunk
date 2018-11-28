/*
 * Name:    mx_url.c
 *
 * Purpose: Functions for URL-based communication (HTTP, etc.)
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_url.h"

MX_EXPORT mx_status_type
mx_url_open( MX_URL *url, char *url_string )
{
	static const char fname[] = "mx_url_open()";

	if ( url == (MX_URL *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_URL pointer passed was NULL." );
	}
	if ( url_string == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The url_string pointer passed was NULL." );
	}

#if 1
	MX_DEBUG(2,("%s invoked for URL '%s'", fname, url_string));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_url_get( MX_URL *url, char *response, size_t max_response_length )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_url_put( MX_URL *url, char *command )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_url_post( MX_URL *url, char *command )
{
	return MX_SUCCESSFUL_RESULT;
}

