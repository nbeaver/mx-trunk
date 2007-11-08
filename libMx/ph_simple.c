/*
 * Name:    ph_simple.c
 *
 * Purpose: Driver for the scan simple permission handler.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2005-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "mx_relay.h"
#include "mx_variable.h"
#include "mx_mpermit.h"
#include "mx_scan.h"
#include "ph_simple.h"

MX_MEASUREMENT_PERMIT_FUNCTION_LIST mxph_simple_function_list = {
		mxph_simple_create_handler,
		mxph_simple_destroy_handler,
		mxph_simple_check_for_permission
};

static mx_status_type
mxph_simple_get_pointers( MX_MEASUREMENT_PERMIT *permit_handler,
			MX_SIMPLE_MEASUREMENT_PERMIT **simple_permit_struct,
			const char *calling_fname )
{
	const char fname[] = "mxph_simple_get_pointers()";

	if ( permit_handler == (MX_MEASUREMENT_PERMIT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MEASUREMENT_PERMIT pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( simple_permit_struct == (MX_SIMPLE_MEASUREMENT_PERMIT **)NULL )
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
	"The MX_SIMPLE_MEASUREMENT_PERMIT pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*simple_permit_struct = (MX_SIMPLE_MEASUREMENT_PERMIT *)
					permit_handler->type_struct;

	if ( *simple_permit_struct == (MX_SIMPLE_MEASUREMENT_PERMIT *)NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_SIMPLE_MEASUREMENT_PERMIT pointer for permit handler '%s' "
	"passed by '%s' is NULL.", permit_handler->mx_typename, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxph_simple_create_handler( MX_MEASUREMENT_PERMIT **permit_handler,
				void *permit_driver_ptr, void *scan_ptr,
				char *description )
{
	const char fname[] = "mxph_simple_create_handler()";

	MX_MEASUREMENT_PERMIT_DRIVER *permit_driver;
	MX_MEASUREMENT_PERMIT *permit_handler_ptr;
	MX_SCAN *scan;
	MX_RECORD *permit_record;
	char *permit_record_name;
	char *comma_ptr, *endptr;
	char description_buffer[100];
	unsigned long permit_value;
	MX_SIMPLE_MEASUREMENT_PERMIT *simple_permit_struct;

	if ( permit_handler == (MX_MEASUREMENT_PERMIT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MEASUREMENT_PERMIT pointer passed is NULL." );
	}
	if ( permit_driver_ptr == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MEASUREMENT_PERMIT_DRIVER pointer passed is NULL." );
	}
	if ( scan_ptr == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCAN pointer passed is NULL." );
	}
	if ( description == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The description pointer passed is NULL." );
	}

	MX_DEBUG( 2,("%s invoked for description '%s'",
			fname, description));

	*permit_handler = NULL;

	permit_driver = (MX_MEASUREMENT_PERMIT_DRIVER *) permit_driver_ptr;

	scan = (MX_SCAN *) scan_ptr;

	MX_DEBUG( 2,("%s: scan = %p", fname, scan));
	MX_DEBUG( 2,("%s: scan->record = %p", fname, scan->record));
	MX_DEBUG( 2,
		("%s: scan->record->name = '%s'", fname, scan->record->name));

	/* Parse the simple permission description string.  The description
	 * should look like
	 *    permit_record_name,permit_value
	 *
	 * An example would be "permit1,1" where permit1 is the permit record
	 * and 1 is the permit value.
	 */

	/* Find the "permit" value, that is, the value the record should
	 * have when there is scan permission.
	 */

	strlcpy(description_buffer, description, sizeof(description_buffer));

	permit_record_name = description_buffer;

	comma_ptr = strchr( permit_record_name, ',' );

	if ( comma_ptr == NULL ) {
		permit_value = 0;
	} else {
		*comma_ptr = '\0';

		comma_ptr++;

		permit_value = strtoul( comma_ptr, &endptr, 0 );

		if ( *endptr != '\0' ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The specified permit value '%s' in simple permission description '%s' "
	"is not a legal number.", comma_ptr, description );
		}
	}

	MX_DEBUG(-2,("%s: permit_record_name = '%s', permit_value = %lu",
		fname, permit_record_name, permit_value));

	permit_record = mx_get_record( scan->record, permit_record_name );

	if ( permit_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The permit record '%s' for the simple permit description '%s' does not exist.",
			permit_record_name, description );
	}

	/* Create the permit handler. */

	permit_handler_ptr = (MX_MEASUREMENT_PERMIT *)
				malloc( sizeof( MX_MEASUREMENT_PERMIT ) );

	if ( permit_handler_ptr == (MX_MEASUREMENT_PERMIT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
    "Ran out of memory trying to create an MX_MEASUREMENT_PERMIT structure.");
	}

	/* Create the type specific structure. */

	simple_permit_struct = ( MX_SIMPLE_MEASUREMENT_PERMIT * )
		malloc( sizeof( MX_SIMPLE_MEASUREMENT_PERMIT ) );

	if ( simple_permit_struct == (MX_SIMPLE_MEASUREMENT_PERMIT *)NULL )
	{
		free( permit_handler_ptr );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory trying to create an MX_SIMPLE_MEASUREMENT_PERMIT structure.");
	}

	/* Fill in the fields in simple_permit_struct. */

	simple_permit_struct->permit_record = permit_record;
	simple_permit_struct->permit_value = permit_value;

	/* Fill in fields in the permit handler structure. */

	permit_handler_ptr->scan = scan;
	permit_handler_ptr->type = permit_driver->type;
	permit_handler_ptr->mx_typename = permit_driver->name;
	permit_handler_ptr->permit_status = FALSE;
	permit_handler_ptr->type_struct = simple_permit_struct;
	permit_handler_ptr->function_list = permit_driver->function_list;

	/* Send the permit handler back to the caller. */

	*permit_handler = permit_handler_ptr;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxph_simple_destroy_handler( MX_MEASUREMENT_PERMIT *permit_handler )
{
	const char fname[] = "mxph_simple_destroy_handler()";

	MX_SIMPLE_MEASUREMENT_PERMIT *simple_permit_struct;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( permit_handler == (MX_MEASUREMENT_PERMIT *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mxph_simple_get_pointers( permit_handler,
					&simple_permit_struct, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( permit_handler->type_struct != NULL ) {
		free( permit_handler->type_struct );

		permit_handler->type_struct = NULL;
	}

	free( permit_handler );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxph_simple_check_for_permission( MX_MEASUREMENT_PERMIT *permit_handler )
{
	const char fname[] = "mxph_simple_check_for_permission()";

	MX_SIMPLE_MEASUREMENT_PERMIT *simple_permit_struct;
	MX_RECORD *permit_record;
	long field_type;
	unsigned long permit_value;

	mx_bool_type bool_value;
	char char_value;
	unsigned char uchar_value;
	short short_value;
	unsigned short ushort_value;
	long long_value;
	unsigned long ulong_value;
	int64_t int64_value;
	uint64_t uint64_value;
	mx_status_type mx_status;

	simple_permit_struct = NULL;

	mx_status = mxph_simple_get_pointers( permit_handler,
					&simple_permit_struct, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	permit_handler->permit_status = FALSE;

	permit_record   = simple_permit_struct->permit_record;
	permit_value = simple_permit_struct->permit_value;

	MX_DEBUG( 2,("%s invoked for permit record '%s', permit_value = %lu.",
			fname, permit_record->name, permit_value));

	switch( permit_record->mx_superclass ) {
	case MXR_DEVICE:
		switch( permit_record->mx_class ) {
		case MXC_DIGITAL_INPUT:
			mx_status = mx_digital_input_read( permit_record,
								&ulong_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( ulong_value == permit_value ) {
				permit_handler->permit_status = TRUE;
			}
			break;
		case MXC_RELAY:
			mx_status = mx_get_relay_status( permit_record,
								&long_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( long_value == permit_value ) {
				permit_handler->permit_status = TRUE;
			}
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Permit record '%s' is of a device class (%ld) that may not be used as "
	"a permit record.", permit_record->name, permit_record->mx_class );
		}
		break;
	case MXR_VARIABLE:
		mx_status = mx_get_variable_parameters( permit_record,
					NULL, NULL, &field_type, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( field_type ) {
		case MXFT_CHAR:
			mx_status = mx_get_char_variable( permit_record,
								&char_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( char_value == permit_value ) {
				permit_handler->permit_status = TRUE;
			}
			break;
		case MXFT_UCHAR:
			mx_status = mx_get_unsigned_char_variable( permit_record,
								&uchar_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( uchar_value == permit_value ) {
				permit_handler->permit_status = TRUE;
			}
			break;
		case MXFT_SHORT:
			mx_status = mx_get_short_variable( permit_record,
								&short_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( short_value == permit_value ) {
				permit_handler->permit_status = TRUE;
			}
			break;
		case MXFT_USHORT:
			mx_status = mx_get_unsigned_short_variable(permit_record,
								&ushort_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( ushort_value == permit_value ) {
				permit_handler->permit_status = TRUE;
			}
			break;
		case MXFT_BOOL:
			mx_status = mx_get_bool_variable( permit_record,
								&bool_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( bool_value == permit_value ) {
				permit_handler->permit_status = TRUE;
			}
			break;
		case MXFT_LONG:
			mx_status = mx_get_long_variable( permit_record,
								&long_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( long_value == permit_value ) {
				permit_handler->permit_status = TRUE;
			}
			break;
		case MXFT_ULONG:
		case MXFT_HEX:
			mx_status = mx_get_unsigned_long_variable( permit_record,
								&ulong_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( ulong_value == permit_value ) {
				permit_handler->permit_status = TRUE;
			}
			break;
		case MXFT_INT64:
			mx_status = mx_get_int64_variable( permit_record,
								&int64_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( int64_value == permit_value ) {
				permit_handler->permit_status = TRUE;
			}
			break;
		case MXFT_UINT64:
			mx_status = mx_get_uint64_variable( permit_record,
								&uint64_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( uint64_value == permit_value ) {
				permit_handler->permit_status = TRUE;
			}
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Permit record '%s' is of a variable field type (%ld) that may not be used as "
"a permit record.", permit_record->name, field_type );
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Permit record '%s' is of a superclass (%ld) that may not be used as "
	"a permit record.", permit_record->name, permit_record->mx_superclass );
	}
	return MX_SUCCESSFUL_RESULT;
}

