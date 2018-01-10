/*
 * Name:    v_rdi_mbc_pathname_builder.c
 *
 * Purpose: MX variable driver for custom pathnames used for RDI detectors
 *          by the Molecular Biology Consortium.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013, 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXV_RDI_MBC_PATHNAME_BUILDER_DEBUG			FALSE

#define MXV_RDI_MBC_PATHNAME_BUILDER_DEBUG_SOURCE_FIELDS	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_variable.h"

#include "v_rdi_mbc_pathname_builder.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxv_rdi_mbc_pathname_builder_record_function_list = {
	mxv_rdi_mbc_pathname_builder_initialize_driver,
	mxv_rdi_mbc_pathname_builder_create_record_structures,
	mxv_rdi_mbc_pathname_builder_finish_record_initialization
};

MX_VARIABLE_FUNCTION_LIST mxv_rdi_mbc_pathname_builder_variable_function_list = {
	mxv_rdi_mbc_pathname_builder_send_variable,
	mxv_rdi_mbc_pathname_builder_receive_variable
};

/* Record field defaults for 'rdi_mbc_pathname_builder'. */

MX_RECORD_FIELD_DEFAULTS mxv_rdi_mbc_pathname_builder_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_RDI_MBC_PATHNAME_BUILDER_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_STRING_VARIABLE_STANDARD_FIELDS
};

