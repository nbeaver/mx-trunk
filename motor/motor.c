/*
 * Name:    motor.c
 *
 * Purpose: Main routine for simple motor scan program.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXMOTOR_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#if defined(OS_WIN32)
#include <windows.h>
#include <direct.h>
#endif

#include "motor.h"
#include "mdialog.h"
#include "mx_log.h"
#include "mx_version.h"
#include "mx_key.h"
#include "mx_cfn.h"
#include "mx_net.h"
#include "mx_debugger.h"

#if defined(DEBUG_MPATROL)
#  include <mpatrol/heapdiff.h>

   static heapdiff mainloop_heapdiff;
#endif

/* Global variables. */

MX_RECORD *motor_record_list;

char motor_savefile[MXU_FILENAME_LENGTH + 1];
char scan_savefile[MXU_FILENAME_LENGTH + 1];
char global_motorrc[MXU_FILENAME_LENGTH + 1];
char user_motorrc[MXU_FILENAME_LENGTH + 1];

COMMAND command_list[] = {
	{ motor_stop_fn,    1, "abort"         },     /* alias for 'stop' */
	{ motor_area_detector_fn,
			    2, "area_detector" },
	{ motor_area_detector_fn,
			    2, "ad"            }, /* alias for 'area_detector'*/
	{ motor_debug_fn,   2, "break"         },
	{ motor_cd_fn,      2, "cd"            },
	{ motor_sample_changer_fn,
		            2, "changer"       },
	{ motor_close_fn,   2, "close"         },
	{ motor_copy_or_rename_fn, 3, "copy"   },
	{ motor_count_fn,   3, "count"         },
	{ motor_debug_fn,   5, "debug"         },
	{ motor_delete_fn,  1, "delete"        },
	{ motor_exec_fn,    3, "exec"          },
	{ motor_execq_fn,   5, "execq"         },
	{ motor_exit_fn,    3, "exit"          },
	{ motor_gpib_fn,    1, "gpib"          },
	{ motor_help_fn,    1, "help"          },
	{ motor_home_fn,    2, "home"          },
	{ motor_close_fn,   1, "insert"        },     /* alias for 'close' */
	{ motor_kill_fn,    1, "kill"          },
	{ motor_load_fn,    1, "load"          },
	{ motor_mabs_fn,    2, "mabs"          },
	{ motor_mca_fn,     3, "mca"           },
	{ motor_mcs_fn,     3, "mcs"           },
	{ motor_measure_fn, 2, "measure"       },
	{ motor_mjog_fn,    2, "mjog"          },
	{ motor_modify_fn,  3, "modify"        },
	{ motor_mabs_fn,    3, "move"          },     /* alias for 'mabs' */
	{ motor_mrel_fn,    2, "mrel"          },
	{ motor_open_fn,    1, "open"          },
	{ motor_ptz_fn,     3, "ptz"           },
	{ motor_exit_fn,    1, "quit"          },     /* alias for 'exit' */
	{ motor_readp_fn,  10, "readparams"    },
	{ motor_open_fn,    3, "remove"        },     /* alias for 'open' */
	{ motor_copy_or_rename_fn, 3, "rename" },
	{ motor_resync_fn,  3, "resynchronize" },
	{ motor_rs232_fn,   2, "rs232",        },
	{ motor_exec_fn,    2, "run"           },     /* alias for 'exec' */
	{ motor_save_fn,    2, "save"          },
	{ motor_sample_changer_fn,
		            3, "sample_changer"},
	{ motor_sca_fn,     3, "sca"           },
	{ motor_scan_fn,    4, "scan"          },
	{ motor_script_fn,  3, "script"        },
	{ motor_set_fn,     3, "set"           },
	{ motor_setup_fn,   4, "setup"         },
	{ motor_showall_fn, 5, "showall"       },
	{ motor_show_fn,    2, "show"          },
	{ motor_start_fn,   3, "start"         },
	{ motor_stop_fn,    3, "stop"          },
	{ motor_system_fn,  2, "system"        },
	{ motor_test_fn,    2, "test"          },
	{ motor_vinput_fn,  3, "vinput"        },
	{ motor_vinput_fn,  3, "video_input"   },
	{ motor_writep_fn, 11, "writeparams"   },
	{ motor_wvout_fn,   5, "wvout"         },
	{ motor_exec_fn,    1, "@"             },     /* alias for 'exec' */
	{ motor_system_fn,  1, "!"             },     /* alias for 'system' */
	{ motor_system_fn,  1, "$"             },     /* alias for 'system' */
	{ motor_script_fn,  1, "&"             },     /* alias for 'script' */
};
int command_list_length;

