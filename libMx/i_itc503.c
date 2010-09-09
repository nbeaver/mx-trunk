/*
 * Name:    i_itc503.c
 *
 * Purpose: MX driver for Oxford Instruments ITC503 and Cryojet
 *          temperature controllers.
 *          
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008-2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_ITC503_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "i_isobus.h"
#include "i_itc503.h"

MX_RECORD_FUNCTION_LIST mxi_itc503_record_function_list = {
	NULL,
	mxi_itc503_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_itc503_open
};

MX_RECORD_FIELD_DEFAULTS mxi_itc503_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_ITC503_STANDARD_FIELDS
};

long mxi_itc503_num_record_fields
		= sizeof( mxi_itc503_record_field_defaults )
			/ sizeof( mxi_itc503_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_itc503_rfield_def_ptr
			= &mxi_itc503_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_itc503_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_itc503_create_record_structures()";

	MX_ITC503 *itc503;

	/* Allocate memory for the necessary structures. */

	itc503 = (MX_ITC503 *) malloc( sizeof(MX_ITC503) );

	if ( itc503 == (MX_ITC503 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ITC503 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = itc503;

	record->record_function_list = &mxi_itc503_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	itc503->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_itc503_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_itc503_open()";

	MX_ITC503 *itc503;
	MX_ISOBUS *isobus;
	char command[10];
	char response[40];
	int c_command_value;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

#if MXI_ITC503_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif

	itc503 = (MX_ITC503 *) record->record_type_struct;

	if ( itc503 == (MX_ITC503 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ITC503 pointer for record '%s' is NULL.", record->name);
	}

	if ( itc503->isobus_record == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"isobus_record pointer for record '%s' is NULL.", record->name);
	}

	isobus = itc503->isobus_record->record_type_struct;

	if ( isobus == (MX_ISOBUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ISOBUS pointer for ISOBUS record '%s' is NULL.",
			itc503->isobus_record->name );
	}

	switch( record->mx_type ) {
	case MXI_CTRL_ITC503:
		strlcpy( itc503->label, "ITC503", sizeof(itc503->label) );
		break;

	case MXI_CTRL_CRYOJET:
		strlcpy( itc503->label, "Cryojet", sizeof(itc503->label) );
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Record '%s' is of MX type %lu, which is not supported "
		"by this driver.",
			record->name, record->mx_type );
		break;
	}

	/* Tell the ITC503 to terminate responses only with a <CR> character. */

	mx_status = mxi_isobus_command( isobus, itc503->isobus_address,
					"Q0", NULL, 0, -1,
					MXI_ITC503_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Ask for the version number of the controller. */

	mx_status = mxi_isobus_command( isobus, itc503->isobus_address,
					"V", response, sizeof(response),
					itc503->maximum_retries,
					MXI_ITC503_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_ITC503_DEBUG
	MX_DEBUG(-2,("%s: ITC503 controller '%s' version = '%s'",
		fname, record->name, response));
#endif

	switch( itc503->record->mx_type ) {
	case MXI_CTRL_CRYOJET:
		if ( strncmp( response, "JET", 3 ) != 0 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"%s controller '%s' did not return the expected "
			"version string in its response to the V command.  "
			"Response = '%s'",
				itc503->label, record->name, response );
		}
		break;

	case MXI_CTRL_ITC503:
		break;
	}

	/* Send a 'Cn' control command.  See the header file
	 * 'i_itc503.h' for a description of this command.
	 */

	c_command_value = (int) ( itc503->itc503_flags & 0x3 );

	snprintf( command, sizeof(command), "C%d", c_command_value );

	mx_status = mxi_isobus_command( isobus, itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					MXI_ITC503_DEBUG );

	return mx_status;
}

