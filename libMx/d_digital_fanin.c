/*
 * Name:    d_digital_fanin.c
 *
 * Purpose: MX digital input driver for reading values from other MX
 *          record fields and performing a logical operation on the
 *          values read (and, or, xor).
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2015, 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_DIGITAL_FANIN_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_digital_input.h"
#include "d_digital_fanin.h"

MX_RECORD_FUNCTION_LIST mxd_digital_fanin_record_function_list = {
	mxd_digital_fanin_initialize_driver,
	mxd_digital_fanin_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_digital_fanin_open
};

MX_DIGITAL_INPUT_FUNCTION_LIST
	mxd_digital_fanin_digital_input_function_list =
{
	mxd_digital_fanin_read
};

MX_RECORD_FIELD_DEFAULTS mxd_digital_fanin_field_default[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_DIGITAL_FANIN_STANDARD_FIELDS
};

long mxd_digital_fanin_num_record_fields
		= sizeof( mxd_digital_fanin_field_default )
		/ sizeof( mxd_digital_fanin_field_default[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_digital_fanin_rfield_def_ptr
			= &mxd_digital_fanin_field_default[0];

/* ===== */

static mx_status_type
mxd_digital_fanin_get_pointers( MX_DIGITAL_INPUT *dinput,
				MX_DIGITAL_FANIN **digital_fanin,
				const char *calling_fname )
{
	static const char fname[] = "mxd_digital_fanin_get_pointers()";

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_INPUT pointer passed was NULL." );
	}
	if ( digital_fanin == (MX_DIGITAL_FANIN **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_FANIN pointer passed was NULL." );
	}

	*digital_fanin = (MX_DIGITAL_FANIN *)
				dinput->record->record_type_struct;

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_digital_fanin_initialize_driver( MX_DRIVER *driver )
{
	static const char fname[] = "mxd_digital_fanin_initialize_driver()";

	MX_RECORD_FIELD_DEFAULTS *field_defaults;
	long referenced_field_index;
	long num_fields_varargs_cookie;
	mx_status_type mx_status;

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DRIVER pointer passed was NULL." );
	}

	mx_status = mx_find_record_field_defaults_index( driver,
						"num_fields",
						&referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie(
			referenced_field_index, 0, &num_fields_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field_defaults( driver,
						"field_name_array",
						&field_defaults );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field_defaults->dimension[0] = num_fields_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_digital_fanin_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_digital_fanin_create_record_structures()";

        MX_DIGITAL_INPUT *dinput;
        MX_DIGITAL_FANIN *digital_fanin;

        /* Allocate memory for the necessary structures. */

        dinput = (MX_DIGITAL_INPUT *) malloc( sizeof(MX_DIGITAL_INPUT) );

        if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for an MX_DIGITAL_INPUT structure." );
        }

        digital_fanin = (MX_DIGITAL_FANIN *) malloc( sizeof(MX_DIGITAL_FANIN) );

        if ( digital_fanin == (MX_DIGITAL_FANIN *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
               "Cannot allocate memory for an MX_DIGITAL_FANIN structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = dinput;
        record->record_type_struct = digital_fanin;
        record->class_specific_function_list
			= &mxd_digital_fanin_digital_input_function_list;

        dinput->record = record;
	digital_fanin->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_digital_fanin_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_digital_fanin_open()";

	MX_DIGITAL_INPUT *dinput = NULL;
	MX_DIGITAL_FANIN *digital_fanin = NULL;
	char *op_name = NULL;
	unsigned long i, num_fields;
	char record_field_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	MX_RECORD *child_record;
	MX_RECORD_FIELD *child_field;
	char *record_name_ptr;
	char *field_name_ptr;
	char default_field_name[] = "value";
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	dinput = (MX_DIGITAL_INPUT *) record->record_class_struct;

	mx_status = mxd_digital_fanin_get_pointers( dinput,
						&digital_fanin, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Figure out what type of logical operation has been requested. */

	op_name = digital_fanin->operation_name;

	if ( mx_strcasecmp( op_name, "and" ) == 0 ) {
		digital_fanin->operation_type = MXT_DFIN_AND;
	} else
	if ( mx_strcasecmp( op_name, "or" ) == 0 ) {
		digital_fanin->operation_type = MXT_DFIN_OR;
	} else
	if ( mx_strcasecmp( op_name, "xor" ) == 0 ) {
		digital_fanin->operation_type = MXT_DFIN_XOR;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized logical operation '%s' specified "
		"for record '%s'.  The allowed types are "
		"'and', 'or', and 'xor'",
			op_name, record->name );
	}

	/* Allocate the field array. */

	num_fields = digital_fanin->num_fields;

	digital_fanin->field_array =
		(MX_RECORD_FIELD **) malloc( num_fields * sizeof(MX_RECORD *) );

	if ( digital_fanin->field_array == (MX_RECORD_FIELD **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"An attempt to allocate a %ld element array of "
		"MX_RECORD_FIELD pointers failed for record '%s'.",
			num_fields, record->name );
	}

	digital_fanin->field_value_array =
		(void **) malloc( num_fields * sizeof(void *) );

	if ( digital_fanin->field_value_array == (void **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"An attempt to allocate a %ld element array of "
		"field value pointers failed for record '%s'.",
			num_fields, record->name );
	}

	/* Fill in the elements of the field array. */

	for ( i = 0; i < num_fields; i++ ) {
		strlcpy( record_field_name,
			digital_fanin->field_name_array[i],
			sizeof(record_field_name) );

		record_name_ptr = record_field_name;
		
		field_name_ptr = strrchr( record_field_name, '.' );

		if ( field_name_ptr == NULL ) {
			field_name_ptr = default_field_name;
		} else {
			*field_name_ptr = '\0';

			field_name_ptr++;
		}

		child_record = mx_get_record( record, record_name_ptr );

		if ( child_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_NULL_ARGUMENT, fname,
			"No record named '%s' was found in the MX database.",
				record_name_ptr );
		}

		mx_status = mx_find_record_field( child_record,
						field_name_ptr,
						&child_field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( child_field->num_dimensions == 0 ) {
			/* This case is OK. */
		} else
		if ( child_field->num_dimensions == 1 ) {
			switch( child_field->datatype ) {
			case MXFT_RECORD:
			case MXFT_RECORDTYPE:
			case MXFT_INTERFACE:
			case MXFT_RECORD_FIELD:
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The field type %ld used by record field "
				"'%s.%s' is not supported by the driver "
				"for record '%s'.",
					child_field->datatype,
					child_field->record->name,
					child_field->name,
					record->name );
				break;
			default:
				break;
			}
		} else {
			return mx_error( MXE_UNSUPPORTED, fname,
			"The driver for record '%s' only supports "
			"record fields of 0 or 1 dimensions.  "
			"Record field '%s.%s' has %ld dimensions.",
				record->name,
				child_field->record->name,
				child_field->name,
				child_field->num_dimensions );
		}

		digital_fanin->field_array[i] = child_field;

		digital_fanin->field_value_array[i] =
			mx_get_field_value_pointer( child_field );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_digital_fanin_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_digital_fanin_read()";

	MX_DIGITAL_FANIN *digital_fanin = NULL;
	unsigned long i, num_fields;
	MX_RECORD_FIELD *field;
	void *value_ptr;
	unsigned long result, field_value;
	mx_status_type mx_status;

	mx_status = mxd_digital_fanin_get_pointers( dinput,
						&digital_fanin, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	result = 0;

	num_fields = digital_fanin->num_fields;

	for ( i = 0; i < num_fields; i++ ) {
		field = digital_fanin->field_array[i];

#if MXD_DIGITAL_FANIN_DEBUG
		MX_DEBUG(-2,("%s: field[%lu] = '%s.%s'",
			fname, i, field->record->name, field->name));
#endif
		mx_status = mx_process_record_field( field->record, field,
							MX_PROCESS_GET, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		value_ptr = digital_fanin->field_value_array[i];

		switch( field->datatype ) {
		case MXFT_STRING:
			field_value = mx_string_to_unsigned_long( value_ptr );
			break;
		case MXFT_CHAR:
			field_value = labs( *((signed char *) value_ptr) );
			break;
		case MXFT_UCHAR:
			field_value = *((unsigned char *) value_ptr);
			break;
		case MXFT_SHORT:
			field_value = labs( *((short *) value_ptr) );
			break;
		case MXFT_USHORT:
			field_value = *((unsigned short *) value_ptr);
			break;
		case MXFT_BOOL:
			field_value = labs( *((mx_bool_type *) value_ptr) );

			if ( field_value ) {
				field_value = TRUE;
			} else {
				field_value = FALSE;
			}
			break;
		case MXFT_LONG:
			field_value = labs( *((long *) value_ptr) );
			break;
		case MXFT_ULONG:
		case MXFT_HEX:
			field_value = *((unsigned long *) value_ptr);
			break;
		case MXFT_FLOAT:
			field_value = mx_round(fabs( *((float *) value_ptr) ));
			break;
		case MXFT_DOUBLE:
			field_value = mx_round(fabs( *((double *) value_ptr) ));
			break;
		default:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Field datatype %ld is not yet implemented.",
			field->datatype );
			break;
		}

		if ( i == 0 ) {
			result = field_value;
		} else {
			switch( digital_fanin->operation_type ) {
			case MXT_DFIN_AND:
				result &= field_value;
				break;
			case MXT_DFIN_OR:
				result |= field_value;
				break;
			case MXT_DFIN_XOR:
				result ^= field_value;
				break;
			default:
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Illegal operation type %lu requested "
				"for record '%s'.",
					digital_fanin->operation_type,
					dinput->record->name );
				break;
			}
		}
	}

	dinput->value = result;

	return mx_status;
}

