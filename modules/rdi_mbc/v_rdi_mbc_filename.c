/*
 * Name:    v_rdi_mbc_filename.c
 *
 * Purpose: MX variable driver for custom filenames used for RDI detectors
 *          by the Molecular Biology Consortium.
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

#define MXV_RDI_MBC_FILENAME_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_variable.h"

#include "v_rdi_mbc_filename.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxv_rdi_mbc_filename_record_function_list = {
	mx_variable_initialize_driver,
	mxv_rdi_mbc_filename_create_record_structures
};

MX_VARIABLE_FUNCTION_LIST mxv_rdi_mbc_filename_variable_function_list = {
	mxv_rdi_mbc_filename_send_variable,
	mxv_rdi_mbc_filename_receive_variable
};

MX_RECORD_FIELD_DEFAULTS mxv_rdi_mbc_filename_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_RDI_MBC_FILENAME_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_STRING_VARIABLE_STANDARD_FIELDS
};

long mxv_rdi_mbc_filename_num_record_fields
		= sizeof( mxv_rdi_mbc_filename_record_field_defaults )
			/ sizeof( mxv_rdi_mbc_filename_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_rdi_mbc_filename_rfield_def_ptr
			= &mxv_rdi_mbc_filename_record_field_defaults[0];

/*---*/

static mx_status_type
mxv_rdi_mbc_filename_get_pointers( MX_VARIABLE *variable,
			MX_RDI_MBC_FILENAME **rdi_mbc_filename,
			MX_VARIABLE **parent_variable,
			const char *calling_fname )
{
	static const char fname[] = "mxv_rdi_mbc_filename_get_pointers()";

	MX_RDI_MBC_FILENAME *rdi_mbc_filename_ptr;
	MX_RECORD *parent_variable_record;
	MX_VARIABLE *parent_variable_ptr;

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	rdi_mbc_filename_ptr = variable->record->record_type_struct;

	if ( rdi_mbc_filename_ptr == (MX_RDI_MBC_FILENAME *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RDI_MBC_FILENAME pointer for record '%s' is NULL.",
			variable->record->name );
	}

	if ( rdi_mbc_filename != (MX_RDI_MBC_FILENAME **) NULL ) {
		*rdi_mbc_filename = rdi_mbc_filename_ptr;
	}

	parent_variable_record = rdi_mbc_filename_ptr->parent_variable_record;

	if ( parent_variable_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The parent_variable_record pointer for record '%s' is NULL.",
				variable->record->name );
	}

	parent_variable_ptr = parent_variable_record->record_class_struct;

	if ( parent_variable_ptr == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VARIABLE pointer for variable record '%s' "
		"used by record '%s' is NULL.",
				parent_variable_record->name,
				variable->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxv_rdi_mbc_filename_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_rdi_mbc_filename_create_record_structures()";

	MX_VARIABLE *variable;
	MX_RDI_MBC_FILENAME *rdi_mbc_filename = NULL;

	variable = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VARIABLE structure." );
	}

	rdi_mbc_filename = (MX_RDI_MBC_FILENAME *)
				malloc( sizeof(MX_RDI_MBC_FILENAME));

	if ( rdi_mbc_filename == (MX_RDI_MBC_FILENAME *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_RDI_MBC_FILENAME structure." );
	}

	record->record_class_struct = variable;
	record->record_type_struct = rdi_mbc_filename;
	record->superclass_specific_function_list = 
			&mxv_rdi_mbc_filename_variable_function_list;
	record->class_specific_function_list = NULL;

	variable->record = record;
	rdi_mbc_filename->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_rdi_mbc_filename_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_rdi_mbc_filename_send_variable()";

	MX_RDI_MBC_FILENAME *rdi_mbc_filename = NULL;
	MX_VARIABLE *parent_variable = NULL;
	MX_RECORD_FIELD *value_field = NULL;
	long num_dimensions, field_type;
	long *dimension_array;
	void *value_ptr;

	char *string_ptr;
	size_t max_string_length;

	mx_status_type mx_status;

	mx_status = mxv_rdi_mbc_filename_get_pointers( variable,
							&rdi_mbc_filename,
							&parent_variable,
							fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field( variable->record,
					"value", &value_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_variable_parameters( variable->record,
						&num_dimensions,
						&dimension_array,
						&field_type,
						&value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	string_ptr = value_ptr;

	max_string_length = dimension_array[0];

	mx_status = mx_set_string_variable( parent_variable->record,
						string_ptr );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_rdi_mbc_filename_receive_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_rdi_mbc_filename_receive_variable()";

	MX_RDI_MBC_FILENAME *rdi_mbc_filename = NULL;
	MX_VARIABLE *parent_variable = NULL;
	MX_RECORD_FIELD *value_field = NULL;
	long num_dimensions, field_type;
	long *dimension_array;
	void *value_ptr;

	char *string_ptr;
	size_t max_string_length;

	mx_status_type mx_status;

	mx_status = mxv_rdi_mbc_filename_get_pointers( variable,
							&rdi_mbc_filename,
							&parent_variable,
							fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field( variable->record,
					"value", &value_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_variable_parameters( variable->record,
						&num_dimensions,
						&dimension_array,
						&field_type,
						&value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	string_ptr = value_ptr;

	max_string_length = dimension_array[0];

	mx_status = mx_get_string_variable( parent_variable->record,
						string_ptr,
						max_string_length );

	return mx_status;
}

