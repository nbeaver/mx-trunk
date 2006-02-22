/*
 * Name:    d_wago750_modbus_dout.c
 *
 * Purpose: This is a slightly modified version of the 'modbus_doutput'
 *          driver intended for use with Wago 750 series digital output
 *          modules via MODBUS.  The difference is that Wago 750 series
 *          digital outputs allow programs to get the current output 
 *          value by reading from a MODBUS address that is 0x200 higher
 *          than the address of the output.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MODBUS_DIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_stdint.h"
#include "mx_modbus.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "d_wago750_modbus_dout.h"

MX_RECORD_FUNCTION_LIST mxd_wago750_modbus_dout_record_function_list = {
	NULL,
	mxd_wago750_modbus_dout_create_record_structures
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
	mxd_wago750_modbus_dout_digital_output_function_list
  = {
	mxd_wago750_modbus_dout_read,
	mxd_modbus_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_wago750_modbus_dout_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_MODBUS_DOUTPUT_STANDARD_FIELDS
};

long mxd_wago750_modbus_dout_num_record_fields
	= sizeof( mxd_wago750_modbus_dout_rf_defaults )
		/ sizeof( mxd_wago750_modbus_dout_rf_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_wago750_modbus_dout_rfield_def_ptr
			= &mxd_wago750_modbus_dout_rf_defaults[0];

static mx_status_type
mxd_wago750_modbus_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_MODBUS_DOUTPUT **modbus_doutput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_wago750_modbus_dout_get_pointers()";

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( modbus_doutput == (MX_MODBUS_DOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MODBUS_DOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*modbus_doutput = (MX_MODBUS_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( *modbus_doutput == (MX_MODBUS_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MODBUS_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_wago750_modbus_dout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_wago750_modbus_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_MODBUS_DOUTPUT *modbus_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *) malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        modbus_doutput = (MX_MODBUS_DOUTPUT *)
				malloc( sizeof(MX_MODBUS_DOUTPUT) );

        if ( modbus_doutput == (MX_MODBUS_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_MODBUS_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = modbus_doutput;
        record->class_specific_function_list
			= &mxd_wago750_modbus_dout_digital_output_function_list;

        digital_output->record = record;
	modbus_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

/* ===== Read function. ===== */

MX_EXPORT mx_status_type
mxd_wago750_modbus_dout_read( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_wago750_modbus_dout_read()";

	MX_MODBUS_DOUTPUT *modbus_doutput;
	long raw_value;
	int function_code, num_bits, num_registers;
	uint8_t mx_uint8_array[4];
	uint16_t mx_uint16_array[4];
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warning. */

	modbus_doutput = NULL;

	mx_status = mxd_wago750_modbus_dout_get_pointers( doutput,
						&modbus_doutput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_bits = modbus_doutput->num_bits;

	if ( num_bits > 32 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Reading more than 32 bits at a time is not supported "
		"by record '%s'.", doutput->record->name );
	}

	function_code = (int) modbus_doutput->modbus_function_code;

	switch( function_code ) {
	case MXF_MOD_WRITE_SINGLE_COIL:
	case MXF_MOD_WRITE_MULTIPLE_COILS:
		mx_status = mx_modbus_read_coils(
					modbus_doutput->modbus_record,
					modbus_doutput->modbus_address + 0x200,
					num_bits,
					mx_uint8_array );
		break;
	case MXF_MOD_WRITE_SINGLE_REGISTER:
	case MXF_MOD_WRITE_MULTIPLE_REGISTERS:
		if ( ( num_bits % 16 ) == 0 ) {
			num_registers = num_bits / 16;
		} else {
			num_registers = 1 + num_bits / 16;
		}
		mx_status = mx_modbus_read_holding_registers(
					modbus_doutput->modbus_record,
					modbus_doutput->modbus_address + 0x200,
					num_registers,
					mx_uint16_array );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported MODBUS write function code %#x for record '%s'.  "
		"The supported function codes are %#x, %#x, %#x, and %#x.",
			function_code, doutput->record->name,
			MXF_MOD_WRITE_SINGLE_COIL,
			MXF_MOD_WRITE_MULTIPLE_COILS,
			MXF_MOD_WRITE_SINGLE_REGISTER,
			MXF_MOD_WRITE_MULTIPLE_REGISTERS );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	raw_value = 0;

	switch( function_code ) {
	case MXF_MOD_WRITE_SINGLE_COIL:
	case MXF_MOD_WRITE_MULTIPLE_COILS:
		if ( num_bits <= 8 ) {
			raw_value = (long) mx_uint8_array[0];
		} else
		if ( num_bits <= 16 ) {
			raw_value = ( (long) mx_uint8_array[0] ) << 8;
			raw_value |= (long) mx_uint8_array[1];
		} else
		if ( num_bits <= 24 ) {
			raw_value = ( (long) mx_uint8_array[0] ) << 16;
			raw_value |= ( (long) mx_uint8_array[1] ) << 8;
			raw_value |= (long) mx_uint8_array[2];
		} else {
			raw_value = ( (long) mx_uint8_array[0] ) << 24;
			raw_value |= ( (long) mx_uint8_array[1] ) << 16;
			raw_value |= ( (long) mx_uint8_array[2] ) << 8;
			raw_value |= (long) mx_uint8_array[3];
		}
		break;
	case MXF_MOD_WRITE_SINGLE_REGISTER:
	case MXF_MOD_WRITE_MULTIPLE_REGISTERS:
		if ( num_bits <= 16 ) {
			raw_value = (long) mx_uint16_array[0];
		} else {
			raw_value = ( (long) mx_uint16_array[0] ) << 16;
			raw_value |= (long) mx_uint16_array[1];
		}
		break;
	}

	doutput->value = raw_value;

	return MX_SUCCESSFUL_RESULT;
}
