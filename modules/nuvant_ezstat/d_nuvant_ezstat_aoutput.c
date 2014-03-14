/*
 * Name:    d_nuvant_ezstat_aoutput.c
 *
 * Purpose: MX driver for NuVant EZstat analog output channels.
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

#define MXD_NUVANT_EZSTAT_AOUTPUT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_output.h"
#include "mx_digital_output.h"
#include "i_nuvant_ezstat.h"
#include "d_nuvant_ezstat_aoutput.h"

/* Initialize the aoutput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_nuvant_ezstat_aoutput_record_function_list = {
	NULL,
	mxd_nuvant_ezstat_aoutput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_nuvant_ezstat_aoutput_open
};

MX_ANALOG_OUTPUT_FUNCTION_LIST
		mxd_nuvant_ezstat_aoutput_analog_output_function_list =
{
	NULL,
	mxd_nuvant_ezstat_aoutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_nuvant_ezstat_aoutput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_NUVANT_EZSTAT_AOUTPUT_STANDARD_FIELDS
};

long mxd_nuvant_ezstat_aoutput_num_record_fields
		= sizeof( mxd_nuvant_ezstat_aoutput_rf_defaults )
		  / sizeof( mxd_nuvant_ezstat_aoutput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_nuvant_ezstat_aoutput_rfield_def_ptr
			= &mxd_nuvant_ezstat_aoutput_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_nuvant_ezstat_aoutput_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_NUVANT_EZSTAT_AOUTPUT **ezstat_aoutput,
			MX_NUVANT_EZSTAT **ezstat,
			const char *calling_fname )
{
	static const char fname[] = "mxd_nuvant_ezstat_aoutput_get_pointers()";

	MX_NUVANT_EZSTAT_AOUTPUT *ezstat_aoutput_ptr;
	MX_RECORD *ezstat_record;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_OUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	ezstat_aoutput_ptr = (MX_NUVANT_EZSTAT_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( ezstat_aoutput_ptr == (MX_NUVANT_EZSTAT_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NUVANT_EZSTAT_AOUTPUT pointer for "
		"analog output '%s' passed by '%s' is NULL",
			aoutput->record->name, calling_fname );
	}

	if ( ezstat_aoutput != (MX_NUVANT_EZSTAT_AOUTPUT **) NULL ) {
		*ezstat_aoutput = ezstat_aoutput_ptr;
	}

	if ( ezstat != (MX_NUVANT_EZSTAT **) NULL ) {
		ezstat_record = ezstat_aoutput_ptr->nuvant_ezstat_record;

		if ( ezstat_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The nuvant_ezstat_record pointer for "
			"analog output '%s' is NULL.",
				aoutput->record->name );
		}

		*ezstat = (MX_NUVANT_EZSTAT *)ezstat_record->record_type_struct;

		if ( (*ezstat) == (MX_NUVANT_EZSTAT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NUVANT_EZSTAT pointer for record '%s' "
			"used by analog output '%s' is NULL.",
				ezstat_record->name,
				aoutput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_nuvant_ezstat_aoutput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_nuvant_ezstat_aoutput_create_record_structures()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_NUVANT_EZSTAT_AOUTPUT *nuvant_ezstat_aoutput;

	/* Allocate memory for the necessary structures. */

	aoutput = (MX_ANALOG_OUTPUT *) malloc( sizeof(MX_ANALOG_OUTPUT) );

	if ( aoutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_ANALOG_OUTPUT structure." );
	}

	nuvant_ezstat_aoutput = (MX_NUVANT_EZSTAT_AOUTPUT *)
				malloc( sizeof(MX_NUVANT_EZSTAT_AOUTPUT) );

	if ( nuvant_ezstat_aoutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_NUVANT_EZSTAT_AOUTPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = aoutput;
	record->record_type_struct = nuvant_ezstat_aoutput;
	record->class_specific_function_list
		= &mxd_nuvant_ezstat_aoutput_analog_output_function_list;

	aoutput->record = record;

	aoutput->subclass = MXT_AOU_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_nuvant_ezstat_aoutput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_nuvant_ezstat_aoutput_open()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_NUVANT_EZSTAT_AOUTPUT *ezstat_aoutput = NULL;
	MX_NUVANT_EZSTAT *ezstat = NULL;
	char *type_name = NULL;
	size_t len;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	aoutput = (MX_ANALOG_OUTPUT *) record->record_class_struct;

	mx_status = mxd_nuvant_ezstat_aoutput_get_pointers( aoutput,
					&ezstat_aoutput, &ezstat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	type_name = ezstat_aoutput->output_type_name;

	len = strlen( type_name );

	if ( mx_strncasecmp( type_name, "potentiostat_voltage", len ) == 0 ) {
		ezstat_aoutput->output_type =
			MXT_NUVANT_EZSTAT_AOUTPUT_POTENTIOSTAT_VOLTAGE;
	} else
	if ( mx_strncasecmp( type_name, "galvanostat_current", len ) == 0 ) {
		ezstat_aoutput->output_type =
			MXT_NUVANT_EZSTAT_AOUTPUT_GALVANOSTAT_CURRENT;
	} else
	if ( mx_strncasecmp( type_name, "potentiostat_current_range", len )
								== 0 )
	{
		ezstat_aoutput->output_type =
			MXT_NUVANT_EZSTAT_AOUTPUT_POTENTIOSTAT_CURRENT_RANGE;
	} else
	if ( mx_strncasecmp( type_name, "galvanostat_current_range", len )
								== 0 )
	{
		ezstat_aoutput->output_type =
			MXT_NUVANT_EZSTAT_AOUTPUT_GALVANOSTAT_CURRENT_RANGE;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Input type '%s' is not supported for "
		"analog output '%s'.", type_name, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

static mx_status_type
mxd_nea_set_potentiostat_voltage( MX_ANALOG_OUTPUT *aoutput,
				MX_NUVANT_EZSTAT_AOUTPUT *ezstat_aoutput,
				MX_NUVANT_EZSTAT *ezstat )
{
	static const char fname[] = "mxd_nea_set_potentiostat_voltage()";

	TaskHandle doutput_task_handle, voltage_task_handle;
	char doutput_channel_names[200];
	char voltage_channel_name[40];
	int32 daqmx_status;
	char daqmx_error_message[200];
	uInt32 pin_values, potentiostat_binary_range;
	uInt32 digital_write_array[1];
	float64 voltage_write_array[1];
	uInt32 samples_written;
	mx_status_type mx_status;

	/* First, we must setup the digital control pins correctly.
	 * 
	 * We set up the following array of digital output channels:
	 * Bit 0 = P0.0 (cell enable)
	 * Bit 1 = P0.1 (external switch)
	 * Bit 2 = P1.0 (select galvanostat or potentiostat mode)
	 * Bit 3 = P1.3 (bits 3 and 4 select the potentiostat current range)
	 * Bit 4 = P1.4
	 * Bit 5 = P1.5 (controls sample and hold circuit for range changes)
	 */

	mx_status = mxi_nuvant_ezstat_create_task(
				"ezstat_configure_potentiostat_bits",
				&doutput_task_handle );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( doutput_channel_names, sizeof(doutput_channel_names),
	"%s/port0/line0:1,%s/port1/line0,%s/port1/line3:4,%s/port1/line5",
		ezstat->device_name, ezstat->device_name,
		ezstat->device_name, ezstat->device_name );

	daqmx_status = DAQmxCreateDOChan( doutput_task_handle,
					doutput_channel_names, NULL,
					DAQmx_Val_ChanForAllLines );

	if ( daqmx_status != 0 ) {
		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to associate digital I/O pins '%s' for "
		"potentiostat voltage output '%s' with DAQmx task %#lx failed."
		"  DAQmx error code = %d, error message = '%s'",
			doutput_channel_names,
			aoutput->record->name,
			(unsigned long) doutput_task_handle,
			(int) daqmx_status, daqmx_error_message );
	}

	mx_status = mxi_nuvant_ezstat_start_task( doutput_task_handle );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Construct the value to send to the digital I/O pins. */

	pin_values = 0x1;	/* Cell enable selected.  All others clear. */

	potentiostat_binary_range = ezstat->potentiostat_binary_range;

	/*** Replace bits 3 and 4 with the potentiostat current range. ***/

	/* Mask off bits 3 and 4. */
	pin_values &= (~ 0x18);

	/* Mask off all but the two low order bits */
	potentiostat_binary_range &= 0x3;

	/* Replace bits 3 and 4 with the potentiostat range bits. */
	pin_values |= ( potentiostat_binary_range << 3 );

	digital_write_array[0] = pin_values;

	/* Send the bit values to the I/O pins. */

	daqmx_status = DAQmxWriteDigitalU32( doutput_task_handle,
					1, FALSE, 1.0,
					DAQmx_Val_GroupByChannel,
					digital_write_array,
					&samples_written, NULL );

	if ( daqmx_status != 0 ) {
		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to write the digital output samples for "
		"DAQmx task %#lx used by record '%s' failed.  "
		"DAQmx error code = %d, error message = '%s'",
			(unsigned long) doutput_task_handle,
			aoutput->record->name,
			(int) daqmx_status, daqmx_error_message );
	}

	/* We are done with the digital output task, so get rid of it. */

	mx_status = mxi_nuvant_ezstat_shutdown_task( doutput_task_handle );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now we send the potentiostat output voltage. */

	mx_status = mxi_nuvant_ezstat_create_task(
				"ezstat_set_potentiostat_voltage",
				&voltage_task_handle );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( voltage_channel_name, sizeof(voltage_channel_name),
		"%s/ao0", ezstat->device_name );

	daqmx_status = DAQmxCreateAOVoltageChan( voltage_task_handle,
						voltage_channel_name, NULL,
						-10.0, 10.0,
						DAQmx_Val_Volts, NULL );

	if ( daqmx_status != 0 ) {
		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to associate analog output pin '%s' for "
		"potentiostat voltage output '%s' with DAQmx task %#lx failed."
		"  DAQmx error code = %d, error message = '%s'",
			voltage_channel_name,
			aoutput->record->name,
			(unsigned long) voltage_task_handle,
			(int) daqmx_status, daqmx_error_message );
	}

	/* Now write the actual voltage. */

	voltage_write_array[0] = aoutput->raw_value.double_value;

	daqmx_status = DAQmxWriteAnalogF64( voltage_task_handle,
					1, FALSE, 1.0,
					DAQmx_Val_GroupByChannel,
					voltage_write_array,
					&samples_written, NULL );

	if ( daqmx_status != 0 ) {
		DAQmxGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to write the potentiostat voltage %g for "
		"DAQmx task %#lx used by record '%s' failed.  "
		"DAQmx error code = %d, error message = '%s'",
			aoutput->raw_value.double_value,
			(unsigned long) doutput_task_handle,
			aoutput->record->name,
			(int) daqmx_status, daqmx_error_message );
	}

	mx_status = mxi_nuvant_ezstat_shutdown_task( voltage_task_handle );

	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_nuvant_ezstat_aoutput_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_nuvant_ezstat_aoutput_write()";

	MX_NUVANT_EZSTAT_AOUTPUT *ezstat_aoutput = NULL;
	MX_NUVANT_EZSTAT *ezstat = NULL;
	unsigned long p11_value, p12_value, p13_value, p14_value;
	double ao0_value, raw_value;
	mx_status_type mx_status;

	mx_status = mxd_nuvant_ezstat_aoutput_get_pointers( aoutput,
					&ezstat_aoutput, &ezstat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ezstat_aoutput->output_type ) {
	case MXT_NUVANT_EZSTAT_AOUTPUT_POTENTIOSTAT_VOLTAGE:
		mx_status = mxd_nea_set_potentiostat_voltage( aoutput,
							ezstat_aoutput,
							ezstat );
		break;
	case MXT_NUVANT_EZSTAT_AOUTPUT_GALVANOSTAT_CURRENT:
		break;
	case MXT_NUVANT_EZSTAT_AOUTPUT_POTENTIOSTAT_CURRENT_RANGE:
		mx_status = mxi_nuvant_ezstat_set_current_range( ezstat,
					MXF_NUVANT_EZSTAT_POTENTIOSTAT_MODE,
					aoutput->raw_value.double_value );
		break;
	case MXT_NUVANT_EZSTAT_AOUTPUT_GALVANOSTAT_CURRENT_RANGE:
		mx_status = mxi_nuvant_ezstat_set_current_range( ezstat,
					MXF_NUVANT_EZSTAT_GALVANOSTAT_MODE,
					aoutput->raw_value.double_value );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal output type %lu requested for record '%s'.",
			ezstat_aoutput->output_type,
			aoutput->record->name );
		break;
	}

	return mx_status;
}

