/*
 * Name:    command.c
 *
 * Purpose: Command interpreter functions.
 *
 * Name:    William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003-2007, 2009-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DEBUG_COMMAND_PARSING	FALSE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "motor.h"
#include "command.h"

static char argv0[64];	/* local copy of the program name. */

/*--------------------------------------------------------------------------*/

int
cmd_execute_command_line( int list_length, COMMAND *list, char *command_line )
{
	COMMAND *command;
	int cmd_argc;
	char **cmd_argv = NULL;
	char *split_buffer = NULL;
	int status;

	/* Break up the command line into arguments. */

	status = cmd_split_command_line( command_line,
				&cmd_argc, &cmd_argv, &split_buffer );

	if ( status == FAILURE ) {
		cmd_free_command_line( cmd_argv, split_buffer );
		return FAILURE;
	}

	if ( cmd_argc <= 0 ) {		/* Command parsing error. */
		cmd_free_command_line( cmd_argv, split_buffer );
		return 0;

	} else if ( cmd_argc == 1 ) {	/* Blank command line. */
		cmd_free_command_line( cmd_argv, split_buffer );
		return 1;
	}

	/* Get the command from the command list. */

	command = cmd_get_command_from_list( list_length, list, cmd_argv[1] );

	if ( command == (COMMAND *) NULL ) {	/* Unrecognized command. */
		cmd_free_command_line( cmd_argv, split_buffer );
		return 0;
	}

	/* Invoke the command returned. */

	status = (*(command->function_ptr))( cmd_argc, cmd_argv );

	cmd_free_command_line( cmd_argv, split_buffer );

	return status;
}

/*--------------------------------------------------------------------------*/

COMMAND *
cmd_get_command_from_list( int num_commands, COMMAND *list, char *string )
{
	COMMAND *command;
	int i;
	size_t length;

	length = strlen( string );

	for ( i = 0; i < num_commands; i++ ) {
		if ( length < list[i].min_required_chars )
			continue;

		if ( strncmp( string, list[i].name, length ) == 0 )
			break;
	}

	if ( i >= num_commands ) {
		command = (COMMAND *) NULL;
	} else {
		command = &( list[i] );
	}

	return command;
}

/*--------------------------------------------------------------------------*/

/* Command line parser for 'motor'. */

