/*
 * Name:    i_keithley2400.c
 *
 * Purpose: MX driver to control Keithley 2400 switching multimeters.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2006, 2008, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define KEITHLEY2400_DEBUG	FALSE

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
#include "i_keithley2400.h"

MX_RECORD_FUNCTION_LIST mxi_keithley2400_record_function_list = {
	NULL,
	mxi_keithley2400_create_record_structures,
	mxi_keithley2400_finish_record_initialization,
	NULL,
	NULL,
	mxi_keithley2400_open,
	mxi_keithley2400_close
};

/* Keithley 2400 data structures. */

MX_RECORD_FIELD_DEFAULTS mxi_keithley2400_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_KEITHLEY2400_STANDARD_FIELDS
};

long mxi_keithley2400_num_record_fields
		= sizeof( mxi_keithley2400_record_field_defaults )
		  / sizeof( mxi_keithley2400_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_keithley2400_rfield_def_ptr
			= &mxi_keithley2400_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxi_keithley2400_get_pointers( MX_RECORD *record,
				MX_KEITHLEY2400 **keithley2400,
				MX_INTERFACE **port_interface,
				const char *calling_fname )
{
	static const char fname[] = "mxi_keithley2400_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( keithley2400 == (MX_KEITHLEY2400 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KEITHLEY2400 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( port_interface == (MX_INTERFACE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERFACE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*keithley2400 = (MX_KEITHLEY2400 *) record->record_type_struct;

	if ( *keithley2400 == (MX_KEITHLEY2400 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_KEITHLEY2400 pointer for record '%s' is NULL.",
			record->name );
	}

	*port_interface = &((*keithley2400)->port_interface);

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxi_keithley2400_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_keithley2400_create_record_structures()";

	MX_KEITHLEY2400 *keithley2400 = NULL;

	/* Allocate memory for the necessary structures. */

	keithley2400 = (MX_KEITHLEY2400 *) malloc( sizeof(MX_KEITHLEY2400) );

	if ( keithley2400 == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_KEITHLEY2400 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = keithley2400;
	record->class_specific_function_list = NULL;

	keithley2400->record = record;

	keithley2400->last_measurement_type = MXT_KEITHLEY2400_INVALID;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_keithley2400_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxi_keithley2400_finish_record_initialization()";

	MX_KEITHLEY2400 *keithley2400 = NULL;
	MX_RECORD *port_record;
	long gpib_address;

	/* Verify that the port_interface field actually corresponds
	 * to a GPIB or RS-232 interface record.
	 */

	keithley2400 = (MX_KEITHLEY2400 *) record->record_type_struct;

	port_record = keithley2400->port_interface.record;

	if ( port_record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"'%s' used by record '%s' is not an interface record.",
			port_record->name, record->name );
	}

	switch( port_record->mx_class ) {
	case MXI_RS232:
		break;
	case MXI_GPIB:
		gpib_address = keithley2400->port_interface.address;

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
mxi_keithley2400_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_keithley2400_open()";

	static char idcode[] = "KEITHLEY INSTRUMENTS INC.,MODEL 24";

	MX_KEITHLEY2400 *keithley2400 = NULL;
	MX_INTERFACE *interface = NULL;
	char response[160];
	mx_status_type mx_status;

	mx_status = mxi_keithley2400_get_pointers( record,
			&keithley2400, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch ( interface->record->mx_class ) {
	case MXI_RS232:
		mx_status = mx_rs232_discard_unread_input( interface->record,
							KEITHLEY2400_DEBUG );

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
		"Interface '%s' for Keithley 2400 record '%s' "
		"is not an RS-232 or GPIB record.",
			interface->record->name, record->name );
	}

	/**** Find out what kind of controller this is. ****/

	/* Need to avoid mxi_keithley_command() at this stage,
	 * since that function automatically calls *STB?, which
	 * we probably do not want when we are trying to establish
	 * what kind of module this is.
	 */

	mx_status = mxi_keithley_putline( record, interface, "*IDN?",
						KEITHLEY2400_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_keithley_getline( record, interface,
						response, sizeof(response),
						KEITHLEY2400_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: *IDN? response for record '%s' is '%s'",
			fname, record->name, response));

	if ( strncmp( response, idcode, strlen(idcode) ) != 0 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The controller '%s' is not a Keithley 2400 switching multimeter.  "
	"Its response to an identification query command '*IDN?' was '%s'.",
			record->name, response );
	}

	/* Clear the 2400 Error Queue. */

	mx_status = mxi_keithley_command( record, interface, "SYST:CLE",
						NULL, 0, KEITHLEY2400_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Disable concurrent measurements. */

	mx_status = mxi_keithley_command( record, interface,
					":SENS:FUNC:CONC OFF",
					NULL, 0, KEITHLEY2400_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find out what the source and measurement channels
	 * are configured for.
	 */

	mx_status = mxi_keithley2400_get_source_type( keithley2400,
						interface, NULL,
						KEITHLEY2400_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_keithley2400_get_measurement_type( keithley2400,
						interface, NULL,
						KEITHLEY2400_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_keithley2400_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_keithley2400_close()";

	MX_KEITHLEY2400 *keithley2400 = NULL;
	MX_INTERFACE *interface = NULL;
	mx_status_type mx_status;

	mx_status = mxi_keithley2400_get_pointers( record,
			&keithley2400, &interface, fname );

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
mxi_keithley2400_get_measurement_type( MX_KEITHLEY2400 *keithley2400,
					MX_INTERFACE *interface,
					long *measurement_type,
					int debug_flag )
{
	static const char fname[] = "mxi_keithley2400_get_measurement_type()";

	char command[80];
	char response[80];
	int measurement_type_value;
	mx_status_type mx_status;

	mx_status = mxi_keithley_command( keithley2400->record,
					interface, ":CONF?",
					response, sizeof(response),
					debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( response, "\"VOLT:DC\"" ) == 0 ) {
		measurement_type_value = MXT_KEITHLEY2400_VOLT;
	} else
	if ( strcmp( response, "\"CURR:DC\"" ) == 0 ) {
		measurement_type_value = MXT_KEITHLEY2400_CURR;
	} else
	if ( strcmp( response, "\"RES\"" ) == 0 ) {
		measurement_type_value = MXT_KEITHLEY2400_RES;
	} else {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unrecognized measurement type '%s' in response "
		"to the command '%s' sent to Keithley 2400 '%s'.",
			response, command, keithley2400->record->name );
	}

	keithley2400->last_measurement_type = measurement_type_value;

	if ( measurement_type != (long *) NULL ) {
		*measurement_type = measurement_type_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_keithley2400_set_measurement_type( MX_KEITHLEY2400 *keithley2400,
					MX_INTERFACE *interface,
					long measurement_type,
					int debug_flag )
{
	static const char fname[] = "mxi_keithley2400_set_measurement_type()";

	char command[80];
	mx_status_type mx_status;

	switch( measurement_type ) {
	case MXT_KEITHLEY2400_VOLT:
		strcpy( command, ":FORM:ELEM VOLT" );
		break;
	case MXT_KEITHLEY2400_CURR:
		strcpy( command, ":FORM:ELEM CURR" );
		break;
	case MXT_KEITHLEY2400_RES:
		strcpy( command, ":FORM:ELEM RES" );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported measurement type %ld for record '%s'",
			measurement_type, keithley2400->record->name );
	}

	mx_status = mxi_keithley_command( keithley2400->record,
					interface, command,
					NULL, 0, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	keithley2400->last_measurement_type = measurement_type;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_keithley2400_get_source_type( MX_KEITHLEY2400 *keithley2400,
					MX_INTERFACE *interface,
					long *source_type,
					int debug_flag )
{
	static const char fname[] = "mxi_keithley2400_get_source_type()";

	char command[80];
	char response[80];
	int source_type_value;
	mx_status_type mx_status;

	mx_status = mxi_keithley_command( keithley2400->record,
					interface, ":CONF?",
					response, sizeof(response),
					debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( response, "\"VOLT:DC\"" ) == 0 ) {
		source_type_value = MXT_KEITHLEY2400_VOLT;
	} else
	if ( strcmp( response, "\"CURR:DC\"" ) == 0 ) {
		source_type_value = MXT_KEITHLEY2400_CURR;
	} else
	if ( strcmp( response, "\"RES\"" ) == 0 ) {
		source_type_value = MXT_KEITHLEY2400_RES;
	} else {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unrecognized source type '%s' in response "
		"to the command '%s' sent to Keithley 2400 '%s'.",
			response, command, keithley2400->record->name );
	}

	keithley2400->last_source_type = source_type_value;

	if ( source_type != (long *) NULL ) {
		*source_type = source_type_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_keithley2400_set_source_type( MX_KEITHLEY2400 *keithley2400,
					MX_INTERFACE *interface,
					long source_type,
					int debug_flag )
{
	static const char fname[] = "mxi_keithley2400_set_source_type()";

	char command[80];
	mx_status_type mx_status;

	switch( source_type ) {
	case MXT_KEITHLEY2400_VOLT:
		strcpy( command, ":FORM:ELEM VOLT" );
		break;
	case MXT_KEITHLEY2400_CURR:
		strcpy( command, ":FORM:ELEM CURR" );
		break;
	case MXT_KEITHLEY2400_RES:
		strcpy( command, ":FORM:ELEM RES" );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported source type %ld for record '%s'",
			source_type, keithley2400->record->name );
	}

	mx_status = mxi_keithley_command( keithley2400->record,
					interface, command,
					NULL, 0, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	keithley2400->last_source_type = source_type;

	return MX_SUCCESSFUL_RESULT;
}

