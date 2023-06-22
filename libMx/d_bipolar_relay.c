/*
 * Name:    d_bipolar_relay.c
 *
 * Purpose: MX driver for bipolar relay support.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2023 Illinois Institute of Technology
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
#include "mx_analog_output.h"
#include "mx_digital_output.h"
#include "mx_relay.h"
#include "d_bipolar_relay.h"

/* Initialize the generic relay driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_bipolar_relay_record_function_list = {
	NULL,
	mxd_bipolar_relay_3reate_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_bipolar_relay_open
};

MX_RELAY_FUNCTION_LIST mxd_bipolar_relay_relay_function_list = {
	mxd_bipolar_relay_relay_command,
	mxd_bipolar_relay_get_relay_status
};

MX_RECORD_FIELD_DEFAULTS mxd_bipolar_relay_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RELAY_STANDARD_FIELDS,
	MXD_BIPOLAR_RELAY_STANDARD_FIELDS
};

long mxd_bipolar_relay_num_record_fields
	= sizeof( mxd_bipolar_relay_record_field_defaults )
		/ sizeof( mxd_bipolar_relay_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bipolar_relay_rfield_def_ptr
			= &mxd_bipolar_relay_record_field_defaults[0];

/* ===== */

MX_EXPORT mx_status_type
mxd_bipolar_relay_3reate_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_bipolar_relay_3reate_record_structures()";

        MX_RELAY *relay;
        MX_BIPOLAR_RELAY *bipolar_relay;

        relay = (MX_RELAY *) malloc(sizeof(MX_RELAY));

        if ( relay == (MX_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_RELAY structure." );
        }

        bipolar_relay = (MX_BIPOLAR_RELAY *)
				malloc( sizeof(MX_BIPOLAR_RELAY) );

        if ( bipolar_relay == (MX_BIPOLAR_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_BIPOLAR_RELAY structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = relay;
        record->record_type_struct = bipolar_relay;
        record->class_specific_function_list
			= &mxd_bipolar_relay_relay_function_list;

        relay->record = record;
	bipolar_relay->record = record;

	relay->relay_command = MXF_RELAY_ILLEGAL_STATUS;
	relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

        return MX_SUCCESSFUL_RESULT;
}

/* This function allows for the possibility that the low level records,
 * are actually digital outputs.
 */

static mx_status_type
mxd_bipolar_relay_raw_command( MX_RECORD *raw_record, long relay_command )
{
        static const char fname[] = "mxd_bipolar_relay_raw_command()";

	long raw_command;
	mx_status_type mx_status;

	switch( raw_record->mx_class ) {
	case MXC_RELAY:
		mx_status = mx_relay_command( raw_record, relay_command );
		break;
	case MXC_DIGITAL_OUTPUT:
		if ( relay_command == MXF_OPEN_RELAY ) {
			raw_command = 0;
		} else {
			raw_command = 1;
		}
		mx_status = mx_digital_output_write( raw_record, raw_command );
		break;
	default:
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Raw record %s is not a relay or digital output.",
			raw_record->name );
	}

	return mx_status;
}

/* Note: mxd_bipolar_relay_open_all_relays() opens the downstream
 * relays before opening the upstream one.  This is done to prevent
 * unwanted voltages from excaping to the relay output.
 */

static mx_status_type
mxd_bipolar_relay_open_all_relays( MX_BIPOLAR_RELAY *bipolar_relay )
{
	mx_status_type mx_status;

	mx_status = mxd_bipolar_relay_raw_command( 
		bipolar_relay->relay_3_record, MXF_OPEN_RELAY );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_bipolar_relay_raw_command( 
		bipolar_relay->relay_4_record, MXF_OPEN_RELAY );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_bipolar_relay_raw_command( 
		bipolar_relay->relay_1_record, MXF_OPEN_RELAY );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_bipolar_relay_raw_command( 
		bipolar_relay->relay_2_record, MXF_OPEN_RELAY );

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mxd_bipolar_relay_open( MX_RECORD *record )
{
        static const char fname[] = "mxd_bipolar_relay_open()";

	MX_RELAY *relay;
        MX_BIPOLAR_RELAY *bipolar_relay;
	mx_status_type mx_status;

	relay = (MX_RELAY *) record->record_class_struct;

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RELAY pointer for record '%s' is NULL.",
			record->name);
	}

	relay->relay_command = MXF_OPEN_RELAY;

        bipolar_relay = (MX_BIPOLAR_RELAY *) record->record_type_struct;

        if ( bipolar_relay == (MX_BIPOLAR_RELAY *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                	"MX_BIPOLAR_RELAY pointer for record '%s' is NULL.",
			record->name);
        }

	/* Make sure that all the relays are open. */

	mx_status = mxd_bipolar_relay_open_all_relays( bipolar_relay );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bipolar_relay_relay_command( MX_RELAY *relay )
{
	static const char fname[] = "mxd_bipolar_relay_relay_command()";

	MX_BIPOLAR_RELAY *bipolar_relay;
	MX_RECORD *first_upstream_record,   *second_upstream_record;
	MX_RECORD *first_downstream_record, *second_downstream_record;
	mx_status_type mx_status;

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RELAY pointer passed is NULL." );
	}

	bipolar_relay = (MX_BIPOLAR_RELAY *)
				relay->record->record_type_struct;

	if ( bipolar_relay == (MX_BIPOLAR_RELAY *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_BIPOLAR_RELAY pointer for record '%s' is NULL.",
			relay->record->name );
	}

	relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

	if ( relay->relay_command == MXF_OPEN_RELAY ) {
		first_upstream_record    = bipolar_relay->relay_1_record;
		first_downstream_record  = bipolar_relay->relay_3_record;
		second_upstream_record   = bipolar_relay->relay_2_record;
		second_downstream_record = bipolar_relay->relay_4_record;
	} else {
		first_upstream_record    = bipolar_relay->relay_2_record;
		first_downstream_record  = bipolar_relay->relay_4_record;
		second_upstream_record   = bipolar_relay->relay_1_record;
		second_downstream_record = bipolar_relay->relay_3_record;
	}

	/* Make sure all relays are open. */

	mx_status = mxd_bipolar_relay_open_all_relays( bipolar_relay );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Generate the first half of the bipolar pulse. */

	MX_DEBUG(-2,("%s: Command first upstream '%s' to CLOSE",
			fname, first_upstream_record->name ));

	mx_status = mxd_bipolar_relay_raw_command( first_upstream_record, MXF_CLOSE_RELAY );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: Command first downstream '%s' to CLOSE",
			fname, first_downstream_record->name ));

	mx_status = mxd_bipolar_relay_raw_command( first_downstream_record,
							MXF_CLOSE_RELAY );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the first half of the bipolar pulse to finish. */

	MX_DEBUG(-2,("%s: First active wait of %lu",
		fname, bipolar_relay->first_active_ms ));

	mx_msleep( bipolar_relay->first_active_ms );

	/* Command all the relays to open. */

	mx_status = mxd_bipolar_relay_open_all_relays( bipolar_relay );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bipolar_relay->between_time_ms > 0 ) {
		/* If the between time is greater than 0, then wait for
		 * the between time to finish.
		 */

		MX_DEBUG(-2,("%s: Between time wait of %lu",
			fname, bipolar_relay->between_time_ms ));

		mx_msleep( bipolar_relay->between_time_ms );
	}

	/* Generate the second half of the bipolar pulse. */

	MX_DEBUG(-2,("%s: Command second upstream '%s' to CLOSE",
			fname, second_upstream_record->name ));

	mx_status = mxd_bipolar_relay_raw_command( second_upstream_record, MXF_CLOSE_RELAY );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: Command second downstream '%s' to CLOSE",
			fname, second_downstream_record->name ));

	mx_status = mxd_bipolar_relay_raw_command( second_downstream_record,
							MXF_CLOSE_RELAY );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;


	/* Wait for the second half of the bipolar pulse to finish. */

	MX_DEBUG(-2,("%s: Second active wait of %lu",
		fname, bipolar_relay->first_active_ms ));

	mx_msleep( bipolar_relay->second_active_ms );

	/* Command all the relays to open. */

	mx_status = mxd_bipolar_relay_open_all_relays( bipolar_relay );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the post-pulse idle time. */

	MX_DEBUG(-2,("%s: Settling time wait of %lu",
			fname, bipolar_relay->settling_time_ms ));

	mx_msleep( bipolar_relay->settling_time_ms );

	return MX_SUCCESSFUL_RESULT;;
}

MX_EXPORT mx_status_type
mxd_bipolar_relay_get_relay_status( MX_RELAY *relay )
{
	return MX_SUCCESSFUL_RESULT;
}

