/*
 * Name:     mx_module.c
 *
 * Purpose:  Support for dynamically loadable MX modules.
 *
 * Author:   William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_MODULE_DEBUG		TRUE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_module.h"

MX_EXPORT mx_status_type
mx_load_module( char *filename, MX_MODULE **module )
{
	static const char fname[] = "mx_load_module()";

	MX_DYNAMIC_LIBRARY *library;
	MX_MODULE *module_ptr;
	void *void_ptr;
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
	("%s: Module '%s', MX version %lu, %lu drivers, %lu extensions.",
		fname, module_ptr->name, module_ptr->mx_version,
		module_ptr->num_drivers, module_ptr->num_extensions));
#endif

	/* If there is a driver table in the module, add this driver table
	 * to our list of drivers.
	 */

	if ( module_ptr->num_drivers > 0 ) {
		if ( module_ptr->driver_table == (MX_DRIVER *) NULL ) {
			mx_warning(
			"Loadable module '%s' says that it has %lu drivers, "
			"but the driver table pointer is NULL.",
				module_ptr->name, module_ptr->num_drivers );
		} else {

#if MX_MODULE_DEBUG
			MX_DEBUG(-2,("%s: Adding %lu drivers from module '%s'.",
				fname, module_ptr->num_drivers,
				module_ptr->name));
#endif
			mx_status = mx_add_driver_table(
					module_ptr->driver_table );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	/* FIXME: Currently we ignore the extensions table. */

	return MX_SUCCESSFUL_RESULT;
}

