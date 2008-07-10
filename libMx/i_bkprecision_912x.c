/*
 * Name:    i_bkprecision_912x.c
 *
 * Purpose: MX interface driver for the BK Precision 912x series
 *          of programmable power supplies.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_BKPRECISION_912X_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_gpib.h"
#include "i_bkprecision_912x.h"
#include "d_bkprecision_912x_timer.h"

MX_RECORD_FUNCTION_LIST mxi_bkprecision_912x_record_function_list = {
	NULL,
	mxi_bkprecision_912x_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_bkprecision_912x_open
};

MX_RECORD_FIELD_DEFAULTS mxi_bkprecision_912x_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_BKPRECISION_912X_STANDARD_FIELDS
};

long mxi_bkprecision_912x_num_record_fields
		= sizeof( mxi_bkprecision_912x_record_field_defaults )
		    / sizeof( mxi_bkprecision_912x_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_bkprecision_912x_rfield_def_ptr
			= &mxi_bkprecision_912x_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_bkprecision_912x_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_bkprecision_912x_create_record_structures()";

	MX_BKPRECISION_912X *bkprecision_912x;

	/* Allocate memory for the necessary structures. */

	bkprecision_912x = (MX_BKPRECISION_912X *)
				malloc( sizeof(MX_BKPRECISION_912X) );

	if ( bkprecision_912x == (MX_BKPRECISION_912X *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_BKPRECISION_912X structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = bkprecision_912x;

	record->record_function_list =
				&mxi_bkprecision_912x_record_function_list;

	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	bkprecision_912x->record = record;

	return MX_SUCCESSFUL_RESULT;
}

#define FREE_912X_STRINGS \
	do {				\
		mx_free(argv);		\
		mx_free(dup_string);	\
	} while(0)

MX_EXPORT mx_status_type
mxi_bkprecision_912x_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_bkprecision_912x_open()";

	MX_BKPRECISION_912X *bkprecision_912x;
	MX_BKPRECISION_912X_TIMER *bkprecision_912x_timer;
	MX_RECORD *interface_record;
	MX_RECORD *current_record;
	char command[40];
	char response[200];
	int argc;
	char **argv;
	char *dup_string;
	unsigned long no_error_checking;
	mx_bool_type timer_found;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL.");
	}

	bkprecision_912x = (MX_BKPRECISION_912X *) record->record_type_struct;

	if ( bkprecision_912x == (MX_BKPRECISION_912X *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BKPRECISION_912X pointer for record '%s' is NULL.",
			record->name);
	}

	interface_record = bkprecision_912x->port_interface.record;

	if ( interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The interface record pointer for BK Precision "
			"power supply '%s' is NULL.", record->name );
	}

	if ( interface_record->mx_class == MXI_RS232 ) {
		/* Throw away any unread characters. */

		mx_status = mx_rs232_discard_unread_input( interface_record,
						MXI_BKPRECISION_912X_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_msleep(500);
	}

	/* Verify that the BK Precision power supply is present by asking
	 * it to identify itself.
	 */

#if MXI_BKPRECISION_912X_DEBUG
	no_error_checking = MXT_BKPRECISION_912X_DEBUG
				| MXT_BKPRECISION_912X_NO_ERROR_CHECKING;
#else
	no_error_checking = MXT_BKPRECISION_912X_NO_ERROR_CHECKING;
#endif

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x, "*IDN?",
						response, sizeof(response),
						no_error_checking );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dup_string = strdup( response );

	if ( dup_string == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to duplicate the BK Precision "
		"response string for record '%s'.", record->name );
	}

	mx_string_split( dup_string, ",", &argc, &argv );

	if ( argc < 4 ) {
		FREE_912X_STRINGS;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The response '%s' from BK Precision power supply '%s' "
		"*IDN? command did not have the expected format of "
		"four strings separated by commas.",
			response, record->name );
	}

	if ( ( strcmp( argv[0], "BK PRECISION" ) != 0 )
	  || ( strncmp( argv[1], "912", 3 ) != 0 ) )
	{
		mx_status = mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"Record '%s' does not appear to be a BK Precision 912x series "
		"power supply.  Instead, it appears to be a model '%s' device "
		"sold by '%s'.", record->name, argv[1], argv[0] );

		FREE_912X_STRINGS;
		return mx_status;
	}

	strlcpy( bkprecision_912x->model_name, argv[1],
			sizeof(bkprecision_912x->model_name) );

#if MXI_BKPRECISION_912X_DEBUG
	MX_DEBUG(-2,
	("%s: Record '%s' is a BK Precision %s, serial number = '%s', "
	"software version = '%s'",
			fname, record->name, bkprecision_912x->model_name,
			argv[2], argv[3]));
