/*
 * Name:    example1.c
 *
 * Purpose: This example performs a simple step scan of a motor
 *          using ordinary motor, scaler, and timer calls.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_record.h"
#include "mx_motor.h"
#include "mx_scaler.h"
#include "mx_timer.h"

#define DATABASE_FILENAME	"example.dat"

int
main( int argc, char *argv[] )
{
	MX_RECORD *record_list;

	MX_RECORD *energy_motor_record;
	MX_RECORD *i_zero_record;
	MX_RECORD *i_trans_record;
	MX_RECORD *timer_record;

	double start, step_size, measurement_time, energy;
	int i, num_steps, error, busy;
	long i_zero_value, i_trans_value;

	mx_status_type status;

	/* Read in the MX database used by this example. */

	status = mx_setup_database( &record_list, DATABASE_FILENAME );

	if ( status.code != MXE_SUCCESS ) {
		fprintf( stderr, "Cannot setup the MX database '%s'.\n",
				DATABASE_FILENAME );
		exit(1);
	}

	/* Find the devices to be used by the scan. */

	energy_motor_record = mx_get_record( record_list, "energy" );

	if ( energy_motor_record == (MX_RECORD *) NULL ) {
		fprintf( stderr, "Cannot find the motor 'energy'.\n" );
		exit(1);
	}

	i_zero_record = mx_get_record( record_list, "Io" );

	if ( i_zero_record == (MX_RECORD *) NULL ) {
		fprintf( stderr, "Cannot find the scaler 'Io'.\n" );

		exit(1);
	}

	i_trans_record = mx_get_record( record_list, "It" );

	if ( i_trans_record == (MX_RECORD *) NULL ) {
		fprintf( stderr, "Cannot find the scaler 'It'.\n" );

		exit(1);
	}

	timer_record = mx_get_record( record_list, "timer1" );

	if ( timer_record == (MX_RECORD *) NULL ) {
		fprintf( stderr, "Cannot find the timer 'timer1'.\n" );
		exit(1);
	}

	/* Setup the scan parameters for a copper K edge scan. */

	start = 8950;			/* in eV */
	step_size = 1.0;		/* in eV */
	num_steps = 101;
	measurement_time = 1.0;		/* in seconds */

	/* Perform the scan. */

	error = FALSE;

	printf( "Moving to start position.\n" );

	for ( i = 0; i < num_steps; i++ ) {

		/* Clear the scalers. */

		status = mx_scaler_clear( i_zero_record );

		if ( status.code != MXE_SUCCESS ) {
			error = TRUE;
			break;
		}

		status = mx_scaler_clear( i_trans_record );

		if ( status.code != MXE_SUCCESS ) {
			error = TRUE;
			break;
		}

		/* Compute the energy for the next step. */

		energy = start + step_size * (double) i;

		/* Move to the new energy. */

		status = mx_motor_move_absolute( energy_motor_record,
							energy, 0 );

		if ( status.code != MXE_SUCCESS ) {
			error = TRUE;
			break;
		}

		/* Wait for the motor to stop moving. */

		busy = TRUE;

		while ( busy ) {
			mx_msleep(10);       /* Wait for 10 milliseconds */

			status = mx_motor_is_busy( energy_motor_record, &busy );

			if ( status.code != MXE_SUCCESS ) {
				error = TRUE;
				break;
			}
		}

		/* Start the measurement timer. */

		status = mx_timer_start( timer_record, measurement_time );

		if ( status.code != MXE_SUCCESS ) {
			error = TRUE;
			break;
		}

		/* Wait until the timer stops counting. */

		busy = TRUE;

		while ( busy ) {
			mx_msleep(10);       /* Wait for 10 milliseconds */

			status = mx_timer_is_busy( timer_record, &busy );

			if ( status.code != MXE_SUCCESS ) {
				error = TRUE;
				break;
			}
		}

		/* Read out the scalers. */

		status = mx_scaler_read( i_zero_record, &i_zero_value );

		if ( status.code != MXE_SUCCESS ) {
			error = TRUE;
			break;
		}

		status = mx_scaler_read( i_trans_record, &i_trans_value );

		if ( status.code != MXE_SUCCESS ) {
			error = TRUE;
			break;
		}

		/* Print out the values. */

		printf( "%10.3f  %10lu  %10lu\n",
			energy, i_zero_value, i_trans_value );
	}

	if ( error ) {
		fprintf(stderr, "The scan aborted abnormally.\n" );
		exit(1);
	}

	exit(0);
}

