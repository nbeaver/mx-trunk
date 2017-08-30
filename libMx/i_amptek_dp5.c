/*
 * Name:    i_amptek_dp5.c
 *
 * Purpose: MX driver for Amptek MCAs that use the DP5 protocol.
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

#define MXI_AMPTEK_DP5_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_process.h"
#include "mx_rs232.h"
#include "mx_gpib.h"
#include "i_amptek_dp5.h"

MX_RECORD_FUNCTION_LIST mxi_amptek_dp5_record_function_list = {
	NULL,
	mxi_amptek_dp5_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_amptek_dp5_open,
	NULL,
	NULL,
	NULL,
	mxi_amptek_dp5_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_amptek_dp5_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_AMPTEK_DP5_STANDARD_FIELDS
};

long mxi_amptek_dp5_num_record_fields
		= sizeof( mxi_amptek_dp5_record_field_defaults )
			/ sizeof( mxi_amptek_dp5_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_amptek_dp5_rfield_def_ptr
			= &mxi_amptek_dp5_record_field_defaults[0];

/*---*/

static mx_status_type mxi_amptek_dp5_process_function( void *record_ptr,
						void *record_field_ptr,
						int operation );

/*---*/

MX_EXPORT mx_status_type
mxi_amptek_dp5_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_amptek_dp5_create_record_structures()";

	MX_AMPTEK_DP5 *amptek_dp5;

	/* Allocate memory for the necessary structures. */

	amptek_dp5 = (MX_AMPTEK_DP5 *) malloc( sizeof(MX_AMPTEK_DP5) );

	if ( amptek_dp5 == (MX_AMPTEK_DP5 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AMPTEK_DP5 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = amptek_dp5;

	record->record_function_list = &mxi_amptek_dp5_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	amptek_dp5->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_amptek_dp5_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_amptek_dp5_open()";

	MX_AMPTEK_DP5 *amptek_dp5 = NULL;
	char response[80];
	unsigned long flags;
	mx_status_type mx_status;

#if MXI_AMPTEK_DP5_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif

	amptek_dp5 = (MX_AMPTEK_DP5 *) record->record_type_struct;

	if ( amptek_dp5 == (MX_AMPTEK_DP5 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_AMPTEK_DP5 pointer for record '%s' is NULL.", record->name);
	}

	flags = amptek_dp5->amptek_dp5_flags;

	/* Verify that this is an AMPTEK DP5 interface. */

	mx_status = mxi_amptek_dp5_command( amptek_dp5, "*IDN?",
					response, sizeof(response),
					MXI_AMPTEK_DP5_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_amptek_dp5_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_AMPTEK_DP5_FOO:
			record_field->process_function
					= mxi_amptek_dp5_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}


/*---*/

MX_EXPORT mx_status_type
mxi_amptek_dp5_command( MX_AMPTEK_DP5 *amptek_dp5,
		char *command,
		char *response,
		size_t max_response_length,
		unsigned long amptek_dp5_flags )
{
	static const char fname[] = "mxi_amptek_dp5_command()";

	mx_bool_type debug;

	if ( amptek_dp5 == (MX_AMPTEK_DP5 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AMPTEK_DP5 pointer passed was NULL." );
	}

	if ( amptek_dp5_flags & MXF_AMPTEK_DP5_DEBUG ) {
		debug = TRUE;
	} else
	if ( amptek_dp5->amptek_dp5_flags & MXF_AMPTEK_DP5_DEBUG ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

/*==================================================================*/

static mx_status_type
mxi_amptek_dp5_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxi_amptek_dp5_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_AMPTEK_DP5 *amptek_dp5;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	amptek_dp5 = (MX_AMPTEK_DP5 *) record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_AMPTEK_DP5_FOO:
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
		case MXLV_AMPTEK_DP5_FOO:
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

