/*
 * Name:    d_databox_mcs.c 
 *
 * Purpose: MX multichannel scaler driver for Databox multichannel
 *          scaler support.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DATABOX_MCS_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_mcs.h"
#include "i_databox.h"
#include "d_databox_mcs.h"
#include "d_databox_motor.h"

/* Initialize the mcs driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_databox_mcs_record_function_list = {
	mxd_databox_mcs_initialize_type,
	mxd_databox_mcs_create_record_structures,
	mxd_databox_mcs_finish_record_initialization,
	mxd_databox_mcs_delete_record,
	NULL,
	mxd_databox_mcs_read_parms_from_hardware,
	mxd_databox_mcs_write_parms_to_hardware,
	mxd_databox_mcs_open,
	mxd_databox_mcs_close
};

MX_MCS_FUNCTION_LIST mxd_databox_mcs_mcs_function_list = {
	mxd_databox_mcs_start,
	mxd_databox_mcs_stop,
	mxd_databox_mcs_clear,
	mxd_databox_mcs_busy,
	mxd_databox_mcs_read_all,
	mxd_databox_mcs_read_scaler,
	mxd_databox_mcs_read_measurement,
	mxd_databox_mcs_read_timer,
	mxd_databox_mcs_get_parameter,
	mxd_databox_mcs_set_parameter
};

/* DATABOX mcs data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_databox_mcs_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCS_STANDARD_FIELDS,
	MXD_DATABOX_MCS_STANDARD_FIELDS
};

mx_length_type mxd_databox_mcs_num_record_fields
		= sizeof( mxd_databox_mcs_record_field_defaults )
			/ sizeof( mxd_databox_mcs_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_databox_mcs_rfield_def_ptr
			= &mxd_databox_mcs_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_databox_mcs_get_pointers( MX_MCS *mcs,
			MX_DATABOX_MCS **databox_mcs,
			MX_DATABOX **databox,
			const char *calling_fname )
{
	const char fname[] = "mxd_databox_mcs_get_pointers()";

	MX_RECORD *databox_record;

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( databox_mcs == (MX_DATABOX_MCS **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DATABOX_MCS pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( databox == (MX_DATABOX **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DATABOX pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( mcs->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_MCS pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*databox_mcs = (MX_DATABOX_MCS *) mcs->record->record_type_struct;

	if ( *databox_mcs == (MX_DATABOX_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DATABOX_MCS pointer for record '%s' is NULL.",
			mcs->record->name );
	}

	databox_record = (*databox_mcs)->databox_record;

	if ( databox_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The databox_record pointer for Databox MCS '%s' is NULL.",
			mcs->record->name );
	}

	*databox = (MX_DATABOX *) databox_record->record_type_struct;

	if ( *databox == (MX_DATABOX *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DATABOX pointer for Databox record '%s' is NULL.",
			databox_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_databox_mcs_initialize_type( long record_type )
{
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	MX_RECORD_FIELD_DEFAULTS *field;
	mx_length_type num_record_fields;
	mx_length_type maximum_num_scalers_varargs_cookie;
	mx_length_type maximum_num_measurements_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mcs_initialize_type( record_type,
				&num_record_fields,
				&record_field_defaults,
				&maximum_num_scalers_varargs_cookie,
				&maximum_num_measurements_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field_defaults(
			record_field_defaults, num_record_fields,
			"motor_position_data", &field );

	field->dimension[0] = maximum_num_measurements_varargs_cookie;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_databox_mcs_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_databox_mcs_create_record_structures()";

	MX_MCS *mcs;
	MX_DATABOX_MCS *databox_mcs;

	/* Allocate memory for the necessary structures. */

	mcs = (MX_MCS *) malloc( sizeof(MX_MCS) );

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCS structure." );
	}

	databox_mcs = (MX_DATABOX_MCS *) malloc( sizeof(MX_DATABOX_MCS) );

	if ( databox_mcs == (MX_DATABOX_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_DATABOX_MCS structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mcs;
	record->record_type_struct = databox_mcs;
	record->class_specific_function_list
			= &mxd_databox_mcs_mcs_function_list;

	mcs->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_mcs_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] = "mxd_databox_mcs_finish_record_initialization()";

	MX_MCS *mcs;
	MX_DATABOX_MCS *databox_mcs;
	MX_DATABOX *databox;
	mx_status_type mx_status;

	mx_status = mx_mcs_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mcs = (MX_MCS *) record->record_class_struct;

	mx_status = mxd_databox_mcs_get_pointers( mcs,
					&databox_mcs, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	databox->mcs_record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_mcs_delete_record( MX_RECORD *record )
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
mxd_databox_mcs_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_mcs_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_mcs_open( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_mcs_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_databox_mcs_start( MX_MCS *mcs )
{
	const char fname[] = "mxd_databox_mcs_start()";

	MX_DATABOX_MCS *databox_mcs;
	MX_DATABOX *databox;
	mx_status_type mx_status;

	mx_status = mxd_databox_mcs_get_pointers( mcs,
					&databox_mcs, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Here we merely record the fact that an MCS scan is to be started
	 * by the next Databox motor move command.
	 */

	databox->last_start_action = MX_DATABOX_MCS_START_ACTION;

	databox_mcs->measurement_index = 0L;

	MX_DEBUG( 2,("%s: assigned %d to databox->last_start_action",
			fname, databox->last_start_action));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_mcs_start_sequence( MX_RECORD *mcs_record,
				double destination,
				int perform_move )
{
	const char fname[] = "mxd_databox_mcs_start_sequence()";

	MX_MCS *mcs;
	MX_DATABOX_MCS *databox_mcs;
	MX_DATABOX *databox;
	MX_RECORD *x_motor_record;
	MX_MOTOR *x_motor;
	MX_RECORD *other_motor_record;
	char response[100];
	int i;
	double start, end, step_size, raw_num_steps;
	double seconds;
	long count_value;
	mx_status_type mx_status;

	if ( mcs_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The mcs_record pointer passed is NULL." );
	}

	mcs = (MX_MCS *) mcs_record->record_class_struct;

	mx_status = mxd_databox_mcs_get_pointers( mcs,
					&databox_mcs, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( databox->moving_motor != '\0' ) {
		mx_status = mxi_databox_get_record_from_motor_name(
				databox, databox->moving_motor,
				&other_motor_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		return mx_error( MXE_TRY_AGAIN, fname,
	"Cannot start MCS '%s' at this time since motor '%s' of Databox '%s' "
	"is currently in motion.  Try again after the move completes.",
			mcs->record->name,
			other_motor_record->name,
			databox->record->name );
	}

	/* Compute the count value. */

	switch( databox->limit_mode ) {
	case MX_DATABOX_CONSTANT_TIME_MODE:

		/* Get the measurement time per point. */

		seconds = mcs->measurement_time;

		count_value = mx_round( seconds );

		MX_DEBUG( 2,("%s invoked for MCS '%s' for %ld seconds.",
			fname, mcs->record->name, count_value));

		if ( fabs( seconds - (double) count_value ) >= 0.001 ) {
			mx_warning(
		"MCS '%s' measurement time of %g sec was rounded to %ld sec.",
				mcs->record->name, seconds, count_value );
		}
		break;

	case MX_DATABOX_CONSTANT_COUNTS_MODE:

		count_value = (long) mcs->measurement_counts;

		MX_DEBUG( 2,("%s invoked for MCS '%s' for %ld counts.",
			fname, mcs->record->name, count_value));
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal Databox limit mode %d.  This is a programming bug.",
			databox->limit_mode );
	}

	/* Get the current position of the X axis. */

	x_motor_record = databox->motor_record_array[0];

	if ( x_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"No X axis motor record has been defined for the Databox '%s'.  "
	"Thus, we cannot start the MCS '%s' since the Databox command "
	"must include the current position of the X axis.",
			databox->record->name,
			mcs->record->name );
	}

	x_motor = (MX_MOTOR *) x_motor_record->record_class_struct;

	mx_status = mxd_databox_motor_get_position( x_motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the sequence parameters. */


	start = x_motor->raw_position.analog;

	if ( perform_move ) {
		end = destination;
	} else {
		end = start;
	}

	raw_num_steps = mx_divide_safely( end - start,
					databox->degrees_per_x_step );

	if ( fabs(raw_num_steps) < 0.5 ) {
		step_size = databox->degrees_per_x_step;
	} else {
		step_size = mx_divide_safely( end - start,
			(double)(mcs->current_num_measurements - 1L) );
	}

	/* Setup the necessary Databox sequence. */

	mx_status = mxi_databox_define_sequence( databox,
					start, end, step_size, count_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the measurement. */

	mx_status = mxi_databox_command( databox, "S\r",
					NULL, 0, DATABOX_MCS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read and discard the first six lines of the response, since
	 * they merely describe the sequence that is about to be run.
	 */

	for ( i = 0; i < 6; i++ ) {

		mx_status = mxi_databox_getline( databox,
						response, sizeof response,
						DATABOX_MCS_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}


MX_EXPORT mx_status_type
mxd_databox_mcs_stop( MX_MCS *mcs )
{
	const char fname[] = "mxd_databox_mcs_stop()";

	MX_DATABOX_MCS *databox_mcs;
	MX_DATABOX *databox;
	char response[100];
	mx_status_type mx_status;

	mx_status = mxd_databox_mcs_get_pointers( mcs,
					&databox_mcs, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send Interrupt execution command. */

	mx_status = mxi_databox_command( databox, "I\r",
					response, sizeof response,
					DATABOX_MCS_DEBUG );

	databox->last_start_action = MX_DATABOX_NO_ACTION;

	MX_DEBUG( 2,("%s: assigned %d to databox->last_start_action",
			fname, databox->last_start_action));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_databox_mcs_clear( MX_MCS *mcs )
{
	const char fname[] = "mxd_databox_mcs_clear()";

	MX_DATABOX_MCS *databox_mcs;
	MX_DATABOX *databox;
	long i;
	mx_status_type mx_status;

	mx_status = mxd_databox_mcs_get_pointers( mcs,
					&databox_mcs, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 0; i < mcs->maximum_num_measurements; i++ ) {
		mcs->data_array[0][i] = 0L;

		databox_mcs->motor_position_data[i] = 0.0;
	}

	for ( i = 0; i <= MX_DATABOX_MCS_BUFFER_LENGTH; i++ ) {
		databox_mcs->buffer[i] = '\0';
	}

	databox_mcs->measurement_index = 0;
	databox_mcs->buffer_index = 0;

	databox_mcs->buffer_status = MXF_DATABOX_MCS_BUFFER_IS_FILLING;

	databox->last_start_action = MX_DATABOX_NO_ACTION;

	MX_DEBUG( 2,("%s: assigned %d to databox->last_start_action",
			fname, databox->last_start_action));

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_databox_mcs_handle_busy_response( MX_MCS *mcs,
					MX_DATABOX_MCS *databox_mcs,
					MX_DATABOX *databox )
{
	const char fname[] = "mxd_databox_handle_busy_response()";

	char response[100];
	char *ptr;
	size_t whitespace_length, string_length;
	long i, j, measurement_number;
	int num_items;
	double position;
	long scaler_value;
	double timer_value;
	mx_status_type mx_status;

	whitespace_length = strspn( databox_mcs->buffer, " \t" );

	ptr = databox_mcs->buffer + whitespace_length;

	MX_DEBUG( 2,("%s: ptr = '%s'", fname, ptr));

	string_length = strlen( ptr );

	if ( string_length == 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Empty response '%s' returned by the Databox '%s'.",
			databox_mcs->buffer, databox->record->name );
	}

	/* Try to parse the response as a scan progress display. */

	if ( databox->limit_mode == MX_DATABOX_CONSTANT_TIME_MODE ) {

		num_items = sscanf( ptr, "%ld %lg %ld",
				&measurement_number,
				&position,
				&scaler_value );
	} else {
		num_items = sscanf( ptr, "%ld %lg %lg",
				&measurement_number,
				&position,
				&timer_value );
	}

	if ( num_items == 3 ) {

		/* The response does appear to be a scan progress display. */

		/* Check to see if this is the expected measurement number. */

		i = databox_mcs->measurement_index;

		if ( measurement_number != ( i + 1L ) )
		{
			return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
"Response '%s' from Databox '%s' indicates that this is measurement %ld "
"but this is supposed to be measurement %ld.",
				databox_mcs->buffer,
				databox->record->name,
				measurement_number,
				i + 1L );
		}

		/* Save the reported values. */

		databox_mcs->motor_position_data[i] = position;

		if ( databox->limit_mode == MX_DATABOX_CONSTANT_TIME_MODE ) {

			mcs->data_array[0][i] = scaler_value;

			mcs->timer_data[i] = mcs->measurement_time;

			MX_DEBUG( 2,
	("%s: measurement_number = %ld, position = %g, scaler_value = %ld",
				fname, measurement_number,
					databox_mcs->motor_position_data[i],
					(long)(mcs->data_array[0][i]) ));
		} else {
			mcs->timer_data[i] = timer_value;

			mcs->data_array[0][i] = (long) mcs->measurement_counts;

			MX_DEBUG( 2,
	("%s: measurement_number = %ld, position = %g, timer_value = %g",
				fname, measurement_number,
					databox_mcs->motor_position_data[i],
					mcs->timer_data[i]));
		}

		/* Increment the measurement index to be ready for the
		 * next measurement.
		 */

		databox_mcs->measurement_index++;

		return MX_SUCCESSFUL_RESULT;
	}

	/* The response returned was not a scan progress display, so we must
	 * determine what it actually was.
	 */

	if ( strncmp( ptr, "Sequence # 2", 12 ) == 0 ) {

		/* We have reached the end of the scan, so discard the four
		 * lines of text that describe the next sequence and then
		 * read the next line as well.
		 */

		for ( j = 0; j <= 4; j++ ) {

			mx_status = mxi_databox_getline( databox,
					response, sizeof response,
					DATABOX_MCS_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		/* The last line read above should be the indication
		 * that execution has halted.  The first non-whitespace
		 * character should be ctrl-G.
		 */

		whitespace_length = strspn( response, " \t" );

		ptr = response + whitespace_length;

		if ( *ptr != 007 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Did not see the expected 'Execution halted' message.  "
		"Instead saw '%s'.", response );
		}

		/* The scan has completed successfully, so record the fact
		 * that the MCS is no longer busy.
		 */

		mcs->busy = FALSE;

		databox->last_start_action = MX_DATABOX_NO_ACTION;

		MX_DEBUG( 2,("%s: assigned %d to databox->last_start_action",
			fname, databox->last_start_action));

		return MX_SUCCESSFUL_RESULT;
	}

	/* Did we get an error message from the controller? */

	if ( *ptr == 007 ) {
		/* Move to the actual text of the message. */

		ptr++;

		whitespace_length = strspn( ptr, " \t" );

		ptr += whitespace_length;

		if ( strncmp( ptr, "**Illegal step size", 19 ) == 0 ) {

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The Databox attempted to use an illegal motor step size.  "
"Legal step sizes must be multiples of the size of a single motor step." );
		}

		/* Other cases are passed on through to the next section
		 * of code.
		 */
	}

	/* If we get here, we did not see a scan progress message and we did
	 * not see a normal end-of-scan sequence, so something else went wrong.
	 */

	return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
	"Got an unexpected error response from the Databox '%s'.  "
	"Response = '%s'.", databox->record->name, databox_mcs->buffer );
}

MX_EXPORT mx_status_type
mxd_databox_mcs_busy( MX_MCS *mcs )
{
	const char fname[] = "mxd_databox_mcs_busy()";

	MX_DATABOX_MCS *databox_mcs;
	MX_DATABOX *databox;
	int i;
	char c;
	uint32_t num_input_bytes_available;
	mx_status_type mx_status;

	mx_status = mxd_databox_mcs_get_pointers( mcs,
						&databox_mcs, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mcs->busy = TRUE;

	MX_DEBUG( 2,("%s invoked.", fname));

	MX_DEBUG( 2,("%s: databox->last_start_action = %d",
				fname, databox->last_start_action));
	MX_DEBUG( 2,("%s: databox->moving_motor = '%c'",
			fname, databox->moving_motor));

	/* If mcs->current_num_measurements == 1, then probably we are
	 * performing a scaler or timer start.  mxd_databox_mcs_start()
	 * does not actually start the sequence, so if the Databox
	 * sequence has not already been started, we must start it here.
	 */

	MX_DEBUG( 2,("%s: mcs->current_num_measurements = %lu",
			fname, (unsigned long) mcs->current_num_measurements));

	if ( ( mcs->current_num_measurements == 1L )
	  && ( databox->last_start_action != MX_DATABOX_COUNTER_START_ACTION ))
	{
		mx_status = mxd_databox_mcs_start_sequence(
					mcs->record, 0.0, FALSE );

		MX_DEBUG( 2,("%s: marker R0", fname));

		databox->last_start_action = MX_DATABOX_COUNTER_START_ACTION;

		return mx_status;
	}

	/* Read as many characters as we can from the scan in progress. */

	for (;;) {
		mx_status = mx_rs232_num_input_bytes_available(
						databox->rs232_record,
						&num_input_bytes_available );

		if ( mx_status.code != MXE_SUCCESS ) {
			MX_DEBUG( 2,("%s: marker R1", fname));

			return mx_status;
		}

		if ( num_input_bytes_available == 0 )
			break;		/* Exit the for() loop. */

		mx_status = mxi_databox_getchar(databox, &c, DATABOX_MCS_DEBUG);

		if ( mx_status.code != MXE_SUCCESS ) {
			MX_DEBUG( 2,("%s: marker R2", fname));

			return mx_status;
		}

		if ( databox_mcs->buffer_status
			== MXF_DATABOX_MCS_BUFFER_CR_SEEN )
		{
			if ( c == MX_LF ) {
				databox_mcs->buffer_status
					= MXF_DATABOX_MCS_BUFFER_COMPLETE;

				break;	/* Exit the for() loop. */
			} else {
				databox_mcs->buffer_status
					= MXF_DATABOX_MCS_BUFFER_IS_FILLING;

				
			}
		} else if ( c == MX_CR ) {
			databox_mcs->buffer_status
				= MXF_DATABOX_MCS_BUFFER_CR_SEEN;
		}

		databox_mcs->buffer[ databox_mcs->buffer_index ] = c;

		(databox_mcs->buffer_index)++;

		if ( databox_mcs->buffer_index > MX_DATABOX_MCS_BUFFER_LENGTH )
		{
			MX_DEBUG( 2,("%s: marker R3", fname));

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The response buffer for Databox MCS '%s' is full.",
				mcs->record->name );
		}
	}

	/* If the buffer is not full yet, just return. */

	if ( databox_mcs->buffer_status != MXF_DATABOX_MCS_BUFFER_COMPLETE ) {
		MX_DEBUG( 2,("%s: marker R4", fname));
		MX_DEBUG( 2,("%s: measurement_index = %ld",
			fname, databox_mcs->measurement_index));
		MX_DEBUG( 2,("%s: buffer_index = %d",
			fname, databox_mcs->buffer_index));
		MX_DEBUG( 2,("%s: buffer_status = %d",
			fname, databox_mcs->buffer_status));

		return MX_SUCCESSFUL_RESULT;
	}

	/* The buffer is full, so figure out what to do with the results. */

	mx_status = mxd_databox_mcs_handle_busy_response( mcs,
						databox_mcs, databox );

	if ( mx_status.code != MXE_SUCCESS ) {
		MX_DEBUG( 2,("%s: marker R5", fname));

		return mx_status;
	}

	/* Clear out the response buffer. */

	for ( i = 0; i <= MX_DATABOX_MCS_BUFFER_LENGTH; i++ ) {
		databox_mcs->buffer[i] = '\0';
	}

	databox_mcs->buffer_index = 0;
	databox_mcs->buffer_status = MXF_DATABOX_MCS_BUFFER_IS_FILLING;

	MX_DEBUG( 2,("%s complete. busy = %d", fname, mcs->busy));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_mcs_read_all( MX_MCS *mcs )
{
	/* All the real work is done in mxd_databox_mcs_busy(), so we do not
	 * need to do anything here.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_mcs_read_scaler( MX_MCS *mcs )
{
	/* All the real work is done in mxd_databox_mcs_busy(), so we do not
	 * need to do anything here.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_mcs_read_measurement( MX_MCS *mcs )
{
	const char fname[] = "mxd_databox_mcs_read_measurement()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"This function is not supported for Databox multichannel scalers." );
}

MX_EXPORT mx_status_type
mxd_databox_mcs_read_timer( MX_MCS *mcs )
{
	/* All the real work is done in mxd_databox_mcs_busy(), so we do not
	 * need to do anything here.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_mcs_get_parameter( MX_MCS *mcs )
{
	const char fname[] = "mxd_databox_mcs_get_parameter()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"This function is not yet implemented." );
}

MX_EXPORT mx_status_type
mxd_databox_mcs_set_parameter( MX_MCS *mcs )
{
	const char fname[] = "mxd_databox_mcs_set_parameter()";

	MX_DATABOX_MCS *databox_mcs;
	MX_DATABOX *databox;
	int limit_mode;
	mx_status_type mx_status;

	mx_status = mxd_databox_mcs_get_pointers( mcs,
						&databox_mcs, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCS '%s', parameter type '%s' (%d)",
		fname, mcs->record->name,
		mx_get_field_label_string( mcs->record, mcs->parameter_type ),
		mcs->parameter_type));

	switch( mcs->parameter_type ) {
	case MXLV_MCS_MEASUREMENT_TIME:

		/* Nothing to do here.  mxd_databox_mcs_start() does the
		 * real work.
		 */

		MX_DEBUG( 2,("%s: Databox MCS '%s' measurement_time = %g",
			fname, mcs->record->name, mcs->measurement_time));
		break;

	case MXLV_MCS_MEASUREMENT_COUNTS:

		/* Nothing to do here.  mxd_databox_mcs_start() does the
		 * real work.
		 */

		MX_DEBUG( 2,("%s: Databox MCS '%s' measurement_counts = %lu",
			fname, mcs->record->name,
			(unsigned long) mcs->measurement_counts));
		break;

	case MXLV_MCS_CURRENT_NUM_MEASUREMENTS:

		/* Nothing to do here.  mxd_databox_mcs_start() does the
		 * real work.
		 */

		MX_DEBUG( 2,
		("%s: Databox MCS '%s' current_num_measurements = %lu",
			fname, mcs->record->name,
			(unsigned long) mcs->current_num_measurements));
		break;

	case MXLV_MCS_MODE:

		switch( mcs->mode ) {
		case MXM_PRESET_TIME:
			limit_mode = MX_DATABOX_CONSTANT_TIME_MODE;
			break;
		case MXM_PRESET_COUNT:
			limit_mode = MX_DATABOX_CONSTANT_COUNTS_MODE;
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal MCS mode %d selected.  Only preset time and "
		"preset count modes are allowed for a Databox MCS.",
				mcs->mode );
		}

		return mxi_databox_set_limit_mode( databox, limit_mode );

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %d is not supported by this driver.",
			mcs->parameter_type );

	}
	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

