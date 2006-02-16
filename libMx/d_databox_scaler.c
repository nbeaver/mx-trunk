/*
 * Name:    d_databox_scaler.c
 *
 * Purpose: MX scaler driver to control the Radix Instruments Databox scaler.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DATABOX_SCALER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_scaler.h"
#include "mx_motor.h"
#include "i_databox.h"
#include "d_databox_scaler.h"
#include "d_databox_motor.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_databox_scaler_record_function_list = {
	NULL,
	mxd_databox_scaler_create_record_structures,
	mxd_databox_scaler_finish_record_initialization,
	NULL,
	mxd_databox_scaler_print_structure
};

MX_SCALER_FUNCTION_LIST mxd_databox_scaler_scaler_function_list = {
	NULL,
	NULL,
	mxd_databox_scaler_read,
	NULL,
	mxd_databox_scaler_is_busy,
	mxd_databox_scaler_start,
	mxd_databox_scaler_stop,
	mxd_databox_scaler_get_parameter,
	mxd_databox_scaler_set_parameter
};

/* Databox scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_databox_scaler_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_DATABOX_SCALER_STANDARD_FIELDS
};

mx_length_type mxd_databox_scaler_num_record_fields
		= sizeof( mxd_databox_scaler_record_field_defaults )
		  / sizeof( mxd_databox_scaler_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_databox_scaler_rfield_def_ptr
			= &mxd_databox_scaler_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_databox_scaler_get_pointers( MX_SCALER *scaler,
			MX_DATABOX_SCALER **databox_scaler,
			MX_DATABOX **databox,
			const char *calling_fname )
{
	static const char fname[] = "mxd_databox_scaler_get_pointers()";

	MX_RECORD *databox_record;

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SCALER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( databox_scaler == (MX_DATABOX_SCALER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DATABOX_SCALER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( databox == (MX_DATABOX **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DATABOX pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*databox_scaler = (MX_DATABOX_SCALER *)
				scaler->record->record_type_struct;

	if ( *databox_scaler == (MX_DATABOX_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DATABOX_SCALER pointer for record '%s' is NULL.",
			scaler->record->name );
	}

	databox_record = (*databox_scaler)->databox_record;

	if ( databox_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The databox_record pointer for scaler '%s' is NULL.",
			scaler->record->name );
	}

	*databox = (MX_DATABOX *) databox_record->record_type_struct;

	if ( *databox == (MX_DATABOX *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DATABOX pointer for Databox interface '%s' is NULL.",
			databox_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_databox_scaler_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_databox_scaler_create_record_structures()";

	MX_SCALER *scaler;
	MX_DATABOX_SCALER *databox_scaler;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER structure." );
	}

	databox_scaler = (MX_DATABOX_SCALER *)
				malloc( sizeof(MX_DATABOX_SCALER) );

	if ( databox_scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_DATABOX_SCALER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = databox_scaler;
	record->class_specific_function_list
			= &mxd_databox_scaler_scaler_function_list;

	scaler->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_scaler_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_databox_scaler_finish_record_initialization()";

	MX_SCALER *scaler;
	mx_status_type mx_status;

	mx_status = mx_scaler_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scaler = (MX_SCALER *) record->record_class_struct;

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SCALER pointer for record '%s' is NULL.",
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_scaler_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_databox_scaler_print_structure()";

	MX_SCALER *scaler;
	MX_DATABOX_SCALER *databox_scaler;
	int32_t current_value;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	scaler = (MX_SCALER *) (record->record_class_struct);

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SCALER pointer for record '%s' is NULL.", record->name);
	}

	databox_scaler = (MX_DATABOX_SCALER *) (record->record_type_struct);

	if ( databox_scaler == (MX_DATABOX_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_DATABOX_SCALER pointer for record '%s' is NULL.", record->name);
	}

	fprintf(file, "SCALER parameters for scaler '%s':\n", record->name);

	fprintf(file, "  Scaler type           = DATABOX_SCALER.\n\n");
	fprintf(file, "  DATABOX record            = %s\n",
					databox_scaler->databox_record->name);

	mx_status = mx_scaler_read( record, &current_value );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read value of scaler '%s'",
			record->name );
	}

	fprintf(file, "  present value         = %ld\n", (long) current_value);

	fprintf(file, "  dark current          = %g counts per second.\n",
					scaler->dark_current);

	fprintf(file, "  scaler flags          = %#lx\n",
					(unsigned long) scaler->scaler_flags);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_scaler_read( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_databox_scaler_read()";

	MX_DATABOX_SCALER *databox_scaler;
	MX_DATABOX *databox;
	char response[100];
	int i, num_items;
	long scaler_value;
	mx_status_type mx_status;

	mx_status = mxd_databox_scaler_get_pointers( scaler,
					&databox_scaler, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for scaler '%s'",fname,scaler->record->name));

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
					DATABOX_SCALER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read and discard response lines from the Databox until we
	 * reach one that starts with the word 'DATA'.
	 */

	for ( i = 0; i < MX_DATABOX_SCALER_MAX_READOUT_LINES; i++ ) {
		mx_status = mxi_databox_getline( databox,
					response, sizeof response,
					DATABOX_SCALER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( strncmp( response, "DATA", 4 ) == 0 ) {

			/* We have found the start of data, so exit
			 * the for() loop.
			 */
			break;
		}
	}

	if ( i >= MX_DATABOX_SCALER_MAX_READOUT_LINES ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Did not find the start of data marker in the readout from "
		"Databox '%s' even after %d response lines were read.",
			databox->record->name,
			MX_DATABOX_SCALER_MAX_READOUT_LINES );
	}

	/* Read a line.  It should contain the actual scaler value. */

	mx_status = mxi_databox_getline( databox,
					response, sizeof response,
					DATABOX_SCALER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%ld", &scaler_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Scaler value not found in Databox response '%s'.",
			response );
	}

	/* There should be one more line to read which contains
	 * an end-of-data marker.
	 */

	mx_status = mxi_databox_getline( databox,
					response, sizeof response,
					DATABOX_SCALER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scaler->raw_value = scaler_value;

	MX_DEBUG( 2,("%s complete.  value = %ld",
		fname, (long) scaler->raw_value));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_scaler_is_busy( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_databox_scaler_is_busy()";

	MX_DATABOX_SCALER *databox_scaler;
	MX_DATABOX *databox;
	char c;
	mx_status_type mx_status;

	mx_status = mxd_databox_scaler_get_pointers( scaler,
					&databox_scaler, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for scaler '%s'",
				fname, scaler->record->name));

	MX_DEBUG( 2,("%s: databox->last_start_action = %d",
			fname, databox->last_start_action));
	MX_DEBUG( 2,("%s: databox->moving_motor = '%c'",
			fname, databox->moving_motor));

	/* Read and discard any characters that may be available from the
	 * RS-232 port.  If any of those characters are an asterisk '*'
	 * character, the scaler has stopped counting.  Otherwise, the
	 * scaler is still counting.
	 */

	scaler->busy = TRUE;

	for (;;) {
		mx_status = mx_rs232_getchar( databox->rs232_record,
						&c, MXF_232_NOWAIT );

		if ( mx_status.code == MXE_NOT_READY ) {
			/* No characters are available, so the scaler
			 * is still counting.
			 */

			break;	/* Exit the for() loop. */
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Have we found an asterisk? */

		if ( c == '*' ) {
			/* This means the scaler has stopped counting. */

			scaler->busy = FALSE;

			databox->last_start_action = MX_DATABOX_NO_ACTION;

			MX_DEBUG( 2,
			  ("%s: assigned %d to databox->last_start_action",
				fname, databox->last_start_action));

			break;	/* Exit the for() loop. */
		}
	}

	if ( scaler->busy == FALSE ) {
		mx_status = mxi_databox_discard_unread_input( databox,
							DATABOX_SCALER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	MX_DEBUG( 2,("%s complete.  busy = %d", fname, (int) scaler->busy));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_scaler_start( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_databox_scaler_start()";

	MX_DATABOX_SCALER *databox_scaler;
	MX_DATABOX *databox;
	MX_RECORD *x_motor_record;
	MX_MOTOR *x_motor;
	MX_RECORD *other_motor_record;
	char response[100];
	int i;
	long preset_counts;
	double step_size;
	mx_status_type mx_status;

	mx_status = mxd_databox_scaler_get_pointers( scaler,
					&databox_scaler, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: databox->last_start_action = %d",
			fname, databox->last_start_action));
	MX_DEBUG( 2,("%s: databox->moving_motor = '%c'\n",
			fname, databox->moving_motor));

	if ( databox->moving_motor != '\0' ) {
		mx_status = mxi_databox_get_record_from_motor_name(
				databox, databox->moving_motor,
				&other_motor_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		return mx_error( MXE_TRY_AGAIN, fname,
"Cannot start scaler '%s' at this time since motor '%s' of Databox '%s' "
"is current in motion.  Try again after the move completes.",
			scaler->record->name,
			other_motor_record->name,
			databox->record->name );
	}

	preset_counts = scaler->raw_value;

	MX_DEBUG( 2,("%s invoked for scaler '%s' for %ld counts.",
			fname, scaler->record->name, preset_counts));

	/* Get the current position of the X axis. */

	x_motor_record = databox->motor_record_array[0];

	if ( x_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"No X axis motor record has been defined for the Databox '%s'.  "
	"Thus, we cannot start the scaler '%s' since the Databox command "
	"must include the current position of the X axis.",
			databox->record->name,
			scaler->record->name );
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
					preset_counts );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the measurement. */

	mx_status = mxi_databox_command( databox, "S\r",
					NULL, 0, DATABOX_SCALER_DEBUG );

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
						DATABOX_SCALER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_databox_scaler_stop( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_databox_scaler_stop()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Stopping Databox scaler '%s' is not supported.",
		scaler->record->name );
}

MX_EXPORT mx_status_type
mxd_databox_scaler_get_parameter( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_databox_scaler_get_parameter()";

	MX_DATABOX_SCALER *databox_scaler;
	MX_DATABOX *databox;
	mx_status_type mx_status;

	mx_status = mxd_databox_scaler_get_pointers( scaler,
					&databox_scaler, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		mx_status = mxi_databox_get_limit_mode( databox );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( databox->limit_mode ) {
		case MX_DATABOX_CONSTANT_TIME_MODE:
			scaler->mode = MXCM_COUNTER_MODE;
			break;

		case MX_DATABOX_CONSTANT_COUNTS_MODE:
			scaler->mode = MXCM_PRESET_MODE;
			break;

		default:
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unexpected mode %d returned by mxi_databox_get_limit_mode().",
				databox->limit_mode );
		}
		break;
	case MXLV_SCL_DARK_CURRENT:
		break;
	default:
		return mx_scaler_default_get_parameter_handler( scaler );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_scaler_set_parameter( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_databox_scaler_set_parameter()";

	MX_DATABOX_SCALER *databox_scaler;
	MX_DATABOX *databox;
	int limit_mode;
	mx_status_type mx_status;

	mx_status = mxd_databox_scaler_get_pointers( scaler,
					&databox_scaler, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		switch( scaler->mode ) {
		case MXCM_COUNTER_MODE:
			limit_mode = MX_DATABOX_CONSTANT_TIME_MODE;
			break;
		case MXCM_PRESET_MODE:
			limit_mode = MX_DATABOX_CONSTANT_COUNTS_MODE;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
	"Scaler mode %d is not supported by the driver for scaler '%s'.",
				(int) scaler->mode, scaler->record->name );
		}

		mx_status = mxi_databox_set_limit_mode( databox, limit_mode );
		break;
	case MXLV_SCL_DARK_CURRENT:
		break;
	default:
		mx_status = mx_scaler_default_set_parameter_handler( scaler );
		break;
	}

	return mx_status;
}