#endif

	FREE_912X_STRINGS;

	/* Turn on the standard event status register.  We turn on
	 * all of the bits except for those that the documentation
	 * says are unused.
	 */

#if 0
	snprintf( command, sizeof(command), "*ESE %d", 0xBD );

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x, command,
					NULL, 0, MXT_BKPRECISION_912X_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	/* Clear out any existing error status. */

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
						"SYSTEM:ERROR?",
						response, sizeof(response),
						no_error_checking );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Figure out what function to use for the rear panel port. */

	if ( mx_strncasecmp( "TRIGGER", bkprecision_912x->port_function_name,
			strlen( bkprecision_912x->port_function_name) ) == 0 )
	{
		bkprecision_912x->port_function = MXT_BKPRECISION_912X_TRIGGER;
	} else
	if ( mx_strncasecmp( "RIDFI", bkprecision_912x->port_function_name,
			strlen( bkprecision_912x->port_function_name) ) == 0 )
	{
		bkprecision_912x->port_function = MXT_BKPRECISION_912X_RIDFI;
	} else
	if ( mx_strncasecmp( "DIGITAL", bkprecision_912x->port_function_name,
			strlen( bkprecision_912x->port_function_name) ) == 0 )
	{
		bkprecision_912x->port_function = MXT_BKPRECISION_912X_DIGITAL;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized port function '%s' specified for "
		"BK Precision power supply '%s'.",
			bkprecision_912x->port_function_name,
			record->name );
	}

	/* Configure the port function. */

	switch( bkprecision_912x->port_function ) {
	case MXT_BKPRECISION_912X_TRIGGER:
		strlcpy( command, "PORT:FUNCTION TRIGGER", sizeof(command) );
		break;
	case MXT_BKPRECISION_912X_RIDFI:
		strlcpy( command, "PORT:FUNCTION RIDFI", sizeof(command) );
		break;
	case MXT_BKPRECISION_912X_DIGITAL:
		strlcpy( command, "PORT:FUNCTION DIGITAL", sizeof(command) );
		break;
	}

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x, command,
					NULL, 0, MXI_BKPRECISION_912X_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off the output timer. */

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					"OUTPUT:TIMER OFF", NULL, 0,
					MXI_BKPRECISION_912X_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if the MX database contains a 'bkprecision_912x_timer' record
	 * that depends on our interface record.
	 */

	timer_found = FALSE;

	current_record = record->next_record;

	while ( current_record != record ) {

	    if ( current_record->mx_type == MXT_TIM_BKPRECISION_912X ) {
		bkprecision_912x_timer = current_record->record_type_struct;

		if ( record == bkprecision_912x_timer->bkprecision_912x_record )
		{
		    /* We have found a match, so exit the while() loop. */

		    timer_found = TRUE;
		    break;
		}
	    }

	    current_record = current_record->next_record;
	}

#if 0 && MXI_BKPRECISION_912X_DEBUG
	MX_DEBUG(-2,("%s: timer found = %d", fname, timer_found));
#endif

	/* If a 'bkprecision_912x_timer' record was found in the MX database,
	 * then we turn off the output.  Otherwise, we turn it on.
	 */

	if ( timer_found ) {
		strlcpy( command, "OUTPUT OFF", sizeof(command) );
	} else {
		strlcpy( command, "OUTPUT ON", sizeof(command) );
	}

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
						command, NULL, 0,
						MXI_BKPRECISION_912X_DEBUG );

	return mx_status;
}

