/*
 * Name:    d_daqmx_base_doutput.c
 *
 * Purpose: MX driver for NI-DAQmx Base digital output channels.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_DAQMX_BASE_DOUTPUT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_output.h"
#include "i_daqmx_base.h"
#include "d_daqmx_base_doutput.h"

MX_RECORD_FUNCTION_LIST mxd_daqmx_base_doutput_record_function_list = {
	NULL,
	mxd_daqmx_base_doutput_create_record_structures,
	mxd_daqmx_base_doutput_finish_record_initialization,
	NULL,
	NULL,
	mxd_daqmx_base_doutput_open,
	mxd_daqmx_base_doutput_close,
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
		mxd_daqmx_base_doutput_digital_output_function_list = {
	NULL,
	mxd_daqmx_base_doutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_daqmx_base_doutput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_DAQMX_BASE_DOUTPUT_STANDARD_FIELDS
};

long mxd_daqmx_base_doutput_num_record_fields
	= sizeof( mxd_daqmx_base_doutput_record_field_defaults )
		/ sizeof( mxd_daqmx_base_doutput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_daqmx_base_doutput_rfield_def_ptr
			= &mxd_daqmx_base_doutput_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_daqmx_base_doutput_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_DAQMX_BASE_DOUTPUT **daqmx_base_doutput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_daqmx_base_doutput_get_pointers()";

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( daqmx_base_doutput == (MX_DAQMX_BASE_DOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DAQMX_BASE_DOUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	*daqmx_base_doutput = (MX_DAQMX_BASE_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( *daqmx_base_doutput == (MX_DAQMX_BASE_DOUTPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_DAQMX_BASE_DOUTPUT pointer for "
			"doutput record '%s' passed by '%s' is NULL",
				doutput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_daqmx_base_doutput_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_daqmx_base_doutput_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_DAQMX_BASE_DOUTPUT *daqmx_base_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *) malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        daqmx_base_doutput = (MX_DAQMX_BASE_DOUTPUT *)
				malloc( sizeof(MX_DAQMX_BASE_DOUTPUT) );

        if ( daqmx_base_doutput == (MX_DAQMX_BASE_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_DAQMX_BASE_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = daqmx_base_doutput;
        record->class_specific_function_list
			= &mxd_daqmx_base_doutput_digital_output_function_list;

        digital_output->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_daqmx_base_doutput_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_daqmx_base_doutput_finish_record_initialization()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_DAQMX_BASE_DOUTPUT *daqmx_base_doutput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	doutput = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	mx_status = mxd_daqmx_base_doutput_get_pointers(
				doutput, &daqmx_base_doutput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_daqmx_base_doutput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_daqmx_base_doutput_open()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_DAQMX_BASE_DOUTPUT *daqmx_base_doutput = NULL;
	int32 daqmx_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	doutput = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	mx_status = mxd_daqmx_base_doutput_get_pointers(
				doutput, &daqmx_base_doutput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Create a DAQmx Base task. */

	daqmx_status = DAQmxBaseCreateTask( "", &(daqmx_base_doutput->handle) );

#if MXD_DAQMX_BASE_DOUTPUT_DEBUG
	MX_DEBUG(-2,
	("%s: DAQmxBaseCreateTask( &(daqmx_base_doutput->handle) ) = %d",
		fname, (int) daqmx_status));

	MX_DEBUG(-2,("%s:   daqmx_base_doutput->handle = %#lx",
		fname, (unsigned long) daqmx_base_doutput->handle));
#endif

	if ( daqmx_status != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to create a DAQmx Base task failed for '%s'.  "
		"DAQmx error code = %d",
			record->name, (int) daqmx_status );
	}

#if 0
	if ( daqmx_base_doutput->handle == 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to create a TaskHandle for '%s' failed.",
			record->name );
	}
#endif

	/* Associate a digital output channel with this task. */

	daqmx_status = DAQmxBaseCreateDOChan( daqmx_base_doutput->handle,
					daqmx_base_doutput->channel_name, NULL,
					DAQmx_Val_ChanForAllLines );