int
cmd_split_command_line( char *command_line,
			int *cmd_argc,
			char ***cmd_argv,
			char **split_buffer )
{
	static char fname[] = "cmd_split_command_line():";

	size_t split_buffer_length, chars_to_skip, src_length;
	char argv1_token = '\0';
	char *command_ptr;
	unsigned long i, max_tokens;
	char *src_ptr, *dest_ptr;
	mx_bool_type in_token, old_in_token, in_quoted_string;

	if ( split_buffer == NULL ) {
		fprintf( output, "%s: split_buffer pointer is NULL.\n", fname );
		return FAILURE;
	}

	/* Skip any leading spaces or tabs. */

	chars_to_skip = strspn( command_line, " \t" );

	command_ptr = command_line + chars_to_skip;

	/* Is the first character one of the special command characters? */

	switch( *command_ptr ) {
	case '!':
	case '$':
	case '@':
	case '&':
	case '#':
		argv1_token = *command_ptr;
		command_ptr++;

		chars_to_skip = strspn( command_ptr, " \t" );
		command_ptr += chars_to_skip;
		break;
	}

	/* In the rest of the command line, tokens are separated by
	 * space and/or tab characters.  However, blocks of characters
	 * inside pairs of double quotes are treated as a single token.
	 */

#if 0
	{
		/* Compute an upper limit to the maximum number of tokens in
		 * the command line by performing a string split using spaces
		 * and tabs as the delimiters.
		 */

		char *temp_string;
		int temp_argc;
		char **temp_argv;

		temp_string = strdup( command_ptr );

		mx_string_split( temp_string, " \t", &temp_argc, &temp_argv );

		mx_free( temp_argv );
		mx_free( temp_string );

		max_tokens = temp_argc + 1;	/* Add 1 for the argv0 token. */
	}
#else
	{
		/* Compute an upper limit on the maximum number of tokens in
		 * the command line.
		 */

		char *ptr;
		mx_bool_type in_delimiter, end_of_string;

		max_tokens = 1;		/* Must include the argv0 token. */

		ptr = command_ptr;

		in_delimiter = TRUE;
		end_of_string = FALSE;

		while (1) {
			switch( *ptr ) {
			case '\0':
				end_of_string = TRUE;
				break;
			case ' ':
			case '\t':
				in_delimiter = TRUE;
				break;
			default:
				if ( in_delimiter ) {
					max_tokens++;
				}
				in_delimiter = FALSE;
				break;
			}

			if ( end_of_string )
				break;

			ptr++;
		}

#if 0
		fprintf(stderr, "Max tokens = %lu\n", max_tokens );
#endif
	}
#endif

	/* If present, add 1 for the argv1 token. */

	if ( argv1_token != '\0' ) {
		max_tokens += 1;
	}

	/* Compute an upper limit to the split buffer length. */

	split_buffer_length = 
		strlen( command_ptr ) + strlen( argv0 ) + max_tokens + 2;

	if ( argv1_token != '\0' ) {
		split_buffer_length += 2;
	}

	/* Allocate space for the split buffer. */

	*split_buffer = malloc( split_buffer_length );

	if ( *split_buffer == NULL ) {
		fprintf( output,
	    "%s: Ran out of memory allocating a split buffer of %ld bytes.\n",
			fname, (long) split_buffer_length );
		return FAILURE;
	}

#if DEBUG_COMMAND_PARSING
	fprintf( output, "max_tokens = %ld\n", max_tokens );
	fprintf( output, "split_buffer_length = %ld\n",
					(long) split_buffer_length );
#endif

	/* Allocate space for the cmd_argv array. */

	*cmd_argv = (char **) malloc( max_tokens * sizeof(char *) );

	if ( *cmd_argv == (char **) NULL ) {
		fprintf( output,
	    "%s: Ran out of memory allocating a %ld element cmd_argv array.\n",
			fname, (long) max_tokens );
	}

	src_ptr = command_ptr;
	dest_ptr = *split_buffer;

	/* Copy in argv0. */

	strlcpy( dest_ptr, argv0, split_buffer_length );

	*cmd_argc = 0;
	(*cmd_argv)[*cmd_argc] = dest_ptr;
	(*cmd_argc)++;
	dest_ptr += strlen( argv0 ) + 1;

	/* If present, copy in the argv1_token. */

	if ( argv1_token != '\0' ) {
		dest_ptr[0] = argv1_token;
		dest_ptr[1] = '\0';
		(*cmd_argv)[*cmd_argc] = dest_ptr;
		(*cmd_argc)++;
		dest_ptr += 2;
	}

	/* Now copy in the rest of the tokens. */

	src_length = strlen( src_ptr );

	in_token = FALSE;
	old_in_token = FALSE;
	in_quoted_string = FALSE;

	for ( i = 0; i < src_length; i++, src_ptr++ ) {

		old_in_token = in_token;

		switch( *src_ptr ) {
		case ' ':
		case '\t':
			if ( in_token ) {
				if ( in_quoted_string ) {
					*dest_ptr = *src_ptr;
				} else {
					in_token = FALSE;

					*dest_ptr = '\0';
				}
				dest_ptr++;
			}
			break;
		case '"':
			if ( in_token ) {
				in_token = FALSE;
				in_quoted_string = FALSE;

				*dest_ptr = '\0';

				dest_ptr++;
			} else {
				in_token = TRUE;
				in_quoted_string = TRUE;

				(*cmd_argv)[*cmd_argc] = dest_ptr;
				(*cmd_argc)++;
			}
			break;
		default:
			if ( in_token == FALSE ) {
				in_token = TRUE;

				(*cmd_argv)[*cmd_argc] = dest_ptr;
				(*cmd_argc)++;
			}

			*dest_ptr = *src_ptr;

			dest_ptr++;
		}
	}

	*dest_ptr = '\0';

#if DEBUG_COMMAND_PARSING
	fprintf( output, "argc = %d\n", *cmd_argc );

	for ( i = 0; i < *cmd_argc; i++ ) {
		fprintf( output, "argv[%lu] = '%s'\n", i, (*cmd_argv)[i] );
	}
#endif

	return SUCCESS;
}

