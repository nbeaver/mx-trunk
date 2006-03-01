/*
 * Name:    d_modbus_aio.c
 *
 * Purpose: MX analog I/O drivers to read MODBUS analog input and output
 *          coils and registers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2006 Illinois Institute of Technology
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
#include "mx_stdint.h"
#include "mx_driver.h"
#include "mx_modbus.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "d_modbus_aio.h"

/* Initialize the MODBUS analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_modbus_ain_record_function_list = {
	NULL,
	mxd_modbus_ain_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_modbus_ain_analog_input_function_list = {
	mxd_modbus_ain_read
};

MX_RECORD_FIELD_DEFAULTS mxd_modbus_ain_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_LONG_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_MODBUS_AINPUT_STANDARD_FIELDS
};

long mxd_modbus_ain_num_record_fields
		= sizeof( mxd_modbus_ain_record_field_defaults )
			/ sizeof( mxd_modbus_ain_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_modbus_ain_rfield_def_ptr
			= &mxd_modbus_ain_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_modbus_aout_record_function_list = {
	NULL,
	mxd_modbus_aout_create_record_structures
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_modbus_aout_analog_output_function_list
  = {
	mxd_modbus_aout_read,
	mxd_modbus_aout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_modbus_aout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_LONG_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_MODBUS_AOUTPUT_STANDARD_FIELDS
};

long mxd_modbus_aout_num_record_fields
		= sizeof( mxd_modbus_aout_record_field_defaults )
			/ sizeof( mxd_modbus_aout_record_field_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_modbus_aout_rfield_def_ptr
			= &mxd_modbus_aout_record_field_defaults[0];

static mx_status_type
mxd_modbus_ain_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_MODBUS_AINPUT **modbus_ainput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_modbus_ain_get_pointers()";

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( modbus_ainput == (MX_MODBUS_AINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MODBUS_AINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( ainput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*modbus_ainput = (MX_MODBUS_AINPUT *)
				ainput->record->record_type_struct;

	if ( *modbus_ainput == (MX_MODBUS_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MODBUS_AINPUT pointer for record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_modbus_aout_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_MODBUS_AOUTPUT **modbus_aoutput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_modbus_aout_get_pointers()";

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( modbus_aoutput == (MX_MODBUS_AOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MODBUS_AOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( aoutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*modbus_aoutput = (MX_MODBUS_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( *modbus_aoutput == (MX_MODBUS_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MODBUS_AOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_modbus_ain_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_modbus_ain_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_MODBUS_AINPUT *modbus_ainput;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        modbus_ainput = (MX_MODBUS_AINPUT *)
				malloc( sizeof(MX_MODBUS_AINPUT) );

        if ( modbus_ainput == (MX_MODBUS_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_MODBUS_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = modbus_ainput;
        record->class_specific_function_list
			= &mxd_modbus_ain_analog_input_function_list;

        analog_input->record = record;
	modbus_ainput->record = record;

	/* Raw analog input values are stored as longs. */

	analog_input->subclass = MXT_AIN_LONG;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_modbus_ain_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_modbus_ain_read()";

	MX_MODBUS_AINPUT *modbus_ainput;
	long raw_value;
	int function_code, num_bits, num_registers;
	uint8_t mx_uint8_array[4];
	uint16_t mx_uint16_array[2];
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warning. */

	modbus_ainput = NULL;

	mx_status = mxd_modbus_ain_get_pointers( ainput,
						&modbus_ainput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_bits = (int) modbus_ainput->num_bits;

	if ( num_bits > 32 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Reading more than 32 bits at a time is not supported "
		"by record '%s'.", ainput->record->name );
	}

	function_code = (int) modbus_ainput->modbus_function_code;

	switch( function_code ) {
	case MXF_MOD_READ_COILS:
		mx_status = mx_modbus_read_coils(
					modbus_ainput->modbus_record,
					modbus_ainput->modbus_address,
					num_bits,
					mx_uint8_array );
		break;
	case MXF_MOD_READ_DISCRETE_INPUTS:
		mx_status = mx_modbus_read_discrete_inputs(
					modbus_ainput->modbus_record,
					modbus_ainput->modbus_address,
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
					modbus_ainput->modbus_record,
					modbus_ainput->modbus_address,
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
					modbus_ainput->modbus_record,
					modbus_ainput->modbus_address,
					num_registers,
					mx_uint16_array );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported MODBUS read function code %#x for record '%s'.  "
		"The supported function codes are %#x, %#x, %#x, and %#x.",
			function_code, ainput->record->name,
			MXF_MOD_READ_COILS,
			MXF_MOD_READ_DISCRETE_INPUTS,
			MXF_MOD_READ_HOLDING_REGISTERS,
			MXF_MOD_READ_INPUT_REGISTERS );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	raw_value = 0;

	switch( function_code ) {
	case MXF_MOD_READ_COILS:
	case MXF_MOD_READ_DISCRETE_INPUTS:
#if 0
		{
			int i;

			for ( i = 0; i < 4; i++ ) {
				MX_DEBUG(-2,("%s: mx_uint8_array[%d] = %lx",
				fname, i,
				(unsigned long)(0xff & mx_uint8_array[i]) ));
			}
		}
#endif
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
#if 0
		{
			int i;

			for ( i = 0; i < 2; i++ ) {
				MX_DEBUG(-2,("%s: mx_uint16_array[%d] = %lx",
				fname, i,
				(unsigned long)(0xffff & mx_uint16_array[i]) ));
			}
		}
#endif
		if ( num_bits <= 16 ) {
			raw_value = (long) mx_uint16_array[0];
		} else {
			raw_value = ( (long) mx_uint16_array[0] ) << 16;
			raw_value |= (long) mx_uint16_array[1];
		}
		break;
	}

	ainput->raw_value.long_value = raw_value;

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_modbus_aout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_modbus_aout_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_MODBUS_AOUTPUT *modbus_aoutput;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *) malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        modbus_aoutput = (MX_MODBUS_AOUTPUT *)
				malloc( sizeof(MX_MODBUS_AOUTPUT) );

        if ( modbus_aoutput == (MX_MODBUS_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_MODBUS_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = modbus_aoutput;
        record->class_specific_function_list
			= &mxd_modbus_aout_analog_output_function_list;

        analog_output->record = record;
	modbus_aoutput->record = record;

	/* Raw analog output values are stored as longs. */

	analog_output->subclass = MXT_AOU_LONG;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_modbus_aout_read( MX_ANALOG_OUTPUT *aoutput )
{
	/* Just return the last value written. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_modbus_aout_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_modbus_aout_write()";

	MX_MODBUS_AOUTPUT *modbus_aoutput;
	long raw_value;
	int function_code, num_bits, num_registers;
	uint8_t mx_uint8_array[4];
	uint16_t mx_uint16_array[2];
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warning. */

	modbus_aoutput = NULL;

	mx_status = mxd_modbus_aout_get_pointers( aoutput,
						&modbus_aoutput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_bits = (int) modbus_aoutput->num_bits;

	if ( num_bits > 32 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Reading more than 32 bits at a time is not supported "
		"by record '%s'.", aoutput->record->name );
	}

	function_code = (int) modbus_aoutput->modbus_function_code;

	raw_value = aoutput->raw_value.long_value;

	MX_DEBUG(-2,("%s: function_code = %d, raw_value = %#lx (%ld)",
		fname, function_code, (long) raw_value, (long) raw_value));

	switch( function_code ) {
	case MXF_MOD_WRITE_SINGLE_COIL:
	case MXF_MOD_WRITE_MULTIPLE_COILS:
		if ( num_bits <= 8 ) {
			mx_uint8_array[0] = (uint8_t) raw_value & 0xff;
		} else
		if ( num_bits <= 16 ) {
			mx_uint8_array[0] = (uint8_t)
						(( raw_value >> 8 ) & 0xff);
			mx_uint8_array[1] = (uint8_t)
						(raw_value & 0xff);
		} else
		if ( num_bits <= 24 ) {
			mx_uint8_array[0] = (uint8_t)
						(( raw_value >> 16 ) & 0xff);
			mx_uint8_array[1] = (uint8_t)
						(( raw_value >> 8 ) & 0xff);
			mx_uint8_array[2] = (uint8_t)
						(raw_value & 0xff);
		} else {
			mx_uint8_array[0] = (uint8_t)
						(( raw_value >> 24 ) & 0xff);
			mx_uint8_array[1] = (uint8_t)
						(( raw_value >> 16 ) & 0xff);
			mx_uint8_array[2] = (uint8_t)
						(( raw_value >> 8 ) & 0xff);
			mx_uint8_array[3] = (uint8_t)
						(raw_value & 0xff);
		}
		break;
	case MXF_MOD_WRITE_SINGLE_REGISTER:
	case MXF_MOD_WRITE_MULTIPLE_REGISTERS:
		if ( num_bits <= 16 ) {
			mx_uint16_array[0] = (uint16_t)
						(raw_value & 0xffff);
		} else {
			mx_uint16_array[0] = (uint16_t)
						(( raw_value >> 16 ) & 0xffff);
			mx_uint16_array[1] = (uint16_t)
						(raw_value & 0xffff);
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported MODBUS write function code %#x for record '%s'.  "
		"The supported function codes are %#x, %#x, %#x, and %#x.",
			function_code, aoutput->record->name,
			MXF_MOD_WRITE_SINGLE_COIL,
			MXF_MOD_WRITE_MULTIPLE_COILS,
			MXF_MOD_WRITE_SINGLE_REGISTER,
			MXF_MOD_WRITE_MULTIPLE_REGISTERS );
	}

	switch( function_code ) {
	case MXF_MOD_WRITE_SINGLE_COIL:
		if ( num_bits != 1 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Record '%s' attempted to use the MODBUS 'write_single_coil' "
		"function code %#x for num_bits == %d.  This function code is "
		"only supported if num_bits == 1.",
				aoutput->record->name,
				MXF_MOD_WRITE_SINGLE_COIL,
				num_bits );
		}

		mx_status = mx_modbus_write_single_coil(
					modbus_aoutput->modbus_record,
					modbus_aoutput->modbus_address,
					mx_uint8_array[0] );
		break;
	case MXF_MOD_WRITE_MULTIPLE_COILS:
		mx_status = mx_modbus_write_multiple_coils(
					modbus_aoutput->modbus_record,
					modbus_aoutput->modbus_address,
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
				aoutput->record->name,
				MXF_MOD_WRITE_SINGLE_REGISTER,
				num_bits );
		}

		mx_status = mx_modbus_write_single_register(
					modbus_aoutput->modbus_record,
					modbus_aoutput->modbus_address,
					mx_uint16_array[0] );
		break;
	case MXF_MOD_WRITE_MULTIPLE_REGISTERS:
		if ( ( num_bits % 16 ) == 0 ) {
			num_registers = num_bits / 16;
		} else {
			num_registers = 1 + num_bits / 16;
		}
		mx_status = mx_modbus_write_multiple_registers(
					modbus_aoutput->modbus_record,
					modbus_aoutput->modbus_address,
					num_registers,
					mx_uint16_array );
		break;
	}

	return mx_status;
}

