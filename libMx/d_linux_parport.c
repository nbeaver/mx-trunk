/*
 * Name:    d_linux_parport.c
 *
 * Purpose: MX input and output drivers to control Linux parallel ports as
 *          digital input/output registers via the Linux parport driver.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#if defined( OS_LINUX )

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_driver.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "i_linux_parport.h"
#include "d_linux_parport.h"

/* Initialize the Linux parport driver tables. */

MX_RECORD_FUNCTION_LIST mxd_linux_parport_in_record_function_list = {
	NULL,
	mxd_linux_parport_in_create_record_structures,
	mxd_linux_parport_in_finish_record_initialization
};

MX_DIGITAL_INPUT_FUNCTION_LIST
 mxd_linux_parport_in_digital_input_function_list = {
	mxd_linux_parport_in_read
};

MX_RECORD_FIELD_DEFAULTS mxd_linux_parport_in_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_LINUX_PARPORT_IN_STANDARD_FIELDS
};

long mxd_linux_parport_in_num_record_fields
		= sizeof( mxd_linux_parport_in_record_field_defaults )
		  / sizeof( mxd_linux_parport_in_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_linux_parport_in_rfield_def_ptr
			= &mxd_linux_parport_in_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_linux_parport_out_record_function_list = {
	NULL,
	mxd_linux_parport_out_create_record_structures,
	mxd_linux_parport_out_finish_record_initialization
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
  mxd_linux_parport_out_digital_output_function_list = {
	mxd_linux_parport_out_read,
	mxd_linux_parport_out_write
};

MX_RECORD_FIELD_DEFAULTS mxd_linux_parport_out_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_LINUX_PARPORT_OUT_STANDARD_FIELDS
};

long mxd_linux_parport_out_num_record_fields
		= sizeof( mxd_linux_parport_out_record_field_defaults )
		  / sizeof( mxd_linux_parport_out_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_linux_parport_out_rfield_def_ptr
			= &mxd_linux_parport_out_record_field_defaults[0];

static mx_status_type
mxd_linux_parport_in_get_pointers( MX_RECORD *record,
				MX_LINUX_PARPORT_IN **linux_parport_in,
				MX_LINUX_PARPORT **linux_parport,
				const char *calling_fname )
{
	MX_LINUX_PARPORT_IN *linux_parport_in_ptr;
	MX_RECORD *linux_parport_record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
			"NULL MX_RECORD pointer passed." );
	}

	linux_parport_in_ptr = (MX_LINUX_PARPORT_IN *)
				record->record_type_struct;

	if ( linux_parport_in_ptr == (MX_LINUX_PARPORT_IN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
			"MX_LINUX_PARPORT_IN pointer for record '%s' is NULL.",
			record->name );
	}

	if ( linux_parport_in != (MX_LINUX_PARPORT_IN **) NULL ) {
		*linux_parport_in = linux_parport_in_ptr;
	}

	if ( linux_parport != (MX_LINUX_PARPORT **) NULL ) {

		linux_parport_record = (*linux_parport_in)->interface_record;

		if ( linux_parport_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
				calling_fname,
		"MX_LINUX_PARPORT interface pointer for record '%s' is NULL.",
				record->name );
		}

		*linux_parport = (MX_LINUX_PARPORT *)
			linux_parport_record->record_type_struct;

		if ( *linux_parport == (MX_LINUX_PARPORT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
				calling_fname,
		"MX_LINUX_PARPORT pointer for interface record '%s' is NULL.",
				linux_parport_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_linux_parport_out_get_pointers( MX_RECORD *record,
				MX_LINUX_PARPORT_OUT **linux_parport_out,
				MX_LINUX_PARPORT **linux_parport,
				const char *calling_fname )
{
	MX_LINUX_PARPORT_OUT *linux_parport_out_ptr;
	MX_RECORD *linux_parport_record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
			"NULL MX_RECORD pointer passed." );
	}

	linux_parport_out_ptr = (MX_LINUX_PARPORT_OUT *)
				record->record_type_struct;

	if ( linux_parport_out_ptr == (MX_LINUX_PARPORT_OUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
			"MX_LINUX_PARPORT_OUT pointer for record '%s' is NULL.",
			record->name );
	}

	if ( linux_parport_out != (MX_LINUX_PARPORT_OUT **) NULL ) {
		*linux_parport_out = linux_parport_out_ptr;
	}

	if ( linux_parport != (MX_LINUX_PARPORT **) NULL ) {

		linux_parport_record = (*linux_parport_out)->interface_record;

		if ( linux_parport_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
				calling_fname,
		"MX_LINUX_PARPORT interface pointer for record '%s' is NULL.",
				record->name );
		}

		*linux_parport = (MX_LINUX_PARPORT *)
			linux_parport_record->record_type_struct;

		if ( *linux_parport == (MX_LINUX_PARPORT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
				calling_fname,
		"MX_LINUX_PARPORT pointer for interface record '%s' is NULL.",
				linux_parport_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_linux_parport_in_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_linux_parport_in_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_LINUX_PARPORT_IN *linux_parport_in;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *)
				malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        linux_parport_in = (MX_LINUX_PARPORT_IN *)
		malloc( sizeof(MX_LINUX_PARPORT_IN) );

        if ( linux_parport_in == (MX_LINUX_PARPORT_IN *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_LINUX_PARPORT_IN structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = linux_parport_in;
        record->class_specific_function_list
		= &mxd_linux_parport_in_digital_input_function_list;

        digital_input->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linux_parport_in_finish_record_initialization( MX_RECORD *record )
{
        const char fname[] =
		"mxd_linux_parport_in_finish_record_initialization()";

        MX_LINUX_PARPORT_IN *linux_parport_in;
	int i, length, record_matches;

        linux_parport_in = (MX_LINUX_PARPORT_IN *) record->record_type_struct;

        if ( linux_parport_in == (MX_LINUX_PARPORT_IN *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
                	"MX_LINUX_PARPORT_IN pointer for record '%s' is NULL.",
			record->name);
        }

        /* Verify that 'interface_record' is the correct type of record. */

	record_matches = mx_verify_driver_type(
			linux_parport_in->interface_record,
			MXR_INTERFACE, MXI_GENERIC, MXI_GEN_LINUX_PARPORT );

	if ( record_matches == FALSE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an 'linux_parport' interface driver.",
			linux_parport_in->interface_record->name );
        }

	/* Convert the port name to lower case. */

	length = (int) strlen( linux_parport_in->port_name );

	for ( i = 0; i < length; i++ ) {
		if ( isupper( (int) (linux_parport_in->port_name[i]) ) ) {
			linux_parport_in->port_name[i] =
			    tolower( (int) (linux_parport_in->port_name[i]) );
		}
	}

        /* Check the port name. */

	if ( strcmp( linux_parport_in->port_name, "data" ) == 0 ) {
		linux_parport_in->port_number = MX_LINUX_PARPORT_DATA_PORT;
	} else
	if ( strcmp( linux_parport_in->port_name, "status" ) == 0 ) {
		linux_parport_in->port_number = MX_LINUX_PARPORT_STATUS_PORT;
	} else
	if ( strcmp( linux_parport_in->port_name, "control" ) == 0 ) {
		linux_parport_in->port_number = MX_LINUX_PARPORT_CONTROL_PORT;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized port type '%s' was specified for record '%s'.",
			linux_parport_in->port_name, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linux_parport_in_read( MX_DIGITAL_INPUT *dinput )
{
	const char fname[] = "mxd_linux_parport_in_read()";

	MX_LINUX_PARPORT_IN *linux_parport_in;
	MX_LINUX_PARPORT *linux_parport;
	uint8_t value;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	linux_parport = NULL;
	linux_parport_in = NULL;

	mx_status = mxd_linux_parport_in_get_pointers( dinput->record,
				&linux_parport_in, &linux_parport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_linux_parport_read_port( linux_parport,
					linux_parport_in->port_number,
					&value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dinput->value = (long) value;

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_linux_parport_out_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_linux_parport_out_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_LINUX_PARPORT_OUT *linux_parport_out;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
				malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        linux_parport_out = (MX_LINUX_PARPORT_OUT *)
		malloc( sizeof(MX_LINUX_PARPORT_OUT) );

        if ( linux_parport_out == (MX_LINUX_PARPORT_OUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_LINUX_PARPORT_OUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = linux_parport_out;
        record->class_specific_function_list
		= &mxd_linux_parport_out_digital_output_function_list;

        digital_output->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linux_parport_out_finish_record_initialization( MX_RECORD *record )
{
        const char fname[] =
		"mxd_linux_parport_out_finish_record_initialization()";

        MX_LINUX_PARPORT_OUT *linux_parport_out;
	int i, length, record_matches;

        linux_parport_out = (MX_LINUX_PARPORT_OUT *) record->record_type_struct;

        if ( linux_parport_out == (MX_LINUX_PARPORT_OUT *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
                	"MX_LINUX_PARPORT_OUT pointer for record '%s' is NULL.",
			record->name);
        }

        /* Verify that 'interface_record' is the correct type of record. */

	record_matches = mx_verify_driver_type(
			linux_parport_out->interface_record,
			MXR_INTERFACE, MXI_GENERIC, MXI_GEN_LINUX_PARPORT );

	if ( record_matches == FALSE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an 'linux_parport' interface driver.",
			linux_parport_out->interface_record->name );
        }

	/* Convert the port name to lower case. */

	length = (int) strlen( linux_parport_out->port_name );

	for ( i = 0; i < length; i++ ) {
		if ( isupper( (int) (linux_parport_out->port_name[i]) ) ) {
			linux_parport_out->port_name[i] =
			    tolower( (int) (linux_parport_out->port_name[i]) );
		}
	}

        /* Check the port name. */

	if ( strcmp( linux_parport_out->port_name, "data" ) == 0 ) {
		linux_parport_out->port_number = MX_LINUX_PARPORT_DATA_PORT;
	} else
	if ( strcmp( linux_parport_out->port_name, "status" ) == 0 ) {
		linux_parport_out->port_number = MX_LINUX_PARPORT_STATUS_PORT;
	} else
	if ( strcmp( linux_parport_out->port_name, "control" ) == 0 ) {
		linux_parport_out->port_number = MX_LINUX_PARPORT_CONTROL_PORT;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized port type '%s' was specified for record '%s'.",
			linux_parport_out->port_name, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linux_parport_out_read( MX_DIGITAL_OUTPUT *doutput )
{
	const char fname[] = "mxd_linux_parport_out_read()";

	MX_LINUX_PARPORT_OUT *linux_parport_out;
	MX_LINUX_PARPORT *linux_parport;
	uint8_t value;
	mx_status_type mx_status;

	mx_status = mxd_linux_parport_out_get_pointers( doutput->record,
				&linux_parport_out, &linux_parport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_linux_parport_read_port( linux_parport,
					linux_parport_out->port_number,
					&value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	doutput->value = (long) value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linux_parport_out_write( MX_DIGITAL_OUTPUT *doutput )
{
	const char fname[] = "mxd_linux_parport_out_write()";

	MX_LINUX_PARPORT_OUT *linux_parport_out;
	MX_LINUX_PARPORT *linux_parport;
	uint8_t value;
	mx_status_type mx_status;

	mx_status = mxd_linux_parport_out_get_pointers( doutput->record,
				&linux_parport_out, &linux_parport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	value = (uint8_t) ( doutput->value & 0xff );

	mx_status = mxi_linux_parport_write_port( linux_parport,
					linux_parport_out->port_number,
					value );

	return mx_status;
}

#endif /* OS_LINUX */

