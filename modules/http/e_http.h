/*
 * Name:    e_http.h
 *
 * Purpose: Header for the MX basic HTTP extension.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __E_HTTP_H__
#define __E_HTTP_H__

typedef struct {
	MX_HTTP *http;
	MX_HTTP_FUNCTION_LIST *http_function_list;
	MX_EXTENSION *extension;

	MX_RECORD *rs232_record;
} MX_HTTP_EXTENSION_PRIVATE;

extern MX_EXTENSION_FUNCTION_LIST mxext_http_extension_function_list;

MX_API mx_status_type mxext_http_initialize( MX_EXTENSION *extension );
MX_API mx_status_type mxext_http_call( MX_EXTENSION *extension,
						int request_code,
						int argc, void **argv );

/*---------*/

/* For MX HTTP connections, we will need a driver table of HTTP functions,
 * so we declare them here.
 */

MX_API mx_status_type mxext_http_create( MX_HTTP * );
MX_API mx_status_type mxext_http_destroy( MX_HTTP * );
MX_API mx_status_type mxext_http_http_delete( MX_HTTP *, char *,
						unsigned long * );
MX_API mx_status_type mxext_http_http_get( MX_HTTP *, char *,
						unsigned long *,
						char *, size_t,
						char *, size_t );
MX_API mx_status_type mxext_http_http_head( MX_HTTP *, char *,
						unsigned long *,
						char *, size_t );
MX_API mx_status_type mxext_http_http_post( MX_HTTP *, char *,
						unsigned long *,
						char *, char *, ssize_t );
MX_API mx_status_type mxext_http_http_put( MX_HTTP *, char *,
						unsigned long *,
						char *, char *, ssize_t );

extern MX_HTTP_FUNCTION_LIST mxext_http_http_function_list;

#endif /* __E_HTTP_H__ */

