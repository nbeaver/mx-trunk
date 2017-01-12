/*
 * Name:    i_wti_nps.c
 *
 * Purpose: MX interface driver for Western Telematic Inc. Network
 *          Power Switchs.
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

#define MXI_WTI_NPS_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_ascii.h"
#include "mx_rs232.h"
#include "i_wti_nps.h"

MX_RECORD_FUNCTION_LIST mxi_wti_nps_record_function_list = {
	NULL,
	mxi_wti_nps_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_wti_nps_open
};

MX_RECORD_FIELD_DEFAULTS mxi_wti_nps_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_WTI_NPS_STANDARD_FIELDS
};

long mxi_wti_nps_num_record_fields
	= sizeof( mxi_wti_nps_rf_defaults )
		    / sizeof( mxi_wti_nps_rf_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxi_wti_nps_rfield_def_ptr
			= &mxi_wti_nps_rf_defaults[0];

MX_EXPORT mx_status_type
mxi_wti_nps_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_wti_nps_create_record_structures()";

	MX_WTI_NPS *wti_nps;

	/* Allocate memory for the necessary structures. */

	wti_nps = (MX_WTI_NPS *) malloc( sizeof(MX_WTI_NPS) );

	if ( wti_nps == (MX_WTI_NPS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_WTI_NPS structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = wti_nps;

	record->record_function_list = &mxi_wti_nps_record_function_list;

	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	wti_nps->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_wti_nps_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_wti_nps_open()";

	MX_WTI_NPS *wti_nps = NULL;
	MX_RECORD *rs232_record = NULL;
	char response[80];
	unsigned long num_input_bytes_available;
	int num_items;
	double timeout;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL.");
	}

	wti_nps = (MX_WTI_NPS *) record->record_type_struct;

	if ( wti_nps == (MX_WTI_NPS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_WTI_NPS pointer for record '%s' is NULL.",
			record->name);
	}

	rs232_record = wti_nps->rs232_record;

	/* If it is working, the WTI NPS controller should send us 
	 * some text when we connected.  Did it?
	 */

	mx_msleep(100);

	mx_status = mx_rs232_num_input_bytes_available( rs232_record,
						&num_input_bytes_available );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_input_bytes_available == 0 ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"During the initial attempt to connect to WTI NPS "
		"controller '%s', we did not see any characters sent by it.  "
		"Are you sure the controller itself is turned on?",
			record->name );
	}

	timeout = 5.0;

	/* At this point, the WTI NPS controller may have put up a 
	 * password prompt or it may not have.  We must check to see.
	 */

	mx_status = mx_rs232_getline_with_timeout( rs232_record,
				response, sizeof(response),
				NULL, MXI_WTI_NPS_DEBUG,
				timeout );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strncmp( response, "Password", 8 ) == 0 ) {

		/* It appears that the controller has sent us a password
		 * prompt, so we should reply with the password.
		 */

		mx_status = mx_rs232_putline( rs232_record,
					wti_nps->password,
					NULL, MXI_WTI_NPS_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Read in the next line from the controller. */

		mx_status = mx_rs232_getline_with_timeout( rs232_record,
					response, sizeof(response),
					NULL, MXI_WTI_NPS_DEBUG,
					timeout );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* The line currently in the response buffer, should contain the
	 * version number of the WTI NPS controller.  Try to parse that.
	 */

	num_items = sscanf( response, "Network Power Switch v%lf",
				&(wti_nps->version_number) );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"We did not find the WTI NPS version number "
		"in the response '%s' received from controller '%s'.  "
		"Either a password failed or this is not a "
		"Western Telematic Inc. Network Power Switch.",
			response, record->name );
	}

	/* If we get here, then the controller should have sent us an
	 * initial help screen.  Discard the help screen.
	 */

	mx_status = mx_rs232_discard_until_string( rs232_record, "NPS>",
					TRUE, MXI_WTI_NPS_DEBUG, 5.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If we get here, then we have successfully started communicating
	 * with the power switch.  Celebrate good times, come on!
	 */

	return MX_SUCCESSFUL_RESULT;
}