FILE *input, *output;

int allow_motor_database_updates;
int allow_scan_database_updates;
int motor_has_unsaved_scans;
int motor_exit_save_policy;
int motor_install_signal_handlers;

int motor_header_prompt_on;
int motor_overwrite_on;
int motor_autosave_on;
int motor_estimate_on;
int motor_bypass_limit_switch;
int motor_parameter_warning_flag;

int motor_default_precision;

/* End of global variables. */

/*------------------------------------------------------------------------*/

static void
timestamp_output( char *string )
{
	char timestamp_buffer[100];

	mx_timestamp( timestamp_buffer, sizeof(timestamp_buffer) );

#if ( defined(OS_WIN32) || defined(OS_VXWORKS) )
	fprintf( stdout, "%s: %s\n", timestamp_buffer, string );
	fflush( stdout );
#else
	fprintf( stderr, "%s: %s\n", timestamp_buffer, string );
#endif
}

static void
timestamp_warning_output( char *string )
{
	char timestamp_buffer[2*MXU_FILENAME_LENGTH + 100];

	mx_timestamp( timestamp_buffer, sizeof(timestamp_buffer) );

#if ( defined(OS_WIN32) || defined(OS_VXWORKS) )
	fprintf( stdout, "%s: Warning: %s\n", timestamp_buffer, string );
	fflush( stdout );
#else
	fprintf( stderr, "%s: Warning: %s\n", timestamp_buffer, string );
#endif
}

/*------------------------------------------------------------------------*/

static void
wait_at_exit_fn( void )
{
	fprintf( stderr, "Press any key to exit...\n" );
	mx_getch();
}

/*------------------------------------------------------------------------*/

#define MAX_SCAN_SAVEFILES	5

#define EXIT_WITH_PROMPT		0
#define EXIT_NO_PROMPT_ALWAYS_SAVE	1
#define EXIT_NO_PROMPT_NEVER_SAVE	2

#if HAVE_MAIN_ROUTINE

int
main( int argc, char *argv[] )

#else /* not HAVE_MAIN_ROUTINE */

int
motor_main( int argc, char *argv[] )

#endif /* HAVE_MAIN_ROUTINE */
{
	COMMAND *command = NULL; /* Used by mx_initialize_stack_calc() below. */
	int cmd_argc;
	char **cmd_argv = NULL;
	char *command_line = NULL;
	char *split_buffer = NULL;
	char *name;
	int status, debug_level, unbuffered_io;
	mx_bool_type init_hw_flags, start_debugger;
	mx_bool_type run_startup_scripts, ignore_scan_savefiles;
	unsigned long network_debug_flags;
	long max_network_dump_bytes;
	mx_status_type mx_status;
	char prompt[80];
	char saved_command_name[80];
	mx_bool_type wait_for_debugger, just_in_time_debugging;
	mx_bool_type wait_at_exit;
	mx_bool_type verify_drivers;

	static char
	    scan_savefile_array[ MAX_SCAN_SAVEFILES ][ MXU_FILENAME_LENGTH+1 ];
	int num_scan_savefiles;

#if HAVE_GETOPT
	int c, error_flag;
#endif
	mx_initialize_stack_calc( &command );

	/* Initialize the MX runtime environment. */

	mx_status = mx_initialize_runtime();

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( stderr,
		"%s: Unable to initialize the MX runtime environment.\n",
			argv[0] );

		exit(1);
	}

