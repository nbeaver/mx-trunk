/*
 * Name:    mdialog.c
 *
 * Purpose: Implements text-mode user dialog handlers for motor.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include "motor.h"
#include "mdialog.h"

#include "mx_ascii.h"
#include "mx_key.h"
#include "keycodes.h"

#define WHITESPACE	" \t"

int
motor_get_int( FILE *file, char *prompt,
		int have_default, int default_value, int *value,
		int lower_limit, int upper_limit)
{
	char buffer[100];
	char default_string[50];
	int input_accepted, status;
	int count, string_length, buffer_length, whitespace_length;

	if ( have_default ) {
		snprintf( default_string, sizeof(default_string),
			"%d", default_value );
	} else {
		strlcpy( default_string, "", sizeof(default_string) );
	}

	input_accepted = FALSE;

	while ( ! input_accepted ) {

		buffer_length = sizeof(buffer) - 1;

		status = motor_get_string(file, prompt, default_string,
						&buffer_length, &buffer[0]);

		if ( status == FAILURE ) {
			return FAILURE;
		}

		string_length = (int) strlen( buffer );

		whitespace_length = (int) strspn( buffer, WHITESPACE );

		if ( whitespace_length >= string_length ) {
			if ( have_default ) {
				*value = default_value;
			} else {
				*value = 0;       /* The ultimate default. */
			}
		}

		count = sscanf( buffer, "%d", value );

		if ( count == 0 ) {
			fprintf( file, "Non-numeric value entered.\n" );

		} else if ( *value < lower_limit || *value > upper_limit ) {
			fprintf( file,
			"Value %d is outside the allowed range of %d to %d\n",
				*value, lower_limit, upper_limit );

		} else {
			input_accepted = TRUE;
		}
	}

	return SUCCESS;
}

int
motor_get_long( FILE *file, char *prompt,
		int have_default, long default_value, long *value,
		long lower_limit, long upper_limit)
{
	char buffer[100];
	char default_string[50];
	int input_accepted, status;
	int count, string_length, buffer_length, whitespace_length;

	if ( have_default ) {
		snprintf( default_string, sizeof(default_string),
			"%ld", default_value );
	} else {
		strlcpy( default_string, "", sizeof(default_string) );
	}

	input_accepted = FALSE;

	while ( ! input_accepted ) {

		buffer_length = sizeof(buffer) - 1;

		status = motor_get_string(file, prompt, default_string,
						&buffer_length, &buffer[0]);

		if ( status == FAILURE ) {
			return FAILURE;
		}

		string_length = (int) strlen( buffer );

		whitespace_length = (int) strspn( buffer, WHITESPACE );

		if ( whitespace_length >= string_length ) {
			if ( have_default ) {
				*value = default_value;
			} else {
				*value = 0;       /* The ultimate default. */
			}
		}

		count = sscanf( buffer, "%ld", value );

		if ( count == 0 ) {
			fprintf( file, "Non-numeric value entered.\n" );

		} else if ( *value < lower_limit || *value > upper_limit ) {
			fprintf( file,
		"Value %ld is outside the allowed range of %ld to %ld\n",
				*value, lower_limit, upper_limit );

		} else {
			input_accepted = TRUE;
		}
	}

	return SUCCESS;
}

int
motor_get_double( FILE *file, char *prompt,
		int have_default, double default_value, double *value,
		double lower_limit, double upper_limit)
{
	char buffer[100];
	char default_string[50];
	int input_accepted, status;
	int count, string_length, buffer_length, whitespace_length;

	if ( have_default ) {
		snprintf( default_string, sizeof(default_string),
			"%g", default_value );
	} else {
		strlcpy( default_string, "", sizeof(default_string) );
	}

	input_accepted = FALSE;

	while ( ! input_accepted ) {

		buffer_length = sizeof(buffer) - 1;

		status = motor_get_string(file, prompt, default_string,
						&buffer_length, &buffer[0]);

		if ( status == FAILURE ) {
			return FAILURE;
		}

		string_length = (int) strlen( buffer );

		whitespace_length = (int) strspn( buffer, WHITESPACE );

		if ( whitespace_length >= string_length ) {
			if ( have_default ) {
				*value = default_value;
			} else {
				*value = 0;       /* The ultimate default. */
			}
		}

		count = sscanf( buffer, "%lf", value );

		if ( count == 0 ) {
			fprintf( file, "Non-numeric value entered.\n" );

		} else if ( *value < lower_limit || *value > upper_limit ) {
			fprintf( file,
		"Value %g is outside the allowed range of %g to %g\n",
				*value, lower_limit, upper_limit );

		} else {
			input_accepted = TRUE;
		}
	}

	return SUCCESS;
}

