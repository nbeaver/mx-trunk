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
	mxd_ni_daqmx_doutput_finish_record_initialization,
	NULL,
	NULL,
	mxd_ni_daqmx_doutput_open,
	mxd_ni_daqmx_doutput_close,
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
			const char *calling_fname )
{
	static const char fname[] = "mxd_ni_daqmx_doutput_get_pointers()";

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( ni_daqmx_doutput == (MX_NI_DAQMX_DOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NI_DAQMX_DOUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	*ni_daqmx_doutput = (MX_NI_DAQMX_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( *ni_daqmx_doutput == (MX_NI_DAQMX_DOUTPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NI_DAQMX_DOUTPUT pointer for "
			"doutput record '%s' passed by '%s' is NULL",
				doutput->record->name, calling_fname );
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
mxd_ni_daqmx_doutput_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_ni_daqmx_doutput_finish_record_initialization()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_NI_DAQMX_DOUTPUT *ni_daqmx_doutput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	doutput = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	mx_status = mxd_ni_daqmx_doutput_get_pointers(
				doutput, &ni_daqmx_doutput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ni_daqmx_doutput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_ni_daqmx_doutput_open()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_NI_DAQMX_DOUTPUT *ni_daqmx_doutput = NULL;
	char daqmx_error_message[400];
	int32 daqmx_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	doutput = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	mx_status = mxd_ni_daqmx_doutput_get_pointers(
				doutput, &ni_daqmx_doutput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Create a DAQmx task. */

	mx_status = mxi_ni_daqmx_create_task( record,
					&(ni_daqmx_doutput->handle) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Associate a digital output channel with this task. */

	daqmx_status = DAQmxCreateDOChan( ni_daqmx_doutput->handle,
					ni_daqmx_doutput->channel_name, NULL,
					DAQmx_Val_ChanForAllLines );

#if MXD_NI_DAQMX_DOUTPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxCreateDOChan( %#lx, '%s', NULL, %#lx ) = %d",
		fname, (unsigned long) ni_daqmx_doutput->handle,
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
			(unsigned long) ni_daqmx_doutput->handle,
			(int) daqmx_status, daqmx_error_message );
	}

	/* Start the task. */

	daqmx_status = DAQmxStartTask( ni_daqmx_doutput->handle );

#if MXD_NI_DAQMX_DOUTPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxStartTask( %#lx ) = %d",
		fname, (unsigned long) ni_daqmx_doutput->handle,
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {

		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to start task %#lx for digital output '%s' "
		"failed.  DAQmx error code = %d, error message = '%s'",
			(unsigned long) ni_daqmx_doutput->handle,
			record->name,
			(int) daqmx_status, daqmx_error_message );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ni_daqmx_doutput_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_ni_daqmx_doutput_close()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_NI_DAQMX_DOUTPUT *ni_daqmx_doutput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	doutput = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	mx_status = mxd_ni_daqmx_doutput_get_pointers(
				doutput, &ni_daqmx_doutput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ni_daqmx_doutput->handle != 0 ) {
		mx_status = mxi_ni_daqmx_shutdown_task( record,
						ni_daqmx_doutput->handle );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_ni_daqmx_doutput_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_ni_daqmx_doutput_write()";

	MX_NI_DAQMX_DOUTPUT *ni_daqmx_doutput;
	char daqmx_error_message[400];
	int32 daqmx_status;
	int32 num_samples;
	bool32 autostart;
	double timeout;
	uInt32 write_array[1];
	int32 num_samples_written;
	mx_status_type mx_status;

	ni_daqmx_doutput = NULL;

	mx_status = mxd_ni_daqmx_doutput_get_pointers(
				doutput, &ni_daqmx_doutput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_samples       = 1;
	autostart         = FALSE;
	timeout           = 10.0;    /* write timeout in seconds */

	write_array[0] = doutput->value;

	daqmx_status = DAQmxWriteDigitalU32( ni_daqmx_doutput->handle,
					num_samples, autostart, timeout,
					DAQmx_Val_GroupByChannel,
					write_array, 
					&num_samples_written, NULL );

#if MXD_NI_DAQMX_DOUTPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxWriteDigitalU32( "
	"%#lx, %lu, %lu, %f, %#x, {%lu}, &num_samples, NULL ) = %d",
		fname, (unsigned long) ni_daqmx_doutput->handle,
		num_samples,
		autostart,
		timeout,
		DAQmx_Val_GroupByChannel,
		write_array[0],
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

