/*
 * Name:    i_dcc_base.c
 *
 * Purpose: DCC++ and DCC-EX model railroad base stations.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2023 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_process.h"
#include "mx_rs232.h"
#include "i_dcc_base.h"

MX_RECORD_FUNCTION_LIST mxi_dcc_base_record_function_list = {
	NULL,
	mxi_dcc_base_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_dcc_base_open,
	NULL,
	NULL,
	NULL,
	mxi_dcc_base_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_dcc_base_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_DCC_BASE_STANDARD_FIELDS
};

long mxi_dcc_base_num_record_fields
		= sizeof( mxi_dcc_base_record_field_defaults )
			/ sizeof( mxi_dcc_base_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_dcc_base_rfield_def_ptr
			= &mxi_dcc_base_record_field_defaults[0];

/*---*/

static mx_status_type mxi_dcc_base_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );

/*---*/

static mx_status_type
mxi_dcc_base_get_pointers( MX_RECORD *record,
				MX_DCC_BASE **dcc_base,
				const char *calling_fname )
{
	static const char fname[] = "mxi_dcc_base_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( dcc_base == (MX_DCC_BASE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_DCC_BASE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*dcc_base = (MX_DCC_BASE *) (record->record_type_struct);

	if ( *dcc_base == (MX_DCC_BASE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DCC_BASE pointer for record '%s' is NULL.",
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxi_dcc_base_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_dcc_base_create_record_structures()";

	MX_DCC_BASE *dcc_base;

	/* Allocate memory for the necessary structures. */

	dcc_base = (MX_DCC_BASE *) malloc( sizeof(MX_DCC_BASE) );

	if ( dcc_base == (MX_DCC_BASE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_DCC_BASE structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = dcc_base;

	record->record_function_list = &mxi_dcc_base_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	dcc_base->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_dcc_base_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_dcc_base_open()";

	MX_DCC_BASE *dcc_base = NULL;
	mx_status_type mx_status;

	mx_status = mxi_dcc_base_get_pointers( record, &dcc_base, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Figure out what kind of DCC Base station this is. */

	/* Discard any unread input that may be left in the rs232 device. */

	mx_status = mx_rs232_discard_unread_input( dcc_base->rs232_record,
								MXF_232_DEBUG );

	return mx_status;
}

/* ---- */

MX_EXPORT mx_status_type
mxi_dcc_base_command( MX_DCC_BASE *dcc_base,
			unsigned long dcc_base_command_flags )
{
	static const char fname[] = "mxi_dcc_base_command()";

	unsigned long debug_flag;

	mx_status_type mx_status;

	if ( dcc_base == (MX_DCC_BASE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DCC_BASE pointer passed was NULL." );
	}

	memset( dcc_base->response, 0, sizeof(dcc_base->response) );

	/*---*/

	if ( dcc_base->dcc_base_flags & MXF_DCC_BASE_DEBUG ) {
		debug_flag = TRUE;
	} else {
		debug_flag = 0;
	}

	/*---*/

	/* Send the command. */

	mx_status = mx_rs232_putline( dcc_base->rs232_record,
					dcc_base->command, NULL, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read back the response. */

	mx_status = mx_rs232_getline_with_timeout( dcc_base->rs232_record,
						dcc_base->response,
						sizeof( dcc_base->response ),
						NULL, debug_flag, 5.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check for error responses. */

	return MX_SUCCESSFUL_RESULT;
}

/* ---- */

/*==================================================================*/

MX_EXPORT mx_status_type
mxi_dcc_base_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case -1:
			record_field->process_function
					= mxi_dcc_base_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}


/*==================================================================*/

static mx_status_type
mxi_dcc_base_process_function( void *record_ptr,
			void *record_field_ptr,
			void *socket_handler_ptr,
			int operation )
{
	static const char fname[] = "mxi_dcc_base_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_DCC_BASE *dcc_base;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	dcc_base = (MX_DCC_BASE *) record->record_type_struct;

	MXW_UNUSED(dcc_base);

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( record_field->label_value ) {
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal datatype %lu requested for Flowbus interface '%s'.",
			record_field->label_value, record->name );
		break;
	}

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
	}

	return mx_status;
}


/*==================================================================*/