#if MXD_DAQMX_BASE_DOUTPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxBaseCreateDOChan( %#lx, '%s', NULL, %#lx ) = %d",
		fname, (unsigned long) daqmx_base_doutput->handle,
		daqmx_base_doutput->channel_name,
		(unsigned long) DAQmx_Val_ChanForAllLines,
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to associate digital output '%s' with "
		"DAQmx Base task %#lx failed.  DAQmx error code = %d",
			record->name,
			(unsigned long) daqmx_base_doutput->handle,
			(int) daqmx_status );
	}

	/* Start the task. */

	daqmx_status = DAQmxBaseStartTask( daqmx_base_doutput->handle );

#if MXD_DAQMX_BASE_DOUTPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxBaseStartTask( %#lx ) = %d",
		fname, (unsigned long) daqmx_base_doutput->handle,
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to start task %#lx for digital output '%s' "
		"failed.  DAQmx error code = %d",
			(unsigned long) daqmx_base_doutput->handle,
			record->name,
			(int) daqmx_status );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_daqmx_base_doutput_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_daqmx_base_doutput_close()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_DAQMX_BASE_DOUTPUT *daqmx_base_doutput = NULL;
	int32 daqmx_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	doutput = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	mx_status = mxd_daqmx_base_doutput_get_pointers(
				doutput, &daqmx_base_doutput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( daqmx_base_doutput->handle != 0 ) {

		/* Stop the task. */

		daqmx_status = DAQmxBaseStopTask( daqmx_base_doutput->handle );

#if MXD_DAQMX_BASE_DOUTPUT_DEBUG
		MX_DEBUG(-2,("%s: DAQmxBaseStopTask( %#lx ) = %d",
			fname, (unsigned long) daqmx_base_doutput->handle,
			(int) daqmx_status));
#endif

		if ( daqmx_status != 0 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to stop task %#lx for digital output '%s' "
			"failed.  DAQmx error code = %d",
				(unsigned long) daqmx_base_doutput->handle,
				record->name,
				(int) daqmx_status );
		}

#if MXD_DAQMX_BASE_DOUTPUT_DEBUG
		MX_DEBUG(-2,("%s: DAQmxBaseClearTask( %#lx ) = %d",
			fname, (unsigned long) daqmx_base_doutput->handle,
			(int) daqmx_status));
#endif

		/* Release the resources used by this task. */

		daqmx_status = DAQmxBaseClearTask( daqmx_base_doutput->handle );

		if ( daqmx_status != 0 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to clear task %#lx for digital output '%s' "
			"failed.  DAQmx error code = %d",
				(unsigned long) daqmx_base_doutput->handle,
				record->name,
				(int) daqmx_status );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_daqmx_base_doutput_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_daqmx_base_doutput_write()";

	MX_DAQMX_BASE_DOUTPUT *daqmx_base_doutput;
	int32 daqmx_status;
	int32 num_samples;
	bool32 autostart;
	double timeout;
	uInt32 write_array[1];
	int32 num_samples_written;
	mx_status_type mx_status;

	daqmx_base_doutput = NULL;

	mx_status = mxd_daqmx_base_doutput_get_pointers(
				doutput, &daqmx_base_doutput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_samples       = 1;
	autostart         = FALSE;
	timeout           = 10.0;    /* write timeout in seconds */

	write_array[0] = doutput->value;

	daqmx_status = DAQmxBaseWriteDigitalU32( daqmx_base_doutput->handle,
					num_samples, autostart, timeout,
					DAQmx_Val_GroupByChannel,
					write_array, 
					&num_samples_written, NULL );

#if MXD_DAQMX_BASE_DOUTPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxBaseWriteDigitalU32( "
	"%#lx, %lu, %lu, %f, %#x, {%lu}, &num_samples, NULL ) = %d",
		fname, (unsigned long) daqmx_base_doutput->handle,
		num_samples,
		autostart,
		timeout,
		DAQmx_Val_GroupByChannel,
		write_array[0],
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to write digital output '%s' failed.  "
		"DAQmx error code = %d",
			doutput->record->name,
			(int) daqmx_status );
	}

#if MXD_DAQMX_BASE_DOUTPUT_DEBUG
	MX_DEBUG(-2,("%s:   num_samples_written = %lu",
		fname, num_samples_written));
#endif

	return mx_status;
}

