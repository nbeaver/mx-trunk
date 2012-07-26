/*
 * Name:    i_ni_daqmx.c
 *
 * Purpose: MX interface driver for the National Instruments DAQmx system.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011-2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_NI_DAQMX_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "i_ni_daqmx.h"

MX_RECORD_FUNCTION_LIST mxi_ni_daqmx_record_function_list = {
	NULL,
	mxi_ni_daqmx_create_record_structures,
	mxi_ni_daqmx_finish_record_initialization,
	NULL,
	NULL,
	mxi_ni_daqmx_open,
	mxi_ni_daqmx_close
};

MX_RECORD_FIELD_DEFAULTS mxi_ni_daqmx_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_NI_DAQMX_STANDARD_FIELDS
};

long mxi_ni_daqmx_num_record_fields
		= sizeof( mxi_ni_daqmx_record_field_defaults )
			/ sizeof( mxi_ni_daqmx_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_ni_daqmx_rfield_def_ptr
			= &mxi_ni_daqmx_record_field_defaults[0];

static mx_status_type
mxi_ni_daqmx_get_pointers( MX_RECORD *record,
				MX_NI_DAQMX **ni_daqmx,
				const char *calling_fname )
{
	static const char fname[] = "mxi_ni_daqmx_get_pointers()";

	MX_NI_DAQMX *ni_daqmx_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	ni_daqmx_ptr = (MX_NI_DAQMX *) record->record_type_struct;

	if ( ni_daqmx_ptr == (MX_NI_DAQMX *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NI_DAQMX pointer for record '%s' is NULL.",
			record->name );
	}

	if ( ni_daqmx != (MX_NI_DAQMX **) NULL ) {
		*ni_daqmx = ni_daqmx_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxi_ni_daqmx_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_ni_daqmx_create_record_structures()";

	MX_NI_DAQMX *ni_daqmx;

	/* Allocate memory for the necessary structures. */

	ni_daqmx = (MX_NI_DAQMX *) malloc( sizeof(MX_NI_DAQMX) );

	if ( ni_daqmx == (MX_NI_DAQMX *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_NI_DAQMX structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = ni_daqmx;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	ni_daqmx->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni_daqmx_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_ni_daqmx_finish_record_initialization()";

	MX_NI_DAQMX *ni_daqmx;
	mx_status_type mx_status;

	mx_status = mxi_ni_daqmx_get_pointers( record,
						&ni_daqmx, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni_daqmx_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_ni_daqmx_open()";

	MX_NI_DAQMX *ni_daqmx;
	MX_LIST *task_list;
	mx_status_type mx_status;

	mx_status = mxi_ni_daqmx_get_pointers( record,
						&ni_daqmx, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if defined(USE_DAQMX_BASE)
	mx_warning( "The National Instruments DAQmx Base system "
	"takes a _long_ time to initialize itself, so please be patient." );
#endif

	/* Create an empty MX DAQmx task list.  This list is used to save
	 * information about DAQmx task handles that have to be shared
	 * between multiple devices.  An example would be a set of 
	 * analog input pins that are read out using a single ADC that
	 * is multiplexed between them.  All of these pins must share
	 * the same task, since the ADC itself cannot be shared between
	 * tasks in a straightforward way.
	 */

	mx_status = mx_list_create( &task_list );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ni_daqmx->task_list = task_list;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni_daqmx_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_ni_daqmx_close()";

	MX_NI_DAQMX *ni_daqmx;
	mx_status_type mx_status;

	mx_status = mxi_ni_daqmx_get_pointers( record,
						&ni_daqmx, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if defined(USE_DAQMX_BASE)
	mx_warning( "Shutting down the National Instruments DAQmx Base "
	"system.  This can take a _long_ time." );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------- Exported driver-specific functions ---------------*/

MX_EXPORT mx_status_type
mxi_ni_daqmx_create_task( MX_NI_DAQMX *ni_daqmx,
			char *task_name,
			MX_NI_DAQMX_TASK **task )
{
	static const char fname[] = "mxi_ni_daqmx_create_task()";

	MX_LIST_ENTRY *task_list_entry;
	TaskHandle task_handle;
	char daqmx_error_message[80];
	int32 daqmx_status;
	mx_status_type mx_status;

	if ( ni_daqmx == (MX_NI_DAQMX *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NI_DAQMX pointer passed was NULL." );
	}
	if ( task_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The task_name pointer passed was NULL." );
	}
	if ( task == (MX_NI_DAQMX_TASK **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NI_DAQMX_TASK pointer passed was NULL." );
	}

	/* Create a DAQmx task. */

	daqmx_status = DAQmxCreateTask( task_name, &task_handle );

#if MXI_NI_DAQMX_DEBUG
	MX_DEBUG(-2,
	("%s: DAQmxCreateTask( '%s', &task_handle ) = %d",
		fname, task_name, (int) daqmx_status ));

	MX_DEBUG(-2,("%s:   task_handle = %#lx",
		fname, (unsigned long) task_handle ));
#endif

	if ( daqmx_status != 0 ) {

		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to create a DAQmx task named '%s' failed.  "
		"DAQmx error code = %d, error message = '%s'",
			task_name, (int) daqmx_status, daqmx_error_message );
	}

#if ( defined(OS_LINUX) && USE_DAQMX_BASE )

	if ( task_handle == 0 ) {

		/* In case you care, National Instruments DAQmx Base is
		 * implemented using a large LabVIEW system that runs 
		 * in the background.  Apparently this system only works
		 * if it can do some initialization steps before the
		 * main() routine of your program is invoked.  If you
		 * wait until dlopen() time, then it is too late.
		 *
		 * The only ways that I know of in Linux to work around
		 * this are:
		 *
		 * 1.  Link the National Instruments libraries into your
		 *     executable that runs main().  This is very _bad_
		 *     for any plugin system like an MX module (or a
		 *     Python module for that matter).
		 *
		 * 2.  Use LD_PRELOAD to load libnidaqmxbase.so before
		 *     main is invoked.  This is the solution that I
		 *     suggest in the error message below.
		 *
		 * If anyone can come up with a way around this, then
		 * I would love to hear about it.  Apparently, National
		 * Instruments's response to issues like this is to declare
		 * that they do not support plugin architectures.  Swell.
		 */

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to create a DAQmx Base TaskHandle "
		"for task name '%s' failed.  "
		"On Linux it is necessary to prefix the shell command that "
		"starts this program with the following environment variable "
		"definition:\n"
		"  LD_PRELOAD=/usr/local/lib/libnidaqmxbase.so\n"
		"This is an unfortunate consequence of the way that "
		"National Instruments has implemented DAQmx Base.",
			task_name );
	}
#else
	if ( task_handle == 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to create a TaskHandle for task name '%s' failed.",
			task_name );
	}
#endif
	/* Create an NI_DAQMX_TASK object to contain the task information. */

	*task = (MX_NI_DAQMX_TASK *) malloc( sizeof(MX_NI_DAQMX_TASK) );

	if ( (*task) == (MX_NI_DAQMX_TASK *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"The attempt to allocate memory for an MX_NI_DAQMX_TASK "
		"structure failed for task name '%s'", task_name );
	}

	strlcpy( (*task)->task_name, task_name, MXU_NI_DAQMX_TASK_NAME_LENGTH );

	(*task)->task_handle = task_handle;

	/* Add the task to the master list of tasks. */

	mx_status = mx_list_entry_create( &task_list_entry, *task, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_list_add_entry( ni_daqmx->task_list, task_list_entry );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_ni_daqmx_shutdown_task( MX_NI_DAQMX *ni_daqmx, MX_NI_DAQMX_TASK *task )
{
	static const char fname[] = "mxi_ni_daqmx_shutdown_task()";

	MX_LIST_ENTRY *task_list_entry;
	char daqmx_error_message[80];
	int32 daqmx_status;
	mx_status_type mx_status;

	if ( ni_daqmx == (MX_NI_DAQMX *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NI_DAQMX pointer passed was NULL." );
	}

	/* If the task pointer passed to us is NULL, then there is nothing
	 * for us to do.  The most useful thing we can do in this situation
	 * is to return a success status.
	 */

	if ( task == (MX_NI_DAQMX_TASK *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Stop the task. */

	daqmx_status = DAQmxStopTask( task->task_handle );

#if MXI_NI_DAQMX_DEBUG
	MX_DEBUG(-2,("%s: DAQmxStopTask( %#lx ) for task '%s' = %d",
		fname, (unsigned long) task->task_handle,
		task->task_name, (int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {

		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to stop DAQmx task '%s' (handle %#lx) failed.  "
		"DAQmx error code = %d",
			task->task_name,
			(unsigned long) task->task_handle,
			(int) daqmx_status );
	}

#if MXI_NI_DAQMX_DEBUG
	MX_DEBUG(-2,("%s: DAQmxClearTask( %#lx ) for task '%s' = %d",
		fname, task->task_name,
		(unsigned long) task->task_handle,
		(int) daqmx_status));
#endif

	/* Release the resources used by this task. */

	daqmx_status = DAQmxClearTask( task->task_handle );

	if ( daqmx_status != 0 ) {

		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to clear DAQmx task '%s' (handle %#lx) failed.  "
		"DAQmx error code = %d",
			task->task_handle,
			(unsigned long) task->task_handle,
			(int) daqmx_status );
	}

	/* Find the list entry in the master list. */

	mx_status = mx_list_find_list_entry( ni_daqmx->task_list, task,
						&task_list_entry );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_list_delete_entry( ni_daqmx->task_list, task_list_entry);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_free( task_list_entry->list_entry_data );

	mx_list_entry_destroy( task_list_entry );

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------*/

static mx_status_type
mxp_ni_daqmx_task_list_traverse_fn( MX_LIST_ENTRY *list_entry,
					void *task_name_ptr,
					void **task_ptr )
{
#if MXI_NI_DAQMX_DEBUG
	static const char fname[] = "mxp_ni_daqmx_task_list_traverse_fn()";
#endif

	MX_NI_DAQMX_TASK *task;
	char *task_name;

	task = list_entry->list_entry_data;

	task_name = task_name_ptr;

#if MXI_NI_DAQMX_DEBUG
	MX_DEBUG(-2,("%s: Checking task '%s' for match to '%s'.",
		fname, task->task_name, task_name ));
#endif

	if ( strcmp( task->task_name, task_name ) == 0 ) {
		*task_ptr = (void *) task;

		return mx_error( MXE_EARLY_EXIT | MXE_QUIET, "", " " );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni_daqmx_find_task( MX_NI_DAQMX *ni_daqmx,
			char *task_name,
			MX_NI_DAQMX_TASK **task )
{
	static const char fname[] = "mxi_ni_daqmx_find_task()";

	MX_NI_DAQMX_TASK *task_ptr;
	mx_status_type mx_status;

	if ( ni_daqmx == (MX_NI_DAQMX *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NI_DAQMX pointer passed was NULL." );
	}
	if ( task_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The task_name pointer passed was NULL." );
	}
	if ( task == (MX_NI_DAQMX_TASK **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NI_DAQMX_TASK pointer passed was NULL." );
	}

	if ( strlen( task_name ) == 0 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Cannot find a task with a zero-length name." );
	}

	mx_status = mx_list_traverse( ni_daqmx->task_list,
					mxp_ni_daqmx_task_list_traverse_fn,
					task_name,
					&task_ptr );

	switch( mx_status.code ) {
	case MXE_EARLY_EXIT:
		/* We found the task. */

		*task = task_ptr;

		return MX_SUCCESSFUL_RESULT;
		break;

	case MXE_SUCCESS:
		/* In this case, MXE_SUCCESS actually means we did _not_
		 * find the task.
		 */

		*task = NULL;

		return mx_error( MXE_NOT_FOUND | MXE_QUIET, fname,
			"DAQmx task '%s' was not found.", task_name );
		break;
	default:
		*task = NULL;

		return mx_status;
		break;
	}
}

/*------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_ni_daqmx_find_or_create_task( MX_NI_DAQMX *ni_daqmx,
				char *task_name,
				MX_NI_DAQMX_TASK **task )
{
	static const char fname[] = "mxi_ni_daqmx_find_or_create_task()";

	mx_status_type mx_status;

	mx_status = mxi_ni_daqmx_find_task( ni_daqmx, task_name, task );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( (*task) != NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mxi_ni_daqmx_create_task( ni_daqmx, task_name, task );

	return mx_status;
}

