/*
 * Name:    e_python.c
 *
 * Purpose: MX Python extension.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "Python.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_module.h"
#include "e_python.h"

MX_EXTENSION_FUNCTION_LIST mxext_python_extension_function_list = {
	mxext_python_init,
	mxext_python_call
};

/*------*/

MX_EXPORT mx_status_type
mxext_python_init( MX_EXTENSION *extension )
{
	static const char fname[] = "mxext_python_init()";

	MX_RECORD *mx_database = NULL;
	MX_LIST_HEAD *list_head = NULL;
	MX_DYNAMIC_LIBRARY *main_executable = NULL;
	void *motor_record_list_ptr = NULL;
	mx_status_type mx_status;

	MX_PYTHON_EXTENSION_PRIVATE *py_ext = NULL;
	PyObject *result = NULL;

	mx_database = extension->record_list;

	if ( mx_database == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for extension '%s' is NULL.",
			extension->name );
	}

	list_head = mx_get_record_list_head_struct( mx_database );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for the MX database %p "
		"used by MX extension '%s' is NULL.",
			mx_database, extension->name );
	}

	py_ext = (MX_PYTHON_EXTENSION_PRIVATE *)
			malloc( sizeof(MX_PYTHON_EXTENSION_PRIVATE) );

	if ( py_ext == (MX_PYTHON_EXTENSION_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_PYTHON_EXTENSION_PRIVATE structure." );
	}

	extension->ext_private = py_ext;

	/* Initialize the Python environment. */

	Py_SetProgramName( list_head->program_name );

	Py_Initialize();

	/* Save some Python objects we may need later. */

	py_ext->py_main = PyImport_AddModule("__main__");

	if ( py_ext->py_main == NULL ) {
		PyErr_Print();

		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The Python __main__ routine was not found." );
	}

	py_ext->py_dict = PyModule_GetDict( py_ext->py_main );

	if ( py_ext->py_dict == NULL ) {
		PyErr_Print();

		return mx_error( MXE_UNKNOWN_ERROR, fname,
	    "The dictionary for the Python __main__ routine was not found.");
	}

	/* Try to load the Mp module. */

	result = PyRun_String( "import Mp",
			Py_single_input, py_ext->py_dict, py_ext->py_dict );

	if ( result == NULL ) {
		PyErr_Print();

		return mx_error( MXE_NOT_FOUND, fname,
		"Could not load the 'Mp' module." );
	}

	/* Are we running in the context of the 'mxmotor' process?
	 *
	 * We find out by searching for the 'motor_record_list' pointer
	 * in the executable that contains main().  We can get an
	 * MX_DYNAMIC_LIBRARY handle that refers to the main() executable
	 * by passing NULL as the filename in a call to the function
	 * mx_dynamic_library_open().
	 */

	mx_status = mx_dynamic_library_open( NULL, &main_executable );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_record_list_ptr =
		mx_dynamic_library_get_symbol_pointer( main_executable,
							"motor_record_list" );

	if ( motor_record_list_ptr == NULL ) {

		/* We are _not_ running in the 'mxmotor' process,
		 * so we are done now.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, we _are_ running in the 'mxmotor' process.
	 *
	 * In that case, try to load the 'MpMtr' module.
	 */

	result = PyRun_String( "import MpMtr",
			Py_single_input, py_ext->py_dict, py_ext->py_dict );

	if ( result == NULL ) {
		PyErr_Print();

		mx_warning( "Could not load the 'MpMtr' Python module "
			"for mxmotor.  We will continue without it." );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxext_python_call( MX_EXTENSION *extension,
			int argc,
			void **argv )
{
	static const char fname[] = "mxext_python_call()";

	MX_RECORD *mx_database = NULL;
	int i;
	char *script_filename = NULL;
	char del_command[80];
	char execfile_command[MXU_FILENAME_LENGTH+20];
	char temp_buffer[500];
	char main_command[1000];

	MX_PYTHON_EXTENSION_PRIVATE *py_ext = NULL;
	PyObject *result = NULL;
	PyObject *main_object = NULL;

	if ( extension == (MX_EXTENSION *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EXTENSION pointer passed was NULL." );
	}
	if ( argc < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"argc (%d) is supposed to have a non-negative value.", argc );
	}

	py_ext = extension->ext_private;

	if ( py_ext == (MX_PYTHON_EXTENSION_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PYTHON_EXTENSION_PRIVATE pointer for extension '%s' "
		"is NULL.", extension->name );
	}

	mx_database = extension->record_list;

	if ( mx_database == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for extension '%s' is NULL.",
			extension->name );
	}

	for ( i = 0; i < argc; i++ ) {
		MX_DEBUG(-2,("%s: argv[%d] = '%s'",
		fname, i, (char *) argv[i]));
	}

	/* Get rid of any preexisting main() function, since
	 * the script we are invoking may provide one.
	 */

	strlcpy( del_command,
			"try:\n"
			"  del main\n"
			"except:\n"
			"  pass\n",
		sizeof(del_command) );

	result = PyRun_String( del_command,
			Py_single_input, py_ext->py_dict, py_ext->py_dict );

	if ( result == NULL ) {
		PyErr_Print();

		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"Running the script '%s' failed.", (char *) argv[0] );
	}

	/* The first argument in argv should be the name of the external
	 * python script to run.
	 */

	script_filename = (char *) argv[0];

	snprintf( execfile_command, sizeof(execfile_command),
			"execfile('%s')", script_filename );

	result = PyRun_String( execfile_command,
			Py_single_input, py_ext->py_dict, py_ext->py_dict );

	if ( result == NULL ) {
		PyErr_Print();

		return mx_error( MXE_NOT_FOUND, fname,
		"Running the script '%s' failed.", script_filename );
	}

	/* Did the script create a main() function? */

	main_object = PyDict_GetItemString( py_ext->py_dict, "main" );

	if ( main_object == NULL ) {
		/* No it did not, so return. */

		return MX_SUCCESSFUL_RESULT;
	}

	MX_DEBUG(-2,("%s: main_object = %p", fname, main_object ));

	/* Is the main object a function? */

	if ( PyFunction_Check(main_object) == FALSE ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "The script '%s' contains a 'main' object which is not a function.",
			script_filename );
	}

	/*---------------------------------------------------------------*/

	/*---------------------------------------------------------------*/

	/* Construct a string representation of a Python command that
	 * invokes the script's 'main()' function with the MX database
	 * object as the first argument.  The contents of the 'argv'
	 * array is represented as a Python list of strings that is
	 * passed to the main() function as its second argument.
	 */

	strlcpy( main_command, "main( mx_database, ["
				, sizeof(main_command) );

	if ( argc > 1 ) {
		snprintf( temp_buffer, sizeof(temp_buffer),
			" \"%s\"", (char *) argv[1] );

		strlcat( main_command, temp_buffer, sizeof(main_command) );
	}

	for ( i = 2; i < argc; i++ ) {
		snprintf( temp_buffer, sizeof(temp_buffer),
			", \"%s\"", (char *) argv[i] );

		strlcat( main_command, temp_buffer, sizeof(main_command) );
	}

	strlcat( main_command, " ] )", sizeof(main_command) );

	MX_DEBUG(-2,("%s: main_command = '%s'", fname, main_command));

	result = PyRun_String( main_command,
			Py_single_input, py_ext->py_dict, py_ext->py_dict );

	if ( result == NULL ) {
		PyErr_Print();

		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"Executing the main() function from the script '%s' failed.",
			(char *) argv[0] );
	}

	return MX_SUCCESSFUL_RESULT;
}

