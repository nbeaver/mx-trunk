/*
 * Name:    mx_mpermit.c
 *
 * Purpose: Functions to support measurement permit handlers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2002, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_mpermit.h"

#include "ph_simple.h"
#include "ph_aps_topup.h"

MX_MEASUREMENT_PERMIT_DRIVER mx_measurement_permit_driver_list[] = {
	{ MXPH_SIMPLE,    "simple",    &mxph_simple_function_list },
	{ MXPH_TOPUP,     "topup",     &mxph_topup_function_list },
	{ -1, "", NULL }
};

static mx_status_type
mx_measurement_permit_get_pointers( MX_MEASUREMENT_PERMIT *permit_handler,
			MX_MEASUREMENT_PERMIT_FUNCTION_LIST **function_list,
			const char *calling_fname )
{
	const char fname[] = "mx_measurement_permit_get_pointers()";

	if ( permit_handler == (MX_MEASUREMENT_PERMIT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MEASUREMENT_PERMIT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( function_list == (MX_MEASUREMENT_PERMIT_FUNCTION_LIST **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
"The MX_MEASUREMENT_PERMIT_FUNCTION_LIST pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*function_list = permit_handler->function_list;

	if ( (*function_list) == (MX_MEASUREMENT_PERMIT_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MEASUREMENT_PERMIT_FUNCTION_LIST pointer "
			"for permit handler '%s' passed by '%s' is NULL.",
			permit_handler->typename,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_measurement_permit_get_driver_by_name(
			MX_MEASUREMENT_PERMIT_DRIVER *permit_driver_list,
			char *name,
			MX_MEASUREMENT_PERMIT_DRIVER **permit_driver )
{
	const char fname[] = "mx_measurement_permit_get_driver_by_name()";


	char *list_name;
	int i;

	if ( permit_driver_list == NULL ) {
		*permit_driver = NULL;

		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Permit driver list passed was NULL." );
	}

	if ( name == NULL ) {
		*permit_driver = NULL;

		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Permit driver name pointer passed is NULL.");
	}

	for ( i = 0; ; i++ ) {
		/* Check for the end of the list. */

		if ( permit_driver_list[i].type < 0 ) {
			*permit_driver = NULL;

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Permit driver type '%s' was not found.", name );
		}

		list_name = permit_driver_list[i].name;

		if ( list_name == NULL ) {
			*permit_driver = NULL;

			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"NULL name ptr for permit driver %ld.",
				permit_driver_list[i].type );
		}

		if ( strcmp( name, list_name ) == 0 ) {
			*permit_driver = &( permit_driver_list[i] );

			return MX_SUCCESSFUL_RESULT;
		}
	}
}

