/*
 * Name:    i_keithley2700.c
 *
 * Purpose: MX driver to control Keithley 2700 switching multimeters.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define KEITHLEY2700_DEBUG	FALSE

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
#include "i_keithley2700.h"

MX_RECORD_FUNCTION_LIST mxi_keithley2700_record_function_list = {
	NULL,
	mxi_keithley2700_create_record_structures,
	mxi_keithley2700_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_keithley2700_open,
	mxi_keithley2700_close
};

/* Keithley 2700 data structures. */

MX_RECORD_FIELD_DEFAULTS mxi_keithley2700_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_KEITHLEY2700_STANDARD_FIELDS
};

long mxi_keithley2700_num_record_fields
		= sizeof( mxi_keithley2700_record_field_defaults )
		  / sizeof( mxi_keithley2700_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_keithley2700_rfield_def_ptr
			= &mxi_keithley2700_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxi_keithley2700_get_pointers( MX_RECORD *record,
				MX_KEITHLEY2700 **keithley2700,
				MX_INTERFACE **port_interface,
				const char *calling_fname )
{
	static const char fname[] = "mxi_keithley2700_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( keithley2700 == (MX_KEITHLEY2700 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KEITHLEY2700 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( port_interface == (MX_INTERFACE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERFACE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*keithley2700 = (MX_KEITHLEY2700 *) record->record_type_struct;

	if ( *keithley2700 == (MX_KEITHLEY2700 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_KEITHLEY2700 pointer for record '%s' is NULL.",
			record->name );
	}

	*port_interface = &((*keithley2700)->port_interface);

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxi_keithley2700_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_keithley2700_create_record_structures()";

	MX_KEITHLEY2700 *keithley2700;

	/* Allocate memory for the necessary structures. */

	keithley2700 = (MX_KEITHLEY2700 *) malloc( sizeof(MX_KEITHLEY2700) );

	if ( keithley2700 == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_KEITHLEY2700 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = keithley2700;
	record->class_specific_function_list = NULL;

	keithley2700->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_keithley2700_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxi_keithley2700_finish_record_initialization()";

	MX_KEITHLEY2700 *keithley2700;
	MX_RECORD *port_record;
	long gpib_address;

	/* Verify that the port_interface field actually corresponds
	 * to a GPIB or RS-232 interface record.
	 */

	keithley2700 = (MX_KEITHLEY2700 *) record->record_type_struct;

	port_record = keithley2700->port_interface.record;

	if ( port_record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"'%s' used by record '%s' is not an interface record.",
			port_record->name, record->name );
	}

	switch( port_record->mx_class ) {
	case MXI_RS232:
		break;
	case MXI_GPIB:
		gpib_address = keithley2700->port_interface.address;

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
mxi_keithley2700_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_keithley2700_open()";

	static char idcode[] = "KEITHLEY INSTRUMENTS INC.,MODEL 27";

	MX_KEITHLEY2700 *keithley2700;
	MX_INTERFACE *interface;
	char command[80];
	char response[160];
	long i, j, max_channels, module_type, num_items, channel_type;
	long dimension_array[2];
	size_t size_array[2];
	size_t length;
	char *ptr;
	mx_status_type mx_status;

	mx_status = mxi_keithley2700_get_pointers( record,
			&keithley2700, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch ( interface->record->mx_class ) {
	case MXI_RS232:
		mx_status = mx_rs232_discard_unread_input( interface->record,
							KEITHLEY2700_DEBUG );

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
		"Interface '%s' for Keithley 2700 record '%s' "
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
						KEITHLEY2700_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_keithley_getline( record, interface,
						response, sizeof(response),
						KEITHLEY2700_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: *IDN? response for record '%s' is '%s'",
			fname, record->name, response));

	if ( strncmp( response, idcode, strlen(idcode) ) != 0 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The controller '%s' is not a Keithley 2700 switching multimeter.  "
	"Its response to an identification query command '*IDN?' was '%s'.",
			record->name, response );
	}

	/* We now know this is a Keithley 2700 with two slots. */

	keithley2700->num_slots = 2;

	/* Clear the 2700 Error Queue. */

	mx_status = mxi_keithley_command( record, interface, "SYST:CLE",
						NULL, 0, KEITHLEY2700_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Unconditionally turn off continuous initiation. */

	mx_status = mxi_keithley_command( record, interface, "INIT:CONT OFF",
						NULL, 0, KEITHLEY2700_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure the output format to only display the measured value. */

	mx_status = mxi_keithley_command( record, interface, "FORM:ELEM READ",
						NULL, 0, KEITHLEY2700_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Allocate some arrays to store information about the individual
	 * channels in the Keithley2700.
	 */

	keithley2700->module_type = (long *) malloc( keithley2700->num_slots
							* sizeof(long) );
	
	if ( keithley2700->module_type == (long *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %d element array "
		"of Keithley 2700 'module_type' values for record '%s'.",
			keithley2700->num_slots, record->name );
	}

	keithley2700->num_channels = (long *) malloc( keithley2700->num_slots
							* sizeof(long) );
	
	if ( keithley2700->num_channels == (long *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %d element array "
		"of Keithley 2700 'num_channels' values for record '%s'.",
			keithley2700->num_slots, record->name );
	}

	/* Figure out what kind of module is in each slot. */

	mx_status = mxi_keithley_command( record, interface, "*OPT?",
						response, sizeof(response),
						KEITHLEY2700_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	max_channels = 0;

	ptr = response;

	for ( i = 0; i < keithley2700->num_slots; i++ ) {
		if ( strncmp( ptr, "NONE", 4 ) == 0 ) {
			module_type = MXT_KEITHLEY2700_INVALID;
		} else {
			num_items = sscanf( ptr, "%ld", &module_type );

			if ( num_items != 1 ) {
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"Could not find a module type at pointer '%s' "
				"for slot %d in the response to " "an '*OPT?' "
				"command sent to Keithley 2700 '%s'.  "
				"Response = '%s'",
					ptr, i+1, record->name, response );
			}
		}
		keithley2700->module_type[i] = module_type;

		ptr = strchr( ptr, ',' );

		if (( ptr == NULL ) && ( i != (keithley2700->num_slots - 1) )) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Could not find the comma at the start of the "
			"slot %d's module type.  Response = '%s'.",
				i+1, response);
		}

		ptr++;

		MX_DEBUG( 2,("%s: module_type[%d] = %d",
			fname, i, keithley2700->module_type[i]));

		switch( module_type ) {
		case MXT_KEITHLEY2700_INVALID:
			keithley2700->num_channels[i] = 0;
			break;
		case 7700:
			keithley2700->num_channels[i] = 22;
			break;
		default:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Support for Keithley switching modules other than "
			"Model 7700 is not yet implemented." );
			break;
		}

		if ( keithley2700->num_channels[i] > max_channels ) {
			max_channels = keithley2700->num_channels[i];
		}
	}

	dimension_array[0] = keithley2700->num_slots;
	dimension_array[1] = max_channels;

	size_array[0] = sizeof(long);
	size_array[1] = sizeof(long *);

	keithley2700->channel_type = (long **)
			mx_allocate_array( 2, dimension_array, size_array );

	if ( keithley2700->channel_type == (long **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %d by %d element array "
		"of Keithley 2700 'channel_type's for record '%s'.",
			keithley2700->num_slots, max_channels, record->name );
	}

	/* Find out the type of measurement that each channel is performing. */

	for ( i = 0; i < keithley2700->num_slots; i++ ) {
		for ( j = 0; j < keithley2700->num_channels[i]; j++ ) {
			sprintf( command, "SENS:FUNC? (@%ld%02ld)", i+1, j+1 );

			mx_status = mxi_keithley_command( record, interface,
					command, response, sizeof(response),
					KEITHLEY2700_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* There should be double quotes at the start
			 * and end of the response.  Strip them off.
			 */

			ptr = response;

			if ( *ptr == '\"' ) {
				ptr++;
			}

			length = strlen(ptr);

			if ( ptr[length-1] == '\"' ) {
				ptr[length-1] = '\0';
			}

			/* Now figure out what type of measurement
			 * is being performed.
			 */

			if ( strcmp( ptr, "VOLT:DC" ) == 0 ) {
				channel_type = MXT_KEITHLEY2700_DCV;
			} else
			if ( strcmp( ptr, "VOLT:AC" ) == 0 ) {
				channel_type = MXT_KEITHLEY2700_ACV;
			} else
			if ( strcmp( ptr, "CURR:DC" ) == 0 ) {
				channel_type = MXT_KEITHLEY2700_DCI;
			} else
			if ( strcmp( ptr, "CURR:AC" ) == 0 ) {
				channel_type = MXT_KEITHLEY2700_ACI;
			} else
			if ( strcmp( ptr, "RES" ) == 0 ) {
				channel_type = MXT_KEITHLEY2700_OHMS_2;
			} else
			if ( strcmp( ptr, "FRES" ) == 0 ) {
				channel_type = MXT_KEITHLEY2700_OHMS_4;
			} else
			if ( strcmp( ptr, "TEMP" ) == 0 ) {
				channel_type = MXT_KEITHLEY2700_TEMP;
			} else
			if ( strcmp( ptr, "FREQ" ) == 0 ) {
				channel_type = MXT_KEITHLEY2700_FREQ;
			} else
			if ( strcmp( ptr, "PER" ) == 0 ) {
				channel_type = MXT_KEITHLEY2700_PERIOD;
			} else
			if ( strcmp( ptr, "CONT" ) == 0 ) {
				channel_type = MXT_KEITHLEY2700_CONT;
			} else {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized measurement type '%s' found for "
			"slot %d, channel %d in Keithley 2700 '%s'.",
					ptr, i+1, j+1, record->name );
			}

			keithley2700->channel_type[i][j] = channel_type;
		}

		/* For unsupported channel numbers, set the channel type
		 * to invalid.
		 */

		for ( ; j < max_channels; j++ ) {
			keithley2700->channel_type[i][j]
					= MXT_KEITHLEY2700_INVALID;
		}
	}

#if 0
	fprintf( stderr, "Keithley2700 '%s' channel types are\n",
			record->name );

	for ( i = 0; i < keithley2700->num_slots; i++ ) {
		fprintf( stderr, "Slot %d:", i );
		for ( j = 0; j < max_channels; j++ ) {
			fprintf( stderr, " %d",
				keithley2700->channel_type[i][j] );
		}
		fprintf( stderr,"\n" );
	}
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_keithley2700_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_keithley2700_close()";

	MX_KEITHLEY2700 *keithley2700;
	MX_INTERFACE *interface;
	mx_status_type mx_status;

	mx_status = mxi_keithley2700_get_pointers( record,
			&keithley2700, &interface, fname );

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

