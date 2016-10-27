/*
 * Name:    i_bnc725_lib.cpp
 *
 * Purpose: MX driver for the vendor-provided Win32 C++ library for the
 *          BNC725 digital delay generator.
 *
 * Author:  William Lavender
 *
 * NOTE:    This file must be compiled as C++ code.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_BNC725_LIB_DEBUG		TRUE

#include <stdio.h>
#include <ctype.h>
#include <windows.h>

#include "mx_util.h"
#include "mx_record.h"
#include "i_bnc725_lib.h"

MX_RECORD_FUNCTION_LIST mxi_bnc725_lib_record_function_list = {
	NULL,
	mxi_bnc725_lib_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_bnc725_lib_open,
	mxi_bnc725_lib_close
};

MX_RECORD_FIELD_DEFAULTS mxi_bnc725_lib_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_BNC725_LIB_STANDARD_FIELDS
};

long mxi_bnc725_lib_num_record_fields
		= sizeof( mxi_bnc725_lib_record_field_defaults )
			/ sizeof( mxi_bnc725_lib_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_bnc725_lib_rfield_def_ptr
			= &mxi_bnc725_lib_record_field_defaults[0];

static mx_status_type
mxi_bnc725_lib_get_pointers( MX_RECORD *record,
				MX_BNC725_LIB **bnc725_lib,
				const char *calling_fname )
{
	static const char fname[] = "mxi_bnc725_lib_get_pointers()";

	MX_BNC725_LIB *bnc725_lib_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	bnc725_lib_ptr = (MX_BNC725_LIB *) record->record_type_struct;

	if ( bnc725_lib_ptr == (MX_BNC725_LIB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BNC725_LIB pointer for record '%s' is NULL.",
			record->name );
	}

	if ( bnc725_lib != (MX_BNC725_LIB **) NULL ) {
		*bnc725_lib = bnc725_lib_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxi_bnc725_lib_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_bnc725_lib_create_record_structures()";

	MX_BNC725_LIB *bnc725_lib;

	/* Allocate memory for the necessary structures. */

	bnc725_lib = (MX_BNC725_LIB *) malloc( sizeof(MX_BNC725_LIB) );

	if ( bnc725_lib == (MX_BNC725_LIB *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_BNC725_LIB structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = bnc725_lib;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	bnc725_lib->record = record;

	bnc725_lib->port = new CLC880;

	if ( bnc725_lib->port == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to create a CLC880 port object." );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_bnc725_lib_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_bnc725_lib_open()";

	MX_BNC725_LIB *bnc725_lib;
	int i, c, num_items, com_number, com_speed, status;
	mx_status_type mx_status;

#if MXI_BNC725_LIB_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	mx_status = mxi_bnc725_lib_get_pointers( record, &bnc725_lib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the COM port number. */

	if ( mx_strncasecmp( "COM", bnc725_lib->port_name, 3 ) != 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The port name '%s' specified for BNC725 '%s' is not "
		"a COM port name.", record->name );
	}

	/* Force the COM port name to upper case. */

	for ( i = 0; i < 3; i++ ) {
		c = bnc725_lib->port_name[i];

		if ( islower(c) ) {
			bnc725_lib->port_name[i] = (char) toupper(c);
		}
	}

	/* Parse out the COM port number. */

	num_items = sscanf( bnc725_lib->port_name, "COM%d:", &com_number );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Could not get the COM port number from port name '%s' "
		"for BNC725 '%s'.", bnc725_lib->port_name, record->name );
	}

	/* Convert the port speed into the form 
	 * expected by the vendor library.
	 */

	switch( bnc725_lib->port_speed ) {
	case 2400:
		com_speed = COMBAUD_2400;
		break;
	case 4800:
		com_speed = COMBAUD_4800;
		break;
	case 9600:
		com_speed = COMBAUD_9600;
		break;
	case 19200:
		com_speed = COMBAUD_19200;
		break;
	case 38400:
		com_speed = COMBAUD_38400;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported port speed %lu requested for BNC725 '%s'.  "
		"The supported port speeds are 2400, 4800, 9600, 19200, "
		"and 38400.", bnc725_lib->port_speed, record->name );
		break;
	}

#if MXI_BNC725_LIB_DEBUG
	MX_DEBUG(-2,("%s: BNC725 '%s', com_number = %d, com_speed = %d.",
		fname, record->name, com_number, com_speed));
#endif

	/* Open a connection to the BNC 725 controller. */

	status = bnc725_lib->port->InitConnection( com_number, com_speed );

	if ( status != 0 ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to open a connection on '%s' "
		"to BNC725 '%s' failed.  status = %d",
			bnc725_lib->port_name,
			record->name, status );
	}

	/* Program all settings to defaults. */

	status = bnc725_lib->port->CmdUpdateAll();

	if ( status == 0 ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to program all settings of BNC725 '%s' failed.  "
		"status = %d", record->name, status );
	}

#if MXI_BNC725_LIB_DEBUG
	MX_DEBUG(-2,("%s: BNC725 '%s' successfully initialized.",
		fname, record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_bnc725_lib_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_bnc725_lib_close()";

	MX_BNC725_LIB *bnc725_lib;
	int bnc_status;
	mx_status_type mx_status;

	mx_status = mxi_bnc725_lib_get_pointers( record, &bnc725_lib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bnc_status = bnc725_lib->port->CloseConnection();

	if ( bnc_status != 0 ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to close the connection to BNC725 '%s' failed.  "
		"BNC status = %d",  record->name, bnc_status );
	}

	return MX_SUCCESSFUL_RESULT;
}

