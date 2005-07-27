/*
 * Name:    d_pdi45_dio.c
 *
 * Purpose: MX input and output drivers to control Prairie Digital Model 45
 *          digital I/O lines.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2005 Illinois Institute of Technology
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
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "i_pdi45.h"
#include "d_pdi45_dio.h"

/* Initialize the PDI45 digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_pdi45_din_record_function_list = {
	NULL,
	mxd_pdi45_din_create_record_structures
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_pdi45_din_digital_input_function_list = {
	mxd_pdi45_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_pdi45_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_PDI45_DINPUT_STANDARD_FIELDS
};

long mxd_pdi45_din_num_record_fields
		= sizeof( mxd_pdi45_din_record_field_defaults )
			/ sizeof( mxd_pdi45_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pdi45_din_rfield_def_ptr
			= &mxd_pdi45_din_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_pdi45_dout_record_function_list = {
	NULL,
	mxd_pdi45_dout_create_record_structures
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_pdi45_dout_digital_output_function_list = {
	mxd_pdi45_dout_read,
	mxd_pdi45_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_pdi45_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_PDI45_DOUTPUT_STANDARD_FIELDS
};

long mxd_pdi45_dout_num_record_fields
		= sizeof( mxd_pdi45_dout_record_field_defaults )
			/ sizeof( mxd_pdi45_dout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pdi45_dout_rfield_def_ptr
			= &mxd_pdi45_dout_record_field_defaults[0];

static mx_status_type
mxd_pdi45_din_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_PDI45_DINPUT **pdi45_dinput,
			MX_PDI45 **pdi45,
			const char *calling_fname )
{
	const char fname[] = "mxd_pdi45_din_get_pointers()";

	MX_RECORD *pdi45_record;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pdi45_dinput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PDI45_DINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pdi45 == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PDI45 pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dinput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pdi45_dinput = (MX_PDI45_DINPUT *)
			dinput->record->record_type_struct;

	if ( *pdi45_dinput == (MX_PDI45_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PDI45_DINPUT pointer for record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	pdi45_record = (*pdi45_dinput)->pdi45_record;

	if ( pdi45_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_PDI45 pointer for PDI45 digital input record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	if ( pdi45_record->mx_type != MXI_GEN_PDI45 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"pdi45_record '%s' for PDI45 digital input '%s' is not a PDI45 record.  "
"Instead, it is a '%s' record.",
			pdi45_record->name, dinput->record->name,
			mx_get_driver_name( pdi45_record ) );
	}

	*pdi45 = (MX_PDI45 *) pdi45_record->record_type_struct;

	if ( *pdi45 == (MX_PDI45 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_PDI45 pointer for PDI45 record '%s' used by "
	"PDI45 digital input record '%s' and passed by '%s' is NULL.",
			pdi45_record->name,
			dinput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pdi45_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_PDI45_DOUTPUT **pdi45_doutput,
			MX_PDI45 **pdi45,
			const char *calling_fname )
{
	const char fname[] = "mxd_pdi45_dout_get_pointers()";

	MX_RECORD *pdi45_record;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pdi45_doutput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PDI45_DOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pdi45 == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PDI45 pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pdi45_doutput = (MX_PDI45_DOUTPUT *)
			doutput->record->record_type_struct;

	if ( *pdi45_doutput == (MX_PDI45_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PDI45_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	pdi45_record = (*pdi45_doutput)->pdi45_record;

	if ( pdi45_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_PDI45 pointer for PDI45 digital output record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	if ( pdi45_record->mx_type != MXI_GEN_PDI45 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"pdi45_record '%s' for PDI45 digital output '%s' is not a PDI45 record.  "
"Instead, it is a '%s' record.",
			pdi45_record->name, doutput->record->name,
			mx_get_driver_name( pdi45_record ) );
	}

	*pdi45 = (MX_PDI45 *) pdi45_record->record_type_struct;

	if ( *pdi45 == (MX_PDI45 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_PDI45 pointer for PDI45 record '%s' used by "
	"PDI45 digital output record '%s' and passed by '%s' is NULL.",
			pdi45_record->name,
			doutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_pdi45_din_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_pdi45_din_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_PDI45_DINPUT *pdi45_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        pdi45_dinput = (MX_PDI45_DINPUT *) malloc( sizeof(MX_PDI45_DINPUT) );

        if ( pdi45_dinput == (MX_PDI45_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_PDI45_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = pdi45_dinput;
        record->class_specific_function_list
                                = &mxd_pdi45_din_digital_input_function_list;

        digital_input->record = record;
	pdi45_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi45_din_read( MX_DIGITAL_INPUT *dinput )
{
	const char fname[] = "mxd_pdi45_din_read()";

	MX_PDI45_DINPUT *pdi45_dinput;
	MX_PDI45 *pdi45;
	char response[80];
	int num_items, hex_value;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	pdi45 = NULL;
	pdi45_dinput = NULL;

	mx_status = mxd_pdi45_din_get_pointers( dinput,
					&pdi45_dinput, &pdi45, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_pdi45_command( pdi45, "00M",
				response, sizeof( response ) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response + 3, "%2x", &hex_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unrecognizable digital I/O levels in response '%s' "
		"to command '00M' for PDI45 controller '%s'.",
			response, pdi45->record->name );
	}

	MX_DEBUG(-2,("%s: hex_value = %#02X", fname, hex_value));

	hex_value &= ( 1 << (pdi45_dinput->line_number) );

	if ( pdi45_dinput->pdi45_dinput_flags & MXF_PDI45_DIO_INVERT ) {

		if ( hex_value != 0 ) {
			dinput->value = 1;
		} else {
			dinput->value = 0;
		}
	} else {
		if ( hex_value != 0 ) {
			dinput->value = 0;
		} else {
			dinput->value = 1;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_pdi45_dout_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_pdi45_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_PDI45_DOUTPUT *pdi45_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        pdi45_doutput = (MX_PDI45_DOUTPUT *) malloc( sizeof(MX_PDI45_DOUTPUT) );

        if ( pdi45_doutput == (MX_PDI45_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_PDI45_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = pdi45_doutput;
        record->class_specific_function_list
                                = &mxd_pdi45_dout_digital_output_function_list;

        digital_output->record = record;
	pdi45_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi45_dout_read( MX_DIGITAL_OUTPUT *doutput )
{
	const char fname[] = "mxd_pdi45_dout_read()";

	MX_PDI45_DOUTPUT *pdi45_doutput;
	MX_PDI45 *pdi45;
	char response[80];
	int num_items, hex_value;
	mx_status_type mx_status;

	mx_status = mxd_pdi45_dout_get_pointers( doutput,
					&pdi45_doutput, &pdi45, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_pdi45_command( pdi45, "00M",
				response, sizeof( response ) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response + 3, "%2x", &hex_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unrecognizable digital I/O levels in response '%s' "
		"to command '00M' for PDI45 controller '%s'.",
			response, pdi45->record->name );
	}

	MX_DEBUG(-2,("%s: hex_value = %#02X", fname, hex_value));

	hex_value &= ( 1 << (pdi45_doutput->line_number) );

	if ( pdi45_doutput->pdi45_doutput_flags & MXF_PDI45_DIO_INVERT ) {

		if ( hex_value != 0 ) {
			doutput->value = 1;
		} else {
			doutput->value = 0;
		}
	} else {
		if ( hex_value != 0 ) {
			doutput->value = 0;
		} else {
			doutput->value = 1;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi45_dout_write( MX_DIGITAL_OUTPUT *doutput )
{
	const char fname[] = "mxd_pdi45_dout_write()";

	MX_PDI45_DOUTPUT *pdi45_doutput;
	MX_PDI45 *pdi45;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_pdi45_dout_get_pointers( doutput,
					&pdi45_doutput, &pdi45, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pdi45_doutput->pdi45_doutput_flags & MXF_PDI45_DIO_INVERT ) {

		if ( doutput->value != 0 ) {
			sprintf( command, "00K%02X",
				( 1 << (pdi45_doutput->line_number) ) );
		} else {
			sprintf( command, "00L%02X",
				( 1 << (pdi45_doutput->line_number) ) );
		}
	} else {
		if ( doutput->value != 0 ) {
			sprintf( command, "00L%02X",
				( 1 << (pdi45_doutput->line_number) ) );
		} else {
			sprintf( command, "00K%02X",
				( 1 << (pdi45_doutput->line_number) ) );
		}
	}

	mx_status = mxi_pdi45_command( pdi45, command, NULL, 0 );

	return mx_status;
}

