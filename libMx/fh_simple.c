/*
 * Name:    fh_simple.c
 *
 * Purpose: Driver for the scan simple fault handler.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001-2002, 2005-2007 Illinois Institute of Technology
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
#include "mx_mfault.h"
#include "mx_scan.h"
#include "fh_simple.h"

MX_MEASUREMENT_FAULT_FUNCTION_LIST mxfh_simple_function_list = {
		mxfh_simple_create_handler,
		mxfh_simple_destroy_handler,
		mxfh_simple_check_for_fault,
		mxfh_simple_reset
};

static mx_status_type
mxfh_simple_get_pointers( MX_MEASUREMENT_FAULT *fault_handler,
			MX_SIMPLE_MEASUREMENT_FAULT **simple_fault_struct,
			const char *calling_fname )
{
	const char fname[] = "mxfh_simple_get_pointers()";

	if ( fault_handler == (MX_MEASUREMENT_FAULT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MEASUREMENT_FAULT pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( simple_fault_struct == (MX_SIMPLE_MEASUREMENT_FAULT **)NULL )
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
	"The MX_SIMPLE_MEASUREMENT_FAULT pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*simple_fault_struct = (MX_SIMPLE_MEASUREMENT_FAULT *)
					fault_handler->type_struct;

	if ( *simple_fault_struct == (MX_SIMPLE_MEASUREMENT_FAULT *)NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_SIMPLE_MEASUREMENT_FAULT pointer for fault handler '%s' "
	"passed by '%s' is NULL.", fault_handler->mx_typename, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxfh_simple_create_handler( MX_MEASUREMENT_FAULT **fault_handler,
				void *fault_driver_ptr, void *scan_ptr,
				char *description )
{
	const char fname[] = "mxfh_simple_create_handler()";

	MX_MEASUREMENT_FAULT_DRIVER *fault_driver;
	MX_MEASUREMENT_FAULT *fault_handler_ptr;
	MX_SCAN *scan;
	MX_RECORD *fault_record, *reset_record;
	char *fault_record_name, *reset_record_name;
	char *semicolon_ptr, *comma_ptr, *endptr;
	char description_buffer[100];
	unsigned long no_fault_value, reset_value;
	MX_SIMPLE_MEASUREMENT_FAULT *simple_fault_struct;

	if ( fault_handler == (MX_MEASUREMENT_FAULT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MEASUREMENT_FAULT pointer passed is NULL." );
	}
	if ( fault_driver_ptr == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MEASUREMENT_FAULT_DRIVER pointer passed is NULL." );
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

	*fault_handler = NULL;

	fault_driver = (MX_MEASUREMENT_FAULT_DRIVER *) fault_driver_ptr;

	scan = (MX_SCAN *) scan_ptr;

	MX_DEBUG( 2,("%s: scan = %p", fname, scan));
	MX_DEBUG( 2,("%s: scan->record = %p", fname, scan->record));
	MX_DEBUG( 2,("%s: scan->record->name = '%s'", fname, scan->record->name));

	/* Parse the simple fault description string.  The description should
	 * look like
	 *    fault_record_name,no_fault_value;reset_record_name,reset_value
	 *
	 * An example would be "fault1,1;reset1,0" where fault1 is the
	 * fault record, 1 is the no fault value, reset1 is the reset record
	 * and 0 is the reset value.
	 */

	/* Split the description into the fault part and the reset part. */

	strlcpy(description_buffer, description, sizeof(description_buffer));

	semicolon_ptr = strchr( description_buffer, ';' );

	if ( semicolon_ptr == NULL ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Cannot interpret the simple fault description string '%s' "
		"as the name of a fault record and a reset record.",
		description );
	}

	*semicolon_ptr = '\0';

	semicolon_ptr++;

	fault_record_name = description_buffer;
	reset_record_name = semicolon_ptr;

	/* Find the "no fault" value, that is, the value the record should
	 * have when there is no fault.
	 */

	comma_ptr = strchr( fault_record_name, ',' );

	if ( comma_ptr == NULL ) {
		no_fault_value = 0;
	} else {
		*comma_ptr = '\0';

		comma_ptr++;

		no_fault_value = strtoul( comma_ptr, &endptr, 0 );

		if ( *endptr != '\0' ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The specified no fault value '%s' in simple fault description '%s' "
	"is not a legal number.", comma_ptr, description );
		}
	}

	/* Find the "reset" value, that is, the value the record should
	 * be set to in order to reset the fault.
	 */

	comma_ptr = strchr( reset_record_name, ',' );

	if ( comma_ptr == NULL ) {
		reset_value = 1;
	} else {
		*comma_ptr = '\0';

		comma_ptr++;

		reset_value = strtoul( comma_ptr, &endptr, 0 );

		if ( *endptr != '\0' ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The specified reset value '%s' in simple fault description '%s' "
	"is not a legal number.", comma_ptr, description );
		}
	}

	MX_DEBUG( 2,("%s: fault_record_name = '%s', reset_record_name = '%s'",
		fname, fault_record_name, reset_record_name));
	MX_DEBUG( 2,("%s: no_fault_value = %lu, reset_value = %lu",
		fname, no_fault_value, reset_value));

	/* Find the needed records. */

	fault_record = mx_get_record( scan->record, fault_record_name );

	if ( fault_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The fault record '%s' for the simple fault description '%s' does not exist.",
			fault_record_name, description );
	}

	reset_record = mx_get_record( scan->record, reset_record_name );

	if ( reset_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The reset record '%s' for the simple fault description '%s' does not exist.",
			reset_record_name, description );
	}

	/* Create the fault handler. */

	fault_handler_ptr = (MX_MEASUREMENT_FAULT *)
				malloc( sizeof( MX_MEASUREMENT_FAULT ) );

	if ( fault_handler_ptr == (MX_MEASUREMENT_FAULT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
    "Ran out of memory trying to create an MX_MEASUREMENT_FAULT structure.");
	}

	/* Create the type specific structure. */

	simple_fault_struct = ( MX_SIMPLE_MEASUREMENT_FAULT * )
		malloc( sizeof( MX_SIMPLE_MEASUREMENT_FAULT ) );

	if ( simple_fault_struct == (MX_SIMPLE_MEASUREMENT_FAULT *)NULL )
	{
		free( fault_handler_ptr );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory trying to create an MX_SIMPLE_MEASUREMENT_FAULT structure.");
	}

	/* Fill in the fields in simple_fault_struct. */

	simple_fault_struct->fault_record = fault_record;
	simple_fault_struct->reset_record = reset_record;

	simple_fault_struct->no_fault_value = no_fault_value;
	simple_fault_struct->reset_value = reset_value;

	/* Fill in fields in the fault handler structure. */

	fault_handler_ptr->scan = scan;
	fault_handler_ptr->type = fault_driver->type;
	fault_handler_ptr->mx_typename = fault_driver->name;
	fault_handler_ptr->fault_status = 0;
	fault_handler_ptr->reset_flags = 0;
	fault_handler_ptr->type_struct = simple_fault_struct;
	fault_handler_ptr->function_list = fault_driver->function_list;

	/* Send the fault handler back to the caller. */

	*fault_handler = fault_handler_ptr;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxfh_simple_destroy_handler( MX_MEASUREMENT_FAULT *fault_handler )
{
	const char fname[] = "mxfh_simple_destroy_handler()";

	MX_SIMPLE_MEASUREMENT_FAULT *simple_fault_struct;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( fault_handler == (MX_MEASUREMENT_FAULT *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mxfh_simple_get_pointers( fault_handler,
					&simple_fault_struct, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( fault_handler->type_struct != NULL ) {
		free( fault_handler->type_struct );

		fault_handler->type_struct = NULL;
	}

	free( fault_handler );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxfh_simple_check_for_fault( MX_MEASUREMENT_FAULT *fault_handler )
{
	const char fname[] = "mxfh_simple_check_for_fault()";

	MX_SIMPLE_MEASUREMENT_FAULT *simple_fault_struct;
	MX_RECORD *fault_record;
	long field_type;
	unsigned long no_fault_value;

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

	simple_fault_struct = NULL;

	mx_status = mxfh_simple_get_pointers( fault_handler,
					&simple_fault_struct, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fault_handler->fault_status = FALSE;

	fault_record   = simple_fault_struct->fault_record;
	no_fault_value = simple_fault_struct->no_fault_value;

	MX_DEBUG( 2,("%s invoked for fault record '%s', no_fault_value = %lu.",
			fname, fault_record->name, no_fault_value));

	switch( fault_record->mx_superclass ) {
	case MXR_DEVICE:
		switch( fault_record->mx_class ) {
		case MXC_DIGITAL_INPUT:
			mx_status = mx_digital_input_read( fault_record,
								&ulong_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( ulong_value != no_fault_value ) {
				fault_handler->fault_status = TRUE;
			}
			break;
		case MXC_RELAY:
			mx_status = mx_get_relay_status( fault_record,
								&long_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( long_value != no_fault_value ) {
				fault_handler->fault_status = TRUE;
			}
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Fault record '%s' is of a device class (%ld) that may not be used as "
	"a fault record.", fault_record->name, fault_record->mx_class );
		}
		break;
	case MXR_VARIABLE:
		mx_status = mx_get_variable_parameters( fault_record,
					NULL, NULL, &field_type, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( field_type ) {
		case MXFT_CHAR:
			mx_status = mx_get_char_variable( fault_record,
								&char_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( char_value != no_fault_value ) {
				fault_handler->fault_status = TRUE;
			}
			break;
		case MXFT_UCHAR:
			mx_status = mx_get_unsigned_char_variable( fault_record,
								&uchar_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( uchar_value != no_fault_value ) {
				fault_handler->fault_status = TRUE;
			}
			break;
		case MXFT_SHORT:
			mx_status = mx_get_short_variable( fault_record,
								&short_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( short_value != no_fault_value ) {
				fault_handler->fault_status = TRUE;
			}
			break;
		case MXFT_USHORT:
			mx_status = mx_get_unsigned_short_variable(fault_record,
								&ushort_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( ushort_value != no_fault_value ) {
				fault_handler->fault_status = TRUE;
			}
			break;
		case MXFT_BOOL:
			mx_status = mx_get_bool_variable( fault_record,
								&bool_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( bool_value != no_fault_value ) {
				fault_handler->fault_status = TRUE;
			}
			break;
		case MXFT_LONG:
			mx_status = mx_get_long_variable( fault_record,
								&long_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( long_value != no_fault_value ) {
				fault_handler->fault_status = TRUE;
			}
			break;
		case MXFT_ULONG:
		case MXFT_HEX:
			mx_status = mx_get_unsigned_long_variable( fault_record,
								&ulong_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( ulong_value != no_fault_value ) {
				fault_handler->fault_status = TRUE;
			}
			break;
		case MXFT_INT64:
			mx_status = mx_get_int64_variable( fault_record,
								&int64_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( int64_value != no_fault_value ) {
				fault_handler->fault_status = TRUE;
			}
			break;
		case MXFT_UINT64:
			mx_status = mx_get_uint64_variable( fault_record,
								&uint64_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( uint64_value != no_fault_value ) {
				fault_handler->fault_status = TRUE;
			}
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Fault record '%s' is of a variable field type (%ld) that may not be used as "
"a fault record.", fault_record->name, field_type );
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Fault record '%s' is of a superclass (%ld) that may not be used as "
	"a fault record.", fault_record->name, fault_record->mx_superclass );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxfh_simple_reset( MX_MEASUREMENT_FAULT *fault_handler )
{
	const char fname[] = "mxfh_simple_reset()";

	MX_SIMPLE_MEASUREMENT_FAULT *simple_fault_struct;
	MX_RECORD *reset_record;
	long field_type;
	unsigned long reset_value;
	mx_status_type mx_status;

	simple_fault_struct = NULL;

	mx_status = mxfh_simple_get_pointers( fault_handler,
					&simple_fault_struct, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	reset_record = simple_fault_struct->reset_record;
	reset_value  = simple_fault_struct->reset_value;

	MX_DEBUG( 2,("%s invoked for reset record '%s', reset_value = %lu.",
			fname, reset_record->name, reset_value));

#if 0
	if ( fault_handler->reset_flags ==
				MXMF_PREPARE_FOR_FIRST_MEASUREMENT_ATTEMPT )
	{
		/* Do nothing before the first actual measurement in 
		 * a measurement fault loop.
		 */

		return MX_SUCCESSFUL_RESULT;
	}
#endif

	switch( reset_record->mx_superclass ) {
	case MXR_DEVICE:
		switch( reset_record->mx_class ) {
		case MXC_DIGITAL_OUTPUT:
			mx_status = mx_digital_output_write( reset_record,
								reset_value );
			break;
		case MXC_RELAY:
			mx_status = mx_relay_command( reset_record,
							(int) reset_value );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Reset record '%s' is of a device class (%ld) that may not be used as "
	"a reset record.", reset_record->name, reset_record->mx_class );
		}
		break;
	case MXR_VARIABLE:
		mx_status = mx_get_variable_parameters( reset_record,
					NULL, NULL, &field_type, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( field_type ) {
		case MXFT_CHAR:
			mx_status = mx_set_char_variable( reset_record,
						(char) reset_value );
			break;
		case MXFT_UCHAR:
			mx_status = mx_set_unsigned_char_variable( reset_record,
						(unsigned char) reset_value );
			break;
		case MXFT_SHORT:
			mx_status = mx_set_short_variable( reset_record,
						(short) reset_value );
			break;
		case MXFT_USHORT:
			mx_status = mx_set_unsigned_short_variable(reset_record,
						(unsigned short) reset_value );
			break;
		case MXFT_BOOL:
			mx_status = mx_set_bool_variable( reset_record,
						(mx_bool_type) reset_value );
			break;
		case MXFT_LONG:
			mx_status = mx_set_long_variable( reset_record,
						(long) reset_value );
			break;
		case MXFT_ULONG:
		case MXFT_HEX:
			mx_status = mx_set_unsigned_long_variable( reset_record,
						(unsigned long) reset_value );
			break;
		case MXFT_INT64:
			mx_status = mx_set_int64_variable( reset_record,
						(int64_t) reset_value );
			break;
		case MXFT_UINT64:
			mx_status = mx_set_uint64_variable( reset_record,
						(uint64_t) reset_value );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Reset record '%s' is of a variable field type (%ld) that may not be used as "
"a reset record.", reset_record->name, field_type );
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Reset record '%s' is of a superclass (%ld) that may not be used as "
	"a reset record.", reset_record->name, reset_record->mx_superclass );
	}

	return mx_status;
}

