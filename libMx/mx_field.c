/*
 * Name:     mx_field.c
 *
 * Purpose:  Generic MX_RECORD_FIELD support.
 *
 *           Please note that the functions in this file are prototyped
 *           in libMx/mx_record.h
 *
 * Author:   William Lavender
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 1999-2016, 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_inttypes.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_array.h"
#include "mx_unistd.h"

#include "mx_variable.h"

#include "mx_scan.h"

#include "mx_rs232.h"
#include "mx_camac.h"

#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "mx_motor.h"
#include "mx_scaler.h"
#include "mx_timer.h"

typedef struct {
	long type_code;
	const char type_text[30];
} MX_FIELD_TYPE_NAME;

/*=====================================================================*/

MX_EXPORT long
mx_get_datatype_from_datatype_name( const char *datatype_name )
{
	static const char fname[] = "mx_get_datatype_from_datatype_name()";

	if ( datatype_name == NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"The datatype_name pointer passed was NULL." );

		return (-1L);
	}

	if ( mx_strcasecmp( datatype_name, "string" ) == 0 ) {
		return MXFT_STRING;
	}
	if ( mx_strcasecmp( datatype_name, "char" ) == 0 ) {
		return MXFT_CHAR;
	}
	if ( mx_strcasecmp( datatype_name, "uchar" ) == 0 ) {
		return MXFT_UCHAR;
	}
	if ( mx_strcasecmp( datatype_name, "short" ) == 0 ) {
		return MXFT_SHORT;
	}
	if ( mx_strcasecmp( datatype_name, "ushort" ) == 0 ) {
		return MXFT_USHORT;
	}
	if ( mx_strcasecmp( datatype_name, "bool" ) == 0 ) {
		return MXFT_BOOL;
	}
	if ( mx_strcasecmp( datatype_name, "long" ) == 0 ) {
		return MXFT_LONG;
	}
	if ( mx_strcasecmp( datatype_name, "ulong" ) == 0 ) {
		return MXFT_ULONG;
	}
	if ( mx_strcasecmp( datatype_name, "float" ) == 0 ) {
		return MXFT_FLOAT;
	}
	if ( mx_strcasecmp( datatype_name, "double" ) == 0 ) {
		return MXFT_DOUBLE;
	}
	if ( mx_strcasecmp( datatype_name, "hex" ) == 0 ) {
		return MXFT_HEX;
	}
	if ( mx_strcasecmp( datatype_name, "int64" ) == 0 ) {
		return MXFT_INT64;
	}
	if ( mx_strcasecmp( datatype_name, "uint64" ) == 0 ) {
		return MXFT_UINT64;
	}
	if ( mx_strcasecmp( datatype_name, "record" ) == 0 ) {
		return MXFT_RECORD;
	}
	if ( mx_strcasecmp( datatype_name, "recordtype" ) == 0 ) {
		return MXFT_RECORDTYPE;
	}
	if ( mx_strcasecmp( datatype_name, "interface" ) == 0 ) {
		return MXFT_INTERFACE;
	}
	if ( mx_strcasecmp( datatype_name, "record_field" ) == 0 ) {
		return MXFT_RECORD_FIELD;
	}

	mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Datatype name '%s' does not correspond to a valid MX datatype.",
		datatype_name );

	return (-1L);
}

/*=====================================================================*/

MX_EXPORT const char *
mx_get_datatype_name_from_datatype( long datatype )
{
	static char datatype_name[80];

	switch( datatype ) {
	case 0:
		strlcpy( datatype_name, "none", sizeof(datatype_name) );
		break;
	case MXFT_STRING:
		strlcpy( datatype_name, "string", sizeof(datatype_name) );
		break;
	case MXFT_CHAR:
		strlcpy( datatype_name, "char", sizeof(datatype_name) );
		break;
	case MXFT_UCHAR:
		strlcpy( datatype_name, "uchar", sizeof(datatype_name) );
		break;
	case MXFT_SHORT:
		strlcpy( datatype_name, "short", sizeof(datatype_name) );
		break;
	case MXFT_USHORT:
		strlcpy( datatype_name, "ushort", sizeof(datatype_name) );
		break;
	case MXFT_BOOL:
		strlcpy( datatype_name, "bool", sizeof(datatype_name) );
		break;
	case MXFT_LONG:
		strlcpy( datatype_name, "long", sizeof(datatype_name) );
		break;
	case MXFT_ULONG:
		strlcpy( datatype_name, "ulong", sizeof(datatype_name) );
		break;
	case MXFT_FLOAT:
		strlcpy( datatype_name, "float", sizeof(datatype_name) );
		break;
	case MXFT_DOUBLE:
		strlcpy( datatype_name, "double", sizeof(datatype_name) );
		break;
	case MXFT_HEX:
		strlcpy( datatype_name, "hex", sizeof(datatype_name) );
		break;
	case MXFT_INT64:
		strlcpy( datatype_name, "int64", sizeof(datatype_name) );
		break;
	case MXFT_UINT64:
		strlcpy( datatype_name, "uint64", sizeof(datatype_name) );
		break;
	case MXFT_RECORD:
		strlcpy( datatype_name, "record", sizeof(datatype_name) );
		break;
	case MXFT_RECORDTYPE:
		strlcpy( datatype_name, "recordtype", sizeof(datatype_name) );
		break;
	case MXFT_INTERFACE:
		strlcpy( datatype_name, "interface", sizeof(datatype_name) );
		break;
	case MXFT_RECORD_FIELD:
		strlcpy( datatype_name, "record_field", sizeof(datatype_name) );
		break;
	default:
		snprintf( datatype_name, sizeof(datatype_name),
			"unknown (%ld)", datatype );
		break;
	}

	return (const char *) datatype_name;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_get_field_by_label_value( MX_RECORD *record,
				long label_value,
				MX_RECORD_FIELD **field )
{
	static const char fname[] = "mx_get_field_by_label_value()";

	MX_RECORD_FIELD *record_field_array, *record_field;
	long i;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}
	if ( field == (MX_RECORD_FIELD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD pointer passed was NULL." );
	}

	*field = NULL;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &( record_field_array[i] );

		if ( label_value == record_field->label_value ) {
			*field = record_field;
		}
	}

	if ( (*field) == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"An MX_RECORD_FIELD with a label value of %ld "
		"was not found in record '%s'.",
			label_value, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT const char *
mx_get_field_label_string( MX_RECORD *record, long label_value )
{
	static const char fname[] = "mx_get_field_label_string()";

	static const char unknown_label_value[] = "unknown";
	static const char null_record_value[] = "NULL";

	long i;

	MX_RECORD_FIELD *record_field_array, *record_field;

	if ( record == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );

		return null_record_value;
	}

	if ( label_value < 0 ) {
		return unknown_label_value;
	}

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &( record_field_array[i] );

		if ( label_value == record_field->label_value ) {
			return record_field->name;
		}
	}

	return unknown_label_value;
}

/*=====================================================================*/

MX_EXPORT const char *
mx_get_field_type_string( long field_type )
{
	static MX_FIELD_TYPE_NAME field_type_list[] = {
	{ MXFT_STRING,		"MXFT_STRING" },
	{ MXFT_CHAR,		"MXFT_CHAR" },
	{ MXFT_UCHAR,		"MXFT_UCHAR" },
	{ MXFT_SHORT,		"MXFT_SHORT" },
	{ MXFT_USHORT,		"MXFT_USHORT" },
	{ MXFT_BOOL,		"MXFT_BOOL" },
	{ MXFT_LONG,		"MXFT_LONG" },
	{ MXFT_ULONG,		"MXFT_ULONG" },
	{ MXFT_INT64,		"MXFT_INT64" },
	{ MXFT_UINT64,		"MXFT_UINT64" },
	{ MXFT_FLOAT,		"MXFT_FLOAT" },
	{ MXFT_DOUBLE,		"MXFT_DOUBLE" },

	{ MXFT_HEX,		"MXFT_HEX" },

	{ MXFT_RECORD,		"MXFT_RECORD" },
	{ MXFT_RECORDTYPE,	"MXFT_RECORDTYPE" },
	{ MXFT_INTERFACE,	"MXFT_INTERFACE" },
	{ MXFT_RECORD_FIELD,	"MXFT_RECORD_FIELD" },
	};

	static int num_field_types
		= sizeof(field_type_list) / sizeof(field_type_list[0]);

	const char unknown_field_type[] = "MXFT_UNKNOWN";

	int i;
	const char *ptr;

	for ( i = 0; i < num_field_types; i++ ) {
		if ( field_type_list[i].type_code == field_type )
			break;		/* exit the for() loop */
	}

	if ( i >= num_field_types ) {
		ptr = unknown_field_type;
	} else {
		ptr = field_type_list[i].type_text;
	}

	return ptr;
}

/*=====================================================================*/

MX_EXPORT MX_RECORD_FIELD *
mx_get_record_field( MX_RECORD *record, const char *field_name )
{
	static const char fname[] = "mx_get_record_field()";

	MX_RECORD_FIELD *field_array, *field;
	long i, num_record_fields;

	if ( record == NULL ) {
		return NULL;
	}

	if ( field_name == NULL ) {
		return NULL;
	}

	num_record_fields = record->num_record_fields;

	if ( num_record_fields <= 0 ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"record->num_record_fields for record '%s' "
		"has an illegal value of %ld.",
			record->name, num_record_fields );

		return NULL;
	}

	field_array = record->record_field_array;

	if ( field_array == NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"record->record_field_array for record '%s' is NULL.",
			record->name );

		return NULL;
	}

	for ( i = 0; i < num_record_fields; i++ ) {

		field = &field_array[i];

		if ( field == NULL ) {
			(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"&field_array[%ld] == NULL for record '%s'.",
			i, record->name );

			return NULL;
		}

		if ( field->name == NULL ) {
			(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"field->name for field %ld (%p) is NULL for "
				"record '%s'.  This probably means that you "
				"invoked this function from within the "
				"create_record_structures function of the "
				"driver.  This function cannot be invoked "
				"before the finish_record_initialization step "
				"of record setup.",
					i, field, record->name );

			return NULL;
		}

		if ( strcmp( field_name, field->name ) == 0 ) {

			return field;
		}
	}

	return NULL;
}

/*=====================================================================*/

