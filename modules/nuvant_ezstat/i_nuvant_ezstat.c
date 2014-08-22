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

#define MXI_NUVANT_EZSTAT_DEBUG			FALSE

#define MXI_NUVANT_EZSTAT_DEBUG_READ_AI		TRUE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_digital_output.h"
#include "i_nuvant_ezstat.h"

MX_RECORD_FUNCTION_LIST mxi_nuvant_ezstat_record_function_list = {
	NULL,
	mxi_nuvant_ezstat_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_nuvant_ezstat_open,
	NULL,
	NULL,
	NULL,
	mxi_nuvant_ezstat_special_processing_setup
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
	char task_name[40];
	TaskHandle task_handle;
	char channel_names[100];
	int32 daqmx_status;
	char daqmx_error_message[200];
	uInt32 pin_value_array[5];
	uInt32 num_samples_read;
	mx_status_type mx_status;

	mx_status = mxi_nuvant_ezstat_get_pointers( record, &ezstat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NUVANT_EZSTAT_DEBUG
	MX_DEBUG(-2,("%s invoked for '%s'.", fname, record->name));
#endif

	/* We need to initialize the internal potentiostat/galvanostat current
	 * range variables and potentiostat/galvanostat resistance variables
	 * to match what the hardware is set to so that we report the correct
	 * values to the user.
	 */

	/* Create the task to use. */

	snprintf(task_name, sizeof(task_name), "%s_open", ezstat->device_name);

	mx_status = mxi_nuvant_ezstat_create_task( task_name, &task_handle );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Create the digital input channel. */

	/* Use the values of port1/line0:4 to get both the setting of the
	 * potentiostat/galvanostat selector as well as the binary range
	 * settings.
	 */

	snprintf( channel_names, sizeof(channel_names),
		"%s/port1/line0:4", ezstat->device_name );

	daqmx_status = DAQmxCreateDIChan( task_handle, channel_names,
					NULL, DAQmx_Val_ChanPerLine );

	if ( daqmx_status != 0 ) {
		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to associate digital I/O pins '%s' for "
		"EZStat '%s' with DAQmx task %#lx failed.  "
		"DAQmx error code = %d, error message = '%s'",
			channel_names,
			ezstat->record->name,
			(unsigned long) task_handle,
			(int) daqmx_status, daqmx_error_message );
	}

	/* Start the task. */

	mx_status = mxi_nuvant_ezstat_start_task( task_handle );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the current values of the pin settings. */

	daqmx_status = DAQmxReadDigitalU32( task_handle, 1, 1.0, 
					DAQmx_Val_GroupByChannel,
					pin_value_array, 5,
					&num_samples_read, NULL );

	if ( daqmx_status != 0 ) {
		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to read the digital inputs for "
		"DAQmx task %#lx failed.  "
		"DAQmx error code = %d, error message = '%s'",
			(unsigned long) task_handle,
			(int) daqmx_status, daqmx_error_message );
	}

	/* Shutdown the task. */

	mx_status = mxi_nuvant_ezstat_shutdown_task( task_handle );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NUVANT_EZSTAT_DEBUG
	{
		int n;

		for ( n = 0; n < 5; n++ ) {
			MX_DEBUG(-2,("%s: pin[%d] = %#x",
			fname, n, pin_value_array[n]));
		}
	}
