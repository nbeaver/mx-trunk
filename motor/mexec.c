/*
 * Name:    mexec.c
 *
 * Purpose: Motor execute command script function.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2005-2006, 2009-2011, 2015, 2021
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

/* There is no branching or conditional logic in these scripts.
 * Features like that will wait for a Tcl/Tk version of the 
 * motor program.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "motor.h"
#include "mx_cfn.h"
#include "command.h"

int
motor_exec_fn( int argc, char *argv[] )
{
	int status;

	if ( argc <= 2 ) {
		fprintf(output, "Usage:  exec 'script_name'\n");
		return FAILURE;
	}

	status =  motor_exec_common( argv[2], TRUE );

	fprintf( output, "\a" );

	return status;
}

int
motor_execq_fn( int argc, char *argv[] )
{
	int status;

	if ( argc <= 2 ) {
		fprintf(output, "execq: No command script name specified.\n");
		return FAILURE;
	}

	status = motor_exec_common( argv[2], FALSE );

	return status;
}

int
motor_exec_script( char *script_name, int verbose_flag )
{
	int status;

	status = motor_exec_common( script_name, verbose_flag );

	switch( status ) {
	case SUCCESS:
		/* Do nothing. */
		break;
	case FAILURE:
		fprintf(output,
			"Error: Script '%s' failed.\n", script_name);
		break;
	case INTERRUPTED:
		fprintf(output,
			"Error: Script '%s' was interrupted.\n",
			script_name);
		break;
	default:
		fprintf(output,
	"Error: Script '%s' failed with unknown error code = %d.\n",
			script_name, status);
		break;
	}

	return status;
}

#define MTR_EXTENSION	".mtr"

int
motor_exec_common( char *script_name, int verbose_flag )
{
	COMMAND *command;
	FILE *script_file;
	int cmd_argc;
	char **cmd_argv = NULL;
	char *split_buffer = NULL;
	char buffer[256];
	int buffer_length, whitespace_length;
	int status, end_of_file;
	char *name_of_found_script = NULL;
	unsigned long flags;

	flags = MXF_FPATH_TRY_WITHOUT_EXTENSION \
		| MXF_FPATH_LOOK_IN_CURRENT_DIRECTORY;

	name_of_found_script = mx_find_file_in_path_old( script_name,
						MTR_EXTENSION,
						"MX_MOTOR_PATH",
						flags );

	if ( name_of_found_script == NULL ) {
		fprintf( output,
	"exec: The command script '%s' was not found in the MX_MOTOR_PATH "
	"or in the current directory.\n",
			script_name );
		return FAILURE;
	}

	script_file = fopen( name_of_found_script, "r" );

	if ( script_file == NULL ) {
		fprintf( output,
		"exec: The command script '%s' is read protected.\n",
			name_of_found_script );

		mx_free( name_of_found_script );
		return FAILURE;
	}

	if ( verbose_flag )
		fprintf( output, "*** Script '%s' invoked.\n",
			name_of_found_script );

	mx_free( name_of_found_script );

	/* Execute the commands in the script one line at a time. */

	status = SUCCESS;
	end_of_file = FALSE;

	while ( status == SUCCESS && end_of_file == FALSE ) {
		/* Read a line from the script. */

		mx_fgets( buffer, sizeof buffer, script_file );

		if ( feof(script_file) ) {
			end_of_file = TRUE;
			continue;   /* cycle the while() loop. */
		}

		if ( ferror(script_file) ) {
			status = FAILURE;
			continue;   /* cycle the while() loop. */
		}

		buffer_length = (int) strlen( buffer );

		if ( verbose_flag )
			fprintf( output, "*** %s\n", buffer );

		/* Is this a comment line?
		 *
		 * First skip over any leading spaces or tabs.
		 */

		whitespace_length = (int) strspn( buffer, " \t" );

		if ( whitespace_length < buffer_length ) {
			/* If the first non-whitespace character is
			 * a comment character '#', then skip over
			 * this line.
			 */

			if ( buffer[whitespace_length] == '#' ) {
				/* Go back to the top of the while() loop
				 * to read another line.
				 */

				continue;	
			}
		}

		/* Parse the supplied command line. */

		status = cmd_split_command_line( buffer,
					&cmd_argc, &cmd_argv, &split_buffer );

		if ( status == FAILURE ) {
			cmd_free_command_line( cmd_argv, split_buffer );
			fclose( script_file );
			return FAILURE;
		}

		if ( cmd_argc < 1 ) {
			fprintf( output,
				"mexec: Invalid command line '%s'.\n", buffer );

			status = FAILURE;
			cmd_free_command_line( cmd_argv, split_buffer );
			continue;	/* cycle the while() loop. */
		}

		if ( cmd_argc == 1 ) {
			/* A blank line was read.  Do nothing. */

			cmd_free_command_line( cmd_argv, split_buffer );
		} else {
			command = cmd_get_command_from_list(
				command_list_length, command_list,
				cmd_argv[1] );

			if ( command == (COMMAND *) NULL ) {
				fprintf( output,
				"mexec: Unrecognized motor command '%s'.\n",
								cmd_argv[1] );

				status = FAILURE;
				cmd_free_command_line( cmd_argv, split_buffer );
				continue;  /* cycle the while() loop. */
			}

			/* Invoke the function. */

			status
			= (*(command->function_ptr))( cmd_argc, cmd_argv );

			cmd_free_command_line( cmd_argv, split_buffer );
		}
	}

	fclose( script_file );

	return status;
}

