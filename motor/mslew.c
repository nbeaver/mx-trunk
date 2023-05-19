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
	double motor_speed;
	long direction;
	int c;
	mx_bool_type exit_loop, busy;
	mx_status_type mx_status;

	if ( argc <= 4 ) {
		fprintf(output,
"Usage: mslew 'motor' 'motor_speed' 'direction' - Start a constant velocity\n"
"                           move in the requested direction 'direction'.  \n"
"                           The motor will continue slewing until the user\n"
"                           aborts the move, or the motor hits a limit, or\n"
"                           some other event stops it.\n"
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

	/* Set the requested speed. */

	motor_speed = atof( argv[3] );

	if ( motor_speed > 0.0 ) {
		mx_status = mx_motor_set_speed( record, motor_speed );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	}

	/* Get the requested direction. */

	if ( strcmp( argv[4], "+" ) == 0 ) {
		direction = 1;
	} else
	if ( strcmp( argv[4], "-" ) == 0 ) {
		direction = -1;
	} else
	if ( strcmp( argv[4], "1" ) == 0 ) {
		direction = 1;
	} else
	if ( strcmp( argv[4], "-1" ) == 0 ) {
		direction = -1;
	} else
	if ( strcmp( argv[4], "0" ) == 0 ) {
		direction = 0;
	} else {
		direction = 0;
	}

	mx_status = mx_motor_constant_velocity_move( record, direction );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	fprintf( output,
	"MSLEW '%s' started.  Hit <ctrl-D> or <escape> to exit.\n",
				motor_name);
	c = 0;
	exit_loop = FALSE;

	while (exit_loop == FALSE) {

		mx_msleep(1000);

		mx_status = mx_motor_is_busy( record, &busy );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		if ( busy == FALSE) {
			mx_status = mx_motor_stop( record );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output, "Motor '%s' move complete.\n",
							record->name );
			return SUCCESS;
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

	mx_status = mx_motor_stop( record );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	fprintf( output, "MJOG finished.\n");

	return SUCCESS;
}