#if ( MX_CMDLINE_PROCESSOR == MX_CMDLINE_VMS )
#include <descrip.h>
#include <ssdef.h>
#include <lib$routines.h>
#endif

int
motor_get_string( FILE *file, char *prompt, char *default_string,
		int *string_length, char *string )
{
	char *ptr;
	char real_prompt[200];
	size_t real_prompt_length;
	int input_accepted;

#if ( MX_CMDLINE_PROCESSOR == MX_CMDLINE_VMS )

	char local_string[200];
	$DESCRIPTOR( real_prompt_desc, real_prompt );
	$DESCRIPTOR( local_string_desc, local_string );
	unsigned short return_length;
	int i, j, c, new_string_length, vms_status;

#elif ( MX_CMDLINE_PROCESSOR == MX_CMDLINE_READLINE )

	/* Nothing for now */
#else
	int i, c, new_string_length;
#endif

	if ( file == NULL ) {
		fprintf( stderr,
			"motor_get_string():  Error! (file == NULL)\n");
		return FAILURE;
	}

	if ( prompt == NULL ) {
		fprintf( file,
			"motor_get_string():  Error! (prompt == NULL)\n");
		return FAILURE;
	}

	if ( string_length == NULL ) {
		fprintf( file,
		"motor_get_string():  Error! (string_length == NULL)\n");
		return FAILURE;
	}

	if ( string == NULL ) {
		fprintf( file,
			"motor_get_string():  Error! (string == NULL)\n");
		return FAILURE;
	}

	if ( *string_length <= 0 ) {
		fprintf( file,
	"motor_get_string():  Error! Invalid length %d for string buffer.\n",
			*string_length );
		return FAILURE;
	}

	/* Construct the prompt the user will actually see. */

#if ( MX_CMDLINE_PROCESSOR != MX_CMDLINE_VMS )

	strlcpy( real_prompt, prompt, sizeof(real_prompt) );

#else /* MX_CMDLINE_VMS */

	/* On VMS, newline (\n) characters in the string must be expanded
	 * into <cr><lf> pairs manually, since lib$get_command() does not
	 * treat the string \n as a special token.
	 */

	for ( i = 0, j = 0; i < strlen(prompt); i++, j++ ) {
		c = prompt[i];

		switch( c ) {
		case '\n':
			real_prompt[j] = MX_CR;
			j++;
			real_prompt[j] = MX_LF;
			break;
		default:
			real_prompt[j] = c;
			break;
		}

		/* Don't write past the end of the buffer. */

		if ( j >= ( sizeof(real_prompt) - 1 ) )
			break;
	}

	real_prompt[j] = '\0';

#endif /* MX_CMDLINE_VMS */

	/* EXTRA_CHARS is the number of characters added to the
	 * prompt for things like " (CR = " and so forth.
	 */

#define EXTRA_CHARS	11

	if ( default_string != NULL && strlen(default_string) > 0 ) {

		real_prompt_length = strlen( real_prompt );

		if ( real_prompt_length + strlen(default_string) + EXTRA_CHARS
						> sizeof( real_prompt ) - 1 ) {
			fprintf( file, "motor_get_string():  Error! "
			"The prompt plus the default string are too long.\n");
			return FAILURE;
		}

		ptr = real_prompt + real_prompt_length;

		snprintf( ptr, sizeof(real_prompt) - real_prompt_length,
			"(CR = '%s') ", default_string );
	}

#if ( MX_CMDLINE_PROCESSOR == MX_CMDLINE_READLINE )

	/* Go into an input loop until we get a valid input. */

	input_accepted = FALSE;

	while ( ! input_accepted ) {
		/* We only use readline() on TTY devices.  This way, if
		 * standard input or output are redirected to pipes or
		 * some such, we will not get hung with readline() trying
		 * to make system calls that only work for TTY devices.
		 */

		if ( isatty( STDIN_FILENO ) && isatty( STDOUT_FILENO ) ) {
			ptr = readline( real_prompt );
	
			if ( ptr == NULL ) {
				return FAILURE;
			} else {
				if ( strlen(ptr) == 0 ) {
					if ( default_string != NULL &&
					  strlen(default_string) > 0 ) {
	
						strlcpy( string, default_string,
							    *string_length );
					} else {
						strlcpy( string, "",
							    *string_length );
					}
				} else {
					strlcpy( string, ptr, *string_length );
				}
				*string_length = (int) strlen(string);
				input_accepted = TRUE;
	
				free(ptr);
			}
		} else {
			fprintf( file, "%s", real_prompt );
			fflush( file );

			fgets( string, *string_length, stdin );

			if ( feof(input) || ferror(input) ) {
				*string = '\0';
				*string_length = 0;
			} else {
				/* Null terminate the string just in case
				 * it isn't.
				 */

				string[ *string_length - 1 ] = '\0';

				*string_length = (int) strlen(string);

				/* Zap any trailing newline. */

				if ( string[ *string_length - 1 ] == '\n' ) {
					string[ *string_length - 1 ] = '\0';

					*string_length = (int) strlen(string);
				}
			}
		}
	}

#elif ( MX_CMDLINE_PROCESSOR == MX_CMDLINE_VMS )

	real_prompt_desc.dsc$w_length = strlen( real_prompt );

	vms_status = lib$get_command( &local_string_desc,
					&real_prompt_desc,
					&return_length );

	if ( vms_status != SS$_NORMAL ) {
		fprintf( output,
		"Error reading line from keyboard.  "
		"VMS error number %d, error message = '%s'\n",
			vms_status, strerror( EVMSERR, vms_status ) );

		return FAILURE;
	}

	if ( return_length < *string_length ) {
		local_string[ return_length ] = '\0';

		new_string_length = return_length + 1;
	} else {
		local_string[ (*string_length) - 1 ] = '\0';

		new_string_length = *string_length;
	}

	if ( ( new_string_length <= 1 )
	  && ( default_string != NULL )
	  && ( strlen(default_string) > 0 ) )
	{
		/* Copy in the default string if the user didn't enter one. */

		strlcpy( string, default_string, *string_length );

		new_string_length = strlen( string ) + 1;
	} else {
		/* Otherwise, copy the string the user provided. */

		strlcpy( string, local_string, *string_length );
	}

	*string_length = new_string_length;

#else  /* MX_CMDLINE_PROCESSOR */

	/* Go into an input loop until we get a valid input. */

	input_accepted = FALSE;

	while ( ! input_accepted ) {
		fprintf( file, "%s", real_prompt );
		fflush( file );

		for ( i = 0; i < *string_length; i++ ) {
			c = mx_getch();

			c &= 0xFF;	/* Mask to 8 bits. */

			if ( c == ESC ) {
				return FAILURE;
			}

			if ( c == CTRL_D ) {
				return FAILURE;
			}

			if ( c == CTRL_H ) {	/* backspace */
				if ( i > 0 ) {
					i--;

					string[i] = '\0';

					i--;

					fputc( CTRL_H, file );
					fputc( ' ', file );
					fputc( CTRL_H, file );
					fflush(file);
				}

				continue;	/* Cycle the for() loop. */
			}

			if ( c == '\n' || c == CR || c == LF ) {
				string[i] = '\0';

				fputc( '\n', file );
				fflush( file );

				break;		/* Exit the for() loop. */
			}

			if ( isprint(c) ) {
				fputc( c, file );
				fflush( file );

				string[i] = (char) c;
			}
		}

		if ( i < *string_length ) {
			new_string_length = i;
		} else {
			new_string_length = *string_length;
		}

		/* Null terminate the string, just in case it isn't already. */

		string[new_string_length] = '\0';

		/* Copy in the default string if the user didn't enter one. */

		if ( ( new_string_length <= 0 )
		  && ( default_string != NULL )
		  && ( strlen(default_string) > 0 ) )
		{
			strlcpy( string, default_string, *string_length );
			new_string_length = strlen(string);
		}

		*string_length = new_string_length;
		input_accepted = TRUE;
	}

#endif  /* MX_CMDLINE_PROCESSOR */

	return SUCCESS;
}

