/*
 * Name:    minit.c
 *
 * Purpose: Motor program initialization routine.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "mx_unistd.h"

#include "motor.h"
#include "mx_log.h"
#include "mx_signal.h"

int
motor_init( char *motor_savefile_name,
		int num_scan_savefiles,
		char scan_savefile_array[][MXU_FILENAME_LENGTH+1],
		int init_hw_flags )
{
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

#ifdef SIGILL
		signal( SIGILL, mx_standard_signal_error_handler );
#endif
#ifdef SIGTRAP
		signal( SIGTRAP, mx_standard_signal_error_handler );
#endif
#ifdef SIGIOT
		signal( SIGIOT, mx_standard_signal_error_handler );
#endif
#ifdef SIGBUS
		signal( SIGBUS, mx_standard_signal_error_handler );
#endif
#ifdef SIGFPE
		signal( SIGFPE, mx_standard_signal_error_handler );
#endif
#ifdef SIGSEGV
		signal( SIGSEGV, mx_standard_signal_error_handler );
#endif

#endif /* ! defined( OS_WIN32 ) */

		/* Trap ctrl-C interrupts. */

		signal( SIGINT, SIG_IGN );

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

	/* Initialize MX device drivers. */

	mx_status = mx_initialize_drivers();

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( output,
		"motor_init: Attempt to initialize record types failed.\n");

		exit(1);
	}

	/* Initialize records. */

	motor_record_list = mx_initialize_record_list();

	if ( motor_record_list == (MX_RECORD *) NULL ) {
		fprintf( output,
"motor_init: Out of memory allocating first record of the record list.\n" );
	}

	/* Set the default display precision for floating point numbers. */

	list_head_struct = mx_get_record_list_head_struct( motor_record_list );

	if ( list_head_struct == NULL ) {
		fprintf( output, "motor_init: list_head_struct is NULL.\n" );
		exit(1);
	}

	list_head_struct->default_precision = motor_default_precision;

	strlcpy( list_head_struct->program_name,
			cmd_program_name(), MXU_PROGRAM_NAME_LENGTH );

	/* Read 'motor.dat' and add the records therein to the record list. */

	mx_status = mx_read_database_file( motor_record_list, 
						motor_savefile_name, 0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( output,"motor_init: Can't load database file '%s'\n",
			motor_savefile_name );
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

			if ( mx_status.code != MXE_SUCCESS ) {
				fprintf( output,
				"motor_init: Can't load database file '%s'\n",
					scan_savefile_array[i] );
				exit(1);
			}
		}
	}

	/* Do initialization steps that have to wait until all the
	 * records have been defined.
	 */

	mx_status = mx_finish_database_initialization( motor_record_list );

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( output,
		"motor_init: Cannot complete database initialization.\n" );

		return FAILURE;
	}

	fprintf( output, "\n");

	/* If a 1-D string variable record name "mx_log" exists,
	 * start up message logging.
	 */

	mx_status = mx_log_open( motor_record_list );

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( output,
		"motor_init: Cannot start MX logging.\n" );
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
		"motor_init: Cannot perform hardware initialization.\n" );
		return FAILURE;
	}
}

