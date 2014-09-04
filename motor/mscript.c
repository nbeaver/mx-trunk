/*
 * Name:    mscript.c
 *
 * Purpose: Run an in-process script in a scripting language such as Python.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "motor.h"
#include "command.h"

#include "mx_module.h"

#define MXU_LANGUAGE_NAME_LENGTH	40

#define MXSCR_ARGV_OFFSET		3

#define MXSCR_PYTHON			1

static struct mx_script_language_type {
	char name[MXU_LANGUAGE_NAME_LENGTH+1];
	int type;
	MX_EXTENSION *extension;
} languages[] = {

{ "python", MXSCR_PYTHON },
};

static size_t num_languages = sizeof(languages) / sizeof(languages[0]);

int
motor_script_fn( int argc, char *argv[] )
{
#if 0
	static const char fname[] = "motor_script_fn()";
#endif

	MX_EXTENSION *extension = NULL;
	MX_EXTENSION_FUNCTION_LIST *flist = NULL;
	size_t i, j, length;
	char *language;
	struct mx_script_language_type *language_struct = NULL;
	int call_argc;
	void **call_argv = NULL;
	unsigned long mx_status_code;
	mx_status_type mx_status;

#if 0
	for ( i = 0; i < argc; i++ ) {
		MX_DEBUG(-2,("%s: argv[%d] = '%s'", fname, (int) i, argv[i]));
	}
#endif

	/* Figure out which scripting language is requested.  This can be
	 * determined from the argument argv[2].
	 */

	language = argv[2];

	length = strlen(language);

	if ( length == 0 ) {
		fprintf( output, "No scripting language specified!\n" );
		return FAILURE;
	}

	language_struct = NULL;

	for ( i = 0; i < num_languages; i++ ) {
#if 0
		MX_DEBUG(-2,("%s: languages[%d] = '%s'",
			fname, (int) i, languages[i].name));
#endif

		if ( strncmp( language, languages[i].name, length ) == 0 ) {
			language_struct = &languages[i];
			break;
		}
	}

	if ( language_struct == NULL ) {
		fprintf( output, "Scripting language '%s' is not supported.\n",
			language );
		return FAILURE;
	}

#if 0
	MX_DEBUG(-2,("%s: language = '%s'", fname, language_struct->name));
#endif

	/* If we have not yet located the extension for this language,
	 * then search for the extension.
	 */

	if ( language_struct->extension == (MX_EXTENSION *) NULL ) {
		mx_status = mx_get_extension( language_struct->name,
						motor_record_list,
						&extension );

		mx_status_code = mx_status.code & ( ~MXE_QUIET );

		switch( mx_status_code ) {
		case MXE_SUCCESS:
			/* Successfully found the extension. */
			break;
		default:
			fprintf( output, "Error: The attempt to find "
			"MX script extension '%s' failed with error code %lu\n",
			language_struct->name, mx_status_code );

			return FAILURE;
		}

#if 0
		MX_DEBUG(-2,("%s: Extension found = '%s'",
			fname, extension->name));
#endif

		/* Prepare to invoke the init function for the extension. */

		flist = extension->extension_function_list;

		if ( flist == NULL ) {
			fprintf( output, "Error: Extension '%s' does not "
			"have an extension function list.\n", extension->name );

			return FAILURE;
		}

		if ( flist->init == NULL ) {
			fprintf( output, "Error: Extension '%s' does not "
			"have an init() function.\n", extension->name );

			return FAILURE;
		}

		/* Invoke the init() function. */

		mx_status = ( flist->init )( extension );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	}

	/* Invoke the call function for this extension. */

	call_argc = argc - MXSCR_ARGV_OFFSET;

#if 0
	MX_DEBUG(-2,("%s: argc = %d, call_argc = %d",
		fname, argc, call_argc));
#endif

	call_argv = malloc( call_argc * sizeof(void *) );

	if ( call_argv == NULL ) {
		fprintf( output, "Error: The attempt to allocate a %d "
		"element array of void pointers to pass "
		"to the extension failed.\n", call_argc );
	}

	for ( j = 0; j < call_argc; j++ ) {
		call_argv[j] = argv[MXSCR_ARGV_OFFSET + j];

#if 0
		MX_DEBUG(-2,("%s: call_argv[%d] = '%s'",
			fname, (int) j, (char *) call_argv[j]));
#endif
	}

	mx_status = ( flist->call )( extension, call_argc, call_argv );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	return SUCCESS;
}