int
motor_get_string_from_list( FILE *file, char *prompt,
		int num_strings, int *min_length_array, char **string_array,
		int default_string_number, int *string_length,
		char *selected_string )
{
	const char fname[] = "motor_get_string_from_list()";

	char real_prompt[120];
	int i, status, length, valid_input, output_buffer_length;
#if 0
	fprintf( stderr, "prompt = '%s'\n", prompt );
	fprintf( stderr, "num_strings = %d\n", num_strings );
	for ( i = 0; i < num_strings; i++ ) {
		fprintf( stderr,
		"min_length_array[%d] = %d, string_array[%d] = '%s'\n",
			i, min_length_array[i], i, string_array[i] );
	}
	fprintf( stderr, "initial *string_length = %d\n", *string_length );
#endif

	if ( file == NULL ) {
		fprintf(stderr, "%s: 'file' argument is NULL\n", fname);
		return FAILURE;
	}
	if ( string_length == NULL ) {
		fprintf(file, "%s: 'string_length' argument is NULL\n", fname);
		return FAILURE;
	}
	if ( selected_string == NULL ) {
		fprintf(file,"%s: 'selected_string' argument is NULL\n",fname);
		return FAILURE;
	}
	if ( min_length_array == NULL ) {
		fprintf(file,"%s: 'min_length_array' argument is NULL\n",fname);
		return FAILURE;
	}
	if ( string_array == NULL ) {
		fprintf(file,"%s: 'string_array' argument is NULL\n",fname);
		return FAILURE;
	}
	if ( num_strings <= 0 ) {
		fprintf(file,
		"%s: Illegal number of string arguments passed (value = %d)\n",
			fname, num_strings);
		return FAILURE;
	}
	if ( *string_length <= 0 ) {
		fprintf(file,
"%s: No room in output buffer for string response (*string_length = %d)\n",
			fname, *string_length );
		return FAILURE;
	}

	if ( prompt == NULL ) {
		strlcpy( real_prompt, "", sizeof(real_prompt) );
	} else {
		strlcpy( real_prompt, prompt, sizeof( real_prompt ) );
	}

	output_buffer_length = *string_length;

	strlcat( real_prompt, "[", sizeof(real_prompt) );

#if 0
	fprintf( stderr, "real_prompt #1 = '%s'\n", real_prompt );
	fprintf( stderr, "output_buffer_length #1 = '%s'\n",
			output_buffer_length);
#endif

	for ( i = 0; i < num_strings; i++ ) {
		if ( i != 0 ) {
			strlcat( real_prompt, ",", sizeof(real_prompt) );
		}

		if ( string_array[i] == NULL ) {
			fprintf(file,
			"%s: 'string_array[%d]' pointer is NULL.\n", fname, i);

			return FAILURE;
		}
		strlcat( real_prompt, string_array[i], sizeof(real_prompt) );
	}
	strlcat( real_prompt, "] -> ", sizeof(real_prompt) );

#if 0
	fprintf( stderr, "real_prompt #2 = '%s'\n", real_prompt );
	fprintf( stderr, "default_string = '%s'\n",
			string_array[ default_string_number ] );
	fprintf( stderr, "output_buffer_length = %d\n",
			(int) output_buffer_length );
	fflush( stderr );
#endif

	valid_input = FALSE;

	while( valid_input == FALSE ) {
		*string_length = output_buffer_length;

		status = motor_get_string( file, real_prompt,
				string_array[ default_string_number ],
				string_length, selected_string );

		if ( status != SUCCESS )
			return status;

		length = (int) strlen( selected_string );

		for ( i = 0; i < num_strings; i++ ) {
			if ( length >= min_length_array[i] ) {

				status = strncmp( selected_string,
					string_array[i], length );

				if ( status == 0 ) {
					valid_input = TRUE;

					break;	/* Exit the for() loop. */
				}
			}
		}
		if ( valid_input == FALSE ) {
			fprintf( file,
	"Invalid response '%s'.  Please try again or hit ctrl-D to abort.\n",
				selected_string );
		} else {
			strlcpy( selected_string, string_array[i],
					output_buffer_length );
		}
	}

	*string_length = (int) strlen( selected_string );

	return SUCCESS;
}

