/*
 * Name:    d_vme_dio.c
 *
 * Purpose: MX input and output drivers to control VME address as if they
 *          were digital input/output registers (which they often are).
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001, 2006, 2008, 2010, 2012 Illinois Institute of Technology
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
#include "mx_stdint.h"
#include "mx_vme.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "d_vme_dio.h"

/* Initialize the VME digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_vme_din_record_function_list = {
	NULL,
	mxd_vme_din_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_vme_din_open
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_vme_din_digital_input_function_list = {
	mxd_vme_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_vme_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_VME_DINPUT_STANDARD_FIELDS
};

long mxd_vme_din_num_record_fields
		= sizeof( mxd_vme_din_record_field_defaults )
			/ sizeof( mxd_vme_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_vme_din_rfield_def_ptr
			= &mxd_vme_din_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_vme_dout_record_function_list = {
	NULL,
	mxd_vme_dout_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_vme_dout_open
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_vme_dout_digital_output_function_list = {
	NULL,
	mxd_vme_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_vme_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_VME_DOUTPUT_STANDARD_FIELDS
};

long mxd_vme_dout_num_record_fields
		= sizeof( mxd_vme_dout_record_field_defaults )
			/ sizeof( mxd_vme_dout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_vme_dout_rfield_def_ptr
			= &mxd_vme_dout_record_field_defaults[0];

static mx_status_type
mxd_vme_din_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_VME_DINPUT **vme_dinput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_vme_din_get_pointers()";

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (vme_dinput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VME_DINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dinput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*vme_dinput = (MX_VME_DINPUT *) dinput->record->record_type_struct;

	if ( *vme_dinput == (MX_VME_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_VME_DINPUT pointer for record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_vme_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_VME_DOUTPUT **vme_doutput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_vme_dout_get_pointers()";

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (vme_doutput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VME_DOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*vme_doutput = (MX_VME_DOUTPUT *) doutput->record->record_type_struct;

	if ( *vme_doutput == (MX_VME_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_VME_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_vme_din_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_vme_din_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_VME_DINPUT *vme_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        vme_dinput = (MX_VME_DINPUT *) malloc( sizeof(MX_VME_DINPUT) );

        if ( vme_dinput == (MX_VME_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_VME_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = vme_dinput;
        record->class_specific_function_list
                                = &mxd_vme_din_digital_input_function_list;

        digital_input->record = record;
	vme_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vme_din_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_vme_din_open()";

	MX_DIGITAL_INPUT *dinput;
	MX_VME_DINPUT *vme_dinput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	dinput = (MX_DIGITAL_INPUT *) record->record_class_struct;

	mx_status = mxd_vme_din_get_pointers( dinput, &vme_dinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Parse the address mode string. */

	mx_status = mx_vme_parse_address_mode( vme_dinput->address_mode_name,
						&(vme_dinput->address_mode) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Parse the data size string. */

	mx_status = mx_vme_parse_data_size( vme_dinput->data_size_name,
						&(vme_dinput->data_size) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the current value of the register. */

	mx_status = mxd_vme_din_read( dinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_vme_din_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_vme_din_read()";

	MX_VME_DINPUT *vme_dinput = NULL;
	uint8_t d8_value;
	uint16_t d16_value;
	uint32_t d32_value;
	mx_status_type mx_status;

	mx_status = mxd_vme_din_get_pointers( dinput, &vme_dinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( vme_dinput->data_size ) {
	case MXF_VME_D8:
		mx_status = mx_vme_in8( vme_dinput->vme_record,
				vme_dinput->crate_number,
				vme_dinput->address_mode,
				vme_dinput->address,
				&d8_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dinput->value = (long) ( d8_value & 0xff );
		break;
	case MXF_VME_D16:
		mx_status = mx_vme_in16( vme_dinput->vme_record,
				vme_dinput->crate_number,
				vme_dinput->address_mode,
				vme_dinput->address,
				&d16_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dinput->value = (long) ( d16_value & 0xffff );
		break;
	case MXF_VME_D32:
		mx_status = mx_vme_in32( vme_dinput->vme_record,
				vme_dinput->crate_number,
				vme_dinput->address_mode,
				vme_dinput->address,
				&d32_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dinput->value = (long) ( d32_value & 0xffffffff );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal data size %lu for VME digital input record '%s'.  "
		"The allowed values are 8, 16, and 32.",
			vme_dinput->data_size, dinput->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_vme_dout_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_vme_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_VME_DOUTPUT *vme_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        vme_doutput = (MX_VME_DOUTPUT *) malloc( sizeof(MX_VME_DOUTPUT) );

        if ( vme_doutput == (MX_VME_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_VME_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = vme_doutput;
        record->class_specific_function_list
                                = &mxd_vme_dout_digital_output_function_list;

        digital_output->record = record;
	vme_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vme_dout_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_vme_dout_open()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_VME_DOUTPUT *vme_doutput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	doutput = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	mx_status = mxd_vme_dout_get_pointers( doutput, &vme_doutput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Parse the address mode string. */

	mx_status = mx_vme_parse_address_mode( vme_doutput->address_mode_name,
						&(vme_doutput->address_mode) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Parse the data size string. */

	mx_status = mx_vme_parse_data_size( vme_doutput->data_size_name,
						&(vme_doutput->data_size) );

	return mx_status;
}

/* We do not provide an mxd_vme_dout_read() function here, since
 * for some devices, it is not safe or appropriate to read from
 * an output address.  Thus, we merely return here the last value
 * written to the address.
 *
 * If you actually need to be able to read from this address,
 * create a vme_dinput record that points to the same address.
 */

MX_EXPORT mx_status_type
mxd_vme_dout_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_vme_dout_write()";

	MX_VME_DOUTPUT *vme_doutput = NULL;
	uint8_t d8_value;
	uint16_t d16_value;
	uint32_t d32_value;
	mx_status_type mx_status;

	mx_status = mxd_vme_dout_get_pointers( doutput, &vme_doutput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( vme_doutput->data_size ) {
	case MXF_VME_D8:
		d8_value = (uint8_t) ( doutput->value & 0xff );

		mx_status = mx_vme_out8( vme_doutput->vme_record,
				vme_doutput->crate_number,
				vme_doutput->address_mode,
				vme_doutput->address,
				d8_value );
		break;
	case MXF_VME_D16:
		d16_value = (uint16_t) ( doutput->value & 0xffff );

		mx_status = mx_vme_out16( vme_doutput->vme_record,
				vme_doutput->crate_number,
				vme_doutput->address_mode,
				vme_doutput->address,
				d16_value );
		break;
	case MXF_VME_D32:
		d32_value = (uint32_t) ( doutput->value & 0xffffffff );

		mx_status = mx_vme_out32( vme_doutput->vme_record,
				vme_doutput->crate_number,
				vme_doutput->address_mode,
				vme_doutput->address,
				d32_value );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal data size %lu for VME digital output record '%s'.  "
		"The allowed values are 8, 16, and 32.",
			vme_doutput->data_size, doutput->record->name );
	}
	return mx_status;
}

