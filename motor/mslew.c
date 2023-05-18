/*
 * Name:    mslew.c
 *
 * Purpose: Motor slew function.
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
#include <string.h>
#include <stdlib.h>

#include "motor.h"
#include "mx_key.h"
#include "keycodes.h"

int
motor_mslew_fn( int argc, char *argv[] )
{
	static const char cname[] = "mslew";

	MX_RECORD *record;
	MX_MOTOR *motor;
	char *motor_name;
	long direction;
	int c;
	mx_bool_type exit_loop, first_time, busy;
	unsigned long naptime, timeout;
	mx_status_type mx_status;

	if ( argc <= 3 ) {
		fprintf(output,
"Usage: mslew 'motor' 'direction' - Use single keystrokes to slew the\n"
"                                   direction 'direction'.  The motor\n"
"                                   will continue slewing until the user\n"
"                                   aborts the move, or the motor hits a\n"
"                                   limit, or some other event stops it.\n"
"\n"
"         WARNING: Motors controlled entirely by MX will have\n"
"                       NO BACKLASH CORRECTION\n"
"                        when moved by mslew.\n"
		);
		return FAILURE;
	}

	/* Get pointer to MX_MOTOR structure. */

	motor_name = argv[2];

	record = mx_get_record( motor_record_list, motor_name );

	if ( record == (MX_RECORD *) NULL ) {
		fprintf(output,
		"%s: Invalid device name '%s'\n", cname, motor_name );

		return FAILURE;
	}

	if ( record->mx_class != MXC_MOTOR ) {
		fprintf(output,
		"%s: '%s' is not a motor.\n", cname, motor_name );

		return FAILURE;
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		fprintf(output,
		"%s: MX_MOTOR pointer for record '%s' is NULL.\n",
			cname, motor_name);

		return FAILURE;
	}

	/* mslew theta + */

	if ( strcmp( argv[3], "+" ) == 0 ) {
		direction = 1;
	} else
	if ( strcmp( argv[3], "-" ) == 0 ) {
		direction = -1;
	} else
	if ( strcmp( argv[3], "1" ) == 0 ) {
		direction = 1;
	} else
	if ( strcmp( argv[3], "-1" ) == 0 ) {
		direction = -1;
	} else
	if ( strcmp( argv[3], "0" ) == 0 ) {
		direction = 0;
	} else {
	}

	fprintf( output,
	"MSLEW '%s' started.  Hit <ctrl-D> or <escape> to exit.\n",
				motor_name);
	c = 0;
	exit_loop = FALSE;
	first_time = TRUE;

	while (exit_loop == FALSE) {

		/* Wait until the motor is not busy. */

		busy = TRUE;

		naptime = 100;		/* milliseconds */

		timeout = 100000;	/* 100 seconds */

		while (busy && timeout > 0) {
			mx_status = mx_motor_is_busy( record, &busy );

			if ( mx_status.code != MXE_SUCCESS ) {
				return FAILURE;
			}

			if ( busy ) {
				timeout -= naptime;
				mx_msleep( (unsigned long) naptime );
			}
		}

		if ( timeout <= 0 ) {
			fprintf( output,
			"Timed out waiting for motor to stop moving.\n");

			break;		/* Exit the while() loop. */
		}

		/* Read a character from the keyboard. */

		c = mx_getch();
#ifdef OS_MSDOS
		/* Mask off high byte of ordinary characters. 
		 * Characters with a low byte of all zeros are
		 * PC scan codes like KEY_LEFT.
		 */

		if ( c & 0xFF ) {
			c &= 0xFF;
		}
#endif /* OS_MSDOS */

		/* Exit on escape key. */

		if ( c == ESC || c == CTRL_D ) {
			break;		/* Exit the while() loop. */
		}

	}

	fprintf( output, "MJOG finished.\n");

	return SUCCESS;
}