#if 0
	{
		double mhz = mx_cpu_speed();

		fprintf(stderr, "mx_cpu_speed(): %f MHz\n", mhz);
	}
#endif

	cmd_set_program_name( "mxmotor" );

	command_list_length = sizeof(command_list) / sizeof(COMMAND);

	debug_level = 0;
	unbuffered_io = FALSE;
	init_hw_flags = FALSE;

	mx_cfn_construct_config_filename( "mxmotor.dat",
				motor_savefile, MXU_FILENAME_LENGTH );

	mx_cfn_construct_scan_filename( "mxscan.dat",
				scan_savefile, MXU_FILENAME_LENGTH );

	mx_cfn_construct_config_filename( "startup/mxmotor.startup",
				global_motorrc, MXU_FILENAME_LENGTH );

	mx_cfn_construct_user_filename( "mxmotor.startup",
				user_motorrc, MXU_FILENAME_LENGTH );

	allow_motor_database_updates = FALSE;
	allow_scan_database_updates = TRUE;
	motor_has_unsaved_scans = FALSE;
	motor_exit_save_policy = EXIT_WITH_PROMPT;
	run_startup_scripts = TRUE;
	motor_install_signal_handlers = TRUE;

	motor_default_precision = 8;

	num_scan_savefiles = 0;
	ignore_scan_savefiles = FALSE;

	start_debugger = FALSE;
	wait_for_debugger = FALSE;
	wait_at_exit = FALSE;
	just_in_time_debugging = FALSE;
	verify_drivers = FALSE;

	network_debug_flags = 0;
	max_network_dump_bytes = -1L;

