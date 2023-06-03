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
	mxd_bipolar_relay_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_bipolar_relay_open
};

MX_RELAY_FUNCTION_LIST mxd_bipolar_relay_relay_function_list = {
	mxd_bipolar_relay_relay_command,
	NULL
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
mxd_bipolar_relay_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_bipolar_relay_create_record_structures()";

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

MX_EXPORT mx_status_type
mxd_bipolar_relay_open( MX_RECORD *record )
{
        static const char fname[] = "mxd_bipolar_relay_open()";

	MX_RELAY *relay;
        MX_BIPOLAR_RELAY *bipolar_relay;
	MX_RECORD *output1_record;
	MX_RECORD *output2_record;

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

	/* Verify that the output records have allowed combinations
	 * device classes.
	 */

	/* Verify that 'output_record' is a digital output record. */

	output1_record = bipolar_relay->output1_record;
	output2_record = bipolar_relay->output2_record;

	switch( output1_record->mx_class ) {
	case MXC_ANALOG_OUTPUT:
	case MXC_DIGITAL_OUTPUT:
		break;
	default:
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"Output 1 record '%s' is not an analog output "
		"or a digital output.",
			bipolar_relay->output1_record->name );
		break;
	}

	if ( output2_record->mx_class != output1_record->mx_class ) {
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR,fname,
		"Output 1 '%s' is an MX '%s', but Output 2 '%s' is _not_ "
		"an MX '%s. Instead Output 2 is an MX '%s'.  "
		"Both records must be of the same MX device class.",
			output1_record->name,
			mx_get_driver_class_name( output1_record ),
			output2_record->name,
			mx_get_driver_class_name( output1_record ),
			mx_get_driver_class_name( output2_record ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_bipolar_relay_output_write( MX_RECORD *output_record, double output_volts )
{
	static const char fname[] = "mxd_bipolar_relay_output_write()";

	mx_status_type mx_status;

	switch( output_record->mx_class ) {
	case MXC_ANALOG_OUTPUT:
		mx_status = mx_analog_output_write( output_record,
							output_volts );
		break;
	case MXC_DIGITAL_OUTPUT:
		mx_status = mx_digital_output_write( output_record,
						mx_round( output_volts ) );
		break;
	default:
		mx_status = mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"Output record '%s' is not an analog output or digital output",
			output_record->name );
		break;
	}
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bipolar_relay_relay_command( MX_RELAY *relay )
{
	static const char fname[] = "mxd_bipolar_relay_relay_command()";

	MX_BIPOLAR_RELAY *bipolar_relay;
	MX_RECORD *first_active_record, *second_active_record;
	double first_active_volts, first_idle_volts;
	double second_active_volts, second_idle_volts;
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
		first_active_record = bipolar_relay->output1_record;
		second_active_record = bipolar_relay->output2_record;

		first_active_volts = bipolar_relay->output1_active_volts;
		first_idle_volts = bipolar_relay->output1_idle_volts;

		second_active_volts = bipolar_relay->output2_active_volts;
		second_idle_volts = bipolar_relay->output2_idle_volts;
	} else {
		first_active_record = bipolar_relay->output2_record;
		second_active_record = bipolar_relay->output1_record;

		first_active_volts = bipolar_relay->output2_active_volts;
		first_idle_volts = bipolar_relay->output2_idle_volts;

		second_active_volts = bipolar_relay->output1_active_volts;
		second_idle_volts = bipolar_relay->output1_idle_volts;
	}

	if ( first_active_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The first active record pointer for record '%s' is NULL.",
			relay->record->name );
	}

	if ( second_active_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The second_active_record pointer for record '%s' is NULL.",
			relay->record->name );
	}

	/* Generate the first half of the bipolar pulse. */

	MX_DEBUG(-2,("%s: Setting first '%s' to %f",
		fname, first_active_record->name, first_active_volts ));

	mx_status = mxd_bipolar_relay_output_write( first_active_record,
							first_active_volts );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: Setting second '%s' to %f",
		fname, second_active_record->name, second_idle_volts ));

	mx_status = mxd_bipolar_relay_output_write( second_active_record,
							second_idle_volts );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the first half of the bipolar pulse to finish. */

	MX_DEBUG(-2,("%s: First active wait of %lu",
		fname, bipolar_relay->first_active_ms ));

	mx_msleep( bipolar_relay->first_active_ms );

	if ( bipolar_relay->between_time_ms > 0 ) {
		/* If the between time is greater than 0, then set the
		 * first active record back to idle.
		 */

		MX_DEBUG(-2,("%s: Setting first '%s' to %f and then wait",
			fname, first_active_record->name, first_idle_volts ));

		mx_status = mxd_bipolar_relay_output_write( first_active_record,
							first_idle_volts );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* And then wait for the between time to finish. */

		MX_DEBUG(-2,("%s: Between time wait of %lu",
			fname, bipolar_relay->between_time_ms ));

		mx_msleep( bipolar_relay->between_time_ms );
	} else {
		/* Otherwise set the first active record back to idle
		 * immediately.
		 */

		MX_DEBUG(-2,("%s: Setting first '%s' to %f without wait",
			fname, first_active_record->name, first_idle_volts ));

		mx_status = mxd_bipolar_relay_output_write( first_active_record,
							first_idle_volts );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Set the second record to active. */

	MX_DEBUG(-2,("%s: Setting second '%s' to %f",
		fname, second_active_record->name, second_active_volts ));

	mx_status = mxd_bipolar_relay_output_write( second_active_record,
							second_active_volts );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the second half of the bipolar pulse to finish. */

	MX_DEBUG(-2,("%s: Second active wait of %lu",
		fname, bipolar_relay->first_active_ms ));

	mx_msleep( bipolar_relay->second_active_ms );

	/* Set the second record back to idle. */

	mx_status = mxd_bipolar_relay_output_write( second_active_record,
							second_idle_volts );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the post-pulse idle time. */

	MX_DEBUG(-2,("%s: Settling time wait of %lu",
			fname, bipolar_relay->settling_time_ms ));

	mx_msleep( bipolar_relay->settling_time_ms );

	return mx_status;
}

