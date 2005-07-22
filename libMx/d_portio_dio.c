/*
 * Name:    d_portio_dio.c
 *
 * Purpose: MX input and output drivers to control I/O ports via digital
 *          input and output device records.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_types.h"
#include "mx_driver.h"
#include "mx_portio.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "d_portio_dio.h"

/* Initialize the PORTIO digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_portio_din_record_function_list = {
	NULL,
	mxd_portio_din_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_portio_din_open,
	mxd_portio_din_close
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_portio_din_digital_input_function_list = {
	mxd_portio_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_portio_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_PORTIO_DINPUT_STANDARD_FIELDS
};

long mxd_portio_din_num_record_fields
		= sizeof( mxd_portio_din_record_field_defaults )
			/ sizeof( mxd_portio_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_portio_din_rfield_def_ptr
			= &mxd_portio_din_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_portio_dout_record_function_list = {
	NULL,
	mxd_portio_dout_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_portio_dout_open,
	mxd_portio_dout_close
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_portio_dout_digital_output_function_list = {
	mxd_portio_dout_read,
	mxd_portio_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_portio_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_PORTIO_DOUTPUT_STANDARD_FIELDS
};

long mxd_portio_dout_num_record_fields
		= sizeof( mxd_portio_dout_record_field_defaults )
			/ sizeof( mxd_portio_dout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_portio_dout_rfield_def_ptr
			= &mxd_portio_dout_record_field_defaults[0];

static mx_status_type
mxd_portio_din_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_PORTIO_DINPUT **portio_dinput,
			const char *calling_fname )
{
	const char fname[] = "mxd_portio_din_get_pointers()";

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (portio_dinput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PORTIO_DINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dinput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*portio_dinput = (MX_PORTIO_DINPUT *)
				dinput->record->record_type_struct;

	if ( *portio_dinput == (MX_PORTIO_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PORTIO_DINPUT pointer for record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_portio_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_PORTIO_DOUTPUT **portio_doutput,
			const char *calling_fname )
{
	const char fname[] = "mxd_portio_dout_get_pointers()";

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (portio_doutput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PORTIO_DOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*portio_doutput = (MX_PORTIO_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( *portio_doutput == (MX_PORTIO_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PORTIO_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_portio_din_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_portio_din_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_PORTIO_DINPUT *portio_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        portio_dinput = (MX_PORTIO_DINPUT *) malloc( sizeof(MX_PORTIO_DINPUT) );

        if ( portio_dinput == (MX_PORTIO_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_PORTIO_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = portio_dinput;
        record->class_specific_function_list
                                = &mxd_portio_din_digital_input_function_list;

        digital_input->record = record;
	portio_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_portio_din_open( MX_RECORD *record )
{
	const char fname[] = "mxd_portio_din_open()";

	MX_PORTIO_DINPUT *portio_dinput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	mx_status = mxd_portio_din_get_pointers(
			(MX_DIGITAL_INPUT *) record->record_class_struct,
			&portio_dinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Try to gain access to the I/O port. */

	mx_status = mx_portio_request_region( portio_dinput->portio_record,
						portio_dinput->address, 1 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_portio_din_close( MX_RECORD *record )
{
	const char fname[] = "mxd_portio_din_close()";

	MX_PORTIO_DINPUT *portio_dinput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	mx_status = mxd_portio_din_get_pointers(
			(MX_DIGITAL_INPUT *) record->record_class_struct,
			&portio_dinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Give up access to the I/O port. */

	mx_status = mx_portio_release_region( portio_dinput->portio_record,
						portio_dinput->address, 1 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_portio_din_read( MX_DIGITAL_INPUT *dinput )
{
	const char fname[] = "mxd_portio_din_read()";

	MX_PORTIO_DINPUT *portio_dinput;
	mx_uint8_type d8_value;
	mx_uint16_type d16_value;
#if 0
	mx_uint32_type d32_value;
#endif
	mx_status_type mx_status;

	mx_status = mxd_portio_din_get_pointers( dinput,
						&portio_dinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( portio_dinput->data_size ) {
	case 8:
		d8_value = mx_portio_inp8( portio_dinput->portio_record,
						portio_dinput->address );

		dinput->value = (long) ( d8_value & 0xff );
		break;
	case 16:
		d16_value = mx_portio_inp16( portio_dinput->portio_record,
						portio_dinput->address );

		dinput->value = (long) ( d16_value & 0xffff );
		break;
#if 0
	case 32:
		d32_value = mx_portio_inp32( portio_dinput->portio_record,
						portio_dinput->address );

		dinput->value = (long) ( d32_value & 0xffffffff );
		break;
#endif
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal data size %lu for PORTIO digital input record '%s'.  "
		"The allowed values are 8, 16, and 32.",
			portio_dinput->data_size, dinput->record->name );
		break;
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_portio_dout_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_portio_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_PORTIO_DOUTPUT *portio_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        portio_doutput = (MX_PORTIO_DOUTPUT *) malloc( sizeof(MX_PORTIO_DOUTPUT) );

        if ( portio_doutput == (MX_PORTIO_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_PORTIO_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = portio_doutput;
        record->class_specific_function_list
                                = &mxd_portio_dout_digital_output_function_list;

        digital_output->record = record;
	portio_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_portio_dout_open( MX_RECORD *record )
{
	const char fname[] = "mxd_portio_dout_open()";

	MX_PORTIO_DOUTPUT *portio_doutput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	mx_status = mxd_portio_dout_get_pointers(
			(MX_DIGITAL_OUTPUT *) record->record_class_struct,
			&portio_doutput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Try to gain access to the I/O port. */

	mx_status = mx_portio_request_region( portio_doutput->portio_record,
						portio_doutput->address, 1 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_portio_dout_close( MX_RECORD *record )
{
	const char fname[] = "mxd_portio_dout_close()";

	MX_PORTIO_DOUTPUT *portio_doutput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	mx_status = mxd_portio_dout_get_pointers(
			(MX_DIGITAL_OUTPUT *) record->record_class_struct,
			&portio_doutput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Give up access to the I/O port. */

	mx_status = mx_portio_release_region( portio_doutput->portio_record,
						portio_doutput->address, 1 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_portio_dout_read( MX_DIGITAL_OUTPUT *doutput )
{
	/* For some devices, it is not safe or appropriate to read from
	 * an output address.  Thus, we merely return here the last value
	 * written to the address.
	 *
	 * If you actually need to be able to read from this address,
	 * create a portio_dinput record that points to the same address.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_portio_dout_write( MX_DIGITAL_OUTPUT *doutput )
{
	const char fname[] = "mxd_portio_dout_write()";

	MX_PORTIO_DOUTPUT *portio_doutput;
	mx_uint8_type d8_value;
	mx_uint16_type d16_value;
#if 0
	mx_uint32_type d32_value;
#endif
	mx_status_type mx_status;

	mx_status = mxd_portio_dout_get_pointers( doutput,
						&portio_doutput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( portio_doutput->data_size ) {
	case 8:
		d8_value = (mx_uint8_type) ( doutput->value & 0xff );

		mx_portio_outp8( portio_doutput->portio_record,
				portio_doutput->address, d8_value );
		break;
	case 16:
		d16_value = (mx_uint16_type) ( doutput->value & 0xffff );

		mx_portio_outp16( portio_doutput->portio_record,
				portio_doutput->address, d16_value );
		break;
#if 0
	case 32:
		d32_value = (mx_uint32_type) ( doutput->value & 0xffffffff );

		mx_portio_outp32( portio_doutput->portio_record,
				portio_doutput->address, d32_value );
		break;
#endif
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Illegal data size %lu for port I/O digital output record '%s'.  "
	"The allowed values are 8, 16, and 32.",
			portio_doutput->data_size, doutput->record->name );
		break;
	}
	return mx_status;
}

