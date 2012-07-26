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

#define MXI_NI_DAQMX_DEBUG		FALSE

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
	mx_status_type mx_status;

	mx_status = mxi_ni_daqmx_get_pointers( record,
						&ni_daqmx, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if defined(USE_DAQMX_BASE)
	mx_warning( "The National Instruments DAQmx Base system "
	"takes a _long_ time to initialize itself, so please be patient." );
#endif

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
mxi_ni_daqmx_create_task( MX_RECORD *record, TaskHandle *task_handle )
{
	static const char fname[] = "mxi_ni_daqmx_create_task()";

	char daqmx_error_message[80];
	int32 daqmx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	if ( task_handle == (TaskHandle *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The TaskHandle pointer passed was NULL." );
	}

	/* Create a DAQmx task. */

	daqmx_status = DAQmxCreateTask( "", task_handle );

#if MXI_NI_DAQMX_DEBUG
	MX_DEBUG(-2,
	("%s: DAQmxCreateTask( &task_handle ) = %d",
		fname, (int) daqmx_status ));

	MX_DEBUG(-2,("%s:   record '%s', task_handle = %#lx",
		fname, record->name, (unsigned long) *task_handle ));
#endif

	if ( daqmx_status != 0 ) {

		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to create a DAQmx task failed for '%s'.  "
		"DAQmx error code = %d, error message = '%s'",
			record->name, (int) daqmx_status, daqmx_error_message );
	}

#if ( defined(OS_LINUX) && USE_DAQMX_BASE )

	if ( (*task_handle) == 0 ) {

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
		"The attempt to create a TaskHandle for '%s' failed.  "
		"On Linux it is necessary to prefix the shell command that "
		"starts this program with the following environment variable "
		"definition:\n"
		"  LD_PRELOAD=/usr/local/lib/libnidaqmxbase.so\n"
		"This is an unfortunate consequence of the way that "
		"National Instruments has implemented DAQmx Base.",
			record->name );
	}
#else
	if ( (*task_handle) == 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to create a TaskHandle for '%s' failed.",
			record->name );
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni_daqmx_shutdown_task( MX_RECORD *record, TaskHandle task_handle )
{
	static const char fname[] = "mxi_ni_daqmx_shutdown_task()";

	char daqmx_error_message[80];
	int32 daqmx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	/* Stop the task. */

	daqmx_status = DAQmxStopTask( task_handle );

#if MXI_NI_DAQMX_DEBUG
	MX_DEBUG(-2,("%s: DAQmxStopTask( %#lx ) = %d",
		fname, (unsigned long) task_handle, (int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {

		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to stop task %#lx for DAQmx device '%s' "
		"failed.  DAQmx error code = %d",
			(unsigned long) task_handle,
			record->name,
			(int) daqmx_status );
	}

#if MXI_NI_DAQMX_DEBUG
	MX_DEBUG(-2,("%s: DAQmxClearTask( %#lx ) = %d",
		fname, (unsigned long) task_handle, (int) daqmx_status));
#endif

	/* Release the resources used by this task. */

	daqmx_status = DAQmxClearTask( task_handle );

	if ( daqmx_status != 0 ) {

		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to clear task %#lx for DAQmx device '%s' "
		"failed.  DAQmx error code = %d",
			(unsigned long) task_handle,
			record->name,
			(int) daqmx_status );
	}

	return MX_SUCCESSFUL_RESULT;
}

