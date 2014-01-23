/*
 * Name:    i_ni_daqmx.c
 *
 * Purpose: MX interface driver for the National Instruments DAQmx system.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011-2012, 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_NI_DAQMX_DEBUG			FALSE

#define MXI_NI_DAQMX_DEBUG_TASK_POINTER		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "i_ni_daqmx.h"

#if MXI_NI_DAQMX_DEBUG_TASK_POINTER
#  if defined( OS_WIN32 )
#    include <windows.h>
#  endif
#endif

MX_RECORD_FUNCTION_LIST mxi_ni_daqmx_record_function_list = {
	NULL,
	mxi_ni_daqmx_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_ni_daqmx_open,
	mxi_ni_daqmx_close,
	mxi_ni_daqmx_finish_delayed_initialization
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

#if USE_DAQMX_BASE
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

/*--------------------------------------------------------------------------*/

static mx_status_type
mxp_ni_daqmx_task_shutdown_traverse_fn( MX_LIST_ENTRY *list_entry,
					void *ni_daqmx_ptr,
					void **unused_output )
{
	MX_NI_DAQMX *ni_daqmx;
	MX_NI_DAQMX_TASK *task;
	mx_status_type mx_status;

	ni_daqmx = ni_daqmx_ptr;

	task = list_entry->list_entry_data;

	mx_status = mxi_ni_daqmx_shutdown_task( ni_daqmx, task );

	return mx_status;
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

#if USE_DAQMX_BASE
	mx_warning( "Shutting down the National Instruments DAQmx Base "
	"system.  This can take a _long_ time." );
#endif

	/* Shutdown all of the DAQmx tasks that were registered with
	 * this driver.
	 */

	mx_status = mx_list_traverse( ni_daqmx->task_list,
					mxp_ni_daqmx_task_shutdown_traverse_fn,
					ni_daqmx, NULL );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxp_ni_daqmx_task_init_traverse_fn( MX_LIST_ENTRY *list_entry,
					void *unused_input,
					void **unused_output )
{
	static const char fname[] = "mxp_ni_daqmx_task_start_traverse_fn()";

	MX_NI_DAQMX_TASK *task;
	char daqmx_error_message[400];
	int32 daqmx_status;

	task = list_entry->list_entry_data;

	/* Create a channel buffer for the task. */

	switch( task->mx_datatype ) {
	case MXFT_CHAR:
	case MXFT_UCHAR:
		task->channel_buffer =
			malloc( task->num_channels * sizeof(int8) );
		break;
	case MXFT_SHORT:
	case MXFT_USHORT:
		task->channel_buffer =
			malloc( task->num_channels * sizeof(int16) );
		break;
	case MXFT_LONG:
	case MXFT_ULONG:
		task->channel_buffer =
			malloc( task->num_channels * sizeof(int32) );
		break;
	case MXFT_DOUBLE:
		task->channel_buffer =
			malloc( task->num_channels * sizeof(float64) );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Attempted to allocate a channel buffer for DAQmx task '%s' "
		"(handle %#lx) with unsupported MX datatype %ld.",
			task->task_name, task->task_handle, task->mx_datatype );
		break;
	}

	if ( task->channel_buffer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"The attempt to allocate a %ld element channel buffer "
		"for DAQmx task '%s' (handle %#lx) failed.",
			task->num_channels, task->task_name, task->task_handle);
	}

	/* Start the task. */

	daqmx_status = DAQmxStartTask( task->task_handle );

#if MXI_NI_DAQMX_DEBUG
	MX_DEBUG(-2,("%s: Started task '%s' (handle %#lx), daqmx_status = %d",
		fname, task->task_name, task->task_handle,
		(int) daqmx_status ));