/*--------------------------------------------------------------------------*/

int
cmd_run_command( char *command_buffer )
{
	COMMAND *command;
	int cmd_argc;
	char **cmd_argv = NULL;
	char *split_buffer = NULL;
	int cmd_status;

	cmd_status = cmd_split_command_line( command_buffer,
				&cmd_argc, &cmd_argv, &split_buffer );

	if ( cmd_status == FAILURE ) {
		cmd_free_command_line( cmd_argv, split_buffer );
		return FAILURE;
	}

	if ( cmd_argc < 1 ) {
		fprintf( output,
			"Invalid command line '%s'.\n", command_buffer );
		cmd_free_command_line( cmd_argv, split_buffer );
		return FAILURE;
	}

	if ( cmd_argc == 0 ) {
		/* The line is blank, so we do nothing. */

		cmd_free_command_line( cmd_argv, split_buffer );
		return SUCCESS;
	} else {
		command = cmd_get_command_from_list(
			command_list_length, command_list,
			cmd_argv[1] );

		if ( command == (COMMAND *) NULL ) {
			fprintf( output,
				"Unrecognized command '%s'.\n", cmd_argv[1] );
			cmd_free_command_line( cmd_argv, split_buffer );
			return FAILURE;
		}

		/* Invoke the function. */

		cmd_status = (*(command->function_ptr))( cmd_argc, cmd_argv );

		cmd_free_command_line( cmd_argv, split_buffer );
		return cmd_status;
	}
}

/*--------------------------------------------------------------------------*/

#if ( MX_CMDLINE_PROCESSOR == MX_CMDLINE_FGETS )

char *
cmd_read_next_command_line( char *prompt )
{
	static char *ptr;

	static char buffer[250];

	fprintf( output, prompt );
	fflush( output );

	mx_fgets( buffer, sizeof buffer, input );

	if ( feof(input) || ferror(input) ) {
		ptr = NULL;
	} else {
		ptr = &buffer[0];
	}

	return ptr;
}

#elif ( MX_CMDLINE_PROCESSOR == MX_CMDLINE_VMS )

#if 1	/* smg$read_composed_line */

#include <descrip.h>
#include <smg$routines.h>
#include <ssdef.h>

static unsigned long cmd_keyboard_id = 0;

char *
cmd_read_next_command_line( char *prompt )
{
	static char buffer[250];
	static char local_prompt[80];
	static char filespec[250];
	$DESCRIPTOR( input_desc, "sys$command" );
	$DESCRIPTOR( buffer_desc, buffer );
	$DESCRIPTOR( prompt_desc, local_prompt );
	$DESCRIPTOR( filespec_desc, filespec );
	int vms_status;
	unsigned char recall_size;
	unsigned short return_length, terminator;

	strlcpy( local_prompt, prompt, sizeof(local_prompt) );

	prompt_desc.dsc$w_length = strlen( local_prompt );

	if ( cmd_keyboard_id == 0 ) {
		recall_size = 20;
		
		vms_status = smg$create_virtual_keyboard(
				&cmd_keyboard_id,
				&input_desc,
				0,
				&filespec_desc,
				&recall_size );

		if ( vms_status != SS$_NORMAL ) {
			fprintf( output,
		"Error creating VMS virtual keyboard.  "
		"VMS error number %d, error message = '%s'\n",
				vms_status, strerror( EVMSERR, vms_status ) );
			return NULL;
		}
	}

	vms_status = smg$read_composed_line(
			&cmd_keyboard_id,
			0,
			&buffer_desc,
			&prompt_desc,
			&return_length,
			0,
			0,
			0,
			0,
			0,
			0,
			&terminator );

	if ( vms_status != SS$_NORMAL ) {
		fprintf( output,
		"Error reading line from keyboard.  "
		"VMS error number %d, error message = '%s'\n",
				vms_status, strerror( EVMSERR, vms_status ) );
		exit(vms_status);
		return NULL;
	}

	return &buffer[0];
}

