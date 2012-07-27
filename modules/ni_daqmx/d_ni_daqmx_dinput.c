/*
 * Name:    d_ni_daqmx_dinput.c
 *
 * Purpose: MX driver for NI-DAQmx digital input channels.
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

#define MXD_NI_DAQMX_DINPUT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_input.h"
#include "i_ni_daqmx.h"
#include "d_ni_daqmx_dinput.h"

MX_RECORD_FUNCTION_LIST mxd_ni_daqmx_dinput_record_function_list = {
	NULL,
	mxd_ni_daqmx_dinput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_ni_daqmx_dinput_open
};

MX_DIGITAL_INPUT_FUNCTION_LIST
		mxd_ni_daqmx_dinput_digital_input_function_list = {
	mxd_ni_daqmx_dinput_read
};

MX_RECORD_FIELD_DEFAULTS mxd_ni_daqmx_dinput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_NI_DAQMX_DINPUT_STANDARD_FIELDS
};

long mxd_ni_daqmx_dinput_num_record_fields
	= sizeof( mxd_ni_daqmx_dinput_record_field_defaults )
		/ sizeof( mxd_ni_daqmx_dinput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ni_daqmx_dinput_rfield_def_ptr
			= &mxd_ni_daqmx_dinput_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_ni_daqmx_dinput_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_NI_DAQMX_DINPUT **ni_daqmx_dinput,
			MX_NI_DAQMX **ni_daqmx,
			const char *calling_fname )
{
	static const char fname[] = "mxd_ni_daqmx_dinput_get_pointers()";

	MX_NI_DAQMX_DINPUT *ni_daqmx_dinput_ptr;
	MX_RECORD *ni_daqmx_record;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_INPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	ni_daqmx_dinput_ptr = (MX_NI_DAQMX_DINPUT *)
				dinput->record->record_type_struct;

	if ( ni_daqmx_dinput_ptr == (MX_NI_DAQMX_DINPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NI_DAQMX_DINPUT pointer for "
			"DAQmx digital input '%s' passed by '%s' is NULL",
				dinput->record->name, calling_fname );
	}

	if ( ni_daqmx_dinput != (MX_NI_DAQMX_DINPUT **) NULL ) {
		*ni_daqmx_dinput = ni_daqmx_dinput_ptr;
	}

	if ( ni_daqmx != (MX_NI_DAQMX **) NULL ) {
		ni_daqmx_record = ni_daqmx_dinput_ptr->ni_daqmx_record;

		if ( ni_daqmx_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The ni_daqmx_record pointer for "
			"DAQmx digital input '%s' is NULL.",
				dinput->record->name );
		}

		*ni_daqmx = (MX_NI_DAQMX *) ni_daqmx_record->record_type_struct;

		if ( (*ni_daqmx) == (MX_NI_DAQMX *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NI_DAQMX pointer for DAQmx record '%s' "
			"used by digital input '%s' is NULL.",
				ni_daqmx_record->name,
				dinput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_ni_daqmx_dinput_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_ni_daqmx_dinput_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_NI_DAQMX_DINPUT *ni_daqmx_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_DIGITAL_INPUT structure." );
        }

        ni_daqmx_dinput = (MX_NI_DAQMX_DINPUT *)
				malloc( sizeof(MX_NI_DAQMX_DINPUT) );

        if ( ni_daqmx_dinput == (MX_NI_DAQMX_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_NI_DAQMX_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = ni_daqmx_dinput;
        record->class_specific_function_list
			= &mxd_ni_daqmx_dinput_digital_input_function_list;

        digital_input->record = record;
	ni_daqmx_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ni_daqmx_dinput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_ni_daqmx_dinput_open()";

	MX_DIGITAL_INPUT *dinput;
	MX_NI_DAQMX_DINPUT *ni_daqmx_dinput = NULL;
	MX_NI_DAQMX *ni_daqmx = NULL;
	char daqmx_error_message[400];
	int32 daqmx_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	dinput = (MX_DIGITAL_INPUT *) record->record_class_struct;

	mx_status = mxd_ni_daqmx_dinput_get_pointers(
				dinput, &ni_daqmx_dinput, &ni_daqmx, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Create a DAQmx task. */

	mx_status = mxi_ni_daqmx_find_or_create_task( ni_daqmx,
						ni_daqmx_dinput->task_name,
						&(ni_daqmx_dinput->task) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Associate a digital input channel with this task. */

	daqmx_status = DAQmxCreateDIChan( ni_daqmx_dinput->task->task_handle,
					ni_daqmx_dinput->channel_name, NULL,
					DAQmx_Val_ChanForAllLines );

#if MXD_NI_DAQMX_DINPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxCreateDIChan( %#lx, '%s', NULL, %#lx ) = %d",
		fname, (unsigned long) ni_daqmx_dinput->task->task_handle,
		ni_daqmx_dinput->channel_name,
		(unsigned long) DAQmx_Val_ChanForAllLines,
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {

		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to associate digital input '%s' with "
		"DAQmx task %#lx failed.  "
		"DAQmx error code = %d, error message = '%s'",
			record->name,
			(unsigned long) ni_daqmx_dinput->task->task_handle,
			(int) daqmx_status, daqmx_error_message );
	}

	/* The task will be started in the "finish delayed initialization"
	 * driver function of the "ni_daqmx" record.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ni_daqmx_dinput_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_ni_daqmx_dinput_read()";

	MX_NI_DAQMX_DINPUT *ni_daqmx_dinput = NULL;
	MX_NI_DAQMX *ni_daqmx = NULL;
	char daqmx_error_message[400];
	int32 daqmx_status;
	int32 num_samples;
	double timeout;
	uInt32 read_array[1];
	uInt32 read_array_length;
	int32 num_samples_read;
	mx_status_type mx_status;

	ni_daqmx_dinput = NULL;

	mx_status = mxd_ni_daqmx_dinput_get_pointers(
				dinput, &ni_daqmx_dinput, &ni_daqmx, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_samples       = 1;
	timeout           = 10.0;    /* read timeout in seconds */
	read_array_length = 1;

	daqmx_status = DAQmxReadDigitalU32( ni_daqmx_dinput->task->task_handle,
					num_samples, timeout,
					DAQmx_Val_GroupByChannel,
					read_array, read_array_length,
					&num_samples_read, NULL );

#if MXD_NI_DAQMX_DINPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxReadDigitalU32( "
	"%#lx, %lu, %f, %#x, read_array, %lu, &num_samples, NULL ) = %d",
		fname, (unsigned long) ni_daqmx_dinput->task->task_handle,
		num_samples,
		timeout,
		DAQmx_Val_GroupByChannel,
		(unsigned long) read_array_length,
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {

		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to read digital input '%s' failed.  "
		"DAQmx error code = %d, error message = '%s'",
			dinput->record->name,
			(int) daqmx_status, daqmx_error_message );
	}

#if MXD_NI_DAQMX_DINPUT_DEBUG
	MX_DEBUG(-2,("%s:   num_samples_read = %lu, read_array[0] = %lu",
		fname, num_samples_read, read_array[0]));
#endif

	dinput->value = read_array[0];

	return mx_status;
}

