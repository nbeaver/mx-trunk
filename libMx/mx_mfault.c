/*
 * Name:    mx_mfault.c
 *
 * Purpose: Functions to support measurement fault handlers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2002, 2005, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_mfault.h"

#include "fh_simple.h"
#include "fh_autoscale.h"

MX_MEASUREMENT_FAULT_DRIVER mx_measurement_fault_driver_list[] = {
	{ MXFH_SIMPLE,    "simple",    &mxfh_simple_function_list },
	{ MXFH_AUTOSCALE, "autoscale", &mxfh_autoscale_function_list },
	{ -1, "", NULL }
};

static mx_status_type
mx_measurement_fault_get_pointers( MX_MEASUREMENT_FAULT *fault_handler,
			MX_MEASUREMENT_FAULT_FUNCTION_LIST **function_list,
			const char *calling_fname )
{
	const char fname[] = "mx_measurement_fault_get_pointers()";

	if ( fault_handler == (MX_MEASUREMENT_FAULT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MEASUREMENT_FAULT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( function_list == (MX_MEASUREMENT_FAULT_FUNCTION_LIST **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
"The MX_MEASUREMENT_FAULT_FUNCTION_LIST pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*function_list = fault_handler->function_list;

	if ( (*function_list) == (MX_MEASUREMENT_FAULT_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MEASUREMENT_FAULT_FUNCTION_LIST pointer "
			"for fault handler '%s' passed by '%s' is NULL.",
			fault_handler->mx_typename,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_measurement_fault_get_driver_by_name(
			MX_MEASUREMENT_FAULT_DRIVER *fault_driver_list,
			char *name,
			MX_MEASUREMENT_FAULT_DRIVER **fault_driver )
{
	const char fname[] = "mx_measurement_fault_get_driver_by_name()";


	char *list_name;
	int i;

	if ( fault_driver_list == NULL ) {
		*fault_driver = NULL;

		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Fault driver list passed was NULL." );
	}

	if ( name == NULL ) {
		*fault_driver = NULL;

		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Fault driver name pointer passed is NULL.");
	}

	for ( i = 0; ; i++ ) {
		/* Check for the end of the list. */

		if ( fault_driver_list[i].type < 0 ) {
			*fault_driver = NULL;

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Fault driver type '%s' was not found.", name );
		}

		list_name = fault_driver_list[i].name;

		if ( list_name == NULL ) {
			*fault_driver = NULL;

			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"NULL name ptr for fault driver %ld.",
				fault_driver_list[i].type );
		}

		if ( strcmp( name, list_name ) == 0 ) {
			*fault_driver = &( fault_driver_list[i] );

			return MX_SUCCESSFUL_RESULT;
		}
	}
}

MX_EXPORT mx_status_type
mx_measurement_fault_create_handler(
			MX_MEASUREMENT_FAULT **fault_handler,
			void *scan,
			char *handler_description )
{
	const char fname[] = "mx_measurement_fault_create_handler()";

	MX_MEASUREMENT_FAULT_DRIVER *fault_driver;
	char driver_name[ MXU_MEASUREMENT_FAULT_DRIVER_NAME_LENGTH + 1 ];
	mx_status_type ( *fptr )
			( MX_MEASUREMENT_FAULT **, void *, void *, char * );
	char *ptr, *driver_specific_description;
	size_t length;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for description '%s'",
		fname, handler_description));

	if ( fault_handler == (MX_MEASUREMENT_FAULT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MEASUREMENT_FAULT pointer passed was NULL." );
	}

	*fault_handler = NULL;

	/* The fault driver name appears at the beginning of the string
	 * and is terminated by a ':' character.
	 */

	ptr = strchr( handler_description, ':' );

	if ( ptr == NULL ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"The fault driver name terminator character ':' was not found in "
	"the fault handler description '%s'.", handler_description );
	}

	length = ptr - handler_description;

	if ( length > MXU_MEASUREMENT_FAULT_DRIVER_NAME_LENGTH ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"The fault driver name in the handler description '%s' is longer "
	"than the maximum allowed length of %d.", handler_description,
			MXU_MEASUREMENT_FAULT_DRIVER_NAME_LENGTH );
	}

	strlcpy( driver_name, handler_description, length + 1 );

	driver_specific_description = ptr + 1;

	mx_status = mx_measurement_fault_get_driver_by_name(
				mx_measurement_fault_driver_list, driver_name,
				&fault_driver );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find the create_handler function for this driver. */

	if ( fault_driver->function_list == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_MEASUREMENT_FAULT_FUNCTION_LIST pointer for fault driver '%s is NULL.",
			fault_driver->name );
	}

	fptr = fault_driver->function_list->create_handler;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The create_handler function pointer for fault driver '%s' is NULL.",
			fault_driver->name );
	}

	MX_DEBUG( 2,("%s: scan = %p", fname, scan));

	/* Invoke the driver specific function. */

	mx_status = ( *fptr ) ( fault_handler, fault_driver, scan,
					driver_specific_description );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_measurement_fault_destroy_handler( MX_MEASUREMENT_FAULT *fault_handler )
{
	const char fname[] = "mx_measurement_fault_destroy_handler()";

	MX_MEASUREMENT_FAULT_FUNCTION_LIST *function_list;
	mx_status_type ( *fptr ) ( MX_MEASUREMENT_FAULT * );
	mx_status_type mx_status;

	if ( fault_handler == (MX_MEASUREMENT_FAULT *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_measurement_fault_get_pointers( fault_handler,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = function_list->destroy_handler;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The destroy_handler function pointer for fault handler '%s' is NULL.",
			fault_handler->mx_typename );
	}

	mx_status = ( *fptr ) ( fault_handler );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_measurement_fault_check_for_fault(
			MX_MEASUREMENT_FAULT *fault_handler,
			int *fault_status )
{
	const char fname[] = "mx_measurement_fault_check_for_fault()";

	MX_MEASUREMENT_FAULT_FUNCTION_LIST *function_list;
	mx_status_type ( *fptr ) ( MX_MEASUREMENT_FAULT * );
	mx_status_type mx_status;

	mx_status = mx_measurement_fault_get_pointers( fault_handler,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( fault_handler == (MX_MEASUREMENT_FAULT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MEASUREMENT_FAULT pointer passed was NULL." );
	}

	function_list = fault_handler->function_list;

	if ( function_list == (MX_MEASUREMENT_FAULT_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MEASUREMENT_FAULT_FUNCTION_LIST pointer "
			"for fault handler '%s' is NULL.",
			fault_handler->mx_typename );
	}

	fptr = function_list->check_for_fault;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The check_for_fault function pointer for fault handler '%s' is NULL.",
			fault_handler->mx_typename );
	}

	mx_status = ( *fptr ) ( fault_handler );

	if ( fault_status != NULL ) {
		*fault_status = fault_handler->fault_status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_measurement_fault_reset( MX_MEASUREMENT_FAULT *fault_handler,
				int reset_flags )
{
	const char fname[] = "mx_measurement_fault_reset()";

	MX_MEASUREMENT_FAULT_FUNCTION_LIST *function_list;
	mx_status_type ( *fptr ) ( MX_MEASUREMENT_FAULT * );
	mx_status_type mx_status;

	mx_status = mx_measurement_fault_get_pointers( fault_handler,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = function_list->reset;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The reset function pointer for fault handler '%s' is NULL.",
			fault_handler->mx_typename );
	}

	fault_handler->reset_flags = reset_flags;

	mx_status = ( *fptr ) ( fault_handler );

	return mx_status;
}

