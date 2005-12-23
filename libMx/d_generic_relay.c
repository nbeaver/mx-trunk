/*
 * Name:    d_generic_relay.c
 *
 * Purpose: MX driver for generic relay support.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004-2005 Illinois Institute of Technology
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
#include "d_generic_relay.h"

/* Initialize the generic relay driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_generic_relay_record_function_list = {
	mxd_generic_relay_initialize_type,
	mxd_generic_relay_create_record_structures,
	mxd_generic_relay_finish_record_initialization,
	mxd_generic_relay_delete_record,
	NULL,
	mxd_generic_relay_read_parms_from_hardware,
	mxd_generic_relay_write_parms_to_hardware,
	mxd_generic_relay_open,
	mxd_generic_relay_close,
	NULL
};

MX_RELAY_FUNCTION_LIST mxd_generic_relay_relay_function_list = {
	mxd_generic_relay_relay_command,
	mxd_generic_relay_get_relay_status
};

MX_RECORD_FIELD_DEFAULTS mxd_generic_relay_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RELAY_STANDARD_FIELDS,
	MXD_GENERIC_RELAY_STANDARD_FIELDS
};

long mxd_generic_relay_num_record_fields
	= sizeof( mxd_generic_relay_record_field_defaults )
		/ sizeof( mxd_generic_relay_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_generic_relay_rfield_def_ptr
			= &mxd_generic_relay_record_field_defaults[0];

/* ===== */

