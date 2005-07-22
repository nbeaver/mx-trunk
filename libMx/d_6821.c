/*
 * Name:    d_6821.c
 *
 * Purpose: MX input and output drivers to control Motorola MC6821 digital
 *          input/output registers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001 Illinois Institute of Technology
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
#include "i_6821.h"
#include "d_6821.h"

/* Initialize the Motorola MC6821 driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_6821_in_record_function_list = {
	mxd_6821_in_initialize_type,
	mxd_6821_in_create_record_structures,
	mxd_6821_in_finish_record_initialization,
	mxd_6821_in_delete_record,
	mxd_6821_in_print_structure,
	mxd_6821_in_read_parms_from_hardware,
	mxd_6821_in_write_parms_to_hardware,
	mxd_6821_in_open,
	mxd_6821_in_close
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_6821_in_digital_input_function_list = {
	mxd_6821_in_read
};

MX_RECORD_FIELD_DEFAULTS mxd_6821_in_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_6821_IN_STANDARD_FIELDS
};

long mxd_6821_in_num_record_fields
		= sizeof( mxd_6821_in_record_field_defaults )
			/ sizeof( mxd_6821_in_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_6821_in_rfield_def_ptr
			= &mxd_6821_in_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_6821_out_record_function_list = {
	mxd_6821_out_initialize_type,
	mxd_6821_out_create_record_structures,
	mxd_6821_out_finish_record_initialization,
	mxd_6821_out_delete_record,
	mxd_6821_out_print_structure,
	mxd_6821_out_read_parms_from_hardware,
	mxd_6821_out_write_parms_to_hardware,
	mxd_6821_out_open,
	mxd_6821_out_close
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_6821_out_digital_output_function_list = {
	mxd_6821_out_read,
	mxd_6821_out_write
};

MX_RECORD_FIELD_DEFAULTS mxd_6821_out_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_6821_OUT_STANDARD_FIELDS
};

long mxd_6821_out_num_record_fields
		= sizeof( mxd_6821_out_record_field_defaults )
			/ sizeof( mxd_6821_out_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_6821_out_rfield_def_ptr
			= &mxd_6821_out_record_field_defaults[0];

static mx_status_type
mxd_6821_in_get_pointers( MX_RECORD *record, MX_6821_IN **mc6821_in,
				MX_6821 **mc6821, const char *calling_fname )
{
	MX_RECORD *mc6821_record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
			"NULL MX_RECORD pointer passed to us." );
	}

	if ( (mc6821_in == NULL) || (mc6821 == NULL) ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
			"NULL pointer(s) passed as arguments." );
	}

	*mc6821_in = (MX_6821_IN *) record->record_type_struct;

	if ( *mc6821_in == (MX_6821_IN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"MX_6821_IN pointer for record '%s' is NULL.", record->name );
	}

	mc6821_record = (*mc6821_in)->interface_record;

	if ( mc6821_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"MX_6821 interface pointer for record '%s' is NULL.",
			record->name );
	}

	*mc6821 = (MX_6821 *) mc6821_record->record_type_struct;

	if ( *mc6821 == (MX_6821 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"MX_6821 pointer for interface record '%s' is NULL.",
			mc6821_record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_6821_out_get_pointers( MX_RECORD *record, MX_6821_OUT **mc6821_out,
				MX_6821 **mc6821, const char *calling_fname )
{
	MX_RECORD *mc6821_record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
			"NULL MX_RECORD pointer passed to us." );
	}

	if ( (mc6821_out == NULL) || (mc6821 == NULL) ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
			"NULL pointer(s) passed as arguments." );
	}

	*mc6821_out = (MX_6821_OUT *) record->record_type_struct;

	if ( *mc6821_out == (MX_6821_OUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"MX_6821_OUT pointer for record '%s' is NULL.", record->name );
	}

	mc6821_record = (*mc6821_out)->interface_record;

	if ( mc6821_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"MX_6821 interface pointer for record '%s' is NULL.",
			record->name );
	}

	*mc6821 = (MX_6821 *) mc6821_record->record_type_struct;

	if ( *mc6821 == (MX_6821 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"MX_6821 pointer for interface record '%s' is NULL.",
			mc6821_record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_6821_in_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_6821_in_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_6821_in_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_6821_IN *mc6821_in;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        mc6821_in = (MX_6821_IN *) malloc( sizeof(MX_6821_IN) );

        if ( mc6821_in == (MX_6821_IN *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_6821_IN structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = mc6821_in;
        record->class_specific_function_list
                                = &mxd_6821_in_digital_input_function_list;

        digital_input->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_6821_in_finish_record_initialization( MX_RECORD *record )
{
        const char fname[] = "mxd_6821_in_finish_record_initialization()";

        MX_6821_IN *mc6821_in;

        mc6821_in = (MX_6821_IN *) record->record_type_struct;

        if ( mc6821_in == (MX_6821_IN *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
                	"MX_6821_IN pointer for record '%s' is NULL.",
			record->name);
        }

        /* Verify that 'interface_record' is the correct type of record. */

        if ( mc6821_in->interface_record->mx_superclass != MXR_INTERFACE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
	                "'%s' is not an interface record.",
			mc6821_in->interface_record->name );
        }
        if ( mc6821_in->interface_record->mx_class != MXI_GENERIC ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an Motorola MC6821 interface driver.",
			mc6821_in->interface_record->name );
        }
        if ( mc6821_in->interface_record->mx_type != MXI_GEN_6821 ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an Motorola MC6821 interface driver.",
			mc6821_in->interface_record->name );
        }

        /* Check the port name and convert lowercase versions of the 
	 * name to upper case.
	 */

	switch( mc6821_in->port ) {
	case 'a':
		mc6821_in->port = 'A';
	case 'A':
		break;

	case 'b':
		mc6821_in->port = 'B';
	case 'B':
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized port name '%c'", mc6821_in->port);
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_6821_in_delete_record( MX_RECORD *record )
{
        if ( record == NULL ) {
                return MX_SUCCESSFUL_RESULT;
        }
        if ( record->record_type_struct != NULL ) {
                free( record->record_type_struct );

                record->record_type_struct = NULL;
        }
        if ( record->record_class_struct != NULL ) {
                free( record->record_class_struct );

                record->record_class_struct = NULL;
        }
        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_6821_in_print_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxd_6821_in_print_structure()";

	MX_DIGITAL_INPUT *dinput;
	MX_6821_IN *i6821_in;
	unsigned long value;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	dinput = (MX_DIGITAL_INPUT *) record->record_class_struct;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_DIGITAL_INPUT pointer for record '%s' is NULL.", record->name );
	}

	i6821_in = (MX_6821_IN *) record->record_type_struct;

	if ( i6821_in == (MX_6821_IN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_6821_IN pointer for record '%s' is NULL.", record->name);
	}

	fprintf(file, "DIGITAL_INPUT parameters for digital input '%s':\n",
			record->name);
	fprintf(file, "  Digital input type = MC6821_IN.\n\n");

	fprintf(file, "  name       = %s\n", record->name);

	status = mx_digital_input_read( record, &value );

	if ( status.code != MXE_SUCCESS )
		return status;

	fprintf(file, "  value      = %ld\n", dinput->value);
	fprintf(file, "  interface  = %s\n", i6821_in->interface_record->name);
	fprintf(file, "  port       = %c\n", i6821_in->port);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_6821_in_read_parms_from_hardware( MX_RECORD *record )
{
	const char fname[] = "mxd_6821_in_read_parms_from_hardware()";

	MX_DIGITAL_INPUT *dinput;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	dinput = (MX_DIGITAL_INPUT *) record->record_class_struct;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DIGITAL_INPUT pointer for record '%s' is NULL.",
			record->name );
	}

	status = mxd_6821_in_read( dinput );

	return status;
}

