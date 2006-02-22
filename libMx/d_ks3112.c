/*
 * Name:    d_ks3112.c
 *
 * Purpose: MX output driver to control KS3112 12-bit DAC modules.
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
#include <math.h>

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_driver.h"
#include "mx_camac.h"
#include "d_ks3112.h"

/* Initialize the KS3112 driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_ks3112_record_function_list = {
	NULL,
	mxd_ks3112_create_record_structures,
	mxd_ks3112_finish_record_initialization,
	NULL,
	mxd_ks3112_print_structure,
	NULL,
	NULL,
	mxd_ks3112_open
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_ks3112_analog_output_function_list = {
	mxd_ks3112_read,
	mxd_ks3112_write
};

MX_RECORD_FIELD_DEFAULTS mxd_ks3112_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_LONG_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_KS3112_STANDARD_FIELDS
};

long mxd_ks3112_num_record_fields
		= sizeof( mxd_ks3112_record_field_defaults )
			/ sizeof( mxd_ks3112_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ks3112_rfield_def_ptr
			= &mxd_ks3112_record_field_defaults[0];

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_ks3112_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_ks3112_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_KS3112 *ks3112;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *) malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        ks3112 = (MX_KS3112 *) malloc( sizeof(MX_KS3112) );

        if ( ks3112 == (MX_KS3112 *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_KS3112 structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = ks3112;
        record->class_specific_function_list
                                = &mxd_ks3112_analog_output_function_list;

        analog_output->record = record;

	/* Raw analog output values are stored as longs. */

	analog_output->subclass = MXT_AOU_LONG;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3112_finish_record_initialization( MX_RECORD *record )
{
        static const char fname[] = "mxd_ks3112_finish_record_initialization()";

        MX_KS3112 *ks3112;

        ks3112 = (MX_KS3112 *) record->record_type_struct;

        if ( ks3112 == (MX_KS3112 *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
                	"MX_KS3112 pointer for record '%s' is NULL.",
			record->name);
        }

        /* Verify that 'camac_record' is the correct type of record. */

        if ( ks3112->camac_record->mx_superclass != MXR_INTERFACE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
	                "'%s' is not an interface record.",
			ks3112->camac_record->name );
        }
        if ( ks3112->camac_record->mx_class != MXI_CAMAC ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not a CAMAC crate.",
			ks3112->camac_record->name );
        }

        /* Check the CAMAC slot number. */

        if ( ks3112->slot < 1 || ks3112->slot > 23 ) {
                return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
                "CAMAC slot number %d is out of the allowed range 1-23.",
                        ks3112->slot );
        }

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3112_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_ks3112_print_structure()";

	MX_ANALOG_OUTPUT *dac;
	MX_KS3112 *ks3112;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	dac = (MX_ANALOG_OUTPUT *) (record->record_class_struct);

	if ( dac == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_ANALOG_OUTPUT pointer for record '%s' is NULL.", record->name);
	}

	ks3112 = (MX_KS3112 *) record->record_type_struct;

	if ( ks3112 == (MX_KS3112 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_KS3112 pointer for record '%s' is NULL.", record->name);
	}

	fprintf(file, "ANALOG_OUTPUT parameters for analog output '%s':\n",
			record->name);
	fprintf(file, "  Analog output type = KS3112_OUT.\n\n");

	fprintf(file, "  name       = %s\n", record->name);
	fprintf(file, "  value      = %ld (%g %s)\n",
				dac->raw_value.long_value,
				dac->value, dac->units);
	fprintf(file, "  crate      = %s\n", ks3112->camac_record->name);
	fprintf(file, "  slot       = %d\n", ks3112->slot);
	fprintf(file, "  subaddress = %d\n", ks3112->subaddress);
	fprintf(file, "  scale      = %g\n", dac->scale );
	fprintf(file, "  offset     = %g\n", dac->offset );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3112_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_ks3112_open()";

	MX_ANALOG_OUTPUT *dac;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	dac = (MX_ANALOG_OUTPUT *) (record->record_class_struct);

	if ( dac == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_ANALOG_OUTPUT pointer for record '%s' is NULL.", record->name);
	}

	dac->value = dac->offset + dac->scale * dac->raw_value.long_value;

	mx_status = mxd_ks3112_write( dac );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_ks3112_read( MX_ANALOG_OUTPUT *dac )
{
	/* The 3112 output register cannot be read from, so there is
	 * nothing we can do here.  However, the mx_analog_output_read()
	 * and mx_analog_output_read_raw() routines which are what normally
	 * call this routine, will do the right thing and use the value
	 * currently in dac->raw_value as if it were the value read
	 * from the hardware.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3112_write( MX_ANALOG_OUTPUT *dac )
{
	static const char fname[] = "mxd_ks3112_write()";

	MX_KS3112 *ks3112;
	int32_t data;
	int camac_Q, camac_X;

	ks3112 = (MX_KS3112 *) (dac->record->record_type_struct);

	if ( ks3112 == (MX_KS3112 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_KS3112 pointer is NULL.");
	}

	data = (int32_t) dac->raw_value.long_value;

	mx_camac( (ks3112->camac_record),
		(ks3112->slot), (ks3112->subaddress), 16,
		&data, &camac_Q, &camac_X );

	dac->raw_value.long_value = data;

	if ( camac_Q == 0 || camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"CAMAC error: Q = %d, X = %d", camac_Q, camac_X );
	}

	return MX_SUCCESSFUL_RESULT;
}

