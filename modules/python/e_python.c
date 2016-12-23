/*
 * Name:    e_python.c
 *
 * Purpose: MX Python extension.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014-2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define PYTHON_MODULE_DEBUG_INITIALIZE	TRUE

#define PYTHON_MODULE_DEBUG_FINALIZE	TRUE

#define PYTHON_MODULE_DEBUG_CAPSULE	TRUE

#define PYTHON_MODULE_DEBUG_CALL	TRUE

#include "Python.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_unistd.h"
#include "mx_module.h"
#include "e_python.h"

MX_EXTENSION_FUNCTION_LIST mxext_python_extension_function_list = {
	mxext_python_initialize,
	mxext_python_finalize,
	mxext_python_call
};

/*------*/

static mx_status_type
mxext_python_print_debug( PyObject *object,
			const char *calling_fname,
			int flags )
{
	static const char fname[] = "mxext_python_print_debug()";

	PyObject *object_description = NULL;
	char *object_string = NULL;

	if ( object == (PyObject *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
		"%s was called with object == NULL", fname );
	}

	if ( flags & Py_PRINT_RAW ) {
		object_description = PyObject_Str( object );
	} else {
		object_description = PyObject_Repr( object );
	}

	if ( object_description == (PyObject *) NULL ) {
		return mx_error( MXE_FUNCTION_FAILED, calling_fname,
	    "%s:The attempt to get a description of Python object %p failed.",
			fname, object );
	}

	object_string = PyString_AsString( object_description );

	if ( object_string == (char *) NULL ) {
		return mx_error( MXE_FUNCTION_FAILED, calling_fname,
		"%s: The attempt to convert the description %p "
		"of Python object %p failed.",
			fname, object_description, object_string );
	}

	MX_DEBUG(-2,("%s", object_string));

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

static mx_status_type
mxext_python_find_record_list_object(
				MX_PYTHON_EXTENSION_PRIVATE *py_ext,
				PyObject *mx_database_capsule,
				PyObject **record_list_class_instance )
{
	static const char fname[] = "mxext_python_find_record_list_object()";

#if PYTHON_MODULE_DEBUG_CAPSULE
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	fprintf( stderr, "The py_ext->py_main object is " );
	mxext_python_print_debug( py_ext->py_main, fname, 0 );
	fprintf( stderr, "\n" );

#if PYTHON_MODULE_DEBUG_CAPSULE
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

extern void
mxext_python_show_exception_type( PyObject *exception_type );

void
mxext_python_show_exception_type( PyObject *exception_type )
{
	static const char fname[] = "mxext_python_show_exception_type()";

	MX_DEBUG(-2,("%s: exception_type = %p", fname, exception_type));

	if ( exception_type == PyExc_BaseException ) {
		MX_DEBUG(-2,("%s: PyExc_BaseException", fname));
	} else
	if ( exception_type == PyExc_Exception ) {
		MX_DEBUG(-2,("%s: PyExc_Exception", fname));
	} else
	if ( exception_type == PyExc_StandardError ) {
		MX_DEBUG(-2,("%s: PyExc_StandardError", fname));
	} else
	if ( exception_type == PyExc_ArithmeticError ) {
		MX_DEBUG(-2,("%s: PyExc_ArithmeticError", fname));
	} else
	if ( exception_type == PyExc_LookupError ) {
		MX_DEBUG(-2,("%s: PyExc_LookupError", fname));
	} else
	if ( exception_type == PyExc_AssertionError ) {
		MX_DEBUG(-2,("%s: PyExc_AssertionError", fname));
	} else
	if ( exception_type == PyExc_AttributeError ) {
		MX_DEBUG(-2,("%s: PyExc_AttributeError", fname));
	} else
	if ( exception_type == PyExc_EOFError ) {
		MX_DEBUG(-2,("%s: PyExc_EOFError", fname));
	} else
	if ( exception_type == PyExc_EnvironmentError ) {
		MX_DEBUG(-2,("%s: PyExc_EnvironmentError", fname));
	} else
	if ( exception_type == PyExc_FloatingPointError ) {
		MX_DEBUG(-2,("%s: PyExc_FloatingPointError", fname));
	} else
	if ( exception_type == PyExc_IOError ) {
		MX_DEBUG(-2,("%s: PyExc_IOError", fname));
	} else
	if ( exception_type == PyExc_ImportError ) {
		MX_DEBUG(-2,("%s: PyExc_ImportError", fname));
	} else
	if ( exception_type == PyExc_IndexError ) {
		MX_DEBUG(-2,("%s: PyExc_IndexError", fname));
	} else
	if ( exception_type == PyExc_KeyError ) {
		MX_DEBUG(-2,("%s: PyExc_KeyError", fname));
	} else
	if ( exception_type == PyExc_KeyboardInterrupt ) {
		MX_DEBUG(-2,("%s: PyExc_KeyboardInterrupt", fname));
	} else
	if ( exception_type == PyExc_MemoryError ) {
		MX_DEBUG(-2,("%s: PyExc_MemoryError", fname));
	} else
	if ( exception_type == PyExc_NameError ) {
		MX_DEBUG(-2,("%s: PyExc_NameError", fname));
	} else
	if ( exception_type == PyExc_NotImplementedError ) {
		MX_DEBUG(-2,("%s: PyExc_NotImplementedError", fname));
	} else
	if ( exception_type == PyExc_OSError ) {
		MX_DEBUG(-2,("%s: PyExc_OSError", fname));
	} else
	if ( exception_type == PyExc_OverflowError ) {
		MX_DEBUG(-2,("%s: PyExc_OverflowError", fname));
	} else
	if ( exception_type == PyExc_ReferenceError ) {
		MX_DEBUG(-2,("%s: PyExc_ReferenceError", fname));
	} else
	if ( exception_type == PyExc_RuntimeError ) {
		MX_DEBUG(-2,("%s: PyExc_RuntimeError", fname));
	} else
	if ( exception_type == PyExc_SyntaxError ) {
		MX_DEBUG(-2,("%s: PyExc_SyntaxError", fname));
	} else
	if ( exception_type == PyExc_SystemError ) {
		MX_DEBUG(-2,("%s: PyExc_SystemError", fname));
	} else
	if ( exception_type == PyExc_SystemExit ) {
		MX_DEBUG(-2,("%s: PyExc_SystemExit", fname));
	} else
	if ( exception_type == PyExc_TypeError ) {
		MX_DEBUG(-2,("%s: PyExc_TypeError", fname));
	} else
	if ( exception_type == PyExc_ValueError ) {
		MX_DEBUG(-2,("%s: PyExc_ValueError", fname));
	} else

#if defined(OS_WIN32)
	if ( exception_type == PyExc_WindowsError ) {
		MX_DEBUG(-2,("%s: PyExc_WindowsError", fname));
	} else
#endif

	if ( exception_type == PyExc_ZeroDivisionError ) {
		MX_DEBUG(-2,("%s: PyExc_ZeroDivisionError", fname));
	} else {
		MX_DEBUG(-2,("%s: unrecognized error = %p",
			fname, exception_type));
	}
}

/*------*/

static mx_status_type
mxext_python_create_record_list_object(
				MX_PYTHON_EXTENSION_PRIVATE *py_ext,
				PyObject *mx_database_capsule,
				PyObject **record_list_class_instance )
{
	static const char fname[] = "mxext_python_create_record_list_object()";

	PyObject *record_list_class_object = NULL;
	PyObject *record_list_constructor_arguments = NULL;

#if PYTHON_MODULE_DEBUG_CAPSULE
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	py_ext->mx_database_initialized_elsewhere = FALSE;

	/* Get the callable object for the Mp.RecordList() constructor
	 * using Py_eval_input to tell the interpreter that we want
	 * to get the class object, rather than getting the result 
	 * of running the string.
	 */

	record_list_class_object = PyRun_String( "Mp.RecordList",
			Py_eval_input, py_ext->py_dict, py_ext->py_dict );

	if ( record_list_class_object == NULL ) {
		PyErr_Print();

		return mx_error( MXE_NOT_FOUND, fname,
		"Could not get the Mp.RecordList class object." );
	}

#if PYTHON_MODULE_DEBUG_CAPSULE
	fprintf( stderr, "The class object is " );

	mxext_python_print_debug( record_list_class_object, fname, 0 );

	fprintf( stderr, "\n" );
#endif

	/* Create an Mp.RecordList object that refers to the already 
	 * existing MX record list object in the C main program.
	 *
	 * First, we build the arguments for the constructor.  We will
	 * not use keyword arguments here.
	 */

	record_list_constructor_arguments = Py_BuildValue( "(O)",
						mx_database_capsule );

	if ( record_list_constructor_arguments == NULL ) {
		PyErr_Print();

		return mx_error( MXE_NOT_FOUND, fname,
		"Could not build the arguments for the constructor "
		"of the Mp.RecordList class." );
	}

	/* Now we invoke the constructor to get a class instance. */

	*record_list_class_instance = PyInstance_New( record_list_class_object,
					record_list_constructor_arguments,
					NULL );

	if ( (*record_list_class_instance) == NULL ) {
		PyErr_Print();

		return mx_error( MXE_NOT_FOUND, fname,
		"Could not create an instance of the Mp.RecordList class." );
	}

	/* FIXME: We have found in the Mp module that it is necessary to
	 * increment the reference count here to avoid a segmentation fault.
	 * So we must do that here too.
	 */

	Py_INCREF( *record_list_class_instance );

#if PYTHON_MODULE_DEBUG_CAPSULE
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

/* NOTE:  In order to understand the calls to PyRun_String() below, it is
 *        very important to know the difference between the following
 *        start symbols:
 *
 *          Py_single_input - used to run a Python statement.
 *          Py_eval_input   - used to evaluate a Python expression.
 */

MX_EXPORT mx_status_type
mxext_python_initialize( MX_EXTENSION *extension )
{
	static const char fname[] = "mxext_python_initialize()";

	MX_RECORD *mx_database = NULL;
	MX_LIST_HEAD *list_head = NULL;
	MX_EXTENSION *default_extension = NULL;
	mx_status_type mx_status;

	MX_PYTHON_EXTENSION_PRIVATE *py_ext = NULL;
	PyObject *mx_database_capsule = NULL;
	PyObject *record_list_class_instance = NULL;
	PyObject *result_of_mp_detect = NULL;
	PyObject *result = NULL;
	int python_status;

	mx_database = extension->record_list;

	if ( mx_database == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for extension '%s' is NULL.",
			extension->name );
	}

	/*---------------------------------------------------------------*/

	list_head = mx_get_record_list_head_struct( mx_database );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for the MX database %p "
		"used by MX extension '%s' is NULL.",
			mx_database, extension->name );
	}

	py_ext = (MX_PYTHON_EXTENSION_PRIVATE *)
			calloc( 1, sizeof(MX_PYTHON_EXTENSION_PRIVATE) );

	if ( py_ext == (MX_PYTHON_EXTENSION_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_PYTHON_EXTENSION_PRIVATE structure." );
	}

	extension->ext_private = py_ext;

	/* Mark this extension as being for a script language. */

	extension->extension_flags |= MXF_EXT_HAS_SCRIPT_LANGUAGE;

	/*---------------------------------------------------------------*/

	/* Has Python already been initialized in this process?
	 *
	 * For example, if an MX 'mpscript' based program loads an
	 * MX database that has a '!load python.mxo' line in it,
	 * then 'mpscript' has already initialized Python and
	 * we must not reinitialize it.
	 */

	if ( Py_IsInitialized() ) {
		py_ext->python_initialized_elsewhere = TRUE;
	} else {
		py_ext->python_initialized_elsewhere = FALSE;

		/* If not, then we need to initialize the Python environment. */

		Py_SetProgramName( list_head->program_name );

		Py_Initialize();
	}

	MX_DEBUG(-2,("%s: python_initialized_elsewhere = %d",
		fname, py_ext->python_initialized_elsewhere));

	/* Save some Python objects we may need later. */

	py_ext->py_main = PyImport_AddModule("__main__");

	if ( py_ext->py_main == NULL ) {
		PyErr_Print();

		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The Python __main__ routine was not found." );
	}

	MX_DEBUG(-2,("%s: py_ext->py_main = %p", fname, py_ext->py_main));

	py_ext->py_dict = PyModule_GetDict( py_ext->py_main );

	if ( py_ext->py_dict == NULL ) {
		PyErr_Print();

		return mx_error( MXE_UNKNOWN_ERROR, fname,
	    "The dictionary for the Python __main__ routine was not found.");
	}

	MX_DEBUG(-2,("%s: py_ext->py_dict = %p", fname, py_ext->py_dict));

	/* Is the Mp module already loaded? */

	result_of_mp_detect = PyRun_String( "Mp",
			Py_single_input, py_ext->py_dict, py_ext->py_dict );

	if ( PyErr_Occurred() ) {
		MX_DEBUG(-2,("%s: PyRun_String(\"Mp\",...) caused "
		"an exception.  This probably means that Mp is not loaded.",
			fname ));

		PyErr_Clear();

		py_ext->mp_initialized_elsewhere = FALSE;
	} else {
		Py_DECREF( result_of_mp_detect );
		
		py_ext->mp_initialized_elsewhere = TRUE;
	}

	MX_DEBUG(-2,("%s: py_ext->mp_initialized_elsewhere = %d",
		fname, py_ext->mp_initialized_elsewhere));

	/* Try to load the Mp module. */

	if ( py_ext->mp_initialized_elsewhere == FALSE ) {
		result = PyRun_String( "import Mp",
			Py_single_input, py_ext->py_dict, py_ext->py_dict );

		if ( result == NULL ) {
			PyErr_Print();

			return mx_error( MXE_NOT_FOUND, fname,
			"Could not load the 'Mp' module." );
		}

		MX_DEBUG(-2,("%s: Mp module has been loaded.", fname));
	} else {
		MX_DEBUG(-2,
			("%s: Mp module does not need to be loaded.", fname));
	}

	/*----------------------------------------------------------------*/

	/* We need to put the mx_database pointer into a Python object,
	 * so that we can create the Python MX database object.  This is
	 * done using a PyCapsule object.  On Python 2.6 and before, we
	 * use PyCObject objects instead.
	 */

#if PYTHON_MODULE_DEBUG_INITIALIZE
	MX_DEBUG(-2,("%s: mx_database = %p", fname, mx_database));
#endif

#if ( PY_VERSION_HEX >= 0x02070000 )
	mx_database_capsule = PyCapsule_New( mx_database, NULL, NULL );
#else
	mx_database_capsule = PyCObject_FromVoidPtr( mx_database, NULL );
#endif

	if ( mx_database_capsule == NULL ) {
		PyErr_Print();

		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"Unable to put the mx_database pointer into a PyCapsule.  "
		"You are not expected to know what this means, so please "
		"report it to the MX developer (William Lavender)." );
	}

#if PYTHON_MODULE_DEBUG_CAPSULE
	fprintf( stderr, "mx_database_capsule = " );

	mxext_python_print_debug( mx_database_capsule, fname, 0 );

	fprintf( stderr, "\n" );
#endif

	/* See if there already is an Mp.RecordList instance available. */

	mx_status = mxext_python_find_record_list_object(
						py_ext,
						mx_database_capsule,
						&record_list_class_instance );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: existing record_class_instance = %p",
		fname, record_list_class_instance));

	/* If necessary, create a new MX database instance. */

	if ( py_ext->mx_database_initialized_elsewhere == FALSE ) {

		mx_status = mxext_python_create_record_list_object(
						py_ext,
						mx_database_capsule,
						&record_list_class_instance );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Save the Python wrapper for the MX database. */

	py_ext->py_record_list = record_list_class_instance;

#if PYTHON_MODULE_DEBUG_CAPSULE
	fprintf( stderr, "The instance object is " );

	mxext_python_print_debug( py_ext->py_record_list, fname, 0 );

	fprintf( stderr, "\n" );
#endif

	/* Add 'mx_database' to the global dictionary so that external
	 * scripts can find it.
	 */

	python_status = PyObject_SetAttrString( py_ext->py_main,
						"mx_database",
						py_ext->py_record_list );

	if ( python_status == -1 ) {
		PyErr_Print();

		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"Could not add 'mx_database' to the main script's dictionary.");
	}

	/*----------------------------------------------------------------*/

	/* If currently there is no default script extension, then set
	 * 'python' to be the default script extension.
	 */

	default_extension = mx_get_default_script_extension();

	if ( default_extension == (MX_EXTENSION *) NULL ) {
		mx_set_default_script_extension( extension );
	}

