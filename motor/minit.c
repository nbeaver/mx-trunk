/*
 * Name:    minit.c
 *
 * Purpose: Motor program initialization routine.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2007, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "motor.h"
#include "mx_log.h"
#include "mx_signal.h"
#include "mx_multi.h"

int
motor_init( char *motor_savefile_name,
		int num_scan_savefiles,
		char scan_savefile_array[][MXU_FILENAME_LENGTH+1],
		int init_hw_flags,
		int network_debug )
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
		"%s: Attempt to initialize record types failed.\n", fname);

		exit(1);
	}

	/* Initialize records. */

	motor_record_list = mx_initialize_record_list();

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

#if 0
	if ( network_debug ) {
		list_head_struct->network_debug = TRUE;
	} else {
		list_head_struct->network_debug = FALSE;
	}
#else
	mx_multi_set_debug_flag( motor_record_list, network_debug );
#endif

	/* Read 'motor.dat' and add the records therein to the record list. */

	mx_status = mx_read_database_file( motor_record_list, 
						motor_savefile_name, 0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( output,"%s: Can't load database file '%s'\n",
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

