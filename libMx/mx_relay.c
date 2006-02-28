/*
 * Name:    mx_relay.c
 *
 * Purpose: MX function library of generic relay operations.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_relay.h"

static mx_status_type
mx_get_relay_pointers( MX_RECORD *record,
			MX_RELAY **relay,
			MX_RELAY_FUNCTION_LIST **flist,
			const char *calling_fname )
{
	static const char fname[] = "mx_get_relay_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed by '%s' was NULL.", calling_fname );
	}
	if ( record->mx_superclass != MXR_DEVICE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"The record '%s' is not a device record.",
			record->name );
	}
	if ( record->mx_class != MXC_RELAY ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"The record '%s' is not a relay record.",
			record->name );
	}
	if ( relay != (MX_RELAY **) NULL ) {
		*relay = (MX_RELAY *) record->record_class_struct;

		if ( *relay == (MX_RELAY *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RELAY pointer for record '%s' passed by '%s' is NULL.",
				record->name, calling_fname );
		}
	}
	if ( flist != (MX_RELAY_FUNCTION_LIST **) NULL ) {
		*flist = (MX_RELAY_FUNCTION_LIST *)
				record->class_specific_function_list;

		if ( *flist == (MX_RELAY_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RELAY_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
				record->name, calling_fname );
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_relay_command( MX_RECORD *record, long relay_command )
{
	static const char fname[] = "mx_relay_command()";

	MX_RELAY *relay;
	MX_RELAY_FUNCTION_LIST *flist;
	mx_status_type ( *fptr )( MX_RELAY * );
	mx_status_type mx_status;

	mx_status = mx_get_relay_pointers( record, &relay, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->relay_command;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"relay_command function pointer for record '%s' is NULL.",
			record->name );
	}

	relay->relay_command = relay_command;

	mx_status = ( *fptr ) ( relay );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_get_relay_status( MX_RECORD *record, long *relay_status )
{
	static const char fname[] = "mx_get_relay_status()";

	MX_RELAY *relay;
	MX_RELAY_FUNCTION_LIST *flist;
	mx_status_type ( *fptr )( MX_RELAY * );
	mx_status_type mx_status;

	mx_status = mx_get_relay_pointers( record, &relay, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->get_relay_status;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_relay_status function pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = ( *fptr ) ( relay );

	if ( relay_status != (long *) NULL ) {
		*relay_status = relay->relay_status;
	}

	return mx_status;
}

