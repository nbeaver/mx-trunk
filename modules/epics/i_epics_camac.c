/*
 * Name:    i_epics_camac.c
 *
 * Purpose: MX driver for the EPICS CAMAC record.
 *
 *          At present, there is no support for block mode CAMAC operations
 *          in this set of functions.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
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
#include "mx_epics.h"
#include "mx_camac.h"
#include "i_epics_camac.h"

MX_RECORD_FUNCTION_LIST mxi_epics_camac_record_function_list = {
	NULL,
	mxi_epics_camac_create_record_structures,
	mxi_epics_camac_finish_record_initialization
};

MX_CAMAC_FUNCTION_LIST mxi_epics_camac_camac_function_list = {
	mxi_epics_camac_get_lam_status,
	mxi_epics_camac_controller_command,
	mxi_epics_camac
};

MX_RECORD_FIELD_DEFAULTS mxi_epics_camac_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_CAMAC_STANDARD_FIELDS,
	MXI_EPICS_CAMAC_STANDARD_FIELDS
};

long mxi_epics_camac_num_record_fields
	= sizeof( mxi_epics_camac_record_field_defaults )
	/ sizeof( mxi_epics_camac_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_epics_camac_rfield_def_ptr
			= &mxi_epics_camac_record_field_defaults[0];

/* ---- */

