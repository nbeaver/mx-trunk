/*
 * Name:    mdisplay.c
 *
 * Purpose: Motor display motor move function.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2002, 2006, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "motor.h"
#include "mx_key.h"

#define MAX_MOTORS	10

mx_status_type
motor_move_report_function( unsigned long flags,
				long num_motors,
				MX_RECORD **motor_record )
{
	static const char fname[] = "motor_move_report_function()";

	unsigned long *motor_status;
	double current_position;
	int i, motors_busy, abort_motors;
	int exit_loop, return_code;
	mx_status_type mx_status;

	/* Don't bother doing anything if the MXF_MTR_NOWAIT
	 * flag is set.
	 */

	if ( flags & MXF_MTR_NOWAIT ) {
		motor_bypass_limit_switch = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	if ( num_motors <= 0 ) {
		motor_bypass_limit_switch = FALSE;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Invoked with an invalid number of motors = %ld", num_motors );
	}

	motor_status = (unsigned long *) malloc(
				num_motors * sizeof(unsigned long) );

	if ( motor_status == (unsigned long *) NULL ) {
		motor_bypass_limit_switch = FALSE;

		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"%s: Ran out of memory allocating a %ld element motor status array.",
			fname, num_motors );
	}

	/* Print the header (if necessary). */

	if ( flags & MXF_MTR_SHOW_MOVE ) {
		if ( num_motors == 1 ) {
			fprintf( output,
				"   Position");
		} else {
			for ( i = 0; i < num_motors; i++ ) {
				fprintf( output, 
				"   Position(%d) ", i );
			}
		}
		fprintf( output, "\n" );
	}

	/* Begin motor display loop. */

	return_code = SUCCESS;
	exit_loop = FALSE;
	abort_motors = FALSE;

	while ( exit_loop == FALSE ) {
		motors_busy = FALSE;

		for ( i = 0; i < num_motors; i++ ) {

			/* Get current motor position and status. */

			mx_status = mx_motor_get_extended_status(
						motor_record[i],
						&current_position,
						&motor_status[i] );

			if ( mx_status.code != MXE_SUCCESS ) {
				motor_bypass_limit_switch = FALSE;

				mx_free( motor_status );

				return mx_status;
			}

			if ( motor_status[i] & MXSF_MTR_IS_BUSY ) {
				motors_busy = TRUE;
			}

			/* Display the result. */

			if ( flags & MXF_MTR_SHOW_MOVE ) {
				fprintf( output, "%10.4f  ", current_position );
			}
		}

		if ( flags & MXF_MTR_SHOW_MOVE ) {
			fprintf( output, "\n" );
		}

		if ( motors_busy == FALSE ) {
			exit_loop = TRUE;
		}

		/* Display any errors that have occurred. */

		for ( i = 0; i < num_motors; i++ ) {

			if ( motor_status[i] & MXSF_MTR_POSITIVE_LIMIT_HIT ) {
				fprintf( output,
				"Positive hardware limit hit for motor '%s'.\n",
					motor_record[i]->name );

				abort_motors = TRUE;
			}
			if ( motor_status[i] & MXSF_MTR_NEGATIVE_LIMIT_HIT ) {
				fprintf( output,
				"Negative hardware limit hit for motor '%s'.\n",
					motor_record[i]->name );

				abort_motors = TRUE;
			}
			if ( motor_status[i] & MXSF_MTR_SOFT_POSITIVE_LIMIT_HIT)
			{
				fprintf( output,
				"Positive software limit hit for motor '%s'.\n",
					motor_record[i]->name );

				abort_motors = TRUE;
			}
			if ( motor_status[i] & MXSF_MTR_SOFT_NEGATIVE_LIMIT_HIT)
			{
				fprintf( output,
				"Negative software limit hit for motor '%s'.\n",
					motor_record[i]->name );

				abort_motors = TRUE;
			}
			if ( motor_status[i] & MXSF_MTR_FOLLOWING_ERROR ) {
				fprintf( output,
				"Following error detected for motor '%s'.\n",
					motor_record[i]->name );

				abort_motors = TRUE;
			}
			if ( motor_status[i] & MXSF_MTR_DRIVE_FAULT ) {
				fprintf( output,
				"Drive fault detected for motor '%s'.\n",
					motor_record[i]->name );

				abort_motors = TRUE;
			}
			if ( motor_status[i] & MXSF_MTR_AXIS_DISABLED ) {
				fprintf( output,
				"Axis disabled for motor '%s'.\n",
					motor_record[i]->name );

				abort_motors = TRUE;
			}
		}

		if ( motor_bypass_limit_switch ) {
			abort_motors = FALSE;
		}

		/* Check to see if we need to abort the motor moves. */

		if ( (flags & MXF_MTR_IGNORE_KEYBOARD) == 0 ) {
			/* Did someone hit a key on the keyboard? */

			if ( mx_user_requested_interrupt() ) {

				abort_motors = TRUE;

				fprintf( output, "Keyboard interrupt.\n");
			}
		}

		if ( abort_motors ) {
			for ( i = 0; i < num_motors; i++ ) {
				mx_motor_soft_abort( motor_record[i] );
			}

			return_code = FAILURE;
			exit_loop = TRUE;
		}

		if ( exit_loop ) {
			break;
		}

		mx_msleep(1000);
	}

	motor_bypass_limit_switch = FALSE;

	mx_free( motor_status );

	if ( return_code == SUCCESS ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		return mx_error(MXE_INTERRUPTED, fname, "Motor move aborted.");
	}
}

