/*
 * Name:    d_cyberstar_x1000.c
 *
 * Purpose: MX driver for the Oxford Danfysik Cyberstar X1000 scintillation
 *          detector and pulse processing system.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define CYBERSTAR_X1000_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_ascii.h"
#include "mx_rs232.h"
#include "mx_sca.h"
#include "d_cyberstar_x1000.h"

/* Initialize the sca driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_cyberstar_x1000_record_function_list = {
	NULL,
	mxd_cyberstar_x1000_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_cyberstar_x1000_open,
	NULL,
	NULL,
	mxd_cyberstar_x1000_resynchronize,
	mxd_cyberstar_x1000_special_processing_setup
};

MX_SCA_FUNCTION_LIST mxd_cyberstar_x1000_sca_function_list = {
	mxd_cyberstar_x1000_get_parameter,
	mxd_cyberstar_x1000_set_parameter
};

/* CYBERSTAR_X1000 sca data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_cyberstar_x1000_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCA_STANDARD_FIELDS,
	MXD_CYBERSTAR_X1000_STANDARD_FIELDS
};

long mxd_cyberstar_x1000_num_record_fields
		= sizeof( mxd_cyberstar_x1000_record_field_defaults )
		  / sizeof( mxd_cyberstar_x1000_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_cyberstar_x1000_rfield_def_ptr
			= &mxd_cyberstar_x1000_record_field_defaults[0];

/* Private functions for the use of the driver. */

static mx_status_type mxd_cyberstar_x1000_process_function( void *record_ptr,
						void *record_field_ptr,
						int operation );

