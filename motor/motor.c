/*
 * Name:    motor.c
 *
 * Purpose: Main routine for simple motor scan program.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "mx_unistd.h"

#if defined(OS_WIN32)
#include <windows.h>
#include <direct.h>
#endif

#include "motor.h"
#include "mdialog.h"
#include "mx_log.h"
#include "mx_version.h"
#include "mx_key.h"

/* Global variables. */

MX_RECORD *motor_record_list;

char motor_savefile[MXU_FILENAME_LENGTH + 1];
char scan_savefile[MXU_FILENAME_LENGTH + 1];
char global_motorrc[MXU_FILENAME_LENGTH + 1];

COMMAND command_list[] = {
	{ motor_stop_fn,    1, "abort"         },     /* alias for 'stop' */
	{ motor_ccd_fn,     2, "ccd"           },
	{ motor_cd_fn,      2, "cd"            },
	{ motor_sample_changer_fn,
		            2, "changer"       },
	{ motor_close_fn,   2, "close"         },
	{ motor_copy_fn,    3, "copy"          },
	{ motor_count_fn,   3, "count"         },
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
	{ motor_resync_fn,  3, "resynchronize" },
	{ motor_rs232_fn,   2, "rs232",        },
	{ motor_exec_fn,    2, "run"           },     /* alias for 'exec' */
	{ motor_save_fn,    2, "save"          },
	{ motor_sample_changer_fn,
		            3, "sample_changer"},
	{ motor_sca_fn,     3, "sca"           },
	{ motor_scan_fn,    4, "scan"          },
	{ motor_set_fn,     3, "set"           },
	{ motor_setup_fn,   4, "setup"         },
	{ motor_showall_fn, 5, "showall"       },
	{ motor_show_fn,    2, "show"          },
	{ motor_start_fn,   3, "start"         },
	{ motor_stop_fn,    3, "stop"          },
	{ motor_system_fn,  2, "system"        },
	{ motor_take_fn,    1, "take"          },
	{ motor_writep_fn, 11, "writeparams"   },
	{ motor_exec_fn,    1, "@"             },     /* alias for 'exec' */
	{ motor_system_fn,  1, "!"             },     /* alias for 'system' */
	{ motor_system_fn,  1, "$"             },     /* alias for 'system' */
	{ motor_take_fn,    1, "&"             },     /* alias for 'take' */
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
int motor_bypass_limit_switch;
int motor_parameter_warning_flag;

int motor_default_precision;

/* End of global variables. */

#define MAX_SCAN_SAVEFILES	5

#define EXIT_WITH_PROMPT		0
#define EXIT_NO_PROMPT_ALWAYS_SAVE	1
#define EXIT_NO_PROMPT_NEVER_SAVE	2

#if HAVE_MAIN_ROUTINE

int
main( int argc, char *argv[] )

#else /* HAVE_MAIN_ROUTINE */

int
motor_main( int argc, char *argv[] )

#endif /* HAVE_MAIN_ROUTINE */
{
	COMMAND *command;
	int cmd_argc;
	char **cmd_argv;
	char *command_line;
	char *name;
	int status, debug_level, unbuffered_io;
	int init_hw_flags, start_debugger;
	int run_startup_scripts;
	mx_status_type mx_status;
	char prompt[80];
	char saved_command_name[80];
	char *startup_filename;

	static char
	    scan_savefile_array[ MAX_SCAN_SAVEFILES ][ MXU_FILENAME_LENGTH+1 ];
	int num_scan_savefiles;

#if HAVE_GETOPT
	int c, error_flag;
#endif

#if defined( OS_WIN32 )
	{
		/* HACK: If a Win32 compiled program is run from
		 * a Cygwin bash shell under rxvt or a remote ssh
		 * session, _isatty() returns 0 and Win32 thinks
		 * that it needs to fully buffer stdout and stderr.
		 * This interferes with timely updates of the 
		 * output from the MX server in such cases.  The
		 * following ugly hack works around this problem.
		 * 
		 * Read
		 *    http://www.khngai.com/emacs/tty.php
		 * or
		 *    http://homepages.tesco.net/~J.deBoynePollard/
		 *		FGA/capture-console-win32.html
		 * or the thread starting at
		 *    http://sources.redhat.com/ml/cygwin/2003-03/msg01325.html
		 *
		 * for more information about this problem.
		 */

		int is_a_tty;

#if defined( __BORLANDC__ )
		is_a_tty = isatty(fileno(stdin));
#else
		is_a_tty = _isatty(fileno(stdin));
#endif

		if ( is_a_tty == 0 ) {
			setvbuf( stdout, (char *) NULL, _IONBF, 0 );
			setvbuf( stderr, (char *) NULL, _IONBF, 0 );
		}
	}
#endif   /* OS_WIN32 */

#if 0
	{
		int i;

		for ( i = 0; i < argc; i++ ) {
			fprintf(stderr,"argv[%d] = '%s'\n",
				i, argv[i]);
		}
	}
#endif

	cmd_set_program_name( "motor" );

	command_list_length = sizeof(command_list) / sizeof(COMMAND);

	debug_level = 0;
	unbuffered_io = FALSE;
	init_hw_flags = FALSE;

	strlcpy( motor_savefile, DEFAULT_MOTOR_SAVEFILE, MXU_FILENAME_LENGTH );
	strlcpy( scan_savefile,  DEFAULT_SCAN_SAVEFILE,  MXU_FILENAME_LENGTH );
	strlcpy( global_motorrc, GLOBAL_MOTOR_STARTUP,   MXU_FILENAME_LENGTH );

	allow_motor_database_updates = TRUE;
	allow_scan_database_updates = TRUE;
	motor_has_unsaved_scans = FALSE;
	motor_exit_save_policy = EXIT_WITH_PROMPT;
	run_startup_scripts = TRUE;
	motor_install_signal_handlers = TRUE;

	motor_default_precision = 8;

	num_scan_savefiles = 0;

	start_debugger = FALSE;

#if HAVE_GETOPT
	/* Process command line arguments, if any. */

	error_flag = FALSE;

	while ((c = getopt(argc, argv, "d:DF:f:Hg:iNnP:p:S:s:tu")) != -1 ) {
		switch (c) {
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
		case 'u':
			unbuffered_io = TRUE;
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
		mx_start_debugger( NULL );
	}

#endif /* HAVE_GETOPT */

	if ( num_scan_savefiles > 0 ) {
		strlcpy( scan_savefile,
			scan_savefile_array[ num_scan_savefiles - 1 ],
			MXU_FILENAME_LENGTH );
	}

	if ( unbuffered_io ) {
		setbuf( stdin, NULL );
		setbuf( stdout, NULL );
	}

	input = stdin;
#if defined(OS_WIN32)
	output = stderr;
#else
	output = stdout;
#endif

	mx_set_debug_level( debug_level );

	fprintf(output, "\nMX version %s\n", mx_get_version_string() );

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

#if 0
	fprintf( output, "motor savefile = '%s'\n", motor_savefile );
	fprintf( output, "scan savefile  = '%s'\n", scan_savefile );
	fprintf( output, "global motorrc = '%s'\n", global_motorrc );
#endif

	/* Initialize variables from the save files. */

	status = motor_init( motor_savefile,
			num_scan_savefiles, scan_savefile_array,
			init_hw_flags );

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
				exit(1);
			}
		}

		/* Run the per user startup script if any. */

		startup_filename = motor_get_startup_filename();

		if ( startup_filename != NULL ) {
			status = access( startup_filename, R_OK );

			if ( status == 0 ) {
				status = motor_exec_script(
						startup_filename, FALSE );

				if ( status != SUCCESS ) {
					fprintf( stderr,
			"Attempt to execute user startup file '%s' failed.\n",
						startup_filename );
					exit(1);
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
			exit(1);
		}
	}
#else
	if ( argc > 1 ) {
		status = motor_exec_script( argv[1], TRUE );

		if ( status != SUCCESS ) {
			fprintf( stderr,
	"Attempt to execute command line startup script '%s' failed.\n",
				argv[1] );
			exit(1);
		}
	}
#endif

	/* Now go into interactive mode. */

	name = cmd_program_name();

	sprintf( prompt, "%s> ", name );

	while(1) {
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
		} else {
			cmd_argv = cmd_parse_command_line(
					&cmd_argc, command_line );
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

				strncpy( saved_command_name, cmd_argv[1],
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
	}

#if defined(__SUNPRO_C)
	/* The Sparcworks C compiler emits a warning if we have 
	 * unreachable code like below.
	 */
#else

	exit(0);

	/* Some compilers emit a warning if main() doesn't return a value. */

	return 0;
#endif
}

char *
motor_get_startup_filename( void )
{
	static char filename_buffer[100];
	char *ptr;
	int buffer_length;

#if defined(OS_VXWORKS)
	/* Real time kernels do not get per-user startup scripts. */

	return NULL;
#endif

	buffer_length = sizeof(filename_buffer) - 1;

	filename_buffer[0] = '\0';
	filename_buffer[buffer_length] = '\0';

#if defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_DJGPP)
	{
		int used_so_far, free_chars_left;

		ptr = getenv("HOME");
		if ( ptr != NULL ) {
			strncpy( filename_buffer, ptr, buffer_length );
		}
		used_so_far = strlen( filename_buffer );
		free_chars_left = buffer_length - used_so_far;

		if (used_so_far > 0 && free_chars_left > 0) {
			strcat( filename_buffer, "/" );
			used_so_far++;
			free_chars_left++;
		}

		strncat( filename_buffer, USER_MOTOR_STARTUP, free_chars_left );
	}
#elif defined(OS_MSDOS) || defined(OS_WIN32)

	/* The following is probably too simplistic.  We might prefer
	 * to set this up using an environment variable.  Also, under
	 * Windows NT, presumably each user should have their own
	 * startup script.  I'm not prepared, at this time, to deal
	 * with the possibility of defining the startup file location
	 * using the Registry.  I need more Win32 programming experience
	 * before attempting something like that.
	 */

	strncpy( filename_buffer, "c:\\motor.ini", buffer_length );

#else
	/* A fallback, if all else fails. */

	strncpy( filename_buffer, "motor.ini", buffer_length );

#endif /* Unix or MSDOS or Win32 or DJGPP */


	ptr = &filename_buffer[0];

#if 0
	fprintf(output,"startup filename = '%s'\n", ptr);
#endif

	return ptr;
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
	char buffer[80];
	int status;
	mx_status_type mx_status;

	/* Automatically save the motor database if we are supposed to. */

	if ( allow_motor_database_updates ) {
		sprintf(buffer, "save motor %s", motor_savefile);

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
		sprintf(buffer, "save scan \"%s\"", scan_savefile);

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
	char buffer[ MXU_FILENAME_LENGTH + 11 ];
	char response[20];
	int response_length, exitloop, status;

	strcpy( default_savefile, "" );

	switch ( motor_exit_save_policy ) {
	case EXIT_NO_PROMPT_NEVER_SAVE:
		return SUCCESS;
		break;

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

	sprintf( buffer, "save scan \"%s\"", scan_savefile );

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
motor_help_fn( int argc, char *argv[] )
{
	int c, num_lines;
	char *ptr;
	static char help_text[] =
"A more complete command summary may be found at:\n"
"\n"
"    http://mx.iit.edu/motor/\n"
"\n"
"Single word commands are:\n"
"    exit                           - Exits the motor program, saving the\n"
"                                       motor parameters in 'motor.dat' and\n"
"                                       the scan parameters in 'scan.dat'\n"
"    help                           - Displays this message.\n"
"\n"
"Below, type the first word of the command to display a usage message.\n"
"\n"
"    cd 'directory name'            - Change directory to 'directory name'.\n"
"    copy scan 'scan1' 'scan2'      - Make 'scan2' an exact copy of 'scan1'.\n"
"    count 'seconds' 'scaler1' 'scaler2' ...\n"
"                                   - Count scaler channels.\n"
"    delete scan 'scan_name'        - Delete scan parameters for scan.\n"
"    gpib ...                       - Communicate with a GPIB device.\n"
"    load ...                       - Load motor or scan params from a file.\n"
"    mabs 'motorname' 'distance'    - Move absolute 'motorname'\n"
"    mabs 'motorname' steps 'steps' - Move absolute 'motorname' in steps.\n"
"    measure dark_currents          - Measure dark currents for all scalers\n"
"                                       and selected analog inputs.\n"
"    mjog 'motorname' [step_size]   - Move motor using single keystrokes.\n"
"    modify scan 'scan_name'        - Modify scan params for 'scan_name'.\n"
"    mrel 'motorname' 'distance'    - Move relative 'motorname'\n"
"    mrel 'motorname' steps 'steps' - Move relative 'motorname' in steps.\n"
"    resynchronize 'record_name'    - Resynchronizes the MX database with the\n"
"                                       controller for record 'record_name'\n"
"    rs232 ...                      - Communicate with an RS-232 device.\n"
"    save ...                       - Save motor or scan params to a file.\n"
"    scan 'scan_name'               - Perform the scan called 'scan_name'.\n"
"    set ...                        - Set motor, device, plot, etc. params.\n"
"    setup scan 'scan_name'         - Prompts for setup of scan parameters.\n"
"    show ...                       - Show motor, device, plot, etc. params.\n"
"                                       Wildcards '*' and '?' may be used to\n"
"                                       list only a subset of records.\n"
"    ! 'command_name'               - Executes an external command.\n"
"      or\n"
"    $ 'command_name'\n"
"    @ 'script_name'                - Executes a script of motor commands.\n"
"    & 'program_name'               - Executes an external program that\n"
"                                       can directly invoke 'motor' program\n"
"                                       commands.\n";

	ptr = help_text;

	while (1) {
		for ( num_lines = 0; num_lines < 23; num_lines++ ) {
			while ( (c = (int) *ptr) != '\n' ) {
				if ( c == '\0' ) {
					return SUCCESS;
				}
				fputc( c, output );
				ptr++;
			}
			fputc( '\n', output );
			ptr++;
		}
		fprintf( output,
		"Press q to quit or any other key to continue." );

		fflush( output );

		c = mx_getch();

		fputc( '\n', output );

		if ( c == 'q' || c == 'Q' ) {
			return SUCCESS;
		}
	}
}

/* FIXME: The following test with MAXPATHLEN should go away if
 *        we change MXU_FILENAME_LENGTH to be defined in terms of
 *        MAXPATHLEN.
 */

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

	string_length = strlen( buffer );

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

		strcat( buffer, "\\" );
	}
#else
	if ( buffer[ string_length - 1 ] != '/' ) {

		strcat( buffer, "/" );
	}
#endif

	string_length = strlen( buffer );
	filename_length = strlen( filename_ptr );

	if ( (string_length + filename_length) > 2 * MXU_FILENAME_LENGTH ) {
		fprintf( output,
"The length of the file name '%s' plus the current directory name '%s' "
"is too long.\n",
			filename, buffer );

		return FAILURE;
	}

	strcat( buffer, filename_ptr );

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

	strcat( filename, buffer );

	return SUCCESS;
}

