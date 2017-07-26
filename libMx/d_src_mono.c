/*
 * Name:    d_src_mono.c
 *
 * Purpose: MX motor driver for monochromators at the Synchrotron Radiation
 *          Center (SRC) of the University of Wisconsin-Madison.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2008, 2010, 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SRC_MONO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_motor.h"
#include "d_src_mono.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_src_mono_record_function_list = {
	NULL,
	mxd_src_mono_create_record_structures,
	mx_motor_finish_record_initialization,
	NULL,
	NULL,
	mxd_src_mono_open
};

MX_MOTOR_FUNCTION_LIST mxd_src_mono_motor_function_list = {
	NULL,
	mxd_src_mono_move_absolute,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_src_mono_get_extended_status
};

/* SRC monochromator data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_src_mono_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_SRC_MONO_STANDARD_FIELDS
};

long mxd_src_mono_num_record_fields
		= sizeof( mxd_src_mono_record_field_defaults )
			/ sizeof( mxd_src_mono_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_src_mono_rfield_def_ptr
			= &mxd_src_mono_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_src_mono_get_pointers( MX_MOTOR *motor,
			MX_SRC_MONO **src_mono,
			MX_RS232 **rs232,
			const char *calling_fname )
{
	static const char fname[] = "mxd_src_mono_get_pointers()";

	MX_SRC_MONO *src_mono_ptr;
	MX_RECORD *rs232_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	src_mono_ptr = (MX_SRC_MONO *) motor->record->record_type_struct;

	if ( src_mono_ptr == (MX_SRC_MONO *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SRC_MONO pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( src_mono != (MX_SRC_MONO **) NULL ) {
		*src_mono = src_mono_ptr;
	}

	if ( rs232 != (MX_RS232 **) NULL ) {
		rs232_record = src_mono_ptr->rs232_record;

		if ( rs232_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The rs232_record pointer for SRC monochromator '%s' "
			"is NULL.", motor->record->name );
		}

		*rs232 = rs232_record->record_class_struct;

		if ( (*rs232) == (MX_RS232 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RS232 pointer for RS-232 record '%s' used "
			"by SRC monochromator '%s' is NULL.",
				rs232_record->name, motor->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_src_mono_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_src_mono_create_record_structures()";

	MX_MOTOR *motor;
	MX_SRC_MONO *src_mono = NULL;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MOTOR structure." );
	}

	src_mono = (MX_SRC_MONO *) malloc( sizeof(MX_SRC_MONO) );

	if ( src_mono == (MX_SRC_MONO *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_SRC_MONO structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = src_mono;
	record->class_specific_function_list =
				&mxd_src_mono_motor_function_list;

	motor->record = record;
	src_mono->record = record;

	/* An SRC monochromator is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	/* The motor does not provide a way to change the acceleration. */

	motor->acceleration_type = MXF_MTR_ACCEL_NONE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_src_mono_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_src_mono_open()";

	MX_MOTOR *motor = NULL;
	MX_SRC_MONO *src_mono = NULL;
	MX_RS232 *rs232 = NULL;
	mx_status_type mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_src_mono_get_pointers(motor, &src_mono, &rs232, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify the RS-232 port parameters. */

#if 0
	mx_status = mx_rs232_verify_configuration( src_mono->rs232_record,
						9600, 8, 'N', 1, 'N',
						0x0d0a, 0x0d,
						rs232->timeout );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	/* Discard any existing bytes in the serial port. */

	(void) mx_rs232_discard_unwritten_output( src_mono->rs232_record,
							MXD_SRC_MONO_DEBUG );

	mx_status = mx_rs232_discard_unread_input( src_mono->rs232_record,
							MXD_SRC_MONO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that the connection is alive by asking for the
	 * current position.
	 */

	src_mono->state = MXS_SRC_MONO_IDLE;

	mx_status = mxd_src_mono_get_extended_status( motor );

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_src_mono_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_src_mono_move_absolute()";

	MX_SRC_MONO *src_mono = NULL;
	char command[200];
	char response[200];
	mx_status_type mx_status;

	mx_status = mxd_src_mono_get_pointers( motor, &src_mono, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the move command. */

	snprintf( command, sizeof(command),
		"SCANEV(%f)", motor->raw_destination.analog );

	mx_status = mx_rs232_putline( src_mono->rs232_record,
				command, NULL, MXD_SRC_MONO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS ) {
		src_mono->state = MXS_SRC_MONO_ERROR;
		return mx_status;
	}

	/* If all went well, we should get a wait response from
	 * the SRC computer.
	 */

	mx_status = mx_rs232_getline( src_mono->rs232_record,
					response, sizeof(response),
					NULL, MXD_SRC_MONO_DEBUG );

	if ( mx_status.code == MXE_TIMED_OUT ) {
		src_mono->state = MXS_SRC_MONO_TIMED_OUT;
	} else
	if ( mx_status.code != MXE_SUCCESS ) {
		src_mono->state = MXS_SRC_MONO_ERROR;
	} else {
		if ( strcmp( response, "Wait!" ) == 0 ) {
			src_mono->state = MXS_SRC_MONO_ACTIVE;
		} else {
			src_mono->state = MXS_SRC_MONO_ERROR;

			mx_status = mx_error(MXE_DEVICE_IO_ERROR, fname,
			"Received unexpected response '%s' from "
			"SRC monochromator '%s'.",
				response, motor->record->name );
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_src_mono_get_extended_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_src_mono_get_extended_status()";

	MX_SRC_MONO *src_mono = NULL;
	unsigned long num_input_bytes_available;
	double current_position;
	int num_items;
	char response[200];
	mx_status_type mx_status;

	mx_status = mxd_src_mono_get_pointers( motor, &src_mono, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SRC_MONO_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s'.", fname, motor->record->name));
	MX_DEBUG(-2,("%s: Starting src_mono->state = %d",
		fname, src_mono->state ));
#endif

	motor->status = MXSF_MTR_ERROR;

	switch( src_mono->state ) {
	case MXS_SRC_MONO_ERROR:
		/* Currently, all we can do is close our eyes, be optimistic
		 * and hope that any existing errors have gone away.  We do
		 * generate a warning about that though.
		 */

		src_mono->state = MXS_SRC_MONO_IDLE;

		mx_warning( "Resetting SRC mono error for motor '%s'.",
			motor->record->name );
		break;

	case MXS_SRC_MONO_TIMED_OUT:
		/* Generate a warning that we are ignoring a timeout. */

		src_mono->state = MXS_SRC_MONO_IDLE;

		mx_warning( "Ignoring timeout for motor '%s'.",
			motor->record->name );
		break;

	case MXS_SRC_MONO_ACTIVE:
		/* A move is currently supposed to be in progress.
		 * Has the move completed yet?  We check for this
		 * by seeing whether or not the SRC computer has
		 * sent us a message.
		 */

		mx_status = mx_rs232_num_input_bytes_available(
						src_mono->rs232_record,
						&num_input_bytes_available );

		if ( mx_status.code != MXE_SUCCESS ) {
			src_mono->state = MXS_SRC_MONO_ERROR;
			break;
		}

		if ( num_input_bytes_available == 0 ) {
			/* We have not been sent any new characters, so
			 * the move must not yet be complete.
			 */

			break;
		}

		/* Apparently the move has completed.  We now verify this
		 * by looking for a scan complete message.
		 */

		mx_status = mx_rs232_getline( src_mono->rs232_record,
						response, sizeof(response),
						NULL, MXD_SRC_MONO_DEBUG );

		if ( mx_status.code == MXE_TIMED_OUT ) {
			src_mono->state = MXS_SRC_MONO_TIMED_OUT;
		} else
		if ( mx_status.code != MXE_SUCCESS ) {
			src_mono->state = MXS_SRC_MONO_ERROR;
		} else {
			if ( strcmp( response, "OK-Scan Completed!" ) == 0 ) {
				src_mono->state = MXS_SRC_MONO_IDLE;
			} else {
				mx_status = mx_error(MXE_DEVICE_IO_ERROR, fname,
				"Received unexpected response '%s' from "
				"SRC monochromator '%s'.",
					response, motor->record->name );
			}
		}
		break;

	case MXS_SRC_MONO_IDLE:
		/* If the mono is idle, then there is nothing that we
		 * need to do at this point in the function.
		 */

		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"SRC mono '%s' is in illegal state %d.  "
		"This should never happen, but if it does, try restarting MX.",
			motor->record->name, src_mono->state );
	}

	if ( src_mono->state == MXS_SRC_MONO_ACTIVE ) {
		motor->status = MXSF_MTR_IS_BUSY;

		return MX_SUCCESSFUL_RESULT;
	}

	/* We should only get here if the monochromator is now idle.  
	 * It should now be safe to ask for the current position.
	 */

	mx_status = mx_rs232_putline( src_mono->rs232_record,
				"ENERGY", NULL, MXD_SRC_MONO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS ) {
		src_mono->state = MXS_SRC_MONO_ERROR;
		return mx_status;
	}

	/* Now read back the motor position. */

	mx_status = mx_rs232_getline( src_mono->rs232_record,
					response, sizeof(response),
					NULL, MXD_SRC_MONO_DEBUG );

	if ( mx_status.code == MXE_TIMED_OUT ) {
		src_mono->state = MXS_SRC_MONO_TIMED_OUT;
		return mx_status;
	} else
	if ( mx_status.code != MXE_SUCCESS ) {
		src_mono->state = MXS_SRC_MONO_ERROR;
		return mx_status;
	}

	num_items = sscanf( response, "%lg", &current_position );

	if ( num_items != 1 ) {
		src_mono->state = MXS_SRC_MONO_ERROR;
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not find the position for SRC mono '%s' in the "
		"response '%s' to the 'ENERGY' command.",
			motor->record->name, response );
	}

	motor->raw_position.analog = current_position;

	motor->status = 0;

	return MX_SUCCESSFUL_RESULT;
}

