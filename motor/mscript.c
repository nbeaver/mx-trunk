/*
 * Name:    mscript.c
 *
 * Purpose: Run an in-process script in a scripting language such as Python.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2014-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DEBUG_MSCRIPT	FALSE

#include <stdio.h>

#include "motor.h"
#include "command.h"

#include "mx_module.h"

#define MXU_LANGUAGE_NAME_LENGTH			40

#define MXSCR_ARGV_OFFSET_USING_EXTENSION_NAME		3
#define MXSCR_ARGV_OFFSET_USING_DEFAULT_EXTENSION	2

int
motor_script_fn( int argc, char *argv[] )
{
	static const char fname[] = "motor_script_fn()";

	MX_EXTENSION *extension = NULL;
	MX_EXTENSION_FUNCTION_LIST *flist = NULL;
	int call_argc;
	char **call_argv = NULL;
	mx_status_type mx_status;

#if DEBUG_MSCRIPT
	int i;
#endif

	if ( argc < 3 ) {
		fprintf( output, "%s: Command is too short.\n", fname );
		return FAILURE;
	}

	/* Find the extension that matches the requested extension name. */

#if DEBUG_MSCRIPT
	MX_DEBUG(-2,("%s invoked for extension '%s'", fname, argv[2]));
#endif

#if DEBUG_MSCRIPT
	for ( i = 0; i < argc; i++ ) {
		MX_DEBUG(-2,("%s: argv[%d] = '%s'", fname, i, argv[i]));
	}
#endif

	/* See if a script extension is available with the name
	 * specified in argv[2].
	 */

	mx_status = mx_get_extension( argv[2],
					motor_record_list,
					&extension );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		/* The extension was found. */

#if DEBUG_MSCRIPT
		MX_DEBUG(-2,("%s: MXE_SUCCESS", fname));
#endif

		call_argc = argc - MXSCR_ARGV_OFFSET_USING_EXTENSION_NAME;
		call_argv = argv + MXSCR_ARGV_OFFSET_USING_EXTENSION_NAME;
		break;

	case MXE_NOT_FOUND:
		/* No extension was found with a name matching the string
		 * in argv[2].  In this case, we assume that argv[2] is
		 * actually the name of the script to run, so we look for
		 * a default script extension to run the script.
		 */

#if DEBUG_MSCRIPT
		MX_DEBUG(-2,("%s: MXE_NOT_FOUND", fname));
#endif

		extension = mx_get_default_script_extension();

		if ( extension == (MX_EXTENSION *) NULL ) {
			fprintf( output,
		    "Error: No script extension was found to run scripts.\n" );
			return FAILURE;
		}

		call_argc = argc - MXSCR_ARGV_OFFSET_USING_DEFAULT_EXTENSION;
		call_argv = argv + MXSCR_ARGV_OFFSET_USING_DEFAULT_EXTENSION;
		break;

	default:
		/* Some other error occurred. */

#if DEBUG_MSCRIPT
		MX_DEBUG(-2,("%s: MXE --- DEFAULT", fname));
#endif

		return FAILURE;
		break;
	}

	/*--------------------------------------------------------------*/

	flist = extension->extension_function_list;

	if ( flist == NULL ) {
		fprintf( output,
	"Error: MX extension '%s' does not have an extension function list!\n"
	"       This means that there is an error in the extension.\n",
			extension->name );

		return FAILURE;
	}

#if DEBUG_MSCRIPT
	for ( i = 0; i < call_argc; i++ ) {
		MX_DEBUG(-2,("%s: call_argv[%d] = '%s'",
			fname, i, call_argv[i]));
	}
#endif

	/* Invoke the call function for this extension. */

	mx_status = ( flist->call )( extension, call_argc, (void **)call_argv );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	return SUCCESS;
}

