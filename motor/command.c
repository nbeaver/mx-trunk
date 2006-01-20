/*
 * Name:    command.c
 *
 * Purpose: Command interpreter functions.
 *
 * Name:    William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "motor.h"
#include "command.h"

static char argv0[64];	/* local copy of the program name. */

int
cmd_execute_command_line( int list_length, COMMAND *list, char *command_line )
{
	COMMAND *command;
	int cmd_argc;
	char **cmd_argv;
	int status;

	/* Break up the command line into arguments. */

	cmd_argv = cmd_parse_command_line( &cmd_argc, command_line );

	if ( cmd_argc <= 0 ) {		/* Command parsing error. */
		return 0;
	} else if ( cmd_argc == 1 ) {	/* Blank command line. */
		return 1;
	}

	/* Get command from the command list. */

	command = cmd_get_command_from_list( list_length, list, cmd_argv[1] );

	if ( command == (COMMAND *) NULL ) {	/* Unrecognized command. */
		return 0;
	}

	/* Invoke the command returned. */

	status = (*(command->function_ptr))( cmd_argc, cmd_argv );

	return status;
}

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

/* cmd_parse_command_line() takes a string 'command_line' and parses it 
 * into an 'argc' and 'argv' combination just like the one passed to 
 * the main() routine.
 *
 * cmd_parse_command_line() returns a pointer to a static buffer.  This means
 * that the next call to cmd_parse_command_line() will overwrite the results
 * from the last call.  If you wish to save the command line, you
 * need to copy it to storage local to the calling routine.
 *
 * At the moment there is a limit of POINTER_ARRAY_LENGTH elements in 
 * the argv array.  This limit could be removed by modifying the routine 
 * to malloc() the necessary storage.  However, this would also require 
 * that the calling routines remember to free() the memory allocated if 
 * not needed anymore.
 */

#define MAX_COMMAND_LENGTH	500
#define POINTER_ARRAY_LENGTH    100

#define DEBUG_COMMAND_PARSING	FALSE

#define ADD_TO_ARGV \
		do { \
			if ( in_token == TRUE && old_in_token == FALSE ) { \
				argv[ *argc ] = dest_ptr; \
				(*argc)++; \
			} \
		} while(0)

char **
cmd_parse_command_line( int *argc, char *command_line )
{
	static char buffer[MAX_COMMAND_LENGTH + 1];
	static char *argv[POINTER_ARRAY_LENGTH];
	char *src_ptr, *dest_ptr;
	int ptr_diff;
	size_t src_length;
	int in_token, old_in_token, first_token, in_quoted_string;
	int i;

	/* Initialize things. */

	*argc = 0;

	for ( i = 0; i < POINTER_ARRAY_LENGTH; i++ ) {
		argv[i] = NULL;
	}
	for ( i = 0; i <= MAX_COMMAND_LENGTH; i++ ) {
		buffer[i] = '\0';
	}

	/* argv[0] is always the name of the main program. */

	argv[0] = argv0;	/* argv0 saved by cmd_set_program_name(). */

	*argc = 1;

	src_ptr = command_line;
	dest_ptr = buffer;

	src_length = strlen( command_line );

	in_token = FALSE;
	old_in_token = FALSE;
	first_token = TRUE;
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

				ADD_TO_ARGV;
			}
			break;

		case '!':
		case '$':
		case '@':
		case '&':
			if ( first_token ) {

				first_token = FALSE;
				in_token = TRUE;

				*dest_ptr = *src_ptr;

				ADD_TO_ARGV;

				dest_ptr++;
				*dest_ptr = '\0';
				dest_ptr++;

				in_token = FALSE;

				break;	/* Exit the switch() statement. */
			}

			/* If this is not the first token on the command
			 * line, we intentionally fall through here to the
			 * 'default' case.
			 */
		default:
			if ( ! in_token ) {
				in_token = TRUE;

				ADD_TO_ARGV;
			}

			*dest_ptr = *src_ptr;

			dest_ptr++;
		}

		if ( first_token == TRUE && in_token == TRUE ) {
			first_token = FALSE;
		}

		/* Check for buffer overruns. */

		/* For the purposes of the "take" command,
		 * argv[POINTER_ARRAY_LENGTH - 1] _must_ be
		 * equal to NULL which is why the following
		 * test is set up the way it is.
		 */

		if ( *argc >= (POINTER_ARRAY_LENGTH - 1) ) {
			if ( ! in_token ) {
				break;		/* Exit the for() loop. */
			}
		}

		ptr_diff = dest_ptr - buffer;

		if ( ptr_diff >= MAX_COMMAND_LENGTH ) {
			if ( ptr_diff == MAX_COMMAND_LENGTH ) {
				*dest_ptr = '\0';
			}
			break;			/* Exit the for() loop. */
		}
	}

#if DEBUG_COMMAND_PARSING
	fprintf( output, "argc = %d\n", *argc );

	for ( i = 0; i < *argc; i++ ) {
		fprintf( output, "argv[%d] = '%s'\n", i, argv[i] );
	}
#endif
	return argv;
}

#if ( MX_CMDLINE_PROCESSOR == MX_CMDLINE_FGETS )

char *
cmd_read_next_command_line( char *prompt )
{
	static char *ptr;

	static char buffer[250];
	int length;

	fprintf( output, prompt );
	fflush( output );

	fgets( buffer, sizeof buffer, input );

	if ( feof(input) || ferror(input) ) {
		ptr = NULL;
	} else {
		ptr = &buffer[0];

		/* Null terminate the buffer just in case it isn't. */

		buffer[ sizeof(buffer) - 1 ] = '\0';

		/* Zap any trailing newline. */

		length = strlen( buffer );

		if ( buffer[ length-1 ] == '\n' ) {
			buffer[ length-1 ] = '\0';
		}
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
	int length;

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

				strncpy( buffer, ptr, sizeof(buffer) - 1 );
			} else {
				strcpy( buffer, "" );
			}

			free(ptr);

			ptr = &buffer[0];
		}
	} else {
		fprintf( output, prompt );
		fflush( output );
	
		fgets( buffer, sizeof buffer, input );
	
		if ( feof(input) || ferror(input) ) {
			ptr = NULL;
		} else {
			ptr = &buffer[0];
	
			/* Null terminate the buffer just in case it isn't. */
	
			buffer[ sizeof(buffer) - 1 ] = '\0';

			/* Zap any trailing newline. */

			length = strlen( buffer );

			if ( buffer[ length-1 ] == '\n' ) {
				buffer[ length-1 ] = '\0';
			}
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
		strcpy( argv0, "" );
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

