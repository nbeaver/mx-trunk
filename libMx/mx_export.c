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

#include "mxconfig.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_export.h"

#if HAVE_EPICS
#include "mx_epics.h"

static mx_status_type (*mx_epics_export_callback_fn)(MX_RECORD *, char *);
#endif

/* FIXME: Danger Will Robinson! KLUDGE! KLUDGE! KLUDGE!  We need a _real_
 * mechanism for setting up exports to other control systems such as EPICS.
 * This kludge has been in place since February 2009.  Please FIXME.
 */

MX_EXPORT mx_status_type
mx_register_export_callback( char *export_typename,
			mx_status_type (*callback_fn)( MX_RECORD *, char * ) )
{
	static const char fname[] = "mx_register_export_callback()";

#if MX_EXPORT_DEBUG
	MX_DEBUG(-2,("%s invoked for export typename = '%s'",
		fname, export_typename));
#endif

	if ( strcmp( export_typename, "epics" ) == 0 ) {
		mx_epics_export_callback_fn = callback_fn;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unrecognized export type '%s' requested.", export_typename );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_invoke_export_callback( MX_RECORD *record_list, char *buffer )
{
	static const char fname[] = "mx_invoke_export_callback()";

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

#if (HAVE_EPICS == FALSE)
		if ( 0 ) {
#else
		if ( strcmp( argv[1], "epics" ) == 0 ) {

			if ( mx_epics_export_callback_fn == NULL ) {
				mx_status = mx_error( MXE_NOT_AVAILABLE, fname,
					"Support for EPICS export is not "
					"available for this server.  "
					"Requested by line '%s'", buffer );
			} else {
				mx_status = (*mx_epics_export_callback_fn)(
						record_list, buffer );
			}
#endif /* HAVE_EPICS */

		} else {
			mx_warning( "Skipping unrecognized !export line '%s'.",
					buffer );
		}
	}

	mx_free(argv);
	mx_free(dup_buffer);

	return mx_status;
}

