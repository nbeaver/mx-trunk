/*
 * Name:    d_synaccess_netbooter_ainput.c
 *
 * Purpose: Driver for the Stanford Research Systems SIM980 summing ainput.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SYNACCESS_NETBOOTER_AINPUT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "i_synaccess_netbooter.h"
#include "d_synaccess_netbooter_ainput.h"

/* Initialize the ainput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_sa_netbooter_ainput_record_function_list = {
	NULL,
	mxd_sa_netbooter_ainput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_sa_netbooter_ainput_open
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_sa_netbooter_ainput_analog_input_function_list =
{
	mxd_sa_netbooter_ainput_read
};

/* SIM980 analog input data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_sa_netbooter_ainput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_LONG_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_SYNACCESS_NETBOOTER_AINPUT_STANDARD_FIELDS
};

long mxd_sa_netbooter_ainput_num_record_fields
		= sizeof( mxd_sa_netbooter_ainput_rf_defaults )
		  / sizeof( mxd_sa_netbooter_ainput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_sa_netbooter_ainput_rfield_def_ptr
			= &mxd_sa_netbooter_ainput_rf_defaults[0];

/* === */

static mx_status_type
mxd_sa_netbooter_ainput_get_pointers( MX_ANALOG_INPUT *ainput,
	      MX_SYNACCESS_NETBOOTER_AINPUT **synaccess_netbooter_ainput,
	      MX_SYNACCESS_NETBOOTER **synaccess_netbooter,
	      const char *calling_fname )
{
	MX_SYNACCESS_NETBOOTER_AINPUT *synaccess_netbooter_ainput_ptr;
	MX_RECORD *synaccess_netbooter_record;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
		"The MX_ANALOG_INPUT pointer passed is NULL." );
	}

	synaccess_netbooter_ainput_ptr = (MX_SYNACCESS_NETBOOTER_AINPUT *)
				ainput->record->record_type_struct;

	if ( synaccess_netbooter_ainput_ptr
			== (MX_SYNACCESS_NETBOOTER_AINPUT *) NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"The MX_SYNACCESS_NETBOOTER_AINPUT pointer "
		"for record '%s' is NULL.",
			ainput->record->name );
	}

	if ( synaccess_netbooter_ainput
			!= (MX_SYNACCESS_NETBOOTER_AINPUT **) NULL )
	{
		*synaccess_netbooter_ainput = synaccess_netbooter_ainput_ptr;
	}

	if ( synaccess_netbooter != (MX_SYNACCESS_NETBOOTER **) NULL ) {

		synaccess_netbooter_record =
		    synaccess_netbooter_ainput_ptr->synaccess_netbooter_record;

		if ( synaccess_netbooter_record == (MX_RECORD *) NULL ) {
			return mx_error(
			MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
			"The synaccess_netbooter_record pointer used by "
			"ainput record '%s' is NULL.",
				ainput->record->name );
		}

		*synaccess_netbooter = (MX_SYNACCESS_NETBOOTER *)
			synaccess_netbooter_record->record_type_struct;

		if ( *synaccess_netbooter == (MX_SYNACCESS_NETBOOTER *) NULL ) {
			return mx_error(
			MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
			"The MX_SYNACCESS_NETBOOTER pointer for "
			"Synaccess Netbooter record '%s' used by "
			"ainput record '%s' is NULL.",
				synaccess_netbooter_record->name,
				ainput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_sa_netbooter_ainput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_sa_netbooter_ainput_create_record_structures()";

	MX_ANALOG_INPUT *ainput;
	MX_SYNACCESS_NETBOOTER_AINPUT *synaccess_netbooter_ainput = NULL;

	/* Allocate memory for the necessary structures. */

	ainput = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_ANALOG_INPUT structure." );
	}

	synaccess_netbooter_ainput = (MX_SYNACCESS_NETBOOTER_AINPUT *)
			malloc( sizeof(MX_SYNACCESS_NETBOOTER_AINPUT) );

	if ( synaccess_netbooter_ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for MX_SYNACCESS_NETBOOTER_AINPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = ainput;
	record->record_type_struct = synaccess_netbooter_ainput;
	record->class_specific_function_list
			= &mxd_sa_netbooter_ainput_analog_input_function_list;

	ainput->record = record;
	synaccess_netbooter_ainput->record = record;

	ainput->subclass = MXT_AIN_LONG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sa_netbooter_ainput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_sa_netbooter_ainput_open()";

	MX_ANALOG_INPUT *ainput;
	MX_SYNACCESS_NETBOOTER_AINPUT *synaccess_netbooter_ainput = NULL;
	MX_SYNACCESS_NETBOOTER *synaccess_netbooter = NULL;
	size_t length;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	ainput = (MX_ANALOG_INPUT *) record->record_class_struct;

	mx_status = mxd_sa_netbooter_ainput_get_pointers( ainput,
						&synaccess_netbooter_ainput,
						&synaccess_netbooter, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	length = strlen( synaccess_netbooter_ainput->input_type_name );

	if ( strncmp( synaccess_netbooter_ainput->input_type_name,
			"current", length ) == 0 )
	{
		synaccess_netbooter_ainput->input_type = 
				MXT_SYNACCESS_NETBOOTER_AINPUT_CURRENT;
	}
	else
	if ( strncmp( synaccess_netbooter_ainput->input_type_name,
			"temperature", length ) == 0 )
	{
		synaccess_netbooter_ainput->input_type = 
				MXT_SYNACCESS_NETBOOTER_AINPUT_TEMPERATURE;
	} else {
		synaccess_netbooter_ainput->input_type = -1;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized input type '%s' seen for record '%s'.  "
		"The allowed types are 'current' and 'temperature'.",
			synaccess_netbooter_ainput->input_type_name,
			record->name );
	}

	mx_warning( "The '%s' driver has not been tested with real hardware.",
		mx_get_driver_name( record ) );

	mx_status = mxd_sa_netbooter_ainput_read( ainput );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sa_netbooter_ainput_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_sa_netbooter_ainput_read()";

	MX_SYNACCESS_NETBOOTER_AINPUT *synaccess_netbooter_ainput = NULL;
	MX_SYNACCESS_NETBOOTER *synaccess_netbooter = NULL;
	char response[80];
	char *first_comma_ptr = NULL;
	char *last_comma_ptr = NULL;
	char *comma_ptr = NULL;
	char *value_ptr = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sa_netbooter_ainput_get_pointers( ainput,
						&synaccess_netbooter_ainput,
						&synaccess_netbooter, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_synaccess_netbooter_command( synaccess_netbooter,
					"$A5", response, sizeof(response),
					MXD_SYNACCESS_NETBOOTER_AINPUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	first_comma_ptr = strchr( response, ',' );

	if ( first_comma_ptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Synaccess netBooter controller '%s' does not support "
		"current or temperature measurements for record '%s'.  "
		"All it supports is turning on or off the AC output ports.",
			synaccess_netbooter->record->name,
			ainput->record->name );
	}

	last_comma_ptr = strrchr( response, ',' );

	if ( last_comma_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Somehow the response '%s' to command '$A5' for "
		"Synaccess netBooter controller '%s' has a first comma, "
		"but not a last comma.  This should be totally impossible, "
		"but somehow it happened.  "
		"In this circumstance, MX may fail in unexpected ways.",
			response, synaccess_netbooter->record->name );
	}

	/*---*/

	switch( synaccess_netbooter_ainput->input_type ) {
	case MXT_SYNACCESS_NETBOOTER_AINPUT_CURRENT:
		/* FIXME: The logic for this case may be wrong.  The only
		 * way to know is to try it for a netBooter controller that
		 * supports current measurement.  The outstanding question
		 * is whether only one overall current measurement is
		 * provided, or whether each port can report its current
		 * as a separate measurement.
		 */

		value_ptr = first_comma_ptr + 1;

		/* Null terminate the value string, if necessary. */

		comma_ptr = strchr( value_ptr, ',' );

		if ( comma_ptr == last_comma_ptr ) {
			return mx_error( MXE_NOT_AVAILABLE, fname,
			"The response '%s' from Synaccess netBooter "
			"controller '%s' does not contain a "
			"current measurement.", response,
				synaccess_netbooter->record->name );
		}

		if ( comma_ptr != NULL ) {
			*comma_ptr = '\0';
		}
		break;
	case MXT_SYNACCESS_NETBOOTER_AINPUT_TEMPERATURE:
		value_ptr = last_comma_ptr + 1;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized input type '%s' seen for record '%s'.  "
		"The allowed types are 'current' and 'temperature'.",
			synaccess_netbooter_ainput->input_type_name,
			ainput->record->name );
		break;
	}

	/*---*/

	ainput->raw_value.long_value = atol( value_ptr );

	return MX_SUCCESSFUL_RESULT;
}

