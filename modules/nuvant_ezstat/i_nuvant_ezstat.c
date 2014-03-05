/*
 * Name:    i_nuvant_ezstat.c
 *
 * Purpose: MX driver for NuVant EZstat controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_NUVANT_EZSTAT_DEBUG		FALSE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_digital_output.h"
#include "i_nuvant_ezstat.h"

MX_RECORD_FUNCTION_LIST mxi_nuvant_ezstat_record_function_list = {
	NULL,
	mxi_nuvant_ezstat_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_nuvant_ezstat_open
};

MX_RECORD_FIELD_DEFAULTS mxi_nuvant_ezstat_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_NUVANT_EZSTAT_STANDARD_FIELDS
};

long mxi_nuvant_ezstat_num_record_fields
		= sizeof( mxi_nuvant_ezstat_record_field_defaults )
		/ sizeof( mxi_nuvant_ezstat_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_nuvant_ezstat_rfield_def_ptr
			= &mxi_nuvant_ezstat_record_field_defaults[0];

static mx_status_type
mxi_nuvant_ezstat_get_pointers( MX_RECORD *record,
				MX_NUVANT_EZSTAT **nuvant_ezstat,
				const char *calling_fname )
{
	static const char fname[] = "mxi_nuvant_ezstat_get_pointers()";

	MX_NUVANT_EZSTAT *nuvant_ezstat_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	nuvant_ezstat_ptr = (MX_NUVANT_EZSTAT *) record->record_type_struct;

	if ( nuvant_ezstat_ptr == (MX_NUVANT_EZSTAT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NUVANT_EZSTAT pointer for record '%s' is NULL.",
			record->name );
	}

	if ( nuvant_ezstat != (MX_NUVANT_EZSTAT **) NULL ) {
		*nuvant_ezstat = nuvant_ezstat_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxi_nuvant_ezstat_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_nuvant_ezstat_create_record_structures()";

	MX_NUVANT_EZSTAT *nuvant_ezstat;

	/* Allocate memory for the necessary structures. */

	nuvant_ezstat = (MX_NUVANT_EZSTAT *) malloc( sizeof(MX_NUVANT_EZSTAT) );

	if ( nuvant_ezstat == (MX_NUVANT_EZSTAT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_NUVANT_EZSTAT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = nuvant_ezstat;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	nuvant_ezstat->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_nuvant_ezstat_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_nuvant_ezstat_open()";

	MX_NUVANT_EZSTAT *ezstat = NULL;
	mx_status_type mx_status;

	mx_status = mxi_nuvant_ezstat_get_pointers( record, &ezstat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NUVANT_EZSTAT_DEBUG
	MX_DEBUG(-2,("%s invoked for '%s'.", fname, record->name));
#endif

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_nuvant_ezstat_create_task( char *task_name, TaskHandle *task_handle )
{
	static const char fname[] = "mxi_nuvant_ezstat_create_task()";

	char daqmx_error_message[200];
	int32 daqmx_status;
	mx_status_type mx_status;

	daqmx_status = DAQmxCreateTask( task_name, task_handle );

	if ( daqmx_status != 0 ) {
		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to create a DAQmx task named '%s' failed.  "
		"DAQmx error code = %d, error message = '%s'",
			task_name, (int) daqmx_status, daqmx_error_message );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_nuvant_ezstat_start_task( TaskHandle task_handle )
{
	static const char fname[] = "mxi_nuvant_ezstat_start_task()";

	char daqmx_error_message[200];
	int32 daqmx_status;
	mx_status_type mx_status;

	/* Start the task. */

	daqmx_status = DAQmxStartTask( task_handle );

	if ( daqmx_status != 0 ) {
		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to start DAQmx task handle %#lx failed.  "
		"DAQmx error code = %d, error message = '%s'",
			(unsigned long) task_handle, 
			(int) daqmx_status,
			daqmx_error_message );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_nuvant_ezstat_shutdown_task( TaskHandle task_handle )
{
	static const char fname[] = "mxi_nuvant_ezstat_shutdown_task()";

	char daqmx_error_message[200];
	int32 daqmx_status;
	mx_status_type mx_status;

	/* Stop the task. */

	daqmx_status = DAQmxStopTask( task_handle );

	if ( daqmx_status != 0 ) {
		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to stop DAQmx task handle %#lx failed.  "
		"DAQmx error code = %d, error message = '%s'",
			(unsigned long) task_handle, 
			(int) daqmx_status,
			daqmx_error_message );
	}

	/* Release the resources used by the task. */

	daqmx_status = DAQmxClearTask( task_handle );

	if ( daqmx_status != 0 ) {
		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to clear DAQmx task handle %#lx failed.  "
		"DAQmx error code = %d, error message = '%s'",
			(unsigned long) task_handle, 
			(int) daqmx_status,
			daqmx_error_message );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#define MXI_NUVANT_EZSTAT_NUM_AI_CHANNELS	4
#define MXI_NUVANT_EZSTAT_NUM_AI_MEASUREMENTS	40

#define MXI_NUVANT_EZSTAT_NUM_AI_SAMPLES \
    (MXI_NUVANT_EZSTAT_NUM_AI_CHANNELS * MXI_NUVANT_EZSTAT_NUM_AI_MEASUREMENTS)

MX_EXPORT mx_status_type
mxi_nuvant_ezstat_read_ai_values( MX_NUVANT_EZSTAT *ezstat,
				double *ai_value_array )
{
	static const char fname[] = "mxi_nuvant_ezstat_read_ai_values()";

	char channel_names[80];
	TaskHandle task_handle;
	double ai_measurement_array[MXI_NUVANT_EZSTAT_NUM_AI_SAMPLES];
	double sum[MXI_NUVANT_EZSTAT_NUM_AI_CHANNELS];
	int32 num_samples_read;

	int i, j;
	double *ai_ptr;

	char daqmx_error_message[200];
	int32 daqmx_status;

	mx_status_type mx_status;

	snprintf( channel_names, sizeof(channel_names),
			"%s/ai0:3", ezstat->device_name );

	/* Create a DAQmx task for the read operation. */

	mx_status = mxi_nuvant_ezstat_create_task( "ezstat_read_ai_values",
							&task_handle );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Associate the analog input channels with the task. */

	daqmx_status = DAQmxCreateAIVoltageChan( task_handle,
					channel_names, NULL,
					DAQmx_Val_RSE,
					-10.0, 10.0,
					DAQmx_Val_Volts, NULL );

	if ( daqmx_status != 0 ) {
		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to associate analog input channels '%s' with "
		"DAQmx task %#lx failed.  "
		"DAQmx error code = %d, error message = '%s'",
			channel_names,
			(unsigned long) task_handle,
			(int) daqmx_status, daqmx_error_message );
	}

	/* Setup the sample clock for this measurement. */

	daqmx_status = DAQmxCfgSampClkTiming( task_handle, NULL,
						40000.0,
						DAQmx_Val_Rising,
						DAQmx_Val_FiniteSamps,
					MXI_NUVANT_EZSTAT_NUM_AI_MEASUREMENTS );

	if ( daqmx_status != 0 ) {
		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to configure the sample clock for "
		"DAQmx task %#lx failed.  "
		"DAQmx error code = %d, error message = '%s'",
			(unsigned long) task_handle,
			(int) daqmx_status, daqmx_error_message );
	}

	/* Start the task. */

	mx_status = mxi_nuvant_ezstat_start_task( task_handle );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Readout the acquired values. */

	daqmx_status = DAQmxReadAnalogF64( task_handle,
					MXI_NUVANT_EZSTAT_NUM_AI_MEASUREMENTS,
					10.0,
					DAQmx_Val_GroupByChannel,
					ai_measurement_array,
					MXI_NUVANT_EZSTAT_NUM_AI_SAMPLES,
					&num_samples_read,
					NULL );

	if ( daqmx_status != 0 ) {
		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to read the analog input samples for "
		"DAQmx task %#lx failed.  "
		"DAQmx error code = %d, error message = '%s'",
			(unsigned long) task_handle,
			(int) daqmx_status, daqmx_error_message );
	}

	/* Shutdown the task. */

	mx_status = mxi_nuvant_ezstat_shutdown_task( task_handle );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the averages of the measurements for each channel. */

	ai_ptr = ai_measurement_array;

	for ( i = 0; i < MXI_NUVANT_EZSTAT_NUM_AI_CHANNELS; i++ ) {
		sum[i] = 0.0;

		for ( j = 0; i < MXI_NUVANT_EZSTAT_NUM_AI_MEASUREMENTS; j++ ) {
			sum[i] += *ai_ptr;
		}

		ai_value_array[i]
			= sum[i] / MXI_NUVANT_EZSTAT_NUM_AI_MEASUREMENTS;
	}

	return MX_SUCCESSFUL_RESULT;
}

