/*
 * Name:    d_flowbus_valve.c
 *
 * Purpose: MX driver for Flowbus valves.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2021 Illinois Institute of Technology
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
#include "mx_relay.h"
#include "i_flowbus.h"
#include "d_flowbus_valve.h"

/* Initialize the field relay driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_flowbus_valve_record_function_list = {
	NULL,
	mxd_flowbus_valve_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_flowbus_valve_open
};

MX_RELAY_FUNCTION_LIST mxd_flowbus_valve_relay_function_list = {
	mxd_flowbus_valve_relay_command,
	mxd_flowbus_valve_get_relay_status
};

MX_RECORD_FIELD_DEFAULTS mxd_flowbus_valve_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RELAY_STANDARD_FIELDS,
	MXD_FLOWBUS_VALVE_STANDARD_FIELDS
};

long mxd_flowbus_valve_num_record_fields
	= sizeof( mxd_flowbus_valve_record_field_defaults )
		/ sizeof( mxd_flowbus_valve_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_flowbus_valve_rfield_def_ptr
			= &mxd_flowbus_valve_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_flowbus_valve_get_pointers( MX_RELAY *relay,
		      MX_FLOWBUS_VALVE **flowbus_valve,
		      MX_FLOWBUS **flowbus,
		      const char *calling_fname )
{
	MX_FLOWBUS_VALVE *flowbus_valve_ptr;
	MX_RECORD *flowbus_record;

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
		"The MX_RELAY pointer passed is NULL." );
	}

	flowbus_valve_ptr = (MX_FLOWBUS_VALVE *)
				relay->record->record_type_struct;

	if ( flowbus_valve_ptr == (MX_FLOWBUS_VALVE *) NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"The MX_FLOWBUS_VALVE pointer "
		"for record '%s' is NULL.",
			relay->record->name );
	}

	if ( flowbus_valve != (MX_FLOWBUS_VALVE **) NULL )
	{
		*flowbus_valve = flowbus_valve_ptr;
	}

	if ( flowbus != (MX_FLOWBUS **) NULL ) {

		flowbus_record = flowbus_valve_ptr->flowbus_record;

		if ( flowbus_record == (MX_RECORD *) NULL ) {
			return mx_error(
			MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
			"The flowbus_record pointer used by "
			"relay record '%s' is NULL.",
				relay->record->name );
		}

		*flowbus = (MX_FLOWBUS *) flowbus_record->record_type_struct;

		if ( *flowbus == (MX_FLOWBUS *) NULL ) {
			return mx_error(
			MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
			"The MX_FLOWBUS pointer for "
			"Synaccess Netbooter record '%s' used by "
			"relay record '%s' is NULL.",
				flowbus_record->name,
				relay->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_flowbus_valve_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_flowbus_valve_create_record_structures()";

        MX_RELAY *relay;
        MX_FLOWBUS_VALVE *flowbus_valve;

        /* Allocate memory for the necessary structures. */

        relay = (MX_RELAY *) malloc(sizeof(MX_RELAY));

        if ( relay == (MX_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_RELAY structure." );
        }

        flowbus_valve = (MX_FLOWBUS_VALVE *)
				malloc( sizeof(MX_FLOWBUS_VALVE) );

        if ( flowbus_valve == (MX_FLOWBUS_VALVE *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_FLOWBUS_VALVE structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = relay;
        record->record_type_struct = flowbus_valve;
        record->class_specific_function_list
			= &mxd_flowbus_valve_relay_function_list;

        relay->record = record;
	flowbus_valve->record = record;

	relay->relay_command = MXF_RELAY_ILLEGAL_STATUS;
	relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_flowbus_valve_open( MX_RECORD *record )
{
        static const char fname[] = "mxd_flowbus_valve_open()";

	MX_RELAY *relay = NULL;
        MX_FLOWBUS_VALVE *flowbus_valve = NULL;
        MX_FLOWBUS *flowbus = NULL;
	mx_status_type mx_status;

        relay = (MX_RELAY *) record->record_class_struct;

        if ( relay == (MX_RELAY *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                	"MX_RELAY pointer for relay '%s' is NULL.",
			record->name);
        }

	mx_status = mxd_flowbus_valve_get_pointers( relay,
		      &flowbus_valve, &flowbus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configuration values for the "Control Mode" parameter. */

	flowbus_valve->process_number = 1;
	flowbus_valve->parameter_number = 4;
	flowbus_valve->parameter_type = MXDT_FLOWBUS_UINT8;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_flowbus_valve_relay_command( MX_RELAY *relay )
{
	static const char fname[] = "mxd_flowbus_valve_relay_command()";

	MX_FLOWBUS_VALVE *flowbus_valve = NULL;
        MX_FLOWBUS *flowbus = NULL;
	unsigned short flowbus_control_mode;
	char parameter_name[80];
	mx_status_type mx_status;

	mx_status = mxd_flowbus_valve_get_pointers( relay,
		      &flowbus_valve, &flowbus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( relay->relay_command ) {
	case MXF_OPEN_RELAY:
		flowbus_control_mode = 0;
		break;
	case MXF_CLOSE_RELAY:
		flowbus_control_mode = 3;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "Illegal MX relay command %lu requested for Flowbus valve '%s'.",
			relay->relay_command, relay->record->name );
		break;
	}

	snprintf( parameter_name, sizeof(parameter_name),
		"Process %lu, Parameter %lu",
		flowbus_valve->process_number,
		flowbus_valve->parameter_number );

	mx_status = mxi_flowbus_send_parameter( flowbus,
					flowbus_valve->node_address,
					parameter_name,
					flowbus_valve->process_number,
					flowbus_valve->parameter_number,
					flowbus_valve->parameter_type,
					&flowbus_control_mode,
					NULL, 0, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_flowbus_valve_get_relay_status( MX_RELAY *relay )
{
	static const char fname[] = "mxd_flowbus_valve_get_relay_status()";

	MX_FLOWBUS_VALVE *flowbus_valve = NULL;
        MX_FLOWBUS *flowbus = NULL;
	unsigned short flowbus_control_mode;
	char parameter_name[80];
	int i, max_attempts;
	mx_status_type mx_status;

	mx_status = mxd_flowbus_valve_get_pointers( relay,
		      &flowbus_valve, &flowbus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( parameter_name, sizeof(parameter_name),
		"Process %lu, Parameter %lu",
		flowbus_valve->process_number,
		flowbus_valve->parameter_number );

	max_attempts = 10;

	for ( i = 0; i < max_attempts; i++ ) {

		mx_status = mxi_flowbus_request_parameter( flowbus,
					flowbus_valve->node_address,
					parameter_name,
					flowbus_valve->process_number,
					flowbus_valve->parameter_number,
					flowbus_valve->parameter_type,
					&flowbus_control_mode,
					sizeof( unsigned short ), 0 );

		if ( mx_status.code == MXE_SUCCESS )
			break;			/* Exit the for() loop. */

		if ( mx_status.code != MXE_TIMED_OUT )
			return mx_status;

		mx_status = mxi_flowbus_timeout_recovery( flowbus );

		if ( mx_status.code == MXE_SUCCESS )
			break;			/* Exit the for() loop. */

		mx_msleep(100);
	}

	if ( i >= max_attempts ) {
		return mx_error( MXE_TIMED_OUT, fname,
		"Failed to read the value of relay '%s' after %d attempts.",
			relay->record->name, max_attempts );
	}

	switch( flowbus_control_mode ) {
	case 0:
		relay->relay_status = MXF_RELAY_IS_OPEN;
		break;
	case 3:
		relay->relay_status = MXF_RELAY_IS_CLOSED;
		break;
	default:
		relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

		mx_warning( "The control mode %u for Flowbus valve '%s' "
			"is not a state that it should be in.  "
			"Normally, the only allowed control modes are 0 and 3.",
				flowbus_control_mode, relay->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

