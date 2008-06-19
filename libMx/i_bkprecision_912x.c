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
	char response[40];
	int argc;
	char **argv;
	char *dup_string;
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

	/* Verify that the BK Precision power supply is present by asking
	 * it to identify itself.
	 */

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x, "*IDN?",
						response, sizeof(response),
						MXI_BKPRECISION_912X_DEBUG );

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
	("%s: Record '%s' is a BK Precision %s, software version = '%s', "
	"serial number = '%s'",
			fname, record->name, bkprecision_912x->model_name,
			argv[2], argv[3]));
#endif

	FREE_912X_STRINGS;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_bkprecision_912x_command( MX_BKPRECISION_912X *bkprecision_912x,
			char *command,
			char *response,
			size_t response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxi_bkprecision_912x_command()";

	MX_RECORD *interface_record;
	long gpib_address;
	mx_status_type mx_status;

	if ( bkprecision_912x == (MX_BKPRECISION_912X *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_BKPRECISION_912X pointer passed was NULL." );
	}

	interface_record = bkprecision_912x->port_interface.record;
	gpib_address     = bkprecision_912x->port_interface.address;

	if ( interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The interface record pointer for BK Precision "
		"power supply '%s' is NULL.",
			bkprecision_912x->record->name );
	}

	if ( command != NULL ) {
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

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( response != NULL ) {
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
	}

	return MX_SUCCESSFUL_RESULT;
}