static mx_status_type
mxi_bkprecision_912x_getline( MX_BKPRECISION_912X *bkprecision_912x,
				char *response,
				size_t response_buffer_length,
				int debug_flag )
{
	static const char fname[] = "mxi_bkprecision_912x_getline()";

	MX_RECORD *interface_record;
	long gpib_address;
	mx_status_type mx_status;

	if ( bkprecision_912x == (MX_BKPRECISION_912X *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_BKPRECISION_912X pointer passed was NULL." );
	}
	if ( response == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The response pointer passed was NULL." );
	}

	interface_record = bkprecision_912x->port_interface.record;
	gpib_address     = bkprecision_912x->port_interface.address;

	if ( interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The interface record pointer for BK Precision "
		"power supply '%s' is NULL.",
			bkprecision_912x->record->name );
	}

	/* Read the response. */

	switch( interface_record->mx_class ) {
	case MXI_RS232:
		mx_status = mx_rs232_getline( interface_record,
				response, response_buffer_length,
				NULL, 0 );
		break;
	case MXI_GPIB:
		mx_status = mx_gpib_getline(
				interface_record, gpib_address,
				response, response_buffer_length,
				NULL, 0 );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported MX class %lu for interface '%s' used by "
		"BK Precision power supply '%s'",
			interface_record->mx_class,
			interface_record->name,
			bkprecision_912x->record->name );
		break;
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
		    fname, response, bkprecision_912x->record->name ));
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_bkprecision_912x_putline( MX_BKPRECISION_912X *bkprecision_912x,
				char *command,
				int debug_flag )
{
	static const char fname[] = "mxi_bkprecision_912x_putline()";

	MX_RECORD *interface_record;
	long gpib_address;
	mx_status_type mx_status;

	if ( bkprecision_912x == (MX_BKPRECISION_912X *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_BKPRECISION_912X pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer passed was NULL." );
	}

	interface_record = bkprecision_912x->port_interface.record;
	gpib_address     = bkprecision_912x->port_interface.address;

	if ( interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The interface record pointer for BK Precision "
		"power supply '%s' is NULL.",
			bkprecision_912x->record->name );
	}

	/* Send the command. */

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
		    fname, command, bkprecision_912x->record->name ));
	}

	switch( interface_record->mx_class ) {
	case MXI_RS232:
		mx_status = mx_rs232_putline( interface_record,
						command, NULL, 0 );
		break;
	case MXI_GPIB:
		mx_status = mx_gpib_putline( interface_record,
				gpib_address, command, NULL, 0 );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported MX class %lu for interface '%s' used by "
		"BK Precision power supply '%s'",
			interface_record->mx_class,
			interface_record->name,
			bkprecision_912x->record->name );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_bkprecision_912x_command( MX_BKPRECISION_912X *bkprecision_912x,
			char *command,
			char *response,
			size_t response_buffer_length,
			unsigned long transaction_flags )
{
	static const char fname[] = "mxi_bkprecision_912x_command()";

	char status[80];
	mx_bool_type debug_flag, do_error_checking;
	unsigned long low_level_flags;
	int error_code;
	char *error_info;
	mx_status_type mx_status;

	debug_flag = transaction_flags & MXT_BKPRECISION_912X_DEBUG;

	if ( transaction_flags & MXT_BKPRECISION_912X_NO_ERROR_CHECKING ) {
		do_error_checking = FALSE;
	} else {
		do_error_checking = TRUE;
	}

#if 0
	low_level_flags = 1;
#else
	low_level_flags = 0;
#endif

	/* Send the command. */

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
		    fname, command, bkprecision_912x->record->name ));
	}

	mx_status = mxi_bkprecision_912x_putline( bkprecision_912x,
						command, low_level_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the response. */

	if ( response != NULL ) {
		mx_status = mxi_bkprecision_912x_getline( bkprecision_912x,
					response, response_buffer_length,
					low_level_flags );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received '%s' from '%s'",
			    fname, response, bkprecision_912x->record->name ));
		}
	}

	/*---- Did an error occur? ----*/

	if ( do_error_checking ) {

		/* Ask for the status of the most recent command. */

		mx_status = mxi_bkprecision_912x_putline( bkprecision_912x,
					"SYSTEM:ERROR?", low_level_flags );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxi_bkprecision_912x_getline( bkprecision_912x,
						status, sizeof(status),
						low_level_flags );

		/* For some reason, the controller does not always respond
		 * to the SYSTEM:ERROR? command and we get a timeout.  If
		 * this happens, we try the command a second time.
		 */

		switch( mx_status.code ) {
		case MXE_SUCCESS:
			break;
		case MXE_TIMED_OUT:
			mx_status = mxi_bkprecision_912x_putline(
					bkprecision_912x, "SYSTEM:ERROR?",
					low_level_flags );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mxi_bkprecision_912x_getline(
						bkprecision_912x,
						status, sizeof(status),
						low_level_flags );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
			break;
		default:
			return mx_status;
			break;
		}

		error_code = atol( status );

		bkprecision_912x->error_code = error_code;

		if ( error_code != 0 ) {
			error_info = strchr( status, '\'' );

			if ( error_info == NULL ) {
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"Error code %d seen for the command '%s' "
				"sent to BK Precision 912x power supply '%s'.",
					error_code, command,
					bkprecision_912x->record->name );
			}

			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"%s (%d) error seen for the command '%s' sent "
				"to BK Precision 912x power supply '%s'.",
					error_info, error_code, command,
					bkprecision_912x->record->name );
		}
	}

	return mx_status;
}