#if HAVE_GETOPT
	/* Process command line arguments, if any. */

	error_flag = FALSE;

	while ( ( c = getopt(argc, argv,
		"aA:d:DF:f:Hg:iJNnP:p:q:S:s:tT:uVwWY:xz") ) != -1)
	{
		switch (c) {
		case 'a':
			network_debug_flags |= MXF_NETDBG_SUMMARY;
			break;
		case 'A':
			network_debug_flags =
				mx_hex_string_to_unsigned_long( optarg );
			break;
		case 'd':
			debug_level = atoi( optarg );
			break;
		case 'D':
			start_debugger = TRUE;
			break;
		case 'F':
			allow_motor_database_updates = FALSE;

			strlcpy( motor_savefile, optarg, MXU_FILENAME_LENGTH );
			break;
		case 'f':
			allow_motor_database_updates = TRUE;

			strlcpy( motor_savefile, optarg, MXU_FILENAME_LENGTH );
			break;
		case 'g':
			strlcpy( global_motorrc, optarg, MXU_FILENAME_LENGTH );
			break;
		case 'H':
			motor_install_signal_handlers = FALSE;
			break;
		case 'i':
			run_startup_scripts = FALSE;
			break;
		case 'J':
			just_in_time_debugging = TRUE;
			break;
		case 'n':
			motor_exit_save_policy = EXIT_NO_PROMPT_ALWAYS_SAVE;
			break;
		case 'N':
			motor_exit_save_policy = EXIT_NO_PROMPT_NEVER_SAVE;
			break;
		case 'p':
			cmd_set_program_name( optarg );
			break;
		case 'P':
			motor_default_precision = atoi( optarg );
			break;
		case 'q':
			max_network_dump_bytes = atoi( optarg );
			break;
		case 'S':
			allow_scan_database_updates = FALSE;

			strlcpy( scan_savefile_array[num_scan_savefiles],
					optarg, MXU_FILENAME_LENGTH );
			num_scan_savefiles++;
			break;
		case 's':
			allow_scan_database_updates = TRUE;

			strlcpy( scan_savefile_array[num_scan_savefiles],
					optarg, MXU_FILENAME_LENGTH );
			num_scan_savefiles++;
			break;
		case 't':
			init_hw_flags |= MXF_INITHW_TRACE_OPENS;
			break;
		case 'T':
			if ( strcmp( optarg, "error" ) == 0 ) {
			    mx_set_error_output_function( timestamp_output );
			} else
			if ( strcmp( optarg, "debug" ) == 0 ) {
			    mx_set_debug_output_function( timestamp_output );
			} else
			if ( strcmp( optarg, "warning" ) == 0 ) {
				mx_set_warning_output_function(
					timestamp_warning_output );
			} else
			if ( strcmp( optarg, "info" ) == 0 ) {
			    mx_set_info_output_function( timestamp_output );
			}
			break;
		case 'u':
			unbuffered_io = TRUE;
			break;
		case 'V':
			verify_drivers = TRUE;
			break;
		case 'w':
			wait_for_debugger = TRUE;
			break;
		case 'W':
			wait_at_exit = TRUE;
			break;
		case 'x':
			putenv("MX_DEBUGGER=xterm -e gdb -tui -p %lu");
			break;
		case 'Y':
			/* Directly set the value of MXDIR. */

			mx_setenv( "MXDIR", optarg );
			break;
		case 'z':
			ignore_scan_savefiles = TRUE;
			break;
		case '?':
			error_flag = TRUE;
			break;
		}

		if ( error_flag == TRUE ) {
			fprintf( stderr,
"Usage: motor [-d debug_level] [-f motor_savefile] [-F motor_savefile]\n"
"          [-g global_motorrc] [-s scan_savefile] [-S scan_savefile] [-u]\n");
			exit(1);
		}
	}

	if ( start_debugger ) {
		mx_prepare_for_debugging( NULL, just_in_time_debugging );

		mx_start_debugger( NULL );
	} else
	if ( just_in_time_debugging ) {
		mx_prepare_for_debugging( NULL, just_in_time_debugging );
	} else {
		mx_prepare_for_debugging( NULL, FALSE );
	}

	if ( wait_for_debugger ) {
		mx_wait_for_debugger();
	}

	if ( wait_at_exit ) {
		fprintf( stderr,
			"This program will wait at exit to be closed.\n" );
		fflush( stderr );

		atexit( wait_at_exit_fn );
	}

#endif /* HAVE_GETOPT */

	if ( ignore_scan_savefiles ) {
		num_scan_savefiles = 0;
	} else {
		if ( num_scan_savefiles == 0 ) {
			num_scan_savefiles = 1;

			strlcpy( scan_savefile_array[0],
				scan_savefile, MXU_FILENAME_LENGTH );
		} else {
			strlcpy( scan_savefile,
				scan_savefile_array[ num_scan_savefiles - 1 ],
				MXU_FILENAME_LENGTH );
		}
	}

	if ( unbuffered_io ) {
		setbuf( stdin, NULL );
		setbuf( stdout, NULL );
	}

	input = stdin;
#if ( defined(OS_WIN32) || defined(OS_VXWORKS) )
	output = stderr;
#else
	output = stdout;
#endif

	mx_set_debug_level( debug_level );

	fprintf(output, "\nMX version %s\n", mx_get_version_full_string() );

	/* Here is some test code to check the operation of the
	 * mx_pointer_is_valid() function.  This code should 
	 * normally be commented out.
	 */

#if 0
	{
		int test;
#if 1
		test = mx_pointer_is_valid(
			(void *) 27, sizeof(int), R_OK);
#else
		test = mx_pointer_is_valid(
			(void *)&debug_level, sizeof(int), R_OK);
#endif

		fprintf(output, "mx_pointer_is_valid() = %d\n", test);
	}
#endif
	/*---*/

	/* Filenames specified on the command line may have been relative
	 * pathnames rather than absolute pathnames.  Just in case this
	 * has happened, we send the filenames through the filename
	 * construction functions again.
	 */

