/*
 * Name:    d_ni_daqmx_thermocouple.c
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

#define MXD_NI_DAQMX_THERMOCOUPLE_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "i_ni_daqmx.h"
#include "d_ni_daqmx_thermocouple.h"

/* Initialize the ainput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_ni_daqmx_thermocouple_record_function_list = {
	NULL,
	mxd_ni_daqmx_thermocouple_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_ni_daqmx_thermocouple_open
};

MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_ni_daqmx_thermocouple_analog_input_function_list =
{
	mxd_ni_daqmx_thermocouple_read
};

/* DAQmx analog input data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_ni_daqmx_thermocouple_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_NI_DAQMX_THERMOCOUPLE_STANDARD_FIELDS
};

long mxd_ni_daqmx_thermocouple_num_record_fields
		= sizeof( mxd_ni_daqmx_thermocouple_rf_defaults )
		  / sizeof( mxd_ni_daqmx_thermocouple_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ni_daqmx_thermocouple_rfield_def_ptr
			= &mxd_ni_daqmx_thermocouple_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_ni_daqmx_thermocouple_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_NI_DAQMX_THERMOCOUPLE **ni_daqmx_thermocouple,
			MX_NI_DAQMX **ni_daqmx,
			const char *calling_fname )
{
	static const char fname[] = "mxd_ni_daqmx_thermocouple_get_pointers()";

	MX_NI_DAQMX_THERMOCOUPLE *ni_daqmx_thermocouple_ptr;
	MX_RECORD *ni_daqmx_record;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_INPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	ni_daqmx_thermocouple_ptr = (MX_NI_DAQMX_THERMOCOUPLE *)
				ainput->record->record_type_struct;

	if ( ni_daqmx_thermocouple_ptr == (MX_NI_DAQMX_THERMOCOUPLE *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NI_DAQMX_THERMOCOUPLE pointer for "
			"DAQmx thermocouple record '%s' passed by '%s' is NULL",
				ainput->record->name, calling_fname );
	}

	if ( ni_daqmx_thermocouple != (MX_NI_DAQMX_THERMOCOUPLE **) NULL ) {
		*ni_daqmx_thermocouple = ni_daqmx_thermocouple_ptr;
	}

	if ( ni_daqmx != (MX_NI_DAQMX **) NULL ) {
		ni_daqmx_record = ni_daqmx_thermocouple_ptr->ni_daqmx_record;

		if ( ni_daqmx_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The ni_daqmx_record pointer for "
			"DAQmx thermocouple '%s' is NULL.",
				ainput->record->name );
		}

		*ni_daqmx = (MX_NI_DAQMX *) ni_daqmx_record->record_type_struct;

		if ( (*ni_daqmx) == (MX_NI_DAQMX *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NI_DAQMX pointer for DAQmx record '%s' "
			"used by thermocouple '%s' is NULL.",
				ni_daqmx_record->name,
				ainput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_ni_daqmx_thermocouple_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_ni_daqmx_thermocouple_create_record_structures()";

	MX_ANALOG_INPUT *ainput;
	MX_NI_DAQMX_THERMOCOUPLE *ni_daqmx_thermocouple;

	/* Allocate memory for the necessary structures. */

	ainput = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_ANALOG_INPUT structure." );
	}

	ni_daqmx_thermocouple = (MX_NI_DAQMX_THERMOCOUPLE *)
				malloc( sizeof(MX_NI_DAQMX_THERMOCOUPLE) );

	if ( ni_daqmx_thermocouple == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for MX_NI_DAQMX_THERMOCOUPLE structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = ainput;
	record->record_type_struct = ni_daqmx_thermocouple;
	record->class_specific_function_list
		= &mxd_ni_daqmx_thermocouple_analog_input_function_list;

	ainput->record = record;

	ainput->subclass = MXT_AOU_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ni_daqmx_thermocouple_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_ni_daqmx_thermocouple_open()";

	MX_ANALOG_INPUT *ainput;
	MX_NI_DAQMX_THERMOCOUPLE *ni_daqmx_thermocouple = NULL;
	MX_NI_DAQMX *ni_daqmx = NULL;
	MX_NI_DAQMX_TASK *task = NULL;
	char daqmx_error_message[400];
	int32 daqmx_status;
	char *units_name;
	char *type_name;
	char *cold_junction_type_name;
	size_t len;
	int32 units;
	int32 thermocouple_type;
	int32 cold_junction_type;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	ainput = (MX_ANALOG_INPUT *) record->record_class_struct;

	mx_status = mxd_ni_daqmx_thermocouple_get_pointers(
			ainput, &ni_daqmx_thermocouple, &ni_daqmx, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* What are the raw thermocouple units? */

	units_name = ni_daqmx_thermocouple->thermocouple_units;

	len = strlen( units_name );

	if ( mx_strncasecmp( units_name, "C", len ) == 0 ) {
		units = DAQmx_Val_DegC;
	} else
	if ( mx_strncasecmp( units_name, "F", len ) == 0 ) {
		units = DAQmx_Val_DegF;
	} else
	if ( mx_strncasecmp( units_name, "K", len ) == 0 ) {
		units = DAQmx_Val_Kelvins;
	} else
	if ( mx_strncasecmp( units_name, "R", len ) == 0 ) {
		units = DAQmx_Val_DegR;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Thermocouple units '%s' are not supported for "
		"analog input '%s'.", units_name, record->name );
	}

	/* What is the thermocouple type? */

	type_name = ni_daqmx_thermocouple->thermocouple_type;

	len = strlen( type_name );

	if ( mx_strncasecmp( type_name, "B", len ) == 0 ) {
		thermocouple_type = DAQmx_Val_B_Type_TC;
	} else
	if ( mx_strncasecmp( type_name, "E", len ) == 0 ) {
		thermocouple_type = DAQmx_Val_E_Type_TC;
	} else
	if ( mx_strncasecmp( type_name, "J", len ) == 0 ) {
		thermocouple_type = DAQmx_Val_J_Type_TC;
	} else
	if ( mx_strncasecmp( type_name, "K", len ) == 0 ) {
		thermocouple_type = DAQmx_Val_K_Type_TC;
	} else
	if ( mx_strncasecmp( type_name, "N", len ) == 0 ) {
		thermocouple_type = DAQmx_Val_N_Type_TC;
	} else
	if ( mx_strncasecmp( type_name, "R", len ) == 0 ) {
		thermocouple_type = DAQmx_Val_R_Type_TC;
	} else
	if ( mx_strncasecmp( type_name, "S", len ) == 0 ) {
		thermocouple_type = DAQmx_Val_S_Type_TC;
	} else
	if ( mx_strncasecmp( type_name, "T", len ) == 0 ) {
		thermocouple_type = DAQmx_Val_T_Type_TC;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Thermocouple type '%s' is not supported for "
		"analog input '%s'.", type_name, record->name );
	}

	/* What is the cold junction type? */

	cold_junction_type_name = ni_daqmx_thermocouple->cold_junction;

	len = strlen( cold_junction_type_name );

	if ( mx_strncasecmp( cold_junction_type_name, "builtin", len ) == 0 ) {
		cold_junction_type = DAQmx_Val_BuiltIn;
	} else
	if ( mx_strncasecmp( cold_junction_type_name, "constant", len ) == 0 ) {
		cold_junction_type = DAQmx_Val_ConstVal;
	} else
	if ( mx_strncasecmp( cold_junction_type_name, "channel", len ) == 0 ) {
		cold_junction_type = DAQmx_Val_Chan;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Cold junction type '%s' is not supported for "
		"analog input '%s'.", cold_junction_type_name, record->name );
	}

	/* Create a DAQmx task. */

	mx_status = mxi_ni_daqmx_find_or_create_task( ni_daqmx,
					ni_daqmx_thermocouple->task_name,
					&task );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_ni_daqmx_set_task_datatype( task, MXFT_DOUBLE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Associate a thermocouple input channel with this task. */

	daqmx_status = DAQmxCreateAIThrmcplChan(
				task->task_handle,
				ni_daqmx_thermocouple->channel_name, NULL,
				ni_daqmx_thermocouple->minimum_value,
				ni_daqmx_thermocouple->maximum_value,
				units, thermocouple_type, cold_junction_type,
			ni_daqmx_thermocouple->cold_junction_temperature,
			ni_daqmx_thermocouple->cold_junction_channel_name );

#if MXD_NI_DAQMX_THERMOCOUPLE_DEBUG
	MX_DEBUG(-2,
	("%s: DAQmxCreateAIThrmcplChan( %#lx, '%s', NULL, %#lx ) = %d",
		fname, (unsigned long) task->task_handle,
		ni_daqmx_thermocouple->channel_name,
		(unsigned long) DAQmx_Val_ChanForAllLines,
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

	ni_daqmx_thermocouple->task = task;

	ni_daqmx_thermocouple->channel_offset = task->num_channels;

	task->num_channels++;

	/* The task will be started in the "finish delayed initialization"
	 * driver function of the "ni_daqmx" record.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ni_daqmx_thermocouple_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_ni_daqmx_thermocouple_read()";

	MX_NI_DAQMX_THERMOCOUPLE *ni_daqmx_thermocouple = NULL;
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

	mx_status = mxd_ni_daqmx_thermocouple_get_pointers(
			ainput, &ni_daqmx_thermocouple, &ni_daqmx, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	task = ni_daqmx_thermocouple->task;

	if ( task == NULL ) {
		return mx_error( MXE_HARDWARE_CONFIGURATION_ERROR, fname,
		"Could not find the hardware for National Instruments device '%s'.",
			ainput->record->name );
	}

	channel        = ni_daqmx_thermocouple->channel_offset;
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

#if MXD_NI_DAQMX_THERMOCOUPLE_DEBUG
	MX_DEBUG(-2,("%s: DAQmxReadAnalogF64( "
	"%#lx, %lu, %f, %#x, read_array, %lu, &num_samples, NULL ) = %d",
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

#if MXD_NI_DAQMX_THERMOCOUPLE_DEBUG
	MX_DEBUG(-2,("%s:   num_samples_read = %lu, channel_buffer[%lu] = %lu",
		fname, num_samples_read, channel, channel_buffer[channel] ));
#endif

	ainput->raw_value.double_value = channel_buffer[channel];

	return mx_status;
}

