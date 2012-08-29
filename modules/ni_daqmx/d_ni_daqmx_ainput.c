/*
 * Name:    d_ni_daqmx_ainput.c
 *
 * Purpose: MX driver for NI-DAQmx analog input channels.
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

#define MXD_NI_DAQMX_AINPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "i_ni_daqmx.h"
#include "d_ni_daqmx_ainput.h"

/* Initialize the ainput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_ni_daqmx_ainput_record_function_list = {
	NULL,
	mxd_ni_daqmx_ainput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_ni_daqmx_ainput_open
};

MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_ni_daqmx_ainput_analog_input_function_list =
{
	mxd_ni_daqmx_ainput_read
};

/* DAQmx analog input data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_ni_daqmx_ainput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_NI_DAQMX_AINPUT_STANDARD_FIELDS
};

long mxd_ni_daqmx_ainput_num_record_fields
		= sizeof( mxd_ni_daqmx_ainput_rf_defaults )
		  / sizeof( mxd_ni_daqmx_ainput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ni_daqmx_ainput_rfield_def_ptr
			= &mxd_ni_daqmx_ainput_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_ni_daqmx_ainput_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_NI_DAQMX_AINPUT **ni_daqmx_ainput,
			MX_NI_DAQMX **ni_daqmx,
			const char *calling_fname )
{
	static const char fname[] = "mxd_ni_daqmx_ainput_get_pointers()";

	MX_NI_DAQMX_AINPUT *ni_daqmx_ainput_ptr;
	MX_RECORD *ni_daqmx_record;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_INPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	ni_daqmx_ainput_ptr = (MX_NI_DAQMX_AINPUT *)
				ainput->record->record_type_struct;

	if ( ni_daqmx_ainput_ptr == (MX_NI_DAQMX_AINPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NI_DAQMX_AINPUT pointer for "
			"DAQMX analog input '%s' passed by '%s' is NULL",
				ainput->record->name, calling_fname );
	}

	if ( ni_daqmx_ainput != (MX_NI_DAQMX_AINPUT **) NULL ) {
		*ni_daqmx_ainput = ni_daqmx_ainput_ptr;
	}

	if ( ni_daqmx != (MX_NI_DAQMX **) NULL ) {
		ni_daqmx_record = ni_daqmx_ainput_ptr->ni_daqmx_record;

		if ( ni_daqmx_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The ni_daqmx_record pointer for "
			"DAQmx analog input '%s' is NULL.",
				ainput->record->name );
		}

		*ni_daqmx = (MX_NI_DAQMX *) ni_daqmx_record->record_type_struct;

		if ( (*ni_daqmx) == (MX_NI_DAQMX *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NI_DAQMX pointer for DAQmx record '%s' "
			"used by analog input '%s' is NULL.",
				ni_daqmx_record->name,
				ainput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_ni_daqmx_ainput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_ni_daqmx_ainput_create_record_structures()";

	MX_ANALOG_INPUT *ainput;
	MX_NI_DAQMX_AINPUT *ni_daqmx_ainput;

	/* Allocate memory for the necessary structures. */

	ainput = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_ANALOG_INPUT structure." );
	}

	ni_daqmx_ainput = (MX_NI_DAQMX_AINPUT *)
				malloc( sizeof(MX_NI_DAQMX_AINPUT) );

	if ( ni_daqmx_ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_NI_DAQMX_AINPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = ainput;
	record->record_type_struct = ni_daqmx_ainput;
	record->class_specific_function_list
			= &mxd_ni_daqmx_ainput_analog_input_function_list;

	ainput->record = record;

	ainput->subclass = MXT_AOU_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ni_daqmx_ainput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_ni_daqmx_ainput_open()";

	MX_ANALOG_INPUT *ainput;
	MX_NI_DAQMX_AINPUT *ni_daqmx_ainput = NULL;
	MX_NI_DAQMX *ni_daqmx = NULL;
	MX_NI_DAQMX_TASK *task = NULL;
	char daqmx_error_message[400];
	int32 daqmx_status;
	char *config_name;
	size_t len;
	int32 terminal_config;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	ainput = (MX_ANALOG_INPUT *) record->record_class_struct;

	mx_status = mxd_ni_daqmx_ainput_get_pointers(
				ainput, &ni_daqmx_ainput, &ni_daqmx, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* What is the terminal configuration for this channel? */

	config_name = ni_daqmx_ainput->terminal_config;

	len = strlen( config_name );

	if ( mx_strncasecmp( config_name, "RSE", len ) == 0 ) {
		terminal_config = DAQmx_Val_RSE;
	} else
	if ( mx_strncasecmp( config_name, "NRSE", len ) == 0 ) {
		terminal_config = DAQmx_Val_NRSE;
	} else
	if ( mx_strncasecmp( config_name, "differential", len ) == 0 ) {
		terminal_config = DAQmx_Val_Diff;
	} else
	if ( mx_strncasecmp( config_name, "single-ended", len ) == 0 ) {
		terminal_config = DAQmx_Val_RSE;
	} else
	if ( mx_strncasecmp( config_name,
				"nonreferenced single-ended", len ) == 0 ) {
		terminal_config = DAQmx_Val_NRSE;
	} else
	if ( mx_strncasecmp( config_name,
				"non-referenced single-ended", len ) == 0 ) {
		terminal_config = DAQmx_Val_NRSE;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Terminal configuration '%s' is not supported for "
		"analog input '%s'.", config_name, record->name );
	}

	/* Find or create a DAQmx task. */

	mx_status = mxi_ni_daqmx_find_or_create_task( ni_daqmx,
						ni_daqmx_ainput->task_name,
						&task );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_ni_daqmx_set_task_datatype( task, MXFT_DOUBLE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Associate a analog input channel with this task. */

	daqmx_status = DAQmxCreateAIVoltageChan( task->task_handle,
					ni_daqmx_ainput->channel_name, NULL,
					terminal_config,
					ni_daqmx_ainput->minimum_value,
					ni_daqmx_ainput->maximum_value,
					DAQmx_Val_Volts, NULL );

#if MXD_NI_DAQMX_AINPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxCreateAIVoltageChan( %#lx, "
	"'%s', NULL, '%s', %g, %g, DAQmx_ValVolts, NULL ) = %d",
		fname, (unsigned long) task->task_handle,
		ni_daqmx_ainput->channel_name,
		config_name,
		ni_daqmx_ainput->minimum_value,
		ni_daqmx_ainput->maximum_value,
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {

		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to associate analog input '%s' with "
		"DAQmx task %#lx failed.  "
		"DAQmx error code = %d, error message = '%s'",
			record->name,
			(unsigned long) task->task_handle,
			(int) daqmx_status, daqmx_error_message );
	}

	/* Save the channel offset in the task. */

	ni_daqmx_ainput->task = task;

	ni_daqmx_ainput->channel_offset = task->num_channels;

	task->num_channels++;

	/* The task will be started in the "finish delayed initialization"
	 * driver function of the "ni_daqmx" record.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ni_daqmx_ainput_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_ni_daqmx_ainput_read()";

	MX_NI_DAQMX_AINPUT *ni_daqmx_ainput = NULL;
	MX_NI_DAQMX *ni_daqmx = NULL;
	MX_NI_DAQMX_TASK *task = NULL;
	unsigned long channel;
	float64 *channel_buffer;
	char daqmx_error_message[400];
	int32 daqmx_status;
	int32 num_samples;
	double timeout;
	int32 num_samples_read;
	mx_status_type mx_status;

	mx_status = mxd_ni_daqmx_ainput_get_pointers(
				ainput, &ni_daqmx_ainput, &ni_daqmx, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	task           = ni_daqmx_ainput->task;
	channel        = ni_daqmx_ainput->channel_offset;
	channel_buffer = task->channel_buffer;

	if ( channel_buffer == NULL ) {
		mx_warning( "Task not yet started for NI-DAQmx device '%s'.  "
			"The attempt to read from '%s' was ignored.",
			ainput->record->name,
			ainput->record->name );

		return MX_SUCCESSFUL_RESULT;
	}

	num_samples    = 1;
	timeout        = 10.0;    /* read timeout in seconds */

	daqmx_status = DAQmxReadAnalogF64( task->task_handle,
					num_samples, timeout,
					DAQmx_Val_GroupByChannel,
					channel_buffer,
					task->num_channels,
					&num_samples_read, NULL );

#if MXD_NI_DAQMX_AINPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxReadAnalogF64( "
	"%#lx, %lu, %f, %#x, channel_buffer, %lu, &num_samples, NULL ) = %d",
		fname, (unsigned long) task->task_handle,
		num_samples,
		timeout,
		DAQmx_Val_GroupByChannel,
		task->num_channels,
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {

		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to read analog input '%s' failed.  "
		"DAQmx error code = %d, error message = '%s'",
			ainput->record->name,
			(int) daqmx_status, daqmx_error_message );
	}

#if MXD_NI_DAQMX_AINPUT_DEBUG
	MX_DEBUG(-2,("%s:   num_samples_read = %lu, channel_buffer[%lu] = %lu",
		fname, num_samples_read,
		channel, channel_buffer[channel] ));
#endif

	ainput->raw_value.double_value = channel_buffer[channel];

	return mx_status;
}

