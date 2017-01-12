/*
 * Name:    d_wti_nps_relay.c
 *
 * Purpose: MX driver for using a Western Telematics Inc. Network Power Switch
 *          outlet as an MX relay record.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_WTI_NPS_RELAY_DEBUG 	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_relay.h"
#include "i_wti_nps.h"
#include "d_wti_nps_relay.h"

/* Initialize the generic relay driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_wti_nps_relay_record_function_list = {
	NULL,
	mxd_wti_nps_relay_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_wti_nps_relay_open
};

MX_RELAY_FUNCTION_LIST mxd_wti_nps_relay_relay_function_list = {
	mxd_wti_nps_relay_relay_command,
	mxd_wti_nps_relay_get_relay_status
};

MX_RECORD_FIELD_DEFAULTS mxd_wti_nps_relay_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RELAY_STANDARD_FIELDS,
	MXD_WTI_NPS_RELAY_STANDARD_FIELDS
};

long mxd_wti_nps_relay_num_record_fields
	= sizeof( mxd_wti_nps_relay_rf_defaults )
		/ sizeof( mxd_wti_nps_relay_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_wti_nps_relay_rfield_def_ptr
			= &mxd_wti_nps_relay_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_wti_nps_relay_get_pointers( MX_RELAY *relay,
		      MX_WTI_NPS_RELAY **wti_nps_relay,
		      MX_WTI_NPS **wti_nps,
		      const char *calling_fname )
{
	MX_WTI_NPS_RELAY *wti_nps_relay_ptr;
	MX_RECORD *wti_nps_record;

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
		"The MX_RELAY pointer passed is NULL." );
	}

	wti_nps_relay_ptr = (MX_WTI_NPS_RELAY *)
				relay->record->record_type_struct;

	if ( wti_nps_relay_ptr
			== (MX_WTI_NPS_RELAY *) NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"The MX_WTI_NPS_RELAY pointer "
		"for record '%s' is NULL.",
			relay->record->name );
	}

	if ( wti_nps_relay
			!= (MX_WTI_NPS_RELAY **) NULL )
	{
		*wti_nps_relay = wti_nps_relay_ptr;
	}

	if ( wti_nps != (MX_WTI_NPS **) NULL ) {

		wti_nps_record =
		    wti_nps_relay_ptr->wti_nps_record;

		if ( wti_nps_record == (MX_RECORD *) NULL ) {
			return mx_error(
			MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
			"The wti_nps_record pointer used by "
			"relay record '%s' is NULL.",
				relay->record->name );
		}

		*wti_nps = (MX_WTI_NPS *)
			wti_nps_record->record_type_struct;

		if ( *wti_nps == (MX_WTI_NPS *) NULL ) {
			return mx_error(
			MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
			"The MX_WTI_NPS pointer for "
			"Synaccess Netbooter record '%s' used by "
			"relay record '%s' is NULL.",
				wti_nps_record->name,
				relay->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_wti_nps_relay_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_wti_nps_relay_create_record_structures()";

        MX_RELAY *relay;
        MX_WTI_NPS_RELAY *wti_nps_relay;

        /* Allocate memory for the necessary structures. */

        relay = (MX_RELAY *) malloc(sizeof(MX_RELAY));

        if ( relay == (MX_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for an MX_RELAY structure." );
        }

        wti_nps_relay = (MX_WTI_NPS_RELAY *) malloc( sizeof(MX_WTI_NPS_RELAY) );

        if ( wti_nps_relay == (MX_WTI_NPS_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
    "Cannot allocate memory for an MX_WTI_NPS_RELAY structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = relay;
        record->record_type_struct = wti_nps_relay;
        record->class_specific_function_list
			= &mxd_wti_nps_relay_relay_function_list;

        relay->record = record;
	wti_nps_relay->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_wti_nps_relay_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_wti_nps_relay_open()";

	MX_RELAY *relay = NULL;
	MX_WTI_NPS_RELAY *wti_nps_relay = NULL;
	MX_WTI_NPS *wti_nps = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed is NULL." );
	}

	relay = (MX_RELAY *) record->record_class_struct;

	mx_status = mxd_wti_nps_relay_get_pointers( relay,
						&wti_nps_relay,
						&wti_nps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_wti_nps_relay_get_relay_status( relay );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_wti_nps_relay_relay_command( MX_RELAY *relay )
{
	static const char fname[] = "mxd_wti_nps_relay_relay_command()";

	MX_WTI_NPS_RELAY *wti_nps_relay = NULL;
	MX_WTI_NPS *wti_nps = NULL;
	MX_RECORD *rs232_record = NULL;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_wti_nps_relay_get_pointers( relay,
						&wti_nps_relay,
						&wti_nps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232_record = wti_nps->rs232_record;

	if ( relay->relay_command == MXF_OPEN_RELAY ) {
		snprintf( command, sizeof(command),
			"/OFF %lu,Y", wti_nps_relay->plug_number );
	} else {
		snprintf( command, sizeof(command),
			"/ON %lu,Y", wti_nps_relay->plug_number );
	}

	mx_status = mx_rs232_putline( rs232_record, command,
					NULL, MXD_WTI_NPS_RELAY_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait until the command completes. */

	mx_status = mx_rs232_discard_until_string( rs232_record, "NPS>",
					TRUE, MXD_WTI_NPS_RELAY_DEBUG, 5.0 );
						

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_wti_nps_relay_get_relay_status( MX_RELAY *relay )
{
	static const char fname[] = "mxd_wti_nps_relay_get_relay_status()";

	MX_WTI_NPS_RELAY *wti_nps_relay = NULL;
	MX_WTI_NPS *wti_nps = NULL;
	MX_RECORD *rs232_record = NULL;
	char response[80];
	char plug_status_format[80];
	char plug_status[20];
	int num_items;
	unsigned long plug_number;
	double timeout;
	mx_status_type mx_status;

	mx_status = mxd_wti_nps_relay_get_pointers( relay,
						&wti_nps_relay,
						&wti_nps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232_record = wti_nps->rs232_record;

	/* Display the Status Screen. */

	mx_status = mx_rs232_putline( rs232_record, "/S",
					NULL, MXD_WTI_NPS_RELAY_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard the input until we see the string 'Plug', which
	 * begins the column header line.
	 */

	timeout = 5.0;

	mx_status = mx_rs232_discard_until_string( rs232_record, "Plug", TRUE,
						MXD_WTI_NPS_RELAY_DEBUG,
						timeout );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard the rest of the header line. */

	mx_status = mx_rs232_getline_with_timeout( rs232_record,
					response, sizeof(response),
					NULL, MXD_WTI_NPS_RELAY_DEBUG,
					timeout );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The next line is just a line of dashes '-', so discard that. */

	mx_status = mx_rs232_getline_with_timeout( rs232_record,
					response, sizeof(response),
					NULL, MXD_WTI_NPS_RELAY_DEBUG,
					timeout );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response[0] != '-' ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not see the initial dash line in the response '%s' "
		"from WTI NPS relay '%s'.",
			response, relay->record->name );
	}

	/* Read lines until we either find the plug number we are looking for
	 * or until we see another line of dashes.
	 */

	while (TRUE) {
		mx_status = mx_rs232_getline_with_timeout( rs232_record,
					response, sizeof(response),
					NULL, MXD_WTI_NPS_RELAY_DEBUG,
					timeout );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( response[0] == '-' ) {
			/* Unfortunately, we got to the line of dashes. */

			mx_status = mx_rs232_discard_until_string(
						rs232_record, "NPS>", TRUE,
						MXD_WTI_NPS_RELAY_DEBUG,
						timeout );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			return mx_error( MXE_NOT_FOUND, fname,
			"Plug number %lu for WTI NPS controller '%s' "
			"was not found.",
				wti_nps_relay->plug_number,
				wti_nps->record->name );
		}

		/* Look for the plug number. */

		num_items = sscanf( response, "%lu", &plug_number );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Did not find a plug number in response '%s' "
			"from WTI NPS controller '%s'.",
				response, wti_nps->record->name );
		}

		if ( plug_number == wti_nps_relay->plug_number ) {

			/* This is the plug number we are looking for.
			 * The fifth string on this line should be the
			 * status of the plug.
			 */

			snprintf( plug_status_format,
				sizeof(plug_status_format),
				"%%*s %%*s %%*s %%*s %%%lds",
				(unsigned long) (sizeof(plug_status) - 1) );

#if 1
			MX_DEBUG(-2,("%s: plug_status_format = '%s'",
				fname, plug_status_format));
#endif

			num_items = sscanf( response,
					plug_status_format, plug_status );

			if ( num_items != 1 ) {
				return mx_error( MXE_DEVICE_IO_ERROR, fname,
				"Did not find the plug status for plug %lu in "
				"response '%s' from WTI NPS controller '%s'.",
					wti_nps_relay->plug_number,
					response, wti_nps->record->name );
			}

			/* Parse the plug status. */

			if ( strcmp( plug_status, "ON" ) == 0 ) {
				relay->relay_status = MXF_RELAY_IS_CLOSED;
			} else
			if ( strcmp( plug_status, "OFF" ) == 0 ) {
				relay->relay_status = MXF_RELAY_IS_OPEN;
			} else {
				relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

				return mx_error( MXE_DEVICE_IO_ERROR, fname,
				"Did not find a valid plug status in the "
				"response '%s' for WTI NPS relay '%s'.",
					response, relay->record->name );
			}

			/* We have found the plug number that we were
			 * looking for, so discard the rest of the
			 * plug status screen and then return.
			 */

			mx_status = mx_rs232_discard_until_string(
						rs232_record, "NPS>", TRUE,
						MXD_WTI_NPS_RELAY_DEBUG,
						timeout );

			return mx_status;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

