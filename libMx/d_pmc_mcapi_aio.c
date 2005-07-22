/*
 * Name:    d_pmc_mcapi_aio.c
 *
 * Purpose: MX input drivers to control PMC MCAPI analog input ports.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PMC_MCAPI_AIO_DEBUG  FALSE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_PMC_MCAPI && defined(OS_WIN32)

#include "windows.h"

/* Vendor include file */

#ifndef _WIN32
#define _WIN32
#endif

#include "Mcapi.h"

#include "mx_util.h"
#include "mx_types.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "i_pmc_mcapi.h"
#include "d_pmc_mcapi_aio.h"

/* Initialize the PMC_MCAPI analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_pmc_mcapi_ain_record_function_list = {
	NULL,
	mxd_pmc_mcapi_ain_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_pmc_mcapi_ain_analog_input_function_list = {
	mxd_pmc_mcapi_ain_read
};

MX_RECORD_FIELD_DEFAULTS mxd_pmc_mcapi_ain_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_PMC_MCAPI_AINPUT_STANDARD_FIELDS
};

long mxd_pmc_mcapi_ain_num_record_fields
		= sizeof( mxd_pmc_mcapi_ain_record_field_defaults )
			/ sizeof( mxd_pmc_mcapi_ain_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pmc_mcapi_ain_rfield_def_ptr
			= &mxd_pmc_mcapi_ain_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_pmc_mcapi_aout_record_function_list = {
	NULL,
	mxd_pmc_mcapi_aout_create_record_structures
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_pmc_mcapi_aout_analog_output_function_list =
{
	mxd_pmc_mcapi_aout_read,
	mxd_pmc_mcapi_aout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_pmc_mcapi_aout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_PMC_MCAPI_AOUTPUT_STANDARD_FIELDS
};

long mxd_pmc_mcapi_aout_num_record_fields
		= sizeof( mxd_pmc_mcapi_aout_record_field_defaults )
			/ sizeof( mxd_pmc_mcapi_aout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pmc_mcapi_aout_rfield_def_ptr
			= &mxd_pmc_mcapi_aout_record_field_defaults[0];

/* === */

