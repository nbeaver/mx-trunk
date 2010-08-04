/*
 * Name:    d_pmac_dio.c
 *
 * Purpose: MX input and output drivers to control PMAC variables as if they
 *          were digital input/output registers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2003, 2005-2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PMAC_DIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "i_pmac.h"
#include "d_pmac_dio.h"

/* Initialize the PMAC digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_pmac_din_record_function_list = {
	mxd_pmac_din_initialize_type,
	mxd_pmac_din_create_record_structures,
	mxd_pmac_din_finish_record_initialization,
	mxd_pmac_din_delete_record,
	NULL,
	mxd_pmac_din_read_parms_from_hardware,
	mxd_pmac_din_write_parms_to_hardware,
	mxd_pmac_din_open,
	mxd_pmac_din_close
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_pmac_din_digital_input_function_list = {
	mxd_pmac_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_pmac_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_PMAC_DINPUT_STANDARD_FIELDS
};

long mxd_pmac_din_num_record_fields
		= sizeof( mxd_pmac_din_record_field_defaults )
			/ sizeof( mxd_pmac_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pmac_din_rfield_def_ptr
			= &mxd_pmac_din_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_pmac_dout_record_function_list = {
	mxd_pmac_dout_initialize_type,
	mxd_pmac_dout_create_record_structures,
	mxd_pmac_dout_finish_record_initialization,
	mxd_pmac_dout_delete_record,
	NULL,
	mxd_pmac_dout_read_parms_from_hardware,
	mxd_pmac_dout_write_parms_to_hardware,
	mxd_pmac_dout_open,
	mxd_pmac_dout_close
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_pmac_dout_digital_output_function_list = {
	mxd_pmac_dout_read,
	mxd_pmac_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_pmac_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_PMAC_DOUTPUT_STANDARD_FIELDS
};

long mxd_pmac_dout_num_record_fields
		= sizeof( mxd_pmac_dout_record_field_defaults )
			/ sizeof( mxd_pmac_dout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pmac_dout_rfield_def_ptr
			= &mxd_pmac_dout_record_field_defaults[0];

static mx_status_type
mxd_pmac_din_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_PMAC_DINPUT **pmac_dinput,
			MX_PMAC **pmac,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pmac_din_get_pointers()";

	MX_RECORD *pmac_record;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pmac_dinput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PMAC_DINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pmac == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PMAC pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dinput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pmac_dinput = (MX_PMAC_DINPUT *) dinput->record->record_type_struct;

	if ( *pmac_dinput == (MX_PMAC_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PMAC_DINPUT pointer for record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	pmac_record = (*pmac_dinput)->pmac_record;

	if ( pmac_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_PMAC pointer for PMAC digital input record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	if ( pmac_record->mx_type != MXI_CTRL_PMAC ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"pmac_record '%s' for PMAC digital input '%s' is not a PMAC record.  "
"Instead, it is a '%s' record.",
			pmac_record->name, dinput->record->name,
			mx_get_driver_name( pmac_record ) );
	}

	*pmac = (MX_PMAC *) pmac_record->record_type_struct;

	if ( *pmac == (MX_PMAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_PMAC pointer for PMAC record '%s' used by PMAC digital input "
	"record '%s' and passed by '%s' is NULL.",
			pmac_record->name,
			dinput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pmac_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_PMAC_DOUTPUT **pmac_doutput,
			MX_PMAC **pmac,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pmac_dout_get_pointers()";

	MX_RECORD *pmac_record;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pmac_doutput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PMAC_DOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pmac == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PMAC pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pmac_doutput = (MX_PMAC_DOUTPUT *) doutput->record->record_type_struct;

	if ( *pmac_doutput == (MX_PMAC_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PMAC_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	pmac_record = (*pmac_doutput)->pmac_record;

	if ( pmac_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_PMAC pointer for PMAC digital output record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	if ( pmac_record->mx_type != MXI_CTRL_PMAC ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"pmac_record '%s' for PMAC digital output '%s' is not a PMAC record.  "
"Instead, it is a '%s' record.",
			pmac_record->name, doutput->record->name,
			mx_get_driver_name( pmac_record ) );
	}

	*pmac = (MX_PMAC *) pmac_record->record_type_struct;

	if ( *pmac == (MX_PMAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_PMAC pointer for PMAC record '%s' used by PMAC digital output "
	"record '%s' and passed by '%s' is NULL.",
			pmac_record->name,
			doutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_pmac_din_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_din_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_pmac_din_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_PMAC_DINPUT *pmac_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        pmac_dinput = (MX_PMAC_DINPUT *) malloc( sizeof(MX_PMAC_DINPUT) );

        if ( pmac_dinput == (MX_PMAC_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_PMAC_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = pmac_dinput;
        record->class_specific_function_list
                                = &mxd_pmac_din_digital_input_function_list;

        digital_input->record = record;
	pmac_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_din_finish_record_initialization( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_din_delete_record( MX_RECORD *record )
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
mxd_pmac_din_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_din_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_din_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pmac_din_open()";

	MX_DIGITAL_INPUT *dinput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	dinput = (MX_DIGITAL_INPUT *) record->record_class_struct;

	/* Read the current value of the register. */

	mx_status = mxd_pmac_din_read( dinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_din_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_pmac_din_close()";

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

	mx_status = mxd_pmac_din_read( dinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_din_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_pmac_din_read()";

	MX_PMAC_DINPUT *pmac_dinput;
	MX_PMAC *pmac;
	char command[80];
	char response[80];
	int num_items;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	pmac = NULL;
	pmac_dinput = NULL;

	mx_status = mxd_pmac_din_get_pointers( dinput,
					&pmac_dinput, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pmac->num_cards > 1 ) {
		snprintf( command, sizeof(command),
			"@%lx%s",
			pmac_dinput->card_number,
			pmac_dinput->pmac_variable_name );
	} else {
		strlcpy( command,
			pmac_dinput->pmac_variable_name,
			sizeof(command) );
	}

	mx_status = mxi_pmac_command( pmac, command,
				response, sizeof( response ),
				MXD_PMAC_DIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lu", &(dinput->value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"PMAC variable value not found in response '%s' for "
		"PMAC digital input record '%s'.",
			response, dinput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_pmac_dout_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_dout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_pmac_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_PMAC_DOUTPUT *pmac_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        pmac_doutput = (MX_PMAC_DOUTPUT *) malloc( sizeof(MX_PMAC_DOUTPUT) );

        if ( pmac_doutput == (MX_PMAC_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_PMAC_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = pmac_doutput;
        record->class_specific_function_list
                                = &mxd_pmac_dout_digital_output_function_list;

        digital_output->record = record;
	pmac_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_dout_finish_record_initialization( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_dout_delete_record( MX_RECORD *record )
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
mxd_pmac_dout_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_dout_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_dout_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pmac_dout_open()";

	MX_DIGITAL_OUTPUT *doutput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	doutput = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	/* Read the current value of the register. */

	mx_status = mxd_pmac_dout_read( doutput );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_dout_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_dout_read( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_pmac_dout_read()";

	MX_PMAC_DOUTPUT *pmac_doutput;
	MX_PMAC *pmac;
	char command[80];
	char response[80];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_pmac_dout_get_pointers( doutput,
					&pmac_doutput, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pmac->num_cards > 1 ) {
		snprintf( command, sizeof(command),
			"@%lx%s",
			pmac_doutput->card_number,
			pmac_doutput->pmac_variable_name );
	} else {
		strlcpy( command,
			pmac_doutput->pmac_variable_name,
			sizeof(command) );
	}

	mx_status = mxi_pmac_command( pmac, command,
				response, sizeof( response ),
				MXD_PMAC_DIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lu", &(doutput->value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"PMAC variable value not found in response '%s' for "
		"PMAC digital output record '%s'.",
			response, doutput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_dout_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_pmac_dout_write()";

	MX_PMAC_DOUTPUT *pmac_doutput;
	MX_PMAC *pmac;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_pmac_dout_get_pointers( doutput,
					&pmac_doutput, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pmac->num_cards > 1 ) {
		snprintf( command, sizeof(command),
			"@%lx%s=%lu",
			pmac_doutput->card_number,
			pmac_doutput->pmac_variable_name,
			doutput->value );
	} else {
		snprintf( command, sizeof(command),
			"%s=%lu",
			pmac_doutput->pmac_variable_name,
			doutput->value );
	}

	mx_status = mxi_pmac_command( pmac, command,
					NULL, 0, MXD_PMAC_DIO_DEBUG );

	return mx_status;
}