#if 0
	/* Convert the save file names to absolute pathnames so that
	 * things like the 'cd' command do not cause us to write our
	 * save files in the wrong place.
	 */

	status = motor_expand_pathname( motor_savefile, MXU_FILENAME_LENGTH );

	if ( status == FAILURE ) {
		fprintf(output,
		"Could not determine full pathname of motor savefile '%s'\n",
			motor_savefile);
		exit(1);
	}

	status = motor_expand_pathname( scan_savefile, MXU_FILENAME_LENGTH );

	if ( status == FAILURE ) {
		fprintf(output,
		"Could not determine full pathname of scan savefile '%s'\n",
			scan_savefile);
		exit(1);
	}
#endif

#if MXMOTOR_DEBUG
	fprintf( stderr, "motor savefile = '%s'\n", motor_savefile );
	fprintf( stderr, "scan savefile  = '%s'\n", scan_savefile );
	fprintf( stderr, "global motorrc = '%s'\n", global_motorrc );
#endif

	/* Initialize variables from the save files. */

	status = motor_init( motor_savefile,
			num_scan_savefiles, scan_savefile_array,
			init_hw_flags, verify_drivers,
			network_debug_flags,
			max_network_dump_bytes );

	if ( status == FAILURE ) {
		fprintf(output,"motor: Initialization failed.  Exiting...\n");
		exit(1);
	}

#if defined(OS_RTEMS) || defined(OS_VXWORKS)

	/* Force off the saving of scan databases for real time kernels. */

	allow_scan_database_updates = FALSE;
	motor_autosave_on = FALSE;
#endif

	/* Make backup copies of the save files. */

	if ( allow_motor_database_updates ) {
		mx_status = motor_make_backup_copy( motor_savefile );

		if ( mx_status.code != MXE_SUCCESS ) {
			fprintf( stderr, "Unable to make backup copy of '%s'\n",
			motor_savefile );
			exit(1);
		}
	}
	if ( allow_scan_database_updates ) {
		mx_status = motor_make_backup_copy( scan_savefile );

		if ( mx_status.code != MXE_SUCCESS ) {
			fprintf( stderr, "Unable to make backup copy of '%s'\n",
			scan_savefile );
			/* This is not regarded as a fatal error. */
		}
	}

	/* Plotting enabled is the default. */

	mx_set_plot_enable( motor_record_list, 1 );

	/* If not disabled, try to run the startup scripts. */

	if ( run_startup_scripts == TRUE ) {

		/* Run the global startup file if any. */

		status = access( global_motorrc, R_OK );

		if ( status == 0 ) {
			status = motor_exec_script( global_motorrc, FALSE );

			if ( status != SUCCESS ) {
				fprintf( stderr,
	 	"Attempt to execute global startup file '%s' failed.\n",
					global_motorrc );
			}
		}

		/* Run the per user startup script if any. */

		if ( strlen(user_motorrc) > 0 ) {
			status = access( user_motorrc, R_OK );

			if ( status == 0 ) {
				status = motor_exec_script(
						user_motorrc, FALSE );

				if ( status != SUCCESS ) {
					fprintf( stderr,
			"Attempt to execute user startup file '%s' failed.\n",
						user_motorrc );
				}
			}
		}
	}

	/* If there were command line arguments, use the first one
	 * as the name of a script to execute.  If you want 'motor'
	 * to exit after the script terminates, put the 'exit'
	 * command as the last line of your script.
	 */

#if HAVE_GETOPT
	if ( argc > optind ) {
		status = motor_exec_script( argv[optind], TRUE );

		if ( status != SUCCESS ) {
			fprintf( stderr,
	"Attempt to execute command line startup script '%s' failed.\n",
				argv[optind] );
		}
	}
#else
	if ( argc > 1 ) {
		status = motor_exec_script( argv[1], TRUE );

		if ( status != SUCCESS ) {
			fprintf( stderr,
	"Attempt to execute command line startup script '%s' failed.\n",
				argv[1] );
		}
	}
