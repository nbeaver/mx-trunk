/*
 * Name:    d_ks3512.c
 *
 * Purpose: MX input driver to control KS3512 12-bit ADC.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004, 2006 Illinois Institute of Technology
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
#include "d_ks3512.h"

/* Initialize the KS3512 driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_ks3512_record_function_list = {
	NULL,
	mxd_ks3512_create_record_structures,
	mxd_ks3512_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_ks3512_analog_input_function_list = {
	mxd_ks3512_read
};

MX_RECORD_FIELD_DEFAULTS mxd_ks3512_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_INT32_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_KS3512_STANDARD_FIELDS
};

mx_length_type mxd_ks3512_num_record_fields
		= sizeof( mxd_ks3512_record_field_defaults )
			/ sizeof( mxd_ks3512_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ks3512_rfield_def_ptr
			= &mxd_ks3512_record_field_defaults[0];

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_ks3512_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_ks3512_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_KS3512 *ks3512;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        ks3512 = (MX_KS3512 *) malloc( sizeof(MX_KS3512) );

        if ( ks3512 == (MX_KS3512 *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_KS3512 structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = ks3512;
        record->class_specific_function_list
                                = &mxd_ks3512_analog_input_function_list;

        analog_input->record = record;

	/* Raw analog input values are stored as 32-bit integers. */

	analog_input->subclass = MXT_AIN_INT32;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3512_finish_record_initialization( MX_RECORD *record )
{
        static const char fname[] = "mxd_ks3512_finish_record_initialization()";

        MX_KS3512 *ks3512;
	mx_status_type mx_status;

        ks3512 = (MX_KS3512 *) record->record_type_struct;

        if ( ks3512 == (MX_KS3512 *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
                	"MX_KS3512 pointer for record '%s' is NULL.",
			record->name);
        }

        /* Verify that 'camac_record' is the correct type of record. */

        if ( ks3512->camac_record->mx_superclass != MXR_INTERFACE ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
	                "'%s' is not an interface record.",
			ks3512->camac_record->name );
        }
        if ( ks3512->camac_record->mx_class != MXI_CAMAC ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not a CAMAC crate.",
			ks3512->camac_record->name );
        }

        /* Check the CAMAC slot number. */

        if ( ks3512->slot < 1 || ks3512->slot > 23 ) {
                return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
                "CAMAC slot number %d is out of the allowed range 1-23.",
                        ks3512->slot );
        }

	mx_status = mx_analog_input_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_ks3512_read( MX_ANALOG_INPUT *adc )
{
	static const char fname[] = "mxd_ks3512_read()";

	MX_KS3512 *ks3512;
	int32_t camac_Q, camac_X, data;

	ks3512 = (MX_KS3512 *) (adc->record->record_type_struct);

	if ( ks3512 == (MX_KS3512 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_KS3512 pointer is NULL.");
	}

	mx_camac( (ks3512->camac_record),
		(ks3512->slot), (ks3512->subaddress), 0,
		&data, &camac_Q, &camac_X );

	adc->raw_value.int32_value = data;

	if ( camac_Q == 0 || camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"CAMAC error: Q = %d, X = %d", camac_Q, camac_X );
	}

	return MX_SUCCESSFUL_RESULT;
}

