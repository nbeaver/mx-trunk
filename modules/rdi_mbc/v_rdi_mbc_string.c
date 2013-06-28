/*
 * Name:    v_rdi_mbc_string.c
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

#define MXV_RDI_MBC_STRING_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_epics.h"
#include "mx_variable.h"

#include "v_rdi_mbc_string.h"

/* The suffix is ####.img */

#define MXU_RDI_MBC_SUFFIX_LENGTH	8

#define MXU_RDI_MBC_MAX_PREFIX_LENGTH	\
		(MXU_EPICS_STRING_LENGTH - MXU_RDI_MBC_SUFFIX_LENGTH)

#define MXU_RDI_MBC_MAX_CHUNK_LENGTH	( (MXU_EPICS_STRING_LENGTH - 1)/2 )

/*---*/

MX_RECORD_FUNCTION_LIST mxv_rdi_mbc_string_record_function_list = {
	mx_variable_initialize_driver,
	mxv_rdi_mbc_string_create_record_structures,
	mxv_rdi_mbc_string_finish_record_initialization
};

MX_VARIABLE_FUNCTION_LIST mxv_rdi_mbc_string_variable_function_list = {
	mxv_rdi_mbc_string_send_variable,
	mxv_rdi_mbc_string_receive_variable
};

MX_RECORD_FIELD_DEFAULTS mxv_rdi_mbc_string_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_RDI_MBC_STRING_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_STRING_VARIABLE_STANDARD_FIELDS
};

