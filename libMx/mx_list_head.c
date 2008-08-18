/*
 * Name:    mx_list_head.c
 *
 * Purpose: Support for the "list head" record type.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003-2004, 2006-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_list_head.h"

MX_RECORD_FUNCTION_LIST mxr_list_head_record_function_list = {
	NULL,
	NULL,  /* It is not an error that this entry is NULL. */
	NULL,
	NULL,
	mxr_list_head_print_structure,
	NULL,
	NULL,
	mxr_list_head_open,
	NULL,
	mxr_list_head_finish_delayed_initialization
};

MX_RECORD_FIELD_DEFAULTS mxr_list_head_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXR_LIST_HEAD_STANDARD_FIELDS
};

long mxr_list_head_num_record_fields
		= sizeof( mxr_list_head_record_field_defaults )
			/ sizeof( mxr_list_head_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxr_list_head_rfield_def_ptr
			= &mxr_list_head_record_field_defaults[0];

MX_EXPORT mx_status_type
mxr_create_list_head( MX_RECORD *record )
{
	static const char fname[] = "mxr_create_list_head()";

	MX_LIST_HEAD *list_head_struct;
	MX_RECORD_FIELD *record_field_array;
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	void *field_data_ptr;
	long i;
	mx_status_type mx_status;

	record->mx_superclass = MXR_LIST_HEAD;
	record->mx_class = 0;
	record->mx_type = 0;

	strlcpy( record->name, "mx_database", MXU_RECORD_NAME_LENGTH );

	record->record_flags = MXF_REC_INITIALIZED | MXF_REC_ENABLED;
	record->record_function_list = &mxr_list_head_record_function_list;

	record->precision = 0;
	record->resynchronize = 0;

	/* Create the list head structure itself. */

	list_head_struct = (MX_LIST_HEAD *) malloc( sizeof(MX_LIST_HEAD) );

	if ( list_head_struct == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Out of memory allocating MX_LIST_HEAD structure for list head." );
	}
	
	record->record_superclass_struct = list_head_struct;

	list_head_struct->record = record;

	/* Allocate an array for the list head record fields. */

	record->num_record_fields = mxr_list_head_num_record_fields;

	record_field_array = (MX_RECORD_FIELD *) malloc(
					mxr_list_head_num_record_fields
					* sizeof(MX_RECORD_FIELD) );

	if ( record_field_array == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory allocating record field array for MX list head record." );
	}

	record->record_field_array = record_field_array;

	/* Fill in the record field array. */

	for ( i = 0; i < record->num_record_fields; i++ ) {

		/* Copy default values for this field to the 
		 * MX_RECORD_FIELD structure.
		 */

		record_field = &record_field_array[i];

		record_field_defaults = &mxr_list_head_record_field_defaults[i];

		mx_status = mx_copy_defaults_to_record_field(
				record_field, record_field_defaults );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Construct a pointer to the field data. */

		mx_status = mx_construct_ptr_to_field_data(
			record, record_field_defaults, &field_data_ptr );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		record_field->data_pointer = field_data_ptr;

		record_field->record = record;

		record_field->active = FALSE;
	}

	/* Fill in the list head structure values by hand. */

	list_head_struct->list_is_active = FALSE;
	list_head_struct->fast_mode = FALSE;
	list_head_struct->allow_fast_mode = TRUE;
	list_head_struct->network_debug = FALSE;

	list_head_struct->is_server = FALSE;
	list_head_struct->connection_acl = NULL;
	list_head_struct->fixup_records_in_use = TRUE;
	list_head_struct->num_fixup_records = 0;
	list_head_struct->fixup_record_array = NULL;
	list_head_struct->plotting_enabled = 0;
	list_head_struct->log_handler = NULL;
	list_head_struct->server_protocols_active = 0;
	list_head_struct->handle_table = NULL;
	list_head_struct->application_ptr = NULL;

	list_head_struct->default_precision = 8;

	list_head_struct->mx_version  = MX_MAJOR_VERSION * 1000000L;
	list_head_struct->mx_version += MX_MINOR_VERSION * 1000L;
	list_head_struct->mx_version += MX_UPDATE_VERSION;

	list_head_struct->num_server_records = 0;
	list_head_struct->server_record_array = NULL;

	list_head_struct->client_callback_handle_table = NULL;
	list_head_struct->server_callback_handle_table = NULL;

	list_head_struct->master_timer = NULL;
	list_head_struct->callback_timer = NULL;

	list_head_struct->callback_pipe = NULL;

	list_head_struct->num_poll_callbacks = 0;

	strlcpy( list_head_struct->hostname, "", MXU_HOSTNAME_LENGTH );

	(void) mx_username( list_head_struct->username, MXU_USERNAME_LENGTH );

	strlcpy( list_head_struct->program_name, "", MXU_PROGRAM_NAME_LENGTH );

	strlcpy( list_head_struct->status, "", MXU_FIELD_NAME_LENGTH );

	/* Since the list head record itself is a record, we initialize
	 * the number of records to 1 rather than 0.
	 */

	list_head_struct->num_records = 1;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxr_list_head_print_structure( FILE *file, MX_RECORD *record )
{
	fprintf(file, "Record = '%s',  Type = Record list head.\n",
		record->name);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxr_list_head_open( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxr_list_head_finish_delayed_initialization( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

