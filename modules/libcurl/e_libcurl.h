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

#define MXU_MAXIMUM_CURL_FORMAT_LENGTH	80

/*----*/

#define MX_LIBCURL_INITIALIZE_CONTEXT( x ) \
	do { \
		(x)->content_type_ptr = NULL; \
		(x)->header_ptr = NULL; \
		(x)->response_ptr = NULL; \
		(x)->max_content_type_length = 0; \
		(x)->max_header_length = 0; \
		(x)->max_response_length = 0; \
		(x)->header_bytes_read = 0; \
		(x)->data_bytes_read = 0; \
		(x)->data_bytes_written = 0; \
		(x)->header_complete = FALSE; \
		(x)->read_complete = FALSE; \
		(x)->write_complete = FALSE; \
	} while(0)

/*----*/

typedef struct {
	MX_DYNAMIC_LIBRARY *libcurl_library;

	MX_HTTP *http;
	MX_HTTP_FUNCTION_LIST *http_function_list;
	MX_EXTENSION *extension;

	CURL *curl_handle;

	char *content_type_ptr;
	char *header_ptr;
	char *response_ptr;

	size_t max_content_type_length;
	size_t max_header_length;
	size_t max_response_length;

	size_t header_bytes_read;
	size_t data_bytes_read;
	size_t data_bytes_written;

	mx_bool_type header_complete;
	mx_bool_type read_complete;
	mx_bool_type write_complete;

	char curl_error_buffer[CURL_ERROR_SIZE+1];
	char curl_error_format[MXU_MAXIMUM_CURL_FORMAT_LENGTH+1];
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
						char *, size_t,
						char *, size_t );
MX_API mx_status_type mxext_libcurl_http_head( MX_HTTP *, char *,
						unsigned long *,
						char *, size_t );
MX_API mx_status_type mxext_libcurl_http_post( MX_HTTP *, char *,
						unsigned long *,
						char *, char *, size_t );
MX_API mx_status_type mxext_libcurl_http_put( MX_HTTP *, char *,
						unsigned long *,
						char *, char *, size_t );

extern MX_HTTP_FUNCTION_LIST mxext_libcurl_http_function_list;

#endif /* __E_LIBCURL_H__ */