#endif

	/* FIXME: For some reason, the pin values are all reading as 0. */

	return mx_status;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxi_nuvant_ezstat_process_function( void *record_ptr,
				void *record_field_ptr,
				int operation )
{
	static const char fname[] = "mxi_nuvant_ezstat_process_function()";

	MX_RECORD *record = NULL;
	MX_RECORD_FIELD *record_field = NULL;
	MX_NUVANT_EZSTAT *ezstat = NULL;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	record_field = (MX_RECORD_FIELD *) record_field_ptr;

	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD pointer passed was NULL." );
	}

	ezstat = (MX_NUVANT_EZSTAT *) record->record_type_struct;

	if ( ezstat == (MX_NUVANT_EZSTAT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NUVANT_EZSTAT pointer for record '%s' is NULL." );
	}

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_NUVANT_EZSTAT_SHOW_PARAMETERS:
			fprintf( stderr, "Record '%s'\n", record->name );
			fprintf( stderr, "  cell_on = %d\n",
						(int) ezstat->cell_on );
			fprintf( stderr, "  ezstat_mode = %d\n",
						(int) ezstat->ezstat_mode );
			fprintf( stderr, "  potentiostat_binary_range = %lu\n",
					ezstat->potentiostat_binary_range );
			fprintf( stderr, "  galvanostat_binary_range = %lu\n",
					ezstat->galvanostat_binary_range );
			fprintf( stderr, "  potentiostat_current_range = %g\n",
					ezstat->potentiostat_current_range );
			fprintf( stderr, "  galvanostat_current_range = %g\n",
					ezstat->galvanostat_current_range );
			fprintf( stderr, "  potentiostat_resistance = %g\n",
					ezstat->potentiostat_resistance );
			fprintf( stderr, "  galvanostat_resistance = %g\n",
					ezstat->galvanostat_resistance );
			fflush( stderr );
			break;
		default:
			break;
		}
		break;

	case MX_PROCESS_PUT:
		break;
	}

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_nuvant_ezstat_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_nuvant_ezstat_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_NUVANT_EZSTAT_SHOW_PARAMETERS:
			record_field->process_function
					= mxi_nuvant_ezstat_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
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

#define MXI_NUVANT_EZSTAT_NUM_AI_CHANNELS	6
#define MXI_NUVANT_EZSTAT_NUM_AI_MEASUREMENTS	40

#define MXI_NUVANT_EZSTAT_NUM_AI_SAMPLES \
    (MXI_NUVANT_EZSTAT_NUM_AI_CHANNELS * MXI_NUVANT_EZSTAT_NUM_AI_MEASUREMENTS)