MX_EXPORT mx_status_type
mxd_generic_relay_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_generic_relay_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_generic_relay_create_record_structures()";

        MX_RELAY *relay;
        MX_GENERIC_RELAY *generic_relay;

        /* Allocate memory for the necessary structures. */

        relay = (MX_RELAY *) malloc(sizeof(MX_RELAY));

        if ( relay == (MX_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_RELAY structure." );
        }

        generic_relay = (MX_GENERIC_RELAY *)
				malloc( sizeof(MX_GENERIC_RELAY) );

        if ( generic_relay == (MX_GENERIC_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_GENERIC_RELAY structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = relay;
        record->record_type_struct = generic_relay;
        record->class_specific_function_list
			= &mxd_generic_relay_relay_function_list;

        relay->record = record;

	relay->relay_command = MXF_RELAY_ILLEGAL_STATUS;
	relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_generic_relay_finish_record_initialization( MX_RECORD *record )
{
        const char fname[]
		= "mxd_generic_relay_finish_record_initialization()";

        MX_GENERIC_RELAY *generic_relay;

        generic_relay = (MX_GENERIC_RELAY *) record->record_type_struct;

        if ( generic_relay == (MX_GENERIC_RELAY *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
                	"MX_GENERIC_RELAY pointer for record '%s' is NULL.",
			record->name);
        }

	/* Verify that 'input_record' is a digital input or output record. */

        if ( generic_relay->input_record->mx_superclass != MXR_DEVICE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
                "Error in record '%s': '%s' is not an device record.",
			record->name,
			generic_relay->input_record->name );
        }
        if ( ( generic_relay->input_record->mx_class != MXC_DIGITAL_INPUT )
          && ( generic_relay->input_record->mx_class != MXC_DIGITAL_OUTPUT ) )
	{
                return mx_error( MXE_TYPE_MISMATCH, fname,
	"Error in record '%s': '%s' is not a digital input or output record.",
			record->name,
			generic_relay->input_record->name );
        }

	/* Verify that 'output_record' is a digital output record. */

        if ( generic_relay->output_record->mx_superclass != MXR_DEVICE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
		"Error in record '%s': '%s' is not an device record.",
			record->name,
			generic_relay->output_record->name );
        }
        if ( generic_relay->output_record->mx_class != MXC_DIGITAL_OUTPUT ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
		"Error in record '%s': '%s' is not a digital output record.",
			record->name,
			generic_relay->output_record->name );
        }

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_generic_relay_delete_record( MX_RECORD *record )
{
        if ( record == NULL ) {
                return MX_SUCCESSFUL_RESULT;
        }
        if ( record->record_type_struct != NULL ) {
                free( record->record_type_struct );

                record->record_type_struct = NULL;
        }
        if ( record->record_class_struct != NULL ) {
                free( record->record_class_struct );

                record->record_class_struct = NULL;
        }
        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_generic_relay_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_generic_relay_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_generic_relay_open( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_generic_relay_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_generic_relay_relay_command( MX_RELAY *relay )
{
	const char fname[] = "mxd_generic_relay_relay_command()";

	MX_GENERIC_RELAY *generic_relay;
	unsigned long requested_value,old_value, new_value;
	unsigned long shifted_value, mask;
	int i;
	mx_status_type status;

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RELAY pointer passed is NULL." );
	}

	generic_relay = (MX_GENERIC_RELAY *)
				relay->record->record_type_struct;

	if ( generic_relay == (MX_GENERIC_RELAY *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_GENERIC_RELAY pointer for record '%s' is NULL.",
			relay->record->name );
	}

	/* Compute the mask for the output value. */

	mask = 0L;

	for ( i = 0; i < generic_relay->num_output_bits; i++ ) {

		mask = mask << 1;

		mask |= 1L;
	}

	mask = mask << ( generic_relay->output_bit_offset );

	/* Get the current value of the output record. */

	status = mx_digital_output_read( generic_relay->output_record,
						&old_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Compute the new value for the output record. */

	new_value = old_value & ( ~mask );

	switch( relay->relay_command ) {
	case MXF_CLOSE_RELAY:
		requested_value = generic_relay->close_output_value;
		break;
	case MXF_OPEN_RELAY:
		requested_value = generic_relay->open_output_value;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The relay command %d is unrecognized.",
			relay->relay_command );
		break;
	}

	if ( generic_relay->grelay_flags & MXF_GRELAY_INVERT_OUTPUT ) {
		requested_value = ~requested_value;
	}

	shifted_value = requested_value << (generic_relay->output_bit_offset);

	shifted_value &= mask;

	new_value = new_value | shifted_value;

	/* Send the new value. */

	MX_DEBUG( 1,("%s: '%s' is sending %lx to '%s'", fname,
				relay->record->name, new_value,
				generic_relay->output_record->name));

	status = mx_digital_output_write( generic_relay->output_record,
						new_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Wait for the settling time before returning.
	 *
	 * Note that the settling time is specified in milliseconds.
	 */

	mx_msleep( generic_relay->settling_time );

	return status;
}

MX_EXPORT mx_status_type
mxd_generic_relay_get_relay_status( MX_RELAY *relay )
{
	const char fname[] = "mxd_generic_relay_get_relay_status()";

	MX_GENERIC_RELAY *generic_relay;
	unsigned long current_value, mask;
	unsigned long closed_value, open_value;
	int i;
	mx_status_type status;

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RELAY pointer passed is NULL." );
	}

	generic_relay = (MX_GENERIC_RELAY *)
				relay->record->record_type_struct;

	if ( generic_relay == (MX_GENERIC_RELAY *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_GENERIC_RELAY pointer for record '%s' is NULL.",
			relay->record->name );
	}

	/* Get the current value of the digital input or output record that has
	 * the relay status in it.
	 */

	switch( generic_relay->input_record->mx_class ) {
	case MXC_DIGITAL_INPUT:
		status = mx_digital_input_read( generic_relay->input_record,
						&current_value );
		break;
	case MXC_DIGITAL_OUTPUT:
		status = mx_digital_output_read( generic_relay->input_record,
						&current_value );
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Input record '%s' for generic relay '%s' is not a "
		"digital input or output record.",
			generic_relay->input_record->name,
			relay->record->name );
		break;
	}

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( generic_relay->grelay_flags & MXF_GRELAY_INVERT_INPUT ) {
		current_value = ~current_value;
	}

	/* Compute the mask for the input value. */

	mask = 0L;

	for ( i = 0; i < generic_relay->num_input_bits; i++ ) {

		mask = mask << 1;

		mask |= 1L;
	}

	/* Mask off all bits but the ones we are interested in. */

	current_value = current_value >> ( generic_relay->input_bit_offset );

	current_value = current_value & mask;

	/* Check to see if this is a valid relay status. */

	closed_value = generic_relay->closed_input_value;
	open_value   = generic_relay->open_input_value;

	MX_DEBUG( 1,
	("%s: '%s' current_value = %lx, closed = %lx, open = %lx, flags = %lx",
		fname, relay->record->name, current_value,
		closed_value, open_value, generic_relay->grelay_flags));

	if ( generic_relay->grelay_flags & MXF_GRELAY_IGNORE_STATUS ) {

		relay->relay_status = relay->relay_command;

	} else
	if ( generic_relay->grelay_flags & MXF_GRELAY_ONLY_CHECK_CLOSED ) {

		if ( current_value == closed_value ) {
			relay->relay_status = MXF_RELAY_IS_CLOSED;
		} else {
			relay->relay_status = MXF_RELAY_IS_OPEN;
		}

	} else
	if ( generic_relay->grelay_flags & MXF_GRELAY_ONLY_CHECK_OPEN ) {

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

			if ( generic_relay->grelay_flags &
				MXF_GRELAY_SUPPRESS_ILLEGAL_STATUS_MESSAGE )
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

