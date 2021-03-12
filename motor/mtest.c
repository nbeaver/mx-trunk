/*
 * Name:    mtest.c
 *
 * Purpose: For MX motor test routines.  Not for normal use.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2009-2010, 2012-2013, 2015-2019, 2021
 *    Illinois Institute of Technology
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
#include "mx_net_interface.h"
#include "mx_callback.h"
#include "mx_signal_alloc.h"
#include "mx_thread.h"
#include "mx_debugger.h"
#include "mx_dynamic_library.h"

int
motor_test_fn( int argc, char *argv[] )
{
	int max_fds, num_open_fds;

	MX_WATCHPOINT *watchpoint = NULL;
	long watch_test;
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
		if ( strcmp( argv[2], "offset" ) == 0 ) {

			MX_DEBUG(-2,("test: stack offset = %ld", 
				mx_get_stack_offset( NULL, &mx_status ) ));

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
		if ( strcmp( argv[2], "watch" ) == 0 ) {
			mx_breakpoint();

			MX_DEBUG(-2,("Debugger is present = %d",
				mx_debugger_is_present() ));

			mx_info( "Beginning watchpoint test." );

			watch_test = 42;

			mx_info( "&watch_test = %p, (watch_test value = %ld)",
				&watch_test, watch_test );

			mx_info( "Setting up watchpoint." );

			mx_set_watchpoint( &watchpoint, NULL,
				&watch_test, MXFT_LONG, W_OK, NULL, NULL );

			mx_show_watchpoints();

			mx_info( "Now waiting to test the watchpoint.");

			mx_msleep(2000);

			mx_info( "Testing watchpoint." );

			watch_test = 39;	/* Should cause SIGTRAP. */

			/* If we get here, shut down the watchpoint. */

			mx_info( "Shutting down watchpoint." );

			mx_clear_watchpoint( watchpoint );

			mx_info( "Watchpoint test complete." );

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
		if ( strcmp( argv[2], "show_file_metadata" ) == 0 ) {
			char *filename = argv[3];

			mx_show_file_metadata( filename );

			return SUCCESS;
		} else
		if ( strcmp( argv[2], "show_fd_metadata" ) == 0 ) {
			int file_descriptor = atoi( argv[3] );

			mx_show_fd_metadata( file_descriptor );

			return SUCCESS;
#if defined(OS_WIN32)
		} else
		if ( strcmp( argv[2], "show_handle_metadata" ) == 0 ) {

#  if defined(_WIN64)
			void *win32_handle = (void *)
			 ((UINT_PTR) mx_hex_string_to_unsigned_long( argv[3] ));
#  else
			void *win32_handle = (void *)
				mx_hex_string_to_unsigned_long( argv[3] );
#  endif

			mx_show_handle_metadata( win32_handle );

			return SUCCESS;
#endif
		} else
		if ( strcmp( argv[2], "address" ) == 0 ) {
#if defined(_WIN64)
			fprintf( output,
			    "Not yet implemented for 64-bit Windows.\n" );

			return FAILURE;
#else
			void *address;
			size_t region_size_in_bytes;

			if ( argc != 5 ) {
				fprintf(stderr,
					"Usage: test address start size\n" );
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
#endif
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
			uint64_t user_total_bytes, user_free_bytes;

			if ( argc < 4 ) {
				fprintf( output,
					"Usage: test disk 'filename'\n" );
				return FAILURE;
			}

			mx_status = mx_get_disk_space( argv[3],
							&user_total_bytes,
							&user_free_bytes );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_info("\nDisk containing '%s':\n"
				"  User total bytes = %" PRIu64 "\n"
				"  User free bytes  = %" PRIu64 "\n",
					argv[3],
					user_total_bytes,
					user_free_bytes );

			return SUCCESS;
		} else
		if ( strcmp( argv[2], "remote_host" ) == 0 ) {
			MX_NETWORK_INTERFACE *ni;
			int ip1, ip2, ip3, ip4;

			if ( argc < 4 ) {
				fprintf( output,
				"Usage: test remote_host 'ip_address'\n");

				return FAILURE;
			}

			mx_status = mx_network_get_interface_from_hostname(
								&ni, argv[3] );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			ip1 = ( ni->ipv4_address ) & 0xff;
			ip2 = ( ( ni->ipv4_address ) >> 8 ) & 0xff;
			ip3 = ( ( ni->ipv4_address ) >> 16 ) & 0xff;
			ip4 = ( ( ni->ipv4_address ) >> 24 ) & 0xff;

			fprintf( output,
			"Computer '%s' uses network interface "
			"'%s' (%d.%d.%d.%d), MTU = %lu\n",
			    argv[3], ni->name, ip1, ip2, ip3, ip4, ni->mtu );

			return SUCCESS;
		}

		else
		if ( strcmp( argv[2], "max_fds" ) == 0 ) {

#if ( defined(OS_WIN32) && !defined(__BORLANDC__) )
			if ( argc >= 4 ) {
				max_fds = atoi( argv[3] );

				_setmaxstdio( max_fds );
			} else {
				max_fds = mx_get_max_file_descriptors();

				mx_info( "max_fds = %d", max_fds );
			}
#else
			max_fds = mx_get_max_file_descriptors();

			mx_info( "max_fds = %d", max_fds );
#endif
			return SUCCESS;
		}

		else
		if ( strcmp( argv[2], "version" ) == 0 ) {
			int os_major, os_minor, os_update;

			mx_status = mx_get_os_version( &os_major,
						&os_minor, &os_update );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_info( "OS: major = %d, minor = %d, update = %d",
				os_major, os_minor, os_update );

#if defined(OS_WIN32)
			{
				unsigned long win32_major, win32_minor;
				unsigned long win32_platform_id;
				unsigned char win32_product_type;

				mx_status = mx_win32_get_osversioninfo(
				    &win32_major, &win32_minor,
				    &win32_platform_id, &win32_product_type );

				if ( mx_status.code != MXE_SUCCESS )
					return FAILURE;

				mx_info( "Win32: major = %lu, minor = %lu, "
				"platform_id = %lu, product_type = %#x",
					win32_major, win32_minor,
					win32_platform_id, win32_product_type );
			}
#endif
			return SUCCESS;
		}

		else
		if ( strcmp( argv[2], "process" ) == 0 ) {
			MX_LIST_HEAD *list_head = NULL;

			mx_info( "Manually processing callbacks." );

			list_head = mx_get_record_list_head_struct(
						motor_record_list );

			if ( list_head == (MX_LIST_HEAD *) NULL ) {
				fprintf( output, "motor_record_list does not "
				"have an MX_LIST_HEAD pointer!!!  "
				"This should never happen!!!\n" );

				abort();
			}

			mx_status = mx_process_callbacks( motor_record_list,
					list_head->callback_pipe );

			if ( mx_status.code == MXE_SUCCESS ) {
				return SUCCESS;
			} else {
				return FAILURE;
			}
		}

		else
		if ( strcmp( argv[2], "signals" ) == 0 ) {
			int i, num_signals;
			int *signal_array;

			num_signals = mx_maximum_signal_number();

			if ( num_signals < 1 ) {
				fprintf( output, "No signals are available.\n");
				return FAILURE;
			}

			signal_array = calloc( num_signals, sizeof(int) );

			if ( signal_array == NULL ) {
				fprintf( output,
			"Allocating a %d element signal status array failed.\n",
					num_signals );
				return FAILURE;
			}

			mx_status = mx_get_signal_allocation( num_signals,
								signal_array );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_free(signal_array);
				return FAILURE;
			}

			for ( i = 0; i < num_signals; i++ ) {
				if ( signal_array[i] ) {
					fprintf( output,
					"Signal %d = TRUE\n", i+1 );
				} else {
					fprintf( output,
					"Signal %d = FALSE\n", i+1 );
				}
			}

			mx_free(signal_array);
			return SUCCESS;
		}

		else
		if ( strcmp( argv[2], "dll_filename" ) == 0 ) {
			MX_DYNAMIC_LIBRARY *library = NULL;
			char dll_filename[MXU_FILENAME_LENGTH+1];

			if ( argc <= 3 ) {
				fprintf( output,
					"DLL filename not specified.\n" );

				return FAILURE;
			} else {
				mx_status = mx_dynamic_library_open(
						argv[3], &library, 0 );

				if ( mx_status.code != MXE_SUCCESS )
					return FAILURE;

				mx_status = mx_dynamic_library_get_filename(
						library, dll_filename,
						sizeof(dll_filename) );

				if ( mx_status.code != MXE_SUCCESS )
					return FAILURE;

				(void) mx_dynamic_library_close( library );

				fprintf( output, "DLL '%s' filename = '%s'\n",
						argv[3], dll_filename );
			}
			return SUCCESS;
		}

		else
		if ( strcmp( argv[2], "show_libraries" ) == 0 ) {

			mx_status = mx_dynamic_library_show_list( output );

			if ( mx_status.code != MXE_SUCCESS ) {
				return FAILURE;
			} else {
				return SUCCESS;
			}
		}
	}

	fprintf( output,
"Usage: test 'test_type and arguments'\n"
"\n"
"Note: Some of these tests can crash mxmotor\n"
"\n"
"test address 'start' 'size'- show information about a memory region\n"
"test disk 'filename' - show disk space info for disk containing the file\n"
"test dll_filename 'filename' - show the file that a dll was opened from\n"
"test max_fds - show the maximum number of file descriptors (Win32)\n"
"test num_open_fds - show the number of open file descriptors\n"
"test offset - test stack offset\n"
"test process - execute all outstanding MX callbacks\n"
"test remote_host 'ip_address'- show network interface for remote host\n"
"test show_fd_metadata 'fd' - show information about a file descriptor\n"
"test show_file_metadata 'filename' - show information about a file\n"
"test show_handle_metadata 'handle' - show information about a Win32 handle\n"
"test show_libraries - show all dynamically loaded libraries\n"
"test show_open_fds - show a list of open file descriptors\n"
"test signals - show how signals are currently allocated\n"
"test stack - test stack traceback\n"
"test version - show the operating system version\n"
"test watch - test watchpoints\n"
"test zero - test zero divide\n"
"\n" );

	return FAILURE;
}

