/*
 * Name:    example1_lowlevel.c
 *
 * Purpose: This example performs a simple step scan using lowlevel
 *          mx_get() and mx_put() calls directly.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_net.h"
#include "mx_constants.h"

#define DATABASE_FILENAME	"example.dat"

#define SERVER_NAME		"localhost"
#define SERVER_PORT		9727

int
main( int argc, char *argv[] )
{
	MX_RECORD *server_record;

	double start, step_size, measurement_time;
	double energy, theta, theta_in_radians, sin_theta, d_spacing;
	int i, num_steps, error;
	long i_zero_value, i_trans_value;
	mx_bool_type busy, clear;
	mx_status_type mx_status;

	MX_NETWORK_FIELD destination_nf;
	MX_NETWORK_FIELD i_trans_clear_nf;
	MX_NETWORK_FIELD i_trans_value_nf;
	MX_NETWORK_FIELD i_zero_clear_nf;
	MX_NETWORK_FIELD i_zero_value_nf;
	MX_NETWORK_FIELD motor_busy_nf;
	MX_NETWORK_FIELD timer_value_nf;
	MX_NETWORK_FIELD timer_busy_nf;

	/* You have two choices on how to connect to the server.
	 * 
	 * 1.  Use the mx_setup_database() call.  This requires you
	 *     to have an MX database file available.
	 *
	 * 2.  Use the mx_connect_to_mx_server() call defined in the
	 *     accompanying file mx_server_connect.c which does _not_
	 *     require a database file.  It was extracted from the
	 *     'mxupdate' program.  This function should probably be
	 *     added to libMx and I guess I will do so in the next
	 *     release.
	 */

#if 0
	{
		MX_RECORD *record_list;

		mx_status = mx_setup_database(&record_list, DATABASE_FILENAME);

		if ( mx_status.code != MXE_SUCCESS ) {
			fprintf( stderr, "Cannot setup the MX database.\n" );
			exit(1);
		}

		server_record = mx_get_record( record_list, "server_record" );

		if ( server_record == NULL ) {
			fprintf( stderr,
			"Cannot find the record 'server_record'\n" );

			exit(1);
		}
	}
#else
	mx_status = mx_connect_to_mx_server( &server_record,
						SERVER_NAME, SERVER_PORT, 8 );

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( stderr, "Cannot connect to the MX server.\n" );
		exit(1);
	}
#endif

	/* Setup the scan parameters for a copper K edge scan. */

	start = 8950;			/* in eV */
	step_size = 1.0;		/* in eV */
	num_steps = 101;
	measurement_time = 1.0;		/* in seconds */

	/* Get the crystal d-spacing.
	 *
	 * We only need to get this value once, so we use the non-cacheing
	 * mx_get_by_name() call which makes only one call to the MX server.
	 *
	 * If we needed to get the d-spacing several different times, it
	 * would be better to use the mx_get() call shown below since it
	 * automatically caches the MX network handle in the supplied
	 * MX_NETWORK_FIELD structure.
	 */

	mx_status = mx_get_by_name( server_record, "d_spacing.value",
					MXFT_DOUBLE, &d_spacing );

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( stderr, "Unable to get the crystal d-spacing.\n" );
		exit(1);
	}

	/* Initialize the network field structures. */

	mx_network_field_init( &destination_nf, server_record,
							"theta.destination" );
	mx_network_field_init( &i_trans_clear_nf, server_record, "It.clear" );
	mx_network_field_init( &i_trans_value_nf, server_record, "It.value" );
	mx_network_field_init( &i_zero_clear_nf, server_record, "Io.clear" );
	mx_network_field_init( &i_zero_value_nf, server_record, "Io.value" );
	mx_network_field_init( &motor_busy_nf, server_record, "theta.busy" );
	mx_network_field_init( &timer_busy_nf, server_record, "timer1.busy" );
	mx_network_field_init( &timer_value_nf, server_record, "timer1.value" );

	/* Perform the scan. */

	error = FALSE;

	clear = 1;
		
	printf( "Moving to start position.\n" );

	for ( i = 0; i < num_steps; i++ ) {

		/* Clear the scalers. */

		mx_status = mx_put( &i_zero_clear_nf, MXFT_BOOL, &clear );

		if ( mx_status.code != MXE_SUCCESS ) {
			error = TRUE;
			break;
		}

		mx_status = mx_put( &i_trans_clear_nf, MXFT_BOOL, &clear );

		if ( mx_status.code != MXE_SUCCESS ) {
			error = TRUE;
			break;
		}

		/* Compute the energy and theta for the next step. */

		energy = start + step_size * (double) i;

		sin_theta = mx_divide_safely( MX_HC, 2.0 * d_spacing * energy );

		theta_in_radians = asin( sin_theta );

		theta = theta_in_radians * 180.0 / MX_PI;

		/* Move to the new theta position. */

		mx_status = mx_put( &destination_nf, MXFT_DOUBLE, &theta );

		if ( mx_status.code != MXE_SUCCESS ) {
			error = TRUE;
			break;
		}

		/* Wait for the motor to stop moving. */

		busy = TRUE;

		while( busy ) {
			mx_msleep(10);		/* Wait for 10 milliseconds */

			mx_status = mx_get( &motor_busy_nf, MXFT_BOOL, &busy );

			if ( mx_status.code != MXE_SUCCESS ) {
				error = TRUE;
				break;
			}
		}

		/* Start the measurement timer. */

		mx_status = mx_put( &timer_value_nf,
					MXFT_DOUBLE, &measurement_time );

		if ( mx_status.code != MXE_SUCCESS ) {
			error = TRUE;
			break;
		}

		/* Wait until the timer stops counting. */

		busy = TRUE;

		while( busy ) {
			mx_msleep(10);		/* Wait for 10 milliseconds */

			mx_status = mx_get( &timer_busy_nf, MXFT_BOOL, &busy );

			if ( mx_status.code != MXE_SUCCESS ) {
				error = TRUE;
				break;
			}
		}

		/* Read out the scalers. */

		mx_status = mx_get( &i_zero_value_nf,
					MXFT_LONG, &i_zero_value );

		if ( mx_status.code != MXE_SUCCESS ) {
			error = TRUE;
			break;
		}

		mx_status = mx_get( &i_trans_value_nf,
					MXFT_LONG, &i_trans_value );

		if ( mx_status.code != MXE_SUCCESS ) {
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

