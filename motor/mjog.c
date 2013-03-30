/*
 * Name:    mjog.c
 *
 * Purpose: Motor jog function.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2006, 2013 Illinois Institute of Technology
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
motor_mjog_fn( int argc, char *argv[] )
{
	static const char cname[] = "mjog";

	MX_RECORD *record;
	MX_MOTOR *motor;
	char *motor_name;
	int c;
	int exit_loop, first_time;
	long naptime, timeout;
	mx_bool_type busy;

	int use_steps, move_motor;
	long steps, relative_steps, jog_step_size;
	double position, relative_motion, step_size, jog_size;
	size_t length;
	mx_status_type mx_status;
	char buffer[100];
	char left_arrow_label[25], ctrl_left_arrow_label[25];
	char right_arrow_label[25], ctrl_right_arrow_label[25];

	if ( argc <= 3 ) {
		fprintf(output,
"Usage: mjog 'motor' 'distance'        - Use single keystrokes to move\n"
"                                        the distance 'distance' in\n"
"                                        engineering units.\n"
"       mjog 'motor' steps 'step_size' - Use single keystrokes to move\n"
"                                        'step_size' motor steps.\n"
"\n"
"         WARNING: Motors controlled entirely by MX will have\n"
"                       NO BACKLASH CORRECTION\n"
"                        when moved by mjog.\n"
		);
		return FAILURE;
	}

	use_steps = FALSE;
	relative_steps = jog_step_size = 0L;
	relative_motion = step_size = jog_size = 0.0;

	motor_name = argv[2];

	/* Get pointer to MX_MOTOR structure. */

	record = mx_get_record( motor_record_list, argv[2] );

	if ( record == (MX_RECORD *) NULL ) {
		fprintf(output,
		"%s: Invalid device name '%s'\n", cname, motor_name);

		return FAILURE;
	}

	if ( record->mx_class != MXC_MOTOR ) {
		fprintf(output,
		"%s: '%s' is not a motor.\n", cname, motor_name);

		return FAILURE;
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		fprintf(output,
		"%s: MX_MOTOR pointer for record '%s' is NULL.\n",
			cname, motor_name);

		return FAILURE;
	}

	/* The following bunch of nested 'if's is to parse all
	 * the various variations on the 'mjog' command.
	 */

	if ( argc == 4 ) {

		/* mjog theta steps */

		length = strlen( argv[3] );

		if ( length == 0 )
			length = 1;

		if ( strncmp( "steps", argv[3], length ) == 0 ) {

			fprintf(output,
			"Error: must specify a jog distance in steps.\n");

			return FAILURE;

		} else {
			/* mjog theta 4.0 */

			jog_size = atof( argv[3] );
			use_steps = FALSE;
		}

	} else if ( argc == 5 ) {	/* mjog theta steps 5 */

		length = strlen( argv[3] );

		if ( length == 0 )
			length = 1;

		if ( strncmp( "steps", argv[3], length ) != 0 ) {
			fprintf(output,
	"%s: Syntax error.  If the 'mjog' command has three arguments,\n"
	"      the second one should be the word 'steps'.\n", cname );

			return FAILURE;
		}

		if ( motor->subclass != MXC_MTR_STEPPER ) {
			fprintf(output,
"%s: '%s' is not a stepping motor, so can't specify movement by steps.\n",
				cname, motor_name);

			return FAILURE;
		}

		jog_step_size = atol( argv[4] );
		use_steps = TRUE;

	} else if ( argc > 5 ) {
		fprintf(output,
			"%s: extra parameter '%s' ignored.\n", cname, argv[5]);
		return FAILURE;
	} else {
		fprintf(output,
			"%s: Somehow got here with argc (%d) less than 4.\n",
			cname, argc );
		return FAILURE;
	}

	/* === End of 'mjog' argument parsing. === */

#ifdef OS_UNIX
	strlcpy(left_arrow_label, "-", sizeof(left_arrow_label) );
	strlcpy(right_arrow_label, "+", sizeof(right_arrow_label) );
	strlcpy(ctrl_left_arrow_label, "<", sizeof(ctrl_left_arrow_label) );
	strlcpy(ctrl_right_arrow_label, ">", sizeof(ctrl_right_arrow_label) );
#else
	strlcpy(left_arrow_label, "<left-arrow>", sizeof(left_arrow_label) );
	strlcpy(right_arrow_label, "<right-arrow>", sizeof(right_arrow_label) );
	strlcpy(ctrl_left_arrow_label, "ctrl-<left-arrow>",
					sizeof(ctrl_left_arrow_label) );
	strlcpy(ctrl_right_arrow_label, "ctrl-<right-arrow>",
					sizeof(ctrl_right_arrow_label) );
