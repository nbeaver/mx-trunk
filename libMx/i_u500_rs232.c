/*
 * Name:    i_u500_rs232.c
 *
 * Purpose: MX driver for sending program lines to Aerotech Unidex 500
 *          series motor controllers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_U500

#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "i_u500.h"
#include "i_u500_rs232.h"

MX_RECORD_FUNCTION_LIST mxi_u500_rs232_record_function_list = {
	NULL,
	mxi_u500_rs232_create_record_structures,
	mxi_u500_rs232_finish_record_initialization
};

MX_RS232_FUNCTION_LIST mxi_u500_rs232_rs232_function_list = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_u500_rs232_putline,
};

MX_RECORD_FIELD_DEFAULTS mxi_u500_rs232_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_U500_RS232_STANDARD_FIELDS
};

long mxi_u500_rs232_num_record_fields
		= sizeof( mxi_u500_rs232_record_field_defaults )
			/ sizeof( mxi_u500_rs232_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_u500_rs232_rfield_def_ptr
			= &mxi_u500_rs232_record_field_defaults[0];

/* ---- */

static mx_status_type
mxi_u500_rs232_get_pointers( MX_RS232 *rs232,
				MX_U500_RS232 **u500_rs232,
				MX_U500 **u500,
				const char *calling_fname )
{
	static const char fname[] = "mxi_u500_rs232_get_pointers()";

	MX_RECORD *u500_rs232_record, *u500_record;
	MX_U500_RS232 *u500_rs232_ptr;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	u500_rs232_record = rs232->record;

	if ( u500_rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RECORD pointer for the MX_RS232 pointer %p "
			"passed by '%s' is NULL.", rs232, calling_fname );
	}

	u500_rs232_ptr = (MX_U500_RS232 *)u500_rs232_record->record_type_struct;

	if ( u500_rs232_ptr == (MX_U500_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_U500_RS232 pointer for record '%s' is NULL.",
			u500_rs232_record->name );
	}

	if ( u500_rs232 != (MX_U500_RS232 **) NULL ) {
		*u500_rs232 = u500_rs232_ptr;
	}

	if ( u500 != (MX_U500 **) NULL ) {
		u500_record = u500_rs232_ptr->u500_record;

		if ( u500_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The u500_record pointer for record '%s' is NULL.",
				u500_rs232_record->name );
		}

		*u500 = (MX_U500 *) u500_record->record_type_struct;

		if ( (*u500) == (MX_U500 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_U500 pointer for record '%s' is NULL.",
				u500_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_u500_rs232_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_u500_rs232_create_record_structures()";

	MX_RS232 *rs232;
	MX_U500_RS232 *u500_rs232;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_RS232 structure for record '%s'.",
			record->name );
	}

	u500_rs232 = (MX_U500_RS232 *) malloc( sizeof(MX_U500_RS232) );

	if ( u500_rs232 == (MX_U500_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for MX_U500_RS232 structure for record '%s'.",
			record->name );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = u500_rs232;
	record->class_specific_function_list =
					&mxi_u500_rs232_rs232_function_list;
	rs232->record = record;
	u500_rs232->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_u500_rs232_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	/* Check to see if the RS-232 parameters are valid. */

	mx_status = mx_rs232_check_port_parameters( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the U500 RS232 device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_u500_rs232_putline( MX_RS232 *rs232,
			char *buffer,
			size_t *bytes_written )
{
	static const char fname[] = "mxi_u500_rs232_putline()";

	MX_U500_RS232 *u500_rs232;
	MX_U500 *u500;
	mx_status_type mx_status;

	mx_status = mxi_u500_rs232_get_pointers( rs232,
						&u500_rs232, &u500, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_u500_command( u500, u500_rs232->board_number, buffer );

	return mx_status;
}

#endif /* HAVE_U500 */

