/*
 * Name:    d_ni_daqmx_doutput.c
 *
 * Purpose: MX driver for NI-DAQmx digital output channels.
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

#define MXD_NI_DAQMX_DOUTPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_output.h"
#include "i_ni_daqmx.h"
#include "d_ni_daqmx_doutput.h"

MX_RECORD_FUNCTION_LIST mxd_ni_daqmx_doutput_record_function_list = {
	NULL,
	mxd_ni_daqmx_doutput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_ni_daqmx_doutput_open
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
		mxd_ni_daqmx_doutput_digital_output_function_list = {
	NULL,
	mxd_ni_daqmx_doutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_ni_daqmx_doutput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_NI_DAQMX_DOUTPUT_STANDARD_FIELDS
};

long mxd_ni_daqmx_doutput_num_record_fields
	= sizeof( mxd_ni_daqmx_doutput_record_field_defaults )
		/ sizeof( mxd_ni_daqmx_doutput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ni_daqmx_doutput_rfield_def_ptr
			= &mxd_ni_daqmx_doutput_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_ni_daqmx_doutput_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_NI_DAQMX_DOUTPUT **ni_daqmx_doutput,
			MX_NI_DAQMX **ni_daqmx,
			const char *calling_fname )
{
	static const char fname[] = "mxd_ni_daqmx_doutput_get_pointers()";

	MX_NI_DAQMX_DOUTPUT *ni_daqmx_doutput_ptr;
	MX_RECORD *ni_daqmx_record;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	ni_daqmx_doutput_ptr = (MX_NI_DAQMX_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( ni_daqmx_doutput_ptr == (MX_NI_DAQMX_DOUTPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NI_DAQMX_DOUTPUT pointer for "
			"DAQmx digital output '%s' passed by '%s' is NULL",
				doutput->record->name, calling_fname );
	}

	if ( ni_daqmx_doutput != (MX_NI_DAQMX_DOUTPUT **) NULL ) {
		*ni_daqmx_doutput = ni_daqmx_doutput_ptr;
	}

	if ( ni_daqmx != (MX_NI_DAQMX **) NULL ) {
		ni_daqmx_record = ni_daqmx_doutput_ptr->ni_daqmx_record;

		if ( ni_daqmx_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The ni_daqmx_record pointer for "
			"DAQmx digital output '%s' is NULL.",
				doutput->record->name );
		}

		*ni_daqmx = (MX_NI_DAQMX *) ni_daqmx_record->record_type_struct;

		if ( (*ni_daqmx) == (MX_NI_DAQMX *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NI_DAQMX pointer for DAQmx record '%s' "
			"used by digital output '%s' is NULL.",
				ni_daqmx_record->name,
				doutput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_ni_daqmx_doutput_create_record_structures( MX_RECORD *record )
{
        const char fname[] =
		"mxd_ni_daqmx_doutput_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_NI_DAQMX_DOUTPUT *ni_daqmx_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        ni_daqmx_doutput = (MX_NI_DAQMX_DOUTPUT *)
				malloc( sizeof(MX_NI_DAQMX_DOUTPUT) );

        if ( ni_daqmx_doutput == (MX_NI_DAQMX_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_NI_DAQMX_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = ni_daqmx_doutput;
        record->class_specific_function_list
			= &mxd_ni_daqmx_doutput_digital_output_function_list;

        digital_output->record = record;
	ni_daqmx_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ni_daqmx_doutput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_ni_daqmx_doutput_open()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_NI_DAQMX_DOUTPUT *ni_daqmx_doutput = NULL;
	MX_NI_DAQMX *ni_daqmx = NULL;
	MX_NI_DAQMX_TASK *task = NULL;
	char daqmx_error_message[400];
	int32 daqmx_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	doutput = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	mx_status = mxd_ni_daqmx_doutput_get_pointers(
				doutput, &ni_daqmx_doutput, &ni_daqmx, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Create a DAQmx task. */

	mx_status = mxi_ni_daqmx_find_or_create_task( ni_daqmx,
						ni_daqmx_doutput->task_name,
						&task );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_ni_daqmx_set_task_datatype( task, MXFT_ULONG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Associate a digital output channel with this task. */

	daqmx_status = DAQmxCreateDOChan( task->task_handle,
					ni_daqmx_doutput->channel_name, NULL,
					DAQmx_Val_ChanForAllLines );

#if MXD_NI_DAQMX_DOUTPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxCreateDOChan( %#lx, '%s', NULL, %#lx ) = %d",
		fname, (unsigned long) task->task_handle,
		ni_daqmx_doutput->channel_name,
		(unsigned long) DAQmx_Val_ChanForAllLines,
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {

		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to associate digital output '%s' with "
		"DAQmx task %#lx failed.  "
		"DAQmx error code = %d, error message = '%s'",
			record->name,
			(unsigned long) task->task_handle,
			(int) daqmx_status, daqmx_error_message );
	}

	/* Save the channel offset in the task. */

	ni_daqmx_doutput->task = task;

	ni_daqmx_doutput->channel_offset = task->num_channels;

	task->num_channels++;

	/* The task will be started in the "finish delayed initialization"
	 * driver function of the "ni_daqmx" record.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ni_daqmx_doutput_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_ni_daqmx_doutput_write()";

	MX_NI_DAQMX_DOUTPUT *ni_daqmx_doutput = NULL;
	MX_NI_DAQMX *ni_daqmx = NULL;
	MX_NI_DAQMX_TASK *task = NULL;
	unsigned long channel;
	int32 *channel_buffer;
	char daqmx_error_message[400];
	int32 daqmx_status;
	int32 num_samples;
	bool32 autostart;
	double timeout;
	int32 num_samples_written;
	mx_status_type mx_status;

	mx_status = mxd_ni_daqmx_doutput_get_pointers(
				doutput, &ni_daqmx_doutput, &ni_daqmx, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	task           = ni_daqmx_doutput->task;
	channel        = ni_daqmx_doutput->channel_offset;
	channel_buffer = task->channel_buffer;

	if ( channel_buffer == NULL ) {
		mx_warning( "Task not yet started for NI-DAQmx device '%s'.  "
			"The attempt to write %lu to '%s' was ignored.",
			doutput->record->name,
			doutput->value,
			doutput->record->name );

		return MX_SUCCESSFUL_RESULT;
	}

	num_samples       = 1;
	autostart         = FALSE;
	timeout           = 10.0;    /* write timeout in seconds */

	channel_buffer[channel] = doutput->value;

	daqmx_status = DAQmxWriteDigitalU32( task->task_handle,
					num_samples, autostart, timeout,
					DAQmx_Val_GroupByChannel,
					channel_buffer,
					&num_samples_written, NULL );

#if MXD_NI_DAQMX_DOUTPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxWriteDigitalU32( "
	"%#lx, %lu, %lu, %f, %#x, {%lu}, &num_samples, NULL ) = %d",
		fname, (unsigned long) task->task_handle,
		num_samples,
		autostart,
		timeout,
		DAQmx_Val_GroupByChannel,
		channel_buffer[channel],
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {

		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to write digital output '%s' failed.  "
		"DAQmx error code = %d, error message = '%s'",
			doutput->record->name,
			(int) daqmx_status, daqmx_error_message );
	}

#if MXD_NI_DAQMX_DOUTPUT_DEBUG
	MX_DEBUG(-2,("%s:   num_samples_written = %lu",
		fname, num_samples_written));
#endif

	return mx_status;
}

