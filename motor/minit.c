/*
 * Name:    minit.c
 *
 * Purpose: Motor program initialization routine.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2007, 2009-2010, 2012-2014, 2016-2017, 2021-2022
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "motor.h"
#include "mx_log.h"
#include "mx_multi.h"
#include "mx_unistd.h"

/*--------------------------------------------------------------------------*/

/* FIXME: The following simple signal handler may not be enough
 * to handle SIGINT correctly for some build targets.  However,
 * for now we assume that it works and hope for the best.
 */

static void
motor_sigint_handler( int signum )
{
	mx_set_user_requested_interrupt( MXF_USER_INT_ABORT );
}

static void
motor_install_sigint_handler( void )
{
	signal( SIGINT, motor_sigint_handler );
}

/*--------------------------------------------------------------------------*/

int
motor_init( char *motor_savefile_name,
		int num_scan_savefiles,
		char scan_savefile_array[][MXU_FILENAME_LENGTH+1],
		mx_bool_type init_hw_flags,
		mx_bool_type verify_drivers,
		unsigned long network_debug_flags,
		unsigned long max_network_dump_bytes )
{
	static const char fname[] = "motor_init()";

	MX_LIST_HEAD *list_head_struct;
	int i, status;
	unsigned long flags;
	mx_status_type mx_status;

	/* Intercept some common signals that may result during a crash
	 * so that we can attempt to display a stack traceback of where
	 * we were when we crashed.
	 */

	if ( motor_install_signal_handlers ) {

#if ! defined( OS_WIN32 )

		/* For Win32 it is better to have the operating system fire up
		 * the debugger.
		 */

		mx_setup_standard_signal_error_handlers();

#endif /* ! defined( OS_WIN32 ) */

		/* Trap SIGINT interrupts (like ctrl-C). */

		motor_install_sigint_handler();
	}

	/* Set some global flags. */

	motor_header_prompt_on = TRUE;
	motor_overwrite_on = FALSE;
	motor_bypass_limit_switch = FALSE;
	motor_parameter_warning_flag = 0;

	/* The default value of 'motor_autosave_on' depends on the value
	 * of 'allow_scan_database_updates'.
	 */

	if ( allow_scan_database_updates ) {
		motor_autosave_on = TRUE;
	} else {
		motor_autosave_on = FALSE;
	}

	motor_estimate_on = TRUE;

	/* Initialize MX device drivers. */

	mx_status = mx_initialize_drivers();

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( output,
		"%s: Attempt to initialize record types failed.\n", fname);

		exit(1);
	}

	/* Create a new MX database. */

	motor_record_list = mx_initialize_database();

	if ( motor_record_list == (MX_RECORD *) NULL ) {
		fprintf( output,
	"%s: Out of memory allocating first record of the record list.\n",
			fname );
	}

	/* Set the default display precision for floating point numbers. */

	list_head_struct = mx_get_record_list_head_struct( motor_record_list );

	if ( list_head_struct == NULL ) {
		fprintf( output, "%s: list_head_struct is NULL.\n", fname );
		exit(1);
	}

	list_head_struct->default_precision = motor_default_precision;

	strlcpy( list_head_struct->program_name,
			cmd_program_name(), MXU_PROGRAM_NAME_LENGTH );

	list_head_struct->network_debug_flags |= network_debug_flags;

	mx_multi_set_debug_flags( motor_record_list,
				list_head_struct->network_debug_flags );

	list_head_struct->max_network_dump_bytes = max_network_dump_bytes;

	/* Read 'mxmotor.dat' and add the records therein to the record list. */

	mx_status = mx_read_database_file( motor_record_list, 
						motor_savefile_name, 0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( output,"%s: Cannot load database file '%s'\n",
			fname, motor_savefile_name );
		exit(1);
	}

	/* Do the same with the scan records in the scan databases.
	 * Ignore any databases that don't exist.
	 */

	for ( i = 0; i < num_scan_savefiles; i++ ) {

		status = access( scan_savefile_array[i], F_OK );

		if ( status != 0 ) {
			fprintf( output,
			"Scan database '%s' does not exist.  Skipping.\n",
				scan_savefile_array[i] );

			if ( i == (num_scan_savefiles - 1) ) {
				motor_has_unsaved_scans = TRUE;
			}
		} else {
			flags = MXFC_ALLOW_SCAN_REPLACEMENT
					| MXFC_DELETE_BROKEN_RECORDS;

			mx_status = mx_read_database_file( motor_record_list,
						scan_savefile_array[i], flags );

			switch( mx_status.code ) {
			case MXE_SUCCESS:
				break;
			case MXE_END_OF_DATA:
				fprintf( output,
				"Scan database '%s' is empty.\n",
					scan_savefile_array[i] );
				break;
			default:
				fprintf( output,
				"%s: Cannot load scan database '%s'\n",
					fname, scan_savefile_array[i] );
				exit(1);
			}
		}
	}

	/* If we were asked to verify loaded drivers, then this is
	 * the correct time to do that since all !load statements in
	 * MX databases have now been executed.
	 */

	if ( verify_drivers ) {
		mx_status = mx_verify_driver_tables();

		if ( mx_status.code != MXE_SUCCESS ) {
			exit(mx_status.code);
		}
	}

	/* Do initialization steps that have to wait until all the
	 * records have been defined.
	 */

	mx_status = mx_finish_database_initialization( motor_record_list );

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( output,
		"%s: Cannot complete database initialization.\n", fname );

		return FAILURE;
	}

	fprintf( output, "\n");

	/* If a 1-D string variable record name "mx_log" exists,
	 * start up message logging.
	 */

	mx_status = mx_log_open( motor_record_list );

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( output,
		"%s: Cannot start MX logging.\n", fname );
		return FAILURE;
	}

	/* Setup motor's handler for scan pause requests. */

	mx_set_scan_pause_request_handler( motor_scan_pause_request_handler );

	/* Now initialize all the hardware described by the record list. */

	mx_status = mx_initialize_hardware( motor_record_list, init_hw_flags );

	if ( mx_status.code == MXE_SUCCESS ) {
		return SUCCESS;
	} else {
		fprintf( output,
		"%s: Cannot perform hardware initialization.\n", fname );
		return FAILURE;
	}
}

