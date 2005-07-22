/*
 * Name:    ph_aps_topup.c
 *
 * Purpose: Driver for the scan APS topup time to inject handler.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_variable.h"
#include "mx_mpermit.h"
#include "mx_scan.h"
#include "ph_aps_topup.h"

MX_MEASUREMENT_PERMIT_FUNCTION_LIST mxph_topup_function_list = {
		mxph_topup_create_handler,
		mxph_topup_destroy_handler,
		mxph_topup_check_for_permission
};

static mx_status_type
mxph_topup_get_pointers( MX_MEASUREMENT_PERMIT *permit_handler,
			MX_TOPUP_MEASUREMENT_PERMIT **topup_permit_struct,
			const char *calling_fname )
{
	const char fname[] = "mxph_topup_get_pointers()";

	if ( permit_handler == (MX_MEASUREMENT_PERMIT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MEASUREMENT_PERMIT pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( topup_permit_struct == (MX_TOPUP_MEASUREMENT_PERMIT **)NULL )
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
	"The MX_TOPUP_MEASUREMENT_PERMIT pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*topup_permit_struct = (MX_TOPUP_MEASUREMENT_PERMIT *)
					permit_handler->type_struct;

	if ( *topup_permit_struct == (MX_TOPUP_MEASUREMENT_PERMIT *)NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_TOPUP_MEASUREMENT_PERMIT pointer for permit handler '%s' "
	"passed by '%s' is NULL.", permit_handler->typename, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxph_topup_create_handler( MX_MEASUREMENT_PERMIT **permit_handler,
				void *permit_driver_ptr, void *scan_ptr,
				char *description )
{
	const char fname[] = "mxph_topup_create_handler()";

	MX_MEASUREMENT_PERMIT_DRIVER *permit_driver;
	MX_MEASUREMENT_PERMIT *permit_handler_ptr;
	MX_SCAN *scan;
	MX_RECORD *topup_time_to_inject_record;
	MX_TOPUP_MEASUREMENT_PERMIT *topup_permit_struct;

	if ( permit_handler == (MX_MEASUREMENT_PERMIT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MEASUREMENT_PERMIT pointer passed is NULL." );
	}
	if ( permit_driver_ptr == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MEASUREMENT_PERMIT_DRIVER pointer passed is NULL." );
	}
	if ( scan_ptr == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCAN pointer passed is NULL." );
	}
	if ( description == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The description pointer passed is NULL." );
	}

	MX_DEBUG( 2,("%s invoked for description '%s'",
			fname, description));

	*permit_handler = NULL;

	permit_driver = (MX_MEASUREMENT_PERMIT_DRIVER *) permit_driver_ptr;

	scan = (MX_SCAN *) scan_ptr;

	MX_DEBUG( 2,("%s: scan = %p", fname, scan));
	MX_DEBUG( 2,("%s: scan->record = %p", fname, scan->record));
	MX_DEBUG( 2,("%s: scan->record->name = '%s'", fname, scan->record->name));

	/* Find the topup time to inject record. */

	topup_time_to_inject_record = mx_get_record( scan->record,
							description );

	if ( topup_time_to_inject_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"The topup time to inject record '%s' does not exist.",
			description );
	}

	/* Is this the correct kind of record. */

	if ( ( topup_time_to_inject_record->mx_superclass != MXR_VARIABLE )
	  || ( topup_time_to_inject_record->mx_class != MXV_CALC )
	  || ( topup_time_to_inject_record->mx_type
			!= MXV_CAL_APS_TOPUP_TIME_TO_INJECT ) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"Topup time to inject record '%s' is not the right type of record.  "
	"It should be a variable record of type 'aps_topup_time'.",
			topup_time_to_inject_record->name );
	}

	/* Create the permit handler. */

	permit_handler_ptr = (MX_MEASUREMENT_PERMIT *)
				malloc( sizeof( MX_MEASUREMENT_PERMIT ) );

	if ( permit_handler_ptr == (MX_MEASUREMENT_PERMIT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
    "Ran out of memory trying to create an MX_MEASUREMENT_PERMIT structure.");
	}

	/* Create the type specific structure. */

	topup_permit_struct = ( MX_TOPUP_MEASUREMENT_PERMIT * )
		malloc( sizeof( MX_TOPUP_MEASUREMENT_PERMIT ) );

	if ( topup_permit_struct == (MX_TOPUP_MEASUREMENT_PERMIT *)NULL )
	{
		free( permit_handler_ptr );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory trying to create an MX_TOPUP_MEASUREMENT_PERMIT structure.");
	}

	/* Fill in the fields in topup_permit_struct. */

	topup_permit_struct->topup_time_to_inject_record
					= topup_time_to_inject_record;

	/* Fill in fields in the permit handler structure. */

	permit_handler_ptr->scan = scan;
	permit_handler_ptr->type = permit_driver->type;
	permit_handler_ptr->typename = permit_driver->name;
	permit_handler_ptr->permit_status = FALSE;
	permit_handler_ptr->type_struct = topup_permit_struct;
	permit_handler_ptr->function_list = permit_driver->function_list;

	/* Send the permit handler back to the caller. */

	*permit_handler = permit_handler_ptr;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxph_topup_destroy_handler( MX_MEASUREMENT_PERMIT *permit_handler )
{
	const char fname[] = "mxph_topup_destroy_handler()";

	MX_TOPUP_MEASUREMENT_PERMIT *topup_permit_struct;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( permit_handler == (MX_MEASUREMENT_PERMIT *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mxph_topup_get_pointers( permit_handler,
					&topup_permit_struct, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( permit_handler->type_struct != NULL ) {
		free( permit_handler->type_struct );

		permit_handler->type_struct = NULL;
	}

	free( permit_handler );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxph_topup_check_for_permission( MX_MEASUREMENT_PERMIT *permit_handler )
{
	const char fname[] = "mxph_topup_check_for_permission()";

	MX_TOPUP_MEASUREMENT_PERMIT *topup_permit_struct;
	MX_RECORD *topup_time_to_inject_record;
	MX_SCAN *scan;
	double time_to_inject, measurement_time;
	long delay_milliseconds;
	mx_status_type mx_status;

	mx_status = mxph_topup_get_pointers( permit_handler,
					&topup_permit_struct, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scan = (MX_SCAN *) permit_handler->scan;

	if ( scan->measurement.type != MXM_PRESET_TIME ) {
		permit_handler->permit_status = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	permit_handler->permit_status = FALSE;

	topup_time_to_inject_record 
		= topup_permit_struct->topup_time_to_inject_record;

	MX_DEBUG(-2,("%s invoked for permit record '%s'",
			fname, topup_time_to_inject_record->name));

	
	measurement_time = mx_scan_get_measurement_time( scan );

	mx_status = mx_get_double_variable( topup_time_to_inject_record,
						&time_to_inject );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2, ("%s: measurement_time = %g, time_to_inject = %g",
				fname, measurement_time, time_to_inject));

	if ( measurement_time >= time_to_inject ) {

		mx_info(
		"Waiting for topup injection in %g seconds...",
			time_to_inject );

		delay_milliseconds = mx_round( 1000.0 *
					time_to_inject );

		while ( delay_milliseconds > 0 ) {

			if ( mx_user_requested_interrupt() ) {
				return mx_error(
					MXE_INTERRUPTED, fname,
			    "Topup time delay interrupted." );
			}

			if ( delay_milliseconds < 1000 ) {
				mx_msleep(delay_milliseconds);
			} else {
				mx_msleep(1000);

				delay_milliseconds -= 1000;
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