#if PYTHON_MODULE_DEBUG_INITIALIZE
	default_extension = mx_get_default_script_extension();

	MX_DEBUG(-2,("%s: default script extension = '%s'",
		fname, default_extension->name ));
#endif

	/*----------------------------------------------------------------*/

	/* Are we running in the context of the 'mxmotor' process?
	 *
	 * 'mxmotor' saves its name in 'list_head->program_name',
	 * so we look there.
	 */

#if PYTHON_MODULE_DEBUG_INITIALIZE
	MX_DEBUG(-2,("%s: Existing list_head->program_name = '%s'",
		fname, list_head->program_name));
#endif

	if ( strcmp( list_head->program_name, "mxmotor" ) != 0 ) {

		/* We are _not_ running in the 'mxmotor' process,
		 * so we are done now.
		 */

#if PYTHON_MODULE_DEBUG_INITIALIZE
		MX_DEBUG(-2,
		("%s: This process is NOT 'mxmotor'.  Returning now.", fname ));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, we _are_ running in the 'mxmotor' process.
	 *
	 * In that case, try to load the 'MpMtr' module.
	 */

#if PYTHON_MODULE_DEBUG_INITIALIZE
	MX_DEBUG(-2,("%s: Attempting to import MpMtr.", fname));
#endif

#if 1
	result = PyRun_String( "import MpMtr",
			Py_single_input, py_ext->py_dict, py_ext->py_dict );

	if ( result == NULL ) {
		PyErr_Print();

		mx_warning( "Could not load the 'MpMtr' Python module "
			"for mxmotor.  We will continue without it." );
	} else {
#if PYTHON_MODULE_DEBUG_INITIALIZE
		MX_DEBUG(-2,("%s: MpMtr imported.", fname));
#endif
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxext_python_finalize( MX_EXTENSION *extension )
{
	static const char fname[] = "mxext_python_finalize()";

	MX_PYTHON_EXTENSION_PRIVATE *py_ext = NULL;
	PyObject *py_record_list = NULL;
	unsigned long flags;

	if ( extension == (MX_EXTENSION *) NULL )
		return MX_SUCCESSFUL_RESULT;

#if PYTHON_MODULE_DEBUG_FINALIZE
	MX_DEBUG(-2,("%s invoked for extension '%s'", fname, extension->name));
#endif

	flags = extension->extension_flags;

	if ( flags & MXF_EXT_IS_DISABLED )
		return MX_SUCCESSFUL_RESULT;

	py_ext = (MX_PYTHON_EXTENSION_PRIVATE *) extension->ext_private;

	if ( py_ext == (MX_PYTHON_EXTENSION_PRIVATE *) NULL )
		return MX_SUCCESSFUL_RESULT;

	py_record_list = py_ext->py_record_list;

	if ( py_record_list == (PyObject *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The py_record_list object for extension '%s' is NULL.",
			extension->name );
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
#if 0
	char del_command[80];
#endif
	char execfile_command[MXU_FILENAME_LENGTH+20];
#if 0
	char temp_buffer[500];
	char main_command[1000];
#endif
	char full_filename[MXU_FILENAME_LENGTH+1];
	unsigned long script_flags;
	int match_found;
#if 0
	int python_status;
#endif

	MX_PYTHON_EXTENSION_PRIVATE *py_ext = NULL;
	PyObject *result = NULL;
	PyObject *main_function = NULL;

	PyObject *main_tuple = NULL;
	PyObject *argv_tuple = NULL;
	PyObject *argv_item = NULL;
	PyObject *main_result = NULL;

	mx_status_type mx_status;

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

#if PYTHON_MODULE_DEBUG_CALL
	for ( i = 0; i < argc; i++ ) {
		MX_DEBUG(-2,("%s: argv[%d] = '%s'",
		fname, i, (char *) argv[i]));
	}
#endif

	/*---------------------------------------------------------------*/

	/* The first argument in argv should be the name of the external
	 * python script to run.
	 */

	script_filename = (char *) argv[0];

	/* Look for the requested script in the PATH environment variable. */

	script_flags = MXF_FPATH_TRY_WITHOUT_EXTENSION
			| MXF_FPATH_LOOK_IN_CURRENT_DIRECTORY;

	mx_status = mx_find_file_in_path( script_filename,
					full_filename,
					sizeof(full_filename),
					"PATH", ".py",
					F_OK, script_flags,
					&match_found );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( match_found == FALSE ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"The Python script '%s' was not found.",
			script_filename );
	}

	/*---------------------------------------------------------------*/

	/* Run the script that we found. */

	snprintf( execfile_command, sizeof(execfile_command),
			"execfile('%s')", full_filename );

#if PYTHON_MODULE_DEBUG_CALL
	MX_DEBUG(-2,("%s: execfile_command = '%s'", fname, execfile_command));
#endif

	result = PyRun_String( execfile_command,
			Py_single_input, py_ext->py_dict, py_ext->py_dict );

	MX_DEBUG(-2,("%s: after execfile()", fname));

	if ( result == NULL ) {
		PyErr_Print();

		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Running the script '%s' failed.", script_filename );
	} else {
		Py_DECREF( result );
	}

	full_filename[0] = '\0';

	/*---------------------------------------------------------------*/

	/* Did the script create a main() function? */

	main_function = PyDict_GetItemString( py_ext->py_dict, "main" );

	if ( main_function == NULL ) {
		/* No it did not, so return. */

#if PYTHON_MODULE_DEBUG_CALL
		MX_DEBUG(-2,
		("%s: No main() function found.  Returning.", fname));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	/* Is the main object a function? */

	if ( PyFunction_Check(main_function) == FALSE ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "The script '%s' contains a 'main' object which is not a function.",
			script_filename );
	}

	/*---------------------------------------------------------------*/

	/* Create a tuple to contain the argv tuple for the main()
	 * function call.
	 */

	argv_tuple = PyTuple_New( argc - 1 );

	if ( argv_tuple == NULL ) {
		MX_DEBUG(-2,
		("%s: PyTuple_New() for argv_tuple failed.", fname));

		PyErr_Print();

		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"Creating argv_tuple failed." );
	}

	/* Fill the tuple with strings from the argv array. */

	for ( i = 1; i < argc; i++ ) {
		MX_DEBUG(-2,
		("%s: tuple[%d] = '%s'", fname, i-1, (char *)argv[i]));

		argv_item = PyString_FromString( argv[i] );

		if ( argv_item == NULL ) {
			MX_DEBUG(-2,
			("%s: PyString_FromString[ ... %d ... } failed.",
				fname, i ));

			return mx_error( MXE_UNKNOWN_ERROR,
				fname, "PyString_FromString() failed." );
		}

		PyTuple_SET_ITEM( argv_tuple, i - 1, argv_item );
	}

	fprintf( stderr, "argv_tuple = " );
	mxext_python_print_debug( argv_tuple, fname, Py_PRINT_RAW );
	fprintf( stderr, "\n" );


	/* Create the top level tuple for the call to main. */
	main_tuple = PyTuple_New( 2 );

	if ( main_tuple == NULL ) {
		MX_DEBUG(-2,
		("%s: PyTuple_New() for main_tuple failed.", fname));

		PyErr_Print();

		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"Creating main_tuple failed." );
	}

	Py_INCREF( main_function );
	Py_INCREF( py_ext->py_record_list );

	PyTuple_SET_ITEM( main_tuple, 0, py_ext->py_record_list );
	PyTuple_SET_ITEM( main_tuple, 1, argv_tuple );

	/* Now call the function. */

	main_result = PyObject_CallObject( main_function, main_tuple );

	if ( main_result == NULL ) {
		/* An error occurred, so tell the world about it. */

		MX_DEBUG(-2,("%s: The call to main() failed.", fname));

		PyErr_Print();

		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"The call to main() failed." );
	}

	/* Get rid of the members of argv_tuple. */

	for ( i = 1; i < argc; i++ ) {
		argv_item = PyTuple_GET_ITEM( argv_tuple, i - 1 );

		Py_DECREF(argv_item);
	}

	/* */

	Py_DECREF( argv_tuple );
	Py_DECREF( main_tuple );
	Py_DECREF( main_function );

	return MX_SUCCESSFUL_RESULT;
}

