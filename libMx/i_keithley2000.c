/*
 * Name:    i_keithley2000.c
 *
 * Purpose: MX driver to control Keithley 2000 series multimeters.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define KEITHLEY2000_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_rs232.h"
#include "mx_gpib.h"
#include "i_keithley.h"
#include "i_keithley2000.h"

MX_RECORD_FUNCTION_LIST mxi_keithley2000_record_function_list = {
	NULL,
	mxi_keithley2000_create_record_structures,
	mxi_keithley2000_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_keithley2000_open,
	mxi_keithley2000_close
};

/* Keithley 2000 data structures. */

MX_RECORD_FIELD_DEFAULTS mxi_keithley2000_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_KEITHLEY2000_STANDARD_FIELDS
};

mx_length_type mxi_keithley2000_num_record_fields
		= sizeof( mxi_keithley2000_record_field_defaults )
		  / sizeof( mxi_keithley2000_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_keithley2000_rfield_def_ptr
			= &mxi_keithley2000_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxi_keithley2000_get_pointers( MX_RECORD *record,
				MX_KEITHLEY2000 **keithley2000,
				MX_INTERFACE **port_interface,
				const char *calling_fname )
{
	static const char fname[] = "mxi_keithley2000_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( keithley2000 == (MX_KEITHLEY2000 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KEITHLEY2000 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( port_interface == (MX_INTERFACE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERFACE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*keithley2000 = (MX_KEITHLEY2000 *) record->record_type_struct;

	if ( *keithley2000 == (MX_KEITHLEY2000 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_KEITHLEY2000 pointer for record '%s' is NULL.",
			record->name );
	}

	*port_interface = &((*keithley2000)->port_interface);

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxi_keithley2000_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_keithley2000_create_record_structures()";

	MX_KEITHLEY2000 *keithley2000;

	/* Allocate memory for the necessary structures. */

	keithley2000 = (MX_KEITHLEY2000 *) malloc( sizeof(MX_KEITHLEY2000) );

	if ( keithley2000 == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_KEITHLEY2000 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = keithley2000;
	record->class_specific_function_list = NULL;

	keithley2000->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_keithley2000_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxi_keithley2000_finish_record_initialization()";

	MX_KEITHLEY2000 *keithley2000;
	MX_RECORD *port_record;
	long gpib_address;

	/* Verify that the port_interface field actually corresponds
	 * to a GPIB or RS-232 interface record.
	 */

	keithley2000 = (MX_KEITHLEY2000 *) record->record_type_struct;

	port_record = keithley2000->port_interface.record;

	if ( port_record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"'%s' used by record '%s' is not an interface record.",
			port_record->name, record->name );
	}

	switch( port_record->mx_class ) {
	case MXI_RS232:
		break;
	case MXI_GPIB:
		gpib_address = keithley2000->port_interface.address;

		/* Check that the GPIB address is valid. */

		if ( gpib_address < 0 || gpib_address > 30 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"GPIB address %ld for record '%s' is out of allowed range 0-30.",
			gpib_address, record->name );
		}

		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"'%s' used by record '%s' is not an RS-232 or GPIB record.",
			port_record->name, record->name );
		break;
	}


	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_keithley2000_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_keithley2000_open()";

	static char idcode[] = "KEITHLEY INSTRUMENTS INC.,MODEL 20";

	MX_KEITHLEY2000 *keithley2000;
	MX_INTERFACE *interface;
	char response[160];
	mx_status_type mx_status;

	mx_status = mxi_keithley2000_get_pointers( record,
			&keithley2000, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch ( interface->record->mx_class ) {
	case MXI_RS232:
		mx_status = mx_rs232_discard_unread_input( interface->record,
							KEITHLEY2000_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	case MXI_GPIB:
		mx_status = mx_gpib_open_device( interface->record,
						interface->address );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_gpib_selective_device_clear( interface->record,
							interface->address );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if 0
		mx_status = mx_gpib_remote_enable( interface->record,
							interface->address );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
#endif
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Interface '%s' for Keithley 2000 record '%s' "
		"is not an RS-232 or GPIB record.",
			interface->record->name, record->name );
		break;
	}

	/**** Find out what kind of controller this is. ****/

	/* Need to avoid mxi_keithley_command() at this stage,
	 * since that function automatically calls *STB?, which
	 * we probably do not want when we are trying to establish
	 * what kind of module this is.
	 */

	mx_status = mxi_keithley_putline( record, interface, "*IDN?",
						KEITHLEY2000_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_keithley_getline( record, interface,
						response, sizeof(response),
						KEITHLEY2000_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: *IDN? response for record '%s' is '%s'",
			fname, record->name, response));

	if ( strncmp( response, idcode, strlen(idcode) ) != 0 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The controller '%s' is not a Keithley 2000 series multimeter.  "
	"Its response to an identification query command '*IDN?' was '%s'.",
			record->name, response );
	}

	/* Clear the 2000 Error Queue. */

	mx_status = mxi_keithley_command( record, interface, "SYST:CLE",
						NULL, 0, KEITHLEY2000_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	keithley2000->last_measurement_type = MXT_KEITHLEY2000_UNKNOWN;

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_keithley2000_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_keithley2000_close()";

	MX_KEITHLEY2000 *keithley2000;
	MX_INTERFACE *interface;
	mx_status_type mx_status;

	mx_status = mxi_keithley2000_get_pointers( record,
			&keithley2000, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch ( interface->record->mx_type ) {
	case MXI_GPIB: 
		mx_status = mx_gpib_go_to_local( interface->record,
					interface->address );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_gpib_close_device( interface->record,
					interface->address );
		break;
	default:
		break;
	}

	return mx_status;
}