MX_EXPORT mx_status_type
mx_measurement_permit_create_handler(
			MX_MEASUREMENT_PERMIT **permit_handler,
			void *scan,
			char *handler_description )
{
	const char fname[] = "mx_measurement_permit_create_handler()";

	MX_MEASUREMENT_PERMIT_DRIVER *permit_driver;
	char driver_name[ MXU_MEASUREMENT_PERMIT_DRIVER_NAME_LENGTH + 1 ];
	mx_status_type ( *fptr )
			( MX_MEASUREMENT_PERMIT **, void *, void *, char * );
	char *ptr, *driver_specific_description;
	size_t length;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for description '%s'",
		fname, handler_description));

	if ( permit_handler == (MX_MEASUREMENT_PERMIT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MEASUREMENT_PERMIT pointer passed was NULL." );
	}

	*permit_handler = NULL;

	/* The permit driver name appears at the beginning of the string
	 * and is terminated by a ':' character.
	 */

	ptr = strchr( handler_description, ':' );

	if ( ptr == NULL ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"The permit driver name terminator character ':' was not found in "
	"the permit handler description '%s'.", handler_description );
	}

	length = ptr - handler_description;

	if ( length > MXU_MEASUREMENT_PERMIT_DRIVER_NAME_LENGTH ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"The permit driver name in the handler description '%s' is longer "
	"than the maximum allowed length of %d.", handler_description,
			MXU_MEASUREMENT_PERMIT_DRIVER_NAME_LENGTH );
	}

	strlcpy( driver_name, handler_description, length + 1 );

	driver_specific_description = ptr + 1;

	mx_status = mx_measurement_permit_get_driver_by_name(
				mx_measurement_permit_driver_list, driver_name,
				&permit_driver );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find the create_handler function for this driver. */

	if ( permit_driver->function_list == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_MEASUREMENT_PERMIT_FUNCTION_LIST pointer for permit driver '%s is NULL.",
			permit_driver->name );
	}

	fptr = permit_driver->function_list->create_handler;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The create_handler function pointer for permit driver '%s' is NULL.",
			permit_driver->name );
	}

	MX_DEBUG( 2,("%s: scan = %p", fname, scan));

	/* Invoke the driver specific function. */

	mx_status = ( *fptr ) ( permit_handler, permit_driver, scan,
					driver_specific_description );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_measurement_permit_destroy_handler( MX_MEASUREMENT_PERMIT *permit_handler )
{
	const char fname[] = "mx_measurement_permit_destroy_handler()";

	MX_MEASUREMENT_PERMIT_FUNCTION_LIST *function_list;
	mx_status_type ( *fptr ) ( MX_MEASUREMENT_PERMIT * );
	mx_status_type mx_status;

	if ( permit_handler == (MX_MEASUREMENT_PERMIT *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_measurement_permit_get_pointers( permit_handler,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = function_list->destroy_handler;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The destroy_handler function pointer for permit handler '%s' is NULL.",
			permit_handler->typename );
	}

	mx_status = ( *fptr ) ( permit_handler );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_measurement_permit_check_for_permission(
			MX_MEASUREMENT_PERMIT *permit_handler,
			int *permit_status )
{
	const char fname[] = "mx_measurement_permit_check_for_permission()";

	MX_MEASUREMENT_PERMIT_FUNCTION_LIST *function_list;
	mx_status_type ( *fptr ) ( MX_MEASUREMENT_PERMIT * );
	mx_status_type mx_status;

	mx_status = mx_measurement_permit_get_pointers( permit_handler,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( permit_handler == (MX_MEASUREMENT_PERMIT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MEASUREMENT_PERMIT pointer passed was NULL." );
	}

	function_list = permit_handler->function_list;

	if ( function_list == (MX_MEASUREMENT_PERMIT_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MEASUREMENT_PERMIT_FUNCTION_LIST pointer "
			"for permit handler '%s' is NULL.",
			permit_handler->typename );
	}

	fptr = function_list->check_for_permission;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The check_for_permission function pointer for permit handler '%s' is NULL.",
			permit_handler->typename );
	}

	mx_status = ( *fptr ) ( permit_handler );

	if ( permit_status != NULL ) {
		*permit_status = permit_handler->permit_status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_measurement_permit_wait_for_permission(
			MX_MEASUREMENT_PERMIT *permit_handler )
{
	const char fname[] = "mx_measurement_permit_wait_for_permission()";

	MX_MEASUREMENT_PERMIT_FUNCTION_LIST *function_list;
	int i, user_message_sent;
	mx_status_type ( *fptr ) ( MX_MEASUREMENT_PERMIT * );
	mx_status_type mx_status;

	mx_status = mx_measurement_permit_get_pointers( permit_handler,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( permit_handler == (MX_MEASUREMENT_PERMIT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MEASUREMENT_PERMIT pointer passed was NULL." );
	}

	function_list = permit_handler->function_list;

	if ( function_list == (MX_MEASUREMENT_PERMIT_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MEASUREMENT_PERMIT_FUNCTION_LIST pointer "
			"for permit handler '%s' is NULL.",
			permit_handler->typename );
	}

	fptr = function_list->check_for_permission;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The check_for_permission function pointer for permit handler '%s' is NULL.",
			permit_handler->typename );
	}

	/* Wait until we get permission to do the measurement or until
	 * the user interrupts the loop.
	 */

	user_message_sent = FALSE;

	for ( i = 0; ; i++ ) {

		mx_status = ( *fptr ) ( permit_handler );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( permit_handler->permit_status == TRUE ) {
			break;			/* Exit the for() loop. */
		}

		if ( i == 3 ) {
			user_message_sent = TRUE;
			mx_info(
			"Waiting for permission to perform the measurement.");
		}
		if ( mx_user_requested_interrupt() ) {
			return mx_error( MXE_INTERRUPTED, fname,
		"The wait for measurement permission was interrupted." );
		}

		mx_msleep(10);
	}
	if ( user_message_sent ) {
		mx_info("Measurement permission received.");
	}
	return MX_SUCCESSFUL_RESULT;
}

