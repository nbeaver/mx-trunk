/*
 * Name:    d_daqmx_base_aoutput.c
 *
 * Purpose: MX driver for NI-DAQmx Base analog output channels.
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

#define MXD_DAQMX_BASE_AOUTPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_output.h"
#include "i_daqmx_base.h"
#include "d_daqmx_base_aoutput.h"

/* Initialize the aoutput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_daqmx_base_aoutput_record_function_list = {
	NULL,
	mxd_daqmx_base_aoutput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_daqmx_base_aoutput_open,
	mxd_daqmx_base_aoutput_close
};

MX_ANALOG_OUTPUT_FUNCTION_LIST
		mxd_daqmx_base_aoutput_analog_output_function_list =
{
	NULL,
	mxd_daqmx_base_aoutput_write
};

/* DAQmx Base analog output data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_daqmx_base_aoutput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_DAQMX_BASE_AOUTPUT_STANDARD_FIELDS
};

long mxd_daqmx_base_aoutput_num_record_fields
		= sizeof( mxd_daqmx_base_aoutput_rf_defaults )
		  / sizeof( mxd_daqmx_base_aoutput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_daqmx_base_aoutput_rfield_def_ptr
			= &mxd_daqmx_base_aoutput_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_daqmx_base_aoutput_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_DAQMX_BASE_AOUTPUT **daqmx_base_aoutput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_daqmx_base_aoutput_get_pointers()";

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_OUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( daqmx_base_aoutput == (MX_DAQMX_BASE_AOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DAQMX_BASE_AOUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	*daqmx_base_aoutput = (MX_DAQMX_BASE_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( *daqmx_base_aoutput == (MX_DAQMX_BASE_AOUTPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_DAQMX_BASE_AOUTPUT pointer for "
			"aoutput record '%s' passed by '%s' is NULL",
				aoutput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_daqmx_base_aoutput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_daqmx_base_aoutput_create_record_structures()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_DAQMX_BASE_AOUTPUT *daqmx_base_aoutput;

	/* Allocate memory for the necessary structures. */

	aoutput = (MX_ANALOG_OUTPUT *) malloc( sizeof(MX_ANALOG_OUTPUT) );

	if ( aoutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_ANALOG_OUTPUT structure." );
	}

	daqmx_base_aoutput = (MX_DAQMX_BASE_AOUTPUT *)
				malloc( sizeof(MX_DAQMX_BASE_AOUTPUT) );

	if ( daqmx_base_aoutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_DAQMX_BASE_AOUTPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = aoutput;
	record->record_type_struct = daqmx_base_aoutput;
	record->class_specific_function_list
			= &mxd_daqmx_base_aoutput_analog_output_function_list;

	aoutput->record = record;

	aoutput->subclass = MXT_AOU_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_daqmx_base_aoutput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_daqmx_base_aoutput_open()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_DAQMX_BASE_AOUTPUT *daqmx_base_aoutput = NULL;
	int32 daqmx_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	aoutput = (MX_ANALOG_OUTPUT *) record->record_class_struct;

	mx_status = mxd_daqmx_base_aoutput_get_pointers(
				aoutput, &daqmx_base_aoutput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Create a DAQmx Base task. */

	daqmx_status = DAQmxBaseCreateTask( "", &(daqmx_base_aoutput->handle) );

#if MXD_DAQMX_BASE_AOUTPUT_DEBUG
	MX_DEBUG(-2,
	("%s: DAQmxBaseCreateTask( &(daqmx_base_aoutput->handle) ) = %d",
		fname, (int) daqmx_status));

	MX_DEBUG(-2,("%s:   daqmx_base_aoutput->handle = %#lx",
		fname, (unsigned long) daqmx_base_aoutput->handle));
#endif

	if ( daqmx_status != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to create a DAQmx Base task failed for '%s'.  "
		"DAQmx error code = %d",
			record->name, (int) daqmx_status );
	}

	if ( daqmx_base_aoutput->handle == 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to create a TaskHandle for '%s' failed.",
			record->name );
	}

	/* Associate a analog output channel with this task. */

	daqmx_status = DAQmxBaseCreateAOVoltageChan( daqmx_base_aoutput->handle,
					daqmx_base_aoutput->channel_name, NULL,
					daqmx_base_aoutput->minimum_value,
					daqmx_base_aoutput->maximum_value,
					DAQmx_Val_Volts, NULL );

#if MXD_DAQMX_BASE_AOUTPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxBaseCreateDOChan( %#lx, '%s', NULL, %#lx ) = %d",
		fname, (unsigned long) daqmx_base_aoutput->handle,
		daqmx_base_aoutput->channel_name,
		(unsigned long) DAQmx_Val_ChanForAllLines,
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to associate analog output '%s' with "
		"DAQmx Base task %#lx failed.  DAQmx error code = %d",
			record->name,
			(unsigned long) daqmx_base_aoutput->handle,
			(int) daqmx_status );
	}

	/* Start the task. */

	daqmx_status = DAQmxBaseStartTask( daqmx_base_aoutput->handle );

#if MXD_DAQMX_BASE_AOUTPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxBaseStartTask( %#lx ) = %d",
		fname, (unsigned long) daqmx_base_aoutput->handle,
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to start task %#lx for analog output '%s' "
		"failed.  DAQmx error code = %d",
			(unsigned long) daqmx_base_aoutput->handle,
			record->name,
			(int) daqmx_status );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_daqmx_base_aoutput_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_daqmx_base_aoutput_close()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_DAQMX_BASE_AOUTPUT *daqmx_base_aoutput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	aoutput = (MX_ANALOG_OUTPUT *) record->record_class_struct;

	mx_status = mxd_daqmx_base_aoutput_get_pointers(
				aoutput, &daqmx_base_aoutput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( daqmx_base_aoutput->handle != 0 ) {
		mx_status = mxi_daqmx_base_shutdown_task( record,
						daqmx_base_aoutput->handle );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_daqmx_base_aoutput_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_daqmx_base_aoutput_write()";

	MX_DAQMX_BASE_AOUTPUT *daqmx_base_aoutput;
	int32 daqmx_status;
	int32 num_samples;
	bool32 autostart;
	double timeout;
	float64 write_array[1];
	int32 num_samples_written;
	mx_status_type mx_status;

	daqmx_base_aoutput = NULL;

	mx_status = mxd_daqmx_base_aoutput_get_pointers(
				aoutput, &daqmx_base_aoutput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_samples       = 1;
	autostart         = FALSE;
	timeout           = 10.0;    /* write timeout in seconds */

	write_array[0] = aoutput->raw_value.double_value;

	daqmx_status = DAQmxBaseWriteAnalogF64( daqmx_base_aoutput->handle,
					num_samples, autostart, timeout,
					DAQmx_Val_GroupByChannel,
					write_array, 
					&num_samples_written, NULL );

#if MXD_DAQMX_BASE_AOUTPUT_DEBUG
	MX_DEBUG(-2,("%s: DAQmxBaseWriteDigitalU32( "
	"%#lx, %lu, %lu, %f, %#x, {%f}, &num_samples, NULL ) = %d",
		fname, (unsigned long) daqmx_base_aoutput->handle,
		num_samples,
		autostart,
		timeout,
		DAQmx_Val_GroupByChannel,
		write_array[0],
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to write analog output '%s' failed.  "
		"DAQmx error code = %d",
			aoutput->record->name,
			(int) daqmx_status );
	}

#if MXD_DAQMX_BASE_AOUTPUT_DEBUG
	MX_DEBUG(-2,("%s:   num_samples_written = %lu",
		fname, num_samples_written));
#endif

	return mx_status;
}