#else	/* NOT smg$read_composed_line */

#include <descrip.h>
#include <ssdef.h>
#include <lib$routines.h>

char *
cmd_read_next_command_line( char *prompt )
{
	static char buffer[250];
	static char local_prompt[80];
	$DESCRIPTOR( buffer_desc, buffer );
	$DESCRIPTOR( prompt_desc, local_prompt );
	unsigned short return_length;
	int vms_status;

	strlcpy( local_prompt, prompt, sizeof(local_prompt) );

	prompt_desc.dsc$w_length = strlen( local_prompt );

	vms_status = lib$get_command( &buffer_desc,
				&prompt_desc,
				&return_length );

	if ( vms_status != SS$_NORMAL ) {
		fprintf( output,
		"Error reading line from keyboard.  "
		"VMS error number %d, error message = '%s'\n",
				vms_status, strerror( EVMSERR, vms_status ) );
		exit(vms_status);
		return NULL;
	}

	buffer[ return_length ] = '\0';

	return &buffer[0];
}

#endif

#elif ( MX_CMDLINE_PROCESSOR == MX_CMDLINE_READLINE )

char *
cmd_read_next_command_line( char *prompt )
{
	static char *ptr;

	static char buffer[250];

	/* We only use readline() on TTY devices.  This way, if standard
	 * input or output are redirected to pipes or some such, we will
	 * not get hung with readline() trying to make system calls that
	 * only work for TTY devices.
	 */

	if ( isatty( STDIN_FILENO ) && isatty( STDOUT_FILENO ) ) {
		fflush( output );

		ptr = readline( prompt );

		if ( ptr != NULL ) {
			if ( *ptr != '\0' ) {
				/* Add to GNU readline command history list. */

				add_history( ptr );

				strlcpy( buffer, ptr, sizeof(buffer) );
			} else {
				strlcpy( buffer, "", sizeof(buffer) );
			}

			free(ptr);

			ptr = &buffer[0];
		}
	} else {
		fputs( prompt, output );
		fflush( output );
	
		mx_fgets( buffer, sizeof buffer, input );
	
		if ( feof(input) || ferror(input) ) {
			ptr = NULL;
		} else {
			ptr = &buffer[0];
	
			/* Null terminate the buffer just in case it isn't. */
	
			buffer[ sizeof(buffer) - 1 ] = '\0';
		}
	}

	return ptr;
}

#else /* MX_CMDLINE_PROCESSOR */

#error "MX_CMDLINE_PROCESSOR is not defined for this operating system."

#endif /* MX_CMDLINE_PROCESSOR */

char *
cmd_program_name( void )
{
	return argv0;
}

void
cmd_set_program_name( char *name )
{
	int i, c;
	char *left_end, *right_end;
	char *ptr;

	/* This algorithm should work for MSDOS 3.0 and above (or for Unix).*/

	if ( name == NULL ) {
		strlcpy( argv0, "", sizeof(argv0) );
	} else {

#if defined(OS_UNIX) || defined(OS_DJGPP)
		left_end = strrchr( name, '/' );
#else  /* MSDOS */
		left_end = strrchr( name, '\\' );
#endif
		if ( left_end == NULL ) {
			left_end = name;
		} else {
			left_end++;
		}

#if defined(OS_UNIX) || defined(OS_DJGPP)
		right_end = name + strlen(name) - 1;
#else  /* MSDOS */
		right_end = strrchr( name, '.' );

		if ( right_end == NULL ) {
			right_end = name + strlen(name) - 1;
		} else {
			right_end--;
		}
#endif
		if ( right_end - left_end >= sizeof(argv0) ) {
			right_end = left_end + sizeof(argv0) - 1;
		}

		for ( i = 0, ptr = left_end; ptr <= right_end; i++, ptr++ ) {
			c = (int) (*ptr);

			if ( isupper(c) ) {
				argv0[i] = (char) tolower(c);
			} else {
				argv0[i] = (char) c;
			}
		}

		argv0[i] = '\0';
	}
	return;
}