long mxv_rdi_mbc_pathname_builder_num_record_fields
	= sizeof( mxv_rdi_mbc_pathname_builder_record_field_defaults )
	/ sizeof( mxv_rdi_mbc_pathname_builder_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_rdi_mbc_pathname_builder_rfield_def_ptr
		= &mxv_rdi_mbc_pathname_builder_record_field_defaults[0];

/*---*/

static mx_status_type
mxv_rdi_mbc_pathname_builder_get_pointers( MX_VARIABLE *variable,
			MX_RDI_MBC_PATHNAME_BUILDER **rdi_mbc_pathname_builder,
			const char *calling_fname )
{
	static const char fname[] =
			"mxv_rdi_mbc_pathname_builder_get_pointers()";

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( variable->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_VARIABLE %p is NULL.",
			variable );
	}

	if ( rdi_mbc_pathname_builder == (MX_RDI_MBC_PATHNAME_BUILDER **) NULL )
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RDI_MBC_PATHNAME_BUILDER pointer passed by '%s' "
		"for record '%s' was NULL.",
			calling_fname, variable->record->name );
	}


	*rdi_mbc_pathname_builder = (MX_RDI_MBC_PATHNAME_BUILDER *)
				variable->record->record_type_struct;

	if ((*rdi_mbc_pathname_builder) == (MX_RDI_MBC_PATHNAME_BUILDER *) NULL)
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_RDI_MBC_PATHNAME_BUILDER pointer for record '%s' is NULL.",
			variable->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxv_rdi_mbc_pathname_builder_initialize_driver( MX_DRIVER *driver )
{
	MX_RECORD_FIELD_DEFAULTS *field;
	long num_source_fields_index;
	long num_source_fields_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_variable_initialize_driver( driver );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Construct a varargs cookie for 'num_source_fields'. */

	mx_status = mx_find_record_field_defaults_index( driver,
						"num_source_fields",
						&num_source_fields_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( num_source_fields_index, 0,
					&num_source_fields_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the 'source_field_name_array' field. */

	mx_status = mx_find_record_field_defaults( driver,
					"source_field_name_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_source_fields_varargs_cookie;

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_rdi_mbc_pathname_builder_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_rdi_mbc_pathname_builder_create_record_structures()";

	MX_VARIABLE *variable;
	MX_RDI_MBC_PATHNAME_BUILDER *rdi_mbc_pathname_builder = NULL;

	variable = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VARIABLE structure." );
	}

	rdi_mbc_pathname_builder = (MX_RDI_MBC_PATHNAME_BUILDER *)
				malloc( sizeof(MX_RDI_MBC_PATHNAME_BUILDER));

	if ( rdi_mbc_pathname_builder == (MX_RDI_MBC_PATHNAME_BUILDER *) NULL )
	{
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_RDI_MBC_PATHNAME_BUILDER structure.");
	}

	record->record_superclass_struct = variable;
	record->record_class_struct = NULL;
	record->record_type_struct = rdi_mbc_pathname_builder;
	record->superclass_specific_function_list = 
			&mxv_rdi_mbc_pathname_builder_variable_function_list;
	record->class_specific_function_list = NULL;

	variable->record = record;
	rdi_mbc_pathname_builder->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_rdi_mbc_pathname_builder_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_rdi_mbc_pathname_builder_finish_record_initialization()";

	MX_VARIABLE *variable = NULL;
	MX_RDI_MBC_PATHNAME_BUILDER *rdi_mbc_pathname_builder = NULL;
	char *duplicate;
	int argc, split_status, saved_errno;
	char **argv;
	char destination_record_name[MXU_RECORD_NAME_LENGTH+1];
	char destination_field_name[MXU_FIELD_NAME_LENGTH+1];

	long internal_num_dimensions;
	long *internal_dimension_array;
	long internal_field_type;
	void *internal_value_ptr;

	char *source_record_field_name;
	char *source_record_name, *source_field_name;
	char *period_ptr;
	long i;

	MX_RECORD *source_record;
	MX_RECORD_FIELD *source_field;

	static char local_value_field_name[] = "value";

	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	variable = (MX_VARIABLE *) record->record_superclass_struct;

	mx_status = mxv_rdi_mbc_pathname_builder_get_pointers( variable,
					&rdi_mbc_pathname_builder, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Save a pointer to this variable's 'value' field. */

	mx_status = mx_find_record_field( record, "value",
			&(rdi_mbc_pathname_builder->internal_value_field) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the length and string pointer for this field. */

	mx_status = mx_get_variable_parameters( record,
						&internal_num_dimensions,
						&internal_dimension_array,
						&internal_field_type,
						&internal_value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( internal_num_dimensions != 1 ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The number of dimensions %ld for field %s.%s is not 1.",
			internal_num_dimensions,
			record->name,
			rdi_mbc_pathname_builder->internal_value_field->name );
	}
	if ( internal_field_type != MXFT_STRING ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The field type %ld for field '%s.%s' is not MXFT_STRING.",
			internal_field_type,
			record->name,
			rdi_mbc_pathname_builder->internal_value_field->name );
	}

	rdi_mbc_pathname_builder->internal_string_ptr =
				(char *) internal_value_ptr;

	rdi_mbc_pathname_builder->internal_string_length =
				(size_t) internal_dimension_array[0];

	/* Parse the destination field name that we were given. */

	duplicate = strdup( rdi_mbc_pathname_builder->destination_field_name );

	if ( duplicate == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to parse the "
		"destination field name '%s' for variable record '%s'.",
			rdi_mbc_pathname_builder->destination_field_name,
			record->name );
	}

	split_status = mx_string_split( duplicate, ".", &argc, &argv );

	if ( split_status != 0 ) {
		saved_errno = errno;

		mx_free( duplicate );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unexpected error code %d (error message = '%s') "
		"occurred when trying to split the destination field name '%s' "
		"using a '.' character.",
			saved_errno, strerror(saved_errno),
			rdi_mbc_pathname_builder->destination_field_name );
	}

	if ( argc == 0 ) {
		mx_free( argv );
		mx_free( duplicate );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"No record or field name were found in the "
		"destination_field_name '%s' variable for record '%s'.",
			rdi_mbc_pathname_builder->destination_field_name,
			record->name );
	} else
	if ( argc == 1 ) {
		strlcpy( destination_record_name, argv[0],
					sizeof(destination_record_name) );
		strlcpy( destination_field_name, "value",
					sizeof(destination_field_name) );
	} else {
		strlcpy( destination_record_name, argv[0],
					sizeof(destination_record_name) );
		strlcpy( destination_field_name, argv[1],
					sizeof(destination_field_name) );
	}

	mx_free( argv );
	mx_free( duplicate );

	/* Find the destination record. */

	rdi_mbc_pathname_builder->destination_record =
			mx_get_record( record, destination_record_name );

	if ( rdi_mbc_pathname_builder->destination_record
		== (MX_RECORD *) NULL )
	{
		return mx_error( MXE_NOT_FOUND, fname,
		"Destination record '%s' used by variable record '%s' "
		"was not found in the MX database.",
			destination_record_name, record->name );
	}

	/* Find the destination field that is used by this variable. */

	mx_status = mx_find_record_field(
			rdi_mbc_pathname_builder->destination_record,
			destination_field_name,
			&(rdi_mbc_pathname_builder->destination_field) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Is the destination field a 1-dimensional string field? */

	if ( rdi_mbc_pathname_builder->destination_field->num_dimensions != 1 ) 
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Destination field '%s' is not a 1-dimensional field.",
			rdi_mbc_pathname_builder->destination_field_name );
	}
	if ( rdi_mbc_pathname_builder->destination_field->datatype
		!= MXFT_STRING )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Destination field '%s' is not a string field.",
			rdi_mbc_pathname_builder->destination_field_name );
	}

	/* Find and save the destination string length and pointer. */

	rdi_mbc_pathname_builder->destination_string_ptr = (char *)
    mx_get_field_value_pointer( rdi_mbc_pathname_builder->destination_field );

	rdi_mbc_pathname_builder->destination_string_length =
		rdi_mbc_pathname_builder->destination_field->dimension[0];

	/* Check for some string length error conditions. */

	if ( rdi_mbc_pathname_builder->internal_string_length != 40 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The string length for variable '%s' is not "
			"40 characters, which is not compatible with "
			"EPICS string PVs.  Instead, it is %ld characters.",
			record->name,
		(long) rdi_mbc_pathname_builder->internal_string_length );
	}

	if ( rdi_mbc_pathname_builder->destination_string_length < 40 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The string length of destination field '%s' "
			"used by variable '%s' is %ld characters, "
			"which is less than the recommended "
			"value of 40 characters or more, "
			"for use with EPICS string PVs.",
				destination_field_name,
				record->name,
		(long) rdi_mbc_pathname_builder->destination_string_length );
	}

	/* Allocate and initialize the 'source_field_array' structure. */

	rdi_mbc_pathname_builder->source_field_array = (MX_RECORD_FIELD **)
		calloc( rdi_mbc_pathname_builder->num_source_fields,
			sizeof(MX_RECORD_FIELD *) );

	if ( rdi_mbc_pathname_builder->source_field_array
		== (MX_RECORD_FIELD **) NULL )
	{
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element array "
		"of MX_RECORD_FIELD pointers for record '%s'.",
			rdi_mbc_pathname_builder->num_source_fields,
			record->name );
	}

	for ( i = 0; i < rdi_mbc_pathname_builder->num_source_fields; i++ ) {
		source_record_field_name =
		 strdup( rdi_mbc_pathname_builder->source_field_name_array[i] );

		/* Find the _last_ period in the source's record field name */

		period_ptr = strrchr( source_record_field_name, '.' );

		if ( period_ptr == (char *) NULL ) {
			/* If the source's record field name does not
			 * have a period '.' in it, then we assume
			 * that the field name is 'value'.
			 */

			source_field_name  = local_value_field_name;
		} else {
			/* Split the specified record field name into
			 * the record name part and the field name part.
			 */

			*period_ptr = '\0';

			source_field_name = ++period_ptr;
		}

		source_record_name = source_record_field_name;

		source_record = mx_get_record( record, source_record_name );

		if ( source_record == (MX_RECORD *) NULL ) {
			mx_status = mx_error( MXE_NOT_FOUND, fname,
			"The specified record '%s' in source name '%s' "
			"for variable '%s' was not found in the MX database.",
				source_record_name,
			   rdi_mbc_pathname_builder->source_field_name_array[i],
				record->name );

			mx_free( source_record_field_name );

			return mx_status;
		}

		mx_status = mx_find_record_field( source_record,
						source_field_name,
						&source_field );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_free( source_record_field_name );

			return mx_status;
		}

		rdi_mbc_pathname_builder->source_field_array[i] = source_field;

		mx_free( source_record_field_name );
	}

