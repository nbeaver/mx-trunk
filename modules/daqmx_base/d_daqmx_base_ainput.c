/*
 * Name:    d_daqmx_base_ainput.c
 *
 * Purpose: MX driver for NI-DAQmx Base analog input channels.
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

#define MXD_DAQMX_BASE_AINPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "i_daqmx_base.h"
#include "d_daqmx_base_ainput.h"

/* Initialize the ainput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_daqmx_base_ainput_record_function_list = {
	NULL,
	mxd_daqmx_base_ainput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_daqmx_base_ainput_open,
	mxd_daqmx_base_ainput_close
};

MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_daqmx_base_ainput_analog_input_function_list =
{
	mxd_daqmx_base_ainput_read
};

/* DAQmx Base analog input data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_daqmx_base_ainput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_DAQMX_BASE_AINPUT_STANDARD_FIELDS
};

long mxd_daqmx_base_ainput_num_record_fields
		= sizeof( mxd_daqmx_base_ainput_rf_defaults )
		  / sizeof( mxd_daqmx_base_ainput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_daqmx_base_ainput_rfield_def_ptr
			= &mxd_daqmx_base_ainput_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_daqmx_base_ainput_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_DAQMX_BASE_AINPUT **daqmx_base_ainput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_daqmx_base_ainput_get_pointers()";

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_INPUT pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( daqmx_base_ainput == (MX_DAQMX_BASE_AINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DAQMX_BASE_AINPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	*daqmx_base_ainput = (MX_DAQMX_BASE_AINPUT *)
				ainput->record->record_type_struct;

	if ( *daqmx_base_ainput == (MX_DAQMX_BASE_AINPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_DAQMX_BASE_AINPUT pointer for "
			"ainput record '%s' passed by '%s' is NULL",
				ainput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_daqmx_base_ainput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_daqmx_base_ainput_create_record_structures()";

	MX_ANALOG_INPUT *ainput;
	MX_DAQMX_BASE_AINPUT *daqmx_base_ainput;

	/* Allocate memory for the necessary structures. */

	ainput = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_ANALOG_INPUT structure." );
	}

	daqmx_base_ainput = (MX_DAQMX_BASE_AINPUT *)
				malloc( sizeof(MX_DAQMX_BASE_AINPUT) );

	if ( daqmx_base_ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_DAQMX_BASE_AINPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = ainput;
	record->record_type_struct = daqmx_base_ainput;
	record->class_specific_function_list
			= &mxd_daqmx_base_ainput_analog_input_function_list;

	ainput->record = record;

	ainput->subclass = MXT_AOU_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_daqmx_base_ainput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_daqmx_base_ainput_open()";

	MX_ANALOG_INPUT *ainput;
	MX_DAQMX_BASE_AINPUT *daqmx_base_ainput = NULL;
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

	mx_status = mxd_daqmx_base_ainput_get_pointers(
				ainput, &daqmx_base_ainput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* What is the terminal configuration for this channel? */

	config_name = daqmx_base_ainput->terminal_config;

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

	/* Create a DAQmx Base task. */

	mx_status = mxi_daqmx_base_create_task( record,
					&(daqmx_base_ainput->handle) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Associate a analog input channel with this task. */

	daqmx_status = DAQmxBaseCreateAIVoltageChan( daqmx_base_ainput->handle,
					daqmx_base_ainput->channel_name, NULL,
					terminal_config,
					daqmx_base_ainput->minimum_value,
					daqmx_base_ainput->maximum_value,
					DAQmx_Val_Volts, NULL );

#if MXD_DAQMX_BASE_AINPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxBaseCreateDOChan( %#lx, '%s', NULL, %#lx ) = %d",
		fname, (unsigned long) daqmx_base_ainput->handle,
		daqmx_base_ainput->channel_name,
		(unsigned long) DAQmx_Val_ChanForAllLines,
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {

		DAQmxBaseGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to associate analog input '%s' with "
		"DAQmx Base task %#lx failed.  "
		"DAQmx error code = %d, error message = '%s'",
			record->name,
			(unsigned long) daqmx_base_ainput->handle,
			(int) daqmx_status, daqmx_error_message );
	}

	/* Start the task. */

	daqmx_status = DAQmxBaseStartTask( daqmx_base_ainput->handle );

#if MXD_DAQMX_BASE_AINPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxBaseStartTask( %#lx ) = %d",
		fname, (unsigned long) daqmx_base_ainput->handle,
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {

		DAQmxBaseGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to start task %#lx for analog input '%s' failed.  "
		"DAQmx error code = %d, error message = '%s'",
			(unsigned long) daqmx_base_ainput->handle, record->name,
			(int) daqmx_status, daqmx_error_message );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_daqmx_base_ainput_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_daqmx_base_ainput_close()";

	MX_ANALOG_INPUT *ainput;
	MX_DAQMX_BASE_AINPUT *daqmx_base_ainput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	ainput = (MX_ANALOG_INPUT *) record->record_class_struct;

	mx_status = mxd_daqmx_base_ainput_get_pointers(
				ainput, &daqmx_base_ainput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( daqmx_base_ainput->handle != 0 ) {
		mx_status = mxi_daqmx_base_shutdown_task( record,
						daqmx_base_ainput->handle );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_daqmx_base_ainput_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_daqmx_base_ainput_read()";

	MX_DAQMX_BASE_AINPUT *daqmx_base_ainput;
	char daqmx_error_message[400];
	int32 daqmx_status;
	int32 num_samples;
	double timeout;
	float64 read_array[1];
	int32 read_array_length;
	int32 num_samples_read;
	mx_status_type mx_status;

	daqmx_base_ainput = NULL;

	mx_status = mxd_daqmx_base_ainput_get_pointers(
				ainput, &daqmx_base_ainput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_samples       = 1;
	timeout           = 10.0;    /* read timeout in seconds */
	read_array_length = 1;

	daqmx_status = DAQmxBaseReadAnalogF64( daqmx_base_ainput->handle,
					num_samples, timeout,
					DAQmx_Val_GroupByChannel,
					read_array, read_array_length,
					&num_samples_read, NULL );

#if MXD_DAQMX_BASE_AINPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxBaseReadDigitalU32( "
	"%#lx, %lu, %f, %#x, read_array, %lu, &num_samples, NULL ) = %d",
		fname, (unsigned long) daqmx_base_ainput->handle,
		num_samples,
		timeout,
		DAQmx_Val_GroupByChannel,
		(unsigned long) read_array_length,
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {

		DAQmxBaseGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to read analog input '%s' failed.  "
		"DAQmx error code = %d, error message = '%s'",
			ainput->record->name,
			(int) daqmx_status, daqmx_error_message );
	}

#if MXD_DAQMX_BASE_AINPUT_DEBUG
	MX_DEBUG(-2,("%s:   num_samples_read = %lu, read_array[0] = %lu",
		fname, num_samples_read, read_array[0]));
#endif

	ainput->raw_value.double_value = read_array[0];

	return mx_status;
}

