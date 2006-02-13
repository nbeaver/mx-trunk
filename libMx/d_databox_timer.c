/*
 * Name:    d_databox_timer.c
 *
 * Purpose: MX timer driver to use a Radix Instruments Databox timer.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DATABOX_TIMER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_timer.h"
#include "mx_motor.h"
#include "i_databox.h"
#include "d_databox_timer.h"
#include "d_databox_motor.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_databox_timer_record_function_list = {
	NULL,
	mxd_databox_timer_create_record_structures,
	mx_timer_finish_record_initialization
};

MX_TIMER_FUNCTION_LIST mxd_databox_timer_timer_function_list = {
	mxd_databox_timer_is_busy,
	mxd_databox_timer_start,
	mxd_databox_timer_stop,
	NULL,
	mxd_databox_timer_read,
	mxd_databox_timer_get_mode,
	mxd_databox_timer_set_mode
};

/* DATABOX timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_databox_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_DATABOX_TIMER_STANDARD_FIELDS
};

mx_length_type mxd_databox_timer_num_record_fields
		= sizeof( mxd_databox_timer_record_field_defaults )
		  / sizeof( mxd_databox_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_databox_timer_rfield_def_ptr
			= &mxd_databox_timer_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_databox_timer_get_pointers( MX_TIMER *timer,
			MX_DATABOX_TIMER **databox_timer,
			MX_DATABOX **databox,
			const char *calling_fname )
{
	static const char fname[] = "mxd_databox_timer_get_pointers()";

	MX_RECORD *databox_record;

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_TIMER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( databox_timer == (MX_DATABOX_TIMER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DATABOX_TIMER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( databox == (MX_DATABOX **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DATABOX pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*databox_timer = (MX_DATABOX_TIMER *)
				timer->record->record_type_struct;

	if ( *databox_timer == (MX_DATABOX_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DATABOX_TIMER pointer for record '%s' is NULL.",
			timer->record->name );
	}

	databox_record = (*databox_timer)->databox_record;

	if ( databox_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The databox_record pointer for timer '%s' is NULL.",
			timer->record->name );
	}

	*databox = (MX_DATABOX *) databox_record->record_type_struct;

	if ( *databox == (MX_DATABOX *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DATABOX pointer for Databox interface '%s' is NULL.",
			databox_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_databox_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_databox_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_DATABOX_TIMER *databox_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	timer->mode = MXCM_UNKNOWN_MODE;

	databox_timer = (MX_DATABOX_TIMER *)
				malloc( sizeof(MX_DATABOX_TIMER) );

	if ( databox_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_DATABOX_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = databox_timer;
	record->class_specific_function_list
			= &mxd_databox_timer_timer_function_list;

	timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_databox_timer_is_busy()";

	MX_DATABOX_TIMER *databox_timer;
	MX_DATABOX *databox;
	char c;
	mx_status_type mx_status;

	mx_status = mxd_databox_timer_get_pointers( timer,
					&databox_timer, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for timer '%s'", fname, timer->record->name));

	MX_DEBUG( 2,("%s: databox->last_start_action = %d",
			fname, databox->last_start_action));
	MX_DEBUG( 2,("%s: databox->moving_motor = '%c'",
			fname, databox->moving_motor));

	/* Read and discard any characters that may be available from the
	 * RS-232 port.  If any of those characters are an asterisk '*'
	 * character, the timer has stopped counting.  Otherwise, the
	 * timer is still counting.
	 */

	timer->busy = TRUE;

	for (;;) {
		mx_status = mx_rs232_getchar( databox->rs232_record,
						&c, MXF_232_NOWAIT );

		if ( mx_status.code == MXE_NOT_READY ) {
			/* No characters are available, so the timer
			 * is still counting.
			 */

			break;	/* Exit the while() loop. */
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Have we found an asterisk? */

		if ( c == '*' ) {
			/* This means the timer has stopped counting. */

			timer->busy = FALSE;

			databox->last_start_action = MX_DATABOX_NO_ACTION;

			MX_DEBUG( 2,
			  ("%s: assigned %d to databox->last_start_action",
				fname, databox->last_start_action));

			break;	/* Exit the while() loop. */
		}
	}

	if ( timer->busy == FALSE ) {
		mx_status = mxi_databox_discard_unread_input( databox,
							DATABOX_TIMER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	MX_DEBUG( 2,("%s complete.  busy = %d", fname, timer->busy));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_databox_timer_start()";

	MX_DATABOX_TIMER *databox_timer;
	MX_DATABOX *databox;
	MX_RECORD *x_motor_record;
	MX_MOTOR *x_motor;
	MX_RECORD *other_motor_record;
	char response[100];
	double seconds, step_size;
	long long_seconds;
	int i;
	mx_status_type mx_status;

	mx_status = mxd_databox_timer_get_pointers( timer,
					&databox_timer, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: databox->last_start_action = %d",
			fname, databox->last_start_action));
	MX_DEBUG( 2,("%s: databox->moving_motor = '%c'",
			fname, databox->moving_motor));

	if ( databox->moving_motor != '\0' ) {
		mx_status = mxi_databox_get_record_from_motor_name(
				databox, databox->moving_motor,
				&other_motor_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		return mx_error( MXE_TRY_AGAIN, fname,
	"Cannot start timer '%s' at this time since motor '%s' of Databox '%s' "
	"is current in motion.  Try again after the move completes.",
			timer->record->name,
			other_motor_record->name,
			databox->record->name );
	}

	seconds = timer->value;

	long_seconds = mx_round( seconds );

	MX_DEBUG( 2,("%s invoked for timer '%s' for %ld seconds.",
			fname, timer->record->name, long_seconds));

	if ( fabs( seconds - (double) long_seconds ) >= 0.001 ) {
		mx_warning(
		"Timer '%s' measurement time of %g sec was rounded to %ld sec.",
			timer->record->name, seconds, long_seconds );
	}

	/* Get the current position of the X axis. */

	x_motor_record = databox->motor_record_array[0];

	if ( x_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"No X axis motor record has been defined for the Databox '%s'.  "
	"Thus, we cannot start the timer '%s' since the Databox command "
	"must include the current position of the X axis.",
			databox->record->name,
			timer->record->name );
	}

	x_motor = (MX_MOTOR *) x_motor_record->record_class_struct;

	mx_status = mxd_databox_motor_get_position( x_motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Setup the necessary Databox sequence.
	 *
	 * Note that the step size does not matter much for this case.
	 * All we need is that it have a non-zero value.
	 */

	step_size = databox->degrees_per_x_step;

	mx_status = mxi_databox_define_sequence( databox,
					x_motor->raw_position.analog,
					x_motor->raw_position.analog,
					step_size,
					long_seconds );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the measurement. */

	mx_status = mxi_databox_command( databox, "S\r",
					NULL, 0, DATABOX_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	databox->last_start_action = MX_DATABOX_COUNTER_START_ACTION;

	MX_DEBUG( 2,("%s: assigned %d to databox->last_start_action",
			fname, databox->last_start_action));

	/* Read and discard the first four lines of the response, since
	 * they merely describe the sequence that is about to be run.
	 */

	for ( i = 0; i < 4; i++ ) {

		mx_status = mxi_databox_getline( databox,
						response, sizeof response,
						DATABOX_TIMER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_databox_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_databox_timer_stop()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Stopping Databox timer '%s' is not supported.",
		timer->record->name );
}

MX_EXPORT mx_status_type
mxd_databox_timer_read( MX_TIMER *timer )
{
	static const char fname[] = "mxd_databox_timer_read()";

	MX_DATABOX_TIMER *databox_timer;
	MX_DATABOX *databox;
	char response[100];
	int i, num_items;
	double timer_value;
	mx_status_type mx_status;

	mx_status = mxd_databox_timer_get_pointers( timer,
					&databox_timer, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for timer '%s'",fname,timer->record->name));

	/* Switch to Monitor mode if we are not already in it. */

	if ( databox->command_mode != MX_DATABOX_MONITOR_MODE ) {

		mx_status = mxi_databox_set_command_mode( databox,
					MX_DATABOX_MONITOR_MODE, TRUE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Send a Quick dump command and ask for only one channel. */

	mx_status = mxi_databox_command( databox, "Q1\r",
					response, sizeof response,
					DATABOX_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read and discard response lines from the Databox until we
	 * reach one that starts with the word 'DATA'.
	 */

	for ( i = 0; i < MX_DATABOX_TIMER_MAX_READOUT_LINES; i++ ) {
		mx_status = mxi_databox_getline( databox,
					response, sizeof response,
					DATABOX_TIMER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( strncmp( response, "DATA", 4 ) == 0 ) {

			/* We have found the start of data, so exit
			 * the for() loop.
			 */
			break;
		}
	}

	if ( i >= MX_DATABOX_TIMER_MAX_READOUT_LINES ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Did not find the start of data marker in the readout from "
		"Databox '%s' even after %d response lines were read.",
			databox->record->name,
			MX_DATABOX_TIMER_MAX_READOUT_LINES );
	}

	/* Read a line.  It should contain the actual timer value. */

	mx_status = mxi_databox_getline( databox,
					response, sizeof response,
					DATABOX_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lg", &timer_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Timer value not found in Databox response '%s'.",
			response );
	}

	/* There should be one more line to read which contains
	 * an end-of-data marker.
	 */

	mx_status = mxi_databox_getline( databox,
					response, sizeof response,
					DATABOX_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->value = timer_value;

	MX_DEBUG( 2,("%s complete.  value = %g", fname, timer->value));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_timer_get_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_databox_timer_get_mode()";

	MX_DATABOX_TIMER *databox_timer;
	MX_DATABOX *databox;
	mx_status_type mx_status;

	mx_status = mxd_databox_timer_get_pointers( timer,
					&databox_timer, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_databox_get_limit_mode( databox );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( databox->limit_mode ) {
	case MX_DATABOX_CONSTANT_TIME_MODE:
		timer->mode = MXCM_PRESET_MODE;
		break;
	case MX_DATABOX_CONSTANT_COUNTS_MODE:
		timer->mode = MXCM_COUNTER_MODE;
		break;
	default:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Unexpected mode %d returned by mxi_databox_get_limit_mode().",
			databox->limit_mode );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_timer_set_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_databox_timer_set_mode()";

	MX_DATABOX_TIMER *databox_timer;
	MX_DATABOX *databox;
	int limit_mode;
	mx_status_type mx_status;

	mx_status = mxd_databox_timer_get_pointers( timer,
					&databox_timer, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( timer->mode ) {
	case MXCM_COUNTER_MODE:
		limit_mode = MX_DATABOX_CONSTANT_COUNTS_MODE;
		break;
	case MXCM_PRESET_MODE:
		limit_mode = MX_DATABOX_CONSTANT_TIME_MODE;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Timer mode %d is not supported by the driver for timer '%s'.",
			timer->mode, timer->record->name );
	}

	mx_status = mxi_databox_set_limit_mode( databox, limit_mode );

	return mx_status;
}

