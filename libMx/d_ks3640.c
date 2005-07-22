/*
 * Name:    d_ks3640.c
 *
 * Purpose: MX encoder driver to control a Kinetic Systems model 3640 
 *          up/down counter.  The up/down counter is treated as an 
 *          incremental encoder.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_types.h"
#include "mx_driver.h"
#include "mx_encoder.h"
#include "mx_camac.h"
#include "d_ks3640.h"

/* Initialize the encoder driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_ks3640_record_function_list = {
	mxd_ks3640_initialize_type,
	mxd_ks3640_create_record_structures,
	mxd_ks3640_finish_record_initialization,
	mxd_ks3640_delete_record,
	NULL,
	mxd_ks3640_read_parms_from_hardware,
	mxd_ks3640_write_parms_to_hardware,
	mxd_ks3640_open,
	mxd_ks3640_close
};

MX_ENCODER_FUNCTION_LIST mxd_ks3640_encoder_function_list = {
	mxd_ks3640_get_overflow_status,
	mxd_ks3640_reset_overflow_status,
	mxd_ks3640_read,
	mxd_ks3640_write
};

MX_RECORD_FIELD_DEFAULTS mxd_ks3640_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ENCODER_STANDARD_FIELDS,
	MXD_KS3640_STANDARD_FIELDS
};

long mxd_ks3640_num_record_fields
		= sizeof( mxd_ks3640_record_field_defaults )
			/ sizeof( mxd_ks3640_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ks3640_rfield_def_ptr
			= &mxd_ks3640_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_ks3640_get_pointers( MX_ENCODER *encoder,
			MX_KS3640 **ks3640,
			const char *calling_fname )
{
	const char fname[] = "mxd_ks3640_get_pointers()";

	if ( encoder == (MX_ENCODER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_ENCODER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( ks3640 == (MX_KS3640 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KS3640 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( encoder->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_ENCODER pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*ks3640 = (MX_KS3640 *) encoder->record->record_type_struct;

	if ( *ks3640 == (MX_KS3640 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_KS3640 pointer for record '%s' is NULL.",
			encoder->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_ks3640_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3640_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_ks3640_create_record_structures()";

	MX_ENCODER *encoder;
	MX_KS3640 *ks3640;

	/* Allocate memory for the necessary structures. */

	encoder = (MX_ENCODER *) malloc( sizeof(MX_ENCODER) );

	if ( encoder == (MX_ENCODER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ENCODER structure." );
	}

	ks3640 = (MX_KS3640 *) malloc( sizeof(MX_KS3640) );

	if ( ks3640 == (MX_KS3640 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_KS3640 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = encoder;
	record->record_type_struct = ks3640;
	record->class_specific_function_list
				= &mxd_ks3640_encoder_function_list;

	encoder->record = record;

	/* Mark this as being an incremental encoder. */

	encoder->encoder_type = MXT_ENC_INCREMENTAL_ENCODER;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3640_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] = "mxd_ks3640_finish_record_initialization()";

	MX_KS3640 *ks3640;

	ks3640 = (MX_KS3640 *) record->record_type_struct;

	/* === CAMAC slot number === */

	if ( ks3640->slot < 1 || ks3640->slot > 23 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"CAMAC slot number %d is out of allowed range 1-23.",
			ks3640->slot );
	}

	/* === Encoder subaddress number === */

	if ( ks3640->subaddress < 0 || ks3640->subaddress > 3 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Subaddress number %d is out of allowed range 0-3.",
			ks3640->subaddress);
	}

	/* The 32 bit modification is a field modification of the 
	 * circuit board that combines the 4 16-bit channels into
	 * 2 32-bit channels.
	 */

	switch( ks3640->use_32bit_mod ) {
	case TRUE:
		if ( ks3640->subaddress == 1 || ks3640->subaddress == 3 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The 32-bit hardware modification requires that subaddresses be even numbers.");
		}
		break;
	case FALSE:
		/* Do nothing. */
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Illegal value %d for 32 bit hardware modification field.  Must be 0 or 1.",
			ks3640->use_32bit_mod );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3640_delete_record( MX_RECORD *record )
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
mxd_ks3640_read_parms_from_hardware( MX_RECORD *record )
{
	const char fname[] = "mxd_ks3640_read_parms_from_hardware()";

	MX_ENCODER *encoder;
	long current_value;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	encoder = (MX_ENCODER *)( record->record_class_struct );

	if ( encoder == (MX_ENCODER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ENCODER pointer passed is NULL." );
	}

	status = mx_encoder_read( record, &current_value );

	if ( status.code != MXE_SUCCESS ) {
		encoder->value = 0;
	} else {
		encoder->value = current_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3640_write_parms_to_hardware( MX_RECORD *record )
{
	const char fname[] = "mxd_ks3640_write_parms_to_hardware()";

	MX_ENCODER *encoder;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	encoder = (MX_ENCODER *)( record->record_class_struct );

	if ( encoder == (MX_ENCODER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ENCODER pointer passed is NULL." );
	}

	status = mx_encoder_write( record, encoder->value );

	return status;
}

MX_EXPORT mx_status_type
mxd_ks3640_open( MX_RECORD *record )
{
	const char fname[] = "mxd_ks3640_open()";

	MX_ENCODER *encoder;
	MX_KS3640 *ks3640;
	mx_sint32_type data;
	int camac_Q, camac_X;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	encoder = (MX_ENCODER *)( record->record_class_struct );

	mx_status = mxd_ks3640_get_pointers( encoder, &ks3640, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We must turn LAMs on in the module in order to be able to find
	 * out when the up/down counter has overflowed or underflowed.
	 * It doesn't hurt to do this more than once.
	 */

	mx_camac( ks3640->camac_record, ks3640->slot, 0, 26,
					&data, &camac_Q, &camac_X );

	if ( camac_Q != 1 || camac_X != 1 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	"Enabling LAMs for the CAMAC device '%s' failed. Q = %d, X = %d",
			record->name, camac_Q, camac_X );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3640_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3640_get_overflow_status( MX_ENCODER *encoder )
{
	const char fname[] = "mxd_ks3640_get_overflow_status()";

	MX_KS3640 *ks3640;
	mx_sint32_type data, status_bits;
	mx_sint32_type underflow_set_temp, overflow_set_temp;
	int N, A;
	int camac_Q, camac_X;
	mx_status_type mx_status;

	mx_status = mxd_ks3640_get_pointers( encoder, &ks3640, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	N = ks3640->slot;
	A = ks3640->subaddress;

	/* Are any of the overflow/underflow LAM bits set? */

	mx_camac( ks3640->camac_record, N, 15, 8, &data, &camac_Q, &camac_X );

	if ( camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"CAMAC error testing whether LAM is set for device '%s': Q = %d, X = %d",
			encoder->record->name, camac_Q, camac_X );
	}

	/* If no bits are set (Q = 0), we need do nothing more. */

	if ( camac_Q == 0 ) {
		encoder->underflow_set = FALSE;
		encoder->overflow_set = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Otherwise, get the LAM status bits for all the subaddresses. */

	mx_camac( ks3640->camac_record, N, 12, 1,
					&status_bits, &camac_Q, &camac_X);

	if ( camac_Q == 0 || camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"CAMAC error getting LAM status bits for device '%s': Q = %d, X = %d",
			encoder->record->name, camac_Q, camac_X );
	}

	underflow_set_temp = status_bits >> (2*A);
	underflow_set_temp &= 0x1;

	if ( underflow_set_temp ) {
		encoder->underflow_set = TRUE;
	} else {
		encoder->underflow_set = FALSE;
	}

	overflow_set_temp = status_bits >> (2*A+1);
	overflow_set_temp &= 0x1;

	if ( overflow_set_temp ) {
		encoder->overflow_set = TRUE;
	} else {
		encoder->overflow_set = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3640_reset_overflow_status( MX_ENCODER *encoder )
{
	const char fname[] = "mxd_ks3640_reset_overflow_status()";

	MX_KS3640 *ks3640;
	mx_sint32_type data;
	int N, A;
	int camac_Q, camac_X;
	mx_status_type mx_status;

	mx_status = mxd_ks3640_get_pointers( encoder, &ks3640, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	N = ks3640->slot;
	A = ks3640->subaddress;

	/* Reset the underflow bit. */

	mx_camac( ks3640->camac_record, N, 2*A, 10,
						&data, &camac_Q, &camac_X );

	if ( camac_Q == 0 || camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"CAMAC error resetting underflow for device '%s': Q = %d, X = %d",
			encoder->record->name, camac_Q, camac_X );
	}

	/* Reset the overflow bit. */

	mx_camac( ks3640->camac_record, N, 2*A+1, 10,
						&data, &camac_Q, &camac_X );

	if ( camac_Q == 0 || camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"CAMAC error resetting overflow for device '%s': Q = %d, X = %d",
			encoder->record->name, camac_Q, camac_X );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ks3640_read( MX_ENCODER *encoder )
{
	const char fname[] = "mxd_ks3640_read()";

	MX_KS3640 *ks3640;
	mx_sint32_type data;
	int camac_Q, camac_X;
	mx_status_type mx_status;

	mx_status = mxd_ks3640_get_pointers( encoder, &ks3640, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_camac( ks3640->camac_record,
		ks3640->slot, ks3640->subaddress, 0,
		&data, &camac_Q, &camac_X );

	encoder->value = (long) data;

	if ( camac_Q == 0 || camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"CAMAC error: Q = %d, X = %d", camac_Q, camac_X );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

MX_EXPORT mx_status_type
mxd_ks3640_write( MX_ENCODER *encoder )
{
	const char fname[] = "mxd_ks3640_write()";

	MX_KS3640 *ks3640;
	mx_sint32_type data;
	int camac_Q, camac_X;
	mx_status_type mx_status;

	mx_status = mxd_ks3640_get_pointers( encoder, &ks3640, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	data = (mx_sint32_type) encoder->value;

	mx_camac( ks3640->camac_record,
		ks3640->slot, ks3640->subaddress, 16,
		&data, &camac_Q, &camac_X );

	if ( camac_Q == 0 || camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"CAMAC error: Q = %d, X = %d", camac_Q, camac_X );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

