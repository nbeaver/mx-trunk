/*
 * Name:    d_daqmx_base_thermocouple.c
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

#define MXD_DAQMX_BASE_THERMOCOUPLE_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "i_daqmx_base.h"
#include "d_daqmx_base_thermocouple.h"

/* Initialize the ainput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_daqmx_base_thermocouple_record_function_list = {
	NULL,
	mxd_daqmx_base_thermocouple_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_daqmx_base_thermocouple_open,
	mxd_daqmx_base_thermocouple_close
};

MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_daqmx_base_thermocouple_analog_input_function_list =
{
	mxd_daqmx_base_thermocouple_read
};

/* DAQmx Base analog input data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_daqmx_base_thermocouple_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_DAQMX_BASE_THERMOCOUPLE_STANDARD_FIELDS
};

long mxd_daqmx_base_thermocouple_num_record_fields
		= sizeof( mxd_daqmx_base_thermocouple_rf_defaults )
		  / sizeof( mxd_daqmx_base_thermocouple_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_daqmx_base_thermocouple_rfield_def_ptr
			= &mxd_daqmx_base_thermocouple_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_daqmx_base_thermocouple_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_DAQMX_BASE_THERMOCOUPLE **daqmx_base_thermocouple,
			const char *calling_fname )
{
	static const char fname[] = "mxd_daqmx_base_thermocouple_get_pointers()";

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_INPUT pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( daqmx_base_thermocouple == (MX_DAQMX_BASE_THERMOCOUPLE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DAQMX_BASE_THERMOCOUPLE pointer passed by '%s' was NULL",
			calling_fname );
	}

	*daqmx_base_thermocouple = (MX_DAQMX_BASE_THERMOCOUPLE *)
				ainput->record->record_type_struct;

	if ( *daqmx_base_thermocouple == (MX_DAQMX_BASE_THERMOCOUPLE *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_DAQMX_BASE_THERMOCOUPLE pointer for "
			"ainput record '%s' passed by '%s' is NULL",
				ainput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_daqmx_base_thermocouple_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_daqmx_base_thermocouple_create_record_structures()";

	MX_ANALOG_INPUT *ainput;
	MX_DAQMX_BASE_THERMOCOUPLE *daqmx_base_thermocouple;

	/* Allocate memory for the necessary structures. */

	ainput = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_ANALOG_INPUT structure." );
	}

	daqmx_base_thermocouple = (MX_DAQMX_BASE_THERMOCOUPLE *)
				malloc( sizeof(MX_DAQMX_BASE_THERMOCOUPLE) );

	if ( daqmx_base_thermocouple == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for MX_DAQMX_BASE_THERMOCOUPLE structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = ainput;
	record->record_type_struct = daqmx_base_thermocouple;
	record->class_specific_function_list
		= &mxd_daqmx_base_thermocouple_analog_input_function_list;

	ainput->record = record;

	ainput->subclass = MXT_AOU_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_daqmx_base_thermocouple_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_daqmx_base_thermocouple_open()";

	MX_ANALOG_INPUT *ainput;
	MX_DAQMX_BASE_THERMOCOUPLE *daqmx_base_thermocouple = NULL;
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

	mx_status = mxd_daqmx_base_thermocouple_get_pointers(
				ainput, &daqmx_base_thermocouple, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* What are the raw thermocouple units? */

	units_name = daqmx_base_thermocouple->thermocouple_units;

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

	type_name = daqmx_base_thermocouple->thermocouple_type;

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

	cold_junction_type_name = daqmx_base_thermocouple->cold_junction;

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

	/* Create a DAQmx Base task. */

	mx_status = mxi_daqmx_base_create_task( record,
					&(daqmx_base_thermocouple->handle) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Associate a thermocouple input channel with this task. */

	daqmx_status = DAQmxBaseCreateAIThrmcplChan(
				daqmx_base_thermocouple->handle,
				daqmx_base_thermocouple->channel_name, NULL,
				daqmx_base_thermocouple->minimum_value,
				daqmx_base_thermocouple->maximum_value,
				units, thermocouple_type, cold_junction_type,
			daqmx_base_thermocouple->cold_junction_temperature,
			daqmx_base_thermocouple->cold_junction_channel_name );

#if MXD_DAQMX_BASE_THERMOCOUPLE_DEBUG
	MX_DEBUG(-2,("%s: DAQmxBaseCreateDOChan( %#lx, '%s', NULL, %#lx ) = %d",
		fname, (unsigned long) daqmx_base_thermocouple->handle,
		daqmx_base_thermocouple->channel_name,
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
			(unsigned long) daqmx_base_thermocouple->handle,
			(int) daqmx_status, daqmx_error_message );
	}

	/* Start the task. */

	daqmx_status = DAQmxBaseStartTask( daqmx_base_thermocouple->handle );

#if MXD_DAQMX_BASE_THERMOCOUPLE_DEBUG
	MX_DEBUG(-2,("%s: DAQmxBaseStartTask( %#lx ) = %d",
		fname, (unsigned long) daqmx_base_thermocouple->handle,
		(int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {

		DAQmxBaseGetExtendedErrorInfo( daqmx_error_message,
					sizeof(daqmx_error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to start task %#lx for analog input '%s' "
		"failed.  DAQmx error code = %d, error message = '%s'",
			(unsigned long) daqmx_base_thermocouple->handle,
			record->name,
			(int) daqmx_status, daqmx_error_message );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_daqmx_base_thermocouple_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_daqmx_base_thermocouple_close()";

	MX_ANALOG_INPUT *ainput;
	MX_DAQMX_BASE_THERMOCOUPLE *daqmx_base_thermocouple = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	ainput = (MX_ANALOG_INPUT *) record->record_class_struct;

	mx_status = mxd_daqmx_base_thermocouple_get_pointers(
				ainput, &daqmx_base_thermocouple, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( daqmx_base_thermocouple->handle != 0 ) {
		mx_status = mxi_daqmx_base_shutdown_task( record,
						daqmx_base_thermocouple->handle );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_daqmx_base_thermocouple_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_daqmx_base_thermocouple_read()";

	MX_DAQMX_BASE_THERMOCOUPLE *daqmx_base_thermocouple;
	char daqmx_error_message[400];
	int32 daqmx_status;
	int32 num_samples;
	double timeout;
	float64 read_array[1];
	int32 read_array_length;
	int32 num_samples_read;
	mx_status_type mx_status;

	daqmx_base_thermocouple = NULL;

	mx_status = mxd_daqmx_base_thermocouple_get_pointers(
				ainput, &daqmx_base_thermocouple, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_samples       = 1;
	timeout           = 10.0;    /* read timeout in seconds */
	read_array_length = 1;

	daqmx_status = DAQmxBaseReadAnalogF64( daqmx_base_thermocouple->handle,
					num_samples, timeout,
					DAQmx_Val_GroupByChannel,
					read_array, read_array_length,
					&num_samples_read, NULL );

#if MXD_DAQMX_BASE_THERMOCOUPLE_DEBUG
	MX_DEBUG(-2,("%s: DAQmxBaseReadAnalogF64( "
	"%#lx, %lu, %f, %#x, read_array, %lu, &num_samples, NULL ) = %d",
		fname, (unsigned long) daqmx_base_thermocouple->handle,
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

#if MXD_DAQMX_BASE_THERMOCOUPLE_DEBUG
	MX_DEBUG(-2,("%s:   num_samples_read = %lu, read_array[0] = %lu",
		fname, num_samples_read, read_array[0]));
#endif

	ainput->raw_value.double_value = read_array[0];

	return mx_status;
}

