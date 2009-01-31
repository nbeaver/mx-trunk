/*
 * Name:     mx_export.c
 *
 * Purpose:  Support for exporting MX records to external control systems.
 *
 * Author:   William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_EXPORT_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_list.h"
#include "mx_export.h"

static MX_LIST *mx_export_list = NULL;

typedef struct {
	char name[80];
	mx_status_type (*callback_fn)( MX_RECORD *, char * );
} MX_EXPORT_LIST_ENTRY;

/*----*/

static mx_status_type
mx_export_list_traverse_fn( MX_LIST_ENTRY *list_entry,
				void *export_type_name_ptr,
				void **callback_fn_ptr )
{
	MX_EXPORT_LIST_ENTRY *export_list_entry;
	char *export_type_name;

	export_list_entry = list_entry->list_entry_data;

	export_type_name = export_type_name_ptr;

	if ( strcmp( export_type_name, export_list_entry->name ) == 0 ) {
		*callback_fn_ptr = export_list_entry->callback_fn;

		return mx_error( MXE_EARLY_EXIT | MXE_QUIET, "", " " );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mx_register_export_callback( char *export_typename,
			mx_status_type (*callback_fn)( MX_RECORD *, char * ) )
{
	static const char fname[] = "mx_register_export_callback()";

	MX_LIST_ENTRY *list_entry;
	MX_EXPORT_LIST_ENTRY *export_list_entry;
	mx_status_type mx_status;

#if MX_EXPORT_DEBUG
	MX_DEBUG(-2,("%s invoked for export typename = '%s'",
		fname, export_typename));
#endif

	if ( mx_export_list == (MX_LIST *) NULL ) {
		mx_status = mx_list_create( &mx_export_list );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	export_list_entry = malloc( sizeof(MX_EXPORT_LIST_ENTRY) );

	if ( export_list_entry == (MX_EXPORT_LIST_ENTRY *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a new "
			"MX_EXPORT_LIST_ENTRY structure." );
	}

	strlcpy( export_list_entry->name, export_typename,
		sizeof( export_list_entry->name ) );

	export_list_entry->callback_fn = callback_fn;

	mx_status = mx_list_entry_create(&list_entry, export_list_entry, NULL);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_list_add_entry( mx_export_list, list_entry );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_invoke_export_callback( MX_RECORD *record_list, char *buffer )
{
#if MX_EXPORT_DEBUG
	static const char fname[] = "mx_invoke_export_callback()";
#endif

	mx_status_type (*callback_fn)( MX_RECORD *, char * );
	void *callback_fn_ptr;

	char *dup_buffer;
	char **argv;
	int argc;
	mx_status_type mx_status;

#if MX_EXPORT_DEBUG
	MX_DEBUG(-2,("%s invoked for buffer line '%s'", fname, buffer));
#endif

	record_list = record_list->list_head;

	mx_status = MX_SUCCESSFUL_RESULT;

	dup_buffer = strdup( buffer );

	mx_string_split( dup_buffer, " ", &argc, &argv );

	if ( argc < 2 ) {
		mx_warning( "Skipping truncated !export directive '%s'",
				    buffer );
	} else {

		mx_status = mx_list_traverse( mx_export_list,
					mx_export_list_traverse_fn,
					argv[1], &callback_fn_ptr );

		switch( mx_status.code ) {
		case MXE_EARLY_EXIT:
			/* Invoke the export callback that we have found. */

			callback_fn = callback_fn_ptr;

			mx_status = (*callback_fn)( record_list, buffer );
			break;

		case MXE_SUCCESS:
			mx_warning( "Skipping unrecognized !export line '%s'.",
					buffer );
			break;

		default:
			/* Return the status code returned by
			 * mx_list_traverse().
			 */
			break;
		}
	}

	mx_free(argv);
	mx_free(dup_buffer);

	return mx_status;
}

