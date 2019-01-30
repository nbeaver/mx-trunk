/*
 * Name:    mx_http.h
 *
 * Purpose: Interfaces to HTTP 1.1 connections.
 *
 * Author:  William Lavender
 *
 * This header file provides a way to make HTTP 1.1 connections to a
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

#ifndef __MX_HTTP_H__
#define __MX_HTTP_H__

#define MXU_HTTP_DRIVER_NAME_LENGTH	40

/* extension call() request code. */

#define MXRC_HTTP_SET_HTTP_POINTER	1
#define MXRC_HTTP_GET_FUNCTION_LIST	2

/*----*/

typedef struct {
	char driver_name[MXU_HTTP_DRIVER_NAME_LENGTH+1];
	MX_EXTENSION *http_extension;
	struct mx_http_function_list *http_function_list;

	mx_bool_type http_debug;
	void *http_private;
} MX_HTTP;

typedef struct mx_http_function_list {
	mx_status_type ( *create )( MX_HTTP *http );
	mx_status_type ( *destroy )( MX_HTTP *http );
	mx_status_type ( *http_delete )( MX_HTTP *http, char *url,
			unsigned long *http_status_code );
	mx_status_type ( *http_get )( MX_HTTP *http, char *url,
			unsigned long *http_status_code,
			char *content_type, size_t max_content_type_length,
			char *received_data, size_t max_received_data_length );
	mx_status_type ( *http_head )( MX_HTTP *http, char *url,
			unsigned long *http_status_code,
			char *received_header,
			size_t max_received_header_length);
	mx_status_type ( *http_post )( MX_HTTP *http, char *url,
			unsigned long *http_status_code,
			char *content_type,
			char *sent_data, size_t sent_data_length );
	mx_status_type ( *http_put )( MX_HTTP *http, char *url,
			unsigned long *http_status_code,
			char *content_type,
			char *sent_data, size_t sent_data_length );
} MX_HTTP_FUNCTION_LIST;

/*-------------------------------------------------------------------------*/

MX_API mx_status_type mx_http_get_default_driver( char *driver_name_buffer,
							size_t max_length );

MX_API void mx_http_set_default_driver( char *driver_name );

MX_API mx_status_type mx_http_create( MX_HTTP **http,
					MX_RECORD *mx_database,
					char *driver_name ); 

/*---*/

MX_API mx_status_type mx_http_delete( MX_HTTP *http, char *url,
					unsigned long *http_status_code );

MX_API mx_status_type mx_http_get( MX_HTTP *http, char *url,
					unsigned long *http_status_code,
					char *content_type,
					size_t max_content_type_length,
					char *received_data,
					size_t max_received_data_length );

MX_API mx_status_type mx_http_head( MX_HTTP *http, char *url,
					unsigned long *http_status_code,
					char *received_header,
					size_t max_received_header_length );

MX_API mx_status_type mx_http_post( MX_HTTP *http, char *url,
					unsigned long *http_status_code,
					char *content_type,
					char *sent_data,
					size_t sent_data_length );

MX_API mx_status_type mx_http_put( MX_HTTP *http, char *url,
					unsigned long *http_status_code,
					char *content_type,
					char *sent_data,
					size_t sent_data_length );

#endif /* __MX_HTTP_H__ */

