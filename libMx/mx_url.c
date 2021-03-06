/*
 * Name:    mx_url.c
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

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_thread.h"
#include "mx_url.h"

MX_EXPORT mx_status_type
mx_url_get_pointers( MX_RECORD *url_record,
			MX_URL_SERVER **url_server,
			MX_URL_FUNCTION_LIST **url_flist,
			const char *calling_fname )
{
	static const char fname[] = "mx_url_get_pointers()";

	if ( url_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The url_record pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( url_server != (MX_URL_SERVER **) NULL ) {
		*url_server = (MX_URL_SERVER *)
				url_record->record_class_struct;

		if ( (*url_server) == (MX_URL_SERVER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_URL_SERVER pointer for record '%s' is NULL.",
				url_record->name );
		}
	}

	if ( url_flist != (MX_URL_FUNCTION_LIST **) NULL ) {
		*url_flist = (MX_URL_FUNCTION_LIST *)
				url_record->class_specific_function_list;

		if ( (*url_flist) == (MX_URL_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The MX_URL_FUNCTION_LIST pointer for record '%s' is NULL.",
				url_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mx_url_get( MX_RECORD *url_record,
		char *url,
		unsigned long *url_status_code,
		char *content_type,
		size_t max_content_type_length,
		char *received_data,
		size_t max_received_data_length )
{
	static const char fname[] = "mx_url_get()";

	MX_URL_SERVER *url_server = NULL;
	MX_URL_FUNCTION_LIST *url_flist = NULL;
	mx_status_type (*get_fn)( MX_URL_SERVER *,
				char *, unsigned long *,
				char *, size_t,
				char *, size_t ) = NULL;
	mx_status_type mx_status;

	mx_status = mx_url_get_pointers( url_record,
					&url_server, &url_flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if( url_server->url_flags & MXF_URL_DEBUG ) {
		MX_DEBUG(-2,("%s: [%#lx] url = '%s'", fname,
		    mx_get_thread_id( mx_get_current_thread_pointer() ), url ));
	}

	get_fn = url_flist->url_get;

	if ( get_fn == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The MX URL record '%s' does not support GET operations.",
			url_record->name );
	}

	mx_status = (*get_fn)( url_server, url, url_status_code,
				content_type, max_content_type_length,
				received_data, max_received_data_length );

	if( url_server->url_flags & MXF_URL_DEBUG ) {
	    if ( url_status_code == NULL ) {
		MX_DEBUG(-2,("%s: [%#lx] received_data = '%s'", fname,
			mx_get_thread_id( mx_get_current_thread_pointer() ),
			received_data));
	    } else {
		MX_DEBUG(-2,
		("%s: [%#lx] url status = %lu, received_data = '%s'", fname,
			mx_get_thread_id( mx_get_current_thread_pointer() ),
		 	*url_status_code, received_data));
	    }
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_url_put( MX_RECORD *url_record,
		char *url,
		unsigned long *url_status_code,
		char *content_type,
		char *sent_data,
		size_t sent_data_length,
		char *response_data,
		size_t max_response_data_length )
{
	static const char fname[] = "mx_url_put()";

	MX_URL_SERVER *url_server = NULL;
	MX_URL_FUNCTION_LIST *url_flist = NULL;
	mx_status_type (*put_fn)( MX_URL_SERVER *,
				char *, unsigned long *,
				char *, char *, ssize_t,
		       		char *, size_t ) = NULL;

	char local_response_data[400];
	mx_status_type mx_status;

	if ( response_data == (char *) NULL ) {
		response_data = local_response_data;
		max_response_data_length = sizeof(local_response_data);
	}

	mx_status = mx_url_get_pointers( url_record,
					&url_server, &url_flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if( url_server->url_flags & MXF_URL_DEBUG ) {
		MX_DEBUG(-2,("%s: [%#lx] url = '%s', sent_data = '%s'", fname,
			mx_get_thread_id( mx_get_current_thread_pointer() ),
			url, sent_data));
	}

	put_fn = url_flist->url_put;

	if ( put_fn == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The MX URL record '%s' does not support PUT operations.",
			url_record->name );
	}

	mx_status = (*put_fn)( url_server, url,
				url_status_code, content_type, 
				sent_data, sent_data_length,
				response_data, max_response_data_length );

	if( url_server->url_flags & MXF_URL_DEBUG ) {
	    if ( url_status_code == NULL ) {
		MX_DEBUG(-2,("%s: [%#lx] response_data = '%s'", fname,
			mx_get_thread_id( mx_get_current_thread_pointer() ),
			response_data));
	    } else {
		MX_DEBUG(-2,
		("%s: [%#lx] url status = %lu, response_data = '%s'", fname,
			mx_get_thread_id( mx_get_current_thread_pointer() ),
		 	*url_status_code, response_data));
	    }
	}

	return mx_status;
}

