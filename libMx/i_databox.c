/*
 * Name:    i_databox.c
 *
 * Purpose: MX driver for the Radix Instruments Databox.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_DATABOX_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_generic.h"
#include "i_databox.h"
#include "d_databox_motor.h"

MX_RECORD_FUNCTION_LIST mxi_databox_record_function_list = {
	NULL,
	mxi_databox_create_record_structures,
	mxi_databox_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_databox_open,
	NULL,
	NULL,
	mxi_databox_resynchronize
};

MX_RECORD_FIELD_DEFAULTS mxi_databox_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_DATABOX_STANDARD_FIELDS
};

mx_length_type mxi_databox_num_record_fields
		= sizeof( mxi_databox_record_field_defaults )
			/ sizeof( mxi_databox_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_databox_rfield_def_ptr
			= &mxi_databox_record_field_defaults[0];

static mx_status_type mxi_databox_resynchronize_basic( MX_RECORD * );

/*==========================*/

MX_EXPORT mx_status_type
mxi_databox_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_databox_create_record_structures()";

	MX_GENERIC *generic;
	MX_DATABOX *databox;

	/* Allocate memory for the necessary structures. */

	generic = (MX_GENERIC *) malloc( sizeof(MX_GENERIC) );

	if ( generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GENERIC structure." );
	}

	databox = (MX_DATABOX *) malloc( sizeof(MX_DATABOX) );

	if ( databox == (MX_DATABOX *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_DATABOX structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = generic;
	record->record_type_struct = databox;
	record->class_specific_function_list = NULL;

	generic->record = record;
	databox->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_databox_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_databox_finish_record_initialization()";

	MX_DATABOX *databox;
	int i;

	databox = (MX_DATABOX *) record->record_type_struct;

	databox->command_mode = 0;
	databox->limit_mode = 0;
	databox->moving_motor = '\0';
	databox->last_start_action = MX_DATABOX_NO_ACTION;
	databox->degrees_per_x_step = 0.0;

	MX_DEBUG( 2,("%s: assigned %d to databox->last_start_action",
			fname, databox->last_start_action));

	/* Initialize the motor array. */

	for ( i = 0; i < MX_MAX_DATABOX_AXES; i++ ) {
		databox->motor_record_array[i] = NULL;
	}

	databox->mcs_record = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_databox_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_databox_open()";

	MX_DATABOX *databox;
	MX_RECORD *motor_record;
	MX_MOTOR *motor;
	MX_DATABOX_MOTOR *databox_motor;
	int i;
	char *ptr;
	char command[80];
	char response[200];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	databox = (MX_DATABOX *) (record->record_type_struct);

	if ( databox == (MX_DATABOX *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATABOX pointer for record '%s' is NULL.",
		record->name);
	}

	/* Try to resynchronize with the controller. */

	mx_status = mxi_databox_resynchronize_basic( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( databox->command_mode == MX_DATABOX_CALIBRATE_MODE ) {

		/* If we are in Calibrate Mode when the program starts up,
		 * this means that the Databox may just have been powered on.
		 * We test for this by trying to go to Monitor mode and
		 * seeing if it fails with a 'Not calibrated message'.
		 */

		mx_status = mxi_databox_command( databox, "E\r",
					response, sizeof response,
					MXI_DATABOX_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Get the next response line.  This will tell us whether
		 * or not the controller is calibrated.
		 */

		mx_status = mxi_databox_getline( databox,
					response, sizeof response,
					MXI_DATABOX_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ptr = strstr( response, "Not calibrated" );

		if ( ptr != NULL ) {

		    /* Turn off the automatic display of the count rate. */

		    mx_status = mxi_databox_command( databox, "C0\r",
					response, sizeof response,
					MXI_DATABOX_DEBUG );

		    if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		    /* We are not calibrated, so setup the calibration. */

		    for ( i = 0; i < MX_MAX_DATABOX_AXES; i++ ) {

			motor_record = databox->motor_record_array[i];

			if ( motor_record != NULL ) {
				motor = (MX_MOTOR *)
					motor_record->record_class_struct;

				databox_motor = (MX_DATABOX_MOTOR *)
					motor_record->record_type_struct;

				/* Set the steps per degree. */

				sprintf( command, "S%c%g\r",
					databox_motor->axis_name,
					databox_motor->steps_per_degree );

				mx_status = mxi_databox_command( databox,
					command, NULL, 0, MXI_DATABOX_DEBUG );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				/* Set the angle. */

				sprintf( command, "A%c%g\r",
					databox_motor->axis_name,
					motor->raw_position.analog );

				mx_status = mxi_databox_command( databox,
					command, NULL, 0, MXI_DATABOX_DEBUG );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
		    }
		}
	}

	/* Put the Databox back into Monitor mode. */

	mx_status = mxi_databox_set_command_mode( databox,
					MX_DATABOX_MONITOR_MODE, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Put the Databox into constant time mode. */

	mx_status = mxi_databox_set_limit_mode( databox,
					MX_DATABOX_CONSTANT_TIME_MODE );

	return mx_status;
}

static mx_status_type
mxi_databox_resynchronize_basic( MX_RECORD *record )
{
	static const char fname[] = "mxi_databox_resynchronize_basic()";

	MX_DATABOX *databox;
	char response[100];
	char *ptr;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	MX_DEBUG( 2,("%s invoked for '%s'", fname, record->name));

	databox = (MX_DATABOX *) (record->record_type_struct);

	if ( databox == (MX_DATABOX *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATABOX pointer for record '%s' is NULL.",
		record->name);
	}

	MX_DEBUG( 2,("%s: databox->last_start_action = %d",
			fname, databox->last_start_action));
	MX_DEBUG( 2,("%s: databox->moving_motor = '%c'",
			fname, databox->moving_motor));

	/* Discard any characters that may still be in the input buffer. */

	mx_status = mxi_databox_discard_unread_input( databox,
							MXI_DATABOX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard any characters that may not yet have been transmitted. */

	mx_status = mx_rs232_discard_unwritten_output( databox->rs232_record,
							MXI_DATABOX_DEBUG );

	if ( (mx_status.code != MXE_SUCCESS)
	  && (mx_status.code != MXE_UNSUPPORTED) )
	{
		return mx_status;
	}

	/* Send two carriage returns just in case the controller is waiting
	 * for a completed command from the user.
	 */

	mx_msleep(10);

	mx_status = mx_rs232_putchar( databox->rs232_record, '\r',
							MXF_232_WAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_msleep(10);

	mx_status = mx_rs232_putchar( databox->rs232_record, '\r',
							MXF_232_WAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the first line of response. */

	mx_msleep(10);

	mx_status = mxi_databox_getline( databox, response, sizeof response,
							MXI_DATABOX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If we got the message that says that the Databox has just been
	 * powered up, then tell it to retain the memory contents.
	 */

	ptr = strstr( response, "Valid memory contents found" );

	if ( ptr != NULL ) {
		mx_status = mxi_databox_putline( databox, "R\r",
						MXI_DATABOX_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Discard any further responses. */

	mx_status = mxi_databox_discard_unread_input( databox,
							MXI_DATABOX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find out what mode the Databox is in. */

	mx_status = mxi_databox_get_command_mode( databox );

	MX_DEBUG( 2,("%s complete.  Returning with status = %ld",
		fname, mx_status.code ));

	databox->last_start_action = MX_DATABOX_NO_ACTION;

	MX_DEBUG( 2,("%s: assigned %d to databox->last_start_action",
			fname, databox->last_start_action));

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_databox_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_databox_resynchronize()";

	MX_DATABOX *databox;
	mx_status_type mx_status;

	mx_status = mxi_databox_resynchronize_basic( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	databox = (MX_DATABOX *) (record->record_type_struct);

	if ( databox == (MX_DATABOX *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATABOX pointer for record '%s' is NULL.",
		record->name);
	}

	/* Try to put the Databox back into Monitor mode. */

	mx_status = mxi_databox_set_command_mode( databox,
					MX_DATABOX_MONITOR_MODE, TRUE );

	return mx_status;
}

/* === Functions specific to this driver. === */

MX_EXPORT mx_status_type
mxi_databox_command( MX_DATABOX *databox,
		char *command, char *response, int response_buffer_length,
		int debug_flag )
{
	mx_status_type mx_status;

	/* Discard any old input. */

	mx_status = mxi_databox_discard_unread_input( databox,
							MXI_DATABOX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the command string. */

	mx_status = mxi_databox_putline( databox, command, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the response, if needed. */

	if ( response != NULL ) {

		mx_status = mxi_databox_getline( databox,
			response, response_buffer_length, debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_databox_putline( MX_DATABOX *databox, char *command, int debug_flag )
{
	static const char fname[] = "mxi_databox_putline()";

	unsigned long i, length;
	mx_status_type mx_status;

	if ( databox == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_DATABOX pointer passed was NULL." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'command' buffer pointer passed was NULL.  No command sent.");
	}
	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
			fname, command, databox->record->name ));
	}

	/* The Databox manual says that we must send characters slowly
	 * enough that we do not overrun the Databox's input buffer. 
	 * The manual recommends that we wait until the Databox has
	 * echoed the most recent character before sending the next one.
	 */

	length = strlen( command );

	for ( i = 0; i < length; i++ ) {

		/* Send the next character. */

		mx_status = mxi_databox_putchar( databox,
						command[i], debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_databox_getline( MX_DATABOX *databox,
		char *response, int response_buffer_length,
		int debug_flag )
{
	static const char fname[] = "mxi_databox_getline()";

	size_t num_chars_returned;
	mx_status_type mx_status;

	if ( databox == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_DATABOX pointer passed was NULL." );
	}
	if ( response == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	"'response' buffer pointer passed was NULL.  No response received.");
	}

	response[0] = '\0';

	/* Get the line. */

	mx_status = mx_rs232_getline( databox->rs232_record,
				response, response_buffer_length,
				&num_chars_returned, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure the buffer is null terminated. */

	response[ num_chars_returned ] = '\0';

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: received '%s'", fname, response));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_databox_putchar( MX_DATABOX *databox, char c, int debug_flag )
{
	static const char fname[] = "mxi_databox_putchar()";

	long i, max_retries;
	char cr;
	mx_status_type mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2,
		("%s: sending character '%c' to record '%s'.",
			fname, c, databox->record->name ));
	}

	mx_status = mx_rs232_putchar( databox->rs232_record, c, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	max_retries = 10;

	/* Wait for the controller to echo something back.  The characters
	 * echoed back do not necessarily exactly match what was sent since
	 * the Databox tries to generate a "user friendly" response to the
	 * characters sent.
	 */

	for ( i = 0; i <= max_retries; i++ ) {

		mx_status = mx_rs232_getchar( databox->rs232_record,
					&cr, MXF_232_NOWAIT );

		if ( mx_status.code == MXE_NOT_READY ) {
			mx_msleep(1);   /* Wait a moment. */

			continue;   /* Go back and try again. */

		} else if ( mx_status.code == MXE_SUCCESS ) {
			break;      /* Exit the for() loop. */

		} else {
			return mx_status;
		}
	}
	if ( i > max_retries ) {
		return mx_error( MXE_TIMED_OUT, fname,
		"Attempt to read reply to character '%c' from "
		"record '%s' timed out after %ld retries.",
			c, databox->record->name, max_retries );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_databox_getchar( MX_DATABOX *databox, char *c, int debug_flag )
{
	static const char fname[] = "mxi_databox_getline()";

	mx_status_type mx_status;

	if ( databox == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_DATABOX pointer passed was NULL." );
	}
	if ( c == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	"'c' character pointer passed was NULL.  No response received.");
	}

	mx_status = mx_rs232_getchar( databox->rs232_record, c, MXF_232_WAIT );

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: received character '%c' %#x", fname, *c, *c));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_databox_get_command_mode( MX_DATABOX *databox )
{
	static const char fname[] = "mxi_databox_get_command_mode()";

	char response[120];
	char *ptr;
	mx_status_type mx_status;

	/* Discard any characters that may still be in the RS-232 buffers. */

	mx_status = mx_rs232_discard_unwritten_output(
			databox->rs232_record, MXI_DATABOX_DEBUG );

	if ( (mx_status.code != MXE_SUCCESS)
	  && (mx_status.code != MXE_UNSUPPORTED) )
	{
		return mx_status;
	}

	mx_status = mxi_databox_discard_unread_input( databox,
							MXI_DATABOX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send a '?' character to the databox.  This should return a list
	 * of currently available commands and include the current mode
	 * in the first line.
	 */

	mx_status = mxi_databox_putchar( databox, '?', MXI_DATABOX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The first line of the response should be blank, since the Databox
	 * sends a CR-LF sequence after the ? is echoed.
	 */

	mx_status = mxi_databox_getline( databox,
				response, sizeof response, MXI_DATABOX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the next line of the response.  This line should describe
	 * what mode we are in.
	 */

	mx_status = mxi_databox_getline( databox,
				response, sizeof response, MXI_DATABOX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard the rest of the response. */

	mx_status = mxi_databox_discard_unread_input( databox,
							MXI_DATABOX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find the first non-blank character. */

	ptr = response + strspn( response, " " );

	/* Do the comparisons to determine which mode we are in. */

	if ( strncmp( ptr, "MONITOR", 7 ) == 0 ) {
		databox->command_mode = MX_DATABOX_MONITOR_MODE;
	} else
	if ( strncmp( ptr, "CALIBRATION", 11 ) == 0 ) {
		databox->command_mode = MX_DATABOX_CALIBRATE_MODE;
	} else
	if ( strncmp( ptr, "EDIT", 4 ) == 0 ) {
		databox->command_mode = MX_DATABOX_EDIT_MODE;
	} else
	if ( strncmp( ptr, "RUN", 3 ) == 0 ) {
		databox->command_mode = MX_DATABOX_RUN_MODE;
	} else
	if ( strncmp( ptr, "OPTION", 6 ) == 0 ) {
		databox->command_mode = MX_DATABOX_OPTION_MODE;
	} else {
		databox->command_mode = MX_DATABOX_UNKNOWN_MODE;
	}

	MX_DEBUG( 2,("%s: databox->command_mode = %d",
					fname, databox->command_mode));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_databox_set_command_mode( MX_DATABOX *databox,
				int new_mode,
				int discard_response )
{
	static const char fname[] = "mxi_databox_set_command_mode()";

	int old_mode;
	mx_status_type mx_status;

	old_mode = databox->command_mode;

	MX_DEBUG( 2,("%s: old mode = %d, new mode = %d",
			fname, old_mode, new_mode ));

	/* If we are already in the correct mode, there is nothing else to do.*/

	if ( new_mode == old_mode )
		return MX_SUCCESSFUL_RESULT;

	/* First, exit back to monitor mode. */

	switch( old_mode ) {
	case MX_DATABOX_MONITOR_MODE:
		break;

	case MX_DATABOX_CALIBRATE_MODE:
	case MX_DATABOX_EDIT_MODE:
	case MX_DATABOX_OPTION_MODE:
		mx_status = mxi_databox_command( databox, "E\r",
						NULL, 0, MXI_DATABOX_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MX_DATABOX_RUN_MODE:
		/* Invoke the 'interrupt execution' command. */

		mx_status = mxi_databox_command( databox, "I\r",
						NULL, 0, MXI_DATABOX_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MX_DATABOX_UNKNOWN_MODE:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"mxi_databox_get_command_mode() could not determine which mode the "
	"Databox '%s' is in, so we do not know what to do.",
			databox->record->name );

	default:
		return mx_error( MXE_FUNCTION_FAILED, fname,
	"mxi_databox_get_command_mode() returned an unexpected mode %d.  "
	"This should not be able to happen and is probably a "
		"sign of a bug in the program.", old_mode );
	}

	/* Now switch to the mode that we want to be in. */

	switch( new_mode ) {
	case MX_DATABOX_MONITOR_MODE:
		break;

	case MX_DATABOX_CALIBRATE_MODE:
		mx_status = mxi_databox_command( databox, "C\r",
						NULL, 0, MXI_DATABOX_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MX_DATABOX_EDIT_MODE:
		mx_status = mxi_databox_command( databox, "E\r",
						NULL, 0, MXI_DATABOX_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MX_DATABOX_OPTION_MODE:
		mx_status = mxi_databox_command( databox, "O\r",
						NULL, 0, MXI_DATABOX_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MX_DATABOX_RUN_MODE:
		/* Execute the 'start execution' command. */

		mx_status = mxi_databox_command( databox, "S\r",
						NULL, 0, MXI_DATABOX_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Requested Databox mode %d is illegal.", new_mode );
	}

	databox->command_mode = new_mode;

	if ( discard_response ) {
		mx_status = mxi_databox_discard_unread_input( databox,
							MXI_DATABOX_DEBUG );
		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_databox_get_limit_mode( MX_DATABOX *databox )
{
	static const char fname[] = "mxi_databox_get_limit_mode()";

	MX_RECORD *motor_record;
	char response[200];
	char mode_string[200];
	int num_items;
	mx_status_type mx_status;

	/* If a move is currently in progress for one of the motors attached
	 * to the Databox, then it is not possible to ask for the limit mode.
	 */

	if ( databox->moving_motor != '\0' ) {
		mx_status = mxi_databox_get_record_from_motor_name(
				databox, databox->moving_motor,
				&motor_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		return mx_error( MXE_TRY_AGAIN, fname,
	"Cannot get the limit mode for Databox '%s' at this time since "
	"motor '%s' is currently in motion.  "
	"Try again after the move completes.",
			databox->record->name,
			motor_record->name );
	}

	if ( databox->command_mode != MX_DATABOX_CALIBRATE_MODE ) {
		mx_status = mxi_databox_set_command_mode( databox,
					MX_DATABOX_CALIBRATE_MODE, TRUE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Send the limit mode command. */

	mx_status = mxi_databox_command( databox, "L\r",
					response, sizeof response,
					MXI_DATABOX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%s", mode_string );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"The response '%s' to the get limit mode command sent to "
		"Databox '%s' is unparseable.",
			response, databox->record->name );
	}

	if ( strcmp( mode_string, "Time" ) == 0 ) {
		databox->limit_mode = MX_DATABOX_CONSTANT_TIME_MODE;
	} else
	if ( strcmp( mode_string, "Counts" ) == 0 ) {
		databox->limit_mode = MX_DATABOX_CONSTANT_COUNTS_MODE;
	} else {
		databox->limit_mode = MX_DATABOX_UNKNOWN_MODE;

		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"The limit mode string '%s' from Databox '%s' does not have "
		"one of the expected values of 'Time' or 'Counts'.",
			mode_string, databox->record->name );
	}

	/* Return to monitor mode. */

	mx_status = mxi_databox_set_command_mode( databox,
					MX_DATABOX_MONITOR_MODE, TRUE );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_databox_set_limit_mode( MX_DATABOX *databox, int limit_mode )
{
	static const char fname[] = "mxi_databox_set_limit_mode()";

	MX_RECORD *motor_record;
	char command[5];
	char response[100];
	mx_status_type mx_status;

	/* If a move is currently in progress for one of the motors attached
	 * to the Databox, then it is not possible to set the limit mode.
	 */

	if ( databox->moving_motor != '\0' ) {
		mx_status = mxi_databox_get_record_from_motor_name(
				databox, databox->moving_motor,
				&motor_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		return mx_error( MXE_TRY_AGAIN, fname,
	"Cannot set the limit mode for Databox '%s' at this time since "
	"motor '%s' is currently in motion.  "
	"Try again after the move completes.",
			databox->record->name,
			motor_record->name );
	}

	if ( databox->command_mode != MX_DATABOX_CALIBRATE_MODE ) {
		mx_status = mxi_databox_set_command_mode( databox,
					MX_DATABOX_CALIBRATE_MODE, TRUE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	switch( limit_mode ) {
	case MX_DATABOX_CONSTANT_TIME_MODE:
		strcpy( command, "LT\r" );
		break;
	case MX_DATABOX_CONSTANT_COUNTS_MODE:
		strcpy( command, "LC\r" );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Limit mode %d for Databox '%s' is unsupported.",
			limit_mode, databox->record->name );
	}

	mx_status = mxi_databox_command( databox, command,
					response, sizeof response,
					MXI_DATABOX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	databox->limit_mode = limit_mode;

	/* Return to monitor mode. */

	mx_status = mxi_databox_set_command_mode( databox,
					MX_DATABOX_MONITOR_MODE, TRUE );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_databox_get_record_from_motor_name( MX_DATABOX *databox,
					char motor_name,
					MX_RECORD **motor_record )
{
	static const char fname[] = "mxi_databox_get_record_from_motor_name()";

	if ( databox == (MX_DATABOX *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DATABOX pointer passed was NULL." );
	}
	if ( motor_record == (MX_RECORD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The motor_record pointer passed was NULL." );
	}

	switch ( motor_name ) {
	case 'X':
		*motor_record = databox->motor_record_array[0];
		break;
	case 'Y':
		*motor_record = databox->motor_record_array[1];
		break;
	case 'Z':
		*motor_record = databox->motor_record_array[2];
		break;
	case '\0':
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The motor name passed for Databox '%s' was a null character.",
			databox->record->name );
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The motor name '%c' (%#x) passed for Databox '%s' is an illegal motor name.",
			motor_name, motor_name, databox->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_databox_define_sequence( MX_DATABOX *databox,
				double initial_angle,
				double final_angle,
				double step_size,
				long count_value )
{
	static const char fname[] = "mxi_databox_define_sequence()";

	char command[80];
	char response[100];
	int i, num_items, current_sequence;
	long num_steps;
	long old_count_value;
	double raw_num_steps, rounded_step_size;
	mx_status_type mx_status;

	/* Switch to Edit mode if we are not already in it. */

	if ( databox->command_mode != MX_DATABOX_EDIT_MODE ) {

		mx_status = mxi_databox_set_command_mode( databox,
						MX_DATABOX_EDIT_MODE, TRUE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Undefine the current sequence. */

	mx_status = mxi_databox_command( databox, "U\r",
					response, sizeof response,
					MXI_DATABOX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The next response line contains the current sequence number. */

	mx_status = mxi_databox_getline( databox,
					response, sizeof response,
					MXI_DATABOX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%*s %*s %d", &current_sequence );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Unable to find a sequence number in the Databox response '%s'.",
			response );
	}

	/* Undefine any old sequences. */

	for ( i = current_sequence; i > 1; i-- ) {

		/* Go to the previous sequence. */

		mx_status = mxi_databox_command( databox, "P\r",
					response, sizeof response,
					MXI_DATABOX_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Undefine the sequence. */

		mx_status = mxi_databox_command( databox, "U\r",
					response, sizeof response,
					MXI_DATABOX_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Set the initial angle. */

	sprintf( command, "I%g\r", initial_angle );

	mx_status = mxi_databox_command( databox, command,
					response, sizeof response,
					MXI_DATABOX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the final angle. */

	sprintf( command, "F%g\r", final_angle );

	mx_status = mxi_databox_command( databox, command,
					response, sizeof response,
					MXI_DATABOX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Round the step size to the nearest integer number of steps. */

	raw_num_steps = mx_divide_safely( step_size,
					databox->degrees_per_x_step );

	num_steps = mx_round( raw_num_steps );

	rounded_step_size = databox->degrees_per_x_step * (double) num_steps;

	if ( fabs( raw_num_steps - (double) num_steps ) >= 0.001 ) {
		mx_warning(
		"Databox '%s' step size of %g deg was rounded to %g deg.",
			databox->record->name, step_size, rounded_step_size );
	}

	MX_DEBUG( 2,("%s: original step_size = %g", fname, step_size));
	MX_DEBUG( 2,("%s: rounded step_size  = %g", fname, rounded_step_size));

	/* Set the step size. */

	sprintf( command, "S%g\r", rounded_step_size );

	mx_status = mxi_databox_command( databox, command,
					response, sizeof response,
					MXI_DATABOX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the count value. */

	switch( databox->limit_mode ) {
	case MX_DATABOX_CONSTANT_TIME_MODE:
		if ( count_value <= 0L ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Attempted to use an illegal measurement time of %ld seconds."
		"  The minimum allowed measurement time is 1 second.",
				count_value );
		}
		break;

	case MX_DATABOX_CONSTANT_COUNTS_MODE:

		/* In constant counts mode, the count value is interpreted as
		 * being hundreds of counts.  We round to the nearest integer
		 * multiple of 100.
		 */

		old_count_value = count_value;

		count_value = ( old_count_value + 50L ) / 100L;

		if ( (count_value * 100L) != old_count_value ) {
			mx_warning(
"Databox '%s' preset count value of %ld counts was rounded to %ld counts.",
				databox->record->name,
				old_count_value,
				count_value * 100L );
		}

		if ( count_value <= 0L ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Attempted to use an illegal preset count value of %ld.  "
		"The minimum allowed preset count value is 100.",
			old_count_value );
		}
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal Databox limit mode %d.  This is a programming bug.",
			databox->limit_mode );
	}

	sprintf( command, "C%ld\r", count_value );

	mx_status = mxi_databox_command( databox, command,
					response, sizeof response,
					MXI_DATABOX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Return to Monitor mode. */

	mx_status = mxi_databox_set_command_mode( databox,
					MX_DATABOX_MONITOR_MODE, TRUE );
	return mx_status;
}

MX_EXPORT mx_status_type
mxi_databox_discard_unread_input( MX_DATABOX *databox, int debug_flag )
{
	static const char fname[] = "mxi_databox_discard_unread_input()";

	MX_RECORD *rs232_record;
	mx_status_type mx_status;

	if ( databox == (MX_DATABOX *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Databox pointer passed was NULL." );
	}

	rs232_record = databox->rs232_record;

	mx_status = mx_rs232_discard_unread_input( rs232_record, debug_flag );

	return mx_status;
}

