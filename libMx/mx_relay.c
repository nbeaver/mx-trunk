/*
 * Name:    mx_relay.c
 *
 * Purpose: MX function library of generic relay operations.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004-2006, 2008 Illinois Institute of Technology
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
#include "mx_hrt.h"
#include "mx_relay.h"

static mx_status_type
mx_relay_get_pointers( MX_RECORD *record,
			MX_RELAY **relay,
			MX_RELAY_FUNCTION_LIST **flist,
			const char *calling_fname )
{
	static const char fname[] = "mx_relay_get_pointers()";

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

	mx_status = mx_relay_get_pointers( record, &relay, &flist, fname );

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

	mx_status = mx_relay_get_pointers( record, &relay, &flist, fname );

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

MX_EXPORT mx_status_type
mx_relay_pulse( MX_RECORD *record,
			long pulse_on_value,
			long pulse_off_value,
			double pulse_duration )
{
	static const char fname[] = "mx_relay_pulse()";

	MX_RELAY *relay;
	MX_RELAY_FUNCTION_LIST *flist;
	mx_status_type ( *pulse_fn ) ( MX_RELAY * );
	mx_status_type mx_status;

	mx_status = mx_relay_get_pointers( record,
					&relay, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pulse_fn = flist->pulse;

	if ( pulse_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"pulse function pointer for record '%s' is NULL.",
			record->name );
	}

	relay->pulse_on_value = pulse_on_value;
	relay->pulse_off_value = pulse_off_value;

	relay->pulse_duration = pulse_duration;

	mx_status = (*pulse_fn)( relay );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_relay_pulse_wait( MX_RECORD *record,
			long pulse_on_value,
			long pulse_off_value,
			double pulse_duration,
			mx_bool_type busy_wait )
{
	unsigned long pulse_seconds, pulse_microseconds;
	mx_bool_type use_seconds;
	mx_status_type mx_status;

	if ( pulse_on_value ) {
		pulse_on_value = MXF_CLOSE_RELAY;
	} else {
		pulse_on_value = MXF_OPEN_RELAY;
	}

	if ( pulse_off_value ) {
		pulse_off_value = MXF_CLOSE_RELAY;
	} else {
		pulse_off_value = MXF_OPEN_RELAY;
	}

	if ( pulse_duration > 3600.0 ) {
		pulse_seconds = mx_round( pulse_duration );
		use_seconds = TRUE;
	} else {
		pulse_microseconds = mx_round( 1000000.0 * pulse_duration );
		use_seconds = FALSE;
	}

	mx_status = mx_relay_command( record, pulse_on_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( use_seconds ) {
		mx_sleep( pulse_seconds );
	} else {
		if ( busy_wait ) {
			mx_udelay( pulse_microseconds );
		} else {
			mx_usleep( pulse_microseconds );
		}
	}

	mx_status = mx_relay_command( record, pulse_off_value );

	return mx_status;
}

