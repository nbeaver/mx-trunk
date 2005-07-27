/*
 * Name:    d_modbus_dio.c
 *
 * Purpose: MX digital I/O drivers to read MODBUS digital input and output
 *          coils and registers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MODBUS_AIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_types.h"
#include "mx_driver.h"
#include "mx_modbus.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "d_modbus_dio.h"

/* Initialize the MODBUS digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_modbus_din_record_function_list = {
	NULL,
	mxd_modbus_din_create_record_structures
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_modbus_din_digital_input_function_list = {
	mxd_modbus_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_modbus_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_MODBUS_DINPUT_STANDARD_FIELDS
};

long mxd_modbus_din_num_record_fields
		= sizeof( mxd_modbus_din_record_field_defaults )
			/ sizeof( mxd_modbus_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_modbus_din_rfield_def_ptr
			= &mxd_modbus_din_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_modbus_dout_record_function_list = {
	NULL,
	mxd_modbus_dout_create_record_structures
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_modbus_dout_digital_output_function_list
  = {
	mxd_modbus_dout_read,
	mxd_modbus_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_modbus_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_MODBUS_DOUTPUT_STANDARD_FIELDS
};

long mxd_modbus_dout_num_record_fields
		= sizeof( mxd_modbus_dout_record_field_defaults )
			/ sizeof( mxd_modbus_dout_record_field_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_modbus_dout_rfield_def_ptr
			= &mxd_modbus_dout_record_field_defaults[0];

static mx_status_type
mxd_modbus_din_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_MODBUS_DINPUT **modbus_dinput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_modbus_din_get_pointers()";

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( modbus_dinput == (MX_MODBUS_DINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MODBUS_DINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dinput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*modbus_dinput = (MX_MODBUS_DINPUT *)
				dinput->record->record_type_struct;

	if ( *modbus_dinput == (MX_MODBUS_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MODBUS_DINPUT pointer for record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_modbus_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_MODBUS_DOUTPUT **modbus_doutput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_modbus_dout_get_pointers()";

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

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_modbus_din_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_modbus_din_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_MODBUS_DINPUT *modbus_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        modbus_dinput = (MX_MODBUS_DINPUT *)
				malloc( sizeof(MX_MODBUS_DINPUT) );

        if ( modbus_dinput == (MX_MODBUS_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_MODBUS_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = modbus_dinput;
        record->class_specific_function_list
			= &mxd_modbus_din_digital_input_function_list;

        digital_input->record = record;
	modbus_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_modbus_din_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_modbus_din_read()";

	MX_MODBUS_DINPUT *modbus_dinput;
	long raw_value;
	int function_code, num_bits, num_registers;
	mx_uint8_type mx_uint8_array[4];
	mx_uint16_type mx_uint16_array[2];
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warning. */

	modbus_dinput = NULL;

	mx_status = mxd_modbus_din_get_pointers( dinput,
						&modbus_dinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_bits = modbus_dinput->num_bits;

	if ( num_bits > 32 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Reading more than 32 bits at a time is not supported "
		"by record '%s'.", dinput->record->name );
	}

	function_code = (int) modbus_dinput->modbus_function_code;

	switch( function_code ) {
	case MXF_MOD_READ_COILS:
		mx_status = mx_modbus_read_coils(
					modbus_dinput->modbus_record,
					modbus_dinput->modbus_address,
					num_bits,
					mx_uint8_array );
		break;
	case MXF_MOD_READ_DISCRETE_INPUTS:
		mx_status = mx_modbus_read_discrete_inputs(
					modbus_dinput->modbus_record,
					modbus_dinput->modbus_address,
					num_bits,
					mx_uint8_array );
		break;
	case MXF_MOD_READ_HOLDING_REGISTERS:
		if ( ( num_bits % 16 ) == 0 ) {
			num_registers = num_bits / 16;
		} else {
			num_registers = 1 + num_bits / 16;
		}
		mx_status = mx_modbus_read_holding_registers(
					modbus_dinput->modbus_record,
					modbus_dinput->modbus_address,
					num_registers,
					mx_uint16_array );
		break;
	case MXF_MOD_READ_INPUT_REGISTERS:
		if ( ( num_bits % 16 ) == 0 ) {
			num_registers = num_bits / 16;
		} else {
			num_registers = 1 + num_bits / 16;
		}
		mx_status = mx_modbus_read_input_registers(
					modbus_dinput->modbus_record,
					modbus_dinput->modbus_address,
					num_registers,
					mx_uint16_array );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported MODBUS read function code %#x for record '%s'.  "
		"The supported function codes are %#x, %#x, %#x, and %#x.",
			function_code, dinput->record->name,
			MXF_MOD_READ_COILS,
			MXF_MOD_READ_DISCRETE_INPUTS,
			MXF_MOD_READ_HOLDING_REGISTERS,
			MXF_MOD_READ_INPUT_REGISTERS );
		break;
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	raw_value = 0;

	switch( function_code ) {
	case MXF_MOD_READ_COILS:
	case MXF_MOD_READ_DISCRETE_INPUTS:
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
	case MXF_MOD_READ_HOLDING_REGISTERS:
	case MXF_MOD_READ_INPUT_REGISTERS:
		if ( num_bits <= 16 ) {
			raw_value = (long) mx_uint16_array[0];
		} else {
			raw_value = ( (long) mx_uint16_array[0] ) << 16;
			raw_value |= (long) mx_uint16_array[1];
		}
		break;
	}

	dinput->value = raw_value;

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_modbus_dout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_modbus_dout_create_record_structures()";

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
			= &mxd_modbus_dout_digital_output_function_list;

        digital_output->record = record;
	modbus_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_modbus_dout_read( MX_DIGITAL_OUTPUT *doutput )
{
	/* Just return the last value written. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_modbus_dout_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_modbus_dout_write()";

	MX_MODBUS_DOUTPUT *modbus_doutput;
	long raw_value;
	int function_code, num_bits, num_registers;
	mx_uint8_type mx_uint8_array[4];
	mx_uint16_type mx_uint16_array[4];
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warning. */

	modbus_doutput = NULL;

	mx_status = mxd_modbus_dout_get_pointers( doutput,
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

	raw_value = (long) doutput->value;

	switch( function_code ) {
	case MXF_MOD_WRITE_SINGLE_COIL:
	case MXF_MOD_WRITE_MULTIPLE_COILS:
		if ( num_bits <= 8 ) {
			mx_uint8_array[0] = (mx_uint8_type) (raw_value & 0xff);
		} else
		if ( num_bits <= 16 ) {
			mx_uint8_array[0] = (mx_uint8_type)
						(( raw_value >> 8 ) & 0xff);
			mx_uint8_array[1] = (mx_uint8_type)
						(raw_value & 0xff);
		} else
		if ( num_bits <= 24 ) {
			mx_uint8_array[0] = (mx_uint8_type)
						(( raw_value >> 16 ) & 0xff);
			mx_uint8_array[1] = (mx_uint8_type)
						(( raw_value >> 8 ) & 0xff);
			mx_uint8_array[2] = (mx_uint8_type)
						(raw_value & 0xff);
		} else {
			mx_uint8_array[0] = (mx_uint8_type)
						(( raw_value >> 24 ) & 0xff);
			mx_uint8_array[1] = (mx_uint8_type)
						(( raw_value >> 16 ) & 0xff);
			mx_uint8_array[2] = (mx_uint8_type)
						(( raw_value >> 8 ) & 0xff);
			mx_uint8_array[3] = (mx_uint8_type)
						(raw_value & 0xff);
		}
		break;
	case MXF_MOD_WRITE_SINGLE_REGISTER:
	case MXF_MOD_WRITE_MULTIPLE_REGISTERS:
		if ( num_bits <= 16 ) {
			mx_uint16_array[0] = (mx_uint16_type)
						(raw_value & 0xffff);
		} else {
			mx_uint16_array[0] = (mx_uint16_type)
						(( raw_value >> 16 ) & 0xffff);
			mx_uint16_array[1] = (mx_uint16_type)
						(raw_value & 0xffff);
		}
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
		break;
	}

	switch( function_code ) {
	case MXF_MOD_WRITE_SINGLE_COIL:
		if ( num_bits != 1 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Record '%s' attempted to use the MODBUS 'write_single_coil' "
		"function code %#x for num_bits == %d.  This function code is "
		"only supported if num_bits == 1.",
				doutput->record->name,
				MXF_MOD_WRITE_SINGLE_COIL,
				num_bits );
		}

		mx_status = mx_modbus_write_single_coil(
					modbus_doutput->modbus_record,
					modbus_doutput->modbus_address,
					mx_uint8_array[0] );
		break;
	case MXF_MOD_WRITE_MULTIPLE_COILS:
		mx_status = mx_modbus_write_multiple_coils(
					modbus_doutput->modbus_record,
					modbus_doutput->modbus_address,
					num_bits,
					mx_uint8_array );
		break;
	case MXF_MOD_WRITE_SINGLE_REGISTER:
		if ( num_bits > 16 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Record '%s' attempted to use the MODBUS "
		"'write_single_register' function code %#x for "
		"num_bits == %#x.  This function code is only supported "
		"for num_bits <= 16.",
				doutput->record->name,
				MXF_MOD_WRITE_SINGLE_REGISTER,
				num_bits );
		}

		mx_status = mx_modbus_write_single_register(
					modbus_doutput->modbus_record,
					modbus_doutput->modbus_address,
					mx_uint16_array[0] );
		break;
	case MXF_MOD_WRITE_MULTIPLE_REGISTERS:
		if ( ( num_bits % 16 ) == 0 ) {
			num_registers = num_bits / 16;
		} else {
			num_registers = 1 + num_bits / 16;
		}
		mx_status = mx_modbus_write_multiple_registers(
					modbus_doutput->modbus_record,
					modbus_doutput->modbus_address,
					num_registers,
					mx_uint16_array );
		break;
	}

	return mx_status;
}