MX_EXPORT MX_RECORD_FIELD *
mx_get_record_field_by_name( MX_RECORD *mx_database,
				const char *record_field_name )
{
	static const char fname[] = "mx_get_record_field_by_name()";

	MX_RECORD *mx_record = NULL;
	MX_RECORD_FIELD *mx_record_field = NULL;
	char *record_field_name_copy = NULL;
	int argc; char **argv;

	if ( mx_database == (MX_RECORD *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
		return NULL;
	}
	if ( record_field_name == (char *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
			"The record_field_name pointer passed was NULL." );
		return NULL;
	}

	/* Make sure that we are at the MX record list head. */

	mx_database = mx_database->list_head;

	mx_record_field = NULL;

	/* Split the record_field_name into the
	 * record name and the field name.
	 */

	record_field_name_copy = strdup( record_field_name );

	if ( record_field_name_copy == (char *) NULL ) {
		(void) mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to make a copy of "
			"record field name '%s'.", record_field_name );
		return NULL;
	}

	mx_string_split( record_field_name_copy, ".", &argc, &argv );

	if ( (argc == 0) || (argc >= 3) ) {
		(void) mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Not able to parse '%s' as a record field name.",
				record_field_name );
	} else {
		mx_record = mx_get_record( mx_database, argv[0] );

		if ( mx_record == (MX_RECORD *) NULL ) {
			(void) mx_error( MXE_NOT_FOUND, fname,
			"Record '%s' specified for record field name '%s' "
			"is not found in the MX database.",
				argv[0], record_field_name );
		} else {
			char mx_field_name[MXU_FIELD_NAME_LENGTH+1];

			if ( (argc == 1 ) || (strlen(argv[1]) == 0 ) ) {
				strlcpy( mx_field_name, "value",
						sizeof(mx_field_name) );
			} else {
				strlcpy( mx_field_name, argv[1],
						sizeof(mx_field_name) );

				mx_record_field = mx_get_record_field(
						mx_record, mx_field_name );
			}
		}
	}

	mx_free( record_field_name_copy );

	return mx_record_field;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_find_record_field( MX_RECORD *record, const char *name_of_field_to_find,
				MX_RECORD_FIELD **field_that_was_found )
{
	static const char fname[] = "mx_find_record_field()";

	MX_RECORD_FIELD *field;
	long i, num_record_fields;

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
					"NULL MX_RECORD pointer passed." );
	}

	if ( name_of_field_to_find == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
				"NULL record field name pointer passed." );
	}

	if ( field_that_was_found == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
"Address of location to place the address of the MX_RECORD_FIELD was NULL.");
	}

	num_record_fields = record->num_record_fields;

	if ( num_record_fields <= 0 ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"Number of record fields = %ld for record '%s' is less than or equal to zero.",
			num_record_fields, record->name );
	}

	field = record->record_field_array;

	if ( field == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"record_field_array for record '%s' was NULL.",
			record->name );
	}

	*field_that_was_found = NULL;

	for ( i = 0; i < num_record_fields; i++ ) {
		if ( strcmp( name_of_field_to_find, field[i].name ) == 0 ) {
			*field_that_was_found = &field[i];
			break;		/* Exit the for() loop. */
		}
	}

	if ( i >= num_record_fields ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Record field '%s' was not found in record '%s'",
			name_of_field_to_find, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_find_record_field_defaults( MX_DRIVER *driver,
		const char *name_of_field_to_find,
		MX_RECORD_FIELD_DEFAULTS **default_field_that_was_found )
{
	static const char fname[] = "mx_find_record_field_defaults()";

	MX_RECORD_FIELD_DEFAULTS *record_field_defaults_array;
	long index_of_field_that_was_found;
	mx_status_type mx_status;

	if ( default_field_that_was_found
			== (MX_RECORD_FIELD_DEFAULTS **) NULL )
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"Address of location to place the address of "
			"the MX_RECORD_FIELD_DEFAULTS was NULL." );
	}

	record_field_defaults_array = *(driver->record_field_defaults_ptr);

	mx_status = mx_find_record_field_defaults_index( driver,
					name_of_field_to_find,
					&index_of_field_that_was_found );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*default_field_that_was_found
	    = &record_field_defaults_array[ index_of_field_that_was_found ];

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_find_record_field_defaults_index( MX_DRIVER *driver,
				const char *name_of_field_to_find,
				long *index_of_field_that_was_found )
{
	static const char fname[] = "mx_find_record_field_defaults_index()";

	MX_RECORD_FIELD_DEFAULTS *record_field_defaults_array;
	long i, num_record_fields;

	if ( driver == (MX_DRIVER *) NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DRIVER pointer passed was NULL." );
	}

	record_field_defaults_array = *(driver->record_field_defaults_ptr);

	if ( record_field_defaults_array == (MX_RECORD_FIELD_DEFAULTS *) NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD_FIELD_DEFAULTS pointer for driver '%s' is NULL.",
			driver->name );
	}

	num_record_fields = *(driver->num_record_fields);

	if ( num_record_fields <= 0 ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The number of record fields = %ld for driver '%s' "
		"is less than or equal to zero.",
			num_record_fields, driver->name );
	}

	if ( name_of_field_to_find == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL record field name pointer passed for driver '%s'.",
			driver->name );
	}

	if ( index_of_field_that_was_found == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Address of location to place the index of "
		"the MX_RECORD_FIELD_DEFAULTS was NULL for driver '%s'.",
			driver->name );
	}

	*index_of_field_that_was_found = -1;

	for ( i = 0; i < num_record_fields; i++ ) {
		if ( strcmp( name_of_field_to_find,
				record_field_defaults_array[i].name ) == 0 ) {

			*index_of_field_that_was_found = i;

			break;		/* Exit the for() loop. */
		}
	}

	if ( i >= num_record_fields ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Record field '%s' was not found for driver '%s'.",
			name_of_field_to_find, driver->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT void *
mx_get_field_value_pointer( MX_RECORD_FIELD *field )
{
	void *value_pointer;

	if ( field == NULL ) {
		return NULL;
	}

	if ( field->data_pointer == NULL ) {
		return NULL;
	}

	if ( field->flags & MXFF_VARARGS ) {
		value_pointer = mx_read_void_pointer_from_memory_location(
						field->data_pointer );
	} else {
		value_pointer = field->data_pointer;
	}
	return value_pointer;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_construct_ptr_to_field_data(
		MX_RECORD *record,
		MX_RECORD_FIELD_DEFAULTS *record_field_defaults,
		void **field_data_ptr )
{
	static const char fname[] = "mx_construct_ptr_to_field_data()";

	char *structure_ptr;

	MX_DEBUG( 8,("%s: record = '%s', field defaults = '%s'",
		fname, record->name, record_field_defaults->name ));

	/* Construct a pointer to the field data. */

	switch( record_field_defaults->structure_id ) {
	case MXF_REC_RECORD_STRUCT:
		structure_ptr = (char *) record;
		break;
	case MXF_REC_SUPERCLASS_STRUCT:
		structure_ptr = (char *) record->record_superclass_struct;
		break;
	case MXF_REC_CLASS_STRUCT:
		structure_ptr = (char *) record->record_class_struct;
		break;
	case MXF_REC_TYPE_STRUCT:
		structure_ptr = (char *) record->record_type_struct;
		break;
	default:
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname, 
		"Record field '%s' has an unrecognized structure id type %d.",
			record_field_defaults->name,
			record_field_defaults->structure_id );
	}

	/* *field_data_ptr contains the address of the given
	 * structure element.
	 */

	*field_data_ptr
		= structure_ptr + record_field_defaults->structure_offset;

	MX_DEBUG( 8,
	("%s: structure_ptr = %p, field offset = 0x%lx, *field_data_ptr = %p",
		fname, structure_ptr,
		(long) record_field_defaults->structure_offset,
		*field_data_ptr ));

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_construct_ptr_to_field_value(
		MX_RECORD *record,
		MX_RECORD_FIELD_DEFAULTS *record_field_defaults,
		void **field_value_ptr )
{
	void *field_data_ptr;
	mx_status_type status;

	status = mx_construct_ptr_to_field_data( record,
				record_field_defaults, &field_data_ptr );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( record_field_defaults->flags & MXFF_VARARGS ) {
		*field_value_ptr =
		    mx_read_void_pointer_from_memory_location(field_data_ptr);

	} else {
		*field_value_ptr = field_data_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_set_1d_field_array_length( MX_RECORD_FIELD *field, unsigned long new_length )
{
	static const char fname[] = "mx_set_1d_field_array_length()";

	if ( field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD pointer passed was NULL." );
	}

	if ( field->num_dimensions != 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Record field '%s' is not a 1-dimensional record field.",
			field->name );
	}

	if ( ( field->flags & MXFF_VARARGS ) == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Record field '%s' is not a varying length field.  "
		"MXFF_VARARGS is not set.", field->name );
	}

	MX_DEBUG( 2,("%s: Field '%s', old length = %ld, new length = %ld",
		fname, field->name, field->dimension[0], new_length ));

	field->dimension[0] = (long) new_length;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_set_1d_field_array_length_by_name( MX_RECORD *record,
					char *field_name,
					unsigned long new_length )
{
	MX_RECORD_FIELD *field;
	mx_status_type mx_status;

	mx_status = mx_find_record_field( record, field_name, &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_set_1d_field_array_length( field, new_length );

	return mx_status;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_create_record_from_description(
		MX_RECORD *record_list,
		char *description,
		MX_RECORD **created_record,
		unsigned long flags )
{
	static const char fname[] = "mx_create_record_from_description()";

	MX_RECORD *current_record;
	MX_RECORD *record;
	MX_RECORD_FUNCTION_LIST *record_function_list;
	MX_DRIVER *superclass_driver;
	MX_DRIVER *class_driver, *type_driver;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults_array;
	MX_RECORD_FIELD_PARSE_STATUS parse_status;
	char token[4][MXU_BUFFER_LENGTH + 1];
	mx_status_type (*fptr)( MX_RECORD * );
	mx_status_type mx_status;
	int i, record_name_length;
	char separators[] = MX_RECORD_FIELD_SEPARATORS;

	MX_DEBUG( 8,("#####################################################"));
	MX_DEBUG( 8,("%s: description = '%s'", fname, description));

	/* Null this out in case we return with an error. */

	*created_record = NULL;

	/* Parse the first four fields of the record description
	 * to find out what kind of record description it is.
	 */

	mx_initialize_parse_status( &parse_status, description, separators );

	for ( i = 0; i < 4; i++ ) {
		mx_status = mx_get_next_record_token( &parse_status,
				token[i], sizeof( token[0] ) );

		if ( mx_status.code != MXE_SUCCESS ) {
		    if ( i == 0 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"No tokens were found on line '%s'.", description );
		    } else
		    if ( i == 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Only 1 token was found on line '%s'.", description );
		    } else {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Only %d tokens were found on line '%s'.", i, description );
		    }
		}
	}

#if 0
	for ( i = 0; i < 4; i++ ) {
		fprintf(stderr, "token[%d] = '%s' ", i, token[i]);
	}
	fprintf(stderr,"\n");
#endif

	record_name_length = (int) strlen( token[0] );

	if ( record_name_length >= MXU_RECORD_NAME_LENGTH ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The number of characters '%d' in record name '%s' "
		"is longer than the maximum allowed length of %d.",
			record_name_length, token[0],
			MXU_RECORD_NAME_LENGTH - 1 );
	}

	/* The first field is the name of this record.  It should be unique. */

	record = mx_get_record( record_list, token[0] );

	if ( record != (MX_RECORD *) NULL ) {

		if ( flags & MXFC_ALLOW_RECORD_REPLACEMENT ) {

			mx_status = mx_delete_record( record );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

		} else if ( flags & MXFC_ALLOW_SCAN_REPLACEMENT ) {

			if ( record->mx_superclass != MXR_SCAN ) {

				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Preexisting record '%s' is not a scan record and may not be deleted.",
					record->name );

			} else if ( strcmp( token[1], "scan" ) != 0 ) {

				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Scan record '%s' may not be replaced by non-scan description '%s'",
					record->name, description );

			} else {
				mx_status = mx_delete_record( record );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
		} else {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The record name '%s' is already in use in record list %p",
				token[0], record_list );
		}
	}

	/* Create a new record in the record list and copy the record
	 * name into it.  Since the record list is circular, inserting
	 * the record before the first record is equivalent to inserting
	 * it after the last record.
	 */

	current_record = mx_create_record();

	if ( current_record == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Out of memory allocating first data record.");
	}

	strlcpy( current_record->name, token[0], MXU_RECORD_NAME_LENGTH );

	mx_status = mx_insert_before_record( record_list, current_record );

	if ( mx_status.code != MXE_SUCCESS ) {
		free( current_record );

		return mx_status;
	}

	/* === Figure out what record SUPERCLASS this is. === */

	superclass_driver = mx_get_superclass_driver_by_name( token[1] );

	if ( superclass_driver == (MX_DRIVER *) NULL ) {
		mx_delete_record( current_record );

		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Record superclass '%s' is unrecognizable on line '%s'.",
			token[1], description );
	}

	current_record->mx_superclass = superclass_driver->mx_superclass;

	/* === Figure out what record CLASS this is. === */

	class_driver = mx_get_class_driver_by_name( token[2] );

	if ( class_driver == (MX_DRIVER *) NULL ) {
		mx_delete_record( current_record );

		return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Record class '%s' is unrecognizable on line '%s'",
					token[2], description );
	}

	current_record->mx_class = class_driver->mx_class;

	/* === Figure out what record TYPE this is. === */

	type_driver = mx_get_driver_by_name( token[3] );

	if ( type_driver == (MX_DRIVER *) NULL ) {
		mx_delete_record( current_record );

		return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Record type '%s' is unrecognizable on line '%s'",
					token[3], description );
	}

	current_record->mx_type = type_driver->mx_type;

	/* Does this record type have new style record field support
	 * that we can use to initialize the record?
	 */

	if ( ( type_driver->num_record_fields == NULL )
	  || ( *(type_driver->num_record_fields) <= 0 )
	  || ( type_driver->record_field_defaults_ptr == NULL ) )
	{
		/* If no record fields are defined, invoke the old
		 * interface initialization function.
		 */

		mx_status = mx_error( MXE_UNSUPPORTED, fname,
			"The driver for record '%s' does not define "
			"any record fields!", current_record->name );

		mx_delete_record( current_record );

		return mx_status;

	} else {
		/* Record field support _is_ available, so use it to
		 * initialize the record.
		 */

		MX_DEBUG( 8,("%s: record type '%s' uses record fields.",
			fname, token[3]));

		current_record->num_record_fields
			= *(type_driver->num_record_fields);

		record_field_defaults_array
			= *(type_driver->record_field_defaults_ptr);

		/* Allocate an array for the record fields of the
		 * current record.
		 */

		current_record->record_field_array
		    = (MX_RECORD_FIELD *) malloc(
				current_record->num_record_fields
				* sizeof(MX_RECORD_FIELD) );

		if ( current_record->record_field_array
					== (MX_RECORD_FIELD *) NULL ) {

			mx_status = mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory allocating record field array for record '%s'",
				current_record->name );

			mx_delete_record( current_record );

			return mx_status;
		}

		/* The create_record_structures function from the
		 * MX_RECORD_FUNCTION_LIST is invoked before we do
		 * anything else, so that any data structures that
		 * have to be set up before we process the record 
		 * field list can be done.  For the example of an
		 * "e500" record, this means malloc-ing an MX_MOTOR
		 * structure and an MX_E500 structure and setting up
		 * the pointers to them in the MX_RECORD.
		 */

		record_function_list
			= type_driver->record_function_list;

		if ( record_function_list
				== (MX_RECORD_FUNCTION_LIST *) NULL ) {

			mx_status = mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"No record function list defined for record type %ld.",
					current_record->mx_type);

			mx_delete_record( current_record );

			return mx_status;
		}

		current_record->record_function_list	/* save the pointer */
			= record_function_list;

		fptr = record_function_list->create_record_structures;

		if ( fptr == NULL ) {
			mx_status = mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"No create_record_structures function defined for record type %ld.",
					current_record->mx_type);

			mx_delete_record( current_record );

			return mx_status;
		}

		mx_status = (*fptr)( current_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_delete_record( current_record );

			return mx_status;
		}

		/* Now parse the record field array. */

		mx_status = mx_parse_record_fields( current_record,
				record_field_defaults_array, &parse_status );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_status = mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Cannot parse record description '%s' for record '%s'",
				description, current_record->name );

			mx_delete_record( current_record );

			return mx_status;
		}
	}

	*created_record = current_record;

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

