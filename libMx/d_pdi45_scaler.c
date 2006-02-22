/*
 * Name:    d_pdi45_scaler.c
 *
 * Purpose: MX scaler driver to control a Prairie Digital Model 45
 *          digital I/O line as an MX scaler.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003-2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_scaler.h"
#include "i_pdi45.h"
#include "d_pdi45_scaler.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_pdi45_scaler_record_function_list = {
	NULL,
	mxd_pdi45_scaler_create_record_structures,
	mxd_pdi45_scaler_finish_record_initialization
};

MX_SCALER_FUNCTION_LIST mxd_pdi45_scaler_scaler_function_list = {
	mxd_pdi45_scaler_clear,
	NULL,
	mxd_pdi45_scaler_read,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_pdi45_scaler_get_parameter,
	mxd_pdi45_scaler_set_parameter
};

/* Ortec 974 scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_pdi45_scaler_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_PDI45_SCALER_STANDARD_FIELDS
};

long mxd_pdi45_scaler_num_record_fields
		= sizeof( mxd_pdi45_scaler_record_field_defaults )
		  / sizeof( mxd_pdi45_scaler_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pdi45_scaler_rfield_def_ptr
			= &mxd_pdi45_scaler_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_pdi45_scaler_get_pointers( MX_SCALER *scaler,
			MX_PDI45_SCALER **pdi45_scaler,
			MX_PDI45 **pdi45,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pdi45_scaler_get_pointers()";

	MX_PDI45_SCALER *pdi45_scaler_ptr;
	MX_RECORD *pdi45_record;

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The scaler pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( scaler->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for scaler pointer passed by '%s' is NULL.",
			calling_fname );
	}

	pdi45_scaler_ptr = (MX_PDI45_SCALER *)
				scaler->record->record_type_struct;

	if ( pdi45_scaler_ptr == (MX_PDI45_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_PDI45_SCALER pointer for scaler record '%s' passed by '%s' is NULL",
				scaler->record->name, calling_fname );
	}

	if ( pdi45_scaler != (MX_PDI45_SCALER **) NULL ) {
		*pdi45_scaler = pdi45_scaler_ptr;
	}

	if ( pdi45 != (MX_PDI45 **) NULL ) {
		pdi45_record = pdi45_scaler_ptr->pdi45_record;

		if ( pdi45_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The pdi45_record pointer for PDI45 scaler '%s' is NULL.",
				scaler->record->name );
		}

		*pdi45 = (MX_PDI45 *) pdi45_record->record_type_struct;

		if ( *pdi45 == (MX_PDI45 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_PDI45 pointer for PDI45 record '%s' used by "
			"scaler record '%s' is NULL.",
				pdi45_record->name, scaler->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_pdi45_scaler_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_pdi45_scaler_create_record_structures()";

	MX_SCALER *scaler;
	MX_PDI45_SCALER *pdi45_scaler;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER structure." );
	}

	pdi45_scaler = (MX_PDI45_SCALER *)
				malloc( sizeof(MX_PDI45_SCALER) );

	if ( pdi45_scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PDI45_SCALER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = pdi45_scaler;
	record->class_specific_function_list
			= &mxd_pdi45_scaler_scaler_function_list;

	scaler->record = record;
	pdi45_scaler->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi45_scaler_finish_record_initialization( MX_RECORD *record )
{
	MX_SCALER *scaler;
	mx_status_type mx_status;

	mx_status = mx_scaler_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS ) {
		scaler = (MX_SCALER *) record->record_class_struct;

		scaler->mode = MXCM_COUNTER_MODE;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pdi45_scaler_clear( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_pdi45_scaler_clear()";

	MX_PDI45_SCALER *pdi45_scaler;
	MX_PDI45 *pdi45;
	char command[40];
	unsigned long io_type;
	mx_status_type mx_status;

	mx_status = mxd_pdi45_scaler_get_pointers( scaler,
			&pdi45_scaler, &pdi45, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	io_type = pdi45->io_type[ pdi45_scaler->line_number ];

	if ( ( io_type == 0x01 )
	  || ( io_type == 0x02 ) )
	{
		sprintf( command, "00Y%02X",
			1 << ( pdi45_scaler->line_number ) );

		mx_status = mxi_pdi45_command( pdi45, command, NULL, 0 );
	}

	scaler->raw_value = 0L;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pdi45_scaler_read( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_pdi45_scaler_read()";

	MX_PDI45_SCALER *pdi45_scaler;
	MX_PDI45 *pdi45;
	char command[40];
	char response[40];
	long data;
	int num_items;
	mx_status_type mx_status;

	scaler->raw_value = 0L;

	mx_status = mxd_pdi45_scaler_get_pointers( scaler,
			&pdi45_scaler, &pdi45, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "00W%02X", 1 << ( pdi45_scaler->line_number ) );

	mx_status = mxi_pdi45_command( pdi45, command,
					response, sizeof(response) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response + 1, "%4lx", &data );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"No counter value found in response '%s' to command '%s' for scaler '%s'.",
			response, command, scaler->record->name );
	}

	scaler->raw_value = data;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi45_scaler_get_parameter( MX_SCALER *scaler )
{
	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		scaler->mode = MXCM_COUNTER_MODE;
		break;
	case MXLV_SCL_DARK_CURRENT:
		break;
	default:
		return mx_scaler_default_get_parameter_handler( scaler );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi45_scaler_set_parameter( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_pdi45_scaler_set_parameter()";

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		if ( scaler->mode != MXCM_COUNTER_MODE ) {
			return mx_error( MXE_UNSUPPORTED, fname,
		"PDI45 scaler '%s' can only be used in counter mode.",
				scaler->record->name );
		}
		break;
	case MXLV_SCL_DARK_CURRENT:
		break;
	default:
		return mx_scaler_default_set_parameter_handler( scaler );
	}

	return MX_SUCCESSFUL_RESULT;
}