#endif


	if ( use_steps ) {
		fprintf( output,
		"MJOG '%s' started.  Hit <ctrl-D> or <escape> to exit.\n",
				motor_name);
		fprintf( output,
			"    Hit %s to move by %ld steps.\n",
				right_arrow_label, jog_step_size);
		fprintf( output,
			"    Hit %s to move by %ld steps.\n",
				left_arrow_label, -jog_step_size);
		fprintf( output,
			"    Hit %s to move by %ld steps.\n",
				ctrl_right_arrow_label, 10 * jog_step_size);
		fprintf( output,
			"    Hit %s to move by %ld steps.\n",
				ctrl_left_arrow_label, -10 * jog_step_size);
		fprintf( output, "\n");
	} else {
		fprintf( output,
		"MJOG '%s' started.  Hit <ctrl-D> or <escape> to exit.\n",
				motor_name);
		fprintf( output,
			"    Hit %s to move by %.*g %s.\n",
				right_arrow_label, record->precision,
				jog_size, motor->units);
		fprintf( output,
			"    Hit %s to move by %.*g %s.\n",
				left_arrow_label, record->precision,
				-jog_size, motor->units);
		fprintf( output,
			"    Hit %s to move by %.*g %s.\n",
				ctrl_right_arrow_label, record->precision,
				10.0 * jog_size, motor->units);
		fprintf( output,
			"    Hit %s to move by %.*g %s.\n",
				ctrl_left_arrow_label, record->precision,
				-10.0 * jog_size, motor->units);
		fprintf( output, "\n");
	}

	c = 0;
	exit_loop = FALSE;
	first_time = TRUE;

	while (exit_loop == FALSE) {

		/* Wait until the motor is not busy. */

		busy = TRUE;

		naptime = 100;		/* milliseconds */

#if 0
		if ( jog_size <= 20 ) {
			timeout = 10000;	/* milliseconds */
		} else {
			timeout = 100000;	/* milliseconds */
		}
#else
		timeout = 100000;	/* 100 seconds */
#endif

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

		if ( ! first_time ) {
			if ( use_steps ) {
				mx_status = mx_motor_get_position_steps( 
							record, &steps );

				if ( mx_status.code != MXE_SUCCESS ) {
					return FAILURE;
				}

				fprintf( output, "%ld  (%.*g %s)\n",
					steps,
					record->precision,
					step_size * (double) steps,
					motor->units );
			} else {
				mx_status = mx_motor_get_position(
							record, &position );

				if ( mx_status.code != MXE_SUCCESS ) {
					return FAILURE;
				}

				fprintf( output, "%.*g %s\n",
					record->precision,
					position, motor->units );
			}

			fflush( output );
		} else {
			first_time = FALSE;
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

		move_motor = FALSE;

		/* Exit on escape key. */

		if ( c == ESC || c == CTRL_D ) {
			break;		/* Exit the while() loop. */
		}

		switch(c) {
		case '-':	/* Move down one jog increment. */
		case 'h':
		case 'H':
		case KEY_LEFT:
			if ( use_steps ) {
				relative_steps = - jog_step_size;
			} else {
				relative_motion = - jog_size;
			}
			move_motor = TRUE;

			break;

		/* For some reason, on a PC keyboard under Linux,
		 * the keypad '+' key gets translated to a ','
		 * character.  For that reason, I include a case ','
		 * below.
		 */

		case '+':	/* Move up one jog increment. */
		case ',':
		case 'l':
		case 'L':
		case KEY_RIGHT:
			if ( use_steps ) {
				relative_steps = jog_step_size;
			} else {
				relative_motion = jog_size;
			}
			move_motor = TRUE;

			break;

		case '<':	/* Move down ten jog increments. */
		case CTRL_H:
		case KEY_CTRL_LEFT:
			if ( use_steps ) {
				relative_steps = -10 * jog_step_size;
			} else {
				relative_motion = -10.0 * jog_size;
			}
			move_motor = TRUE;

			break;

		case '>':	/* Move up ten jog increments. */
		case CTRL_L:
		case KEY_CTRL_RIGHT:
			if ( use_steps ) {
				relative_steps = 10 * jog_step_size;
			} else {
				relative_motion = 10.0 * jog_size;
			}
			move_motor = TRUE;

			break;

		default:
			if ( use_steps ) {
				relative_steps = 0;
			} else {
				relative_motion = 0.0;
			}

			fprintf( output,
			"%s: unrecognized command key '%c' (value = %d)\n",
				cname, c, c);
			break;
		}

		if ( move_motor ) {
			if ( use_steps ) {
				fprintf( output,
				"Moving '%s' by %ld steps to ",
				motor_name, relative_steps );

				mx_status
				  = mx_motor_move_relative_steps_with_report(
					record,
					relative_steps,
					motor_move_report_function,
				MXF_MTR_IGNORE_BACKLASH | MXF_MTR_NOWAIT);
			} else {
				fprintf( output,
					"Moving '%s' by %.*g %s to ",
					motor_name, record->precision,
					relative_motion, motor->units );

				mx_status = mx_motor_move_relative_with_report(
					record,
					relative_motion,
					motor_move_report_function,
				MXF_MTR_IGNORE_BACKLASH | MXF_MTR_NOWAIT);
			}
		}
	}

	/* Rewrite the mxmotor.dat file with the new position. */

	if ( allow_motor_database_updates ) {
		fprintf( output, "Saving new position to 'mxmotor.dat'\n");

		snprintf( buffer, sizeof(buffer),
			"save motor %s", motor_savefile );

		cmd_execute_command_line( command_list_length,
						command_list, buffer );
	}

	fprintf( output, "MJOG finished.\n");

	return SUCCESS;
}

