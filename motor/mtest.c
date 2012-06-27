/*
 * Name:    mtest.c
 *
 * Purpose: For MX motor test routines.  Not for normal use.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2009-2010, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "motor.h"
#include "command.h"
#include "mx_io.h"

int
motor_test_fn( int argc, char *argv[] )
{
	int max_fds, num_open_fds;

	static MX_FILE_MONITOR *monitor = NULL;
	static char monitor_filename[MXU_FILENAME_LENGTH+1];
	unsigned long monitor_access_type = 0;
	mx_bool_type file_changed;
	mx_status_type mx_status;

	if ( argc >= 3 ) {
		if ( strcmp( argv[2], "stack" ) == 0 ) {
			mx_stack_traceback();
			return SUCCESS;
		} else
		if ( strcmp( argv[2], "zero" ) == 0 ) {
			int i = 42;
			int j = 0;
			int k;

			k = i / j;
			
			return SUCCESS;
		} else
		if ( strcmp( argv[2], "num_open_fds" ) == 0 ) {

			max_fds = mx_get_max_file_descriptors();

			num_open_fds = mx_get_number_of_open_file_descriptors();

			mx_info(
			"num_open_fds: max_fds = %d, num_open_fds = %d",
				max_fds, num_open_fds );

			return SUCCESS;
		} else
		if ( strcmp( argv[2], "show_open_fds" ) == 0 ) {
			mx_show_fd_names( mx_process_id() );

			return SUCCESS;
		}
		if ( strcmp( argv[2], "monitor" ) == 0 ) {
			if ( argc > 3 ) {
				if ( monitor != NULL ) {
					mx_status = mx_delete_file_monitor(
							monitor );

					if ( mx_status.code != MXE_SUCCESS )
						return FAILURE;
				}

				strlcpy( monitor_filename, argv[3],
					sizeof(monitor_filename) );

				mx_status = mx_create_file_monitor(
							&monitor,
							monitor_access_type,
							monitor_filename );

				if ( mx_status.code != MXE_SUCCESS )
					return FAILURE;
			}

			if ( monitor == NULL ) {
				mx_info(
				"A file monitor has not yet been set up.\n" );

				return FAILURE;
			}

			file_changed = mx_file_has_changed( monitor );

			mx_info( "monitor '%s', file_changed = %d",
				monitor_filename, file_changed );

			return SUCCESS;
		}

#if defined(OS_WIN32)
		else
		if ( strcmp( argv[2], "max_fds" ) == 0 ) {
			if ( argc >= 4 ) {
				max_fds = atoi( argv[3] );

				_setmaxstdio( max_fds );
			} else {
				max_fds = _getmaxstdio();

				mx_info( "max_fds = %d", max_fds );
			}
			return SUCCESS;
		}
#endif
	}

	mx_info( "Nothing happens." );

	return FAILURE;
}