#endif

	/* Now go into interactive mode. */

	name = cmd_program_name();

	snprintf( prompt, sizeof(prompt), "%s> ", name );

#if defined(DEBUG_MPATROL)
	heapdiffstart( mainloop_heapdiff, HD_UNFREED | HD_FULL );
#endif

	for (;;) {
		/* Clear any saved user interrupt flag. */

		mx_set_user_requested_interrupt( MXF_USER_INT_NONE );

		/* Read a command line. */

		command_line = cmd_read_next_command_line( prompt );

#if 0
		if ( command_line == NULL ) {
			fprintf(stderr,"command_line = NULL\n");
		} else {
			fprintf(stderr,"command_line = '%s'\n", command_line);
		}
#endif

		if ( command_line == NULL ) {
			cmd_argc = 0;
			cmd_argv = NULL;
			split_buffer = NULL;
		} else {
			status = cmd_split_command_line( command_line,
					&cmd_argc, &cmd_argv, &split_buffer );

			if ( status == FAILURE ) {
				cmd_free_command_line( cmd_argv, split_buffer );
				return FAILURE;
			}
		}

		if ( cmd_argc < 1 ) {

			fprintf(output, "motor: Invalid command line.\n");

		} else if ( cmd_argc == 1 ) {

			/* Do nothing. */

		} else {
			command = cmd_get_command_from_list(
			    command_list_length, command_list, cmd_argv[1] );

			if ( command == (COMMAND *) NULL ) {
				fprintf(output, 
				"motor: Unrecognized command '%s'.\n",
					cmd_argv[1] );
			} else {
				/* The contents of cmd_argv[1] may be
				 * changed by the function we are
				 * invoking, so save a copy of the
				 * command name first.
				 */

				strlcpy( saved_command_name, cmd_argv[1],
					sizeof(saved_command_name) );

				/* Invoke the function. */

				status 
			= (*(command->function_ptr))( cmd_argc, cmd_argv );

				switch( status ) {
				case SUCCESS:
					/* Do nothing. */
					break;
				case FAILURE:
#if 0
					fprintf(output,
					"Error: '%s' command failed.\n",
						saved_command_name);
#endif
					break;
				case INTERRUPTED:
					fprintf(output,
				"Warning: '%s' command was interrupted.\n",
						saved_command_name);
					break;

				default:
					fprintf(output,
		"Error: '%s' command failed with unknown error code = %d.\n"
		">>> This is a bug.\n", saved_command_name, status);
					break;
				}
			}
		}

		cmd_free_command_line( cmd_argv, split_buffer );
	}

	/* Some compilers emit a warning if main() doesn't return a value. */

	MXW_NOT_REACHED( exit(0); return 0; )
}

int
motor_debug_fn( int argc, char *argv[] )
{
	char command[200];
	int i;

	if ( argc <= 2 ) {
		mx_breakpoint();
	} else {
		command[0] = '\0';

		for ( i = 2; i < argc; i++ ) {
			strlcat( command, argv[i], sizeof(command) );
			strlcat( command, " ", sizeof(command) );
		}

		mx_start_debugger( command );
	}

	return SUCCESS;
}

int
motor_cd_fn( int argc, char *argv[] )
{
	static char usage[] = "Usage:  cd 'directory_name'\n";

	int status, saved_errno;

	if ( argc != 3 ) {
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}

	status = chdir( argv[2] );

	if ( status != 0 ) {
		saved_errno = errno;

		fprintf( output,
		"cd: Can't change directory to '%s'.  Error = '%s'\n",
			argv[2], strerror( saved_errno ) );

		return FAILURE;
	}
	return SUCCESS;
}

