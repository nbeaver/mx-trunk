/*
 * Name:    mtest.c
 *
 * Purpose: For MX motor test routines.  Not for normal use.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2009-2010, 2012-2013, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "motor.h"
#include "command.h"
#include "mx_inttypes.h"
#include "mx_io.h"
#include "mx_vm_alloc.h"

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

			MXW_UNUSED( k );
			
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
			mx_info( "Open file descriptors:" );
			mx_show_fd_names( mx_process_id() );

#if defined(OS_WIN32)
			mx_info( "Open sockets:" );
			mx_win32_show_socket_names();
#endif
			return SUCCESS;
		} else
		if ( strcmp( argv[2], "address" ) == 0 ) {
			void *address;
			size_t region_size_in_bytes;

			if ( argc != 5 ) {
				fprintf(stderr,
					"Usage: xyzzy address start size\n" );
				return FAILURE;
			}

			address = (void *)
				mx_hex_string_to_unsigned_long( argv[3] );
			region_size_in_bytes
				= mx_hex_string_to_unsigned_long( argv[4] );

			mx_status = mx_vm_show_os_info( stderr,
							address,
							region_size_in_bytes );

			if ( mx_status.code == MXE_SUCCESS ) {
				return SUCCESS;
			} else {
				return FAILURE;
			}
		} else
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
				monitor_filename, (int) file_changed );

			return SUCCESS;
		} else
		if ( strcmp( argv[2], "disk" ) == 0 ) {
			uint64_t total_bytes, total_free_bytes;
			uint64_t user_free_bytes;

			if ( argc < 4 ) {
				fprintf( output,
					"Usage: test disk 'filename'\n" );
				return FAILURE;
			}

			mx_status = mx_get_disk_space( argv[3],
							&total_bytes,
							&total_free_bytes,
							&user_free_bytes );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_info( "Disk containing '%s':\n"
				"  Total bytes      = %" PRIu64 "\n"
				"  Total free bytes = %" PRIu64 "\n"
				"  User free bytes  = %" PRIu64 "\n",
					argv[3], total_bytes,
					total_free_bytes,
					user_free_bytes );

			return SUCCESS;
		}

#if defined(OS_WIN32)
		else
		if ( strcmp( argv[2], "max_fds" ) == 0 ) {
			if ( argc >= 4 ) {
				max_fds = atoi( argv[3] );

				_setmaxstdio( max_fds );
			} else {
				max_fds = mx_get_max_file_descriptors();

				mx_info( "max_fds = %d", max_fds );
			}
			return SUCCESS;
		}
#endif
	}

	mx_info( "Nothing happens." );

	return FAILURE;
}

