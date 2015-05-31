/*
 * Name:    mx_vfield.c
 *
 * Purpose: Support for local field variables in MX database files.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2012, 2014-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_net.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_variable.h"
#include "mx_vfield.h"

MX_RECORD_FUNCTION_LIST mxv_field_variable_record_function_list = {
	mx_variable_initialize_driver,
	mxv_field_variable_create_record_structures,
	mxv_field_variable_finish_record_initialization,
	NULL,
	NULL,
	mxv_field_variable_open,
};

MX_VARIABLE_FUNCTION_LIST mxv_field_variable_variable_function_list = {
	mxv_field_variable_send_variable,
	mxv_field_variable_receive_variable
};

/********************************************************************/

MX_RECORD_FIELD_DEFAULTS mxv_field_string_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_FIELD_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_STRING_VARIABLE_STANDARD_FIELDS
};

long mxv_field_string_variable_num_record_fields
			= sizeof( mxv_field_string_variable_defaults )
			/ sizeof( mxv_field_string_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_field_string_variable_dptr
			= &mxv_field_string_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_field_char_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_FIELD_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_CHAR_VARIABLE_STANDARD_FIELDS
};

long mxv_field_char_variable_num_record_fields
			= sizeof( mxv_field_char_variable_defaults )
			/ sizeof( mxv_field_char_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_field_char_variable_dptr
			= &mxv_field_char_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_field_uchar_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_FIELD_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_UCHAR_VARIABLE_STANDARD_FIELDS
};

long mxv_field_uchar_variable_num_record_fields
			= sizeof( mxv_field_uchar_variable_defaults )
			/ sizeof( mxv_field_uchar_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_field_uchar_variable_dptr
			= &mxv_field_uchar_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_field_short_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_FIELD_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_SHORT_VARIABLE_STANDARD_FIELDS
};

long mxv_field_short_variable_num_record_fields
			= sizeof( mxv_field_short_variable_defaults )
			/ sizeof( mxv_field_short_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_field_short_variable_dptr
			= &mxv_field_short_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_field_ushort_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_FIELD_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_USHORT_VARIABLE_STANDARD_FIELDS
};

long mxv_field_ushort_variable_num_record_fields
			= sizeof( mxv_field_ushort_variable_defaults )
			/ sizeof( mxv_field_ushort_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_field_ushort_variable_dptr
			= &mxv_field_ushort_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_field_bool_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_FIELD_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_BOOL_VARIABLE_STANDARD_FIELDS
};

long mxv_field_bool_variable_num_record_fields
			= sizeof( mxv_field_bool_variable_defaults )
			/ sizeof( mxv_field_bool_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_field_bool_variable_dptr
			= &mxv_field_bool_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_field_long_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_FIELD_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_LONG_VARIABLE_STANDARD_FIELDS
};

long mxv_field_long_variable_num_record_fields
			= sizeof( mxv_field_long_variable_defaults )
			/ sizeof( mxv_field_long_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_field_long_variable_dptr
			= &mxv_field_long_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_field_ulong_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_FIELD_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_ULONG_VARIABLE_STANDARD_FIELDS
};

long mxv_field_ulong_variable_num_record_fields
			= sizeof( mxv_field_ulong_variable_defaults )
			/ sizeof( mxv_field_ulong_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_field_ulong_variable_dptr
			= &mxv_field_ulong_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_field_int64_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_FIELD_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_INT64_VARIABLE_STANDARD_FIELDS
};

long mxv_field_int64_variable_num_record_fields
			= sizeof( mxv_field_int64_variable_defaults )
			/ sizeof( mxv_field_int64_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_field_int64_variable_dptr
			= &mxv_field_int64_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_field_uint64_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_FIELD_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_UINT64_VARIABLE_STANDARD_FIELDS
};

long mxv_field_uint64_variable_num_record_fields
			= sizeof( mxv_field_uint64_variable_defaults )
			/ sizeof( mxv_field_uint64_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_field_uint64_variable_dptr
			= &mxv_field_uint64_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_field_float_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_FIELD_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_FLOAT_VARIABLE_STANDARD_FIELDS
};

long mxv_field_float_variable_num_record_fields
			= sizeof( mxv_field_float_variable_defaults )
			/ sizeof( mxv_field_float_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_field_float_variable_dptr
			= &mxv_field_float_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_field_double_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_FIELD_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_DOUBLE_VARIABLE_STANDARD_FIELDS
};

long mxv_field_double_variable_num_record_fields
			= sizeof( mxv_field_double_variable_defaults )
			/ sizeof( mxv_field_double_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_field_double_variable_dptr
			= &mxv_field_double_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_field_hex_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_FIELD_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_HEX_VARIABLE_STANDARD_FIELDS
};