int
motor_exit_fn( int argc, char *argv[] )
{
	char buffer[2*MXU_FILENAME_LENGTH + 20];
	int status;
	mx_status_type mx_status;

	/* Automatically save the motor database if we are supposed to. */

	if ( allow_motor_database_updates ) {
		snprintf( buffer, sizeof(buffer),
			"save motor %s", motor_savefile );

		status = cmd_execute_command_line( command_list_length,
						command_list, buffer );

		if ( status == FAILURE ) {
			fprintf( output,
				"motor_exit: Failed to save motor file '%s'\n",
				motor_savefile );
		}
	}

	/* Automatically save the scan database if we are supposed to. */

	if ( motor_autosave_on && motor_has_unsaved_scans ) {
		snprintf( buffer, sizeof(buffer),
			"save scan \"%s\"", scan_savefile );

		if ( motor_record_list != (MX_RECORD *) NULL ) {
			status = cmd_execute_command_line( command_list_length, 
				command_list, buffer );

			if ( status == FAILURE ) {
				fprintf( output,
				"motor_exit: Failed to save scan file '%s'\n",
				scan_savefile );
			}
		}

		motor_has_unsaved_scans = FALSE;
	}

	/* If there are unsaved scans, ask the user whether or not
	 * to save them and where.
	 */

	if ( motor_has_unsaved_scans ) {
		motor_exit_save_dialog();
	}

	/* Shutdown and close all the hardware. */

	mx_status = mx_shutdown_hardware( motor_record_list );

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( output,
		"motor_exit: Unable to shutdown hardware during exit.\n");
	}

	/* Shutdown MX logging. */

	mx_status = mx_log_close( motor_record_list );

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( output,
		"motor_exit: Unable to stop the MX log system during exit.\n");
	}

#if defined(DEBUG_MPATROL)
	heapdiffend( mainloop_heapdiff );
#endif

#if defined(OS_RTEMS)
	motor_rtems_reboot();
#endif

	exit(0);

	/* This function should never return. */

	return FAILURE;
}

int
motor_exit_save_dialog( void )
{
	char default_savefile[ MXU_FILENAME_LENGTH + 1 ];
	char buffer[ MXU_FILENAME_LENGTH + 20 ];
	char response[20];
	int response_length, exitloop, status;

	strlcpy( default_savefile, "", sizeof(default_savefile) );

	switch ( motor_exit_save_policy ) {
	case EXIT_NO_PROMPT_NEVER_SAVE:
		return SUCCESS;

	case EXIT_NO_PROMPT_ALWAYS_SAVE:
		break;

	case EXIT_WITH_PROMPT:
		exitloop = FALSE;

		while( exitloop == FALSE ) {
			response_length = sizeof( response ) - 1;

			status = motor_get_string( output,
		    "\nThere are unsaved scans.  Do you want to save them? ",
				"Y", &response_length, response );

			if ( status != SUCCESS )
				return status;

			switch( response[0] ) {
				case 'Y':
				case 'y':
					exitloop = TRUE;
					break;
				case 'N':
				case 'n':
					fprintf( output,"Scans not saved.\n" );
					return SUCCESS;
					break;
				default:
					fprintf( output,
					    "Please enter either Y or N.\n" );
					break;
			}
		}

		response_length = MXU_FILENAME_LENGTH;

		strlcpy( default_savefile, scan_savefile,
						MXU_FILENAME_LENGTH );

		status = motor_get_string( output,
				"Enter scan savefile name:\n  ",
				default_savefile, &response_length,
				scan_savefile );

		if ( status != SUCCESS ) {
			fprintf( output, "Aborting.  No scan file saved.\n" );
			return SUCCESS;
		}
		break;
	default:
		fprintf( output,
"motor_exit_save_policy has the unexpected value %d.  This is a bug.\n",
			motor_exit_save_policy );
		fprintf( output, "Exiting without saving scans.\n" );
		break;
	}

	snprintf( buffer, sizeof(buffer), "save scan \"%s\"", scan_savefile );

	status = cmd_execute_command_line( command_list_length,
						command_list, buffer );

	if ( status == FAILURE ) {
		fprintf( output, "motor_exit: Failed to save scan file '%s'\n",
			scan_savefile );

		return FAILURE;
	}
	fprintf( output, "Scans successfully saved.\n" );

	return SUCCESS;
}

