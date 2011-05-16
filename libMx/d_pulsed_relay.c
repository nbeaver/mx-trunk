/*
 * Name:    d_pulsed_relay.c
 *
 * Purpose: MX driver for pulsed relay support.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004-2006, 2011 Illinois Institute of Technology
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
#include "mx_hrt.h"
#include "mx_relay.h"
#include "d_pulsed_relay.h"

/* Initialize the pulsed relay driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_pulsed_relay_record_function_list = {
	NULL,
	mxd_pulsed_relay_create_record_structures,
	mxd_pulsed_relay_finish_record_initialization
};

MX_RELAY_FUNCTION_LIST mxd_pulsed_relay_relay_function_list = {
	mxd_pulsed_relay_relay_command,
	mxd_pulsed_relay_get_relay_status
};

MX_RECORD_FIELD_DEFAULTS mxd_pulsed_relay_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RELAY_STANDARD_FIELDS,
	MXD_PULSED_RELAY_STANDARD_FIELDS
};

long mxd_pulsed_relay_num_record_fields
	= sizeof( mxd_pulsed_relay_record_field_defaults )
		/ sizeof( mxd_pulsed_relay_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pulsed_relay_rfield_def_ptr
			= &mxd_pulsed_relay_record_field_defaults[0];

/* ===== */

