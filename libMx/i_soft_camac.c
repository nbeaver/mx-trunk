/*
 * Name:    i_soft_camac.c
 *
 * Purpose: An MX CAMAC driver for a software emulated interface.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_SCAMAC_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_camac.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "i_soft_camac.h"

MX_RECORD_FUNCTION_LIST mxi_scamac_record_function_list = {
	NULL,
	mxi_scamac_create_record_structures,
	mxi_scamac_finish_record_initialization
};

MX_CAMAC_FUNCTION_LIST mxi_scamac_camac_function_list = {
	mxi_scamac_get_lam_status,
	mxi_scamac_controller_command,
	mxi_scamac_camac
};

MX_RECORD_FIELD_DEFAULTS mxi_scamac_record_field_defaults[] = {
  MX_RECORD_STANDARD_FIELDS,
  MX_CAMAC_STANDARD_FIELDS,
  MXI_SCAMAC_STANDARD_FIELDS
};

mx_length_type mxi_scamac_num_record_fields
				= sizeof( mxi_scamac_record_field_defaults )
				/ sizeof( mxi_scamac_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_scamac_record_field_def_ptr
			= &mxi_scamac_record_field_defaults[0];

#define LOGFILE_NAME_LENGTH 255

MX_EXPORT mx_status_type
mxi_scamac_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_scamac_create_record_structures()";

	MX_CAMAC *crate;
	MX_SCAMAC *scamac;

	/* Allocate memory for the necessary structures. */

	crate = (MX_CAMAC *) malloc( sizeof(MX_CAMAC) );

	if ( crate == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_CAMAC structure." );
	}

	scamac = (MX_SCAMAC *) malloc( sizeof(MX_SCAMAC) );

	if ( scamac == NULL ) {
		mx_free(crate);

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_SCAMAC structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = crate;
	record->record_type_struct = scamac;
	record->class_specific_function_list = &mxi_scamac_camac_function_list;

	crate->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_scamac_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxi_scamac_finish_record_initialization()";

	MX_SCAMAC *scamac;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	scamac = (MX_SCAMAC *) record->record_type_struct;

	if ( scamac == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SCAMAC pointer for record '%s' is NULL.",
			record->name );
	}

	scamac->logfile = fopen( scamac->logfile_name, "w" );

	if ( scamac->logfile == NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot open CAMAC log file '%s'", scamac->logfile_name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_scamac_get_lam_status( MX_CAMAC *crate, int *lam_n )
{
	*lam_n = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_scamac_controller_command( MX_CAMAC *crate, int command )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_scamac_camac( MX_CAMAC *crate, int slot, int subaddress,
		int function_code, int32_t *data, int *Q, int *X)
{
	static const char fname[] = "mxi_scamac_camac()";

	MX_SCAMAC *scamac;

	if ( crate == (MX_CAMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CAMAC pointer passed was NULL." );
	}
	if ( crate->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_CAMAC pointer %p is NULL.",
			crate );
	}

#if MXI_SCAMAC_DEBUG
	if ( function_code >= 16 && function_code <= 23 ) {
		MX_DEBUG(-2, ("mxi_scamac_camac(%s, %d, %d, %d, %ld) invoked.",
			crate->record->name, slot, subaddress,
			function_code, *data));
	} else {
		MX_DEBUG(-2, ("mxi_scamac_camac(%s, %d, %d, %d) invoked.",
			crate->record->name, slot, subaddress, function_code));
	}
#endif

	scamac = (MX_SCAMAC *) crate->record->record_type_struct;

	if ( scamac == (MX_SCAMAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SCAMAC structure pointer is NULL.");
	}

	if ( (scamac->logfile) != NULL ) {
		if ( function_code >= 16 && function_code <= 23 ) {
			fprintf(scamac->logfile,
				"camac: %s, %d, %d, %d, %lu (0x%lx)\n",
				crate->record->name, slot, subaddress,
				function_code, (unsigned long) *data,
				(unsigned long) *data);

			fflush(scamac->logfile);
		} else {
			fprintf(scamac->logfile,
				"camac: %s, %d, %d, %d\n",
				crate->record->name, slot, subaddress,
				function_code);

			fflush(scamac->logfile);
		}
	}

	if ( slot < 1 || slot > 24 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal CAMAC slot number = %d", slot);
	}

	if ( subaddress < 0 || subaddress > 15 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal CAMAC subaddress number = %d", subaddress);
	}

	if ( function_code < 0 || function_code > 31 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal CAMAC function code = %d", function_code);
	}

	/* For CAMAC read operations, always return 1. */

	if ( function_code >= 0 && function_code <= 7 ) {

		*data = 1;

	}

	*Q = 1;
	*X = 1;

	return MX_SUCCESSFUL_RESULT;
}