static mx_status_type
mxd_pmc_mcapi_ain_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_PMC_MCAPI_AINPUT **pmc_mcapi_ainput,
			MX_PMC_MCAPI **pmc_mcapi,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pmc_mcapi_ain_get_pointers()";

	MX_RECORD *pmc_mcapi_record;
	MX_PMC_MCAPI_AINPUT *pmc_mcapi_ainput_pointer;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	pmc_mcapi_ainput_pointer = (MX_PMC_MCAPI_AINPUT *)
				ainput->record->record_type_struct;

	if ( pmc_mcapi_ainput_pointer == (MX_PMC_MCAPI_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PMC_MCAPI_AINPUT pointer for record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	if ( pmc_mcapi_ainput != (MX_PMC_MCAPI_AINPUT **) NULL ) {
		*pmc_mcapi_ainput = pmc_mcapi_ainput_pointer;
	}

	if ( pmc_mcapi != (MX_PMC_MCAPI **) NULL ) {

		pmc_mcapi_record =
			pmc_mcapi_ainput_pointer->pmc_mcapi_record;

		if ( pmc_mcapi_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PMC_MCAPI pointer for PMC_MCAPI analog input "
		"record '%s' passed by '%s' is NULL.",
				ainput->record->name, calling_fname );
		}

		*pmc_mcapi = (MX_PMC_MCAPI *)
			pmc_mcapi_record->record_type_struct;

		if ( *pmc_mcapi == (MX_PMC_MCAPI *) NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_PMC_MCAPI pointer for PMC MCAPI record '%s' used by "
	"PMC MCAPI analog input record '%s' and passed by '%s' is NULL.",
				pmc_mcapi_record->name,
				ainput->record->name,
				calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pmc_mcapi_aout_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_PMC_MCAPI_AOUTPUT **pmc_mcapi_aoutput,
			MX_PMC_MCAPI **pmc_mcapi,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pmc_mcapi_aout_get_pointers()";

	MX_RECORD *pmc_mcapi_record;
	MX_PMC_MCAPI_AOUTPUT *pmc_mcapi_aoutput_pointer;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	pmc_mcapi_aoutput_pointer = (MX_PMC_MCAPI_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( pmc_mcapi_aoutput_pointer == (MX_PMC_MCAPI_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PMC_MCAPI_AOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	if ( pmc_mcapi_aoutput != (MX_PMC_MCAPI_AOUTPUT **) NULL ) {
		*pmc_mcapi_aoutput = pmc_mcapi_aoutput_pointer;
	}

	if ( pmc_mcapi != (MX_PMC_MCAPI **) NULL ) {

		pmc_mcapi_record =
			pmc_mcapi_aoutput_pointer->pmc_mcapi_record;

		if ( pmc_mcapi_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PMC_MCAPI pointer for PMC_MCAPI analog output "
		"record '%s' passed by '%s' is NULL.",
				aoutput->record->name, calling_fname );
		}

		*pmc_mcapi = (MX_PMC_MCAPI *)
			pmc_mcapi_record->record_type_struct;

		if ( *pmc_mcapi == (MX_PMC_MCAPI *) NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_PMC_MCAPI pointer for PMC MCAPI record '%s' used by "
	"PMC MCAPI analog output record '%s' and passed by '%s' is NULL.",
				pmc_mcapi_record->name,
				aoutput->record->name,
				calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions ===== */

MX_EXPORT mx_status_type
mxd_pmc_mcapi_ain_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_pmc_mcapi_ain_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_PMC_MCAPI_AINPUT *pmc_mcapi_ainput;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        pmc_mcapi_ainput = (MX_PMC_MCAPI_AINPUT *)
				malloc( sizeof(MX_PMC_MCAPI_AINPUT) );

        if ( pmc_mcapi_ainput == (MX_PMC_MCAPI_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_PMC_MCAPI_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = pmc_mcapi_ainput;
        record->class_specific_function_list
			= &mxd_pmc_mcapi_ain_analog_input_function_list;

        analog_input->record = record;
	pmc_mcapi_ainput->record = record;

	/* Raw analog input values are stored as longs. */

	analog_input->subclass = MXT_AIN_LONG;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_ain_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_pmc_mcapi_ain_read()";

	MX_PMC_MCAPI_AINPUT *pmc_mcapi_ainput;
	MX_PMC_MCAPI *pmc_mcapi;
	DWORD value;
	long mcapi_status;
	char error_buffer[100];
	mx_status_type mx_status;

	mx_status = mxd_pmc_mcapi_ain_get_pointers( ainput,
			&pmc_mcapi_ainput, &pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mcapi_status = MCGetAnalogEx( pmc_mcapi->controller_handle,
					pmc_mcapi_ainput->channel_number,
					&value );

	if ( mcapi_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( mcapi_status,
			error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Error reading value for analog input '%s'.  "
		"MCAPI error message = '%s'",
			ainput->record->name,
			error_buffer );
	}

	ainput->raw_value.long_value = value;

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions ===== */

MX_EXPORT mx_status_type
mxd_pmc_mcapi_aout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_pmc_mcapi_aout_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_PMC_MCAPI_AOUTPUT *pmc_mcapi_aoutput;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *) malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        pmc_mcapi_aoutput = (MX_PMC_MCAPI_AOUTPUT *)
				malloc( sizeof(MX_PMC_MCAPI_AOUTPUT) );

        if ( pmc_mcapi_aoutput == (MX_PMC_MCAPI_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_PMC_MCAPI_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = pmc_mcapi_aoutput;
        record->class_specific_function_list
			= &mxd_pmc_mcapi_aout_analog_output_function_list;

        analog_output->record = record;
	pmc_mcapi_aoutput->record = record;

	/* Raw analog output values are stored as longs. */

	analog_output->subclass = MXT_AOU_LONG;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_aout_read( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_pmc_mcapi_aout_read()";

	MX_PMC_MCAPI_AOUTPUT *pmc_mcapi_aoutput;
	MX_PMC_MCAPI *pmc_mcapi;
	DWORD value;
	long mcapi_status;
	char error_buffer[100];
	mx_status_type mx_status;

	mx_status = mxd_pmc_mcapi_aout_get_pointers( aoutput,
			&pmc_mcapi_aoutput, &pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mcapi_status = MCGetAnalogEx( pmc_mcapi->controller_handle,
					pmc_mcapi_aoutput->channel_number,
					&value );

	if ( mcapi_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( mcapi_status,
			error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Error reading value for analog output '%s'.  "
		"MCAPI error message = '%s'",
			aoutput->record->name,
			error_buffer );
	}

	aoutput->raw_value.long_value = value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_aout_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_pmc_mcapi_aout_write()";

	MX_PMC_MCAPI_AOUTPUT *pmc_mcapi_aoutput;
	MX_PMC_MCAPI *pmc_mcapi;
	DWORD value;
	long mcapi_status;
	char error_buffer[100];
	mx_status_type mx_status;

	mx_status = mxd_pmc_mcapi_aout_get_pointers( aoutput,
			&pmc_mcapi_aoutput, &pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	value = aoutput->raw_value.long_value;

	mcapi_status = MCSetAnalogEx( pmc_mcapi->controller_handle,
					pmc_mcapi_aoutput->channel_number,
					value );

	if ( mcapi_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( mcapi_status,
			error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Error writing value for analog output '%s'.  "
		"MCAPI error message = '%s'",
			aoutput->record->name,
			error_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_PMC_MCAPI && defined(OS_WIN32) */
