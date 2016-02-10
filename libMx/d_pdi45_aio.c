/*
 * Name:    d_pdi45_aio.c
 *
 * Purpose: MX input and output drivers to control Prarie Digital Model 45
 *          analog I/O lines.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2006, 2010, 2012, 2015-2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "i_pdi45.h"
#include "d_pdi45_aio.h"

/* Initialize the PDI45 analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_pdi45_ain_record_function_list = {
	NULL,
	mxd_pdi45_ain_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_pdi45_ain_analog_input_function_list = {
	mxd_pdi45_ain_read
};

MX_RECORD_FIELD_DEFAULTS mxd_pdi45_ain_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_PDI45_AINPUT_STANDARD_FIELDS
};

long mxd_pdi45_ain_num_record_fields
		= sizeof( mxd_pdi45_ain_record_field_defaults )
			/ sizeof( mxd_pdi45_ain_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pdi45_ain_rfield_def_ptr
			= &mxd_pdi45_ain_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_pdi45_aout_record_function_list = {
	NULL,
	mxd_pdi45_aout_create_record_structures
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_pdi45_aout_analog_output_function_list = {
	mxd_pdi45_aout_read,
	mxd_pdi45_aout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_pdi45_aout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_PDI45_AOUTPUT_STANDARD_FIELDS
};

long mxd_pdi45_aout_num_record_fields
		= sizeof( mxd_pdi45_aout_record_field_defaults )
			/ sizeof( mxd_pdi45_aout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pdi45_aout_rfield_def_ptr
			= &mxd_pdi45_aout_record_field_defaults[0];

static mx_status_type
mxd_pdi45_ain_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_PDI45_AINPUT **pdi45_ainput,
			MX_PDI45 **pdi45,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pdi45_ain_get_pointers()";

	MX_RECORD *pdi45_record;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pdi45_ainput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PDI45_AINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pdi45 == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PDI45 pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( ainput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pdi45_ainput = (MX_PDI45_AINPUT *)
				ainput->record->record_type_struct;

	if ( *pdi45_ainput == (MX_PDI45_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PDI45_AINPUT pointer for record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	pdi45_record = (*pdi45_ainput)->pdi45_record;

	if ( pdi45_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_PDI45 pointer for PDI45 analog input record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	if ( pdi45_record->mx_type != MXI_CTRL_PDI45 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"pdi45_record '%s' for PDI45 analog input '%s' is not a PDI45 record.  "
"Instead, it is a '%s' record.",
			pdi45_record->name, ainput->record->name,
			mx_get_driver_name( pdi45_record ) );
	}

	*pdi45 = (MX_PDI45 *) pdi45_record->record_type_struct;

	if ( *pdi45 == (MX_PDI45 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_PDI45 pointer for PDI45 record '%s' used by PDI45 analog input "
	"record '%s' and passed by '%s' is NULL.",
			pdi45_record->name,
			ainput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pdi45_aout_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_PDI45_AOUTPUT **pdi45_aoutput,
			MX_PDI45 **pdi45,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pdi45_aout_get_pointers()";

	MX_RECORD *pdi45_record;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pdi45_aoutput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PDI45_AOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pdi45 == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PDI45 pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( aoutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pdi45_aoutput = (MX_PDI45_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( *pdi45_aoutput == (MX_PDI45_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PDI45_AOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	pdi45_record = (*pdi45_aoutput)->pdi45_record;

	if ( pdi45_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_PDI45 pointer for PDI45 analog output record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	if ( pdi45_record->mx_type != MXI_CTRL_PDI45 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"pdi45_record '%s' for PDI45 analog output '%s' is not a PDI45 record.  "
"Instead, it is a '%s' record.",
			pdi45_record->name, aoutput->record->name,
			mx_get_driver_name( pdi45_record ) );
	}

	*pdi45 = (MX_PDI45 *) pdi45_record->record_type_struct;

	if ( *pdi45 == (MX_PDI45 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_PDI45 pointer for PDI45 record '%s' used by "
	"PDI45 analog output record '%s' and passed by '%s' is NULL.",
			pdi45_record->name,
			aoutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_pdi45_ain_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_pdi45_ain_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_PDI45_AINPUT *pdi45_ainput;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        pdi45_ainput = (MX_PDI45_AINPUT *) malloc( sizeof(MX_PDI45_AINPUT) );

        if ( pdi45_ainput == (MX_PDI45_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_PDI45_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = pdi45_ainput;
        record->class_specific_function_list
                                = &mxd_pdi45_ain_analog_input_function_list;

        analog_input->record = record;
	pdi45_ainput->record = record;

	/* Raw analog input values are stored as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi45_ain_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_pdi45_ain_read()";

	MX_PDI45_AINPUT *pdi45_ainput;
	MX_PDI45 *pdi45;
	char command[80];
	char response[80];
	int num_items;
	long hex_value;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	pdi45 = NULL;
	pdi45_ainput = NULL;

	mx_status = mxd_pdi45_ain_get_pointers( ainput,
					&pdi45_ainput, &pdi45, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
			"FFL%02X", 1 << (pdi45_ainput->line_number) );


	mx_status = mxi_pdi45_command( pdi45, command,
				response, sizeof( response ) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response + 3, "%2lx", &hex_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unrecognizable response '%s' to command '%s' for "
		"PDI45 analog input '%s'.",
			response, command, ainput->record->name );
	}

	ainput->raw_value.double_value = ( 5.0 / 256.0 ) * (double) hex_value;

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_pdi45_aout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_pdi45_aout_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_PDI45_AOUTPUT *pdi45_aoutput;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *) malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        pdi45_aoutput = (MX_PDI45_AOUTPUT *) malloc( sizeof(MX_PDI45_AOUTPUT) );

        if ( pdi45_aoutput == (MX_PDI45_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_PDI45_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = pdi45_aoutput;
        record->class_specific_function_list
                                = &mxd_pdi45_aout_analog_output_function_list;

        analog_output->record = record;
	pdi45_aoutput->record = record;

	/* Raw analog output values are stored as doubles. */

	analog_output->subclass = MXT_AOU_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi45_aout_read( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_pdi45_aout_read()";

	MX_PDI45_AOUTPUT *pdi45_aoutput;
	MX_PDI45 *pdi45;
	char command[80];
	char response[80];
	int num_items;
	long hex_value;
	mx_status_type mx_status;

	mx_status = mxd_pdi45_aout_get_pointers( aoutput,
					&pdi45_aoutput, &pdi45, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
			"FFK%02X", 1 << (pdi45_aoutput->line_number) );


	mx_status = mxi_pdi45_command( pdi45, command,
				response, sizeof( response ) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response + 2, "%2lx", &hex_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unrecognizable response '%s' to command '%s' for "
		"PDI45 analog output '%s'.",
			response, command, aoutput->record->name );
	}

	aoutput->raw_value.double_value = ( 5.0 / 256.0 ) * (double) hex_value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi45_aout_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_pdi45_aout_write()";

	MX_PDI45_AOUTPUT *pdi45_aoutput;
	MX_PDI45 *pdi45;
	char command[80];
	long hex_value;
	mx_status_type mx_status;

	mx_status = mxd_pdi45_aout_get_pointers( aoutput,
					&pdi45_aoutput, &pdi45, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	hex_value = mx_round( aoutput->raw_value.double_value * 256.0 / 5.0 );

	if ( hex_value < 0 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"Requested raw voltage of %g volts for DAC '%s' is "
			"less than the minimum allowed value of 0 volts.",
			aoutput->raw_value.double_value,
			aoutput->record->name );
	}
	if ( hex_value > 0xff ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"Requested raw voltage of %g volts for DAC '%s' is "
			"greater than the maximum allowed value of %g volts.",
			aoutput->raw_value.double_value,
			aoutput->record->name,
			( 5.0 / 256.0 ) * (double) 0xff );
	}

	snprintf( command, sizeof(command),
		"FFJ000%01X%03lX",
		1 << ( pdi45_aoutput->line_number ), hex_value );

	mx_status = mxi_pdi45_command( pdi45, command, NULL, 0 );

	return mx_status;
}

