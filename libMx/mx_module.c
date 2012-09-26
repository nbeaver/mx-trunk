/*
 * Name:     mx_module.c
 *
 * Purpose:  Support for dynamically loadable MX modules.
 *
 * Author:   William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2010-2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_MODULE_DEBUG			FALSE

#define MX_MODULE_DEBUG_EXTENSION	TRUE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_list.h"
#include "mx_module.h"

MX_EXPORT mx_status_type
mx_load_module( char *filename, MX_RECORD *record_list, MX_MODULE **module )
{
	static const char fname[] = "mx_load_module()";

	MX_DYNAMIC_LIBRARY *library;
	MX_MODULE *module_ptr, *test_module;
	void *void_ptr, *init_ptr;
	MX_LIST_HEAD *record_list_head;
	MX_LIST *module_list;
	MX_LIST_ENTRY *module_list_entry;
	MX_MODULE_INIT *module_init_fn;
	unsigned long module_status_code;
	char module_init_name[100];
	mx_status_type mx_status;

	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}

	/* An MX module is packaged as a dynamically loadable library,
	 * so we begin by attempting to load the specified file as a
	 * library.
	 */

	mx_status = mx_dynamic_library_open( filename, &library );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The contents of the module are described by an MX_MODULE
	 * structure.  The MX_MODULE structure can be found by searching
	 * for a symbol named __MX_MODULE__.
	 */

	mx_status = mx_dynamic_library_find_symbol( library,
					"__MX_MODULE__", &void_ptr, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	module_ptr = (MX_MODULE *) void_ptr;

	/* Is there a module with this name already loaded? */

	if ( record_list != NULL ) {
		mx_status = mx_get_module( module_ptr->name,
				record_list, &test_module );

		module_status_code = mx_status.code & (~MXE_QUIET);

		/* A status code of MXE_NOT_FOUND means that the module
		 * has not yet been loaded.
		 */

		if ( module_status_code != MXE_NOT_FOUND ) {

			char local_name[MXU_MODULE_NAME_LENGTH+1];

			strlcpy( local_name, module_ptr->name,
					MXU_MODULE_NAME_LENGTH );

			mx_status = mx_dynamic_library_close( library );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( module_status_code == MXE_SUCCESS ) {
				/* This means a  module by this name
				 * has been loaded already.
				 */
				return mx_error( MXE_ALREADY_EXISTS, fname,
				"A module named '%s' has already been loaded "
				"into the running database.", local_name );
			} else {
				return mx_status;
			}
		}
	}

	/* If requested, return a copy of the MX_MODULE pointer to the caller.*/

	if ( module != (MX_MODULE **) NULL ) {
		*module = module_ptr;
	}

	/* Save a copy of the pointer to the dynamically-loaded library. */

	module_ptr->library = library;

#if MX_MODULE_DEBUG
	MX_DEBUG(-2,("%s: Module '%s' loaded from file '%s'.",
		fname, module_ptr->name, filename ));
	MX_DEBUG(-2,
	("%s: Module '%s', MX version %lu, drivers = %p, extensions = %p",
		fname, module_ptr->name, module_ptr->mx_version,
		module_ptr->driver_table, module_ptr->extension_table));
#endif

	/*-----------------------------------------------------------------*/

	/* If we have a pointer to the MX database, then add the module
	 * to the MX database's module_list and also save the pointer
	 * in the module itself.
	 */

	module_ptr->record_list = NULL;

	if ( record_list != NULL ) {

		module_ptr->record_list = record_list;

		record_list_head = mx_get_record_list_head_struct(record_list);

		if ( record_list_head == (MX_LIST_HEAD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_LIST_HEAD pointer for record '%s' is NULL.",
				record_list->name );
		}

		if ( record_list_head->module_list == (void *) NULL ) {
			mx_status = mx_list_create( &module_list );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			record_list_head->module_list = module_list;
		} else {
			module_list = record_list_head->module_list;
		}

		mx_status = mx_list_entry_create( &module_list_entry,
							module_ptr, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_list_add_entry( module_list, module_list_entry );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/*-----------------------------------------------------------------*/

	/* If there is a driver table in the module, add this driver table
	 * to our list of drivers.
	 */

	if ( module_ptr->driver_table != (MX_DRIVER *) NULL ) {

#if MX_MODULE_DEBUG
		MX_DEBUG(-2,("%s: Adding drivers from module '%s'.",
				fname, module_ptr->name));
		{
			MX_DRIVER *driver;
			unsigned long i;

			for ( i = 0; ; i++ ) {
				driver = &(module_ptr->driver_table[i]);

				if ( driver->mx_superclass == 0 ) {
					break;
				}

				MX_DEBUG(-2,("%s: Found driver '%s'",
					fname, driver->name));
			}
		}
#endif

		mx_status = mx_add_driver_table( module_ptr->driver_table );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* FIXME: Currently we ignore the extensions table. */

	/*-----------------------------------------------------------------*/

	/* Does the module contain module initialization function?
	 * 
	 * The name of the MX module init function is constructed from
	 * the name of the module read out of the __MX_MODULE__ struct.
	 *
	 * On Unix-like platforms, if you pass a name to dlsym() that
	 * is not actually present in the specified dynamic library,
	 * dlsym() will go and look for that symbol in all other dynamic
	 * libraries that it knows about.  In older versions of MX, all
	 * modules used the name __MX_MODULE_INIT__, which meant that
	 * all modules had to include this symbol, even if it was NULL,
	 * to prevent dlsym() from finding the wrong symbol.
	 *
	 * The way around this is to use a module-specific name for the
	 * init function.  By doing this, if a module does not have an
	 * init function, dlsym() will not accidentally find the wrong
	 * init function in a different module.
	 */

	snprintf( module_init_name, sizeof(module_init_name),
		"__MX_MODULE_INIT_%s__", module_ptr->name );

	mx_status = mx_dynamic_library_find_symbol( library,
			module_init_name, &init_ptr, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return MX_SUCCESSFUL_RESULT;

	module_init_fn = init_ptr;

	if ( ( module_init_fn != NULL )
	  && ( *module_init_fn != NULL ) )
	{
		(*module_init_fn)( module_ptr );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxp_module_list_traverse_fn( MX_LIST_ENTRY *list_entry,
				void *module_name_ptr,
				void **module_ptr )
{
#if MX_MODULE_DEBUG
	static const char fname[] = "mxp_module_list_traverse_fn()";
#endif

	MX_MODULE *module;
	char *module_name;

	module = list_entry->list_entry_data;

	module_name = module_name_ptr;

#if MX_MODULE_DEBUG
	MX_DEBUG(-2,("%s: Checking module '%s' for match to '%s'.",
		fname, module->name, module_name ));
#endif

	if ( strcmp( module->name, module_name ) == 0 ) {
		*module_ptr = (void *) module;

		return mx_error( MXE_EARLY_EXIT | MXE_QUIET, "", " " );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_get_module( char *module_name, MX_RECORD *record_list, MX_MODULE **module )
{
	static const char fname[] = "mx_get_module()";

	MX_LIST_HEAD *record_list_head;
	MX_LIST *module_list;
	void *module_ptr;
	unsigned long mx_status_code;
	mx_status_type mx_status;

	if ( module_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The module_name pointer passed was NULL." );
	}
	if ( record_list == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD list pointer passed was NULL." );
	}
	if ( module == (MX_MODULE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The pointer to the MX_MODULE pointer is NULL." );
	}

#if MX_MODULE_DEBUG
	MX_DEBUG(-2,("%s: Looking for module '%s'.", fname, module_name ));
#endif

	record_list_head = mx_get_record_list_head_struct( record_list );

	if ( record_list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for record '%s' is NULL.",
			record_list->name );
	}

	/* If the module_list pointer is NULL, then
	 * no modules have been loaded.
	 */

	if ( record_list_head->module_list == NULL ) {
		*module = NULL;

#if MX_MODULE_DEBUG
		MX_DEBUG(-2,("%s: No modules are loaded.", fname));
#endif

#if MX_MODULE_DEBUG
		mx_status_code = MXE_NOT_FOUND;
#else
		mx_status_code = MXE_NOT_FOUND | MXE_QUIET;
#endif
		return mx_error( mx_status_code, fname,
		"Did not find module '%s' since no modules "
		"have been loaded yet.", module_name );
	}

	module_list = record_list_head->module_list;

	mx_status = mx_list_traverse( module_list,
					mxp_module_list_traverse_fn,
					module_name,
					&module_ptr );
#if MX_MODULE_DEBUG
	MX_DEBUG(-2,("%s: mx_list_traverse() returned status code %lu",
		fname, mx_status.code ));
#endif

	switch( mx_status.code ) {
	case MXE_EARLY_EXIT:
		/* We found the module. */

		*module = module_ptr;

		return MX_SUCCESSFUL_RESULT;
		break;

	case MXE_SUCCESS:
		/* In this case, MXE_SUCCESS actually means we did _not_
		 * find the module.
		 */

		*module = NULL;

#if MX_MODULE_DEBUG
		mx_status_code = MXE_NOT_FOUND;
#else
		mx_status_code = MXE_NOT_FOUND | MXE_QUIET;
#endif
		return mx_error( mx_status_code, fname,
			"Module '%s' is not loaded for the current database.",
			module_name );
		break;
	default:
		*module = NULL;

		return mx_status;
	}
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxp_extension_traverse_fn( MX_LIST_ENTRY *list_entry,
				void *extension_name_ptr,
				void **extension_ptr )
{
#if MX_MODULE_DEBUG_EXTENSION
	static const char fname[] = "mxp_extension_traverse_fn()";
#endif

	MX_EXTENSION *extension, *extension_table;
	char *extension_name;

	MX_MODULE *module;
	unsigned long i;

	extension_name = extension_name_ptr;

	module = list_entry->list_entry_data;

#if MX_MODULE_DEBUG_EXTENSION
	MX_DEBUG(-2,("%s: Checking module '%s' for extension '%s'.",
		fname, module->name, extension_name ));
#endif

	extension_table = module->extension_table;

	if ( extension_table == (MX_EXTENSION *) NULL ) {
		/* This module does not have any extensions. */

		return MX_SUCCESSFUL_RESULT;
	}

	/* Walk through the extensions. */

	for ( i = 0; ; i++ ) {
		extension = &extension_table[i];

		if ( extension->name[0] == '\0' ) {

			/* We have reached the end of the extension table
			 * without finding the extension, so return.
			 */

			return MX_SUCCESSFUL_RESULT;
		}

		if ( strcmp( extension->name, extension_name ) == 0 ) {

			/* We have _found_ the extension. */

			/* If the extension has not set a pointer back to
			 * the enclosing module, then add that pointer now.
			 */

			if ( extension->module == (MX_MODULE *) NULL ) {
				extension->module = module;
			}

			/* Return the extension pointer. */

			*extension_ptr = (void *) extension;

			return mx_error( MXE_EARLY_EXIT | MXE_QUIET, "", " " );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

/* mx_get_extension() walks through all of the loaded modules looking for
 * the requested extension.
 */

MX_EXPORT mx_status_type
mx_get_extension( char *extension_name, MX_RECORD *record_list,
					MX_EXTENSION **extension )
{
	static const char fname[] = "mx_get_extension()";

	MX_LIST_HEAD *record_list_head;
	MX_LIST *module_list;
	void *extension_ptr;
	unsigned long mx_status_code;
	mx_status_type mx_status;

	if ( extension_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The extension_name pointer passed was NULL." );
	}
	if ( record_list == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD list pointer passed was NULL." );
	}
	if ( extension == (MX_EXTENSION **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The pointer to the MX_EXTENSION pointer is NULL." );
	}

#if MX_MODULE_DEBUG_EXTENSION
	MX_DEBUG(-2,("%s: Looking for extension '%s'.",
		fname, extension_name ));
#endif

	record_list_head = mx_get_record_list_head_struct( record_list );

	if ( record_list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for record '%s' is NULL.",
			record_list->name );
	}

	/* If the module_list pointer is NULL, then
	 * no modules have been loaded.
	 */

	if ( record_list_head->module_list == NULL ) {
		*extension = NULL;

#if MX_MODULE_DEBUG_EXTENSION
		MX_DEBUG(-2,("%s: No modules are loaded.", fname));
#endif

#if MX_MODULE_DEBUG_EXTENSION
		mx_status_code = MXE_NOT_FOUND;
#else
		mx_status_code = MXE_NOT_FOUND | MXE_QUIET;
#endif
		return mx_error( mx_status_code, fname,
		"Did not find extension '%s' since no modules "
		"have been loaded yet.", extension_name );
	}

	module_list = record_list_head->module_list;

	mx_status = mx_list_traverse( module_list,
					mxp_extension_traverse_fn,
					extension_name,
					&extension_ptr );
#if MX_MODULE_DEBUG_EXTENSION
	MX_DEBUG(-2,("%s: mx_list_traverse() returned status code %lu",
		fname, mx_status.code ));
#endif

	switch( mx_status.code ) {
	case MXE_EARLY_EXIT:
		/* We found the extension. */

		*extension = extension_ptr;

		return MX_SUCCESSFUL_RESULT;
		break;

	case MXE_SUCCESS:
		/* In this case, MXE_SUCCESS actually means we did _not_
		 * find the extension.
		 */

		*extension = NULL;

#if MX_MODULE_DEBUG_EXTENSION
		mx_status_code = MXE_NOT_FOUND;
#else
		mx_status_code = MXE_NOT_FOUND | MXE_QUIET;
#endif
		return mx_error( mx_status_code, fname,
		"Extension '%s' is not loaded for the current database.",
			extension_name );
		break;
	default:
		*extension = NULL;

		return mx_status;
	}
}

/*-------------------------------------------------------------------------*/

