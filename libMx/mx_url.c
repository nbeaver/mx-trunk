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

#define MX_URL_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
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

#if MX_URL_DEBUG
	MX_DEBUG(-2,("%s: url = '%s'", fname, url));
#endif

	get_fn = url_flist->url_get;

	if ( get_fn == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The MX URL record '%s' does not support GET operations.",
			url_record->name );
	}

	mx_status = (*get_fn)( url_server, url, url_status_code,
				content_type, max_content_type_length,
				received_data, max_received_data_length );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_url_put( MX_RECORD *url_record,
		char *url,
		unsigned long *url_status_code,
		char *content_type,
		char *sent_data,
		size_t sent_data_length )
{
	static const char fname[] = "mx_url_put()";

	MX_URL_SERVER *url_server = NULL;
	MX_URL_FUNCTION_LIST *url_flist = NULL;
	mx_status_type (*put_fn)( MX_URL_SERVER *,
				char *, unsigned long *,
				char *, char *, ssize_t ) = NULL;
	mx_status_type mx_status;

	mx_status = mx_url_get_pointers( url_record,
					&url_server, &url_flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_URL_DEBUG
	MX_DEBUG(-2,("%s: url = '%s'", fname, url));
	MX_DEBUG(-2,("%s: sent_data = '%s'", fname, sent_data));
#endif

	put_fn = url_flist->url_put;

	if ( put_fn == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The MX URL record '%s' does not support PUT operations.",
			url_record->name );
	}

	mx_status = (*put_fn)( url_server, url,
				url_status_code, content_type, 
				sent_data, sent_data_length );

	return mx_status;
}

