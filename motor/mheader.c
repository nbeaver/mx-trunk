/*
 * Name:    mheader.c
 *
 * Purpose: Prompt for changes to the scan header.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>

#include "motor.h"
#include "mdialog.h"
#include "mx_variable.h"

int
motor_prompt_for_scan_header( MX_SCAN *scan )
{
	MX_RECORD *variable_record;
	char variable_name[40];
	char variable_stem[20];
	char prompt[100];
	char new_value[200];
	int i, status, header_string_length;
	mx_length_type variable_length;
	mx_status_type mx_status;
	void *pointer_to_value;

	/* Have we been told not to prompt for header lines? */

	if ( motor_header_prompt_on == FALSE ) {
		return SUCCESS;
	}

	switch( scan->datafile.type ) {
	case MXDF_SFF:
		strlcpy(variable_stem, "sff_header", sizeof (variable_stem));
		break;
	case MXDF_XAFS:
		strlcpy(variable_stem, "xafs_header", sizeof (variable_stem));
		break;
	default:
		/* All datafile types not mentioned are assumed to not
		 * need any pre-scan changes.
		 */
		return SUCCESS;
		break;
	}

	for ( i = 1; i < 100; i++ ) {
		snprintf( variable_name, sizeof(variable_name),
			"%s%d", variable_stem, i );

		variable_record = mx_get_record(
				motor_record_list, variable_name );

		/* If we have reached the end of the list of scan headers,
		 * return to our caller.
		 */

		if ( variable_record == NULL )
			return SUCCESS;

		mx_status = mx_get_1d_array( variable_record,
			MXFT_STRING, &variable_length, &pointer_to_value );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		snprintf( prompt, sizeof(prompt), "Header #%d -> ", i );

		header_string_length = sizeof( new_value ) - 1;

		status = motor_get_string( output, prompt, pointer_to_value,
					&header_string_length, new_value );

		if ( status != SUCCESS )
			return status;

		strlcpy( (char *)pointer_to_value,
				new_value, variable_length - 1 );

		mx_status = mx_send_variable( variable_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	}

	return SUCCESS;
}

