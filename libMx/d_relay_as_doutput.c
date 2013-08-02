/*
 * Name:    d_relay_as_doutput.c
 *
 * Purpose: MX digital output driver for treating an MX relay as an
 *          MX digital output.  The driver commands the relay to open
 *          or close depending on the value of the digital output (0/1).
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_RELAY_AS_DOUTPUT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_relay.h"
#include "mx_digital_output.h"
#include "d_relay_as_doutput.h"

MX_RECORD_FUNCTION_LIST mxd_relay_as_doutput_record_function_list = {
	NULL,
	mxd_relay_as_doutput_create_record_structures
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
	mxd_relay_as_doutput_digital_output_function_list =
{
	mxd_relay_as_doutput_read,
	mxd_relay_as_doutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_relay_as_doutput_field_default[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_RELAY_AS_DOUTPUT_STANDARD_FIELDS
};

long mxd_relay_as_doutput_num_record_fields
		= sizeof( mxd_relay_as_doutput_field_default )
		/ sizeof( mxd_relay_as_doutput_field_default[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_relay_as_doutput_rfield_def_ptr
			= &mxd_relay_as_doutput_field_default[0];

/* ===== */

static mx_status_type
mxd_relay_as_doutput_get_pointers( MX_DIGITAL_OUTPUT *doutput,
				MX_RELAY_AS_DOUTPUT **relay_as_doutput,
				MX_RECORD **relay_record,
				const char *calling_fname )
{
	static const char fname[] = "mxd_relay_as_doutput_get_pointers()";

	MX_RELAY_AS_DOUTPUT *relay_as_doutput_ptr;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_OUTPUT pointer passed was NULL." );
	}

	relay_as_doutput_ptr = (MX_RELAY_AS_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( relay_as_doutput_ptr == (MX_RELAY_AS_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RELAY_AS_DOUTPUT pointer for digital output '%s' is NULL",
			doutput->record->name );
	}

	if ( relay_as_doutput != (MX_RELAY_AS_DOUTPUT **) NULL ) {
		*relay_as_doutput = relay_as_doutput_ptr;
	}

	if ( relay_record != (MX_RECORD **) NULL ) {
		*relay_record =
			relay_as_doutput_ptr->relay_record;

		if ( (*relay_record) == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The relay_record pointer for record '%s' is NULL.",
				doutput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_relay_as_doutput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_relay_as_doutput_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_RELAY_AS_DOUTPUT *relay_as_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
				malloc( sizeof(MX_DIGITAL_OUTPUT) );

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for an MX_DIGITAL_OUTPUT structure." );
        }

        relay_as_doutput = (MX_RELAY_AS_DOUTPUT *)
				malloc( sizeof(MX_RELAY_AS_DOUTPUT) );

        if ( relay_as_doutput == (MX_RELAY_AS_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
               "Can't allocate memory for MX_RELAY_AS_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = relay_as_doutput;
        record->class_specific_function_list
			= &mxd_relay_as_doutput_digital_output_function_list;

        digital_output->record = record;
	relay_as_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_relay_as_doutput_read( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_relay_as_doutput_read()";

	MX_RELAY_AS_DOUTPUT *relay_as_doutput = NULL;
	MX_RECORD *relay_record = NULL;
	long relay_status;
	mx_status_type mx_status;

	mx_status = mxd_relay_as_doutput_get_pointers( doutput,
						&relay_as_doutput,
						&relay_record,
						fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_relay_status( relay_record, &relay_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( relay_status ) {
	case MXF_RELAY_IS_CLOSED:
		if ( relay_as_doutput->use_inverted_logic ) {
			doutput->value = 0;
		} else {
			doutput->value = 1;
		}
		break;
	case MXF_RELAY_IS_OPEN:
		if ( relay_as_doutput->use_inverted_logic ) {
			doutput->value = 1;
		} else {
			doutput->value = 0;
		}
		break;
	default:
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unexpected relay status value %ld was returned by "
		"relay '%s' for digital output '%s'",
			relay_status,
			relay_record->name,
			doutput->record->name );
		break;
	}

#if MXD_RELAY_AS_DOUTPUT_DEBUG
	MX_DEBUG(-2,("%s: relay '%s' = %ld  ==>  doutput '%s' = %ld",
		fname, relay_record->name, relay_status,
		doutput->record->name, doutput->value));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_relay_as_doutput_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_relay_as_doutput_write()";

	MX_RELAY_AS_DOUTPUT *relay_as_doutput = NULL;
	MX_RECORD *relay_record = NULL;
	long relay_command;
	mx_status_type mx_status;

	mx_status = mxd_relay_as_doutput_get_pointers( doutput,
						&relay_as_doutput,
						&relay_record,
						fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( doutput->value == 0 ) {
		if ( relay_as_doutput->use_inverted_logic ) {
			relay_command = MXF_RELAY_IS_CLOSED;
		} else {
			relay_command = MXF_RELAY_IS_OPEN;
		}
	} else {
		if ( relay_as_doutput->use_inverted_logic ) {
			relay_command = MXF_RELAY_IS_OPEN;
		} else {
			relay_command = MXF_RELAY_IS_CLOSED;
		}
	}

#if MXD_RELAY_AS_DOUTPUT_DEBUG
	MX_DEBUG(-2,("%s: relay '%s' = %ld  <==  doutput '%s' = %ld",
		fname, relay_record->name, relay_command,
		doutput->record->name, doutput->value));
#endif
	mx_status = mx_relay_command( relay_record, relay_command );

	return mx_status;
}

