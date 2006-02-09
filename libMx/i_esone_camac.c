/*
 * Name:    i_esone_camac.c
 *
 * Purpose: MX driver for CAMAC access via an ESONE CAMAC library.
 *
 *          At present, there is no support for block mode CAMAC operations
 *          in this set of functions.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_JORWAY_CAMAC

#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_camac.h"
#include "i_esone_camac.h"

#if HAVE_JORWAY_CAMAC
#  include "ieee_fun_types.h"

#  define ctlm(e,l) ctgl(e,l)

   extern unsigned ctgl( int ext, int *lvalue );
#endif

MX_RECORD_FUNCTION_LIST mxi_esone_camac_record_function_list = {
	NULL,
	mxi_esone_camac_create_record_structures
};

MX_CAMAC_FUNCTION_LIST mxi_esone_camac_camac_function_list = {
	mxi_esone_camac_get_lam_status,
	mxi_esone_camac_controller_command,
	mxi_esone_camac
};

MX_RECORD_FIELD_DEFAULTS mxi_esone_camac_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_CAMAC_STANDARD_FIELDS,
	MXI_ESONE_CAMAC_STANDARD_FIELDS
};

mx_length_type mxi_esone_camac_num_record_fields
	= sizeof( mxi_esone_camac_record_field_defaults )
	/ sizeof( mxi_esone_camac_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_esone_camac_rfield_def_ptr
			= &mxi_esone_camac_record_field_defaults[0];

/* ---- */

/* A private function for the use of the driver. */

static mx_status_type
mxi_esone_camac_get_pointers( MX_CAMAC *camac,
			MX_ESONE_CAMAC **esone_camac,
			const char *calling_fname )
{
	static const char fname[] = "mxi_esone_camac_get_pointers()";

	MX_RECORD *esone_camac_record;

	if ( camac == (MX_CAMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_CAMAC pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( esone_camac == (MX_ESONE_CAMAC **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ESONE_CAMAC pointer passed by '%s' was NULL.",
			calling_fname );
	}

	esone_camac_record = camac->record;

	if ( esone_camac_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The esone_camac_record pointer for the "
			"MX_CAMAC pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*esone_camac = (MX_ESONE_CAMAC *)
			esone_camac_record->record_type_struct;

	if ( *esone_camac == (MX_ESONE_CAMAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ESONE_CAMAC pointer for record '%s' is NULL.",
			esone_camac_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ------------------------------------------------------ */

MX_EXPORT mx_status_type
mxi_esone_camac_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_esone_camac_create_record_structures()";

	MX_CAMAC *crate;
	MX_ESONE_CAMAC *esone_camac;

	/* Allocate memory for the necessary structures. */

	crate = (MX_CAMAC *) malloc( sizeof(MX_CAMAC) );

	if ( crate == (MX_CAMAC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory allocating an MX_CAMAC structure." );
	}

	esone_camac = (MX_ESONE_CAMAC *) malloc( sizeof(MX_ESONE_CAMAC) );

	if ( esone_camac == (MX_ESONE_CAMAC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory allocating an MX_ESONE_CAMAC structure." );
	}

	record->record_class_struct = crate;
	record->record_type_struct = esone_camac;
	record->class_specific_function_list =
			&mxi_esone_camac_camac_function_list;

	crate->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_esone_camac_get_lam_status( MX_CAMAC *camac, int *lam_status )
{
	static const char fname[] = "mxi_esone_camac_get_lam_status()";

	MX_ESONE_CAMAC *esone_camac;
	int ext;
	mx_status_type mx_status;

	mx_status = mxi_esone_camac_get_pointers( camac, &esone_camac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	cdreg( &ext, camac->branch_number, camac->crate_number, 0, 0 );

	ctlm( ext, lam_status );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_esone_camac_controller_command(MX_CAMAC *camac, int command )
{
	static const char fname[] = "mxi_esone_camac_controller_command()";

	MX_ESONE_CAMAC *esone_camac;
	int ext;
	mx_status_type mx_status;

	mx_status = mxi_esone_camac_get_pointers( camac, &esone_camac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	cdreg( &ext, camac->branch_number, camac->crate_number, 0, 0 );

	if ( command & MX_CAMAC_Z ) {
		cccz( ext );
	}
	if ( command & MX_CAMAC_C ) {
		cccc( ext );
	}
	if ( command & MX_CAMAC_I ) {
		ccci( ext, 1 );
	} else {
		ccci( ext, 0 );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_esone_camac( MX_CAMAC *camac, int slot, int subaddress,
		int function_code, int32_t *data, int *Q, int *X)
{
	static const char fname[] = "mxi_esone_camac()";

	MX_ESONE_CAMAC *esone_camac;
	int ext;
	mx_status_type mx_status;

	mx_status = mxi_esone_camac_get_pointers( camac, &esone_camac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	cdreg( &ext, camac->branch_number, camac->crate_number,
		slot, subaddress );

	cfsa( function_code, ext, (int *) data, Q );

#if HAVE_JORWAY_CAMAC
	*X = xrespn();
#else
	*X = 1;
#endif

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_JORWAY_CAMAC */
