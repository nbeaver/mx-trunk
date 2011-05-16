/*
 * Name:    d_bit.c
 *
 * Purpose: MX input and output drivers to control bit ranges in digital
 *          input and output records.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001, 2010-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "d_bit.h"

MX_RECORD_FUNCTION_LIST mxd_bit_in_record_function_list = {
	NULL,
	mxd_bit_in_create_record_structures,
	mxd_bit_in_finish_record_initialization
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_bit_in_digital_input_function_list = {
	mxd_bit_in_read
};

MX_RECORD_FIELD_DEFAULTS mxd_bit_in_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_BIT_IN_STANDARD_FIELDS
};

long mxd_bit_in_num_record_fields
		= sizeof( mxd_bit_in_record_field_defaults )
			/ sizeof( mxd_bit_in_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bit_in_rfield_def_ptr
			= &mxd_bit_in_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_bit_out_record_function_list = {
	NULL,
	mxd_bit_out_create_record_structures,
	mxd_bit_out_finish_record_initialization,
	NULL,
	NULL,
	mxd_bit_out_open
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_bit_out_digital_output_function_list = {
	mxd_bit_out_read,
	mxd_bit_out_write
};

MX_RECORD_FIELD_DEFAULTS mxd_bit_out_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_BIT_OUT_STANDARD_FIELDS
};

long mxd_bit_out_num_record_fields
		= sizeof( mxd_bit_out_record_field_defaults )
			/ sizeof( mxd_bit_out_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bit_out_rfield_def_ptr
			= &mxd_bit_out_record_field_defaults[0];

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_bit_in_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_bit_in_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_BIT_IN *bit_in;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        bit_in = (MX_BIT_IN *) malloc( sizeof(MX_BIT_IN) );

        if ( bit_in == (MX_BIT_IN *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_BIT_IN structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = bit_in;
        record->class_specific_function_list
                                = &mxd_bit_in_digital_input_function_list;

        digital_input->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bit_in_finish_record_initialization( MX_RECORD *record )
{
        static const char fname[] = "mxd_bit_in_finish_record_initialization()";

        MX_BIT_IN *bit_in;
	int i;

        bit_in = (MX_BIT_IN *) record->record_type_struct;

        if ( bit_in == (MX_BIT_IN *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                	"MX_BIT_IN pointer for record '%s' is NULL.",
			record->name);
        }

        /* Verify that 'input_record' is the correct type of record. */

	if ( bit_in->input_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The input_record pointer for record '%s' is NULL.",
			record->name );
	}
        if ( bit_in->input_record->mx_superclass != MXR_DEVICE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
	                "'%s' is not a device record.",
			bit_in->input_record->name );
        }
        if ( ( bit_in->input_record->mx_class != MXC_DIGITAL_INPUT )
	  && ( bit_in->input_record->mx_class != MXC_DIGITAL_OUTPUT ) )
	{
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not a digital input or output record.",
			bit_in->input_record->name );
        }

	/* Compute the input mask. */

	bit_in->input_mask = 0L;

	for ( i = 0; i < bit_in->num_input_bits; i++ ) {

		bit_in->input_mask <<= 1;

		bit_in->input_mask |= 1L;
	}

	bit_in->input_mask <<= bit_in->input_bit_offset;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bit_in_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_bit_in_read()";

	MX_BIT_IN *bit_in;
	unsigned long input_record_value;
	mx_status_type mx_status;

	bit_in = (MX_BIT_IN *) dinput->record->record_type_struct;

	if ( bit_in == (MX_BIT_IN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BIT_IN pointer for record '%s' is NULL.",
			dinput->record->name );
	}

	mx_status = mx_digital_input_read( bit_in->input_record,
						&input_record_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bit_in->invert ) {
		input_record_value = ~input_record_value;
	}

	dinput->value = input_record_value & bit_in->input_mask;

	dinput->value >>= bit_in->input_bit_offset;

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_bit_out_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_bit_out_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_BIT_OUT *bit_out;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        bit_out = (MX_BIT_OUT *) malloc( sizeof(MX_BIT_OUT) );

        if ( bit_out == (MX_BIT_OUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_BIT_OUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = bit_out;
        record->class_specific_function_list
                                = &mxd_bit_out_digital_output_function_list;

        digital_output->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bit_out_finish_record_initialization( MX_RECORD *record )
{
        static const char fname[] = "mxd_bit_out_finish_record_initialization()";

        MX_BIT_OUT *bit_out;
	int i;

        bit_out = (MX_BIT_OUT *) record->record_type_struct;

        if ( bit_out == (MX_BIT_OUT *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                	"MX_BIT_OUT pointer for record '%s' is NULL.",
			record->name);
        }

        /* Verify that 'input_record' is the correct type of record. */

	if ( bit_out->input_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The input_record pointer for record '%s' is NULL.",
			record->name );
	}
        if ( bit_out->input_record->mx_superclass != MXR_DEVICE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
	                "'%s' is not a device record.",
			bit_out->input_record->name );
        }
        if ( ( bit_out->input_record->mx_class != MXC_DIGITAL_INPUT )
	  && ( bit_out->input_record->mx_class != MXC_DIGITAL_OUTPUT ) )
	{
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not a digital input or output record.",
			bit_out->input_record->name );
        }

        /* Verify that 'output_record' is the correct type of record. */

	if ( bit_out->output_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The output_record pointer for record '%s' is NULL.",
			record->name );
	}
        if ( bit_out->output_record->mx_superclass != MXR_DEVICE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
	                "'%s' is not a device record.",
			bit_out->output_record->name );
        }
        if ( bit_out->output_record->mx_class != MXC_DIGITAL_OUTPUT ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not a digital output record.",
			bit_out->output_record->name );
        }

	/* Compute the input mask. */

	bit_out->input_mask = 0L;

	for ( i = 0; i < bit_out->num_input_bits; i++ ) {

		bit_out->input_mask <<= 1;

		bit_out->input_mask |= 1L;
	}

	bit_out->input_mask <<= bit_out->input_bit_offset;

	/* Compute the output mask. */

	bit_out->output_mask = 0L;

	for ( i = 0; i < bit_out->num_output_bits; i++ ) {

		bit_out->output_mask <<= 1;

		bit_out->output_mask |= 1L;
	}

	bit_out->output_mask <<= bit_out->output_bit_offset;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bit_out_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_bit_out_open()";

	MX_DIGITAL_OUTPUT *doutput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	doutput = (MX_DIGITAL_OUTPUT *) (record->record_class_struct);

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_DIGITAL_OUTPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_bit_out_read( doutput );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bit_out_read( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_bit_out_read()";

	MX_BIT_OUT *bit_out;
	unsigned long input_record_value;
	mx_status_type mx_status;

	bit_out = (MX_BIT_OUT *) doutput->record->record_type_struct;

	if ( bit_out == (MX_BIT_OUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BIT_OUT pointer for record '%s' is NULL.",
			doutput->record->name );
	}

	mx_status = mx_digital_input_read( bit_out->input_record,
						&input_record_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bit_out->invert ) {
		input_record_value = ~input_record_value;
	}

	doutput->value = input_record_value & bit_out->input_mask;

	doutput->value >>= bit_out->input_bit_offset;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bit_out_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_bit_out_write()";

	MX_BIT_OUT *bit_out;
	unsigned long old_value, new_value, requested_value, shifted_value;
	mx_status_type mx_status;

	bit_out = (MX_BIT_OUT *) doutput->record->record_type_struct;

	if ( bit_out == (MX_BIT_OUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BIT_OUT pointer for record '%s' is NULL.",
			doutput->record->name );
	}

	requested_value = doutput->value;

	if ( bit_out->invert ) {
		requested_value = ~requested_value;
	}

	mx_status = mx_digital_output_read( bit_out->output_record,
						&old_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Shift the requested range of bits to the right position. */

	shifted_value = requested_value << bit_out->output_bit_offset;

	shifted_value &= bit_out->output_mask;

	/* Compute the new value for the output record. */

	new_value = old_value & ( ~bit_out->output_mask );

	new_value |= shifted_value;

	mx_status = mx_digital_output_write( bit_out->output_record,
						new_value );
	return mx_status;
}