int
motor_expand_pathname( char *filename, int max_filename_length )
{
	char buffer[ 2 * MXU_FILENAME_LENGTH + 2 ];
	int string_length, filename_length, saved_errno;
	char *filename_ptr;

#if defined( OS_WIN32 ) || defined( OS_MSDOS )
	char drive_letter;
#endif

#if defined( OS_VMS )
	/* On VMS, we currently do not attempt to do any pathname expansion,
	 * so return immediately.
	 */

	return SUCCESS;
#endif

	filename_ptr = filename;

#if defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_DJGPP)

	/* Is this already an absolute pathname? */

	if ( filename[0] == '/' ) {
		/* This is already an absolute filename,
		 * so we leave it alone.
		 */

		return SUCCESS;
	}
#endif

#if defined( OS_WIN32 ) || defined( OS_MSDOS )

	/* Is there a drive letter present? */

	if ( filename[1] == ':' ) {
		drive_letter = filename[0];

		max_filename_length -= 2;

		filename_ptr = filename + 2;
	} else {
		drive_letter = '\0';

		filename_ptr = filename;
	}

	/* Is the remainder of the filename already an absolute pathname? */

	if ( ( filename_ptr[0] == '\\' ) || ( filename_ptr[0] == '/' ) ) {
		/* This is already an absolute filename,
		 * so we leave it alone.
		 */

		return SUCCESS;
	}
#endif

	/* If not, convert the filename into an absolute pathname. */

	if ( getcwd( buffer, sizeof buffer ) == NULL ) {
		saved_errno = errno;

		fprintf( output,
	"Could not determine name of current directory.  Reason = '%s'\n",
			strerror( saved_errno ) );

		return FAILURE;
	}

	string_length = (int) strlen( buffer );

	if ( string_length + 1 >= (2 * MXU_FILENAME_LENGTH) ) {
		fprintf( output,
"The length of the current directory name '%s' is too long.\n",
			buffer );

		return FAILURE;
	}

	/* Do we have to append a path separator to the current
	 * directory name?  If so, then do that.
	 */

#if defined( OS_WIN32 ) || (!defined(OS_DJGPP) && defined( OS_MSDOS ))

	if ( buffer[ string_length - 1 ] != '\\' ) {

		strlcat( buffer, "\\", sizeof(buffer) );
	}
#else
	if ( buffer[ string_length - 1 ] != '/' ) {

		strlcat( buffer, "/", sizeof(buffer) );
	}
#endif

	string_length   = (int) strlen( buffer );
	filename_length = (int) strlen( filename_ptr );

	if ( (string_length + filename_length) > 2 * MXU_FILENAME_LENGTH ) {
		fprintf( output,
"The length of the file name '%s' plus the current directory name '%s' "
"is too long.\n",
			filename, buffer );

		return FAILURE;
	}

	strlcat( buffer, filename_ptr, sizeof(buffer) );

	if ( strlen( buffer ) > max_filename_length ) {
		fprintf( output,
		"The length of the expanded pathname '%s' is too long.\n",
			buffer );

		return FAILURE;
	}

#if defined( OS_WIN32 ) || (!defined(OS_DJGPP) && defined( OS_MSDOS ))
	if ( drive_letter != '\0' ) {
		filename[0] = drive_letter;
		filename[1] = ':';
		filename[2] = '\0';
	} else {
		filename[0] = '\0';
	}
#else
	filename[0] = '\0';
#endif

#if defined( OS_WIN32 ) || defined( OS_MSDOS )
	MXW_UNUSED( drive_letter );
#endif

	strlcat( filename, buffer, max_filename_length );

	return SUCCESS;
}

