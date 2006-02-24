/*
 * Name:    d_blind_relay.c
 *
 * Purpose: MX driver for blind relay support.  A blind relay does not have
 *          a way to read its present setting.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001, 2004-2006 Illinois Institute of Technology
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
#include "mx_digital_output.h"
#include "mx_relay.h"
#include "d_blind_relay.h"

/* Initialize the generic relay driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_blind_relay_record_function_list = {
	mxd_blind_relay_initialize_type,
	mxd_blind_relay_create_record_structures,
	mxd_blind_relay_finish_record_initialization,
	mxd_blind_relay_delete_record,
	NULL,
	mxd_blind_relay_read_parms_from_hardware,
	mxd_blind_relay_write_parms_to_hardware,
	mxd_blind_relay_open,
	mxd_blind_relay_close,
	NULL
};

MX_RELAY_FUNCTION_LIST mxd_blind_relay_relay_function_list = {
	mxd_blind_relay_relay_command,
	mxd_blind_relay_get_relay_status
};

MX_RECORD_FIELD_DEFAULTS mxd_blind_relay_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RELAY_STANDARD_FIELDS,
	MXD_BLIND_RELAY_STANDARD_FIELDS
};

long mxd_blind_relay_num_record_fields
	= sizeof( mxd_blind_relay_record_field_defaults )
		/ sizeof( mxd_blind_relay_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_blind_relay_rfield_def_ptr
			= &mxd_blind_relay_record_field_defaults[0];

/* ===== */

MX_EXPORT mx_status_type
mxd_blind_relay_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_blind_relay_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_blind_relay_create_record_structures()";

        MX_RELAY *relay;
        MX_BLIND_RELAY *blind_relay;

        /* Allocate memory for the necessary structures. */

        relay = (MX_RELAY *) malloc(sizeof(MX_RELAY));

        if ( relay == (MX_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_RELAY structure." );
        }

        blind_relay = (MX_BLIND_RELAY *)
				malloc( sizeof(MX_BLIND_RELAY) );

        if ( blind_relay == (MX_BLIND_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_BLIND_RELAY structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = relay;
        record->record_type_struct = blind_relay;
        record->class_specific_function_list
			= &mxd_blind_relay_relay_function_list;

        relay->record = record;

	relay->relay_command = MXF_RELAY_ILLEGAL_STATUS;
	relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_blind_relay_finish_record_initialization( MX_RECORD *record )
{
        const char fname[]
		= "mxd_blind_relay_finish_record_initialization()";

	MX_RELAY *relay;
        MX_BLIND_RELAY *blind_relay;

	relay = (MX_RELAY *) record->record_class_struct;

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RELAY pointer for record '%s' is NULL.",
			record->name);
	}

	relay->relay_command = MXF_OPEN_RELAY;

        blind_relay = (MX_BLIND_RELAY *) record->record_type_struct;

        if ( blind_relay == (MX_BLIND_RELAY *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                	"MX_BLIND_RELAY pointer for record '%s' is NULL.",
			record->name);
        }

	/* Verify that 'output_record' is a digital output record. */

        if ( blind_relay->output_record->mx_superclass != MXR_DEVICE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
		"Error in record '%s': '%s' is not an device record.",
			record->name,
			blind_relay->output_record->name );
        }
        if ( blind_relay->output_record->mx_class != MXC_DIGITAL_OUTPUT ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
		"Error in record '%s': '%s' is not a digital output record.",
			record->name,
			blind_relay->output_record->name );
        }

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_blind_relay_delete_record( MX_RECORD *record )
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
mxd_blind_relay_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_blind_relay_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_blind_relay_open( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_blind_relay_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_blind_relay_relay_command( MX_RELAY *relay )
{
	const char fname[] = "mxd_blind_relay_relay_command()";

	MX_BLIND_RELAY *blind_relay;
	unsigned long old_value, new_value, shifted_value, mask;
	int i;
	mx_status_type status;

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RELAY pointer passed is NULL." );
	}

	blind_relay = (MX_BLIND_RELAY *)
				relay->record->record_type_struct;

	if ( blind_relay == (MX_BLIND_RELAY *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_BLIND_RELAY pointer for record '%s' is NULL.",
			relay->record->name );
	}

	/* Compute the mask for the output value. */

	mask = 0L;

	for ( i = 0; i < blind_relay->num_output_bits; i++ ) {

		mask = mask << 1;

		mask |= 1L;
	}

	mask = mask << ( blind_relay->output_bit_offset );

	/* Get the current value of the output record. */

	status = mx_digital_output_read( blind_relay->output_record,
						&old_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Compute the new value for the output record. */

	new_value = old_value & ( ~mask );

	switch( relay->relay_command ) {
	case MXF_CLOSE_RELAY:
		shifted_value = ( blind_relay->close_output_value )
				<< ( blind_relay->output_bit_offset );
		break;
	case MXF_OPEN_RELAY:
		shifted_value = ( blind_relay->open_output_value )
				<< ( blind_relay->output_bit_offset );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The relay command %ld is unrecognized.",
			relay->relay_command );
	}

	new_value = new_value | shifted_value;

	/* Send the new value. */

	status = mx_digital_output_write( blind_relay->output_record,
						new_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Wait for the settling time before returning.
	 *
	 * Note that the settling time is specified in milliseconds.
	 */

	mx_msleep( blind_relay->settling_time );

	return status;
}

MX_EXPORT mx_status_type
mxd_blind_relay_get_relay_status( MX_RELAY *relay )
{
	const char fname[] = "mxd_blind_relay_get_relay_status()";

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RELAY pointer passed is NULL." );
	}

	if ( relay->relay_command == MXF_CLOSE_RELAY ) {
		relay->relay_status = MXF_RELAY_IS_CLOSED;

	} else if ( relay->relay_command == MXF_OPEN_RELAY ) {
		relay->relay_status = MXF_RELAY_IS_OPEN;

	} else {
		relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Last relay command %ld for blind relay '%s' was illegal.",
			relay->relay_command, relay->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

