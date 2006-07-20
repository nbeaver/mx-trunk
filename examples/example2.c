/*
 * Name:    example2.c
 *
 * Purpose: This example performs a simple step scan of a motor
 *          by creating and then executing an MX scan record.
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
#include "mx_scan.h"

#define DATABASE_FILENAME	"example.dat"

int
main( int argc, char *argv[] )
{
	MX_RECORD *record_list;

	MX_RECORD *scan_record;

	char description[250];

	double start, step_size, measurement_time, settling_time;
	int num_steps;

	mx_status_type status;

	/* Read in the MX database used by this example. */

	status = mx_setup_database( &record_list, DATABASE_FILENAME );

	if ( status.code != MXE_SUCCESS ) {
		fprintf( stderr, "Cannot setup the MX database '%s'.\n",
				DATABASE_FILENAME );
		exit(1);
	}

	/* Enable the automatic display of scan progress plots. */

	mx_set_plot_enable( record_list, TRUE );

	/* Setup the scan parameters for a copper K edge scan. */

	start = 8950;			/* in eV */
	step_size = 1.0;		/* in eV */
	num_steps = 101;
	measurement_time = 1.0;		/* in seconds */
	settling_time = 0.1;		/* in seconds */

	/* Construct a scan description for the requested scan.  The string
	 * concatenation feature of ANSI C allows us to split the format
	 * string for sprintf() across several lines.
	 */

	sprintf( description,
		"example2scan scan linear_scan motor_scan \"\" \"\" "

		"1 1 1 energy 2 Io It 0 %g preset_time \"%g timer1\" "

		"text example2scan.out gnuplot log($f[0]/$f[1]) %g %g %d",

		settling_time, measurement_time,

		start, step_size, num_steps );

	/* Display the scan description. */

	printf( "Scan description = '%s'\n\n", description );

	/* Add a corresponding scan record to the MX database of the current
	 * process.
	 */

	status = mx_create_record_from_description( record_list, description,
					&scan_record, 0 );

	if ( status.code != MXE_SUCCESS ) {
		fprintf(stderr, "Cannot create the scan record.\n");
		exit(1);
	}

	status = mx_finish_record_initialization( scan_record );

	if ( status.code != MXE_SUCCESS ) {
		fprintf(stderr,
			"Cannot finish initialization of the scan record.\n");
		exit(1);
	}

	/* Perform the scan. */

	status = mx_perform_scan( scan_record );

	if ( status.code != MXE_SUCCESS ) {
		fprintf(stderr, "The scan aborted abnormally.\n" );
		exit(1);
	}

	/* Delete the scan record from the client's database.
	 *
	 * Strictly speaking, this is not really necessary since the example
	 * program is about to exit anyway.  It is shown primarily to give
	 * an example of how scan records are normally deleted in long running
	 * application programs.
	 */

	status = mx_delete_record( scan_record );

	if ( status.code != MXE_SUCCESS ) {
		fprintf(stderr,
			"The attempt to delete the scan record failed.\n");
		exit(1);
	}

	exit(0);
}