#endif

	if ( daqmx_status != 0 ) {
		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to start task '%s' (handle %#lx) failed.  "
		"DAQmx error code = %d, error message = '%s'",
			task->task_name, (unsigned long) task->task_handle,
			(int) daqmx_status, daqmx_error_message );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni_daqmx_finish_delayed_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxi_ni_daqmx_close()";

	MX_NI_DAQMX *ni_daqmx;
	mx_status_type mx_status;

	mx_status = mxi_ni_daqmx_get_pointers( record,
						&ni_daqmx, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Walk through all of the DAQmx tasks that were created
	 * by the drivers for the various DAQmx devices.  For each
	 * driver, we do this:
	 *
	 * 1. Create a datatype appropriate channel buffer for the task.
	 *
	 * 2. Start the task.
	 */

	mx_status = mx_list_traverse( ni_daqmx->task_list,
					mxp_ni_daqmx_task_init_traverse_fn,
					NULL, NULL );

	return mx_status;
}

/*------------------------------------------------------------------*/

static void
mxi_ni_daqmx_task_list_entry_destructor( void *list_entry_data )
{
	static const char fname[] = "mxi_ni_daqmx_task_list_entry_destructor()";

	MX_NI_DAQMX_TASK *task;

	if ( list_entry_data == NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The list_entry_data pointer passed was NULL.  "
		"Stack traceback will be shown." );

		mx_stack_traceback();

		return;
	}

	task = (MX_NI_DAQMX_TASK *) list_entry_data;

#if 0
	MX_DEBUG(-2,("%s: task = %p", fname, task));
	MX_DEBUG(-2,("%s: task_name = '%s'", fname, task->task_name));
	MX_DEBUG(-2,("%s: task_handle = %lu",
			fname, (unsigned long) task->task_handle));
	MX_DEBUG(-2,("%s: mx_datatype = %ld", fname, task->mx_datatype));
	MX_DEBUG(-2,("%s: num_channels = %lu", fname, task->num_channels));
#endif

	if ( mx_heap_pointer_is_valid( task ) ) {
		free( task );
		return;
	}

	/* FIXME: If we get here, then something is wrong with the pointer. */

	MX_DEBUG(-2,("%s: Task pointer %p was invalid", fname, task));
	MX_DEBUG(-2,("%s: Task pointer %p was used by DAQmx task '%s'.",
		fname, task, task->task_name));

#if MXI_NI_DAQMX_DEBUG_TASK_POINTER
#  if defined( OS_WIN32 )
	{
		MEMORY_BASIC_INFORMATION memory_info;
		SIZE_T bytes_returned;

		bytes_returned = VirtualQuery( task,
					&memory_info, sizeof(memory_info) );

		MX_DEBUG(-2,("%s: bytes_returned = %lu",
			fname, bytes_returned));

		MX_DEBUG(-2,("%s: base address = %p",
			fname, memory_info.BaseAddress));
		MX_DEBUG(-2,("%s: allocation base = %p",
			fname, memory_info.AllocationBase));
		MX_DEBUG(-2,("%s: allocation protect = %#lx",
			fname, memory_info.AllocationProtect));
		MX_DEBUG(-2,("%s: region size = %lu",
			fname, (unsigned long) memory_info.RegionSize));
		MX_DEBUG(-2,("%s: state = %#lx", fname, memory_info.State));
		MX_DEBUG(-2,("%s: protect = %#lx", fname, memory_info.Protect));
		MX_DEBUG(-2,("%s: type = %#lx", fname, memory_info.Type));

		free( task );

		MX_DEBUG(-2,("%s: The task pointer has been freed.",fname));
	}
#  else
#    error mxi_ni_daqmx_task_list_entry_destructor() does not yet handle invalid pointers for this platform.
#  endif
#endif /* MXI_NI_DAQMX_DEBUG_TASK_POINTER */
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

	(*task)->mx_datatype = 0;

	(*task)->num_channels = 0;

	(*task)->channel_buffer = NULL;

	/* Add the task to the master list of tasks. */

	mx_status = mx_list_entry_create_and_add( ni_daqmx->task_list, *task,
				mxi_ni_daqmx_task_list_entry_destructor );

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

	/* Release the resources used by this task. */

	daqmx_status = DAQmxClearTask( task->task_handle );

#if MXI_NI_DAQMX_DEBUG
	MX_DEBUG(-2,("%s: DAQmxClearTask( %#lx ) for task '%s' = %d",
		fname, (unsigned long) task->task_handle,
		task->task_name,
		(int) daqmx_status));
#endif

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

	mx_status = mx_list_entry_find_and_destroy( ni_daqmx->task_list, task );

	return mx_status;
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
	mx_status_type mx_status;

	mx_status = mxi_ni_daqmx_find_task( ni_daqmx, task_name, task );

	if ( mx_status.code == MXE_SUCCESS ) {
		return MX_SUCCESSFUL_RESULT;
	} else
	if ( mx_status.code != MXE_NOT_FOUND ) {
		return mx_status;
	}

	/* If we get here mxi_ni_daqmx_find_task() returned MXE_NOT_FOUND,
	 * so we have to create the task ourself.
	 */

	mx_status = mxi_ni_daqmx_create_task( ni_daqmx, task_name, task );

	return mx_status;
}

/*------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_ni_daqmx_set_task_datatype( MX_NI_DAQMX_TASK *task,
				long mx_datatype )
{
	static const char fname[] = "mxi_ni_daqmx_set_task_datatype()";

	if ( task == (MX_NI_DAQMX_TASK *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NI_DAQMX_TASK pointer passed was NULL." );
	}

	if ( task->mx_datatype <= 0 ) {
		task->mx_datatype = mx_datatype;

		return MX_SUCCESSFUL_RESULT;
	}

	if ( task->mx_datatype != mx_datatype ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The requested MX datatype %ld does not match the "
		"existing datatype %ld for DAQmx task '%s' (handle %#lx).",
			mx_datatype, task->mx_datatype,
			task->task_name, task->task_handle );
	}

	return MX_SUCCESSFUL_RESULT;
}

