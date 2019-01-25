/*
 * Name:    e_libcurl.h
 *
 * Purpose: Header for the MX libcurl extension.
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

#ifndef __E_LIBCURL_H__
#define __E_LIBCURL_H__

/*----*/

typedef struct {
	MX_DYNAMIC_LIBRARY *libcurl_library;

	MX_HTTP_FUNCTION_LIST *http_function_list;
} MX_LIBCURL_EXTENSION_PRIVATE;

extern MX_EXTENSION_FUNCTION_LIST mxext_libcurl_extension_function_list;

MX_API mx_status_type mxext_libcurl_initialize( MX_EXTENSION *extension );
MX_API mx_status_type mxext_libcurl_call( MX_EXTENSION *extension,
						int request_code,
						int argc, void **argv );

/*---------*/

/* For MX_HTTP connections, we will need a driver table of HTTP functions,
 * so we declare them here.
 */

MX_API mx_status_type mxext_libcurl_create( MX_HTTP * );
MX_API mx_status_type mxext_libcurl_destroy( MX_HTTP * );
MX_API mx_status_type mxext_libcurl_http_delete( MX_HTTP *, char *,
						unsigned long * );
MX_API mx_status_type mxext_libcurl_http_get( MX_HTTP *, char *,
						unsigned long *,
						char **, size_t * );
MX_API mx_status_type mxext_libcurl_http_head( MX_HTTP *, char *,
						unsigned long *,
						char **, size_t * );
MX_API mx_status_type mxext_libcurl_http_post( MX_HTTP *, char *,
						unsigned long *,
						char *, size_t );
MX_API mx_status_type mxext_libcurl_http_put( MX_HTTP *, char *,
						unsigned long *,
						char *, size_t );

extern MX_HTTP_FUNCTION_LIST mxext_libcurl_http_function_list;

#endif /* __E_LIBCURL_H__ */

