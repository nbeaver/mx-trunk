/*
 * Name:    mx_url.h
 *
 * Purpose: MX server class for URL-based servers.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_URL_H__
#define __MX_URL_H__

/*----*/

typedef struct {
	MX_RECORD *record;

	unsigned long url_flags;
} MX_URL_SERVER;

typedef struct mx_url_function_list {
	mx_status_type ( *url_delete )( MX_URL_SERVER *url_server, char *url,
			unsigned long *url_status_code );
	mx_status_type ( *url_get )( MX_URL_SERVER *url_server, char *url,
			unsigned long *url_status_code,
			char *content_type, size_t max_content_type_length,
			char *received_data, size_t max_received_data_length );
	mx_status_type ( *url_head )( MX_URL_SERVER *url_server, char *url,
			unsigned long *url_status_code,
			char *received_header,
			size_t max_received_header_length);
	mx_status_type ( *url_post )( MX_URL_SERVER *url_server, char *url,
			unsigned long *url_status_code,
			char *content_type,
			char *sent_data, ssize_t sent_data_length );
	mx_status_type ( *url_put )( MX_URL_SERVER *url_server, char *url,
			unsigned long *url_status_code,
			char *content_type,
			char *sent_data, ssize_t sent_data_length );
} MX_URL_FUNCTION_LIST;

/*-------------------------------------------------------------------------*/

MX_API mx_status_type mx_url_get_pointers( MX_RECORD *url_record,
					MX_URL_SERVER **url_server,
					MX_URL_FUNCTION_LIST **url_flist,
					const char *calling_fname );

MX_API mx_status_type mx_url_delete( MX_RECORD *url_record, char *url,
					unsigned long *url_status_code );

MX_API mx_status_type mx_url_get( MX_RECORD *url_record, char *url,
					unsigned long *url_status_code,
					char *content_type,
					size_t max_content_type_length,
					char *received_data,
					size_t max_received_data_length );

MX_API mx_status_type mx_url_head( MX_RECORD *url_record, char *url,
					unsigned long *url_status_code,
					char *received_header,
					size_t max_received_header_length );

MX_API mx_status_type mx_url_post( MX_RECORD *url_record, char *url,
					unsigned long *url_status_code,
					char *content_type,
					char *sent_data,
					size_t sent_data_length );

MX_API mx_status_type mx_url_put( MX_RECORD *url_record, char *url,
					unsigned long *url_status_code,
					char *content_type,
					char *sent_data,
					size_t sent_data_length );

#endif /* __MX_URL_H__ */