MX_EXPORT mx_status_type
mxd_6821_in_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_6821_in_open( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_6821_in_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_6821_in_read( MX_DIGITAL_INPUT *dinput )
{
	const char fname[] = "mxd_6821_in_read()";

	MX_6821_IN *mc6821_in;
	MX_6821 *mc6821;
	unsigned long address;
	mx_status_type status;

	status = mxd_6821_in_get_pointers( dinput->record,
						&mc6821_in, &mc6821, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	switch( mc6821_in->port ) {
	case 'A':
		address = mc6821->base_address + MX_MC6821_PORT_A_DATA;
		break;
	case 'B':
		address = mc6821->base_address + MX_MC6821_PORT_B_DATA;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized port name '%c'", mc6821_in->port);
		break;
	}

	dinput->value = (long) mx_portio_inp8( mc6821->portio_record, address );

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_6821_out_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_6821_out_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_6821_out_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_6821_OUT *mc6821_out;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        mc6821_out = (MX_6821_OUT *) malloc( sizeof(MX_6821_OUT) );

        if ( mc6821_out == (MX_6821_OUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_6821_OUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = mc6821_out;
        record->class_specific_function_list
                                = &mxd_6821_out_digital_output_function_list;

        digital_output->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_6821_out_finish_record_initialization( MX_RECORD *record )
{
        const char fname[] = "mxd_6821_out_finish_record_initialization()";

        MX_6821_OUT *mc6821_out;

        mc6821_out = (MX_6821_OUT *) record->record_type_struct;

        if ( mc6821_out == (MX_6821_OUT *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
                	"MX_6821_OUT pointer for record '%s' is NULL.",
			record->name);
        }

        /* Verify that 'interface_record' is the correct type of record. */

        if ( mc6821_out->interface_record->mx_superclass != MXR_INTERFACE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
	                "'%s' is not an interface record.",
			mc6821_out->interface_record->name );
        }
        if ( mc6821_out->interface_record->mx_class != MXI_GENERIC ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an Motorola MC6821 interface driver.",
			mc6821_out->interface_record->name );
        }
        if ( mc6821_out->interface_record->mx_type != MXI_GEN_6821 ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an Motorola MC6821 interface driver.",
			mc6821_out->interface_record->name );
        }

        /* Check the port name and convert lowercase versions of the 
	 * name to upper case.
	 */

	switch( mc6821_out->port ) {
	case 'a':
		mc6821_out->port = 'A';
	case 'A':
		break;

	case 'b':
		mc6821_out->port = 'B';
	case 'B':
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized port name '%c'", mc6821_out->port);
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_6821_out_delete_record( MX_RECORD *record )
{
        if ( record == NULL ) {
                return MX_SUCCESSFUL_RESULT;
        }
        if ( record->record_type_struct != NULL ) {
                free( record->record_type_struct );

                record->record_type_struct = NULL;
        }
        if ( record->record_class_struct != NULL ) {
                free( record->record_class_struct );

                record->record_class_struct = NULL;
        }
        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_6821_out_print_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxd_6821_out_print_structure()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_6821_OUT *mc6821_out;
	unsigned long value;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	doutput = (MX_DIGITAL_OUTPUT *) (record->record_class_struct);

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_DIGITAL_OUTPUT pointer for record '%s' is NULL.", record->name);
	}

	mc6821_out = (MX_6821_OUT *) record->record_type_struct;

	if ( mc6821_out == (MX_6821_OUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_6821_OUT pointer for record '%s' is NULL.", record->name);
	}

	fprintf(file, "DIGITAL_OUTPUT parameters for digital output '%s':\n",
			record->name);
	fprintf(file, "  Digital output type = MC6821_OUT.\n\n");

	fprintf(file, "  name       = %s\n", record->name);

	status = mx_digital_output_read( record, &value );

	if ( status.code != MXE_SUCCESS )
		return status;

	fprintf(file, "  value      = %ld\n", doutput->value);
	fprintf(file, "  interface  = %s\n",
					mc6821_out->interface_record->name);
	fprintf(file, "  port       = %c\n", mc6821_out->port);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_6821_out_read_parms_from_hardware( MX_RECORD *record )
{
	const char fname[] = "mxd_6821_out_read_parms_from_hardware()";

	MX_DIGITAL_OUTPUT *doutput;
	mx_status_type status;

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

	status = mxd_6821_out_read( doutput );

	return status;
}

MX_EXPORT mx_status_type
mxd_6821_out_write_parms_to_hardware( MX_RECORD *record )
{
	const char fname[] = "mxd_6821_out_write_parms_to_hardware()";

	MX_DIGITAL_OUTPUT *doutput;
	mx_status_type status;

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

	status = mxd_6821_out_write( doutput );

	return status;
}

MX_EXPORT mx_status_type
mxd_6821_out_open( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_6821_out_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_6821_out_read( MX_DIGITAL_OUTPUT *doutput )
{
	const char fname[] = "mxd_6821_out_read()";

	MX_6821_OUT *mc6821_out;
	MX_6821 *mc6821;
	unsigned long address;
	mx_status_type status;

	status = mxd_6821_out_get_pointers( doutput->record,
						&mc6821_out, &mc6821, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	switch( mc6821_out->port ) {
	case 'A':
		address = mc6821->base_address + MX_MC6821_PORT_A_DATA;
		break;
	case 'B':
		address = mc6821->base_address + MX_MC6821_PORT_B_DATA;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized port name '%c'", mc6821_out->port);
		break;
	}

	doutput->value = (long) mx_portio_inp8(
					mc6821->portio_record, address );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_6821_out_write( MX_DIGITAL_OUTPUT *doutput )
{
	const char fname[] = "mxd_6821_out_write()";

	MX_6821_OUT *mc6821_out;
	MX_6821 *mc6821;
	unsigned long address;
	mx_uint8_type value;
	mx_status_type status;

	status = mxd_6821_out_get_pointers( doutput->record,
						&mc6821_out, &mc6821, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	switch( mc6821_out->port ) {
	case 'A':
		address = mc6821->base_address + MX_MC6821_PORT_A_DATA;
		break;
	case 'B':
		address = mc6821->base_address + MX_MC6821_PORT_B_DATA;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized port name '%c'", mc6821_out->port);
		break;
	}

	value = (mx_uint8_type) ( doutput->value & 0xff );

	mx_portio_outp8( mc6821->portio_record, address, value );

	return MX_SUCCESSFUL_RESULT;
}

