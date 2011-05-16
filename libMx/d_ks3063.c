/*
 * Name:    d_ks3063.c
 *
 * Purpose: MX input and output drivers to control KS3063 16-bit input 
 *          gate/output registers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006, 2010-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_driver.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "mx_camac.h"
#include "d_ks3063.h"

/* Initialize the KS3063 driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_ks3063_in_record_function_list = {
	NULL,
	mxd_ks3063_in_create_record_structures,
	mxd_ks3063_in_finish_record_initialization,
	NULL,
	mxd_ks3063_in_print_structure
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_ks3063_in_digital_input_function_list = {
	mxd_ks3063_in_read
};

MX_RECORD_FIELD_DEFAULTS mxd_ks3063_in_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_KS3063_IN_STANDARD_FIELDS
};

long mxd_ks3063_in_num_record_fields
		= sizeof( mxd_ks3063_in_record_field_defaults )
			/ sizeof( mxd_ks3063_in_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ks3063_in_rfield_def_ptr
			= &mxd_ks3063_in_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_ks3063_out_record_function_list = {
	NULL,
	mxd_ks3063_out_create_record_structures,
	mxd_ks3063_out_finish_record_initialization,
	NULL,
	mxd_ks3063_out_print_structure,
	mxd_ks3063_out_open
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_ks3063_out_digital_output_function_list = {
	mxd_ks3063_out_read,
	mxd_ks3063_out_write
};

MX_RECORD_FIELD_DEFAULTS mxd_ks3063_out_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_KS3063_OUT_STANDARD_FIELDS
};

long mxd_ks3063_out_num_record_fields
		= sizeof( mxd_ks3063_out_record_field_defaults )
			/ sizeof( mxd_ks3063_out_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ks3063_out_rfield_def_ptr
			= &mxd_ks3063_out_record_field_defaults[0];

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_ks3063_in_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_ks3063_in_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_KS3063_IN *ks3063_in;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        ks3063_in = (MX_KS3063_IN *) malloc( sizeof(MX_KS3063_IN) );

        if ( ks3063_in == (MX_KS3063_IN *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_KS3063_IN structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = ks3063_in;
        record->class_specific_function_list
                                = &mxd_ks3063_in_digital_input_function_list;

        digital_input->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3063_in_finish_record_initialization( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_ks3063_in_finish_record_initialization()";

        MX_KS3063_IN *ks3063_in;

        ks3063_in = (MX_KS3063_IN *) record->record_type_struct;

        if ( ks3063_in == (MX_KS3063_IN *) NULL ) { 
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                	"MX_KS3063_IN pointer for record '%s' is NULL.",
			record->name);
        }

        /* Verify that 'camac_record' is the correct type of record. */

        if ( ks3063_in->camac_record->mx_superclass != MXR_INTERFACE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
	                "'%s' is not an interface record.",
			ks3063_in->camac_record->name );
        }
        if ( ks3063_in->camac_record->mx_class != MXI_CAMAC ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not a CAMAC crate.",
			ks3063_in->camac_record->name );
        }

        /* Check the CAMAC slot number. */

        if ( ks3063_in->slot < 1 || ks3063_in->slot > 23 ) {
                return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
                "CAMAC slot number %ld is out of the allowed range 1-23.",
                        ks3063_in->slot );
        }

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3063_in_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_ks3063_in_print_structure()";

	MX_KS3063_IN *ks3063_in;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	ks3063_in = (MX_KS3063_IN *) record->record_type_struct;

	if ( ks3063_in == (MX_KS3063_IN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_KS3063_IN pointer for record '%s' is NULL.", record->name);
	}

	fprintf(file, "DIGITAL_INPUT parameters for digital input '%s':\n",
			record->name);
	fprintf(file, "  Digital input type = KS3063_IN.\n\n");

	fprintf(file, "  name       = %s\n", record->name);
	fprintf(file, "  crate      = %s\n", ks3063_in->camac_record->name);
	fprintf(file, "  slot       = %ld\n", ks3063_in->slot);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3063_in_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_ks3063_in_read()";

	MX_KS3063_IN *ks3063_in;
	int32_t data;
	int camac_Q, camac_X;

	ks3063_in = (MX_KS3063_IN *) (dinput->record->record_type_struct);

	if ( ks3063_in == (MX_KS3063_IN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_KS3063_IN pointer is NULL.");
	}

	mx_camac( (ks3063_in->camac_record), (ks3063_in->slot), 0, 0,
		&data, &camac_Q, &camac_X );

	dinput->value = (long) data;

	if ( camac_Q == 0 || camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"CAMAC error: Q = %d, X = %d", camac_Q, camac_X );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_ks3063_out_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_ks3063_out_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_KS3063_OUT *ks3063_out;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        ks3063_out = (MX_KS3063_OUT *) malloc( sizeof(MX_KS3063_OUT) );

        if ( ks3063_out == (MX_KS3063_OUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_KS3063_OUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = ks3063_out;
        record->class_specific_function_list
                                = &mxd_ks3063_out_digital_output_function_list;

        digital_output->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3063_out_finish_record_initialization( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_ks3063_out_finish_record_initialization()";

        MX_KS3063_OUT *ks3063_out;

        ks3063_out = (MX_KS3063_OUT *) record->record_type_struct;

        if ( ks3063_out == (MX_KS3063_OUT *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                	"MX_KS3063_OUT pointer for record '%s' is NULL.",
			record->name);
        }

        /* Verify that 'camac_record' is the correct type of record. */

        if ( ks3063_out->camac_record->mx_superclass != MXR_INTERFACE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
	                "'%s' is not an interface record.",
			ks3063_out->camac_record->name );
        }
        if ( ks3063_out->camac_record->mx_class != MXI_CAMAC ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not a CAMAC crate.",
			ks3063_out->camac_record->name );
        }

        /* Check the CAMAC slot number. */

        if ( ks3063_out->slot < 1 || ks3063_out->slot > 23 ) {
                return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
                "CAMAC slot number %ld is out of the allowed range 1-23.",
                        ks3063_out->slot );
        }

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3063_out_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_ks3063_out_print_structure()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_KS3063_OUT *ks3063_out;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	doutput = (MX_DIGITAL_OUTPUT *) (record->record_class_struct);

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_DIGITAL_OUTPUT pointer for record '%s' is NULL.", record->name);
	}

	ks3063_out = (MX_KS3063_OUT *) record->record_type_struct;

	if ( ks3063_out == (MX_KS3063_OUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_KS3063_OUT pointer for record '%s' is NULL.", record->name);
	}

	fprintf(file, "DIGITAL_OUTPUT parameters for digital output '%s':\n",
			record->name);
	fprintf(file, "  Digital output type = KS3063_OUT.\n\n");

	fprintf(file, "  value      = %lu\n", doutput->value);
	fprintf(file, "  name       = %s\n", record->name);
	fprintf(file, "  crate      = %s\n", ks3063_out->camac_record->name);
	fprintf(file, "  slot       = %ld\n", ks3063_out->slot);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3063_out_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_ks3063_out_open()";

	MX_DIGITAL_OUTPUT *doutput;
	mx_status_type mx_status;

	doutput = (MX_DIGITAL_OUTPUT *) (record->record_class_struct);

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_DIGITAL_OUTPUT pointer is NULL.");
	}

	mx_status = mxd_ks3063_out_write( doutput );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_ks3063_out_read( MX_DIGITAL_OUTPUT *doutput )
{
	/* It is not possible to read the value currently being output
	 * by the Kinetic Systems 3063 module.  Thus, all we can do is
	 * return the value that was previously written to it.  As long
	 * as we leave the 'doutput->value' field alone, the calling
	 * routine 'mx_digital_output_read()' will automatically do
	 * this for us.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3063_out_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_ks3063_out_write()";

	MX_KS3063_OUT *ks3063_out;
	int32_t data;
	int camac_Q, camac_X;

	ks3063_out = (MX_KS3063_OUT *) (doutput->record->record_type_struct);

	if ( ks3063_out == (MX_KS3063_OUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_KS3063_OUT pointer is NULL.");
	}

	data = (int32_t) doutput->value;

	mx_camac( (ks3063_out->camac_record), (ks3063_out->slot), 1, 16,
		&data, &camac_Q, &camac_X );

	if ( camac_Q == 0 || camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"CAMAC error: Q = %d, X = %d", camac_Q, camac_X );
	}

	return MX_SUCCESSFUL_RESULT;
}

