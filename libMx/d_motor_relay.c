/*
 * Name:    d_motor_relay.c
 *
 * Purpose: MX relay driver for using an MX motor to drive a mechanical relay.
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

#define MXD_MOTOR_RELAY_DEBUG 	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_relay.h"
#include "mx_motor.h"
#include "d_motor_relay.h"

/* Initialize the generic relay driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_motor_relay_record_function_list = {
	NULL,
	mxd_motor_relay_create_record_structures
};

MX_RELAY_FUNCTION_LIST mxd_motor_relay_relay_function_list = {
	mxd_motor_relay_relay_command,
	mxd_motor_relay_get_relay_status
};

MX_RECORD_FIELD_DEFAULTS mxd_motor_relay_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RELAY_STANDARD_FIELDS,
	MXD_MOTOR_RELAY_STANDARD_FIELDS
};

long mxd_motor_relay_num_record_fields
	= sizeof( mxd_motor_relay_rf_defaults )
		/ sizeof( mxd_motor_relay_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_motor_relay_rfield_def_ptr
			= &mxd_motor_relay_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_motor_relay_get_pointers( MX_RELAY *relay,
			      MX_MOTOR_RELAY **motor_relay,
			      MX_RECORD **motor_record,
			      const char *fname )
{
	MX_MOTOR_RELAY *motor_relay_ptr;

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RELAY pointer passed is NULL." );
	}

	motor_relay_ptr = (MX_MOTOR_RELAY *)
				relay->record->record_type_struct;

	if ( motor_relay_ptr == (MX_MOTOR_RELAY *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR_RELAY pointer for record '%s' is NULL.",
			relay->record->name );
	}

	if ( motor_relay != (MX_MOTOR_RELAY **) NULL ) {
		*motor_relay = motor_relay_ptr;
	}

	if ( motor_record != (MX_RECORD **) NULL ) {
		*motor_record = motor_relay_ptr->motor_record;

		if ( *motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The motor_record pointer for MX motor relay '%s' "
			"is NULL.", motor_relay_ptr->motor_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_motor_relay_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_motor_relay_create_record_structures()";

        MX_RELAY *relay;
        MX_MOTOR_RELAY *motor_relay;

        /* Allocate memory for the necessary structures. */

        relay = (MX_RELAY *) malloc(sizeof(MX_RELAY));

        if ( relay == (MX_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_RELAY structure." );
        }

        motor_relay = (MX_MOTOR_RELAY *)
					malloc( sizeof(MX_MOTOR_RELAY) );

        if ( motor_relay == (MX_MOTOR_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for an MX_MOTOR_RELAY structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = relay;
        record->record_type_struct = motor_relay;
        record->class_specific_function_list =
				&mxd_motor_relay_relay_function_list;

        relay->record = record;
	motor_relay->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_motor_relay_relay_command( MX_RELAY *relay )
{
	static const char fname[] = "mxd_motor_relay_relay_command()";

	MX_MOTOR_RELAY *motor_relay = NULL;
	MX_RECORD *motor_record = NULL;
	mx_status_type mx_status;

	mx_status = mxd_motor_relay_get_pointers( relay,
				&motor_relay, &motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( relay->relay_command ) {
	case MXF_CLOSE_RELAY:
		mx_status = mx_motor_move_absolute( motor_record,
					motor_relay->closed_position, 0 );
		break;
	case MXF_OPEN_RELAY:
		mx_status = mx_motor_move_absolute( motor_record,
					motor_relay->open_position, 0 );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal relay command value %ld for MX motor relay '%s'.  "
		"The allowed values are 0 and 1",
			relay->relay_command, relay->record->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_motor_relay_get_relay_status( MX_RELAY *relay )
{
	static const char fname[] = "mxd_motor_relay_get_relay_status()";

	MX_MOTOR_RELAY *motor_relay = NULL;
	MX_RECORD *motor_record = NULL;
	double motor_position;
	double open_position_difference, closed_position_difference;
	mx_status_type mx_status;

	mx_status = mxd_motor_relay_get_pointers( relay,
				&motor_relay, &motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_position( motor_record, &motor_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

	open_position_difference =
		fabs( motor_position - motor_relay->open_position );

	if ( open_position_difference < motor_relay->position_deadband ) {

		relay->relay_status = MXF_RELAY_IS_OPEN;

		return MX_SUCCESSFUL_RESULT;
	}

	/*---*/

	closed_position_difference =
		fabs( motor_position - motor_relay->closed_position );

	if ( closed_position_difference < motor_relay->position_deadband ) {

		relay->relay_status = MXF_RELAY_IS_CLOSED;

		return MX_SUCCESSFUL_RESULT;
	}

	relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

	return MX_SUCCESSFUL_RESULT;
}

