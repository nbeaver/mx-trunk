/*
 * Name:    d_ni_daqmx_aoutput.c
 *
 * Purpose: MX driver for NI-DAQmx analog output channels.
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

#define MXD_NI_DAQMX_AOUTPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_output.h"
#include "i_ni_daqmx.h"
#include "d_ni_daqmx_aoutput.h"

/* Initialize the aoutput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_ni_daqmx_aoutput_record_function_list = {
	NULL,
	mxd_ni_daqmx_aoutput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_ni_daqmx_aoutput_open
};

MX_ANALOG_OUTPUT_FUNCTION_LIST
		mxd_ni_daqmx_aoutput_analog_output_function_list =
{
	NULL,
	mxd_ni_daqmx_aoutput_write
};

/* DAQmx analog output data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_ni_daqmx_aoutput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_NI_DAQMX_AOUTPUT_STANDARD_FIELDS
};

long mxd_ni_daqmx_aoutput_num_record_fields
		= sizeof( mxd_ni_daqmx_aoutput_rf_defaults )
		  / sizeof( mxd_ni_daqmx_aoutput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ni_daqmx_aoutput_rfield_def_ptr
			= &mxd_ni_daqmx_aoutput_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_ni_daqmx_aoutput_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_NI_DAQMX_AOUTPUT **ni_daqmx_aoutput,
			MX_NI_DAQMX **ni_daqmx,
			const char *calling_fname )
{
	static const char fname[] = "mxd_ni_daqmx_aoutput_get_pointers()";

	MX_NI_DAQMX_AOUTPUT *ni_daqmx_aoutput_ptr;
	MX_RECORD *ni_daqmx_record;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_OUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	ni_daqmx_aoutput_ptr = (MX_NI_DAQMX_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( ni_daqmx_aoutput_ptr == (MX_NI_DAQMX_AOUTPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NI_DAQMX_AOUTPUT pointer for "
			"DAQmx analog output '%s' passed by '%s' is NULL",
				aoutput->record->name, calling_fname );
	}

	if ( ni_daqmx_aoutput != (MX_NI_DAQMX_AOUTPUT **) NULL ) {
		*ni_daqmx_aoutput = ni_daqmx_aoutput_ptr;
	}

	if ( ni_daqmx != (MX_NI_DAQMX **) NULL ) {
		ni_daqmx_record = ni_daqmx_aoutput_ptr->ni_daqmx_record;

		if ( ni_daqmx_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The ni_daqmx_record pointer for "
			"DAQmx analog output '%s' is NULL.",
				aoutput->record->name );
		}

		*ni_daqmx = (MX_NI_DAQMX *) ni_daqmx_record->record_type_struct;

		if ( (*ni_daqmx) == (MX_NI_DAQMX *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NI_DAQMX pointer for DAQmx record '%s' "
			"used by analog output '%s' is NULL.",
				ni_daqmx_record->name,
				aoutput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_ni_daqmx_aoutput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_ni_daqmx_aoutput_create_record_structures()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_NI_DAQMX_AOUTPUT *ni_daqmx_aoutput;

	/* Allocate memory for the necessary structures. */

	aoutput = (MX_ANALOG_OUTPUT *) malloc( sizeof(MX_ANALOG_OUTPUT) );

	if ( aoutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_ANALOG_OUTPUT structure." );
	}

	ni_daqmx_aoutput = (MX_NI_DAQMX_AOUTPUT *)
				malloc( sizeof(MX_NI_DAQMX_AOUTPUT) );

	if ( ni_daqmx_aoutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_NI_DAQMX_AOUTPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = aoutput;
	record->record_type_struct = ni_daqmx_aoutput;
	record->class_specific_function_list
			= &mxd_ni_daqmx_aoutput_analog_output_function_list;

	aoutput->record = record;

	aoutput->subclass = MXT_AOU_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ni_daqmx_aoutput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_ni_daqmx_aoutput_open()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_NI_DAQMX_AOUTPUT *ni_daqmx_aoutput = NULL;
	MX_NI_DAQMX *ni_daqmx = NULL;
	MX_NI_DAQMX_TASK *task = NULL;
	char daqmx_error_message[400];
	int32 daqmx_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	aoutput = (MX_ANALOG_OUTPUT *) record->record_class_struct;

	mx_status = mxd_ni_daqmx_aoutput_get_pointers(
				aoutput, &ni_daqmx_aoutput, &ni_daqmx, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Create a DAQmx task. */

	mx_status = mxi_ni_daqmx_find_or_create_task( ni_daqmx,
					ni_daqmx_aoutput->task_name,
					&task );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_ni_daqmx_set_task_datatype( task, MXFT_DOUBLE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Associate a analog output channel with this task. */

	daqmx_status = DAQmxCreateAOVoltageChan(
					task->task_handle,
					ni_daqmx_aoutput->channel_name, NULL,
					ni_daqmx_aoutput->minimum_value,
					ni_daqmx_aoutput->maximum_value,
					DAQmx_Val_Volts, NULL );

#if MXD_NI_DAQMX_AOUTPUT_DEBUG
	MX_DEBUG(-2,
	("%s: DAQmxCreateAOVoltageChan( %#lx, '%s', NULL, %#lx ) = %d",
		fname, (unsigned long) task->task_handle,
		ni_daqmx_aoutput->channel_name,
		(unsigned long) DAQmx_Val_ChanForAllLines,
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {

		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to associate analog output '%s' with "
		"DAQmx task %#lx failed.  "
		"DAQmx error code = %d, error message = '%s'",
			record->name,
			(unsigned long) task->task_handle,
			(int) daqmx_status, daqmx_error_message );
	}

	/* Save the channel offset in the task. */

	ni_daqmx_aoutput->task = task;

	ni_daqmx_aoutput->channel_offset = task->num_channels;

	task->num_channels++;

	/* The task will be started in the "finish delayed initialization"
	 * driver function of the "ni_daqmx" record.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ni_daqmx_aoutput_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_ni_daqmx_aoutput_write()";

	MX_NI_DAQMX_AOUTPUT *ni_daqmx_aoutput = NULL;
	MX_NI_DAQMX *ni_daqmx = NULL;
	MX_NI_DAQMX_TASK *task = NULL;
	unsigned long channel;
	float64 *channel_buffer;
	char daqmx_error_message[400];
	int32 daqmx_status;
	int32 num_samples;
	bool32 autostart;
	double timeout;
	int32 num_samples_written;
	mx_status_type mx_status;

	ni_daqmx_aoutput = NULL;

	mx_status = mxd_ni_daqmx_aoutput_get_pointers(
				aoutput, &ni_daqmx_aoutput, &ni_daqmx, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	task           = ni_daqmx_aoutput->task;
	channel        = ni_daqmx_aoutput->channel_offset;
	channel_buffer = task->channel_buffer;

	if ( channel_buffer == NULL ) {
		mx_warning( "Task not yet started for NI-DAQmx device '%s'.  "
			"The attempt to write %g to '%s' was ignored.",
			aoutput->record->name,
			aoutput->raw_value.double_value,
			aoutput->record->name );

		return MX_SUCCESSFUL_RESULT;
	}

	num_samples       = 1;
	autostart         = FALSE;
	timeout           = 10.0;    /* write timeout in seconds */

	channel_buffer[channel] = aoutput->raw_value.double_value;

	daqmx_status = DAQmxWriteAnalogF64( task->task_handle,
					num_samples, autostart, timeout,
					DAQmx_Val_GroupByChannel,
					channel_buffer, 
					&num_samples_written, NULL );

#if MXD_NI_DAQMX_AOUTPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxWriteAnalogF64( "
	"%#lx, %lu, %lu, %f, %#x, {%f}, &num_samples, NULL ) = %d",
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
		"The attempt to write analog output '%s' failed.  "
		"DAQmx error code = %d, error message = '%s'",
			aoutput->record->name,
			(int) daqmx_status, daqmx_error_message );
	}

#if MXD_NI_DAQMX_AOUTPUT_DEBUG
	MX_DEBUG(-2,("%s:   num_samples_written = %lu",
		fname, num_samples_written));
#endif

	return mx_status;
}