long mxv_rdi_mbc_string_num_record_fields
		= sizeof( mxv_rdi_mbc_string_record_field_defaults )
			/ sizeof( mxv_rdi_mbc_string_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_rdi_mbc_string_rfield_def_ptr
			= &mxv_rdi_mbc_string_record_field_defaults[0];

/*---*/

static mx_status_type
mxv_rdi_mbc_string_get_pointers( MX_VARIABLE *variable,
			MX_RDI_MBC_STRING **rdi_mbc_string,
			const char *calling_fname )
{
	static const char fname[] = "mxv_rdi_mbc_string_get_pointers()";

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

	if ( rdi_mbc_string == (MX_RDI_MBC_STRING **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RDI_MBC_STRING pointer passed by '%s' "
		"for record '%s' was NULL.",
			calling_fname, variable->record->name );
	}


	*rdi_mbc_string = (MX_RDI_MBC_STRING *)
				variable->record->record_type_struct;

	if ( (*rdi_mbc_string) == (MX_RDI_MBC_STRING *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RDI_MBC_STRING pointer for record '%s' is NULL.",
			variable->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxv_rdi_mbc_string_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_rdi_mbc_string_create_record_structures()";

	MX_VARIABLE *variable;
	MX_RDI_MBC_STRING *rdi_mbc_string = NULL;

	variable = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VARIABLE structure." );
	}

	rdi_mbc_string = (MX_RDI_MBC_STRING *)
				malloc( sizeof(MX_RDI_MBC_STRING));

	if ( rdi_mbc_string == (MX_RDI_MBC_STRING *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_RDI_MBC_STRING structure." );
	}

	record->record_superclass_struct = variable;
	record->record_class_struct = NULL;
	record->record_type_struct = rdi_mbc_string;
	record->superclass_specific_function_list = 
			&mxv_rdi_mbc_string_variable_function_list;
	record->class_specific_function_list = NULL;

	variable->record = record;
	rdi_mbc_string->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_rdi_mbc_string_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_rdi_mbc_string_finish_record_initialization()";

	MX_VARIABLE *variable = NULL;
	MX_RDI_MBC_STRING *rdi_mbc_string = NULL;
	MX_DRIVER *mx_driver = NULL;
	const char *driver_name;
	char *duplicate;
	int argc, split_status, saved_errno;
	char **argv;
	char record_name[MXU_RECORD_NAME_LENGTH+1];
	char field_name[MXU_FIELD_NAME_LENGTH+1];
	long internal_num_dimensions;
	long *internal_dimension_array;
	long internal_field_type;
	void *internal_value_ptr;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	variable = (MX_VARIABLE *) record->record_superclass_struct;

	mx_status = mxv_rdi_mbc_string_get_pointers( variable,
						&rdi_mbc_string, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Figure out what type of RDI MBC string record this is. */
#if 1
	mx_driver = mx_get_driver_for_record( record );

	MX_DEBUG(-2,("%s: '%s' driver = %p", fname, record->name, mx_driver));
#endif

	driver_name = mx_get_driver_name( record );

#if MXV_RDI_MBC_STRING_DEBUG
	MX_DEBUG(-2,("%s: '%s' driver_name = '%s'",
		fname, record->name, driver_name));
#endif

	if ( strcmp( driver_name, "rdi_mbc_string" ) == 0 ) {
		rdi_mbc_string->string_type = MXT_RDI_MBC_STRING;
	} else
	if ( strcmp( driver_name, "rdi_mbc_filename" ) == 0 ) {
		rdi_mbc_string->string_type = MXT_RDI_MBC_FILENAME;
	} else
	if ( strcmp( driver_name, "rdi_mbc_datafile_prefix" ) == 0 ) {
		rdi_mbc_string->string_type = MXT_RDI_MBC_DATAFILE_PREFIX;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The driver name '%s' for record '%s' is not recognized.",
		driver_name, record->name );
			
	}

#if MXV_RDI_MBC_STRING_DEBUG
	MX_DEBUG(-2,("%s: '%s' string_type = %ld",
		fname, record->name, rdi_mbc_string->string_type));
#endif

	/* Save a pointer to this record's 'value' field. */

	mx_status = mx_find_record_field( record, "value",
					&(rdi_mbc_string->internal_field) );

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
			rdi_mbc_string->internal_field->name );
	}
	if ( internal_field_type != MXFT_STRING ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The field type %ld for field '%s.%s' is not MXFT_STRING.",
			internal_field_type,
			record->name,
			rdi_mbc_string->internal_field->name );
	}

	rdi_mbc_string->internal_string_ptr = (char *) internal_value_ptr;
	rdi_mbc_string->internal_string_length =
				(size_t) internal_dimension_array[0];

	/* Parse the external field name that we were given. */

	duplicate = strdup( rdi_mbc_string->external_field_name );

	if ( duplicate == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to parse the "
		"external field name '%s' for variable record '%s'.",
			rdi_mbc_string->external_field_name,
			record->name );
	}

	split_status = mx_string_split( duplicate, ".", &argc, &argv );

	if ( split_status != 0 ) {
		saved_errno = errno;

		mx_free( duplicate );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unexpected error code %d (error message = '%s') "
		"occurred when trying to split the external field name '%s' "
		"using a '.' character.",
			saved_errno, strerror(saved_errno),
			rdi_mbc_string->external_field_name );
	}

	if ( argc == 0 ) {
		mx_free( argv );
		mx_free( duplicate );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"No record or field name were found in the "
		"external_field_name '%s' variable for record '%s'.",
			rdi_mbc_string->external_field_name,
			record->name );
	} else
	if ( argc == 1 ) {
		strlcpy( record_name, argv[0], sizeof(record_name) );
		strlcpy( field_name, "value", sizeof(field_name) );
	} else {
		strlcpy( record_name, argv[0], sizeof(record_name) );
		strlcpy( field_name, argv[1], sizeof(field_name) );
	}

	mx_free( argv );
	mx_free( duplicate );

	/* Find the external record. */

	rdi_mbc_string->external_record = mx_get_record( record, record_name );

	if ( rdi_mbc_string->external_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"External record '%s' used by variable record '%s' "
		"was not found in the MX database.",
			record_name, record->name );
	}

	/* Find the external record field that is used by this variable. */

	mx_status = mx_find_record_field( rdi_mbc_string->external_record,
					field_name,
					&(rdi_mbc_string->external_field) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Is this field a 1-dimensional string field? */

	if ( rdi_mbc_string->external_field->num_dimensions != 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"External field '%s' is not a 1-dimensional field.",
			rdi_mbc_string->external_field_name );
	}
	if ( rdi_mbc_string->external_field->datatype != MXFT_STRING ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"External field '%s' is not a string field.",
			rdi_mbc_string->external_field_name );
	}

	/* Find and save the external string length and pointer. */

	rdi_mbc_string->external_string_ptr = (char *)
		mx_get_field_value_pointer( rdi_mbc_string->external_field );

	rdi_mbc_string->external_string_length =
			rdi_mbc_string->external_field->dimension[0];

	/* Check for some string length error conditions. */

	if ( rdi_mbc_string->internal_string_length != 40 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The string length for variable '%s' is not "
			"40 characters, which is not compatible with "
			"EPICS string PVs.  Instead, it is %ld characters.",
			record->name,
			rdi_mbc_string->internal_string_length );
	}
	if ( rdi_mbc_string->external_string_length < 40 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The string length of external field '%s' "
			"used by variable '%s' is %ld characters, "
			"which is less than the recommended "
			"value of 40 characters or more, "
			"for use with EPICS string PVs.",
				rdi_mbc_string->external_field_name,
				record->name,
				rdi_mbc_string->external_string_length );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_rdi_mbc_string_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_rdi_mbc_string_send_variable()";

	MX_RDI_MBC_STRING *rdi_mbc_string = NULL;
	size_t prefix_length;
	mx_status_type mx_status;

	mx_status = mxv_rdi_mbc_string_get_pointers( variable,
						&rdi_mbc_string, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXV_RDI_MBC_STRING_DEBUG
	MX_DEBUG(-2,("%s: sending '%s'",
		fname, rdi_mbc_string->internal_string_ptr));
#endif

	switch( rdi_mbc_string->string_type ) {
	case MXT_RDI_MBC_DATAFILE_PREFIX:
		prefix_length = strlen( rdi_mbc_string->internal_string_ptr );

		if ( prefix_length <= MXU_RDI_MBC_MAX_PREFIX_LENGTH ) {

			strlcpy( rdi_mbc_string->external_string_ptr,
				rdi_mbc_string->internal_string_ptr,
				rdi_mbc_string->external_string_length );
		} else {
			strlcpy( rdi_mbc_string->external_string_ptr,
				rdi_mbc_string->internal_string_ptr,
				MXU_RDI_MBC_MAX_PREFIX_LENGTH );
		}
		
		strlcat( rdi_mbc_string->external_string_ptr,
			"####.img",
			rdi_mbc_string->external_string_length );
		break;
	default:
		strlcpy( rdi_mbc_string->external_string_ptr,
			rdi_mbc_string->internal_string_ptr,
			rdi_mbc_string->external_string_length );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_rdi_mbc_string_receive_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_rdi_mbc_string_receive_variable()";

	MX_RDI_MBC_STRING *rdi_mbc_string = NULL;
	size_t string_length;
	char *end_chunk_ptr, *suffix_ptr;
	mx_status_type mx_status;

	mx_status = mxv_rdi_mbc_string_get_pointers( variable,
						&rdi_mbc_string, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXV_RDI_MBC_STRING_DEBUG
	MX_DEBUG(-2,("%s: receiving '%s'",
		fname, rdi_mbc_string->external_string_ptr));
#endif

	switch( rdi_mbc_string->string_type ) {
	case MXT_RDI_MBC_FILENAME:
		string_length = strlen( rdi_mbc_string->external_string_ptr );

		if ( string_length < MXU_EPICS_STRING_LENGTH ) {
			strlcpy( rdi_mbc_string->internal_string_ptr,
				rdi_mbc_string->external_string_ptr,
				rdi_mbc_string->internal_string_length );
		} else {
			strlcpy( rdi_mbc_string->internal_string_ptr,
				rdi_mbc_string->external_string_ptr,
				MXU_RDI_MBC_MAX_CHUNK_LENGTH );

			strlcat( rdi_mbc_string->internal_string_ptr,
				"~",
				rdi_mbc_string->internal_string_length );

			end_chunk_ptr = rdi_mbc_string->external_string_ptr
					+ string_length
					- MXU_RDI_MBC_MAX_CHUNK_LENGTH;

			strlcat( rdi_mbc_string->internal_string_ptr,
				end_chunk_ptr,
				rdi_mbc_string->internal_string_length );
		}
		break;
	case MXT_RDI_MBC_DATAFILE_PREFIX:
		string_length = strlen( rdi_mbc_string->external_string_ptr );

		suffix_ptr = rdi_mbc_string->external_string_ptr
				+ string_length - MXU_RDI_MBC_SUFFIX_LENGTH;

		if ( string_length > (MXU_EPICS_STRING_LENGTH + 1) ) {

			strlcpy( rdi_mbc_string->internal_string_ptr,
			"** MX DATAFILE PATTERN TOO LONG **",
			rdi_mbc_string->internal_string_length );
		} else
		if ( strcmp( suffix_ptr, "####.img" ) != 0 ) {

			strlcpy( rdi_mbc_string->internal_string_ptr,
			"** ILLEGAL MX DATAFILE SUFFIX **",
			rdi_mbc_string->internal_string_length );
		} else {
			strlcpy( rdi_mbc_string->internal_string_ptr,
				rdi_mbc_string->external_string_ptr,
				rdi_mbc_string->internal_string_length );
		}
		break;
	case MXT_RDI_MBC_STRING:
	default:
		strlcpy( rdi_mbc_string->internal_string_ptr,
			rdi_mbc_string->external_string_ptr,
			rdi_mbc_string->internal_string_length );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