static mx_status_type
mx_parse_string_field( void *dataptr, char *token,
			MX_RECORD *record, MX_RECORD_FIELD *field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status)
{
	static const char fname[] = "mx_parse_string_field()";

	MX_DEBUG( 8,("%s: dataptr = %p, token = '%s'",
			fname, dataptr, token));

	strlcpy( (char *) dataptr, token,
			parse_status->max_string_token_length );

	if ( strlen(token) >= parse_status->max_string_token_length ) {

		if ( record == (MX_RECORD *) NULL ) {
			mx_warning(
			"String token '%s' was longer than "
			"the maximum of %ld bytes.  "
			"The string was truncated to '%s'.",
				token,
				parse_status->max_string_token_length,
				(char *) dataptr );
		} else {
			mx_warning(
			"String token '%s' for record '%s' was longer than "
			"the maximum of %ld bytes.  "
			"The string was truncated to '%s'.",
				token, record->name,
				parse_status->max_string_token_length,
				(char *) dataptr );
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_construct_string_field( void *dataptr,
			char *token_buffer, size_t token_buffer_length,
			MX_RECORD *record, MX_RECORD_FIELD *record_field )
{
	char *ptr;
	int whitespace, string_length;

	MX_DEBUG( 8,
		("mx_construct_string_field: dataptr = %p, *dataptr = '%s'",
		dataptr, (char *) dataptr));

	ptr = (char *) dataptr;

	string_length = (int) strlen(ptr);

	/* Are there any whitespace characters in this string? */

	if ( string_length == 0 ) {
			snprintf( token_buffer, token_buffer_length, "\"\"" );
	} else {
		if (strcspn(ptr, MX_RECORD_FIELD_SEPARATORS) < string_length) {
			whitespace = TRUE;
		} else {
			whitespace = FALSE;
		}
		if ( whitespace) {
			snprintf( token_buffer, token_buffer_length,
					"\"%s\"", (char *) dataptr);
		} else {
			snprintf( token_buffer, token_buffer_length,
					"%s", (char *) dataptr);
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_parse_char_field( void *dataptr, char *token,
			MX_RECORD *record, MX_RECORD_FIELD *field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status)
{
	static const char fname[] = "mx_parse_char_field()";

	int num_items;

	num_items = sscanf( token, "%c", (char *) dataptr );

	if ( num_items != 1 )
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Char not found in token '%s'", token );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_construct_char_field( void *dataptr,
			char *token_buffer, size_t token_buffer_length,
			MX_RECORD *record, MX_RECORD_FIELD *record_field )
{
	snprintf( token_buffer, token_buffer_length,
			"%c", *((char *) dataptr) );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_parse_uchar_field( void *dataptr, char *token,
			MX_RECORD *record, MX_RECORD_FIELD *field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status)
{
	static const char fname[] = "mx_parse_uchar_field()";

	int num_items;

#if defined(__INTEL_COMPILER)
	/* The Intel C compiler thinks that %c used with (unsigned char *)
	 * is a type mismatch.  It is the only compiler I know of that
	 * thinks this.
	 */

	num_items = sscanf( token, "%c", (char *) dataptr );
#else
	num_items = sscanf( token, "%c", (unsigned char *) dataptr );
#endif

	if ( num_items != 1 )
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Unsigned char not found in token '%s'", token );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_construct_uchar_field( void *dataptr,
			char *token_buffer, size_t token_buffer_length,
			MX_RECORD *record, MX_RECORD_FIELD *record_field )
{
	snprintf( token_buffer, token_buffer_length,
			"%c", *((unsigned char *) dataptr) );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_parse_short_field( void *dataptr, char *token,
			MX_RECORD *record, MX_RECORD_FIELD *field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status)
{
	static const char fname[] = "mx_parse_short_field()";

	int num_items;

	num_items = sscanf( token, "%hd", (short *) dataptr );

	if ( num_items != 1 )
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Short not found in token '%s'", token );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_construct_short_field( void *dataptr,
			char *token_buffer, size_t token_buffer_length,
			MX_RECORD *record, MX_RECORD_FIELD *record_field )
{
	snprintf( token_buffer, token_buffer_length,
			"%hd", *((short *) dataptr) );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_parse_ushort_field( void *dataptr, char *token,
			MX_RECORD *record, MX_RECORD_FIELD *field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status)
{
	static const char fname[] = "mx_parse_ushort_field()";

	int num_items;

	num_items = sscanf( token, "%hu", (unsigned short *) dataptr );

	if ( num_items != 1 )
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Unsigned short not found in token '%s'", token );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_construct_ushort_field( void *dataptr,
			char *token_buffer, size_t token_buffer_length,
			MX_RECORD *record, MX_RECORD_FIELD *record_field )
{
	snprintf( token_buffer, token_buffer_length,
			"%hu", *((unsigned short *) dataptr) );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_parse_bool_field( void *dataptr, char *token,
			MX_RECORD *record, MX_RECORD_FIELD *field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status)
{
	static const char fname[] = "mx_parse_bool_field()";

	int int_value;
	mx_bool_type *bool_ptr;
	int num_items;

	bool_ptr = dataptr;

	num_items = sscanf( token, "%d", &int_value );

	if ( num_items != 1 )
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Boolean value not found in token '%s'", token );

	switch( int_value ) {
	case 0:
	case 1:
		*bool_ptr = (mx_bool_type) int_value;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		  "Value %d found for boolean field is not a legal boolean "
		  "value.  The allowed values are 0 and 1.", int_value );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_construct_bool_field( void *dataptr,
			char *token_buffer, size_t token_buffer_length,
			MX_RECORD *record, MX_RECORD_FIELD *record_field )
{
	mx_bool_type bool_value;

	bool_value = *((mx_bool_type *) dataptr);

	if ( bool_value ) {
		snprintf( token_buffer, token_buffer_length, "1" );
	} else {
		snprintf( token_buffer, token_buffer_length, "0" );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_parse_long_field( void *dataptr, char *token,
			MX_RECORD *record, MX_RECORD_FIELD *field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status)
{
	static const char fname[] = "mx_parse_long_field()";

	int num_items;

	num_items = sscanf( token, "%ld", (long *) dataptr );

	if ( num_items != 1 )
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Long not found in token '%s'", token );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_construct_long_field( void *dataptr,
			char *token_buffer, size_t token_buffer_length,
			MX_RECORD *record, MX_RECORD_FIELD *record_field )
{
	snprintf( token_buffer, token_buffer_length,
			"%ld", *((long *) dataptr) );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_parse_ulong_field( void *dataptr, char *token,
			MX_RECORD *record, MX_RECORD_FIELD *field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status)
{
	static const char fname[] = "mx_parse_ulong_field()";

	int num_items;

	num_items = sscanf( token, "%lu", (unsigned long *) dataptr );

	if ( num_items != 1 )
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Unsigned long not found in token '%s'", token );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_construct_ulong_field( void *dataptr,
			char *token_buffer, size_t token_buffer_length,
			MX_RECORD *record, MX_RECORD_FIELD *record_field )
{
	snprintf( token_buffer, token_buffer_length,
			"%lu", *((unsigned long *) dataptr) );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_parse_int64_field( void *dataptr, char *token,
			MX_RECORD *record, MX_RECORD_FIELD *field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status)
{
	static const char fname[] = "mx_parse_int64_field()";

	int64_t *int64_ptr;
	int num_items;

	int64_ptr = (int64_t *) dataptr;

	num_items = sscanf( token, "%" SCNd64, int64_ptr );

	if ( num_items != 1 )
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Integer not found in token '%s'", token );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_construct_int64_field( void *dataptr,
			char *token_buffer, size_t token_buffer_length,
			MX_RECORD *record, MX_RECORD_FIELD *record_field )
{
	int64_t int64_value;

	int64_value = *((int64_t *) dataptr);

	snprintf( token_buffer, token_buffer_length,
			"%" PRId64, int64_value );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_parse_uint64_field( void *dataptr, char *token,
			MX_RECORD *record, MX_RECORD_FIELD *field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status)
{
	static const char fname[] = "mx_parse_uint64_field()";

	uint64_t *uint64_ptr;
	int num_items;

	uint64_ptr = (uint64_t *) dataptr;

	num_items = sscanf( token, "%" SCNu64, uint64_ptr );

	if ( num_items != 1 )
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Unsigned integer not found in token '%s'", token );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_construct_uint64_field( void *dataptr,
			char *token_buffer, size_t token_buffer_length,
			MX_RECORD *record, MX_RECORD_FIELD *record_field )
{
	uint64_t uint64_value;

	uint64_value = *((uint64_t *) dataptr);

	snprintf( token_buffer, token_buffer_length,
			"%" PRIu64, uint64_value );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_parse_float_field( void *dataptr, char *token,
			MX_RECORD *record, MX_RECORD_FIELD *field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status)
{
	static const char fname[] = "mx_parse_float_field()";

	int num_items;

	num_items = sscanf( token, "%g", (float *) dataptr );

	if ( num_items != 1 )
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Float not found in token '%s'", token );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_construct_float_field( void *dataptr,
			char *token_buffer, size_t token_buffer_length,
			MX_RECORD *record, MX_RECORD_FIELD *record_field )
{
	int precision;

	if ( record != NULL ) {
		precision = record->precision;
	} else {
		precision = 15;
	}

	snprintf( token_buffer, token_buffer_length,
			"%.*g", precision, *((float *) dataptr) );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_parse_double_field( void *dataptr, char *token,
			MX_RECORD *record, MX_RECORD_FIELD *field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status)
{
	static const char fname[] = "mx_parse_double_field()";

	int num_items;

	num_items = sscanf( token, "%lg", (double *) dataptr );

	if ( num_items != 1 )
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Double not found in token '%s'", token );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_construct_double_field( void *dataptr,
			char *token_buffer, size_t token_buffer_length,
			MX_RECORD *record, MX_RECORD_FIELD *record_field )
{
	int precision;

	if ( record != NULL ) {
		precision = record->precision;
	} else {
		precision = 15;
	}

	snprintf( token_buffer, token_buffer_length,
			"%.*g", precision, *((double *) dataptr) );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_parse_hex_field( void *dataptr, char *token,
			MX_RECORD *record, MX_RECORD_FIELD *field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status)
{
	static const char fname[] = "mx_parse_hex_field()";

	char *endptr;
	unsigned long *value_ptr;

	value_ptr = (unsigned long *) dataptr;

	*value_ptr = strtoul( token, &endptr, 0 );

	if ( *endptr != '\0' ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Unsigned long or hex value not found in token '%s'", token );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_construct_hex_field( void *dataptr,
			char *token_buffer, size_t token_buffer_length,
			MX_RECORD *record, MX_RECORD_FIELD *record_field )
{
	unsigned long data_value;
	int width;

	data_value = *((unsigned long *) dataptr);

	if ( data_value < 256L ) {
		width = 2;
	} else if ( data_value < 65536L ) {
		width = 4;
	} else if ( data_value < 16777216L ) {
		width = 6;
	} else {
		width = 8;
	}

	snprintf( token_buffer, token_buffer_length,
			"%#*lx", width, data_value );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_parse_mx_record_field( void *memory_location, char *token,
			MX_RECORD *record, MX_RECORD_FIELD *field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status)
{
	static const char fname[] = "mx_parse_mx_record_field()";

	MX_RECORD *list_head_record;
	MX_LIST_HEAD *list_head_struct;
	MX_RECORD *referenced_record;
	MX_RECORD *placeholder_record;
	void **fixup_record_array;
	long num_fixup_records, num_fixup_record_blocks;
	size_t new_array_size;
	mx_status_type status;

	MX_DEBUG( 8,(
	"%s: Token = '%s', memory_location = %p, referencing record = '%s'",
			fname, token, memory_location, record->name));

	/* If placeholder records are not in use, just look up the
	 * record entry in the list.  Otherwise, construct a placeholder
	 * record.
	 */

	list_head_record = (MX_RECORD *) (record->list_head);

	list_head_struct
	    = (MX_LIST_HEAD *) (list_head_record->record_superclass_struct);

#if 0
	if ( strlen(token) == 0 ) {
		referenced_record = NULL;

		mx_write_void_pointer_to_memory_location( memory_location,
						referenced_record );
	} else
#endif
	if ( list_head_struct->fixup_records_in_use == FALSE ) {
		referenced_record = mx_get_record( list_head_record, token );

		if ( referenced_record == NULL ) {
			return mx_error( MXE_NOT_FOUND, fname,
			"The record '%s' mentioned by '%s' does not exist.",
				token, record->name );
		}

		mx_write_void_pointer_to_memory_location( memory_location,
						referenced_record );

	} else {

		/* Construct a placeholder for this record name that is to be
		 * replaced later by a pointer to the real record.  This is 
		 * in order to allow for forward references to record names.
		 */

		status = mx_construct_placeholder_record(
				record, token, &placeholder_record );

		if ( status.code != MXE_SUCCESS )
			return status;

		mx_write_void_pointer_to_memory_location( memory_location,
							placeholder_record );

		MX_DEBUG( 8,
		("%s: placeholder name = '%s', memory_location = %p",
			fname, placeholder_record->name, memory_location));

		/* Add 'memory_location' to the list of MX_RECORD pointers that
		 * need to be fixed up later by mx_fixup_placeholder_records().
		 */

		num_fixup_records = list_head_struct->num_fixup_records;

		fixup_record_array = list_head_struct->fixup_record_array;

		if (num_fixup_records % MX_FIXUP_RECORD_ARRAY_BLOCK_SIZE == 0){
			num_fixup_record_blocks
			= num_fixup_records / MX_FIXUP_RECORD_ARRAY_BLOCK_SIZE;

			num_fixup_record_blocks++;

			new_array_size = num_fixup_record_blocks
				* MX_FIXUP_RECORD_ARRAY_BLOCK_SIZE
				* sizeof(void *);

			/* The ANSI C standard says that if the first
			 * argument of realloc() is NULL, then it should
			 * act just like malloc().  Unfortunately,
			 * not all "ANSI C" implementations seem to
			 * know this.  acc under SunOS 4.1.4 is an
			 * example of this.
			 */

			if ( fixup_record_array == NULL ) {
				fixup_record_array = (void **) malloc(
					new_array_size );
			} else {
				fixup_record_array = (void **) realloc(
					fixup_record_array,  new_array_size );
			}

			if ( fixup_record_array == NULL ) {
				return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory trying to increase size of fixup record array.  "
"Requested array element blocks = %ld, requested array size = %ld bytes.",
				num_fixup_record_blocks, (long) new_array_size);
			}

			list_head_struct->fixup_record_array
				= fixup_record_array;
		}

		/* Save in the placeholder record the array index value
		 * of where the placeholder is located in the fixup record
		 * array.
		 */

		placeholder_record->record_flags = num_fixup_records;

		/* Store a pointer to the placeholder record in the fixup
		 * record array.
		 */

		fixup_record_array[ num_fixup_records ] = memory_location;

		list_head_struct->num_fixup_records = num_fixup_records + 1;
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_construct_mx_record_field( void *memory_location,
			char *token_buffer, size_t token_buffer_length,
			MX_RECORD *record, MX_RECORD_FIELD *record_field )
{
	MX_RECORD *referenced_record;

	referenced_record = (MX_RECORD *)
		mx_read_void_pointer_from_memory_location( memory_location );

	if ( referenced_record == NULL ) {
		snprintf( token_buffer, token_buffer_length, "NULL_record" );
	} else {
		snprintf( token_buffer, token_buffer_length,
				"%s", referenced_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_parse_recordtype_field( void *dataptr, char *token,
			MX_RECORD *record, MX_RECORD_FIELD *field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status)
{
	static const char fname[] = "mx_parse_recordtype_field()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname, "Not done yet." );
}

static mx_status_type
mx_construct_recordtype_field( void *dataptr,
			char *token_buffer, size_t token_buffer_length,
			MX_RECORD *record, MX_RECORD_FIELD *record_field )
{
	static const char fname[] = "mx_construct_recordtype_field()";

	MX_DRIVER *driver;

	driver = (MX_DRIVER *)(record_field->typeinfo);

	if ( driver == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Field '%s' in record '%s' has a NULL typeinfo entry.",
			record_field->name, record->name );
	}

	snprintf( token_buffer, token_buffer_length,
			"%s", driver->name );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_parse_interface_field( void *dataptr, char *token,
			MX_RECORD *record, MX_RECORD_FIELD *field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status)
{
	static const char fname[] = "mx_parse_interface_field()";

	MX_INTERFACE *interface;
	char *ptr;
	mx_status_type status;

	interface = (MX_INTERFACE *) dataptr;

	if ( interface == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"Memory has not been allocated for the MX_INTERFACE field for record '%s'",
			record->name );
	}

	/* Look for the colon ':' that separates the record name from
	 * the address field.
	 */

	ptr = strchr( token, ':' );

	if ( ptr != NULL ) {
		*ptr = '\0';

		ptr++;
	}

	/* Parse the record name. */

	status = mx_parse_mx_record_field( &(interface->record), token,
				record, field, parse_status );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Parse the address field. */

	interface->address_name[0] = '\0';

	if ( ptr == NULL ) {
		interface->address = 0;
	} else {
		strlcat( interface->address_name, ptr, 
				MXU_INTERFACE_ADDRESS_NAME_LENGTH );

		interface->address = atol( ptr );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_construct_interface_field( void *dataptr,
			char *token_buffer, size_t token_buffer_length,
			MX_RECORD *record, MX_RECORD_FIELD *record_field )
{
	MX_INTERFACE *interface;

	interface = (MX_INTERFACE *) dataptr;

	if ( strlen(interface->address_name) > 0 ) {
		snprintf( token_buffer, token_buffer_length,
			"%s:%s",
			interface->record->name,
			interface->address_name );
	} else {
		strlcpy( token_buffer,
			interface->record->name,
			token_buffer_length );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* NOTE: ..._field_field below is not a mistake, since it is dealing with
 * a referenced MX_RECORD_FIELD rather than a referenced MX_RECORD.
 */

static mx_status_type
mx_parse_mx_record_field_field( void *memory_location, char *token,
			MX_RECORD *record, MX_RECORD_FIELD *local_field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status )
{
	static const char fname[] = "mx_parse_mx_record_field_field()";

	char *ptr, *dup_token;
	char *record_name, *field_name;
	MX_RECORD *referenced_record;
	MX_RECORD_FIELD *referenced_field;
	mx_status_type mx_status;

	/* Duplicate the token, since we will be writing a null byte
	 * to the middle of it.
	 */

	dup_token = strdup( token );

	if ( dup_token == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a duplicate of the "
		"token passed to us." );
	}

	/* A valid token for an MX_RECORD_FIELD will look like the
	 * string 'record_name.field_name'.  Thus, the token must
	 * have a '.' period character in it.  If the token has
	 * more than one period in it, we look for the last one.
	 */

	ptr = strrchr( dup_token, '.' );

	if ( ptr == NULL ) {
		mx_free( dup_token );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"No period character '.' was found in the record field name "
		"'%s' that was passed to us.", token );
	}

	/* Overwrite the period to split the string into two strings. */

	*ptr = '\0';

	record_name = dup_token;
	field_name  = ptr + 1;

	referenced_record = mx_get_record( record, record_name );

	if ( referenced_record == (MX_RECORD *) NULL ) {

		/* FIXME: We should allow for placeholder records here, just
		 * like for MXFT_RECORD fields.
		 */

		mx_status = mx_error( MXE_NOT_FOUND, fname,
		"No record named '%s' as found in record field name '%s' "
		"has been found in the MX database before this record.  "
		"You will also see this error message if the referenced "
		"record appears after this line in the MX database.",
			record_name, token );

		mx_free( dup_token );

		return mx_status;
	}

	/* Look for the requested field name in the record we just found. */

	mx_status = mx_find_record_field( referenced_record, field_name,
						&referenced_field );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_status = mx_error( MXE_NOT_FOUND, fname,
		"Field '%s' was not found in record '%s' referenced by "
		"database token '%s'.",
			field_name, referenced_record->name, token );

		mx_free( dup_token );

		return mx_status;
	}

	mx_free( dup_token );

	mx_write_void_pointer_to_memory_location( memory_location,
						referenced_field );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_construct_mx_record_field_field( void *memory_location,
			char *token_buffer, size_t token_buffer_length,
			MX_RECORD *record, MX_RECORD_FIELD *local_record_field )
{
	MX_RECORD_FIELD *referenced_record_field;

	referenced_record_field = (MX_RECORD_FIELD *)
		mx_read_void_pointer_from_memory_location( memory_location );

	if ( referenced_record_field == NULL ) {
		strlcpy( token_buffer,
			"NULL_record_field", token_buffer_length );
	} else {
		snprintf( token_buffer, token_buffer_length,
			"%s", referenced_record_field->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

static mx_status_type
mx_compute_static_subarray_size( void *array_ptr,
		long dimension_level,
		MX_RECORD_FIELD *field,
		size_t *subarray_size )
{
	static const char fname[] = "mx_compute_static_subarray_size()";

	long i, multiplier, num_dimensions;

	num_dimensions = field->num_dimensions;

#if 1
	MX_DEBUG( 8,("%s: array_ptr = %p, dimension_level = %ld",
		fname, array_ptr, dimension_level));
	MX_DEBUG( 8,("%s: field = '%s'", fname, field->name));

	for ( i = 0; i < num_dimensions; i++ ) {
	    MX_DEBUG( 8,("%s: dimension[%ld] = %ld, element_size[%ld] = %ld",
	      fname, i, field->dimension[i],
	      i, (long) field->data_element_size[i]));
	}
#endif

	*subarray_size = field->data_element_size[0];

	for ( i = num_dimensions - 1;
			i >= (num_dimensions - dimension_level); i-- ) {

		multiplier = field->dimension[i];

		MX_DEBUG( 8,
		("%s: i = %ld, *subarray_size was %ld; multiplying by %ld.",
			fname, i, (long)*subarray_size, multiplier));

		*subarray_size *= multiplier;
	}

	MX_DEBUG( 8,("%s: *subarray_size = %ld", fname, (long)*subarray_size));

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_parse_array_description( void *array_ptr,
		long dimension_level,
		MX_RECORD *record,
		MX_RECORD_FIELD *field,
		MX_RECORD_FIELD_PARSE_STATUS *parse_status,
		mx_status_type (*token_parser) ( void *, char *,
					MX_RECORD *, MX_RECORD_FIELD *,
					MX_RECORD_FIELD_PARSE_STATUS * ) )
{
	static const char fname[] = "mx_parse_array_description()";

	const char short_label[] = "parsing array";

	long i, n, num_dimension_elements, new_dimension_level;
	long row_ptr_step_size;
	size_t dimension_element_size, subarray_size;
	char *row_ptr, *element_ptr;
	void *new_array_ptr;
	char token_buffer[500];
	mx_status_type status;

	dimension_element_size = field->data_element_size[ dimension_level ];

	n = field->num_dimensions - dimension_level - 1;

	num_dimension_elements = field->dimension[n];

	new_dimension_level = dimension_level - 1;

	MX_DEBUG( 8,
	("%s: dimension_level = %ld, n = %ld, num_dimension_elements = %ld",
		 short_label, dimension_level, n, num_dimension_elements));
	MX_DEBUG( 8,("%s:    array_ptr = %p", short_label, array_ptr));

	if ( dimension_level < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Negative dimension_level %ld passed for record field '%s.%s'",
			dimension_level, record->name, field->name );
	}
	if ( ((long)dimension_element_size) <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,

	"Illegal value %ld for data_element_size for record field '%s.%s' at "
	"dimension_level %ld.  This is almost certainly an error in the "
	"MX_RECORD_FIELD_DEFAULTS definition.",

			(long) dimension_element_size,
			record->name, field->name,
			dimension_level );
	}
	if ( num_dimension_elements < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Negative num_dimension_elements %ld passed for record field '%s.%s'.  "
	"This is probably due to failing to initialize an MXFF_VARARGS field.",
			dimension_level, record->name, field->name );
	}

	/*-----*/

	if (dimension_level == 0) {
	    if (field->datatype == MXFT_STRING) {

		/**** Strings must be handled specially. ****/

		/* Get the token. */

		status = mx_get_next_record_token( parse_status,
			token_buffer, sizeof(token_buffer) );

		if ( status.code != MXE_SUCCESS ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Next string token not found." );
		}

		MX_DEBUG( 8, ("%s: *****> String token  = '%s'",
				short_label, token_buffer));

		/* Parse the string. */

		status = (*token_parser)( (void *) array_ptr,
				token_buffer, record, field, parse_status );

		if ( status.code != MXE_SUCCESS )
			return status;

		MX_DEBUG( 8,("%s: *****> String token successfully parsed.",
			short_label));
	    } else {

		/** At bottom level of the array, so start parsing tokens. **/

		MX_DEBUG( 8,
			("%s: +++> Token parser to be invoked for %ld tokens",
			short_label, num_dimension_elements));
		MX_DEBUG( 8,
			("%s: +++> dimension_element_size = %lu",
			short_label, (unsigned long) dimension_element_size));

		for ( i = 0, element_ptr = (char *)array_ptr;
		      i < num_dimension_elements;
		      i++, element_ptr += dimension_element_size ) {

			/* Get the token. */

			status = mx_get_next_record_token( parse_status,
				token_buffer, sizeof(token_buffer) );

			if ( status.code != MXE_SUCCESS ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
					"Array was too short." );
			}

			MX_DEBUG( 8,
		("%s: *****> Array token [%ld] = '%s', element_ptr = %p",
				short_label, i, token_buffer, element_ptr ));

			/* Parse the token. */

			status = (*token_parser)( (void *) element_ptr,
				token_buffer, record, field, parse_status );

			if ( status.code != MXE_SUCCESS )
				return status;
		}
	    }
	} else {
		/** More levels in the array to descend through. **/

		MX_DEBUG( 8, ("%s: -> Go to next dimension level = %ld",
			short_label, new_dimension_level));

		row_ptr = (char *)array_ptr;

		if ( field->flags & MXFF_VARARGS ) {
			row_ptr_step_size = (long) dimension_element_size;

		} else {
			status = mx_compute_static_subarray_size( array_ptr,
				dimension_level, field, &subarray_size );

			if ( status.code != MXE_SUCCESS )
				return status;

			row_ptr_step_size = (long) subarray_size;
		}

		MX_DEBUG( 8,("%s: row_ptr = %p, row_ptr_step_size = %ld",
			fname, row_ptr, row_ptr_step_size));

		for ( i = 0; i < num_dimension_elements; i++ ) {

			if ( field->flags & MXFF_VARARGS ) {
				new_array_ptr =
				    mx_read_void_pointer_from_memory_location(
					row_ptr );
			} else {
				new_array_ptr = row_ptr;
			}

			status = mx_parse_array_description( new_array_ptr,
				new_dimension_level, record, field,
				parse_status, token_parser );

			if ( status.code != MXE_SUCCESS )
				return status;

		      	row_ptr += row_ptr_step_size;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT void
mx_initialize_parse_status( MX_RECORD_FIELD_PARSE_STATUS *parse_status,
				char *description, char *separators )
{
	if ( parse_status == NULL )
		return;

	parse_status->description = description;
	parse_status->separators = separators;

	if ( separators == NULL ) {
		parse_status->num_separators = 0;
	} else {
		parse_status->num_separators = (int) strlen( separators );
	}

	parse_status->start_of_trailing_whitespace = NULL;
	parse_status->max_string_token_length = 0;

	return;
}

/*=====================================================================*/

MX_EXPORT long
mx_get_max_string_token_length( MX_RECORD_FIELD *field )
{
	long last_dimension, max_length;

	if ( field == (MX_RECORD_FIELD *) NULL ) {
		return 0L;
	}
	if ( field->datatype != MXFT_STRING ) {
		return 0L;
	}
	if ( field->num_dimensions < 1 ) {
		return 0L;
	}

	last_dimension = field->num_dimensions - 1;

	max_length = field->dimension[ last_dimension ];

	return max_length;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_copy_defaults_to_record_field( MX_RECORD_FIELD *field,
		MX_RECORD_FIELD_DEFAULTS *field_defaults )
{
	static const char fname[] = "mx_copy_defaults_to_record_field()";

	if ( field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD_FIELD pointer passed was NULL." );
	}
	if ( field_defaults == (MX_RECORD_FIELD_DEFAULTS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD_FIELD_DEFAULTS pointer passed was NULL." );
	}

	field->label_value          = field_defaults->label_value;
	field->field_number         = field_defaults->field_number;
	field->name                 = field_defaults->name;
	field->datatype             = field_defaults->datatype;
	field->typeinfo             = field_defaults->typeinfo;
	field->num_dimensions       = field_defaults->num_dimensions;
	field->dimension            = field_defaults->dimension;
	field->data_pointer         = NULL;		/* Filled in later. */
	field->data_element_size    = field_defaults->data_element_size;
	field->process_function     = field_defaults->process_function;
	field->flags                = field_defaults->flags;
	field->timer_interval       = field_defaults->timer_interval;
	field->value_change_threshold
				= field_defaults->value_change_threshold;
	field->last_value           = 0.0;
	field->value_has_changed_manual_override = FALSE;
	field->value_changed_test_function
				= field_defaults->value_changed_test_function;
	field->callback_list        = NULL;
	field->application_ptr      = NULL;
	field->record               = NULL;
	field->active               = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_parse_record_fields( MX_RECORD *record,
			MX_RECORD_FIELD_DEFAULTS *record_field_defaults_array,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status )
{
	static const char fname[] = "mx_parse_record_fields()";

	MX_RECORD_FIELD *record_field, *record_field_array;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	mx_status_type (*fptr)(void *, char *, MX_RECORD *, MX_RECORD_FIELD *,
					MX_RECORD_FIELD_PARSE_STATUS *);

	/* field_data_ptr is the value of record_field->data_pointer,
	 * while field_value_ptr is the location that the actual data
	 * may be found at.
	 * 
	 * For a non-varargs field, field_value_ptr = field_data_ptr.
	 *
	 * For a varargs field, field_value_ptr =
	 *       mx_read_void_pointer_from_memory_location( field_data_ptr ).
	 *
	 * There _must_ be a more intuitive pair of names for these two
	 * variables, but I haven't been able to come up with them yet.
	 */

	void *field_data_ptr;
	void *field_value_ptr;

	void *array_ptr;
	long i, j, token_number;
	long num_record_fields, field_type;
	long dimension_size;
	int allocate_the_array;
	char token_buffer[500];
	mx_status_type status;

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL.");
	}

	/* There must be at least MX_NUM_RECORD_ID_FIELDS records
	 * in a valid MX_RECORD_FIELD array.  This corresponds to the 
	 * part of a record description like "testmotor  dev motor e500".
	 */

	num_record_fields = record->num_record_fields;

	MX_DEBUG( 8,
		("********* Record '%s', num_record_fields = %ld *********",
		record->name, num_record_fields));

	if (num_record_fields < MX_NUM_RECORD_ID_FIELDS) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Too few record fields for the record '%s'.  "
			"Num defined = %ld, min allowed = %d",
			record->name, num_record_fields,
			MX_NUM_RECORD_ID_FIELDS );
	}

	/* Now get a pointer to the record_field array. */

	record_field_array = record->record_field_array;

	if ( record_field_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The record_field_array pointer for the record %s is NULL.",
			record->name );
	}

	/* Loop through all of the fields in the record list. */

	token_number = MX_NUM_RECORD_ID_FIELDS;

	for ( i = 0; i < num_record_fields; i++ ) {

		/* Copy default values for this field to the
		 * MX_RECORD_FIELD structure.
		 */

		record_field = &record_field_array[i];

		record_field_defaults = &record_field_defaults_array[i];

		mx_copy_defaults_to_record_field(
			record_field, record_field_defaults );

		MX_DEBUG( 8,("====== Field = '%s' ======",record_field->name));

		MX_DEBUG( 8, ("record_field = %p, record_field_defaults = %p",
			record_field, record_field_defaults));

		/* Save a pointer to the record this field is part of. */

		record_field->record = record;

		record_field->active = FALSE;

		/* What type of field is this supposed to be? */

		field_type = record_field->datatype;

		/* Construct a pointer to the field data. */

		status = mx_construct_ptr_to_field_data(
			record, record_field_defaults, &field_data_ptr );

		if ( status.code != MXE_SUCCESS )
			return status;

		record_field->data_pointer = field_data_ptr;

		MX_DEBUG( 8,( "record_field->data_pointer = %p",
			record_field->data_pointer));

		/*** The first MX_NUM_RECORD_ID_FIELDS had their data value
		 *** fields initialized in mx_create_record_from_description(),
		 *** so we don't do anything further to initialize them.
		 ***/

		if ( i < MX_NUM_RECORD_ID_FIELDS ) {
			continue;  /* Go back to the start of the for() loop.*/
		}

		/***************************************************/
		/*** Proceed on for i >= MX_NUM_RECORD_ID_FIELDS ***/
		/***************************************************/

		MX_DEBUG( 8,("%s: record_field->num_dimensions = %ld",
			fname, record_field->num_dimensions));

		if ( record_field->num_dimensions > 0 ) {
			if ( record_field->dimension == NULL ) {
				MX_DEBUG( 8,
				("%s: record_field->dimension == NULL.",
					fname));
			} else {
				MX_DEBUG( 8,
				("%s: record_field->dimension[0] = %ld",
					fname, record_field->dimension[0]));
			}
		}

		/* If this is a varargs field, replace any
		 * varargs cookies with the actual array dimensions
		 * and then allocate memory for the field.
		 */

		if ( record_field->flags & MXFF_VARARGS ) {
			status = mx_replace_varargs_cookies_with_values(
				record, i, FALSE );

			if ( status.code != MXE_SUCCESS )
				return status;

			MX_DEBUG( 8,(
	"MXFF_VARARGS: num_dimensions = %ld, record_field->data_pointer = %p",
				record_field->num_dimensions,
				record_field->data_pointer));

			if ( record_field->num_dimensions < 0 ) {
				return mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname,
	"Record field '%s' has a negative number of dimensions = %ld",
					record_field->name,
					record_field->num_dimensions );
			} else if ( record_field->num_dimensions == 0 ) {
				return mx_error(
					MXE_UNSUPPORTED, fname,
"Record field '%s': An MXFF_VARARGS field with num_dimensions = 0 "
"is not currently supported.", record_field->name );

			} else {
				/* Don't allocate the array if one or more
				 * of the dimension sizes is <= 0.
				 */

				allocate_the_array = TRUE;

				for ( j = 0; j < record_field->num_dimensions;
						j++ )
				{
					dimension_size
						= record_field->dimension[j];

					MX_DEBUG( 8,(
			"MXFF_VARARGS: j = %ld, dimension_size = %ld",
						j, dimension_size));

					if ( dimension_size <= 0 )
						allocate_the_array = FALSE;

					/* An array dimension of zero is
					 * allowed since the record field
					 * is assumed to be a pointer that
					 * may be repointed at run time by
					 * a device driver.  An example of
					 * this is the 'scaler_data' field
					 * of the MX MCS driver.
					 *
					 * On the other hand, an array 
					 * dimension with a negative value
					 * is probably due to a programming
					 * bug, such as a varargs field that
					 * did not have a varargs cookie
					 * installed, so we emit a warning
					 * about it.
					 */

					if ( dimension_size < 0 ) {
						mx_warning(
"Array dimension %ld of record field '%s' for record '%s' has a negative "
"size, namely %ld.  This is probably a sign of a programming bug.",
						j, record_field->name,
						record->name,
						dimension_size );
					}
				}

				MX_DEBUG( 8,
				("MXFF_VARARGS: allocate_the_array = %d",
					allocate_the_array));

				if ( allocate_the_array == FALSE ) {
					array_ptr = NULL;
				} else {
					array_ptr = mx_allocate_array(
						field_type,
						record_field->num_dimensions,
						record_field->dimension,
					    record_field->data_element_size );

					if ( array_ptr == NULL ) {
						return mx_error(
							MXE_OUT_OF_MEMORY,
							fname,
			"Attempt to allocate memory for varargs field '%s' "
			"in record '%s' failed.", record_field->name,
						record->name );
					}
				}
				MX_DEBUG( 8,(
					"MXFF_VARARGS: --> array_ptr = %p",
					array_ptr ));

				mx_write_void_pointer_to_memory_location(
					field_data_ptr, array_ptr );
			}
		}

		/* Is this token supposed to be listed in the record 
		 * description string?
		 */

		if ( (record_field->flags & MXFF_IN_DESCRIPTION) == 0 ) {

			MX_DEBUG( 8,
		("Field[%ld] = '%s', field type = %s, no token expected.",
				i, record_field->name,
				mx_get_field_type_string(field_type)));

		} else {
			/**********************************************
			 * Figure out which function is used to parse *
			 * the field.                                 *
			 **********************************************/

			status = mx_get_token_parser( field_type, &fptr );

			if ( status.code != MXE_SUCCESS )
				return status;

			/* Construct a pointer to the field values. */

			if ( record_field->flags & MXFF_VARARGS ) {
				field_value_ptr =
				    mx_read_void_pointer_from_memory_location(
					field_data_ptr );

			} else {
				field_value_ptr = field_data_ptr;
			}

			MX_DEBUG( 8,
			    ("%s: field_data_ptr = %p, field_value_ptr = %p",
				fname, field_data_ptr, field_value_ptr));

			/* If this field is a string field, get the maximum
			 * length of a string token for use by the token
			 * parser.
			 */

			if ( field_type == MXFT_STRING ) {
				parse_status->max_string_token_length =
				  mx_get_max_string_token_length(record_field);
			} else {
				parse_status->max_string_token_length = 0;
			}

			/* If the input field is zero-dimensional or
			 * a one-dimensional string, call the token
			 * parser directly.
			 */

			if ( (record_field->num_dimensions == 0)
			  || ((field_type == MXFT_STRING)
			     && (record_field->num_dimensions == 1)) ) {

				/*** Single Token ***/

				/* Is there a next token for this field? */

				status = mx_get_next_record_token(parse_status, 
					token_buffer, sizeof(token_buffer) );

				if ( status.code != MXE_SUCCESS ) {
					return mx_error(
					MXE_UNPARSEABLE_STRING, fname,
				"Record description string was too short.  "
				"Did not find valid token(s) for field '%s'",
						record_field->name );
				}

				MX_DEBUG( 8,
		("Field[%ld] = '%s', field type = %s, token[%ld] = '%s'",
					i, record_field->name, 
					mx_get_field_type_string(field_type),
					token_number, token_buffer));

				token_number++;

				/* Now parse the token. */

				status = (*fptr)(field_value_ptr, token_buffer,
					record, record_field, parse_status);
			} else {
				/*** Array of tokens ***/

				/* Call mx_parse_array_description() to
				 * get all of the tokens and put them into 
				 * the array.
				 */

				MX_DEBUG( 8,
		("Field[%ld] = '%s', field type = %s, %ld dimensional array",
					i, record_field->name,
					mx_get_field_type_string(field_type),
					record_field->num_dimensions));

				token_number++;

				status = mx_parse_array_description(
					field_value_ptr,
					(record_field->num_dimensions - 1),
					record, record_field,
					parse_status, fptr );
			}
			switch( status.code ) {
			case MXE_SUCCESS:
				break;
			case MXE_UNPARSEABLE_STRING:
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Invalid token found in record '%s' at field '%s'.",
					record->name, record_field->name );
			default:
				return status;
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_get_next_record_token( MX_RECORD_FIELD_PARSE_STATUS *parse_status,
			char *output_buffer,
			size_t output_buffer_length)
{
	static const char fname[] = "mx_get_next_record_token()";

	int i, j, k, in_quoted_string, remaining_length, num_separators;
	int saw_separator;
	char *remaining_string, *separators;

	MX_DEBUG( 8,("%s entered.", fname));

	/* This implementation should be thread-safe if strlen()
	 * is thread-safe.
	 */

	if ( parse_status == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL MX_RECORD_FIELD_PARSE_STATUS structure passed." );
	}

	if ( output_buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL output buffer pointer passed." );
	}

	/* Need to start differently depending on whether this is the
	 * first call to this function for this description string.
	 * We use start_of_trailing_whitespace == NULL as the indicator
	 * that this is the first call.
	 */

	if ( parse_status->start_of_trailing_whitespace == NULL ) {

		remaining_string = parse_status->description;
	} else {
		remaining_string = parse_status->start_of_trailing_whitespace;
		remaining_string++;
	}

	separators = parse_status->separators;
	num_separators = parse_status->num_separators;

	remaining_length = (int) strlen(remaining_string);

	if ( remaining_length <= 0 ) {
		return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
			"String passed was of zero length." );
	}

	/* Skip any leading whitespace. */

	for ( i = 0; i < remaining_length; i++ ) {
		saw_separator = FALSE;

		for ( j = 0; j < num_separators; j++ ) {
			if ( remaining_string[i] == separators[j] ) {
				saw_separator = TRUE;
				break;       /* Exit the inner for() loop. */
			}
		}

		if ( saw_separator == FALSE )
			break;               /* Exit the outer for() loop. */
	}

	if ( i >= remaining_length ) {
		return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
			"String passed consisted entirely of whitespace." );
	}

	/* Is the non-whitespace character a double quote character
	 * denoting the beginning of a string?
	 */

	if ( remaining_string[i] != '"' ) {
		/* Anything other than a quoted string. */

		for ( k=0;
		      (i < remaining_length) && (k < output_buffer_length - 1);
		      i++, k++ ) {

			saw_separator = FALSE;

			for ( j = 0; j < num_separators; j++ ) {
				if ( remaining_string[i] == separators[j] ) {
					saw_separator = TRUE;
					break; /* Exit the inner for() loop */
				}
			}

			if ( saw_separator == TRUE ) {
				break;         /* Exit the outer for() loop */
			} else {
				output_buffer[k] = remaining_string[i];
			}
		}

		/* Make sure the output buffer is null terminated. */

		if ( k >= output_buffer_length - 1 ) {
			output_buffer[output_buffer_length - 1] = '\0';
		} else {
			output_buffer[k] = '\0';
		}
	} else {
		in_quoted_string = TRUE;

		i++;      /* Skip over the " character. */

		for ( k=0;
		      (i < remaining_length) && (k < output_buffer_length - 1);
		      i++, k++ ) {

			/* When we see another double quote, we are at
			 * the end of the quoted string token.
			 */

			if ( remaining_string[i] == '"' ) {
				in_quoted_string = FALSE;
				i++;         /* Skip over the " character. */
				break;       /* Exit the for() loop. */
			}

			output_buffer[k] = remaining_string[i];
		}

		/* Make sure the output buffer is null terminated. */

		if ( k >= output_buffer_length - 1 ) {
			output_buffer[output_buffer_length - 1] = '\0';
		} else {
			output_buffer[k] = '\0';
		}

		/* Did the string terminate normally? */

		if ( in_quoted_string == TRUE ) {
			if ( (k >= output_buffer_length - 1)
			  && (i < remaining_length) ) {

				return mx_error( MXE_LIMIT_WAS_EXCEEDED, fname,
"String token was too long to fit in calling routine's buffer size of %ld.  "
"The token in error started with '%s'", (long) output_buffer_length,
					output_buffer );

			} else {
				return mx_error(
					MXE_UNEXPECTED_END_OF_DATA, fname,
				"End of quoted string token not seen.  "
				"The token in error started with '%s'",
					output_buffer );
			}
		}
	}

	if ( i >= remaining_length ) {
		/* This is expected to end up pointed at a trailing '\0'
		 * character if things are going normally.  In this case,
		 * any attempt to get a "next token" out of the description
		 * will result in an error.
		 */

		parse_status->start_of_trailing_whitespace
			= &remaining_string[remaining_length];
	} else {
		parse_status->start_of_trailing_whitespace
			= &remaining_string[i];
	}

	MX_DEBUG( 8,("%s: token found = '%s'", fname, output_buffer));

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_get_token_parser( long field_type,
		mx_status_type ( **token_parser ) (
			void *dataptr,
			char *token,
			MX_RECORD *record,
			MX_RECORD_FIELD *record_field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status )
		)
{
	static const char fname[] = "mx_get_token_parser()";

	/* Figure out which function is used to parse the field. */

	switch( field_type ) {
	case MXFT_STRING:
		*token_parser = mx_parse_string_field;
		break;
	case MXFT_CHAR:
		*token_parser = mx_parse_char_field;
		break;
	case MXFT_UCHAR:
		*token_parser = mx_parse_uchar_field;
		break;
	case MXFT_SHORT:
		*token_parser = mx_parse_short_field;
		break;
	case MXFT_USHORT:
		*token_parser = mx_parse_ushort_field;
		break;
	case MXFT_BOOL:
		*token_parser = mx_parse_bool_field;
		break;
	case MXFT_LONG:
		*token_parser = mx_parse_long_field;
		break;
	case MXFT_ULONG:
		*token_parser = mx_parse_ulong_field;
		break;
	case MXFT_INT64:
		*token_parser = mx_parse_int64_field;
		break;
	case MXFT_UINT64:
		*token_parser = mx_parse_uint64_field;
		break;
	case MXFT_FLOAT:
		*token_parser = mx_parse_float_field;
		break;
	case MXFT_DOUBLE:
		*token_parser = mx_parse_double_field;
		break;
	case MXFT_HEX:
		*token_parser = mx_parse_hex_field;
		break;
	case MXFT_RECORD:
		*token_parser = mx_parse_mx_record_field;
		break;
	case MXFT_RECORDTYPE:
		*token_parser = mx_parse_recordtype_field;
		break;
	case MXFT_INTERFACE:
		*token_parser = mx_parse_interface_field;
		break;
	case MXFT_RECORD_FIELD:
		*token_parser = mx_parse_mx_record_field_field;
		break;
	default:
		*token_parser = NULL;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname, 
			"Unrecognized field type %ld.", field_type );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_get_token_constructor( long field_type,
		mx_status_type ( **token_constructor ) (
			void *dataptr,
			char *token_buffer,
			size_t token_buffer_length,
			MX_RECORD *record,
			MX_RECORD_FIELD *record_field )
		)
{
	static const char fname[] = "mx_get_token_constructor()";

	/* Figure out which function is used to
	 * construct the token for this field type.
	 */

	switch( field_type ) {
	case MXFT_STRING:
		*token_constructor = mx_construct_string_field;
		break;
	case MXFT_CHAR:
		*token_constructor = mx_construct_char_field;
		break;
	case MXFT_UCHAR:
		*token_constructor = mx_construct_uchar_field;
		break;
	case MXFT_SHORT:
		*token_constructor = mx_construct_short_field;
		break;
	case MXFT_USHORT:
		*token_constructor = mx_construct_ushort_field;
		break;
	case MXFT_BOOL:
		*token_constructor = mx_construct_bool_field;
		break;
	case MXFT_LONG:
		*token_constructor = mx_construct_long_field;
		break;
	case MXFT_ULONG:
		*token_constructor = mx_construct_ulong_field;
		break;
	case MXFT_INT64:
		*token_constructor = mx_construct_int64_field;
		break;
	case MXFT_UINT64:
		*token_constructor = mx_construct_uint64_field;
		break;
	case MXFT_FLOAT:
		*token_constructor = mx_construct_float_field;
		break;
	case MXFT_DOUBLE:
		*token_constructor = mx_construct_double_field;
		break;
	case MXFT_HEX:
		*token_constructor = mx_construct_hex_field;
		break;
	case MXFT_RECORD:
		*token_constructor = mx_construct_mx_record_field;
		break;
	case MXFT_RECORDTYPE:
		*token_constructor = mx_construct_recordtype_field;
		break;
	case MXFT_INTERFACE:
		*token_constructor = mx_construct_interface_field;
		break;
	case MXFT_RECORD_FIELD:
		*token_constructor = mx_construct_mx_record_field_field;
		break;
	default:
		*token_constructor = NULL;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized field type %ld.", field_type );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_create_array_description( void *array_ptr,
		long dimension_level,
		char *buffer_ptr, size_t max_buffer_length,
		MX_RECORD *record, MX_RECORD_FIELD *field,
		mx_status_type (*token_creater) ( void *, char *, size_t,
					MX_RECORD *, MX_RECORD_FIELD * ) )
{
	static const char fname[] = "mx_create_array_description()";

	const char short_label[] = "creating array";

	long i, n, num_dimension_elements, new_dimension_level;
	long row_ptr_step_size;
	size_t dimension_element_size, subarray_size;
	char *row_ptr, *element_ptr;
	void *new_array_ptr;
	char *new_buffer_ptr;
	long buffer_current_length, new_buffer_length;
	mx_status_type status;

	MX_DEBUG( 2,(".................................................."));

	dimension_element_size = field->data_element_size[ dimension_level ];

	n = field->num_dimensions - dimension_level - 1;

	num_dimension_elements = field->dimension[n];

	new_dimension_level = dimension_level - 1;

	if ( record != NULL ) {
		MX_DEBUG( 8,("%s: record = '%s'", fname, record->name));
	}
	if ( field != NULL ) {
		MX_DEBUG( 8,("%s: field = '%s'", fname, field->name));
	}

	MX_DEBUG( 8,
	("%s: dimension_level = %ld, n = %ld, num_dimension_elements = %ld",
		 short_label, dimension_level, n, num_dimension_elements));
	MX_DEBUG( 8,
	("%s:    array_ptr = %p, buffer_ptr = %p, max_buffer_length = %ld",
		short_label, array_ptr, buffer_ptr, (long) max_buffer_length));

	if (dimension_level == 0) {
	    if (field->datatype == MXFT_STRING) {

		/**** Strings must be handled specially. ****/

		strlcat( buffer_ptr, " ", max_buffer_length );

		buffer_current_length = (long) strlen(buffer_ptr);

		new_buffer_ptr = buffer_ptr + buffer_current_length;
		new_buffer_length = (long) max_buffer_length
						- buffer_current_length;

		if ( new_buffer_length <= 0 ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"No room left for next token in array description." );
		}

		/* Create the token. */

		status = (*token_creater)( array_ptr,
		    new_buffer_ptr, (size_t) new_buffer_length, record, field );

		if ( status.code != MXE_SUCCESS )
			return status;

		MX_DEBUG( 8,("%s: *****> String token = '%s'",
				short_label, new_buffer_ptr));

	    } else {

		/** At bottom level of the array, so start creating tokens. **/

		MX_DEBUG( 8,
			("%s: +++> Token creater to be invoked for %ld tokens",
			short_label, num_dimension_elements));

		element_ptr = (char *) array_ptr;

		for ( i = 0; i < num_dimension_elements; i++ ) {

			strlcat( buffer_ptr, " ", max_buffer_length );

			buffer_current_length = (long) strlen(buffer_ptr);

			new_buffer_ptr = buffer_ptr + buffer_current_length;
			new_buffer_length = (long) max_buffer_length
						- buffer_current_length;

			MX_DEBUG( 9,
	("%s: i = %ld, buffer_current_length = %ld, new_buffer_length = %ld",
				short_label, i, buffer_current_length,
				new_buffer_length));

			if ( new_buffer_length <= 0 ) {
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"No room left for next token in array description." );
			}

			/* Create the token. */

			MX_DEBUG( 2,
		("%s: About to create token %ld from element_ptr %p",
				short_label, i, element_ptr));

			status = (*token_creater)( (void *) element_ptr,
				new_buffer_ptr, (size_t) new_buffer_length,
				record, field );

			if ( status.code != MXE_SUCCESS )
				return status;

			MX_DEBUG( 8,
		("%s: *****> Array token [%ld] = '%s', element_ptr = %p",
			short_label, i, new_buffer_ptr, element_ptr ));

			element_ptr += dimension_element_size;
		}
	    }
	} else {
		/** More levels in the array to descend through. **/

		MX_DEBUG( 8,
("%s: -> At dimension level %ld.  Go to next dimension level = %ld",
			short_label, dimension_level, new_dimension_level));

		row_ptr = (char *)array_ptr;

		if ( field->flags & MXFF_VARARGS ) {
			row_ptr_step_size = (long) dimension_element_size;

		} else {
			status = mx_compute_static_subarray_size( array_ptr,
				dimension_level, field, &subarray_size );

			if ( status.code != MXE_SUCCESS )
				return status;

			row_ptr_step_size = (long) subarray_size;
		}

		MX_DEBUG( 8,("%s: row_ptr = %p, row_ptr_step_size = %ld",
			fname, row_ptr, row_ptr_step_size));

		for ( i = 0; i < num_dimension_elements; i++ ) {

			if ( field->flags & MXFF_VARARGS ) {
				new_array_ptr =
				    mx_read_void_pointer_from_memory_location(
					row_ptr );

				MX_DEBUG( 2,
	("%s: DEREFERENCING row %ld row_ptr %p to get new_array_ptr %p",
					fname, i, row_ptr, new_array_ptr));
			} else {
				new_array_ptr = row_ptr;

				MX_DEBUG( 2,
	("%s: NO DEREFERENCE needed for row %ld row_ptr %p",
					fname, i, row_ptr));
			}

			buffer_current_length = (long) strlen(buffer_ptr);

			new_buffer_ptr = buffer_ptr + buffer_current_length;
			new_buffer_length = (long) max_buffer_length
						- buffer_current_length;

			status = mx_create_array_description( new_array_ptr,
				new_dimension_level,
				new_buffer_ptr, (size_t) new_buffer_length,
				record, field, token_creater );

			if ( status.code != MXE_SUCCESS )
				return status;

			row_ptr += row_ptr_step_size;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_create_description_from_record(
		MX_RECORD *current_record,
		char *description_buffer,
		size_t description_buffer_length )
{
	static const char fname[] = "mx_create_description_from_record()";

	MX_RECORD_FIELD *record_field, *record_field_array;
	mx_status_type (*fptr)( void *, char *, size_t,
				MX_RECORD *, MX_RECORD_FIELD * );

	/* See the comment in mx_create_record_from_description()
	 * for the difference between the following two pointers.
	 */

	void *field_data_ptr;
	void *field_value_ptr;

	char token_buffer[MXU_BUFFER_LENGTH+1];
	char *buffer_ptr;
	long i, num_record_fields, field_type;
	long token_number, buffer_current_length, buffer_space_left;
	mx_status_type status;

	if ( current_record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}
	if ( description_buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"description_buffer pointer is NULL.");
	}
	if ( ((long) description_buffer_length) <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal description buffer length = %ld",
			(long) description_buffer_length );
	}

	/* Does this record type have new style record field support
	 * that we can use to write out the record?
	 */

	record_field_array = current_record->record_field_array;

	if ( record_field_array == NULL ) {

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The old style record using write_record_description() is no longer supported."
				);

	} else {
		/* Record field support _is_available, so use it to
		 * write out the record.
		 */

		MX_DEBUG( 8,("%s: record '%s' uses new record field support.",
			fname, current_record->name));

		description_buffer[0] = '\0';

		buffer_ptr = description_buffer;

		num_record_fields = current_record->num_record_fields;

		if ( num_record_fields < MX_NUM_RECORD_ID_FIELDS ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"Record '%s' has only %ld record fields.  The minimum allowed is %d",
				current_record->name,
				num_record_fields,
				MX_NUM_RECORD_ID_FIELDS );
		}

		/* The record name field is formatted specially.  We use
		 * the pointer current_record->name directly, since all
		 * records MUST have this field.
		 */

		snprintf( description_buffer, description_buffer_length,
			"%*s", -(MXU_FIELD_NAME_LENGTH),
			current_record->name );

		buffer_current_length = (long) strlen(description_buffer);
		buffer_ptr = description_buffer + buffer_current_length;

		token_number = 1;

		for ( i = 1; i < num_record_fields; i++ ) {
			/* What type of record field is this? */

			record_field = &record_field_array[i];

			field_type = record_field->datatype;

			/* Is this token supposed to be listed in the record
			 * description string?
			 */

			if ((record_field->flags & MXFF_IN_DESCRIPTION) == 0){
				MX_DEBUG( 8,
		("Field[%ld] = '%s', field type = %s, no token expected.",
					i, record_field->name,
					mx_get_field_type_string(field_type)));
			} else {
				field_data_ptr = record_field->data_pointer;

				/* Put a space between fields. */

				buffer_current_length =
					(long) strlen(description_buffer);

				buffer_space_left = 
					(long) description_buffer_length
					- buffer_current_length - 1;

				strlcat( buffer_ptr, " ", buffer_space_left );

				/* Construct a pointer to the place in the
				 * description buffer where the token is
				 * to be written to.
				 */

				buffer_current_length
					= (long) strlen( description_buffer );

				buffer_ptr = description_buffer
					+ buffer_current_length;

				/******************************************
				 * Figure out which function is used to   *
				 * construct the token(s) for this field. *
				 ******************************************/

				status = mx_get_token_constructor(
							field_type, &fptr );

				if ( status.code != MXE_SUCCESS )
					return status;

				/* Construct a pointer to the field values. */

				if ( record_field->flags & MXFF_VARARGS ) {
					field_value_ptr =
				    mx_read_void_pointer_from_memory_location(
						field_data_ptr );

				} else {
					field_value_ptr = field_data_ptr;
				}

				MX_DEBUG( 8,
			("%s: field_data_ptr = %p, field_value_ptr = %p",
				    fname, field_data_ptr, field_value_ptr));

				/* Is this field zero-dimensional or
				 * a one-dimensional string?
				 */

				if ( (record_field->num_dimensions == 0)
				  || ((field_type == MXFT_STRING)
				     && (record_field->num_dimensions == 1))){

					/*** Single Token ***/

					/* Construct the token. */

					status = (*fptr)(
						field_value_ptr,
						token_buffer,
						sizeof(token_buffer),
						current_record,
						record_field);

					if ( status.code != MXE_SUCCESS )
						return status;

					MX_DEBUG( 8,
		("Field[%ld] = '%s', field type = %s, token[%ld] = '%s'",
					i, record_field->name,
					mx_get_field_type_string(field_type),
					token_number, token_buffer));

					token_number++;

					/* Now copy the token to the
					 * description buffer.
					 */

					buffer_space_left =
						(long)description_buffer_length
						- buffer_current_length - 1;

					if ( strlen(token_buffer)
						> buffer_space_left ) {

						return mx_error(
							MXE_WOULD_EXCEED_LIMIT,
							fname,
			"Token '%s' is too long to fit into the remaining "
			"%ld bytes in the description buffer",
							token_buffer,
							buffer_space_left );
					}

					strlcat( buffer_ptr, token_buffer,
							buffer_space_left );
				} else {
					/*** Array of tokens ***/

					/* Call mx_create_array_description()
					 * to construct a character string
					 * description which consists of
					 * the printable ASCII values of the
					 * array elements.
					 */

					MX_DEBUG( 8,
		("Field[%ld] = '%s', field type = %s, %ld dimensional array",
					i, record_field->name,
					mx_get_field_type_string(field_type),
					record_field->num_dimensions));

					token_number++;

					buffer_space_left =
						(long)description_buffer_length
						- buffer_current_length - 1;

					status
				= mx_create_array_description(
					    field_value_ptr,
					    (record_field->num_dimensions - 1),
					    buffer_ptr, buffer_space_left,
					    current_record, record_field,
					    fptr );

					if ( status.code != MXE_SUCCESS )
						return status;
				}
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT mx_bool_type
mx_database_is_valid( MX_RECORD *record, unsigned long flags )
{
	static const char fname[] = "mx_database_is_valid()";

	MX_RECORD *list_head_record, *current_record;
	MX_RECORD *previous_record, *next_record;
	int valid_pointer;

	if ( flags & MXF_DATABASE_VALID_DEBUG ) {
		MX_DEBUG(-2,("%s invoked for record pointer %p",
			fname, record ));
	}

	valid_pointer = mx_pointer_is_valid( record, sizeof(MX_RECORD), R_OK );

	if ( valid_pointer == FALSE ) {
		mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The MX_RECORD pointer (%p) passed to us "
		"is not a valid pointer.",
			record );

		return FALSE;
	}

	list_head_record = record->list_head;

	valid_pointer = mx_pointer_is_valid( record, sizeof(MX_RECORD), R_OK );

	if ( valid_pointer == FALSE ) {
		mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The list_head pointer (%p) for record '%s' (%p) "
		"passed to us is invalid.",
			list_head_record,
			record->name,
			record );

		return FALSE;
	}

	current_record = list_head_record;

	while (1) {
		if ( flags & MXF_DATABASE_VALID_DEBUG ) {
			MX_DEBUG(-2,
			("%s: current_record = %p", fname, current_record));

			MX_DEBUG(-2,("%s:     current_record name = '%s'",
				fname, current_record->name));
		}

		/* See if the next_record pointer is valid. */

		next_record = current_record->next_record;

		valid_pointer = mx_pointer_is_valid( record,
						sizeof(record), R_OK );

		if ( valid_pointer == FALSE ) {
			mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The next_record pointer (%p) for current record "
			"'%s' (%p) is not a valid pointer.",
				next_record, current_record->name,
				current_record );

			return FALSE;
		}

		/* Does the next record's previous_record pointer
		 * point back to us?
		 */

		if ( current_record != next_record->previous_record ) {
			if ( flags & MXF_DATABASE_VALID_DEBUG ) {
				MX_DEBUG(-2,("%s: next_record = '%s' (%p)",
				fname, next_record->name, next_record ));
			}

			mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The record list structure is broken, since "
			"current_record '%s' = %p is not equal to "
			"next_record->previous_record = %p.",
				current_record->name, current_record,
				next_record->previous_record );

			return FALSE;
		}

		/* See if the previous_record pointer is valid. */

		previous_record = current_record->previous_record;

		valid_pointer = mx_pointer_is_valid( record,
						sizeof(record), R_OK );

		if ( valid_pointer == FALSE ) {
			mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The previous_record pointer (%p) for current record "
			"'%s' (%p) is not a valid pointer.",
				previous_record, current_record->name,
				current_record );

			return FALSE;
		}

		/* Does the previous record's next_record pointer
		 * point back to us?
		 */

		if ( current_record != previous_record->next_record ) {
			if ( flags & MXF_DATABASE_VALID_DEBUG ) {
				MX_DEBUG(-2,("%s: previous_record = '%s' (%p)",
				fname, previous_record->name, previous_record));
			}

			mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The record list structure is broken, since "
			"current_record '%s' = %p is not equal to "
			"previous_record->next_record = %p.",
				current_record->name, current_record,
				previous_record->next_record );

			return FALSE;
		}

		/* Proceed on to the next record. */

		current_record = current_record->next_record;

		/* Exit the loop if we have returned to the list head record. */

		if ( current_record == list_head_record )
			break;
	}

	return TRUE;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_construct_placeholder_record( MX_RECORD *referencing_record,
		char *record_name, MX_RECORD **placeholder_record )
{
	static const char fname[] = "mx_construct_placeholder_record()";

	if ( record_name == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
				"Pointer to record name is NULL." );
	}
	if ( placeholder_record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD ** passed was NULL." );
	}

	MX_DEBUG( 8, ("%s: record name = '%s'", fname, record_name));

	*placeholder_record = (MX_RECORD *) malloc( sizeof(MX_RECORD) );

	if ( *placeholder_record == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Out of memory trying to allocate placeholder record for '%s'",
			record_name );
	}

	memset( *placeholder_record, 0x0, sizeof(MX_RECORD) );

	strlcpy( (*placeholder_record)->name,
			record_name, MXU_RECORD_NAME_LENGTH );

	(*placeholder_record)->mx_superclass = MXR_PLACEHOLDER;

	(*placeholder_record)->allocated_by = referencing_record;

	return MX_SUCCESSFUL_RESULT;
}

#define KEEP_SCAN_PLACEHOLDERS	TRUE

#if KEEP_SCAN_PLACEHOLDERS
static void
mxp_scan_save_placeholder( MX_RECORD *referencing_record,
			MX_RECORD *placeholder_record )
{
	static const char fname[] = "mxp_scan_save_placeholer()";

	MX_SCAN *scan;

	if ( referencing_record == (MX_RECORD *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
			"The referencing_record pointer passed was NULL." );
		return;
	}
	if ( placeholder_record == (MX_RECORD *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
			"The placeholder_record pointer passed was NULL." );
		return;
	}

	scan = (MX_SCAN *) referencing_record->record_superclass_struct;

	if ( scan == (MX_SCAN *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SCAN pointer for scan '%s' is NULL.",
			referencing_record->name );
		return;
	}

	/* Increase the size of missing_record_array. */

	if ( scan->missing_record_array == (MX_RECORD **) NULL ) {
		scan->missing_record_array =
			(MX_RECORD **) malloc( sizeof(MX_RECORD *) );

		scan->num_missing_records = 1;
	} else {
		scan->missing_record_array = (MX_RECORD **)
			realloc( scan->missing_record_array,
				(scan->num_missing_records + 1)
					* sizeof(MX_RECORD *) );

		if ( scan->missing_record_array == (MX_RECORD **) NULL ) {
			(void) mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to increase the size "
			"of a placeholder missing_record_array referred "
			"to by scan '%s' to %lu elements.",
				referencing_record->name,
				scan->num_missing_records + 1 );
		}

		scan->num_missing_records++;
	}

	if ( scan->missing_record_array == (MX_RECORD **) NULL ) {
		(void) mx_error( MXE_OUT_OF_MEMORY, fname,
    "Ran out of memory trying to allocate a %ld array of MX_RECORD pointers.",
    			scan->num_missing_records );
		return;
	}

	/* Save the placeholder record in the missing record array so that
	 * we can find it if we need to delete it at some later date.
	 */

	scan->missing_record_array[ scan->num_missing_records - 1 ]
		= placeholder_record;

	return;
}
#endif

MX_EXPORT mx_status_type
mx_fixup_placeholder_records( MX_RECORD *list_head_record )
{
	static const char fname[] = "mx_fixup_placeholder_records()";

	MX_LIST_HEAD *list_head_struct;
	MX_RECORD *real_record, *placeholder_record, *referencing_record;
	void **fixup_record_array;
	void *memory_location;
	long i, num_fixup_records;

	list_head_struct
	    = (MX_LIST_HEAD *) (list_head_record->record_superclass_struct);

	num_fixup_records = list_head_struct->num_fixup_records;

	if ( num_fixup_records <= 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	fixup_record_array = list_head_struct->fixup_record_array;

	if ( fixup_record_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"fixup_record_array = NULL, although num_fixup_records = %ld",
			num_fixup_records );
	}

	/* Look for the real record corresponding to the name mentioned
	 * in the placeholder record.
	 */

	for ( i = 0; i < num_fixup_records; i++ ) {
		memory_location = fixup_record_array[i];

		MX_DEBUG( 8, ("%s: i = %ld, memory_location = %p",
			fname, i, memory_location));

		/* If memory_location is NULL, the corresponding 
		 * placeholder record has already been deleted.
		 */

		if ( memory_location == NULL ) {

			continue;   /* Go back to the top of the for() loop. */
		}

		placeholder_record = (MX_RECORD *)
		  mx_read_void_pointer_from_memory_location( memory_location );

		if ( placeholder_record == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"fixup_record_array[%ld] == NULL when num_fixup_records = %ld.",
				i, num_fixup_records );
		}

		referencing_record
			= (MX_RECORD *)(placeholder_record->allocated_by);

		MX_DEBUG( 8, ("%s: placeholder = %p, referencing_record = %p",
			fname, placeholder_record, referencing_record));

		if ( referencing_record == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"Placeholder for record '%s' is not referenced by any record.",
				placeholder_record->name );
		}

		MX_DEBUG( 8, ("%s: name = '%s', referencing record = '%s'.",
			fname, placeholder_record->name,
			referencing_record->name ));

		real_record = mx_get_record(
				list_head_record, placeholder_record->name );

		if ( real_record == NULL ) {
			if ( referencing_record->mx_superclass != MXR_SCAN ) {
				return mx_error( MXE_NOT_FOUND, fname,
			"Record '%s' mentioned by record '%s' does not exist.",
					placeholder_record->name,
					referencing_record->name );
			} else {
#if KEEP_SCAN_PLACEHOLDERS
				mx_warning(
		"Record '%s' mentioned by scan record '%s' does not exist.  "
		"Scan record '%s' will not be scannable.",
					placeholder_record->name,
					referencing_record->name,
					referencing_record->name );

				mxp_scan_save_placeholder( referencing_record,
							placeholder_record );

#else /* KEEP_SCAN_PLACEHOLDERS */
				mx_warning(
		"Record '%s' mentioned by scan record '%s' does not exist.  "
		"Scan record '%s' will be deleted.",
					placeholder_record->name,
					referencing_record->name,
					referencing_record->name );

				free( placeholder_record );

				/* Marking the record as broken _should_ result
				 * in the record being deleted by the function
				 * that called us.
				 */

				referencing_record->record_flags
					|= MXF_REC_BROKEN;
#endif /* KEEP_SCAN_PLACEHOLDERS */
			}
		} else {

			MX_DEBUG( 8, ("%s: real_record = %p '%s'",
				fname, real_record, real_record->name));

			free( placeholder_record );

			mx_write_void_pointer_to_memory_location(
						memory_location, real_record );
		}
	}

	list_head_struct->fixup_records_in_use = FALSE;

	list_head_struct->num_fixup_records = 0;

	free( list_head_struct->fixup_record_array );

	list_head_struct->fixup_record_array = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_datatype_sizeof_array( long datatype, size_t **sizeof_array )
{
	static const char fname[] = "mx_get_datatype_sizeof_array()";

	static size_t string_sizeof[MXU_FIELD_MAX_DIMENSIONS]
							= MXA_STRING_SIZEOF;
	static size_t char_sizeof[MXU_FIELD_MAX_DIMENSIONS]  = MXA_CHAR_SIZEOF;
	static size_t uchar_sizeof[MXU_FIELD_MAX_DIMENSIONS] = MXA_UCHAR_SIZEOF;
	static size_t short_sizeof[MXU_FIELD_MAX_DIMENSIONS] = MXA_SHORT_SIZEOF;
	static size_t ushort_sizeof[MXU_FIELD_MAX_DIMENSIONS]
							= MXA_USHORT_SIZEOF;
	static size_t bool_sizeof[MXU_FIELD_MAX_DIMENSIONS]  = MXA_BOOL_SIZEOF;
	static size_t long_sizeof[MXU_FIELD_MAX_DIMENSIONS]  = MXA_LONG_SIZEOF;
	static size_t ulong_sizeof[MXU_FIELD_MAX_DIMENSIONS] = MXA_ULONG_SIZEOF;
	static size_t int64_sizeof[MXU_FIELD_MAX_DIMENSIONS]
							= MXA_INT64_SIZEOF;
	static size_t uint64_sizeof[MXU_FIELD_MAX_DIMENSIONS]
							= MXA_UINT64_SIZEOF;
	static size_t float_sizeof[MXU_FIELD_MAX_DIMENSIONS] = MXA_FLOAT_SIZEOF;
	static size_t double_sizeof[MXU_FIELD_MAX_DIMENSIONS]
							= MXA_DOUBLE_SIZEOF;
#if 0
	static size_t record_sizeof[MXU_FIELD_MAX_DIMENSIONS]
							= MXA_RECORD_SIZEOF;
	static size_t interface_sizeof[MXU_FIELD_MAX_DIMENSIONS]
							= MXA_INTERFACE_SIZEOF;
#endif

	switch( datatype ) {
	case MXFT_STRING:
		*sizeof_array = string_sizeof;
		break;
	case MXFT_CHAR:
		*sizeof_array = char_sizeof;
		break;
	case MXFT_UCHAR:
		*sizeof_array = uchar_sizeof;
		break;
	case MXFT_SHORT:
		*sizeof_array = short_sizeof;
		break;
	case MXFT_USHORT:
		*sizeof_array = ushort_sizeof;
		break;
	case MXFT_BOOL:
		*sizeof_array = bool_sizeof;
		break;
	case MXFT_LONG:
		*sizeof_array = long_sizeof;
		break;
	case MXFT_ULONG:
	case MXFT_HEX:
		*sizeof_array = ulong_sizeof;
		break;
	case MXFT_INT64:
		*sizeof_array = int64_sizeof;
		break;
	case MXFT_UINT64:
		*sizeof_array = uint64_sizeof;
		break;
	case MXFT_FLOAT:
		*sizeof_array = float_sizeof;
		break;
	case MXFT_DOUBLE:
		*sizeof_array = double_sizeof;
		break;
	case MXFT_RECORD:
	case MXFT_RECORDTYPE:
	case MXFT_INTERFACE:
	case MXFT_RECORD_FIELD:
		*sizeof_array = string_sizeof;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported datatype argument %ld.", datatype );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_initialize_temp_record_field( MX_RECORD_FIELD *temp_record_field,
			long datatype,
			long num_dimensions,
			long *dimension,
			size_t *data_element_size,
			void *value_ptr )
{
	static const char fname[] = "mx_initialize_temp_record_field()";

	static char temp_field_name[] = "temp_field";

	size_t *sizeof_array;
	long i;
	mx_status_type status;

	if ( temp_record_field == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD_FIELD pointer passed is NULL." );
	}
#if 0
	if ( dimension == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"dimension array pointer passed is NULL." );
	}
#endif
	if ( data_element_size == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"data_element_size array pointer passed is NULL." );
	}
	temp_record_field->label_value = 0;
	temp_record_field->field_number = 0;
	temp_record_field->name = temp_field_name;
	temp_record_field->datatype = datatype;
	temp_record_field->typeinfo = NULL;
	temp_record_field->num_dimensions = num_dimensions;
	temp_record_field->dimension = dimension;
	temp_record_field->data_element_size = data_element_size;
	temp_record_field->process_function = NULL;
	temp_record_field->flags = MXFF_VARARGS;
	temp_record_field->callback_list = NULL;
	temp_record_field->application_ptr = NULL;
	temp_record_field->record = NULL;
	temp_record_field->active = FALSE;
	temp_record_field->data_pointer = value_ptr;

	if ( num_dimensions <= 0L ) {
		temp_record_field->data_element_size[0] = 0L;

	} else if ( ( data_element_size == NULL )
		 || ( data_element_size[0] == 0L ) )
	{
		status = mx_get_datatype_sizeof_array( datatype,
						&sizeof_array );

		if ( status.code != MXE_SUCCESS )
			return status;

		for ( i = 0; i < num_dimensions; i++ ) {
			temp_record_field->data_element_size[i]
				= sizeof_array[i];
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/* mx_traverse_field() is used to walk through all the array levels of
 * the data managed by an MX_RECORD_FIELD and apply a handler function
 * to the data at each level.
 *
 * Please note that the MX_RECORD pointer may be NULL under certain
 * circumstances.  For example, the Python wrapper Mp does this.
 */

MX_EXPORT mx_status_type
mx_traverse_field( MX_RECORD *record,
		MX_RECORD_FIELD *field,
		MX_TRAVERSE_FIELD_HANDLER *handler_fn,
		void *handler_data_ptr,
		long *array_indices )
{
	static const char fname[] = "mx_traverse_field()";

	void *array_ptr;
	long i, dimension_level;
	mx_status_type status;

	MX_DEBUG( 4,("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

	/* Get a pointer to the data in the record field. */

	if ( field->flags & MXFF_VARARGS ) {
		array_ptr = mx_read_void_pointer_from_memory_location(
				field->data_pointer );
		MX_DEBUG( 4,
		("%s: DEREFERENCING data_pointer %p to get array_ptr %p",
			fname, field->data_pointer, array_ptr));
	} else {
		array_ptr = field->data_pointer;

		MX_DEBUG( 4,("%s: NO DEREFERENCE of data pointer %p",
			fname, field->data_pointer));
	}

	if ( array_ptr == NULL ) {

		/* There is nothing in the field, so there is nothing to do. */

		return MX_SUCCESSFUL_RESULT;
	}

	/* dimension_level tells us how far away we are from the lowest
	 * level in the field array.
	 */

	dimension_level = field->num_dimensions;

	MX_DEBUG( 4,("%s: field->num_dimensions = %ld",
			fname, field->num_dimensions));

	if ( dimension_level < 0 ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"Illegal number of dimensions %ld for field '%s'",
			dimension_level, field->name );
	}

	/* At some arbitrary level in traversing the field data, the
	 * pointer array_ptr points to some slice of the total data
	 * belonging to the field.  If defined, array_indices is used
	 * to contain the array index of this slice in the field data
	 * as a whole.  Since we are at the top level, array_ptr
	 * currently refers to the array as a whole, so we initialize
	 * all the array indices to -1L to indicate that they are not
	 * valid values.
	 */

	if ( array_indices != NULL ) {
		for ( i = 0; i < field->num_dimensions; i++ ) {
			array_indices[i] = -1L;
		}
	}

	/* Invoke the traverse_field handler on the top level of the array. */

	MX_DEBUG( 4,("%s: dimension_level = %ld", fname, dimension_level));

	if (( dimension_level == 0 )
	  || (( field->datatype == MXFT_STRING ) && ( dimension_level == 1 )))
	{
		MX_DEBUG( 4,("ROUTE *, ROUTE *, ROUTE *, ROUTE *, ROUTE *"));
		MX_DEBUG( 4,("ROUTE *, field = '%s'", field->name));

		status = (*handler_fn)( record,
				field,
				handler_data_ptr,
				array_indices,
				array_ptr,
				dimension_level );

		if ( status.code != MXE_SUCCESS )
			return status;
	} else {
		MX_DEBUG( 4,("ROUTE 0, ROUTE 0, ROUTE 0, ROUTE 0, ROUTE 0"));
		MX_DEBUG( 4,("ROUTE 0, field = '%s'", field->name));

		status = (*handler_fn)( record,
				field,
				handler_data_ptr,
				array_indices,
				array_ptr,
				dimension_level );

		if ( status.code != MXE_SUCCESS )
			return status;

		/* If we are not starting at the bottom level of the array,
		 * recurse down to the next lower level.
		 */

		dimension_level--;

		status = mx_traverse_field_array( record,
						field,
						handler_fn,
						handler_data_ptr,
						array_indices,
						array_ptr,
						dimension_level );
	}

	return status;
}

MX_EXPORT mx_status_type
mx_traverse_field_array( MX_RECORD *record,
			MX_RECORD_FIELD *field,
			MX_TRAVERSE_FIELD_HANDLER *handler_fn,
			void *handler_data_ptr,
			long *array_indices,
			void *array_ptr,
			long dimension_level )
{
	static const char fname[] = "mx_traverse_field_array()";

	long i, n, num_dimension_elements, new_dimension_level;
	long row_ptr_step_size;
	size_t dimension_element_size, subarray_size;
	char *row_ptr, *element_ptr;
	void *new_array_ptr;
	int dynamically_allocated;
	mx_status_type status;

	MX_DEBUG( 8,
	("====== mx_traverse_field_array(), dimension_level = %ld  ======",
		dimension_level));

	MX_DEBUG( 8,("Field '%s', dimension level = %ld, num_dimensions = %ld",
		field->name, dimension_level,
		field->num_dimensions));

	if ( field->flags & MXFF_VARARGS ) {
		dynamically_allocated = TRUE;
	} else {
		dynamically_allocated = FALSE;
	}

	dimension_element_size = field->data_element_size[ dimension_level ];

	n = field->num_dimensions - dimension_level - 1;

	num_dimension_elements = field->dimension[n];

	new_dimension_level = dimension_level - 1;

	MX_DEBUG( 8,
	("%s: dimension_level = %ld, n = %ld, num_dimension_elements = %ld",
		fname, dimension_level, n, num_dimension_elements));
	MX_DEBUG( 8,("%s:    array_ptr = %p", fname, array_ptr));

	/* Decide what we have to do about the next level down. */

	if ( ( dimension_level == 0 ) 
	  || ( (field->datatype == MXFT_STRING) && (dimension_level == 1) ) )
	{

		MX_DEBUG( 4,("ROUTE 1, ROUTE 1, ROUTE 1, ROUTE1, ROUTE 1"));

#if 0
		if ( field->datatype == MXFT_STRING ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Encountered an MXFT_STRING at dimension level 0.  "
	"This message should never appear, so if you see it "
	"you've found a bug in MX and should report it to the author." );
		}
#endif

		/* Step through the bottom level of the array.  */

		element_ptr = (char *)array_ptr;

		for ( i = 0; i < num_dimension_elements; i++ ) {

			MX_DEBUG( 4,("ROUTE 1, i = %ld, n = %ld", i, n));

			if ( array_indices != NULL ) {
				array_indices[n] = i;

				MX_DEBUG( 4,
					("ROUTE 1, array_indices[%ld] = %ld",
					n, array_indices[n]));
			}

#if 1
			if ( field->datatype == MXFT_STRING ) {
				new_array_ptr =
		mx_read_void_pointer_from_memory_location( element_ptr );
			} else {
				new_array_ptr = element_ptr;
			}
#else
			new_array_ptr = element_ptr;
#endif

			status = (*handler_fn)( record,
						field,
						handler_data_ptr,
						array_indices,
						new_array_ptr,
						dimension_level );

			if ( status.code != MXE_SUCCESS )
				return status;

			element_ptr += dimension_element_size;
		}
#if 0
	} else
	if ( (field->datatype == MXFT_STRING) && (dimension_level == 1) ) {

		/* Strings are specially handled. */

		MX_DEBUG( 4,("ROUTE 2, ROUTE 2, ROUTE 2, ROUTE 2, ROUTE 2"));

		if ( array_indices != NULL ) {
			array_indices[n] = 0;
		}

		if ( dynamically_allocated ) {
			new_array_ptr =
		    mx_read_void_pointer_from_memory_location( array_ptr );

			MX_DEBUG( 4,
	("%s: DEREFERENCING string array_ptr %p to get new_array_ptr %p",
					fname, array_ptr, new_array_ptr));
		} else {
			new_array_ptr = array_ptr;

			MX_DEBUG( 4,("%s: NO DEREFERENCE of array_ptr %p",
					fname,  array_ptr ));
		}

		status = (*handler_fn)( record,
					field,
					handler_data_ptr,
					array_indices,
					new_array_ptr,
					dimension_level );

		if ( status.code != MXE_SUCCESS )
			return status;
#endif
	} else {
		MX_DEBUG( 4,("ROUTE 3, ROUTE 3, ROUTE 3, ROUTE3, ROUTE 3"));

		/* More levels in the array to descend through. */

		MX_DEBUG( 8,("%s: -> Go to next dimension level = %ld",
			fname, new_dimension_level));

		/* Compute how far apart in memory the row pointers are. */

		if ( dynamically_allocated ) {
			row_ptr_step_size = (long) dimension_element_size;
		} else {
			status = mx_compute_static_subarray_size( array_ptr,
				dimension_level, field, &subarray_size );

			if ( status.code != MXE_SUCCESS )
				return status;

			row_ptr_step_size = (long) subarray_size;
		}

		/* Step through each subarray corresponding to one
		 * element in this row.
		 */

		row_ptr = (char *) array_ptr;

		MX_DEBUG( 8,("%s: row_ptr = %p, row_ptr_step_size = %ld",
			fname, row_ptr, row_ptr_step_size));

		for ( i = 0; i < num_dimension_elements; i++ ) {

			MX_DEBUG( 4,("ROUTE 3, i = %ld, n = %ld", i, n));

			if ( array_indices != NULL ) {
				array_indices[n] = i;

				MX_DEBUG( 4,
					("ROUTE 3, array_indices[%ld] = %ld",
					n, array_indices[n]));
			}

			/* Invoke the handler function for this level. */

			status = (*handler_fn)( record,
						field,
						handler_data_ptr,
						array_indices,
						array_ptr,
						dimension_level );

			if ( status.code != MXE_SUCCESS )
				return status;

			/* Get the array pointer for the next layer down. */

			if ( dynamically_allocated ) {
				new_array_ptr =
				    mx_read_void_pointer_from_memory_location(
					row_ptr );

				MX_DEBUG( 4,
	("%s: DEREFERENCING row %ld row_ptr %p to get new_array_ptr %p",
					fname, i, row_ptr, new_array_ptr));
			} else {
				new_array_ptr = row_ptr;

				MX_DEBUG( 4,
	("%s: NO DEREFERENCE of row %ld row_ptr %p",
					fname, i, row_ptr ));
			}

			/* Go to the next layer down. */

			status = mx_traverse_field_array( record,
							field,
							handler_fn,
							handler_data_ptr,
							array_indices,
							new_array_ptr,
							new_dimension_level );

			if ( status.code != MXE_SUCCESS )
				return status;
	
			row_ptr += row_ptr_step_size;
		}
	}
	if ( array_indices != NULL ) {
		array_indices[n] = -1;
	}
	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_create_field_from_description( MX_RECORD *record,
		MX_RECORD_FIELD *record_field,
		MX_RECORD_FIELD_PARSE_STATUS *parse_status,
		char *field_description )
{
	static const char fname[] = "mx_create_field_from_description()";

	MX_RECORD_FIELD_PARSE_STATUS temp_parse_status;
	MX_RECORD_FIELD_PARSE_STATUS *parse_status_ptr;
	void *pointer_to_value;
	mx_status_type mx_status;
	mx_status_type ( *token_parser ) ( void *, char *,
				MX_RECORD *, MX_RECORD_FIELD *,
				MX_RECORD_FIELD_PARSE_STATUS * );

	static char separators[] = MX_RECORD_FIELD_SEPARATORS;

	if ( record_field == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD_FIELD pointer passed was NULL." );
	}
	if ( field_description == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"field_description pointer passed was NULL." );
	}

	if ( record_field->flags & MXFF_VARARGS ) {
		pointer_to_value
			= mx_read_void_pointer_from_memory_location(
				record_field->data_pointer );
	} else {
		pointer_to_value = record_field->data_pointer;
	}

	mx_status = mx_get_token_parser( record_field->datatype,
						&token_parser );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( parse_status != NULL ) {
		parse_status_ptr = parse_status;
	} else {
		mx_initialize_parse_status( &temp_parse_status,
					field_description, separators );

		parse_status_ptr = &temp_parse_status;
	}

	/* If the field is a string field, get the maximum length of
	 * a string token for use by the token parser.
	 */

	if ( record_field->datatype == MXFT_STRING ) {
		parse_status_ptr->max_string_token_length =
				mx_get_max_string_token_length( record_field );
	} else {
		parse_status_ptr->max_string_token_length = 0L;
	}

	/* Parse the field description. */

	if ( (record_field->num_dimensions == 0)
	  || ((record_field->datatype == MXFT_STRING)
	    && (record_field->num_dimensions == 1)) ) {

		mx_status = mx_get_next_record_token( parse_status_ptr,
					field_description,
					1 + strlen( field_description ) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = (*token_parser)( pointer_to_value,
				field_description, NULL, record_field,
				parse_status_ptr );
	} else {
		mx_status = mx_parse_array_description( pointer_to_value,
				record_field->num_dimensions - 1, NULL,
				record_field, parse_status_ptr, token_parser );
	}
	return mx_status;
}

MX_EXPORT mx_status_type
mx_create_description_from_field( MX_RECORD *record,
		MX_RECORD_FIELD *record_field,
		char *field_description_buffer,
		size_t field_description_buffer_length )
{
	static const char fname[] = "mx_create_description_from_field()";

	void *pointer_to_value;
	mx_status_type mx_status;
	mx_status_type ( *token_constructor )
		(void *, char *, size_t, MX_RECORD *, MX_RECORD_FIELD *);

	MX_DEBUG( 2,("%s invoked.", fname));

	MX_DEBUG( 2,("%s: record_field = %p, buffer = %p",
		fname, record_field, field_description_buffer));

	if ( record_field == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD_FIELD pointer passed was NULL." );
	}

	MX_DEBUG( 2,("%s: field '%s': ", fname, record_field->name ));

	if ( record_field->flags & MXFF_VARARGS ) {
		pointer_to_value
			= mx_read_void_pointer_from_memory_location(
				record_field->data_pointer );
		MX_DEBUG( 2,
		("%s: DEREFERENCING data_pointer %p to get pointer_to_value %p",
			fname, record_field->data_pointer, pointer_to_value));
	} else {
		pointer_to_value = record_field->data_pointer;

		MX_DEBUG( 2,
		("%s: NO DEREFERENCE of data pointer %p",
			fname, record_field->data_pointer));
	}

	mx_status = mx_get_token_constructor(
			record_field->datatype, &token_constructor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: token_constructor = %p", fname, token_constructor));

	{
	  long i, j, k;
	  char **twod_string;
	  char ***threed_string;
	  double *oned_double;
	  double **twod_double;
	  double ***threed_double;

	  MX_DEBUG( 2,("%s: FOOEY...", fname));
	  MX_DEBUG( 2,("%s: record_field->name = %p",
			fname, record_field->name));
	  MX_DEBUG( 2,("%s: record_field->name = '%s'",
			fname, record_field->name));

	  if ( record_field->datatype == MXFT_STRING ) {
		MX_DEBUG( 2,("%s: MXFT_STRING field, num_dimensions = %ld",
			fname, record_field->num_dimensions));

		if ( record_field->num_dimensions == 1 ) {
		    MX_DEBUG( 2,("%s: 1-d string = %p",
				fname, (char *) pointer_to_value ));
		    MX_DEBUG( 2,("%s: 1-d string = '%s'",
				fname, (char *) pointer_to_value ));
		} else
		if ( record_field->num_dimensions == 2 ) {
		    twod_string = ( char ** ) pointer_to_value;

		    MX_DEBUG( 2,("%s: twod_string = %p", fname, twod_string));

		    for ( i = 0; i < record_field->dimension[0]; i++ ) {
			MX_DEBUG( 2,("%s: twod_string[%ld] = %p",
				fname, i, twod_string[i]));
			MX_DEBUG( 2,("%s: twod_string[%ld] = '%s'",
				fname, i, twod_string[i]));
		    }
		} else
		if ( record_field->num_dimensions == 3 ) {
		    threed_string = ( char ***) pointer_to_value;

		    MX_DEBUG( 2,("%s: threed_string = %p",
				fname, threed_string));

		    for ( i = 0; i < record_field->dimension[0]; i++ ) {
			MX_DEBUG( 2,("%s: threed_string[%ld] = %p",
				fname, i, threed_string[i]));

			for ( j = 0; j < record_field->dimension[1]; j++ ) {
			    MX_DEBUG( 2,("%s: threed_string[%ld][%ld] = %p",
				fname, i, j, threed_string[i][j]));

			    MX_DEBUG( 2,("%s: threed_string[%ld][%ld] = '%s'",
				fname, i, j, threed_string[i][j]));
			}
		    }
		} else {
		    MX_DEBUG( 2,("%s: not a 1-d or 2-d string array.", fname));
		}
	  } else if ( record_field->datatype == MXFT_DOUBLE ) {
		MX_DEBUG( 2,("%s: MXFT_DOUBLE field, num_dimensions = %ld",
			fname, record_field->num_dimensions));

		for ( i = 0; i < record_field->num_dimensions; i++ ) {
			MX_DEBUG( 2,("%s: dimension[%ld] = %ld",
				fname, i, record_field->dimension[i]));
		}

		if ( record_field->num_dimensions == 1 ) {
		    oned_double = (double *) pointer_to_value;

		    MX_DEBUG( 2,("%s: oned_double = %p", fname, oned_double));

		    for ( i = 0; i < record_field->dimension[0]; i++ ) {
			MX_DEBUG( 2,("%s: &oned_double[%ld] = %p",
				fname, i, &(oned_double[i]) ));

			MX_DEBUG( 2,("%s: oned_double[%ld] = %g",
				fname, i, oned_double[i]));
		    }
		} else if ( record_field->num_dimensions == 2 ) {
		    twod_double = (double **) pointer_to_value;

		    MX_DEBUG( 2,("%s: twod_double = %p", fname, twod_double));

		    for ( i = 0; i < record_field->dimension[0]; i++ ) {
			MX_DEBUG( 2,("%s: twod_double[%ld] = %p",
			    fname, i, twod_double[i] ));

			for ( j = 0; j < record_field->dimension[1]; j++ ) {
				MX_DEBUG( 2,("%s: &twod_double[%ld][%ld] = %p",
				fname, i, j, &(twod_double[i][j]) ));

				MX_DEBUG( 2,("%s: twod_double[%ld][%ld] = %g",
				fname, i, j, twod_double[i][j] ));
			}
		    }
		} else
		if ( record_field->num_dimensions == 3 ) {
		    threed_double = ( double *** ) pointer_to_value;

		    MX_DEBUG( 2,("%s: threed_double = %p",
				fname, threed_double));

		    for ( i = 0; i < record_field->dimension[0]; i++ ) {
			MX_DEBUG( 2,("%s: threed_double[%ld] = %p",
				fname, i, threed_double[i]));

			for ( j = 0; j < record_field->dimension[1]; j++ ) {
			    MX_DEBUG( 2,("%s: threed_double[%ld][%ld] = %p",
				fname, i, j, threed_double[i][j]));

			    for ( k = 0; k < record_field->dimension[2]; k++ ) {
				MX_DEBUG( 2,
				("%s: &threed_double[%ld][%ld][%ld] = %p",
				    fname, i, j, k, &(threed_double[i][j][k])));

				MX_DEBUG( 2,
				("%s: threed_double[%ld][%ld][%ld] = %g",
				    fname, i, j, k, threed_double[i][j][k] ));
			    }
			}
		    }
		} else {
		    MX_DEBUG( 2,("%s: num_dimensions = %ld",
			fname, record_field->num_dimensions));
		}
	  } else {
		MX_DEBUG( 2,("%s: datatype = %ld",
			fname, record_field->datatype));
		MX_DEBUG( 2,("%s: datatype = %s", fname,
			mx_get_field_type_string( record_field->datatype ) ));
	  }
	}

	/* Construct the string description of this field. */

	field_description_buffer[0] = '\0';

	if ( (record_field->num_dimensions == 0)
	  || ((record_field->datatype == MXFT_STRING)
	     && (record_field->num_dimensions == 1)) ) {

		mx_status = (*token_constructor)( pointer_to_value,
				field_description_buffer,
				field_description_buffer_length,
				record, record_field );
	} else {
		mx_status = mx_create_array_description( pointer_to_value,
				record_field->num_dimensions - 1,
				field_description_buffer,
				field_description_buffer_length,
				record, record_field, token_constructor );
	}
	return mx_status;
}

