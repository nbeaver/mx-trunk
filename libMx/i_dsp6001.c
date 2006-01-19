/*
 * Name:    i_dsp6001.c
 *
 * Purpose: MX driver for the DSP Technology Model 6001/6002
 *          CAMAC crate controller for PC compatible machines.
 *
 *          This driver currently only works under Watcom C/386 under
 *          the Rational Systems DOS extender.
 *
 *          At present, there is no support for block mode CAMAC operations
 *          in this set of functions.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2004, 2006 Illinois Institute of Technology
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
#include "mx_camac.h"
#include "mx_portio.h"
#include "i_dsp6001.h"

MX_RECORD_FUNCTION_LIST mxi_dsp6001_record_function_list = {
	NULL,
	mxi_dsp6001_create_record_structures,
	mxi_dsp6001_finish_record_initialization
};

MX_CAMAC_FUNCTION_LIST mxi_dsp6001_camac_function_list = {
	mxi_dsp6001_get_lam_status,
	mxi_dsp6001_controller_command,
	mxi_dsp6001_camac
};

MX_RECORD_FIELD_DEFAULTS mxi_dsp6001_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_CAMAC_STANDARD_FIELDS,
	MXI_DSP6001_STANDARD_FIELDS
};

long mxi_dsp6001_num_record_fields
	= sizeof( mxi_dsp6001_record_field_defaults )
	/ sizeof( mxi_dsp6001_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_dsp6001_rfield_def_ptr
			= &mxi_dsp6001_record_field_defaults[0];

/* --- Static data and functions that are private to this driver. --- */

static int  pr_last_crate_used = -1;

static void
pr_camac_crate_select( MX_RECORD *portio_record,
				unsigned long base_address, int crate )
{
	uint8_t select_byte;

	select_byte = ( uint8_t ) ( crate - 1 );

	select_byte &= 0x03;            /* Only allow crates 1 - 4. */

	select_byte <<= 4;

	mx_portio_outp8( portio_record, base_address + 15, select_byte );
}

/* ------------------------------------------------------ */