long mxv_field_hex_variable_num_record_fields
			= sizeof( mxv_field_hex_variable_defaults )
			/ sizeof( mxv_field_hex_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_field_hex_variable_dptr
			= &mxv_field_hex_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_field_record_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_FIELD_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_DOUBLE_VARIABLE_STANDARD_FIELDS
};

long mxv_field_record_variable_num_record_fields
			= sizeof( mxv_field_record_variable_defaults )
			/ sizeof( mxv_field_record_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_field_record_variable_dptr
			= &mxv_field_record_variable_defaults[0];

/********************************************************************/

static mx_status_type
mxv_field_variable_get_pointers( MX_VARIABLE *variable,
				MX_FIELD_VARIABLE **field_variable,
				const char *calling_fname )
{
	static const char fname[] = "mxv_field_variable_get_pointers()";

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( variable->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for MX_VARIABLE structure %p passed by '%s' is NULL.",
			variable, calling_fname );
	}

	if ( (variable->record->mx_superclass != MXR_VARIABLE)
	  || (variable->record->mx_class != MXV_FIELD) ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The record '%s' passed by '%s' is not a local field variable.",
			variable->record->name, calling_fname );
	}

	if ( field_variable != (MX_FIELD_VARIABLE **) NULL ) {

		*field_variable = (MX_FIELD_VARIABLE *)
				variable->record->record_class_struct;

		if ( *field_variable == (MX_FIELD_VARIABLE *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_FIELD_VARIABLE pointer for variable '%s' passed by '%s' is NULL.",
				variable->record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/********************************************************************/

MX_EXPORT mx_status_type
mxv_field_variable_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_field_variable_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_FIELD_VARIABLE *field_variable;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCAN structure." );
	}

	field_variable = (MX_FIELD_VARIABLE *)
				malloc( sizeof(MX_FIELD_VARIABLE) );

	if ( field_variable == (MX_FIELD_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_FIELD_VARIABLE structure." );
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	field_variable->record = record;
	field_variable->internal_field = NULL;

	field_variable->external_record = NULL;
	field_variable->external_field = NULL;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = field_variable;
	record->record_type_struct = NULL;

	record->superclass_specific_function_list =
				&mxv_field_variable_variable_function_list;
	record->class_specific_function_list = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_field_variable_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_field_variable_finish_record_initialization()";

	MX_VARIABLE *variable;
	MX_FIELD_VARIABLE *field_variable;
	MX_RECORD_FIELD *internal_field, *external_field;
	char *duplicate;
	int argc, split_status, saved_errno;
	char **argv;
	char record_name[MXU_RECORD_NAME_LENGTH+1];
	char field_name[MXU_FIELD_NAME_LENGTH+1];
	size_t *external_sizeof_array;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	variable = (MX_VARIABLE *) record->record_superclass_struct;

	mx_status = mxv_field_variable_get_pointers( variable,
						&field_variable, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Parse the external field name that we were given. */

	duplicate = strdup( field_variable->external_field_name );

	if ( duplicate == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to parse the "
		"external field name '%s' for variable record '%s'.",
			field_variable->external_field_name,
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
			field_variable->external_field_name );
	}

	if ( argc == 0 ) {
		mx_free( argv );
		mx_free( duplicate );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"No record or field name were found in the "
		"external_field_name '%s' variable for record '%s'.",
			field_variable->external_field_name,
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

	field_variable->external_record = mx_get_record( record, record_name );

	if ( field_variable->external_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"External record '%s' used by variable record '%s' "
		"was not found in the MX database.",
			record_name, record->name );
	}

	/* Find the external record field that is used by this variable. */

	mx_status = mx_find_record_field( field_variable->external_record,
						field_name, &external_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field_variable->external_field = external_field;

	/* Save a pointer to this record's own 'value' field. */

	mx_status = mx_find_record_field( record, "value", &internal_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field_variable->internal_field = internal_field;

	/* Save some parameters from the original contents of the internal
	 * field.  If we are supposed to write to the external field at
	 * startup time, then we will need these values then.
	 */

	field_variable->original_datatype = internal_field->datatype;
	field_variable->original_num_dimensions =
					internal_field->num_dimensions;
	field_variable->original_dimension = internal_field->dimension;
	field_variable->original_data_element_size =
					internal_field->data_element_size;
	field_variable->original_data_pointer = internal_field->data_pointer;
	field_variable->original_value_pointer =
				mx_get_field_value_pointer( internal_field );

	/* Now modify the internal field to match the external field. */

	mx_status = mx_get_datatype_sizeof_array( external_field->datatype,
						&external_sizeof_array );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( external_field->num_dimensions == 0 ) {
		internal_field->num_dimensions = 1;
		internal_field->dimension[0] = 1;
		internal_field->data_element_size[0] = external_sizeof_array[0];
	} else
	if ( external_field->num_dimensions == 1 ) {
		internal_field->num_dimensions = 1;
		internal_field->dimension[0] = external_field->dimension[0];
		internal_field->data_element_size[0]
					= external_field->data_element_size[0];
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Field variables with dimensions higher than 1 are "
		"not supported.  Field record '%s' wanted %ld dimensions.",
			record->name, external_field->num_dimensions );
	}

	internal_field->flags = external_field->flags;
	internal_field->flags |= MXFF_IN_SUMMARY;

	internal_field->datatype = external_field->datatype;

	internal_field->data_pointer = external_field->data_pointer;

#if 0
	fprintf( stderr, "Internal " );
	mx_print_field_definition( stderr, internal_field );
	fprintf( stderr, "\n" );
	fprintf( stderr, "External " );
	mx_print_field_definition( stderr, external_field );
	fprintf( stderr, "\n" );
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_field_variable_open( MX_RECORD *record )
{
	static const char fname[] = "mxv_field_variable_open()";

	MX_VARIABLE *variable = NULL;
	MX_FIELD_VARIABLE *field_variable = NULL;
	MX_RECORD_FIELD *internal_field = NULL;
	MX_RECORD_FIELD *external_field = NULL;
	void *external_value_ptr = NULL;
	unsigned long flags;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	variable = (MX_VARIABLE *) record->record_superclass_struct;

	mx_status = mxv_field_variable_get_pointers( variable,
						&field_variable, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*----*/

	internal_field = field_variable->internal_field;

	if ( internal_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The internal_field pointer of variable '%s' is NULL.",
			variable->record->name );
	}

	/*----*/

	external_field = field_variable->external_field;

	if ( external_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The external_field pointer of variable '%s' is NULL.",
			variable->record->name );
	}

	external_value_ptr = mx_get_field_value_pointer(external_field);

	if ( external_value_ptr == (void *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The external_value_ptr for external field '%s.%s' used by "
		"variable '%s' is NULL.",
			external_field->record->name,
			external_field->name,
			variable->record->name );
	}

	/*----*/

	flags = field_variable->external_field_flags;

	if ( flags & MXF_FIELD_VARIABLE_READ_ONLY ) {
		/* Enforce the read-only nature of this variable by setting
		 * the MXFF_READ_ONLY flag in the internal 'value' field's
		 * 'flags' element.
		 */

		internal_field->flags |= MXFF_READ_ONLY;
	} else
	if ( (external_field->flags) & MXFF_READ_ONLY ) {
		/* In addition, if the external field itself has the
		 * read-only flag set, then we set the internal 'value'
		 * field to read-only as well.
		 */

		internal_field->flags |= MXFF_READ_ONLY;
	} else
	if ( flags & MXF_FIELD_VARIABLE_WRITE_ON_OPEN ) {

		/* If we are to write a value from the field variable to the
		 * external field at startup time, then we must copy the
		 * starting value of the field from the original saved value
		 * array found at field_variable->original_value_pointer.
		 */

		mx_status = mx_initialize_record_processing(
				field_variable->external_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_convert_and_copy_array(
				field_variable->original_value_pointer,
				field_variable->original_datatype,
				field_variable->original_num_dimensions,
				field_variable->original_dimension,
				field_variable->original_data_element_size,
				external_value_ptr,
				external_field->datatype,
				external_field->num_dimensions,
				external_field->dimension,
				external_field->data_element_size );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_process_record_field(
				field_variable->external_record,
				external_field,
				MX_PROCESS_PUT, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_field_variable_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_field_variable_send_variable()";

	MX_FIELD_VARIABLE *field_variable;
	unsigned long flags;
	mx_status_type mx_status;

	mx_status = mxv_field_variable_get_pointers( variable,
						&field_variable, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

	flags = field_variable->external_field_flags;

	if ( flags & MXF_FIELD_VARIABLE_READ_ONLY ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
		"MX local field variable '%s' is configured to be read only.",
			variable->record->name );
	}

	/*---*/

	mx_status = mx_initialize_record_processing(
				field_variable->external_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

	/* The internal field contents point to the external field contents,
	 * so all we need to do is to process the external record.
	 */

	mx_status = mx_process_record_field( field_variable->external_record,
						field_variable->external_field,
						MX_PROCESS_PUT, NULL );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_field_variable_receive_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_field_variable_receive_variable()";

	MX_FIELD_VARIABLE *field_variable;
	mx_status_type mx_status;

	mx_status = mxv_field_variable_get_pointers( variable,
						&field_variable, fname );


	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

	mx_status = mx_initialize_record_processing(
				field_variable->external_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

	/* The internal field contents point to the external field contents,
	 * so all we need to do is to process the external record.
	 */

	mx_status = mx_process_record_field( field_variable->external_record,
						field_variable->external_field,
						MX_PROCESS_GET, NULL );

	return mx_status;
}