#if MXV_RDI_MBC_PATHNAME_BUILDER_DEBUG_SOURCE_FIELDS
	for ( i = 0; i < rdi_mbc_pathname_builder->num_source_fields; i++ ) {
		MX_DEBUG(-2,("%s: source_field_array[%lu] = '%s.%s'",
	  	fname, i,
		rdi_mbc_pathname_builder->source_field_array[i]->record->name,
		rdi_mbc_pathname_builder->source_field_array[i]->name ));
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_rdi_mbc_pathname_builder_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] =
		"mxv_rdi_mbc_pathname_builder_send_variable()";

	MX_RDI_MBC_PATHNAME_BUILDER *rdi_mbc_pathname_builder = NULL;
	MX_RECORD_FIELD *record_field = NULL;
	char *destination_string = NULL;
	size_t max_destination_chars;
	long i;
	mx_status_type mx_status;

	mx_status = mxv_rdi_mbc_pathname_builder_get_pointers( variable,
					&rdi_mbc_pathname_builder, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	destination_string = rdi_mbc_pathname_builder->destination_string_ptr;

	max_destination_chars =
			rdi_mbc_pathname_builder->destination_string_length;

	/* Copy the first field value to the destination_string. */

	record_field = rdi_mbc_pathname_builder->source_field_array[0];

	strlcpy( destination_string,
		mx_get_field_value_pointer( record_field ),
		max_destination_chars );

	/* Concatenate the rest of the field values. */

	for ( i = 1; i < rdi_mbc_pathname_builder->num_source_fields; i++ ) {
		record_field = rdi_mbc_pathname_builder->source_field_array[i];

		strlcat( destination_string,
			mx_get_field_value_pointer( record_field ),
			max_destination_chars );
	}

	/* Invoke the destination string's process function. */

	mx_status = mx_process_record_field( record_field->record,
						record_field,
						MX_PROCESS_PUT, NULL );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_rdi_mbc_pathname_builder_receive_variable( MX_VARIABLE *variable )
{
#if 0
	static const char fname[] =
		"mxv_rdi_mbc_pathname_builder_receive_variable()";
#endif
	/* For now, we just return the value most recently written. */

	return MX_SUCCESSFUL_RESULT;
}