MX_EXPORT mx_status_type
mxi_dsp6001_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxi_dsp6001_create_record_structures()";

	MX_CAMAC *crate;
	MX_DSP6001 *dsp6001;

	/* Allocate memory for the necessary structures. */

	crate = (MX_CAMAC *) malloc( sizeof(MX_CAMAC) );

	if ( crate == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_CAMAC structure." );
	}

	dsp6001 = (MX_DSP6001 *) malloc( sizeof(MX_DSP6001) );

	if ( dsp6001 == NULL ) {
		free(crate);

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_DSP6001 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = crate;
	record->record_type_struct = dsp6001;
	record->class_specific_function_list = &mxi_dsp6001_camac_function_list;

	crate->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_dsp6001_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] = "mxi_dsp6001_finish_record_initialization()";

	MX_CAMAC *crate;

	crate = (MX_CAMAC *) record->record_class_struct;

	if ( crate->crate_number < 1 || crate->crate_number > 4 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Crate number %ld is out of the allowed range of 1-4.",
			crate->crate_number);
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_dsp6001_get_lam_status( MX_CAMAC *crate, int *lam_status )
{
	MX_RECORD *portio_record;
	MX_DSP6001 *dsp6001;
	unsigned long base_address;
	int crate_number;
	uint8_t status;

	dsp6001 = (MX_DSP6001 *) crate->record->record_type_struct;

	portio_record = dsp6001->portio_record;
	base_address = dsp6001->base_address;

	crate_number = crate->crate_number;

	if ( crate_number != pr_last_crate_used ) {
		pr_camac_crate_select( portio_record,
				base_address, crate_number );
	}

	status = mx_portio_inp8( portio_record, base_address + 8 );

	status >>= 2;

	status &= 0x1F;         /* Mask off unwanted bits. */

	*lam_status = status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_dsp6001_controller_command(MX_CAMAC *crate, int command )
{
	MX_RECORD *portio_record;
	MX_DSP6001 *dsp6001;
	unsigned long base_address;
	int crate_number;
	uint8_t command_byte;

	dsp6001 = (MX_DSP6001 *) crate->record->record_type_struct;

	portio_record = dsp6001->portio_record;
	base_address = dsp6001->base_address;

	crate_number = crate->crate_number;

	if ( crate_number != pr_last_crate_used ) {
		pr_camac_crate_select( portio_record,
				base_address, crate_number );
	}

	command_byte = ( uint8_t ) ( command & 0xff );

	mx_portio_outp8( portio_record, base_address + 6, command_byte );

	/* If 'command' sets Z and/or C, do a dataway cycle. */

	command_byte &= ( MX_CAMAC_Z | MX_CAMAC_C );

	if ( command_byte != 0 ) {
		mx_portio_outp8( portio_record, base_address + 7, 0 );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_dsp6001_camac( MX_CAMAC *crate, int slot, int subaddress,
		int function_code, int32_t *data, int *Q, int *X)
{
	MX_RECORD *portio_record;
	MX_DSP6001 *dsp6001;
	unsigned long base_address;
	int crate_number, X_temp;
	uint8_t data_byte;
	int32_t data_value;

	dsp6001 = (MX_DSP6001 *) crate->record->record_type_struct;

	portio_record = dsp6001->portio_record;
	base_address = dsp6001->base_address;

	crate_number = crate->crate_number;

	if ( crate_number != pr_last_crate_used ) {
		pr_camac_crate_select( portio_record,
				base_address, crate_number );
	}

	/* Set NAF codes ( slot, subaddress, function code ) */

	mx_portio_outp8( portio_record,
			base_address + 3, (uint8_t) subaddress );

	mx_portio_outp8( portio_record,
			base_address + 4, (uint8_t) function_code );

	mx_portio_outp8( portio_record,
			base_address + 5, (uint8_t) slot );

	/* Load data to be written, if necessary. */

	if ( function_code >= 16 && function_code <= 23 ) {

		data_value = *data;
		data_byte = ( uint8_t ) ( data_value & 0xFF );
		mx_portio_outp8( portio_record,
					base_address + 2, data_byte );

		data_value >>= 8;
		data_byte = ( uint8_t ) ( data_value & 0xFF );
		mx_portio_outp8( portio_record,
					base_address + 1, data_byte );

		data_value >>= 8;
		data_byte = ( uint8_t ) ( data_value & 0xFF );
		mx_portio_outp8( portio_record,
					base_address + 0, data_byte );
	}

	/* Execute a CAMAC dataway cycle. */

	mx_portio_outp8( portio_record, base_address + 7, 0 );

	/* Get the Q and X responses from the cycle. */

	data_byte = mx_portio_inp8( portio_record, base_address + 8 );

	*Q = data_byte & MX_CAMAC_Q;

	data_byte = mx_portio_inp8( portio_record, base_address + 8 );

	X_temp = data_byte & MX_CAMAC_X;
	*X = X_temp >> 1;

	/* Abort now if X == 0. */

	if ( *X == 0 ) {
		*data = 0;      /* Can't trust any data returned. */

		/* We don't have enough information at this low level
		 * to evaluate whether or not "X == 0" is really
		 * an error or not.  There are known to be CAMAC modules
		 * that return "X == 0", even for some cases where they
		 * have done something valid.  Thus, we send back a
		 * successful completion code.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	/* Read any returned data. */

	if ( function_code >= 0 && function_code <= 7 ) {

		data_value = mx_portio_inp8(portio_record, base_address + 9);

		data_value <<= 8;
		data_value += mx_portio_inp8(portio_record, base_address + 10);

		data_value <<= 8;
		data_value += mx_portio_inp8(portio_record, base_address + 11);
	} else {
		data_value = 0;
	}

	*data = data_value;

	return MX_SUCCESSFUL_RESULT;
}

