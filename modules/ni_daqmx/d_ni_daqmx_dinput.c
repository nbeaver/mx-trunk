/*
 * Name:    d_daqmx_base_dinput.c
 *
 * Purpose: MX driver for NI-DAQmx Base digital input channels.
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

#define MXD_DAQMX_BASE_DINPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_input.h"
#include "i_daqmx_base.h"
#include "d_daqmx_base_dinput.h"

MX_RECORD_FUNCTION_LIST mxd_daqmx_base_dinput_record_function_list = {
	NULL,
	mxd_daqmx_base_dinput_create_record_structures,
	mxd_daqmx_base_dinput_finish_record_initialization,
	NULL,
	NULL,
	mxd_daqmx_base_dinput_open,
	mxd_daqmx_base_dinput_close,
};

MX_DIGITAL_INPUT_FUNCTION_LIST
		mxd_daqmx_base_dinput_digital_input_function_list = {
	mxd_daqmx_base_dinput_read
};

MX_RECORD_FIELD_DEFAULTS mxd_daqmx_base_dinput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_DAQMX_BASE_DINPUT_STANDARD_FIELDS
};

long mxd_daqmx_base_dinput_num_record_fields
	= sizeof( mxd_daqmx_base_dinput_record_field_defaults )
		/ sizeof( mxd_daqmx_base_dinput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_daqmx_base_dinput_rfield_def_ptr
			= &mxd_daqmx_base_dinput_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_daqmx_base_dinput_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_DAQMX_BASE_DINPUT **daqmx_base_dinput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_daqmx_base_dinput_get_pointers()";

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_INPUT pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( daqmx_base_dinput == (MX_DAQMX_BASE_DINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DAQMX_BASE_DINPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	*daqmx_base_dinput = (MX_DAQMX_BASE_DINPUT *)
				dinput->record->record_type_struct;

	if ( *daqmx_base_dinput == (MX_DAQMX_BASE_DINPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_DAQMX_BASE_DINPUT pointer for "
			"dinput record '%s' passed by '%s' is NULL",
				dinput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_daqmx_base_dinput_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_daqmx_base_dinput_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_DAQMX_BASE_DINPUT *daqmx_base_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_DIGITAL_INPUT structure." );
        }

        daqmx_base_dinput = (MX_DAQMX_BASE_DINPUT *)
				malloc( sizeof(MX_DAQMX_BASE_DINPUT) );

        if ( daqmx_base_dinput == (MX_DAQMX_BASE_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_DAQMX_BASE_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = daqmx_base_dinput;
        record->class_specific_function_list
			= &mxd_daqmx_base_dinput_digital_input_function_list;

        digital_input->record = record;
	daqmx_base_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_daqmx_base_dinput_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_daqmx_base_dinput_finish_record_initialization()";

	MX_DIGITAL_INPUT *dinput;
	MX_DAQMX_BASE_DINPUT *daqmx_base_dinput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	dinput = (MX_DIGITAL_INPUT *) record->record_class_struct;

	mx_status = mxd_daqmx_base_dinput_get_pointers(
				dinput, &daqmx_base_dinput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_daqmx_base_dinput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_daqmx_base_dinput_open()";

	MX_DIGITAL_INPUT *dinput;
	MX_DAQMX_BASE_DINPUT *daqmx_base_dinput = NULL;
	char daqmx_error_message[400];
	int32 daqmx_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	dinput = (MX_DIGITAL_INPUT *) record->record_class_struct;

	mx_status = mxd_daqmx_base_dinput_get_pointers(
				dinput, &daqmx_base_dinput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Create a DAQmx Base task. */

	mx_status = mxi_daqmx_base_create_task( record,
					&(daqmx_base_dinput->handle) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Associate a digital input channel with this task. */

	daqmx_status = DAQmxBaseCreateDIChan( daqmx_base_dinput->handle,
					daqmx_base_dinput->channel_name, NULL,
					DAQmx_Val_ChanForAllLines );

#if MXD_DAQMX_BASE_DINPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxBaseCreateDIChan( %#lx, '%s', NULL, %#lx ) = %d",
		fname, (unsigned long) daqmx_base_dinput->handle,
		daqmx_base_dinput->channel_name,
		(unsigned long) DAQmx_Val_ChanForAllLines,
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {

		DAQmxBaseGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to associate digital input '%s' with "
		"DAQmx Base task %#lx failed.  "
		"DAQmx error code = %d, error message = '%s'",
			record->name,
			(unsigned long) daqmx_base_dinput->handle,
			(int) daqmx_status, daqmx_error_message );
	}

	/* Start the task. */

	daqmx_status = DAQmxBaseStartTask( daqmx_base_dinput->handle );

#if MXD_DAQMX_BASE_DINPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxBaseStartTask( %#lx ) = %d",
		fname, (unsigned long) daqmx_base_dinput->handle,
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {

		DAQmxBaseGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to start task %#lx for digital input '%s' "
		"failed.  DAQmx error code = %d, error message = '%s'",
			(unsigned long) daqmx_base_dinput->handle,
			record->name,
			(int) daqmx_status, daqmx_error_message );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_daqmx_base_dinput_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_daqmx_base_dinput_close()";

	MX_DIGITAL_INPUT *dinput;
	MX_DAQMX_BASE_DINPUT *daqmx_base_dinput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	dinput = (MX_DIGITAL_INPUT *) record->record_class_struct;

	mx_status = mxd_daqmx_base_dinput_get_pointers(
				dinput, &daqmx_base_dinput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( daqmx_base_dinput->handle != 0 ) {
		mx_status = mxi_daqmx_base_shutdown_task( record,
						daqmx_base_dinput->handle );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_daqmx_base_dinput_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_daqmx_base_dinput_read()";

	MX_DAQMX_BASE_DINPUT *daqmx_base_dinput;
	char daqmx_error_message[400];
	int32 daqmx_status;
	int32 num_samples;
	double timeout;
	uInt32 read_array[1];
	uInt32 read_array_length;
	int32 num_samples_read;
	mx_status_type mx_status;

	daqmx_base_dinput = NULL;

	mx_status = mxd_daqmx_base_dinput_get_pointers(
				dinput, &daqmx_base_dinput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_samples       = 1;
	timeout           = 10.0;    /* read timeout in seconds */
	read_array_length = 1;

	daqmx_status = DAQmxBaseReadDigitalU32( daqmx_base_dinput->handle,
					num_samples, timeout,
					DAQmx_Val_GroupByChannel,
					read_array, read_array_length,
					&num_samples_read, NULL );

#if MXD_DAQMX_BASE_DINPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxBaseReadDigitalU32( "
	"%#lx, %lu, %f, %#x, read_array, %lu, &num_samples, NULL ) = %d",
		fname, (unsigned long) daqmx_base_dinput->handle,
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
		"The attempt to read digital input '%s' failed.  "
		"DAQmx error code = %d, error message = '%s'",
			dinput->record->name,
			(int) daqmx_status, daqmx_error_message );
	}

#if MXD_DAQMX_BASE_DINPUT_DEBUG
	MX_DEBUG(-2,("%s:   num_samples_read = %lu, read_array[0] = %lu",
		fname, num_samples_read, read_array[0]));
#endif

	dinput->value = read_array[0];

	return mx_status;
}

