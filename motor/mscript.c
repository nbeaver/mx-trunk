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

#include <stdio.h>

#include "motor.h"
#include "command.h"

#include "mx_module.h"

#define MXU_LANGUAGE_NAME_LENGTH	40

#define MXSCR_ARGV_OFFSET		3

int
motor_script_fn( int argc, char *argv[] )
{
#if 1
	static const char fname[] = "motor_script_fn()";
#endif

	MX_EXTENSION *extension = NULL;
	MX_EXTENSION_FUNCTION_LIST *flist = NULL;
	int i, call_argc;
	char **call_argv = NULL;
	mx_status_type mx_status;

	if ( argc < 3 ) {
		fprintf( output, "%s: Command is too short.\n", fname );
		return FAILURE;
	}

	/* Find the extension that matches the requested extension name. */

#if 1
	MX_DEBUG(-2,("%s invoked for extension '%s'", fname, argv[2]));
#endif

#if 1
	for ( i = 0; i < argc; i++ ) {
		MX_DEBUG(-2,("%s: argv[%d] = '%s'", fname, i, argv[i]));
	}
#endif

	mx_status = mx_get_extension( argv[2],
					motor_record_list,
					&extension );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	flist = extension->extension_function_list;

	/* Invoke the call function for this extension. */

	call_argc = argc - MXSCR_ARGV_OFFSET;

	call_argv = argv + MXSCR_ARGV_OFFSET;

	mx_status = ( flist->call )( extension, call_argc, (void **)call_argv );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	return SUCCESS;
}

