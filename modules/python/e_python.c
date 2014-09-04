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
	PyObject *py_main = NULL;
	PyObject *py_dict = NULL;
	PyObject *result = NULL;

	MX_DEBUG(-2,("%s invoked.", fname));

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

	/* Initialize the Python environment. */

	Py_SetProgramName( list_head->program_name );

	Py_Initialize();

	/* Try to load the Mp module. */

	py_main = PyImport_AddModule("__main__");

	if ( py_main == NULL ) {
		PyErr_Print();

		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The Python __main__ routine was not found." );
	}

	py_dict = PyModule_GetDict( py_main );

	if ( py_dict == NULL ) {
		PyErr_Print();

		return mx_error( MXE_UNKNOWN_ERROR, fname,
	    "The dictionary for the Python __main__ routine was not found.");
	}

	MX_DEBUG(-2,("%s: LD_LIBRARY_PATH = '%s'",
		fname, getenv("LD_LIBRARY_PATH") ));
	MX_DEBUG(-2,("%s: PYTHONPATH = '%s'",
		fname, getenv("PYTHONPATH") ));

	PyRun_SimpleString("import sys\nprint sys.path");

	result = PyRun_String( "import Mp",
			Py_single_input, py_dict, py_dict );

	if ( result == NULL ) {
		PyErr_Print();

		return mx_error( MXE_NOT_FOUND, fname,
		"Could not load the 'Mp' module." );
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

	if ( extension == (MX_EXTENSION *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EXTENSION pointer passed was NULL." );
	}
	if ( argc < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"argc (%d) is supposed to have a non-negative value.", argc );
	}

	MX_DEBUG(-2,("%s invoked", fname));

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

	return MX_SUCCESSFUL_RESULT;
}