MX_EXPORT mx_status_type
mxd_pulsed_relay_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_pulsed_relay_create_record_structures()";

        MX_RELAY *relay;
        MX_PULSED_RELAY *pulsed_relay;

        /* Allocate memory for the necessary structures. */

        relay = (MX_RELAY *) malloc(sizeof(MX_RELAY));

        if ( relay == (MX_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_RELAY structure." );
        }

        pulsed_relay = (MX_PULSED_RELAY *)
				malloc( sizeof(MX_PULSED_RELAY) );

        if ( pulsed_relay == (MX_PULSED_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_PULSED_RELAY structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = relay;
        record->record_type_struct = pulsed_relay;
        record->class_specific_function_list
			= &mxd_pulsed_relay_relay_function_list;

        relay->record = record;

	relay->relay_command = MXF_RELAY_ILLEGAL_STATUS;
	relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pulsed_relay_finish_record_initialization( MX_RECORD *record )
{
        static const char fname[]
		= "mxd_pulsed_relay_finish_record_initialization()";

        MX_PULSED_RELAY *pulsed_relay;

        pulsed_relay = (MX_PULSED_RELAY *) record->record_type_struct;

        if ( pulsed_relay == (MX_PULSED_RELAY *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                	"MX_PULSED_RELAY pointer for record '%s' is NULL.",
			record->name);
        }

	/* Verify that 'input_record' is a digital input or output record. */

        if ( pulsed_relay->input_record->mx_superclass != MXR_DEVICE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
                "Error in record '%s': '%s' is not an device record.",
			record->name,
			pulsed_relay->input_record->name );
        }
        if ( ( pulsed_relay->input_record->mx_class != MXC_DIGITAL_INPUT )
          && ( pulsed_relay->input_record->mx_class != MXC_DIGITAL_OUTPUT ) )
	{
                return mx_error( MXE_TYPE_MISMATCH, fname,
	"Error in record '%s': '%s' is not a digital input or output record.",
			record->name,
			pulsed_relay->input_record->name );
        }

	/* Verify that 'output_record' is a digital output record. */

        if ( pulsed_relay->output_record->mx_superclass != MXR_DEVICE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
		"Error in record '%s': '%s' is not an device record.",
			record->name,
			pulsed_relay->output_record->name );
        }
        if ( pulsed_relay->output_record->mx_class != MXC_DIGITAL_OUTPUT ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
		"Error in record '%s': '%s' is not a digital output record.",
			record->name,
			pulsed_relay->output_record->name );
        }

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pulsed_relay_relay_command( MX_RELAY *relay )
{
	static const char fname[] = "mxd_pulsed_relay_relay_command()";

	MX_PULSED_RELAY *pulsed_relay;
	unsigned long requested_start_value, requested_finish_value;
	unsigned long new_start_value, new_finish_value;
	unsigned long shifted_start_value, shifted_finish_value;
	unsigned long old_value, mask;
	int i;
	mx_status_type mx_status;

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RELAY pointer passed is NULL." );
	}

	pulsed_relay = (MX_PULSED_RELAY *)
				relay->record->record_type_struct;

	if ( pulsed_relay == (MX_PULSED_RELAY *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PULSED_RELAY pointer for record '%s' is NULL.",
			relay->record->name );
	}

	/* Compute the mask for the output value. */

	mask = 0L;

	for ( i = 0; i < pulsed_relay->num_output_bits; i++ ) {

		mask = mask << 1;

		mask |= 1L;
	}

	mask = mask << ( pulsed_relay->output_bit_offset );

	/* Get the current value of the output record. */

	mx_status = mx_digital_output_read( pulsed_relay->output_record,
						&old_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the new values for the output record. */

	new_start_value  = old_value & ( ~mask );
	new_finish_value = old_value & ( ~mask );

	switch( relay->relay_command ) {
	case MXF_CLOSE_RELAY:
		requested_start_value  = pulsed_relay->close_output_value;
		requested_finish_value = pulsed_relay->close_idle_value;
		break;
	case MXF_OPEN_RELAY:
		requested_start_value  = pulsed_relay->open_output_value;
		requested_finish_value = pulsed_relay->open_idle_value;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The relay command %ld is unrecognized.",
			relay->relay_command );
	}

	if ( pulsed_relay->prelay_flags & MXF_PRELAY_INVERT_OUTPUT ) {
		requested_start_value  = ~requested_start_value;
		requested_finish_value = ~requested_finish_value;
	}

	shifted_start_value =
		requested_start_value << (pulsed_relay->output_bit_offset);

	shifted_start_value &= mask;

	new_start_value = new_start_value | shifted_start_value;

	shifted_finish_value =
		requested_finish_value << (pulsed_relay->output_bit_offset);

	shifted_finish_value &= mask;

	new_finish_value = new_finish_value | shifted_finish_value;

	/* Send the start value. */

	MX_DEBUG( 1,("%s: '%s' is sending start value %lx to '%s'", fname,
				relay->record->name, new_start_value,
				pulsed_relay->output_record->name));

	mx_status = mx_digital_output_write( pulsed_relay->output_record,
						new_start_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the pulse duration in milliseconds before sending the
	 * finish value.  By setting a flag bit, you can choose either to
	 * use mx_msleep() or mx_udelay() for your wait.  mx_udelay() is
	 * much less likely to be preempted by the operating system for a
	 * task switch than mx_msleep() is.
	 */

	if ( pulsed_relay->prelay_flags & MXF_PRELAY_USE_MX_UDELAY ) {
		mx_udelay( 1000 * pulsed_relay->pulse_time );
	} else {
		mx_msleep( pulsed_relay->pulse_time );
	}

	/* Send the finish value. */

	MX_DEBUG( 1,("%s: '%s' is sending finish value %lx to '%s'", fname,
				relay->record->name, new_finish_value,
				pulsed_relay->output_record->name));

	mx_status = mx_digital_output_write( pulsed_relay->output_record,
						new_finish_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the settling time before returning.
	 *
	 * Note that the settling time is specified in milliseconds.
	 */

	mx_msleep( pulsed_relay->settling_time );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pulsed_relay_get_relay_status( MX_RELAY *relay )
{
	static const char fname[] = "mxd_pulsed_relay_get_relay_status()";

	MX_PULSED_RELAY *pulsed_relay;
	unsigned long current_value, mask;
	unsigned long closed_value, open_value;
	int i;
	mx_status_type mx_status;

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RELAY pointer passed is NULL." );
	}

	pulsed_relay = (MX_PULSED_RELAY *)
				relay->record->record_type_struct;

	if ( pulsed_relay == (MX_PULSED_RELAY *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PULSED_RELAY pointer for record '%s' is NULL.",
			relay->record->name );
	}

	/* Get the current value of the digital input or output record that has
	 * the relay status in it.
	 */

	switch( pulsed_relay->input_record->mx_class ) {
	case MXC_DIGITAL_INPUT:
		mx_status = mx_digital_input_read( pulsed_relay->input_record,
						&current_value );
		break;
	case MXC_DIGITAL_OUTPUT:
		mx_status = mx_digital_output_read( pulsed_relay->input_record,
						&current_value );
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Input record '%s' for pulsed relay '%s' is not a "
		"digital input or output record.",
			pulsed_relay->input_record->name,
			relay->record->name );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pulsed_relay->prelay_flags & MXF_PRELAY_INVERT_INPUT ) {
		current_value = ~current_value;
	}

	/* Compute the mask for the input value. */

	mask = 0L;

	for ( i = 0; i < pulsed_relay->num_input_bits; i++ ) {

		mask = mask << 1;

		mask |= 1L;
	}

	/* Mask off all bits but the ones we are interested in. */

	current_value = current_value >> ( pulsed_relay->input_bit_offset );

	current_value = current_value & mask;

	/* Check to see if this is a valid relay status. */

	closed_value = pulsed_relay->closed_input_value;
	open_value   = pulsed_relay->open_input_value;

	MX_DEBUG( 1,
	("%s: '%s' current_value = %lx, closed = %lx, open = %lx, flags = %lx",
		fname, relay->record->name, current_value,
		closed_value, open_value, pulsed_relay->prelay_flags));

	if ( pulsed_relay->prelay_flags & MXF_PRELAY_IGNORE_STATUS ) {

		relay->relay_status = relay->relay_command;

	} else
	if ( pulsed_relay->prelay_flags & MXF_PRELAY_ONLY_CHECK_CLOSED ) {

		if ( current_value == closed_value ) {
			relay->relay_status = MXF_RELAY_IS_CLOSED;
		} else {
			relay->relay_status = MXF_RELAY_IS_OPEN;
		}

	} else
	if ( pulsed_relay->prelay_flags & MXF_PRELAY_ONLY_CHECK_OPEN ) {

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

			if ( pulsed_relay->prelay_flags &
				MXF_PRELAY_SUPPRESS_ILLEGAL_STATUS_MESSAGE )
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

