/*
 * Name:    mhome.c
 *
 * Purpose: Command to perform a home search on a motor.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2000, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "motor.h"

int
motor_home_fn( int argc, char *argv[] )
{
	static const char cname[] = "home";

	MX_RECORD *record;
	char *endptr;
	int direction, busy, home_search_succeeded, limit_hit;
	double position;
	mx_hex_type motor_status;
	mx_status_type mx_status;

	static char usage[] =
"Usage:  home 'motorname' 'direction' -  Perform a home search on 'motorname'\n"
"\n"
"  Normally, direction > 0 causes a search in the positive direction,\n"
"  while direction < 0 causes a search in the negative direction.  Some\n"
"  drivers that can't do a home search will perform some other operation\n"
"  related to position calibration.  You must examine the individual driver\n"
"  to see exactly what it will do.  In all cases, direction == 0 is performs\n"
"  some driver specific function.\n";

	if ( argc != 4 ) {
		fprintf(output, "%s\n", usage);
		return FAILURE;
	}

	record = mx_get_record( motor_record_list, argv[2] );

	if ( record == (MX_RECORD *) NULL ) {
		fprintf(output,"%s: invalid motor name '%s'\n",cname,argv[2]);
		return FAILURE;
	}

	/* Is this a motor? */

	if ( record->mx_class != MXC_MOTOR ) {
		fprintf(output,"%s: '%s' is not a motor.\n",cname,argv[2]);
		return FAILURE;
	}

	direction = (int) strtol( argv[3], &endptr, 10 );

	if ( *endptr != '\0' ) {
		fprintf(output,
			"%s: specified direction '%s' is not a number.\n",
			cname, argv[3] );
		return FAILURE;
	}

	/* Start the home search. */

	mx_status = mx_motor_find_home_position( record, direction );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* Wait for the home search to complete. */

	fprintf(output,"*** Home search in progress ***\n");

	for (;;) {
		mx_status = mx_motor_get_extended_status( record,
						&position, &motor_status );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		fprintf(output, "  %g\n", position);

		if ( mx_user_requested_interrupt() ) {
			fprintf(output, "*** Home search interrupted ***\n");

			(void) mx_motor_soft_abort( record );

			break;		/* Exit the for(;;) loop. */
		}

		busy = (int) (motor_status & MXSF_MTR_IS_BUSY);

		if ( busy == FALSE ) {

			/* The motor has stopped, so exit the for(;;) loop. */

			break;
		}

		mx_msleep(1000);
	}

	mx_status = mx_motor_home_search_succeeded( record,
						&home_search_succeeded );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	if ( home_search_succeeded ) {
		fprintf(output,
		"*** Home search for motor '%s' completed successfully. ***\n",
			record->name );

		return SUCCESS;
	} else {
		/* Check to see if the limit switches are tripped. */

		(void) mx_motor_positive_limit_hit( record, &limit_hit );

		if ( limit_hit ) {
			fprintf(output,
			"*** Positive limit hit for motor '%s'. ***\n",
				record->name);
		}

		(void) mx_motor_negative_limit_hit( record, &limit_hit );

		if ( limit_hit ) {
			fprintf(output,
			"*** Negative limit hit for motor '%s'. ***\n",
				record->name);
		}

		fprintf(output,
"\007*** Home search failed.  Motor '%s' is not at the home switch. ***\n",
			record->name );

		return FAILURE;
	}
}

