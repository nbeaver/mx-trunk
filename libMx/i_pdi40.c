/*
 * Name:    i_pdi40.c
 *
 * Purpose: MX driver for Prairie Digital, Inc. Model 40 data acquisition
 *          and control module.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006, 2010, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "i_pdi40.h"

MX_RECORD_FUNCTION_LIST mxi_pdi40_record_function_list = {
	NULL,
	mxi_pdi40_create_record_structures,
	mxi_pdi40_finish_record_initialization,
	NULL,
	NULL,
	mxi_pdi40_open
};

MX_GENERIC_FUNCTION_LIST mxi_pdi40_generic_function_list = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

MX_RECORD_FIELD_DEFAULTS mxi_pdi40_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_PDI40_STANDARD_FIELDS
};

long mxi_pdi40_num_record_fields
		= sizeof( mxi_pdi40_record_field_defaults )
			/ sizeof( mxi_pdi40_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_pdi40_rfield_def_ptr
			= &mxi_pdi40_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_pdi40_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_pdi40_create_record_structures()";

	MX_GENERIC *generic;
	MX_PDI40 *pdi40;

	/* Allocate memory for the necessary structures. */

	generic = (MX_GENERIC *) malloc( sizeof(MX_GENERIC) );

	if ( generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GENERIC structure." );
	}

	pdi40 = (MX_PDI40 *) malloc( sizeof(MX_PDI40) );

	if ( pdi40 == (MX_PDI40 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_PDI40 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = generic;
	record->record_type_struct = pdi40;
	record->class_specific_function_list
				= &mxi_pdi40_generic_function_list;

	generic->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pdi40_finish_record_initialization( MX_RECORD *record )
{
	MX_PDI40 *pdi40;

	pdi40 = (MX_PDI40 *) record->record_type_struct;

	/* Set the stepper motor parameters to illegal defaults. */

	pdi40->stepper_mode = 'Z';
	pdi40->stepper_speed = -1;
	pdi40->stepper_stop_delay = -1;
	pdi40->currently_moving_stepper = '\0';
	pdi40->current_move_distance = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pdi40_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_pdi40_open()";

	MX_PDI40 *pdi40;
	MX_RS232 *rs232;
	char response[80];
	char c;
	mx_status_type mx_status;

	MX_DEBUG(2, ("mxi_pdi40_open() invoked."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	pdi40 = (MX_PDI40 *) (record->record_class_struct);

	if ( pdi40 == (MX_PDI40 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PDI40 pointer for record '%s' is NULL.", record->name);
	}

	/* Require the RS232 line to be 9600 baud, 8 data bits,
	 * no parity, and 1 stop bit.
	 */

	rs232 = (MX_RS232 *) pdi40->rs232_record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RS232 pointer for record '%s' is NULL.", record->name);
	}

	if ( (rs232->speed != 9600)
	  || (rs232->word_size != 8)
	  || (rs232->parity != MXF_232_NO_PARITY)
	  || (rs232->stop_bits != 1) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"RS232 config is invalid: Must be 9600 baud, 8 bit, no parity, 1 stop bit.");
	}

	/* Reset the PDI40 controller to its power-up configuration. */

	mx_status = mxi_pdi40_putline( pdi40, "RESET", FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/******* Read the power up banner, which is four lines long. *******/

		/* The first line is just '\0', CR, LF. */

	mx_status = mxi_pdi40_getline( pdi40, response, sizeof response, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

		/* Read the identification banner. */

	mx_status = mxi_pdi40_getline( pdi40, response, sizeof response, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( response, "Welcome to the ITC232-A" ) != 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Did not see the normal power up banner; instead saw '%s'",
		response );
	}

		/* Read the help message. */

	mx_status = mxi_pdi40_getline( pdi40, response, sizeof response, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( response, "? or h for help" ) != 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Did not see the normal power up help message; instead saw '%s'",
		response );
	}

		/* Next line is just a bell character. */

	mx_status = mxi_pdi40_getline( pdi40, response, sizeof response, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( response, "\007" ) != 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Did not see the normal power up beep; instead saw '%s'",
		response );
	}

		/* The response is terminated by the MX_PDI40_END_OF_RESPONSE
		 * character.
		 */

	mx_status = mxi_pdi40_getc( pdi40, &c, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( c != MX_PDI40_END_OF_RESPONSE ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Did not see the PDI40 end of response character; instead saw '%c'",
			c );
	}

	/* Configure Results defaults to (D)ecimal after a reset which
	 * is what we want, so we need do nothing about it here.
	 */

	return MX_SUCCESSFUL_RESULT;
}

/* === Functions specific to this driver. === */

#define ALWAYS_SHOW_BUFFER  FALSE

MX_EXPORT mx_status_type
mxi_pdi40_getline( MX_PDI40 *pdi40,
		char *buffer, int buffer_length, int debug_flag )
{
	static const char fname[] = "mxi_pdi40_getline()";

	char c;
	int i;
	mx_status_type mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2, ("mxi_pdi40_getline() invoked."));
	}

	for ( i = 0; i < (buffer_length - 1) ; i++ ) {
		mx_status = mx_rs232_getchar( pdi40->rs232_record,
						&c, MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS ) {
			/* Make the buffer contents a valid C string
			 * before returning, so that we can at least
			 * see what appeared before the error.
			 */

			buffer[i] = '\0';

			if ( debug_flag ) {
				MX_DEBUG(-2,
				("mxi_pdi40_getline() failed.\nbuffer = '%s'",
					buffer));

				MX_DEBUG(-2,
				("Failed with status = %ld, c = 0x%x '%c'",
				mx_status.code, c, c));
			}

			return mx_status;
		}

		if ( debug_flag ) {
			MX_DEBUG(-2,
			("mxi_pdi40_getline: received c = 0x%x '%c'", c, c));
		}

		buffer[i] = c;

		/* Responses from the PDI40 should be terminated
		 * by the end of line sequence CR, LF.
		 */

		if ( c == MX_PDI40_END_OF_LINE ) {
			i++;

			/* There is also a line feed character to throw away.*/

			mx_status = mx_rs232_getchar( pdi40->rs232_record,
						&c, MXF_232_WAIT );

			if ( mx_status.code != MXE_SUCCESS ) {
				if ( debug_flag ) {
					MX_DEBUG(-2,
			("mxi_pdi40_getline: failed to see linefeed."));
				}

				return mx_status;
			}

			if ( debug_flag ) {
				MX_DEBUG(-2,
			("mxi_pdi40_getline: received c = 0x%x '%c'", c, c));
			}

			break;		/* exit the for() loop */
		}
	}

	/* Check to see if the last character was an end of string
	 * and if it is then overwrite it with a NULL.
	 */

	if ( buffer[i-1] != MX_PDI40_END_OF_LINE ) {
		mx_status = mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Warning: Input buffer overrun." );

		buffer[i] = '\0';
	} else {
		mx_status = MX_SUCCESSFUL_RESULT;

		buffer[i-1] = '\0';
	}

#if ALWAYS_SHOW_BUFFER
	if ( 1 ) {
#else
	if ( debug_flag ) {
#endif
		MX_DEBUG(-2, ("mxi_pdi40_getline: buffer = '%s'", buffer) );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_pdi40_putline( MX_PDI40 *pdi40, char *buffer, int debug_flag )
{
	char *ptr;
	char c;
	mx_status_type mx_status;

#if ALWAYS_SHOW_BUFFER
	if ( 1 ) {
#else
	if ( debug_flag ) {
#endif
		MX_DEBUG(-2, ("mxi_pdi40_putline: buffer = '%s'",buffer));
	}

	ptr = &buffer[0];

	while ( *ptr != '\0' ) {
		c = *ptr;

		mx_status = mx_rs232_putchar( pdi40->rs232_record,
						c, MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG(-2,
			("mxi_pdi40_putline: sent 0x%x '%c'", c, c));
		}

		ptr++;
	}

	/* Was the last character an end of string character?
	 * If not, then send one.
	 */

	if ( *(ptr - 1) != MX_PDI40_END_OF_COMMAND ) {
		c = MX_PDI40_END_OF_COMMAND;

		/* Send the end of string. */

		mx_status = mx_rs232_putchar( pdi40->rs232_record,
						c, MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG(-2,
			("mxi_pdi40_putline(eos): sent 0x%x '%c'", c, c));
		}

	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pdi40_getc( MX_PDI40 *pdi40, char *c, int debug_flag )
{
	mx_status_type mx_status;

	mx_status = mx_rs232_getchar( pdi40->rs232_record, c, MXF_232_WAIT );

#if ALWAYS_SHOW_BUFFER
	if ( 1 ) {
#else
	if ( debug_flag ) {
#endif
		MX_DEBUG(-2, ("mxi_pdi40_getc: received 0x%x '%c'", *c, *c));
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_pdi40_putc( MX_PDI40 *pdi40, char c, int debug_flag )
{
	mx_status_type mx_status;

	mx_status = mx_rs232_putchar( pdi40->rs232_record, c, MXF_232_WAIT );

#if ALWAYS_SHOW_BUFFER
	if ( 1 ) {
#else
	if ( debug_flag ) {
#endif
		MX_DEBUG(-2, ("mxi_pdi40_putc: sent 0x%x '%c'", c, c));
	}

	return mx_status;
}

/* mxi_pdi40_is_any_motor_busy() exists because when a PDI40 stepping
 * motor is in motion, all other PDI40 commands except (S)top motor
 * are locked out.  Thus, we must wait until the motor stops moving
 * before sending another command.  This has the unfortunate side
 * effect of prohibiting more than one PDI40 controlled stepping
 * motor from moving at a time.
 */

MX_EXPORT mx_status_type
mxi_pdi40_is_any_motor_busy( MX_PDI40 *pdi40, mx_bool_type *a_motor_is_busy )
{
	static const char fname[] = "mxi_pdi40_is_any_motor_busy()";

	MX_MOTOR *motor;
	char buffer[80];
	char c;
	mx_status_type mx_status;

	/* First test the flag in the MX_PDI40 structure. */

	if ( pdi40->currently_moving_stepper == '\0' ) {

		*a_motor_is_busy = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* The most recently started motor move may have finished by now.
	 * When the move is complete, the PDI40 will send a blank line
	 * followed by "OK", followed by another blank line and then
	 * the character '>'.
	 *
	 * If the motor is not finished moving, the following call to
	 * mxi_pdi40_getline() will fail.
	 */

	mx_status = mxi_pdi40_getline( pdi40, buffer, sizeof buffer, FALSE );

	if ( mx_status.code != MXE_SUCCESS ) {

		/* MXE_NOT_READY means that no characters were read
		 * and the motor move is not complete.  In that case,
		 * return MX_SUCCESSFUL_RESULT, but otherwise return
		 * the error status.
		 *
		 * In any case, say that a motor is busy, since
		 * we take busy to be the default;
		 */

		*a_motor_is_busy = TRUE;

		if ( mx_status.code == MXE_NOT_READY ) {
			return MX_SUCCESSFUL_RESULT;
		} else {
			return mx_status;
		}
	}

	/* If we get here, then the motor has probably stopped moving.
	 * But we still test for the rest of the handshake sequence.
	 */

	if ( strcmp( buffer, "" ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Did not get blank line before the 'OK' response; instead saw '%s'",
			buffer );
	}

	mx_status = mxi_pdi40_getline( pdi40, buffer, sizeof buffer, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( buffer, "OK" ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Did not get 'OK' response to PDI40 command.  Response was '%s'",
			buffer );
	}

	mx_status = mxi_pdi40_getline( pdi40, buffer, sizeof buffer, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( buffer, "" ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Did not get blank line after the 'OK' response; instead saw '%s'",
			buffer );
	}

	mx_status = mxi_pdi40_getc( pdi40, &c, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( c != MX_PDI40_END_OF_RESPONSE ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Did not see the PDI40 end of response character; instead saw '%c'",
			c );
	}

	/* OK, the motor that was moving is now stopped.  We need to
	 * update its software position.
	 */

	switch( pdi40->currently_moving_stepper ) {
	case 'A':	motor = pdi40->stepper_motor[ MX_PDI40_STEPPER_A ];
			break;
	case 'B':	motor = pdi40->stepper_motor[ MX_PDI40_STEPPER_B ];
			break;
	case 'C':	motor = pdi40->stepper_motor[ MX_PDI40_STEPPER_C ];
			break;
	default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The PDI40 motor that supposedly just stopped has an illegal name = '%c'",
				pdi40->currently_moving_stepper );
	}

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
"The PDI40 motor that supposedly just stopped (%c) is not in the database.",
				pdi40->currently_moving_stepper );
	}

	/* Update the position. */

	motor->raw_position.stepper += pdi40->current_move_distance;

	/* Now clean up and return. */

	pdi40->currently_moving_stepper = '\0';
	pdi40->current_move_distance = 0;

	return MX_SUCCESSFUL_RESULT;
}

