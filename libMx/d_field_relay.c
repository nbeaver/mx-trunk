/*
 * Name:    d_field_relay.c
 *
 * Purpose: MX driver for field relay support.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2021-2022 Illinois Institute of Technology
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
#include "mx_process.h"
#include "mx_relay.h"
#include "d_field_relay.h"

/* Initialize the field relay driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_field_relay_record_function_list = {
	NULL,
	mxd_field_relay_create_record_structures,
	mxd_field_relay_finish_record_initialization
};

MX_RELAY_FUNCTION_LIST mxd_field_relay_relay_function_list = {
	mxd_field_relay_relay_command,
	mxd_field_relay_get_relay_status
};

MX_RECORD_FIELD_DEFAULTS mxd_field_relay_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RELAY_STANDARD_FIELDS,
	MXD_FIELD_RELAY_STANDARD_FIELDS
};

long mxd_field_relay_num_record_fields
	= sizeof( mxd_field_relay_record_field_defaults )
		/ sizeof( mxd_field_relay_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_field_relay_rfield_def_ptr
			= &mxd_field_relay_record_field_defaults[0];

/* ===== */

MX_EXPORT mx_status_type
mxd_field_relay_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_field_relay_create_record_structures()";

        MX_RELAY *relay;
        MX_FIELD_RELAY *field_relay;

        /* Allocate memory for the necessary structures. */

        relay = (MX_RELAY *) malloc(sizeof(MX_RELAY));

        if ( relay == (MX_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_RELAY structure." );
        }

        field_relay = (MX_FIELD_RELAY *)
				malloc( sizeof(MX_FIELD_RELAY) );

        if ( field_relay == (MX_FIELD_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_FIELD_RELAY structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = relay;
        record->record_type_struct = field_relay;
        record->class_specific_function_list
			= &mxd_field_relay_relay_function_list;

        relay->record = record;
	field_relay->record = record;

	relay->relay_command = MXF_RELAY_ILLEGAL_STATUS;
	relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_field_relay_finish_record_initialization( MX_RECORD *record )
{
        static const char fname[]
		= "mxd_field_relay_finish_record_initialization()";

        MX_FIELD_RELAY *field_relay;

        field_relay = (MX_FIELD_RELAY *) record->record_type_struct;

        if ( field_relay == (MX_FIELD_RELAY *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                	"MX_FIELD_RELAY pointer for record '%s' is NULL.",
			record->name);
        }

	field_relay->input_field =
		mx_get_record_field_by_name( record, field_relay->input_name );

	if ( field_relay->input_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested input field name '%s' for field relay '%s' "
		"is not found in the current database.",
			field_relay->input_name, record->name );
	}

	field_relay->output_field =
		mx_get_record_field_by_name( record, field_relay->output_name );

	if ( field_relay->output_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested output field name '%s' for field relay '%s' "
		"is not found in the current database.",
			field_relay->output_name, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_field_relay_relay_command( MX_RELAY *relay )
{
	static const char fname[] = "mxd_field_relay_relay_command()";

	MX_FIELD_RELAY *field_relay = NULL;
	void *field_value_ptr = NULL;
	unsigned long requested_value,old_value, new_value;
	unsigned long shifted_value, mask;
	int i;
	mx_status_type mx_status;

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RELAY pointer passed is NULL." );
	}

	field_relay = (MX_FIELD_RELAY *)
				relay->record->record_type_struct;

	if ( field_relay == (MX_FIELD_RELAY *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_FIELD_RELAY pointer for record '%s' is NULL.",
			relay->record->name );
	}

	/* Compute the mask for the output value. */

	mask = 0L;

	for ( i = 0; i < field_relay->num_output_bits; i++ ) {

		mask = mask << 1;

		mask |= 1L;
	}

	mask = mask << ( field_relay->output_bit_offset );

	/* Compute the new value for the output field. */

	old_value = relay->relay_command;

	new_value = old_value & ( ~mask );

	switch( relay->relay_command ) {
	case MXF_CLOSE_RELAY:
		requested_value = field_relay->close_output_value;
		break;
	case MXF_OPEN_RELAY:
		requested_value = field_relay->open_output_value;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The relay command %ld is unrecognized.",
			relay->relay_command );
	}

	if ( field_relay->frelay_flags & MXF_FRELAY_INVERT_OUTPUT ) {
		requested_value = ~requested_value;
	}

	shifted_value = requested_value << (field_relay->output_bit_offset);

	shifted_value &= mask;

	new_value = new_value | shifted_value;

	/* Get a pointer to where the new value goes. */

	field_value_ptr =
		mx_get_field_value_pointer( field_relay->output_field );

	if ( field_value_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The field value pointer for output field '%s' of "
		"relay '%s' is NULL.",
			field_relay->output_name,
			relay->record->name );
	}

	/* Copy the new value to the output field. */

	MX_DEBUG(-1,("%s: '%s' is sending %lx to '%s'", fname,
				relay->record->name, new_value,
				field_relay->output_name));

	/* Do the data copy now. */

	switch( field_relay->output_field->datatype ) {
	case MXFT_CHAR:
	case MXFT_UCHAR:
	case MXFT_INT8:
	case MXFT_UINT8:
		*( (uint8_t *) field_value_ptr ) = (uint8_t) new_value;
		break;

	case MXFT_SHORT:
	case MXFT_USHORT:
		*( (unsigned short *) field_value_ptr )
						= (unsigned short) new_value;
		break;

	case MXFT_INT16:
	case MXFT_UINT16:
		*( (uint16_t *) field_value_ptr ) = (uint16_t) new_value;
		break;

	case MXFT_BOOL:
		*( (uint32_t *) field_value_ptr ) = (uint32_t) new_value;
		break;

	case MXFT_INT32:
	case MXFT_UINT32:
		*( (uint32_t *) field_value_ptr ) = (uint32_t) new_value;
		break;

	case MXFT_LONG:
	case MXFT_ULONG:
	case MXFT_HEX:
		*( (unsigned long *) field_value_ptr )
						= (unsigned long) new_value;
		break;

	case MXFT_INT64:
	case MXFT_UINT64:
		*( (uint64_t *) field_value_ptr ) = (uint64_t) new_value;
		break;
	
	case MXFT_FLOAT:
	case MXFT_DOUBLE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Floating point output fields are not supported for "
		"output field '%s' used by relay '%s'.",
			field_relay->output_name, relay->record->name );
		break;

	case MXFT_RECORD:
	case MXFT_RECORDTYPE:
	case MXFT_INTERFACE:
	case MXFT_RECORD_FIELD:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Pointer fields are not supported for output field '%s' "
		"used by relay '%s'.",
			field_relay->output_name, relay->record->name );
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"MX fields of datatype %lu are not supported for "
		"output field '%s' used by relay '%s'.",
			field_relay->output_field->datatype,
			field_relay->output_name, relay->record->name );
		break;
	}

	/* Send the new value. */

	MX_DEBUG(-1,("%s: '%s' is sending %lx to '%s'", fname,
				relay->record->name, new_value,
				field_relay->output_name));

	mx_status = mx_process_record_field( field_relay->output_field->record,
						field_relay->output_field,
						NULL, MX_PROCESS_PUT, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

	relay->relay_command = new_value;

	/* Wait for the settling time before returning.
	 *
	 * Note that the settling time is specified in milliseconds.
	 */

	mx_msleep( field_relay->settling_time );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_field_relay_get_relay_status( MX_RELAY *relay )
{
	static const char fname[] = "mxd_field_relay_get_relay_status()";

	MX_FIELD_RELAY *field_relay = NULL;
	void *field_value_ptr = NULL;
	unsigned long current_value, mask;
	unsigned long closed_value, open_value;
	int i;
	mx_status_type mx_status;

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RELAY pointer passed is NULL." );
	}

	field_relay = (MX_FIELD_RELAY *)
				relay->record->record_type_struct;

	if ( field_relay == (MX_FIELD_RELAY *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_FIELD_RELAY pointer for record '%s' is NULL.",
			relay->record->name );
	}

	/* Get the current value of the digital input or output record that has
	 * the relay status in it.
	 */

	mx_status = mx_process_record_field( field_relay->output_field->record,
						field_relay->output_field,
						NULL, MX_PROCESS_GET, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get a pointer to where the new status comes from. */

	field_value_ptr =
		mx_get_field_value_pointer( field_relay->input_field );

	if ( field_value_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The field value pointer for input field '%s' of "
		"relay '%s' is NULL.",
			field_relay->input_name,
			field_relay->record->name );
	}

	/* Do the data copy now. */

	switch( field_relay->input_field->datatype ) {
	case MXFT_CHAR:
	case MXFT_UCHAR:
	case MXFT_INT8:
	case MXFT_UINT8:
		current_value = *( (uint8_t *) field_value_ptr );
		break;

	case MXFT_SHORT:
	case MXFT_USHORT:
		current_value = *( (unsigned short *) field_value_ptr );
		break;

	case MXFT_BOOL:
		current_value = *( (uint32_t *) field_value_ptr );
		break;

	case MXFT_LONG:
	case MXFT_ULONG:
	case MXFT_HEX:
		current_value = *( (unsigned long *) field_value_ptr );
		break;

	case MXFT_INT64:
	case MXFT_UINT64:
		current_value = *( (uint64_t *) field_value_ptr );
		break;
	
	case MXFT_FLOAT:
	case MXFT_DOUBLE:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Floating point input fields are not supported for "
		"input field '%s' used by relay '%s'.",
			field_relay->input_name, relay->record->name );
		break;

	case MXFT_RECORD:
	case MXFT_RECORDTYPE:
	case MXFT_INTERFACE:
	case MXFT_RECORD_FIELD:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Pointer fields are not supported for input field '%s' "
		"used by relay '%s'.",
			field_relay->input_name, relay->record->name );
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"MX fields of datatype %lu are not supported for "
		"input field '%s' used by relay '%s'.",
			field_relay->input_field->datatype,
			field_relay->input_name, relay->record->name );
		break;
	}

	if ( field_relay->frelay_flags & MXF_FRELAY_INVERT_INPUT ) {
		current_value = ~current_value;
	}

	/* Compute the mask for the input value. */

	mask = 0L;

	for ( i = 0; i < field_relay->num_input_bits; i++ ) {

		mask = mask << 1;

		mask |= 1L;
	}

	/* Mask off all bits but the ones we are interested in. */

	current_value = current_value >> ( field_relay->input_bit_offset );

	current_value = current_value & mask;

	/* Check to see if this is a valid relay status. */

	closed_value = field_relay->closed_input_value;
	open_value   = field_relay->open_input_value;

	MX_DEBUG( 1,
	("%s: '%s' current_value = %lx, closed = %lx, open = %lx, flags = %lx",
		fname, relay->record->name, current_value,
		closed_value, open_value, field_relay->frelay_flags));

	if ( field_relay->frelay_flags & MXF_FRELAY_IGNORE_STATUS ) {

		relay->relay_status = relay->relay_command;

	} else
	if ( field_relay->frelay_flags & MXF_FRELAY_ONLY_CHECK_CLOSED ) {

		if ( current_value == closed_value ) {
			relay->relay_status = MXF_RELAY_IS_CLOSED;
		} else {
			relay->relay_status = MXF_RELAY_IS_OPEN;
		}

	} else
	if ( field_relay->frelay_flags & MXF_FRELAY_ONLY_CHECK_OPEN ) {

		if ( current_value == open_value ) {
			relay->relay_status = MXF_RELAY_IS_OPEN;
		} else {
			relay->relay_status = MXF_RELAY_IS_CLOSED;
		}
	} else {
		if ( current_value == closed_value ) {
			relay->relay_status = MXF_RELAY_IS_CLOSED;

		} else if ( current_value == open_value ) {
			relay->relay_status = MXF_RELAY_IS_OPEN;

		} else {

			relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

			if ( field_relay->frelay_flags &
				MXF_FRELAY_SUPPRESS_ILLEGAL_STATUS_MESSAGE )
			{
			    return MX_SUCCESSFUL_RESULT;
			} else {
			    return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
				"Relay '%s' has illegal position status %#lx",
				relay->record->name, current_value );
			}
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

