/*
 * Name:    d_lpt.c
 *
 * Purpose: MX input and output drivers to control LPT digital
 *          input/output registers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_driver.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "i_lpt.h"
#include "d_lpt.h"

/* Initialize the LPT driver tables. */

MX_RECORD_FUNCTION_LIST mxd_lpt_in_record_function_list = {
	NULL,
	mxd_lpt_in_create_record_structures,
	mxd_lpt_in_finish_record_initialization
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_lpt_in_digital_input_function_list = {
	mxd_lpt_in_read
};

MX_RECORD_FIELD_DEFAULTS mxd_lpt_in_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_LPT_IN_STANDARD_FIELDS
};

long mxd_lpt_in_num_record_fields
		= sizeof( mxd_lpt_in_record_field_defaults )
			/ sizeof( mxd_lpt_in_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_lpt_in_rfield_def_ptr
			= &mxd_lpt_in_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_lpt_out_record_function_list = {
	NULL,
	mxd_lpt_out_create_record_structures,
	mxd_lpt_out_finish_record_initialization
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_lpt_out_digital_output_function_list = {
	mxd_lpt_out_read,
	mxd_lpt_out_write
};

MX_RECORD_FIELD_DEFAULTS mxd_lpt_out_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_LPT_OUT_STANDARD_FIELDS
};

long mxd_lpt_out_num_record_fields
		= sizeof( mxd_lpt_out_record_field_defaults )
			/ sizeof( mxd_lpt_out_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_lpt_out_rfield_def_ptr
			= &mxd_lpt_out_record_field_defaults[0];

static mx_status_type
mxd_lpt_in_get_pointers( MX_RECORD *record, MX_LPT_IN **lpt_in,
				MX_LPT **lpt, const char *calling_fname )
{
	MX_RECORD *lpt_record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
			"NULL MX_RECORD pointer passed to us." );
	}

	if ( (lpt_in == NULL) || (lpt == NULL) ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
			"NULL pointer(s) passed as arguments." );
	}

	*lpt_in = (MX_LPT_IN *) record->record_type_struct;

	if ( *lpt_in == (MX_LPT_IN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"MX_LPT_IN pointer for record '%s' is NULL.", record->name );
	}

	lpt_record = (*lpt_in)->interface_record;

	if ( lpt_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"MX_LPT interface pointer for record '%s' is NULL.",
			record->name );
	}

	*lpt = (MX_LPT *) lpt_record->record_type_struct;

	if ( *lpt == (MX_LPT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"MX_LPT pointer for interface record '%s' is NULL.",
			lpt_record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_lpt_out_get_pointers( MX_RECORD *record, MX_LPT_OUT **lpt_out,
				MX_LPT **lpt, const char *calling_fname )
{
	MX_RECORD *lpt_record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
			"NULL MX_RECORD pointer passed to us." );
	}

	if ( (lpt_out == NULL) || (lpt == NULL) ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
			"NULL pointer(s) passed as arguments." );
	}

	*lpt_out = (MX_LPT_OUT *) record->record_type_struct;

	if ( *lpt_out == (MX_LPT_OUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"MX_LPT_OUT pointer for record '%s' is NULL.", record->name );
	}

	lpt_record = (*lpt_out)->interface_record;

	if ( lpt_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"MX_LPT interface pointer for record '%s' is NULL.",
			record->name );
	}

	*lpt = (MX_LPT *) lpt_record->record_type_struct;

	if ( *lpt == (MX_LPT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"MX_LPT pointer for interface record '%s' is NULL.",
			lpt_record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_lpt_in_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_lpt_in_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_LPT_IN *lpt_in;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        lpt_in = (MX_LPT_IN *) malloc( sizeof(MX_LPT_IN) );

        if ( lpt_in == (MX_LPT_IN *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_LPT_IN structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = lpt_in;
        record->class_specific_function_list
                                = &mxd_lpt_in_digital_input_function_list;

        digital_input->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_lpt_in_finish_record_initialization( MX_RECORD *record )
{
        static const char fname[] = "mxd_lpt_in_finish_record_initialization()";

        MX_LPT_IN *lpt_in;
	int i, length;

        lpt_in = (MX_LPT_IN *) record->record_type_struct;

        if ( lpt_in == (MX_LPT_IN *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
                	"MX_LPT_IN pointer for record '%s' is NULL.",
			record->name);
        }

        /* Verify that 'interface_record' is the correct type of record. */

        if ( lpt_in->interface_record->mx_superclass != MXR_INTERFACE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
	                "'%s' is not an interface record.",
			lpt_in->interface_record->name );
        }
        if ( lpt_in->interface_record->mx_class != MXI_CONTROLLER ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an LPT interface driver.",
			lpt_in->interface_record->name );
        }
        if ( lpt_in->interface_record->mx_type != MXI_CTRL_LPT ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an LPT interface driver.",
			lpt_in->interface_record->name );
        }

	/* Convert the port name to lower case. */

	length = (int) strlen( lpt_in->port_name );

	for ( i = 0; i < length; i++ ) {
		if ( isupper( (int) (lpt_in->port_name[i]) ) ) {
			lpt_in->port_name[i] =
				tolower( (int) (lpt_in->port_name[i]) );
		}
	}

        /* Check the port name. */

	if ( strcmp( lpt_in->port_name, "data" ) == 0 ) {
		lpt_in->port_number = MX_LPT_DATA_PORT;
	} else
	if ( strcmp( lpt_in->port_name, "status" ) == 0 ) {
		lpt_in->port_number = MX_LPT_STATUS_PORT;
	} else
	if ( strcmp( lpt_in->port_name, "control" ) == 0 ) {
		lpt_in->port_number = MX_LPT_CONTROL_PORT;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized port type '%s' was specified for record '%s'.",
			lpt_in->port_name, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_lpt_in_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_lpt_in_read()";

	MX_LPT_IN *lpt_in;
	MX_LPT *lpt;
	uint8_t value;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	lpt = NULL;
	lpt_in = NULL;

	mx_status = mxd_lpt_in_get_pointers( dinput->record,
						&lpt_in, &lpt, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_lpt_read_port( lpt, lpt_in->port_number, &value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dinput->value = (long) value;

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_lpt_out_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_lpt_out_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_LPT_OUT *lpt_out;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        lpt_out = (MX_LPT_OUT *) malloc( sizeof(MX_LPT_OUT) );

        if ( lpt_out == (MX_LPT_OUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_LPT_OUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = lpt_out;
        record->class_specific_function_list
                                = &mxd_lpt_out_digital_output_function_list;

        digital_output->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_lpt_out_finish_record_initialization( MX_RECORD *record )
{
        static const char fname[] = "mxd_lpt_out_finish_record_initialization()";

        MX_LPT_OUT *lpt_out;
	int i, length;

        lpt_out = (MX_LPT_OUT *) record->record_type_struct;

        if ( lpt_out == (MX_LPT_OUT *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
                	"MX_LPT_OUT pointer for record '%s' is NULL.",
			record->name);
        }

        /* Verify that 'interface_record' is the correct type of record. */

        if ( lpt_out->interface_record->mx_superclass != MXR_INTERFACE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
	                "'%s' is not an interface record.",
			lpt_out->interface_record->name );
        }
        if ( lpt_out->interface_record->mx_class != MXI_CONTROLLER ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an LPT interface driver.",
			lpt_out->interface_record->name );
        }
        if ( lpt_out->interface_record->mx_type != MXI_CTRL_LPT ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an LPT interface driver.",
			lpt_out->interface_record->name );
        }

	/* Convert the port name to lower case. */

	length = (int) strlen( lpt_out->port_name );

	for ( i = 0; i < length; i++ ) {
		if ( isupper( (int) (lpt_out->port_name[i]) ) ) {
			lpt_out->port_name[i] =
				tolower( (int) (lpt_out->port_name[i]) );
		}
	}

        /* Check the port name. */

	if ( strcmp( lpt_out->port_name, "data" ) == 0 ) {
		lpt_out->port_number = MX_LPT_DATA_PORT;
	} else
	if ( strcmp( lpt_out->port_name, "status" ) == 0 ) {
		lpt_out->port_number = MX_LPT_STATUS_PORT;
	} else
	if ( strcmp( lpt_out->port_name, "control" ) == 0 ) {
		lpt_out->port_number = MX_LPT_CONTROL_PORT;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized port type '%s' was specified for record '%s'.",
			lpt_out->port_name, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_lpt_out_read( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_lpt_out_read()";

	MX_LPT_OUT *lpt_out;
	MX_LPT *lpt;
	uint8_t value;
	mx_status_type mx_status;

	mx_status = mxd_lpt_out_get_pointers( doutput->record,
						&lpt_out, &lpt, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_lpt_read_port( lpt, lpt_out->port_number, &value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	doutput->value = (long) value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_lpt_out_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_lpt_out_write()";

	MX_LPT_OUT *lpt_out;
	MX_LPT *lpt;
	uint8_t value;
	mx_status_type mx_status;

	mx_status = mxd_lpt_out_get_pointers( doutput->record,
						&lpt_out, &lpt, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	value = (uint8_t) ( doutput->value & 0xff );

	mx_status = mxi_lpt_write_port( lpt, lpt_out->port_number, value );

	return mx_status;
}

