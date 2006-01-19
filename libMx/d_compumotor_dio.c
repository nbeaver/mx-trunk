/*
 * Name:    d_compumotor_dio.c
 *
 * Purpose: MX drivers to control Compumotor 6K digital inputs and outputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001, 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_COMPUMOTOR_DIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "i_compumotor.h"
#include "d_compumotor_dio.h"

/* Initialize the COMPUMOTOR digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_compumotor_din_record_function_list = {
	mxd_compumotor_din_initialize_type,
	mxd_compumotor_din_create_record_structures,
	mxd_compumotor_din_finish_record_initialization,
	mxd_compumotor_din_delete_record,
	NULL,
	mxd_compumotor_din_read_parms_from_hardware,
	mxd_compumotor_din_write_parms_to_hardware,
	mxd_compumotor_din_open,
	mxd_compumotor_din_close
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_compumotor_din_digital_input_function_list
= {
	mxd_compumotor_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_compumotor_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_COMPUMOTOR_DINPUT_STANDARD_FIELDS
};

long mxd_compumotor_din_num_record_fields
		= sizeof( mxd_compumotor_din_record_field_defaults )
			/ sizeof( mxd_compumotor_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_compumotor_din_rfield_def_ptr
			= &mxd_compumotor_din_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_compumotor_dout_record_function_list = {
	mxd_compumotor_dout_initialize_type,
	mxd_compumotor_dout_create_record_structures,
	mxd_compumotor_dout_finish_record_initialization,
	mxd_compumotor_dout_delete_record,
	NULL,
	mxd_compumotor_dout_read_parms_from_hardware,
	mxd_compumotor_dout_write_parms_to_hardware,
	mxd_compumotor_dout_open,
	mxd_compumotor_dout_close
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_compumotor_dout_digital_output_function_list
= {
	mxd_compumotor_dout_read,
	mxd_compumotor_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_compumotor_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_COMPUMOTOR_DOUTPUT_STANDARD_FIELDS
};

long mxd_compumotor_dout_num_record_fields
		= sizeof( mxd_compumotor_dout_record_field_defaults )
			/ sizeof( mxd_compumotor_dout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_compumotor_dout_rfield_def_ptr
			= &mxd_compumotor_dout_record_field_defaults[0];

static mx_status_type
mxd_compumotor_din_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_COMPUMOTOR_DINPUT **compumotor_dinput,
			MX_COMPUMOTOR_INTERFACE **compumotor_interface,
			const char *calling_fname )
{
	const char fname[] = "mxd_compumotor_din_get_pointers()";

	MX_RECORD *compumotor_interface_record;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (compumotor_dinput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_COMPUMOTOR_DINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (compumotor_interface == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_COMPUMOTOR_INTERFACE pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dinput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*compumotor_dinput = (MX_COMPUMOTOR_DINPUT *)
					dinput->record->record_type_struct;

	if ( *compumotor_dinput == (MX_COMPUMOTOR_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_COMPUMOTOR_DINPUT pointer for record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	compumotor_interface_record
		= (*compumotor_dinput)->compumotor_interface_record;

	if ( compumotor_interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"compumotor_interface_record pointer for Compumotor digital input "
	"record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	if ( compumotor_interface_record->mx_type != MXI_GEN_COMPUMOTOR ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"compumotor_interface_record '%s' for Compumotor digital "
		"input '%s' is not a Compumotor interface record.  "
		"Instead, it is a '%s' record.",
			compumotor_interface_record->name, dinput->record->name,
			mx_get_driver_name( compumotor_interface_record ) );
	}

	*compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
			compumotor_interface_record->record_type_struct;

	if ( *compumotor_interface == (MX_COMPUMOTOR_INTERFACE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_COMPUMOTOR_INTERFACE pointer for Compumotor record '%s' used "
	"by Compumotor digital input record '%s' and passed by '%s' is NULL.",
			compumotor_interface_record->name,
			dinput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_compumotor_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_COMPUMOTOR_DOUTPUT **compumotor_doutput,
			MX_COMPUMOTOR_INTERFACE **compumotor_interface,
			const char *calling_fname )
{
	const char fname[] = "mxd_compumotor_dout_get_pointers()";

	MX_RECORD *compumotor_interface_record;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (compumotor_doutput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_COMPUMOTOR_DOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (compumotor_interface == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_COMPUMOTOR_INTERFACE pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*compumotor_doutput = (MX_COMPUMOTOR_DOUTPUT *)
					doutput->record->record_type_struct;

	if ( *compumotor_doutput == (MX_COMPUMOTOR_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_COMPUMOTOR_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	compumotor_interface_record
		= (*compumotor_doutput)->compumotor_interface_record;

	if ( compumotor_interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"compumotor_interface_record pointer for Compumotor digital output "
	"record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	if ( compumotor_interface_record->mx_type != MXI_GEN_COMPUMOTOR ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"compumotor_interface_record '%s' for Compumotor digital "
		"output '%s' is not a Compumotor interface record.  "
		"Instead, it is a '%s' record.",
		compumotor_interface_record->name, doutput->record->name,
			mx_get_driver_name( compumotor_interface_record ) );
	}

	*compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
			compumotor_interface_record->record_type_struct;

	if ( *compumotor_interface == (MX_COMPUMOTOR_INTERFACE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_COMPUMOTOR_INTERFACE pointer for Compumotor record '%s' used "
	"by Compumotor digital output record '%s' and passed by '%s' is NULL.",
			compumotor_interface_record->name,
			doutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_compumotor_din_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_din_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_compumotor_din_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_COMPUMOTOR_DINPUT *compumotor_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        compumotor_dinput = (MX_COMPUMOTOR_DINPUT *)
				malloc( sizeof(MX_COMPUMOTOR_DINPUT) );

        if ( compumotor_dinput == (MX_COMPUMOTOR_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_COMPUMOTOR_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = compumotor_dinput;
        record->class_specific_function_list
			= &mxd_compumotor_din_digital_input_function_list;

        digital_input->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_din_finish_record_initialization( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_din_delete_record( MX_RECORD *record )
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
mxd_compumotor_din_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_din_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_din_open( MX_RECORD *record )
{
	const char fname[] = "mxd_compumotor_din_open()";

	MX_DIGITAL_INPUT *dinput;
	MX_COMPUMOTOR_DINPUT *compumotor_dinput;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	dinput = (MX_DIGITAL_INPUT *) record->record_class_struct;

	mx_status = mxd_compumotor_din_get_pointers(dinput, &compumotor_dinput,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_compumotor_get_controller_index( compumotor_interface,
				compumotor_dinput->controller_number,
				&(compumotor_dinput->controller_index));

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the current value of the register. */

	mx_status = mxd_compumotor_din_read( dinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_compumotor_din_close( MX_RECORD *record )
{
	const char fname[] = "mxd_compumotor_din_close()";

	MX_DIGITAL_INPUT *dinput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	dinput = (MX_DIGITAL_INPUT *) record->record_class_struct;

	/* Read the value of the input so that it can be recorded
	 * correctly in the database file if the database file is
	 * configured to be written to at program shutdown.
	 */

	mx_status = mxd_compumotor_din_read( dinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_compumotor_din_read( MX_DIGITAL_INPUT *dinput )
{
	const char fname[] = "mxd_compumotor_din_read()";

	MX_COMPUMOTOR_DINPUT *compumotor_dinput;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	char command[80];
	char response[200];
	int i, j, num_items;
	unsigned long controller_type;
	mx_status_type mx_status;

	mx_status = mxd_compumotor_din_get_pointers(dinput, &compumotor_dinput,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	controller_type = compumotor_interface->controller_type[
					compumotor_dinput->controller_index ];

	if ( compumotor_dinput->num_bits == 1 ) {

		if ( controller_type == MXT_COMPUMOTOR_6K ) {
			sprintf( command, "%d_!%dTIN.%d",
				compumotor_dinput->controller_number,
				compumotor_dinput->brick_number,
				compumotor_dinput->first_bit );
		} else {
			sprintf( command, "%d_!TIN.%d",
				compumotor_dinput->controller_number,
				compumotor_dinput->first_bit );
		}
	} else {
		if ( controller_type == MXT_COMPUMOTOR_6K ) {
			sprintf( command, "%d_!%dTIN",
				compumotor_dinput->controller_number,
				compumotor_dinput->brick_number );
		} else {
			sprintf( command, "%d_!TIN",
				compumotor_dinput->controller_number );
		}
	}

	mx_status = mxi_compumotor_command( compumotor_interface, command,
				response, sizeof( response ),
				MXD_COMPUMOTOR_DIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( compumotor_dinput->num_bits == 1 ) {
		num_items = sscanf( response, "%ld", &(dinput->value) );

		if ( num_items != 1 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Compumotor input status bit not found in response '%s' for "
		"Compumotor digital input record '%s'.",
				response, dinput->record->name );
		}
	} else {

		dinput->value = 0;

		/* Read out the requested bits.  The response from the
		 * Compumotor controller includes underscore '_' characters
		 * that split the bits up into groups of four.  We must be
		 * careful to step over these underscore characters.
		 */

		for ( i = 0; i < compumotor_dinput->num_bits; i++ ) {

			j = i + compumotor_dinput->first_bit - 1;

			j += 4*(j/4);	/* Skip over the underscores. */

			switch ( response[j] ) {
			case '1':
				dinput->value |= ( 1 << i );
				break;
			case '0':
				break;
			default:
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Character %d in response '%s' is not a '0' "
				"or '1'.  Instead it is a '%c' character.",
					j, response, response[j] );
				break;
			}
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_compumotor_dout_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_dout_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_compumotor_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_COMPUMOTOR_DOUTPUT *compumotor_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        compumotor_doutput = (MX_COMPUMOTOR_DOUTPUT *)
				malloc( sizeof(MX_COMPUMOTOR_DOUTPUT) );

        if ( compumotor_doutput == (MX_COMPUMOTOR_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_COMPUMOTOR_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = compumotor_doutput;
        record->class_specific_function_list
			= &mxd_compumotor_dout_digital_output_function_list;

        digital_output->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_dout_finish_record_initialization( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_dout_delete_record( MX_RECORD *record )
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
mxd_compumotor_dout_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_dout_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_dout_open( MX_RECORD *record )
{
	const char fname[] = "mxd_compumotor_dout_open()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_COMPUMOTOR_DOUTPUT *compumotor_doutput;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	doutput = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	mx_status = mxd_compumotor_dout_get_pointers(
						doutput, &compumotor_doutput,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_compumotor_get_controller_index( compumotor_interface,
				compumotor_doutput->controller_number,
				&(compumotor_doutput->controller_index));

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the current value of the register. */

	mx_status = mxd_compumotor_dout_read( doutput );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_compumotor_dout_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_dout_read( MX_DIGITAL_OUTPUT *doutput )
{
	const char fname[] = "mxd_compumotor_dout_read()";

	MX_COMPUMOTOR_DOUTPUT *compumotor_doutput;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	char command[80];
	char response[200];
	int i, j, num_items;
	unsigned long controller_type;
	mx_status_type mx_status;

	mx_status = mxd_compumotor_dout_get_pointers(doutput,
						&compumotor_doutput,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	controller_type = compumotor_interface->controller_type[
					compumotor_doutput->controller_index ];

	if ( compumotor_doutput->num_bits == 1 ) {

		if ( controller_type == MXT_COMPUMOTOR_6K ) {
			sprintf( command, "%d_!%dTOUT.%d",
				compumotor_doutput->controller_number,
				compumotor_doutput->brick_number,
				compumotor_doutput->first_bit );
		} else {
			sprintf( command, "%d_!TOUT.%d",
				compumotor_doutput->controller_number,
				compumotor_doutput->first_bit );
		}
	} else {
		if ( controller_type == MXT_COMPUMOTOR_6K ) {
			sprintf( command, "%d_!%dTOUT",
				compumotor_doutput->controller_number,
				compumotor_doutput->brick_number );
		} else {
			sprintf( command, "%d_!TOUT",
				compumotor_doutput->controller_number );
		}
	}

	mx_status = mxi_compumotor_command( compumotor_interface, command,
				response, sizeof( response ),
				MXD_COMPUMOTOR_DIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( compumotor_doutput->num_bits == 1 ) {
		num_items = sscanf( response, "%ld", &(doutput->value) );

		if ( num_items != 1 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Compumotor output status bit not found in response '%s' for "
		"Compumotor digital output record '%s'.",
				response, doutput->record->name );
		}
	} else {

		doutput->value = 0;

		/* Read out the requested bits.  The response from the
		 * Compumotor controller includes underscore '_' characters
		 * that split the bits up into groups of four.  We must be
		 * careful to step over these underscore characters.
		 */

		for ( i = 0; i < compumotor_doutput->num_bits; i++ ) {

			j = i + compumotor_doutput->first_bit - 1;

			j += 4*(j/4);	/* Skip over the underscores. */

			switch ( response[j] ) {
			case '1':
				doutput->value |= ( 1 << i );
				break;
			case '0':
				break;
			default:
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Character %d in response '%s' is not a '0' "
				"or '1'.  Instead it is a '%c' character.",
					j, response, response[j] );
				break;
			}
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_dout_write( MX_DIGITAL_OUTPUT *doutput )
{
	const char fname[] = "mxd_compumotor_dout_write()";

	MX_COMPUMOTOR_DOUTPUT *compumotor_doutput;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	char command[200];
	int i, first_offset, last_offset;
	unsigned long value, controller_type;
	size_t buffer_left;
	mx_status_type mx_status;

	mx_status = mxd_compumotor_dout_get_pointers( doutput,
					&compumotor_doutput,
					&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	value = doutput->value;

	controller_type = compumotor_interface->controller_type[
					compumotor_doutput->controller_index ];

	if ( controller_type == MXT_COMPUMOTOR_6K ) {
		sprintf( command, "%d_!%dOUT",
			compumotor_doutput->controller_number,
			compumotor_doutput->brick_number );
	} else {
		sprintf( command, "%d_!OUT",
			compumotor_doutput->controller_number );
	}

	first_offset = compumotor_doutput->first_bit - 1;

	last_offset = first_offset + compumotor_doutput->num_bits - 1;

	/* Leave the leading bits alone. */

	buffer_left = sizeof(command) - strlen(command);

	for ( i = 0; i < first_offset; i++ ) {

		strncat( command, "X", buffer_left );

		buffer_left--;
	}

	/* Copy the bits from the digital output device to the command. */

	for ( i = first_offset; i <= last_offset; i++ ) {

		if ( value & 0x1 ) {
			strncat( command, "1", buffer_left );
		} else {
			strncat( command, "0", buffer_left );
		}

		buffer_left--;

		value >>= 1;
	}

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, MXD_COMPUMOTOR_DIO_DEBUG );

	return mx_status;
}

