/*
 * Name:    i_sr630.c
 *
 * Purpose: MX driver to control the Stanford Research Systems model SR630
 *          16 channel thermocouple reader.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005, 2007, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define SR630_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_rs232.h"
#include "mx_gpib.h"
#include "i_sr630.h"

MX_RECORD_FUNCTION_LIST mxi_sr630_record_function_list = {
	NULL,
	mxi_sr630_create_record_structures,
	mxi_sr630_finish_record_initialization,
	NULL,
	NULL,
	mxi_sr630_open,
	mxi_sr630_close
};

/* SR630 data structures. */

MX_RECORD_FIELD_DEFAULTS mxi_sr630_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_SR630_STANDARD_FIELDS
};

long mxi_sr630_num_record_fields
		= sizeof( mxi_sr630_record_field_defaults )
		  / sizeof( mxi_sr630_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_sr630_rfield_def_ptr
			= &mxi_sr630_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxi_sr630_get_pointers( MX_RECORD *record,
				MX_SR630 **sr630,
				MX_INTERFACE **port_interface,
				const char *calling_fname )
{
	static const char fname[] = "mxi_sr630_get_pointers()";

	MX_SR630 *sr630_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	sr630_ptr = (MX_SR630 *) record->record_type_struct;

	if ( sr630_ptr == (MX_SR630 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SR630 pointer for record '%s' is NULL.",
			record->name );
	}

	if ( sr630 != (MX_SR630 **) NULL ) {
		*sr630 = sr630_ptr;
	}

	if ( port_interface != (MX_INTERFACE **) NULL ) {
		*port_interface = &(sr630_ptr->port_interface);
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxi_sr630_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_sr630_create_record_structures()";

	MX_SR630 *sr630;

	/* Allocate memory for the necessary structures. */

	sr630 = (MX_SR630 *) malloc( sizeof(MX_SR630) );

	if ( sr630 == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SR630 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = sr630;
	record->class_specific_function_list = NULL;

	sr630->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sr630_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxi_sr630_finish_record_initialization()";

	MX_SR630 *sr630;
	MX_RECORD *port_record;
	long gpib_address;

	/* Verify that the port_interface field actually corresponds
	 * to a GPIB or RS-232 interface record.
	 */

	sr630 = (MX_SR630 *) record->record_type_struct;

	port_record = sr630->port_interface.record;

	if ( port_record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"'%s' used by record '%s' is not an interface record.",
			port_record->name, record->name );
	}

	switch( port_record->mx_class ) {
	case MXI_RS232:
		break;
	case MXI_GPIB:
		gpib_address = sr630->port_interface.address;

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
	}


	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sr630_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_sr630_open()";

	static char idcode[] = "StanfordResearchSystems,SR630,";

	MX_SR630 *sr630;
	MX_INTERFACE *interface;
	char response[160];
	mx_status_type mx_status;

	sr630 = NULL;
	interface = NULL;

	mx_status = mxi_sr630_get_pointers( record,
			&sr630, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch ( interface->record->mx_class ) {
	case MXI_RS232:
		mx_status = mx_rs232_discard_unread_input( interface->record,
							SR630_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	case MXI_GPIB:
		mx_status = mx_gpib_open_device( interface->record,
						interface->address );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_gpib_remote_enable( interface->record,
							interface->address );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Interface '%s' for SR630 record '%s' "
		"is not an RS-232 or GPIB record.",
			interface->record->name, record->name );
	}

	/**** Find out what kind of controller this is. ****/

	mx_status = mxi_sr630_command( sr630, "*IDN?",
					response, sizeof(response),
					SR630_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: *IDN? response for record '%s' is '%s'",
			fname, record->name, response));

	if ( strncmp( response, idcode, strlen(idcode) ) != 0 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The controller '%s' is not a SR630 switching multimeter.  "
	"Its response to an identification query command '*IDN?' was '%s'.",
			record->name, response );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_sr630_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_sr630_close()";

	MX_SR630 *sr630;
	MX_INTERFACE *interface;
	mx_status_type mx_status;

	interface = NULL;

	mx_status = mxi_sr630_get_pointers( record,
			&sr630, &interface, fname );

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

MX_EXPORT mx_status_type
mxi_sr630_command( MX_SR630 *sr630,
		char *command,
		char *response, int response_buffer_length,
		int debug_flag )
{
	static const char fname[] = "mxi_sr630_command()";

	MX_INTERFACE *port_interface;
	mx_status_type mx_status;

	if ( sr630 == (MX_SR630 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SR630 pointer passed was NULL." );
	}

	port_interface = &(sr630->port_interface);

	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'command' pointer passed was NULL.");
	}

	/* Send the command string. */

	switch( port_interface->record->mx_class ) {
	case MXI_RS232:
		mx_status = mx_rs232_putline( port_interface->record,
						command, NULL, debug_flag );
		break;
	case MXI_GPIB:
		mx_status = mx_gpib_putline( port_interface->record,
						port_interface->address,
						command, NULL, debug_flag );
		break;
	default:
		mx_status = mx_error( MXE_TYPE_MISMATCH, fname,
		"Interface record '%s' is not an RS-232 or GPIB record.",
			port_interface->record->name );
		break;
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the response, if one is expected. */

	if ( response != (char *) NULL ) {
		switch( port_interface->record->mx_class ) {
		case MXI_RS232:
			mx_status = mx_rs232_getline( port_interface->record,
					response, response_buffer_length,
					NULL, debug_flag );
			break;
		case MXI_GPIB:
			mx_status = mx_gpib_getline( port_interface->record,
					port_interface->address,
					response, response_buffer_length,
					NULL, debug_flag );
			break;
		}
	}

	return mx_status;
}

