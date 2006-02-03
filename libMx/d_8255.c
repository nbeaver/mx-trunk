/*
 * Name:    d_8255.c
 *
 * Purpose: MX input and output drivers to control Intel 8255 digital
 *          input/output registers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006 Illinois Institute of Technology
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
#include "mx_stdint.h"
#include "mx_driver.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "i_8255.h"
#include "d_8255.h"

/* Initialize the Intel 8255 driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_8255_in_record_function_list = {
	mxd_8255_in_initialize_type,
	mxd_8255_in_create_record_structures,
	mxd_8255_in_finish_record_initialization,
	mxd_8255_in_delete_record,
	mxd_8255_in_print_structure,
	mxd_8255_in_read_parms_from_hardware,
	mxd_8255_in_write_parms_to_hardware,
	mxd_8255_in_open,
	mxd_8255_in_close
};

MX_GENERIC_FUNCTION_LIST mxd_8255_in_generic_function_list = {
	NULL
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_8255_in_digital_input_function_list = {
	mxd_8255_in_read
};

MX_RECORD_FIELD_DEFAULTS mxd_8255_in_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_8255_IN_STANDARD_FIELDS
};

mx_length_type mxd_8255_in_num_record_fields
		= sizeof( mxd_8255_in_record_field_defaults )
			/ sizeof( mxd_8255_in_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_8255_in_rfield_def_ptr
			= &mxd_8255_in_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_8255_out_record_function_list = {
	mxd_8255_out_initialize_type,
	mxd_8255_out_create_record_structures,
	mxd_8255_out_finish_record_initialization,
	mxd_8255_out_delete_record,
	mxd_8255_out_print_structure,
	mxd_8255_out_read_parms_from_hardware,
	mxd_8255_out_write_parms_to_hardware,
	mxd_8255_out_open,
	mxd_8255_out_close
};

MX_GENERIC_FUNCTION_LIST mxd_8255_out_generic_function_list = {
	NULL
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_8255_out_digital_output_function_list = {
	mxd_8255_out_read,
	mxd_8255_out_write
};

MX_RECORD_FIELD_DEFAULTS mxd_8255_out_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_8255_OUT_STANDARD_FIELDS
};

mx_length_type mxd_8255_out_num_record_fields
		= sizeof( mxd_8255_out_record_field_defaults )
			/ sizeof( mxd_8255_out_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_8255_out_rfield_def_ptr
			= &mxd_8255_out_record_field_defaults[0];

static mx_status_type
mxd_8255_in_get_pointers( MX_RECORD *record, MX_8255_IN **i8255_in,
				MX_8255 **i8255, const char *calling_fname )
{
	MX_RECORD *i8255_record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
			"NULL MX_RECORD pointer passed to us." );
	}

	if ( (i8255_in == NULL) || (i8255 == NULL) ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
			"NULL pointer(s) passed as arguments." );
	}

	*i8255_in = (MX_8255_IN *) record->record_type_struct;

	if ( *i8255_in == (MX_8255_IN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"MX_8255_IN pointer for record '%s' is NULL.", record->name );
	}

	i8255_record = (*i8255_in)->interface_record;

	if ( i8255_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"MX_8255 interface pointer for record '%s' is NULL.",
			record->name );
	}

	*i8255 = (MX_8255 *) i8255_record->record_type_struct;

	if ( *i8255 == (MX_8255 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"MX_8255 pointer for interface record '%s' is NULL.",
			i8255_record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_8255_out_get_pointers( MX_RECORD *record, MX_8255_OUT **i8255_out,
				MX_8255 **i8255, const char *calling_fname )
{
	MX_RECORD *i8255_record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
			"NULL MX_RECORD pointer passed to us." );
	}

	if ( (i8255_out == NULL) || (i8255 == NULL) ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
			"NULL pointer(s) passed as arguments." );
	}

	*i8255_out = (MX_8255_OUT *) record->record_type_struct;

	if ( *i8255_out == (MX_8255_OUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"MX_8255_OUT pointer for record '%s' is NULL.", record->name );
	}

	i8255_record = (*i8255_out)->interface_record;

	if ( i8255_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"MX_8255 interface pointer for record '%s' is NULL.",
			record->name );
	}

	*i8255 = (MX_8255 *) i8255_record->record_type_struct;

	if ( *i8255 == (MX_8255 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"MX_8255 pointer for interface record '%s' is NULL.",
			i8255_record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_8255_in_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_8255_in_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_8255_in_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_8255_IN *i8255_in;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        i8255_in = (MX_8255_IN *) malloc( sizeof(MX_8255_IN) );

        if ( i8255_in == (MX_8255_IN *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_8255_IN structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = i8255_in;
        record->class_specific_function_list
                                = &mxd_8255_in_digital_input_function_list;

        digital_input->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_8255_in_finish_record_initialization( MX_RECORD *record )
{
        const char fname[] = "mxd_8255_in_finish_record_initialization()";

        MX_8255_IN *i8255_in;

        i8255_in = (MX_8255_IN *) record->record_type_struct;

        if ( i8255_in == (MX_8255_IN *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
                	"MX_8255_IN pointer for record '%s' is NULL.",
			record->name);
        }

        /* Verify that 'interface_record' is the correct type of record. */

        if ( i8255_in->interface_record->mx_superclass != MXR_INTERFACE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
	                "'%s' is not an interface record.",
			i8255_in->interface_record->name );
        }
        if ( i8255_in->interface_record->mx_class != MXI_GENERIC ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an Intel 8255 interface driver.",
			i8255_in->interface_record->name );
        }
        if ( i8255_in->interface_record->mx_type != MXI_GEN_8255 ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an Intel 8255 interface driver.",
			i8255_in->interface_record->name );
        }

        /* Check the port number. */

	switch( i8255_in->port[0] ) {
	case 'a':
	case 'A':
		i8255_in->port_number = MX_8255_PORT_A;

		strcpy( i8255_in->port, "A" );
		break;
	case 'b':
	case 'B':
		i8255_in->port_number = MX_8255_PORT_B;

		strcpy( i8255_in->port, "B" );
		break;
	case 'c':
	case 'C':
		switch( i8255_in->port[1] ) {
		case '\0':
			i8255_in->port_number = MX_8255_PORT_C;

			strcpy( i8255_in->port, "C" );
			break;
		case 'h':
		case 'H':
			i8255_in->port_number = MX_8255_PORT_CH;

			strcpy( i8255_in->port, "CH" );
			break;
		case 'l':
		case 'L':
			i8255_in->port_number = MX_8255_PORT_CL;

			strcpy( i8255_in->port, "CL" );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized half port name '%s'", i8255_in->port);
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized port name '%s'", i8255_in->port);
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_8255_in_delete_record( MX_RECORD *record )
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
mxd_8255_in_print_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxd_8255_in_print_structure()";

	MX_DIGITAL_INPUT *dinput;
	MX_8255_IN *i8255_in;
	uint32_t value;
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

	i8255_in = (MX_8255_IN *) record->record_type_struct;

	if ( i8255_in == (MX_8255_IN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_8255_IN pointer for record '%s' is NULL.", record->name);
	}

	fprintf(file, "DIGITAL_INPUT parameters for digital input '%s':\n",
			record->name);
	fprintf(file, "  Digital input type = I8255_IN.\n\n");

	fprintf(file, "  name       = %s\n", record->name);

	status = mx_digital_input_read( record, &value );

	if ( status.code != MXE_SUCCESS )
		return status;

	fprintf(file, "  value      = %lu\n", (unsigned long) dinput->value);
	fprintf(file, "  interface  = %s\n", i8255_in->interface_record->name);
	fprintf(file, "  port       = %s\n", i8255_in->port);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_8255_in_read_parms_from_hardware( MX_RECORD *record )
{
	const char fname[] = "mxd_8255_in_read_parms_from_hardware()";

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

	status = mxd_8255_in_read( dinput );

	return status;
}

MX_EXPORT mx_status_type
mxd_8255_in_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_8255_in_open( MX_RECORD *record )
{
	const char fname[] = "mxd_8255_in_open()";

	MX_8255_IN *i8255_in;
	MX_8255 *i8255;
	uint8_t port_d_value;
	mx_status_type status;

	status = mxd_8255_in_get_pointers( record, &i8255_in, &i8255, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Get the previous status of port D. */

	status = mxi_8255_read_port( i8255, MX_8255_PORT_D, &port_d_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Set the port to be an input port. */

	switch( i8255_in->port_number ) {
	case MX_8255_PORT_A:
		port_d_value |= MXF_8255_PORT_A_INPUT;
		break;
	case MX_8255_PORT_B:
		port_d_value |= MXF_8255_PORT_B_INPUT;
		break;
	case MX_8255_PORT_C:
		port_d_value
			|= ( MXF_8255_PORT_CH_INPUT | MXF_8255_PORT_CL_INPUT );
		break;
	case MX_8255_PORT_CH:
		port_d_value |= MXF_8255_PORT_CH_INPUT;
		break;
	case MX_8255_PORT_CL:
		port_d_value |= MXF_8255_PORT_CL_INPUT;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal 8255 port number %d", i8255_in->port_number );
		break;
	}

	status = mxi_8255_write_port( i8255, MX_8255_PORT_D, port_d_value );

	return status;
}

MX_EXPORT mx_status_type
mxd_8255_in_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_8255_in_read( MX_DIGITAL_INPUT *dinput )
{
	const char fname[] = "mxd_8255_in_read()";

	MX_8255_IN *i8255_in;
	MX_8255 *i8255;
	uint8_t value;
	mx_status_type status;

	status = mxd_8255_in_get_pointers( dinput->record,
						&i8255_in, &i8255, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mxi_8255_read_port( i8255, i8255_in->port_number, &value );

	if ( status.code != MXE_SUCCESS )
		return status;

	dinput->value = (long) value;

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_8255_out_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_8255_out_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_8255_out_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_8255_OUT *i8255_out;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        i8255_out = (MX_8255_OUT *) malloc( sizeof(MX_8255_OUT) );

        if ( i8255_out == (MX_8255_OUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_8255_OUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = i8255_out;
        record->class_specific_function_list
                                = &mxd_8255_out_digital_output_function_list;

        digital_output->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_8255_out_finish_record_initialization( MX_RECORD *record )
{
        const char fname[] = "mxd_8255_out_finish_record_initialization()";

        MX_8255_OUT *i8255_out;

        i8255_out = (MX_8255_OUT *) record->record_type_struct;

        if ( i8255_out == (MX_8255_OUT *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
                	"MX_8255_OUT pointer for record '%s' is NULL.",
			record->name);
        }

        /* Verify that 'interface_record' is the correct type of record. */

        if ( i8255_out->interface_record->mx_superclass != MXR_INTERFACE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
	                "'%s' is not an interface record.",
			i8255_out->interface_record->name );
        }
        if ( i8255_out->interface_record->mx_class != MXI_GENERIC ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an Intel 8255 interface driver.",
			i8255_out->interface_record->name );
        }
        if ( i8255_out->interface_record->mx_type != MXI_GEN_8255 ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an Intel 8255 interface driver.",
			i8255_out->interface_record->name );
        }

        /* Check the port number. */

	switch( i8255_out->port[0] ) {
	case 'a':
	case 'A':
		i8255_out->port_number = MX_8255_PORT_A;

		strcpy( i8255_out->port, "A" );
		break;
	case 'b':
	case 'B':
		i8255_out->port_number = MX_8255_PORT_B;

		strcpy( i8255_out->port, "B" );
		break;
	case 'c':
	case 'C':
		switch( i8255_out->port[1] ) {
		case '\0':
			i8255_out->port_number = MX_8255_PORT_C;

			strcpy( i8255_out->port, "C" );
			break;
		case 'h':
		case 'H':
			i8255_out->port_number = MX_8255_PORT_CH;

			strcpy( i8255_out->port, "CH" );
			break;
		case 'l':
		case 'L':
			i8255_out->port_number = MX_8255_PORT_CL;

			strcpy( i8255_out->port, "CL" );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized half port name '%s'", i8255_out->port);
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized port name '%s'", i8255_out->port);
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_8255_out_delete_record( MX_RECORD *record )
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
mxd_8255_out_print_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxd_8255_out_print_structure()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_8255_OUT *i8255_out;
	uint32_t value;
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

	i8255_out = (MX_8255_OUT *) record->record_type_struct;

	if ( i8255_out == (MX_8255_OUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_8255_OUT pointer for record '%s' is NULL.", record->name);
	}

	fprintf(file, "DIGITAL_OUTPUT parameters for digital output '%s':\n",
			record->name);
	fprintf(file, "  Digital output type = I8255_OUT.\n\n");

	fprintf(file, "  name       = %s\n", record->name);

	status = mx_digital_output_read( record, &value );

	if ( status.code != MXE_SUCCESS )
		return status;

	fprintf(file, "  value      = %lu\n", (unsigned long) doutput->value);
	fprintf(file, "  interface  = %s\n", i8255_out->interface_record->name);
	fprintf(file, "  port       = %s\n", i8255_out->port);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_8255_out_read_parms_from_hardware( MX_RECORD *record )
{
	const char fname[] = "mxd_8255_out_read_parms_from_hardware()";

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

	status = mxd_8255_out_read( doutput );

	return status;
}

MX_EXPORT mx_status_type
mxd_8255_out_write_parms_to_hardware( MX_RECORD *record )
{
	const char fname[] = "mxd_8255_out_write_parms_to_hardware()";

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

	status = mxd_8255_out_write( doutput );

	return status;
}

MX_EXPORT mx_status_type
mxd_8255_out_open( MX_RECORD *record )
{
	const char fname[] = "mxd_8255_out_open()";

	MX_8255_OUT *i8255_out;
	MX_8255 *i8255;
	uint8_t port_d_value, mask;
	mx_status_type status;

	status = mxd_8255_out_get_pointers( record, &i8255_out, &i8255, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Get the previous status of port D. */

	status = mxi_8255_read_port( i8255, MX_8255_PORT_D, &port_d_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Set the port to be an output port. */

	switch( i8255_out->port_number ) {
	case MX_8255_PORT_A:
		port_d_value &= ( ~MXF_8255_PORT_A_INPUT );
		break;
	case MX_8255_PORT_B:
		port_d_value &= ( ~MXF_8255_PORT_B_INPUT );
		break;
	case MX_8255_PORT_C:
		mask = (MXF_8255_PORT_CH_INPUT | MXF_8255_PORT_CL_INPUT);

		port_d_value &= ( ~mask );
		break;
	case MX_8255_PORT_CH:
		port_d_value &= ( ~MXF_8255_PORT_CH_INPUT );
		break;
	case MX_8255_PORT_CL:
		port_d_value &= ( ~MXF_8255_PORT_CL_INPUT );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal 8255 port number %d", i8255_out->port_number );
		break;
	}

	status = mxi_8255_write_port( i8255, MX_8255_PORT_D, port_d_value );

	return status;
}

MX_EXPORT mx_status_type
mxd_8255_out_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_8255_out_read( MX_DIGITAL_OUTPUT *doutput )
{
	const char fname[] = "mxd_8255_out_read()";

	MX_8255_OUT *i8255_out;
	MX_8255 *i8255;
	uint8_t value;
	mx_status_type status;

	status = mxd_8255_out_get_pointers( doutput->record,
						&i8255_out, &i8255, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mxi_8255_read_port( i8255, i8255_out->port_number, &value );

	if ( status.code != MXE_SUCCESS )
		return status;

	doutput->value = (long) value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_8255_out_write( MX_DIGITAL_OUTPUT *doutput )
{
	const char fname[] = "mxd_8255_out_write()";

	MX_8255_OUT *i8255_out;
	MX_8255 *i8255;
	uint8_t value;
	mx_status_type status;

	status = mxd_8255_out_get_pointers( doutput->record,
						&i8255_out, &i8255, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	value = (uint8_t) ( doutput->value & 0xff );

	status = mxi_8255_write_port( i8255, i8255_out->port_number, value );

	return status;
}

