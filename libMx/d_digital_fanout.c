/*
 * Name:    d_digital_fanout.c
 *
 * Purpose: MX digital output driver for forwarding the digital output value
 *          written to this record to a number of other record fields.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_DIGITAL_FANOUT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_digital_output.h"
#include "d_digital_fanout.h"

MX_RECORD_FUNCTION_LIST mxd_digital_fanout_record_function_list = {
	mxd_digital_fanout_initialize_driver,
	mxd_digital_fanout_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_digital_fanout_open
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
	mxd_digital_fanout_digital_output_function_list =
{
	NULL,
	mxd_digital_fanout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_digital_fanout_field_default[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_DIGITAL_FANOUT_STANDARD_FIELDS
};

long mxd_digital_fanout_num_record_fields
		= sizeof( mxd_digital_fanout_field_default )
		/ sizeof( mxd_digital_fanout_field_default[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_digital_fanout_rfield_def_ptr
			= &mxd_digital_fanout_field_default[0];

/* ===== */

static mx_status_type
mxd_digital_fanout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
				MX_DIGITAL_FANOUT **digital_fanout,
				const char *calling_fname )
{
	static const char fname[] = "mxd_digital_fanout_get_pointers()";

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_OUTPUT pointer passed was NULL." );
	}
	if ( digital_fanout == (MX_DIGITAL_FANOUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_FANOUT pointer passed was NULL." );
	}

	*digital_fanout = (MX_DIGITAL_FANOUT *)
				doutput->record->record_type_struct;

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_digital_fanout_initialize_driver( MX_DRIVER *driver )
{
	static const char fname[] = "mxd_digital_fanout_initialize_driver()";

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
mxd_digital_fanout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_digital_fanout_create_record_structures()";

        MX_DIGITAL_OUTPUT *doutput;
        MX_DIGITAL_FANOUT *digital_fanout;

        /* Allocate memory for the necessary structures. */

        doutput = (MX_DIGITAL_OUTPUT *) malloc( sizeof(MX_DIGITAL_OUTPUT) );

        if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for an MX_DIGITAL_OUTPUT structure." );
        }

        digital_fanout = (MX_DIGITAL_FANOUT *)
				malloc( sizeof(MX_DIGITAL_FANOUT) );

        if ( digital_fanout == (MX_DIGITAL_FANOUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
               "Cannot allocate memory for MX_DIGITAL_FANOUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = doutput;
        record->record_type_struct = digital_fanout;
        record->class_specific_function_list
			= &mxd_digital_fanout_digital_output_function_list;

        doutput->record = record;
	digital_fanout->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_digital_fanout_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_digital_fanout_open()";

	MX_DIGITAL_OUTPUT *doutput = NULL;
	MX_DIGITAL_FANOUT *digital_fanout = NULL;
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

	doutput = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	mx_status = mxd_digital_fanout_get_pointers( doutput,
						&digital_fanout, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_fields = digital_fanout->num_fields;

	/* Allocate the field array. */

	digital_fanout->field_array =
		(MX_RECORD_FIELD **) malloc( num_fields * sizeof(MX_RECORD *) );

	if ( digital_fanout->field_array == (MX_RECORD_FIELD **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"An attempt to allocate a %ld element array of "
		"MX_RECORD_FIELD pointers failed for record '%s'.",
			num_fields, record->name );
	}

	digital_fanout->field_value_array =
		(void **) malloc( num_fields * sizeof(void *) );

	if ( digital_fanout->field_value_array == (void **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"An attempt to allocate a %ld element array of "
		"field value pointers failed for record '%s'.",
			num_fields, record->name );
	}

	/* Fill in the elements of the field array. */

	for ( i = 0; i < num_fields; i++ ) {
		strlcpy( record_field_name,
			digital_fanout->field_name_array[i],
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

		digital_fanout->field_array[i] = child_field;

		digital_fanout->field_value_array[i] =
			mx_get_field_value_pointer( child_field );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_digital_fanout_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_digital_fanout_write()";

	MX_DIGITAL_FANOUT *digital_fanout = NULL;
	unsigned long i, num_fields;
	MX_RECORD_FIELD *field;
	void *value_ptr;
	mx_status_type mx_status;

	mx_status = mxd_digital_fanout_get_pointers( doutput,
						&digital_fanout, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_fields = digital_fanout->num_fields;

	for ( i = 0; i < num_fields; i++ ) {
		field = digital_fanout->field_array[i];

		MX_DEBUG(-2,("%s: field[%lu] = '%s.%s'",
			fname, i, field->record->name, field->name));

		value_ptr = digital_fanout->field_value_array[i];

		switch( field->datatype ) {
		case MXFT_STRING:
		case MXFT_CHAR:
			*((char *) value_ptr) = (char) doutput->value;
			break;
		case MXFT_UCHAR:
			*((unsigned char *) value_ptr) =
						(unsigned char) doutput->value;
			break;
		case MXFT_SHORT:
			*((short *) value_ptr) = (short) doutput->value;
			break;
		case MXFT_USHORT:
			*((unsigned short *) value_ptr) =
						(unsigned short) doutput->value;
			break;
		case MXFT_BOOL:
			*((mx_bool_type *) value_ptr) =
						(mx_bool_type) doutput->value;
			break;
		case MXFT_LONG:
			*((long *) value_ptr) = (long) doutput->value;
			break;
		case MXFT_ULONG:
			*((unsigned long *) value_ptr) =
						(unsigned long) doutput->value;
			break;
		case MXFT_FLOAT:
			*((float *) value_ptr) = (float) doutput->value;
			break;
		case MXFT_DOUBLE:
			*((double *) value_ptr) = (double) doutput->value;
			break;
		default:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Field datatype %ld is not yet implemented.",
			field->datatype );
			break;
		}

		mx_status = mx_process_record_field( field->record, field,
							MX_PROCESS_PUT, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return mx_status;
}

