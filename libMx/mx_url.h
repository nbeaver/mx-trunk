/*
 * Name:    mx_url.h
 *
 * Purpose: Definitions for URI-based communication (HTTP, etc.)
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

#ifndef __MX_URL_H__
#define __MX_URL_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/*----*/

typedef struct {
	void *url_handle;
} MX_URL;

typedef struct {
	mx_status_type ( *open ) ( MX_URL *url, char *url_string );
	mx_status_type ( *get ) ( MX_URL *url,
				char *response, size_t max_response_length );
	mx_status_type ( *put ) ( MX_URL *url, char *command );
	mx_status_type ( *post ) ( MX_URL *url, char *command );
} MX_URL_FUNCTION_LIST;

/*----*/

MX_API mx_status_type mx_url_open( MX_URL *url, char *url_string );

MX_API mx_status_type mx_url_get( MX_URL *url,
				char *response,
				size_t max_response_length );

MX_API mx_status_type mx_url_put( MX_URL *put, char *command );

MX_API mx_status_type mx_url_post( MX_URL *put, char *command );

#ifdef __cplusplus
}
#endif

#endif /* __MX_URL_H__ */
