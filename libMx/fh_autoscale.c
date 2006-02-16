/*
 * Name:    fh_autoscale.c
 *
 * Purpose: Driver for the scan autoscaling handler.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2006 Illinois Institute of Technology
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
#include "mx_mfault.h"
#include "mx_autoscale.h"
#include "mx_scan.h"
#include "d_auto_scaler.h"
#include "fh_autoscale.h"

MX_MEASUREMENT_FAULT_FUNCTION_LIST mxfh_autoscale_function_list = {
		mxfh_autoscale_create_handler,
		mxfh_autoscale_destroy_handler,
		mxfh_autoscale_check_for_fault,
		mxfh_autoscale_reset
};

static mx_status_type
mxfh_autoscale_get_pointers( MX_MEASUREMENT_FAULT *fault_handler,
			MX_AUTOSCALE_MEASUREMENT_FAULT **autoscale_fault_struct,
			const char *calling_fname )
{
	const char fname[] = "mxfh_autoscale_get_pointers()";

	if ( fault_handler == (MX_MEASUREMENT_FAULT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MEASUREMENT_FAULT pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( autoscale_fault_struct == (MX_AUTOSCALE_MEASUREMENT_FAULT **)NULL )
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
	"The MX_AUTOSCALE_MEASUREMENT_FAULT pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*autoscale_fault_struct = (MX_AUTOSCALE_MEASUREMENT_FAULT *)
					fault_handler->type_struct;

	if ( *autoscale_fault_struct == (MX_AUTOSCALE_MEASUREMENT_FAULT *)NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_AUTOSCALE_MEASUREMENT_FAULT pointer for fault handler '%s' "
	"passed by '%s' is NULL.", fault_handler->typename, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxfh_autoscale_create_handler( MX_MEASUREMENT_FAULT **fault_handler,
				void *fault_driver_ptr, void *scan_ptr,
				char *description )
{
	const char fname[] = "mxfh_autoscale_create_handler()";

	MX_MEASUREMENT_FAULT_DRIVER *fault_driver;
	MX_MEASUREMENT_FAULT *fault_handler_ptr;
	MX_SCAN *scan;
	MX_RECORD *autoscale_scaler_record;
	MX_AUTOSCALE_SCALER *autoscale_scaler;
	MX_AUTOSCALE_MEASUREMENT_FAULT *autoscale_fault_struct;
	int i, scaler_used;

	if ( fault_handler == (MX_MEASUREMENT_FAULT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MEASUREMENT_FAULT pointer passed is NULL." );
	}
	if ( fault_driver_ptr == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MEASUREMENT_FAULT_DRIVER pointer passed is NULL." );
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

	*fault_handler = NULL;

	fault_driver = (MX_MEASUREMENT_FAULT_DRIVER *) fault_driver_ptr;

	scan = (MX_SCAN *) scan_ptr;

	MX_DEBUG( 2,("%s: scan = %p", fname, scan));
	MX_DEBUG( 2,("%s: scan->record = %p", fname, scan->record));
	MX_DEBUG( 2,("%s: scan->record->name = '%s'", fname, scan->record->name));

	/* The description should consist of the name of an
	 * autoscale scaler.
	 */

	autoscale_scaler_record = mx_get_record( scan->record, description );

	if ( autoscale_scaler_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"The record '%s' does not exist.", description);
	}

	/* Is this an autoscale scaler record? */

	if ( ( autoscale_scaler_record->mx_superclass != MXR_DEVICE )
	  || ( autoscale_scaler_record->mx_class != MXC_SCALER )
	  || ( autoscale_scaler_record->mx_type != MXT_SCL_AUTOSCALE ) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Record '%s' is not an autoscale scaler record and cannot "
		"be used by an autoscale scan fault handler.",
			autoscale_scaler_record->name );
	}

	/* Is the autoscale scaler record used by this scan? */

	scaler_used = FALSE;

	for ( i = 0; i < scan->num_input_devices; i++ ) {

		if ( (scan->input_device_array)[i] == autoscale_scaler_record )
			scaler_used = TRUE;
	}

	/* If it is not used, return without setting up a fault handler. */

	if ( scaler_used == FALSE ) {
		return MX_SUCCESSFUL_RESULT;
	}

	autoscale_scaler = (MX_AUTOSCALE_SCALER *)
				autoscale_scaler_record->record_type_struct;

	if ( autoscale_scaler == (MX_AUTOSCALE_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AUTOSCALE_SCALER pointer for record '%s' is NULL.",
			autoscale_scaler_record->name );
	}

	if ( autoscale_scaler->autoscale_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The autoscale_record pointer for autoscale scaler '%s' is NULL.",
			autoscale_scaler_record->name );
	}

	/* Create the fault handler. */

	fault_handler_ptr = (MX_MEASUREMENT_FAULT *)
				malloc( sizeof( MX_MEASUREMENT_FAULT ) );

	if ( fault_handler_ptr == (MX_MEASUREMENT_FAULT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to create an MX_MEASUREMENT_FAULT "
		"structure for scaler '%s'", autoscale_scaler_record->name );
	}

	/* Create the type specific structure. */

	autoscale_fault_struct = ( MX_AUTOSCALE_MEASUREMENT_FAULT * )
		malloc( sizeof( MX_AUTOSCALE_MEASUREMENT_FAULT ) );

	if ( autoscale_fault_struct == (MX_AUTOSCALE_MEASUREMENT_FAULT *)NULL )
	{
		free( fault_handler_ptr );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory trying to create an MX_AUTOSCALE_MEASUREMENT_FAULT "
	"structure for scaler '%s'", autoscale_scaler_record->name );
	}

	/* Fill in the fields in autoscale_fault_struct. */

	autoscale_fault_struct->autoscale_scaler_record
					= autoscale_scaler_record;
	autoscale_fault_struct->autoscale_record
					= autoscale_scaler->autoscale_record;

	/* Fill in fields in the fault handler structure. */

	fault_handler_ptr->scan = scan;
	fault_handler_ptr->type = fault_driver->type;
	fault_handler_ptr->typename = fault_driver->name;
	fault_handler_ptr->fault_status = 0;
	fault_handler_ptr->reset_flags = 0;
	fault_handler_ptr->type_struct = autoscale_fault_struct;
	fault_handler_ptr->function_list = fault_driver->function_list;

	/* Send the fault handler back to the caller. */

	*fault_handler = fault_handler_ptr;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxfh_autoscale_destroy_handler( MX_MEASUREMENT_FAULT *fault_handler )
{
	const char fname[] = "mxfh_autoscale_destroy_handler()";

	MX_AUTOSCALE_MEASUREMENT_FAULT *autoscale_fault_struct;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( fault_handler == (MX_MEASUREMENT_FAULT *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mxfh_autoscale_get_pointers( fault_handler,
					&autoscale_fault_struct, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( fault_handler->type_struct != NULL ) {
		free( fault_handler->type_struct );

		fault_handler->type_struct = NULL;
	}

	free( fault_handler );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxfh_autoscale_check_for_fault( MX_MEASUREMENT_FAULT *fault_handler )
{
	const char fname[] = "mxfh_autoscale_check_for_fault()";

	MX_AUTOSCALE_MEASUREMENT_FAULT *autoscale_fault_struct;
	int change_request;
	mx_status_type mx_status;

	mx_status = mxfh_autoscale_get_pointers( fault_handler,
					&autoscale_fault_struct, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for autoscale scaler '%s'.", fname,
			autoscale_fault_struct->autoscale_scaler_record->name));

	mx_status = mx_autoscale_get_change_request(
			autoscale_fault_struct->autoscale_record,
			&change_request );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	autoscale_fault_struct->change_request = change_request;

	if ( change_request == MXF_AUTO_NO_CHANGE ) {
		fault_handler->fault_status = FALSE;
	} else {
		fault_handler->fault_status = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxfh_autoscale_reset( MX_MEASUREMENT_FAULT *fault_handler )
{
	const char fname[] = "mxfh_autoscale_reset()";

	MX_AUTOSCALE_MEASUREMENT_FAULT *autoscale_fault_struct;
	mx_status_type mx_status;

	mx_status = mxfh_autoscale_get_pointers( fault_handler,
					&autoscale_fault_struct, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for autoscale scaler '%s', reset_flags = %d.",
			fname,
			autoscale_fault_struct->autoscale_scaler_record->name,
			fault_handler->reset_flags ));

	if ( fault_handler->reset_flags ==
				MXMF_PREPARE_FOR_FIRST_MEASUREMENT_ATTEMPT )
	{
		/* Do nothing before the first actual measurement in 
		 * a measurement fault loop.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	MX_DEBUG( 2,("%s: sending %d.",
			fname, autoscale_fault_struct->change_request ));

	mx_status = mx_autoscale_change_control(
			autoscale_fault_struct->autoscale_record,
			autoscale_fault_struct->change_request );

	return mx_status;
}