static mx_status_type
mxd_cyberstar_x1000_get_pointers( MX_SCA *sca,
				MX_CYBERSTAR_X1000 **cyberstar_x1000,
				const char *calling_fname )
{
	static const char fname[] = "mxd_cyberstar_x1000_get_pointers()";

	MX_RECORD *record;

	if ( sca == (MX_SCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SCA pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( cyberstar_x1000 == (MX_CYBERSTAR_X1000 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CYBERSTAR_X1000 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	record = sca->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_SCA structure "
		"passed by '%s' is NULL.", calling_fname );
	}

	*cyberstar_x1000 = (MX_CYBERSTAR_X1000 *) record->record_type_struct;

	if ( *cyberstar_x1000 == (MX_CYBERSTAR_X1000 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_CYBERSTAR_X1000 pointer for record '%s' is NULL.",
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_cyberstar_x1000_create_record_structures( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_cyberstar_x1000_create_record_structures()";

	MX_SCA *sca;
	MX_CYBERSTAR_X1000 *cyberstar_x1000;

	/* Allocate memory for the necessary structures. */

	sca = (MX_SCA *) malloc( sizeof(MX_SCA) );

	if ( sca == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCA structure." );
	}

	cyberstar_x1000 = (MX_CYBERSTAR_X1000 *)
				malloc( sizeof(MX_CYBERSTAR_X1000) );

	if ( cyberstar_x1000 == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_CYBERSTAR_X1000 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = sca;
	record->record_type_struct = cyberstar_x1000;
	record->class_specific_function_list
			= &mxd_cyberstar_x1000_sca_function_list;

	sca->record = record;
	cyberstar_x1000->record = record;

	cyberstar_x1000->discard_echoed_command_line = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cyberstar_x1000_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_cyberstar_x1000_open()";

	MX_SCA *sca;
	MX_CYBERSTAR_X1000 *cyberstar_x1000;
	unsigned long num_input_bytes_available;
	mx_status_type mx_status;

	sca = (MX_SCA *) (record->record_class_struct);

	if ( sca == (MX_SCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SCA pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_cyberstar_x1000_get_pointers(sca,
						&cyberstar_x1000, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Clear the RS-232 port buffers. */

	mx_status = mx_rs232_discard_unread_input(
						cyberstar_x1000->rs232_record,
						CYBERSTAR_X1000_DEBUG );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
	case MXE_UNSUPPORTED:
		break;
	default:
		return mx_status;
	}

	mx_status = mx_rs232_discard_unwritten_output(
						cyberstar_x1000->rs232_record,
						CYBERSTAR_X1000_DEBUG );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
	case MXE_UNSUPPORTED:
		break;
	default:
		return mx_status;
	}

	mx_status = mx_sca_set_parameter( record,
			MXLV_CYBERSTAR_X1000_FORCED_REMOTE_CONTROL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	cyberstar_x1000->security_status = 0;

	mx_status = mx_sca_set_parameter( record,
					MXLV_CYBERSTAR_X1000_SECURITY_STATUS );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if the Cyberstar X1000 is listening to us by asking for
	 * its gain setting.
	 */

	mx_status = mx_sca_get_gain( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If there are still characters available from the RS-232 port,
	 * then the serial port is echoing back part of the transmitted
	 * command.  This means that the RS-232 cable is incorrectly
	 * wired, but we will attempt to continue anyway.
	 */

	mx_status = mx_rs232_num_input_bytes_available(
					cyberstar_x1000->rs232_record,
					&num_input_bytes_available );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_input_bytes_available > 0 ) {

		cyberstar_x1000->discard_echoed_command_line = TRUE;

		(void) mx_rs232_discard_unread_input(
				cyberstar_x1000->rs232_record, FALSE );

		mx_warning(
	"Some or all of the command string transmitted to Cyberstar X1000 "
	"device '%s' was echoed back to the serial port.   This means that "
	"the RS-232 cable is incorrectly wired, but we will attempt to "
	"continue by discarding the echoed characters.  However, this slows "
	"down the driver, so it would be better to fix the wiring.",
			record->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_cyberstar_x1000_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_cyberstar_x1000_resynchronize()";

	MX_SCA *sca;
	MX_CYBERSTAR_X1000 *cyberstar_x1000;
	char command[40];
	mx_status_type mx_status;

	sca = (MX_SCA *) (record->record_class_struct);

	if ( sca == (MX_SCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SCA pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_cyberstar_x1000_get_pointers(sca,
						&cyberstar_x1000, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	(void) mx_rs232_discard_unread_input( cyberstar_x1000->rs232_record,
						CYBERSTAR_X1000_DEBUG );

	(void) mx_rs232_discard_unwritten_output( cyberstar_x1000->rs232_record,
						CYBERSTAR_X1000_DEBUG );

	sprintf(command, "*RST%d", cyberstar_x1000->address );

	mx_status = mxd_cyberstar_x1000_command( cyberstar_x1000,
			command, NULL, 0, CYBERSTAR_X1000_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_cyberstar_x1000_open( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_cyberstar_x1000_get_parameter( MX_SCA *sca )
{
	static const char fname[] = "mxd_cyberstar_x1000_get_parameter()";

	MX_CYBERSTAR_X1000 *cyberstar_x1000;
	char command[40];
	char response[80];
	int num_items;
	double double_value;
	unsigned long time_constant_value;
	mx_status_type mx_status;

	mx_status = mxd_cyberstar_x1000_get_pointers( sca,
						&cyberstar_x1000, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for sca '%s' for parameter type '%s' (%d).",
		fname, sca->record->name,
		mx_get_field_label_string( sca->record,
			sca->parameter_type ),
		sca->parameter_type));

	switch( sca->parameter_type ) {
	case MXLV_SCA_LOWER_LEVEL:
		sprintf(command, ":SENS%d:SCA:LOW?", cyberstar_x1000->address);

		mx_status = mxd_cyberstar_x1000_command( cyberstar_x1000,
					command, response, sizeof response,
					CYBERSTAR_X1000_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &(sca->lower_level) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cyberstar X1000 '%s' returned an unrecognizable response '%s' "
		"to the command '%s'.",
				sca->record->name, command, response );
		}
		break;

	case MXLV_SCA_UPPER_LEVEL:
		sprintf(command, ":SENS%d:SCA:UPP?", cyberstar_x1000->address);

		mx_status = mxd_cyberstar_x1000_command( cyberstar_x1000,
					command, response, sizeof response,
					CYBERSTAR_X1000_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &(sca->upper_level) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cyberstar X1000 '%s' returned an unrecognizable response '%s' "
		"to the command '%s'.",
				sca->record->name, command, response );
		}
		break;

	case MXLV_SCA_GAIN:
		sprintf(command, ":INP%d:GAIN?", cyberstar_x1000->address);

		mx_status = mxd_cyberstar_x1000_command( cyberstar_x1000,
					command, response, sizeof response,
					CYBERSTAR_X1000_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &(sca->gain) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cyberstar X1000 '%s' returned an unrecognizable response '%s' "
		"to the command '%s'.",
				sca->record->name, command, response );
		}
		break;

	case MXLV_SCA_TIME_CONSTANT:
		sprintf(command, ":SENS%d:PKT?", cyberstar_x1000->address);

		mx_status = mxd_cyberstar_x1000_command( cyberstar_x1000,
					command, response, sizeof response,
					CYBERSTAR_X1000_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lu", &time_constant_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cyberstar X1000 '%s' returned an unrecognizable response '%s' "
		"to the command '%s'.",
				sca->record->name, command, response );
		}

		sca->time_constant = 1.0e-9 * (double) time_constant_value;
		break;

	case MXLV_SCA_MODE:
		sca->sca_mode = MXF_SCA_UNKNOWN_MODE;
		break;

	case MXLV_CYBERSTAR_X1000_HIGH_VOLTAGE:
		sprintf(command, ":SOUR%d:VOLT?", cyberstar_x1000->address);

		mx_status = mxd_cyberstar_x1000_command( cyberstar_x1000,
					command, response, sizeof response,
					CYBERSTAR_X1000_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg",
				&(cyberstar_x1000->high_voltage) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cyberstar X1000 '%s' returned an unrecognizable response '%s' "
		"to the command '%s'.",
				sca->record->name, command, response );
		}
		break;

	case MXLV_CYBERSTAR_X1000_DELAY:
		sprintf(command, ":TRIG%d:ECO?", cyberstar_x1000->address);

		mx_status = mxd_cyberstar_x1000_command( cyberstar_x1000,
					command, response, sizeof response,
					CYBERSTAR_X1000_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &double_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cyberstar X1000 '%s' returned an unrecognizable response '%s' "
		"to the command '%s'.",
				sca->record->name, command, response );
		}

		cyberstar_x1000->delay = 0.1 * double_value;
		break;

	case MXLV_CYBERSTAR_X1000_FORCED_REMOTE_CONTROL:
		sprintf(command, ":SYST%d:COMM:REM?", cyberstar_x1000->address);

		mx_status = mxd_cyberstar_x1000_command( cyberstar_x1000,
					command, response, sizeof response,
					CYBERSTAR_X1000_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%d",
				&(cyberstar_x1000->forced_remote_control) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cyberstar X1000 '%s' returned an unrecognizable response '%s' "
		"to the command '%s'.",
				sca->record->name, command, response );
		}
		break;

	case MXLV_CYBERSTAR_X1000_SECURITY_STATUS:
		sprintf(command, ":SYST%d:SEC?", cyberstar_x1000->address);

		mx_status = mxd_cyberstar_x1000_command( cyberstar_x1000,
					command, response, sizeof response,
					CYBERSTAR_X1000_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%d",
				&(cyberstar_x1000->security_status) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cyberstar X1000 '%s' returned an unrecognizable response '%s' "
		"to the command '%s'.",
				sca->record->name, command, response );
		}
		break;

	default:
		return mx_sca_default_get_parameter_handler( sca );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cyberstar_x1000_set_parameter( MX_SCA *sca )
{
	static const char fname[] = "mxd_cyberstar_x1000_set_parameter()";

	MX_CYBERSTAR_X1000 *cyberstar_x1000;
	char command[40], on_off[10];
	long time_constant_value;
	mx_status_type mx_status;

	mx_status = mxd_cyberstar_x1000_get_pointers( sca,
						&cyberstar_x1000, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for sca '%s' for parameter type '%s' (%d).",
		fname, sca->record->name,
		mx_get_field_label_string( sca->record,
			sca->parameter_type ),
		sca->parameter_type));

	switch( sca->parameter_type ) {
	case MXLV_SCA_LOWER_LEVEL:
		sprintf(command, ":SENS%d:SCA:LOW %g",
				cyberstar_x1000->address,
				sca->lower_level);

		mx_status = mxd_cyberstar_x1000_command( cyberstar_x1000,
				command, NULL, 0, CYBERSTAR_X1000_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXLV_SCA_UPPER_LEVEL:
		sprintf(command, ":SENS%d:SCA:UPP %g",
				cyberstar_x1000->address,
				sca->upper_level);

		mx_status = mxd_cyberstar_x1000_command( cyberstar_x1000,
				command, NULL, 0, CYBERSTAR_X1000_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXLV_SCA_GAIN:
		sprintf(command, ":INP%d:GAIN %g",
				cyberstar_x1000->address,
				sca->gain);

		mx_status = mxd_cyberstar_x1000_command( cyberstar_x1000,
				command, NULL, 0, CYBERSTAR_X1000_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXLV_SCA_TIME_CONSTANT:
		/* Force the time constant to one of the allowed values,
		 * namely, 300 ns, 500 ns, 1000 ns, or 3000 ns.
		 */

		if ( sca->time_constant < 400.0e-9 ) {
			sca->time_constant = 300.0e-9;
		} else
		if ( sca->time_constant < 750.0e-9 ) {
			sca->time_constant = 500.0e-9;
		} else
		if ( sca->time_constant < 2000.0e-9 ) {
			sca->time_constant = 1000.0e-9;
		} else {
			sca->time_constant = 3000.0e-9;
		}

		time_constant_value = mx_round( 1.0e9 * sca->time_constant );

		sprintf(command, ":SENS%d:PKT %ld",
				cyberstar_x1000->address,
				time_constant_value);

		mx_status = mxd_cyberstar_x1000_command( cyberstar_x1000,
				command, NULL, 0, CYBERSTAR_X1000_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXLV_SCA_MODE:
		sca->sca_mode = MXF_SCA_UNKNOWN_MODE;
		break;

	case MXLV_CYBERSTAR_X1000_HIGH_VOLTAGE:
		if ( (cyberstar_x1000->high_voltage < 250.0)
		  || (cyberstar_x1000->high_voltage > 1250.0) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested high voltage of %g volts for "
			"Cyberstar X1000 '%s' is outside the allowed "
			"range of 250 volts to 1250 volts.",
				cyberstar_x1000->high_voltage,
				sca->record->name );
		}

		sprintf(command, ":SOUR%d:VOLT %g",
				cyberstar_x1000->address,
				cyberstar_x1000->high_voltage );

		mx_status = mxd_cyberstar_x1000_command( cyberstar_x1000,
				command, NULL, 0, CYBERSTAR_X1000_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXLV_CYBERSTAR_X1000_DELAY:
		if ( (cyberstar_x1000->delay < 2.0)
		  || (cyberstar_x1000->delay > 300.0) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested saturation delay of %g seconds for "
			"Cyberstar X1000 '%s' is outside the allowed "
			"range of 2.0 seconds to 300.0 seconds",
				cyberstar_x1000->delay,
				sca->record->name );
		}

		sprintf(command, ":TRIG%d:ECO %g",
				cyberstar_x1000->address,
				10.0 * cyberstar_x1000->delay );

		mx_status = mxd_cyberstar_x1000_command( cyberstar_x1000,
				command, NULL, 0, CYBERSTAR_X1000_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXLV_CYBERSTAR_X1000_FORCED_REMOTE_CONTROL:
		if ( cyberstar_x1000->forced_remote_control == 0 ) {
			strcpy( on_off, "OFF" );
		} else {
			strcpy( on_off, "ON" );
		}

		sprintf(command, ":SYST%d:COMM:REM %s",
				cyberstar_x1000->address, on_off );

		mx_status = mxd_cyberstar_x1000_command( cyberstar_x1000,
				command, NULL, 0, CYBERSTAR_X1000_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXLV_CYBERSTAR_X1000_SECURITY_STATUS:
		if ( cyberstar_x1000->security_status == 0 ) {
			strcpy( on_off, "OFF" );
		} else {
			strcpy( on_off, "ON" );
		}

		sprintf(command, ":SYST%d:SEC %s",
				cyberstar_x1000->address, on_off );

		mx_status = mxd_cyberstar_x1000_command( cyberstar_x1000,
				command, NULL, 0, CYBERSTAR_X1000_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	default:
		return mx_sca_default_get_parameter_handler( sca );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cyberstar_x1000_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_cyberstar_x1000_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG( 2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_CYBERSTAR_X1000_HIGH_VOLTAGE:
		case MXLV_CYBERSTAR_X1000_DELAY:
		case MXLV_CYBERSTAR_X1000_FORCED_REMOTE_CONTROL:
		case MXLV_CYBERSTAR_X1000_SECURITY_STATUS:
			record_field->process_function
					= mxd_cyberstar_x1000_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif

static mx_status_type
mxd_cyberstar_x1000_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	const char fname[] = "mxd_cyberstar_x1000_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_CYBERSTAR_X1000 *cyberstar_x1000;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	cyberstar_x1000 = (MX_CYBERSTAR_X1000 *) (record->record_type_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_CYBERSTAR_X1000_HIGH_VOLTAGE:
		case MXLV_CYBERSTAR_X1000_DELAY:
		case MXLV_CYBERSTAR_X1000_FORCED_REMOTE_CONTROL:
		case MXLV_CYBERSTAR_X1000_SECURITY_STATUS:
			mx_status = mx_sca_get_parameter( record,
						record_field->label_value );

			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_CYBERSTAR_X1000_HIGH_VOLTAGE:
		case MXLV_CYBERSTAR_X1000_DELAY:
		case MXLV_CYBERSTAR_X1000_FORCED_REMOTE_CONTROL:
		case MXLV_CYBERSTAR_X1000_SECURITY_STATUS:
			mx_status = mx_sca_set_parameter( record,
						record_field->label_value );

			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
	}

	return mx_status;
}

/* === Extra functions for the use of this driver. === */

MX_EXPORT mx_status_type
mxd_cyberstar_x1000_command( MX_CYBERSTAR_X1000 *cyberstar_x1000,
			char *command,
			char *response,
			size_t response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxd_cyberstar_x1000_command()";

	char c;
	int i, max_attempts;
	unsigned long sleep_ms, num_input_bytes_available;
	mx_status_type mx_status, mx_status2;

	if ( cyberstar_x1000 == (MX_CYBERSTAR_X1000 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL MX_CYBERSTAR_X1000 pointer passed." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL command buffer pointer passed." );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
			fname, command, cyberstar_x1000->record->name ));
	}

	/* Send the command string. */

	mx_status = mx_rs232_putline( cyberstar_x1000->rs232_record,
						command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The Cyberstar X1000 always sends an ACK character to acknowledge
	 * receipt of the LF terminator for the command that we just sent.
	 * Even if we expect no other response, we must still read and discard
	 * this ACK character.
	 */

	mx_status = mx_rs232_getchar( cyberstar_x1000->rs232_record,
						&c, MXF_232_WAIT );

	if ( mx_status.code == MXE_NOT_READY ) {
		return mx_error( MXE_NOT_READY, fname,
		"No response received from Cyberstar X1000 amplifier '%s' "
		"for command '%s'.  Are you sure it is plugged in "
		"and turned on?", cyberstar_x1000->record->name, command );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( c != MX_ACK ) {
		(void) mx_rs232_discard_unread_input(
					cyberstar_x1000->rs232_record,
					CYBERSTAR_X1000_DEBUG );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Did not receive an ACK acknowledgement character from "
	"Cyberstar X1000 interface '%s' in response to the command '%s'.  "
	"Instead, saw a %#x (%c) character.",
		cyberstar_x1000->record->name, command, c, c );
	}

	/* If we expect a response, then read it in. */

	if ( response != NULL ) {
		mx_status = mx_rs232_getline( cyberstar_x1000->rs232_record,
					response, response_buffer_length,
					NULL, 0 );

		if ( debug_flag & (mx_status.code == MXE_SUCCESS) ) {
			MX_DEBUG(-2,("%s: received '%s' from '%s'",
			fname, response, cyberstar_x1000->record->name ));
		}
	} else {
		if ( debug_flag ) {
			MX_DEBUG(-2,("%s complete.", fname));
		}
	}

	/* If the Cyberstar X1000 echoes the command line back to us, then
	 * we must throw this away.
	 */

	if ( cyberstar_x1000->discard_echoed_command_line ) {
		max_attempts = 100;
		sleep_ms = 1;

		for ( i = 0; i < max_attempts; i++ ) {
			mx_status2 = mx_rs232_num_input_bytes_available(
						cyberstar_x1000->rs232_record,
						&num_input_bytes_available );

			if ( mx_status2.code != MXE_SUCCESS )
				break;

			if ( num_input_bytes_available > 0 )
				break;

			mx_msleep( sleep_ms );
		}

		if ( i >= max_attempts ) {
			if ( strncmp( command, "*RST", 4 ) == 0 ) {
				/* Reset commands do not echo any characters. */
			} else {
				mx_status = mx_error( MXE_TIMED_OUT, fname,
				"Timed out waiting for Cyberstar X1000 '%s' "
				"to echo the command line '%s' back to us.",
				cyberstar_x1000->record->name, command );
			}
		}

		(void) mx_rs232_discard_unread_input(
				cyberstar_x1000->rs232_record, FALSE );
	}

	return mx_status;
}