MX_EXPORT mx_status_type
mxi_nuvant_ezstat_read_ai_values( MX_NUVANT_EZSTAT *ezstat,
				double *ai_value_array )
{
	static const char fname[] = "mxi_nuvant_ezstat_read_ai_values()";

	char channel_names[80];
	char task_name[40];
	TaskHandle task_handle;
	double ai_measurement_array[MXI_NUVANT_EZSTAT_NUM_AI_SAMPLES];
	double sum[MXI_NUVANT_EZSTAT_NUM_AI_CHANNELS];
	int32 num_samples_read;

	int i, j;
	double *ai_ptr;
	double ai_value;

	char daqmx_error_message[200];
	int32 daqmx_status;

	mx_status_type mx_status;

	snprintf( channel_names, sizeof(channel_names),
			"%s/ai0:3,%s/ai14:15",
			ezstat->device_name, ezstat->device_name );

	/* Create a DAQmx task for the read operation. */

	snprintf( task_name, sizeof(task_name),
		"%s_read_ai_values", ezstat->device_name );

	mx_status = mxi_nuvant_ezstat_create_task( task_name, &task_handle );

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

#if MXI_NUVANT_EZSTAT_DEBUG_READ_AI
	MX_DEBUG(-2,(""));
#endif

	for ( i = 0; i < MXI_NUVANT_EZSTAT_NUM_AI_CHANNELS; i++ ) {
		sum[i] = 0.0;

		for ( j = 0; j < MXI_NUVANT_EZSTAT_NUM_AI_MEASUREMENTS; j++ ) {
			ai_value = *ai_ptr;

#if 0
			MX_DEBUG(-2,("%s: ai_value (%lu)(%lu) = %g",
				fname, i, j, ai_value));
#endif

			sum[i] += ai_value;

			ai_ptr++;
		}

		ai_value_array[i]
			= sum[i] / MXI_NUVANT_EZSTAT_NUM_AI_MEASUREMENTS;

#if MXI_NUVANT_EZSTAT_DEBUG_READ_AI
		MX_DEBUG(-2,("%s: ai_value_array[%lu] = %g",
			fname, i, ai_value_array[i]));
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_nuvant_ezstat_set_binary_range( MX_NUVANT_EZSTAT *ezstat,
				unsigned long ezstat_mode,
				unsigned long binary_range )
{
	static const char fname[] = "mxi_nuvant_ezstat_set_binary_range()";

	char doutput_task_name[40];
	TaskHandle doutput_task_handle;
	char doutput_channel_names[200];
	int32 daqmx_status;
	char daqmx_error_message[200];
	uInt32 pin_value_array[4];
	uInt32 samples_written;
	mx_status_type mx_status;

	if ( ezstat == (MX_NUVANT_EZSTAT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NUVANT_EZSTAT pointer passed was NULL." );
	}
	if ( binary_range > 0x3 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal EZStat binary range %#lx requested for record '%s'.  "
		"The allowed binary ranges are from 0x0 to 0x3.",
			binary_range, ezstat->record->name );
	}

	ezstat->ezstat_mode = ezstat_mode;

	switch( ezstat_mode ) {
	case MXF_NUVANT_EZSTAT_POTENTIOSTAT_MODE:

		snprintf( doutput_channel_names, sizeof(doutput_channel_names),
		"%s/port1/line0,%s/port1/line3:4,%s/port1/line5",
		ezstat->device_name, ezstat->device_name, ezstat->device_name );

		ezstat->potentiostat_binary_range = binary_range;

		switch( binary_range ) {
		case 0x3:
			ezstat->potentiostat_current_range = 1.0;
			ezstat->potentiostat_resistance = 9.0998;
			break;
		case 0x2:
			ezstat->potentiostat_current_range = 0.01;
			ezstat->potentiostat_resistance = 499.19;
			break;
		case 0x1:
			ezstat->potentiostat_current_range = 100.0e-6;
			ezstat->potentiostat_resistance = 45408.7;
			break;
		case 0x0:
			ezstat->potentiostat_current_range = 0.01e-6;
			ezstat->potentiostat_resistance = 499000.0;
			break;
		}
		break;
	case MXF_NUVANT_EZSTAT_GALVANOSTAT_MODE:

		snprintf( doutput_channel_names, sizeof(doutput_channel_names),
		"%s/port1/line0,%s/port1/line1:2,%s/port1/line5",
		ezstat->device_name, ezstat->device_name, ezstat->device_name );

		ezstat->galvanostat_binary_range = binary_range;

		switch( binary_range ) {
		case 0x3:
			ezstat->galvanostat_current_range = 1.0;
			ezstat->galvanostat_resistance = 9.07;
			break;
		case 0x2:
			ezstat->galvanostat_current_range = 0.1;
			ezstat->galvanostat_resistance = 100.0;
			break;
		case 0x1:
			ezstat->galvanostat_current_range = 0.001;
			ezstat->galvanostat_resistance = 10000.0;
			break;
		case 0x0:
			ezstat->galvanostat_current_range = 100.0e-6;
			ezstat->galvanostat_resistance = 100000.0;
			break;
		}
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal EZStat mode %lu requested for record '%s'.  "
		"The allowed mode values are "
		"'potentiostat mode' (0) and 'galvanostat mode' (1).",
			ezstat_mode,
			ezstat->record->name );
		break;
	}

	/* Prepare to reprogram the digital output bits. */

	snprintf( doutput_task_name, sizeof(doutput_task_name),
		"%s_set_range", ezstat->device_name );

	mx_status = mxi_nuvant_ezstat_create_task( doutput_task_name,
						&doutput_task_handle );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Create the digital output channel. */

	daqmx_status = DAQmxCreateDOChan( doutput_task_handle,
					doutput_channel_names, NULL,
					DAQmx_Val_ChanPerLine );

	if ( daqmx_status != 0 ) {
		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to associate digital I/O pins '%s' for "
		"EZStat '%s' with DAQmx task %#lx failed.  "
		"DAQmx error code = %d, error message = '%s'",
			doutput_channel_names,
			ezstat->record->name,
			(unsigned long) doutput_task_handle,
			(int) daqmx_status, daqmx_error_message );
	}

	/* Start the task. */

	mx_status = mxi_nuvant_ezstat_start_task( doutput_task_handle );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Construct the value to send to the digital I/O pins. */

	if ( ezstat_mode == MXF_NUVANT_EZSTAT_POTENTIOSTAT_MODE ) {
		pin_value_array[0] = 0;
	} else {
		pin_value_array[0] = 1;
	}

	pin_value_array[1] = ( binary_range & 0x1 );

	pin_value_array[2] = ( binary_range & 0x2 ) >> 1;

	pin_value_array[3] = 1;	/* Enable range change with /port1/line5. */

	daqmx_status = DAQmxWriteDigitalU32( doutput_task_handle,
					1, FALSE, 1.0,
					DAQmx_Val_GroupByChannel,
					pin_value_array,
					&samples_written, NULL );

	if ( daqmx_status != 0 ) {
		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to write the digital output samples for "
		"DAQmx task %#lx used by record '%s' failed.  "
		"DAQmx error code = %d, error message = '%s'",
			(unsigned long) doutput_task_handle,
			ezstat->record->name,
			(int) daqmx_status, daqmx_error_message );
	}

	/* We are done with the digital I/O task now, so shut it down. */

	mx_status = mxi_nuvant_ezstat_shutdown_task( doutput_task_handle );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_nuvant_ezstat_set_current_range( MX_NUVANT_EZSTAT *ezstat,
				unsigned long ezstat_mode,
				double current_range )
{
	static const char fname[] = "mxi_nuvant_ezstat_set_current_range()";

	unsigned long binary_range;
	mx_status_type mx_status;

	if ( ezstat == (MX_NUVANT_EZSTAT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NUVANT_EZSTAT pointer passed was NULL." );
	}

	switch( ezstat_mode ) {
	case MXF_NUVANT_EZSTAT_POTENTIOSTAT_MODE:
		if ( current_range > 1.0 ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested potentiostat current range (%g amps) "
			"is greater than the maximum allowed current range "
			"of 1.0 amps for record '%s'.",
				current_range,
				ezstat->record->name );
		} else
		if ( current_range > 0.01 ) {	/* 10 mA to 1 A range */
			binary_range = 0x3;
		} else
		if ( current_range > 0.0001 ) {	/* 100 uA to 10 mA range */
			binary_range = 0x2;
		} else
		if ( current_range > 1.0e-6 ) {	/* 1 uA to 100 uA range */
			binary_range = 0x1;
		} else {			/* 1 nA to 1 uA range */
			binary_range = 0x0;
		}
		break;
	case MXF_NUVANT_EZSTAT_GALVANOSTAT_MODE:
		if ( current_range > 1.0 ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested galvanostat current range (%g amps) "
			"is greater than the maximum allowed current range "
			"of 1.0 amps for record '%s'.",
				current_range,
				ezstat->record->name );
		} else
		if ( current_range > 0.1 ) {	/* 100 mA to 1 A range */
			binary_range = 0x3;
		} else
		if ( current_range > 0.001 ) {	/* 1 mA to 100 mA range */
			binary_range = 0x2;
		} else
		if ( current_range > 100.0e-6 ) { /* 100 uA to 1 mA range */
			binary_range = 0x1;
		} else {			/* 1 uA to 100 uA range */
			binary_range = 0x0;
		}
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal EZStat mode %lu requested for record '%s'.  "
		"The allowed mode values are "
		"'potentiostat mode' (0) and 'galvanostat mode' (1).",
			ezstat_mode,
			ezstat->record->name );
		break;
	}

	mx_status = mxi_nuvant_ezstat_set_binary_range( ezstat,
							ezstat_mode,
							binary_range );

	return mx_status;
}