static mx_status_type
mxi_epics_camac_get_pointers( MX_CAMAC *camac,
			MX_EPICS_CAMAC **epics_camac,
			const char *calling_fname )
{
	static const char fname[] = "mxi_epics_camac_get_pointers()";

	MX_RECORD *epics_camac_record;

	if ( camac == (MX_CAMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_CAMAC pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( epics_camac == (MX_EPICS_CAMAC **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_CAMAC pointer passed by '%s' was NULL.",
			calling_fname );
	}

	epics_camac_record = camac->record;

	if ( epics_camac_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The epics_camac_record pointer for the "
			"MX_CAMAC pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*epics_camac = (MX_EPICS_CAMAC *)
			epics_camac_record->record_type_struct;

	if ( *epics_camac == (MX_EPICS_CAMAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPICS_CAMAC pointer for record '%s' is NULL.",
			epics_camac_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ------------------------------------------------------ */

MX_EXPORT mx_status_type
mxi_epics_camac_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_epics_camac_create_record_structures()";

	MX_CAMAC *camac;
	MX_EPICS_CAMAC *epics_camac;

	/* Allocate memory for the necessary structures. */

	camac = (MX_CAMAC *) malloc( sizeof(MX_CAMAC) );

	if ( camac == (MX_CAMAC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory allocating an MX_CAMAC structure." );
	}

	epics_camac = (MX_EPICS_CAMAC *) malloc( sizeof(MX_EPICS_CAMAC) );

	if ( epics_camac == (MX_EPICS_CAMAC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory allocating an MX_EPICS_CAMAC structure." );
	}

	record->record_class_struct = camac;
	record->record_type_struct = epics_camac;
	record->class_specific_function_list =
			&mxi_epics_camac_camac_function_list;

	camac->record = record;
	epics_camac->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_camac_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_epics_camac_finish_record_initialization()";

	MX_CAMAC *camac = NULL;
	MX_EPICS_CAMAC *epics_camac = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	camac = (MX_CAMAC *) record->record_class_struct;

	mx_status = mxi_epics_camac_get_pointers( camac, &epics_camac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize MX EPICS data structures. */

	mx_epics_pvname_init( &(epics_camac->a_pv),
				"%s.A", epics_camac->epics_record_name );

	mx_epics_pvname_init( &(epics_camac->b_pv),
				"%s.B", epics_camac->epics_record_name );

	mx_epics_pvname_init( &(epics_camac->c_pv),
				"%s.C", epics_camac->epics_record_name );

	mx_epics_pvname_init( &(epics_camac->ccmd_pv),
				"%s.CCMD", epics_camac->epics_record_name );

	mx_epics_pvname_init( &(epics_camac->f_pv),
				"%s.F", epics_camac->epics_record_name );

	mx_epics_pvname_init( &(epics_camac->n_pv),
				"%s.N", epics_camac->epics_record_name );

	mx_epics_pvname_init( &(epics_camac->q_pv),
				"%s.Q", epics_camac->epics_record_name );

	mx_epics_pvname_init( &(epics_camac->tmod_pv),
				"%s.TMOD", epics_camac->epics_record_name );

	mx_epics_pvname_init( &(epics_camac->val_pv),
				"%s.VAL", epics_camac->epics_record_name );

	mx_epics_pvname_init( &(epics_camac->x_pv),
				"%s.X", epics_camac->epics_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_camac_get_lam_status( MX_CAMAC *camac, long *lam_status )
{
	static const char fname[] = "mxi_epics_camac_get_lam_status()";

	MX_EPICS_CAMAC *epics_camac = NULL;
	int32_t lam_data;
	int Q, X;
	mx_status_type mx_status;

	mx_status = mxi_epics_camac_get_pointers( camac, &epics_camac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Use function code 8 to get the LAM status. */

	mx_status = mxi_epics_camac( camac, 25, 0, 8, &lam_data, &Q, &X );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( lam_status != (long *) NULL ) {
		*lam_status = (long) lam_data;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_camac_controller_command( MX_CAMAC *camac, long command )
{
	static const char fname[] = "mxi_epics_camac_controller_command()";

	MX_EPICS_CAMAC *epics_camac = NULL;
	long ccmd_value;
	mx_status_type mx_status;

	mx_status = mxi_epics_camac_get_pointers( camac, &epics_camac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( command & MX_CAMAC_Z ) {
		ccmd_value = 4;		/* Initialize (Z) */

		mx_status = mx_caput( &(epics_camac->ccmd_pv), 
				1, MX_CA_LONG, &ccmd_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	if ( command & MX_CAMAC_C ) {
		ccmd_value = 3;		/* Clear (C) */

		mx_status = mx_caput( &(epics_camac->ccmd_pv), 
				1, MX_CA_LONG, &ccmd_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	if ( command & MX_CAMAC_I ) {
		ccmd_value = 2;		/* Set Inhibit */

		mx_status = mx_caput( &(epics_camac->ccmd_pv), 
				1, MX_CA_LONG, &ccmd_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else {
		ccmd_value = 1;		/* Clear Inhibit */

		mx_status = mx_caput( &(epics_camac->ccmd_pv), 
				1, MX_CA_LONG, &ccmd_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_camac( MX_CAMAC *camac, long slot, long subaddress,
		long function_code, int32_t *data, int *Q, int *X)
{
	static const char fname[] = "mxi_epics_camac()";

	MX_EPICS_CAMAC *epics_camac = NULL;
	long branch_number, crate_number;
	long tmod_value, long_data, q_response, x_response;
	mx_status_type mx_status;

	mx_status = mxi_epics_camac_get_pointers( camac, &epics_camac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( data == (int32_t *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The data pointer passed was NULL." );
	}

	/* Select Single transfer mode. */

	tmod_value = 0;

	mx_status = mx_caput( &(epics_camac->tmod_pv),
				1, MX_CA_LONG, &tmod_value );

	if ( mx_status.code != MXE_SUCCESS );
		return mx_status;

	/* Select the Branch. */

	branch_number = camac->branch_number;

	mx_status = mx_caput( &(epics_camac->b_pv),
				1, MX_CA_LONG, &branch_number );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Select the Crate. */

	crate_number = camac->crate_number;

	mx_status = mx_caput( &(epics_camac->c_pv),
				1, MX_CA_LONG, &crate_number );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Select the Slot. */

	mx_status = mx_caput( &(epics_camac->n_pv), 1, MX_CA_LONG, &slot );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Select the Subaddress. */

	mx_status = mx_caput( &(epics_camac->a_pv), 1, MX_CA_LONG, &subaddress);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If this is a write command or a control command, then we must
	 * first send the data value to the crate.
	 */

	if ( function_code >= 8 ) {
		long_data = (long) (*data);

		mx_status = mx_caput( &(epics_camac->val_pv),
				1, MX_CA_LONG, &long_data );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Send the function code to the crate. */

	mx_status = mx_caput( &(epics_camac->f_pv),
				1, MX_CA_LONG, &function_code );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If this was not a "write" function code, then read back
	 * the data from the crate.
	 */

	if ( ( function_code < 8 ) || ( function_code > 23 ) ) {

		mx_status = mx_caget( &(epics_camac->val_pv),
				1, MX_CA_LONG, &long_data );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		*data = (int32_t) long_data;
	}

	if ( Q != (int *) NULL ) {
		/* Get the Q-response for the preceding command. */

		mx_status = mx_caget( &(epics_camac->q_pv),
				1, MX_CA_LONG, &q_response );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		*Q = q_response;
	}

	if ( X != (int *) NULL ) {
		/* Get the X-response for the preceding command. */

		mx_status = mx_caget( &(epics_camac->x_pv),
				1, MX_CA_LONG, &q_response );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		*X = x_response;
	}

	return MX_SUCCESSFUL_RESULT;
}

