/*
 * Name:    d_scaler_function.c 
 *
 * Purpose: MX scaler driver for a pseudoscaler that is a linear function
 *          of several real scalers and variables.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2002-2006 Illinois Institute of Technology
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
#include "mx_scaler.h"
#include "mx_variable.h"
#include "mx_measurement.h"
#include "d_scaler_function.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_scaler_function_record_function_list = {
	mxd_scaler_function_initialize_type,
	mxd_scaler_function_create_record_structures,
	mxd_scaler_function_finish_record_initialization,
	mxd_scaler_function_delete_record,
	mxd_scaler_function_print_scaler_structure
};

MX_SCALER_FUNCTION_LIST mxd_scaler_function_scaler_function_list = {
	mxd_scaler_function_clear,
	mxd_scaler_function_overflow_set,
	mxd_scaler_function_read,
	NULL,
	mxd_scaler_function_is_busy,
	NULL,
	NULL,
	mxd_scaler_function_get_parameter,
	mxd_scaler_function_set_parameter
};

/* Linear function scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_scaler_function_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_SCALER_FUNCTION_STANDARD_FIELDS
};

long mxd_scaler_function_num_record_fields
		= sizeof(mxd_scaler_function_record_field_defaults)
			/ sizeof(mxd_scaler_function_record_field_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_scaler_function_rfield_def_ptr
			= &mxd_scaler_function_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_scaler_function_get_pointers( MX_SCALER *scaler,
			MX_SCALER_FUNCTION **scaler_function,
			const char *calling_fname )
{
	static const char fname[] = "mxd_scaler_function_get_pointers()";

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SCALER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( scaler_function == (MX_SCALER_FUNCTION **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCALER_FUNCTION pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*scaler_function = (MX_SCALER_FUNCTION *)
				(scaler->record->record_type_struct);

	if ( *scaler_function == (MX_SCALER_FUNCTION *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SCALER_FUNCTION pointer for record '%s' is NULL.",
			scaler->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

#define NUM_SCALER_FUNCTION_FIELDS	3

MX_EXPORT mx_status_type
mxd_scaler_function_initialize_type( long type )
{
        static const char fname[] = "mxs_scaler_function_initialize_type()";

        const char field_name[NUM_SCALER_FUNCTION_FIELDS]
					[MXU_FIELD_NAME_LENGTH+1]
            = {
                "record_array",
		"real_scale",
		"real_offset" };

        MX_DRIVER *driver;
        MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
        MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
        MX_RECORD_FIELD_DEFAULTS *field;
        int i;
        long num_record_fields;
	long referenced_field_index;
        long num_records_varargs_cookie;
        mx_status_type mx_status;

        driver = mx_get_driver_by_type( type );

        if ( driver == (MX_DRIVER *) NULL ) {
                return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
                        "Record type %ld not found.", type );
        }

        record_field_defaults_ptr = driver->record_field_defaults_ptr;

        if (record_field_defaults_ptr == (MX_RECORD_FIELD_DEFAULTS **) NULL) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                "'record_field_defaults_ptr' for record type '%s' is NULL.",
                        driver->name );
        }

        record_field_defaults = *record_field_defaults_ptr;

        if ( record_field_defaults == (MX_RECORD_FIELD_DEFAULTS *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                "'record_field_defaults_ptr' for record type '%s' is NULL.",
                        driver->name );
        }

        if ( driver->num_record_fields == (long *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                "'num_record_fields' pointer for record type '%s' is NULL.",
                        driver->name );
        }

	num_record_fields = *(driver->num_record_fields);

        mx_status = mx_find_record_field_defaults_index(
                        record_field_defaults, num_record_fields,
                        "num_records", &referenced_field_index );

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

        mx_status = mx_construct_varargs_cookie(
                        referenced_field_index, 0, &num_records_varargs_cookie);

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

        MX_DEBUG( 2,("%s: num_records varargs cookie = %ld",
                        fname, num_records_varargs_cookie));

	for ( i = 0; i < NUM_SCALER_FUNCTION_FIELDS; i++ ) {
		mx_status = mx_find_record_field_defaults(
			record_field_defaults, num_record_fields,
			field_name[i], &field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		field->dimension[0] = num_records_varargs_cookie;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scaler_function_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_scaler_function_create_record_structures()";

	MX_SCALER *scaler;
	MX_SCALER_FUNCTION *scaler_function;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER structure." );
	}

	scaler_function = (MX_SCALER_FUNCTION *)
				malloc( sizeof(MX_SCALER_FUNCTION) );

	if ( scaler_function == (MX_SCALER_FUNCTION *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Can't allocate memory for MX_SCALER_FUNCTION structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = scaler_function;
	record->class_specific_function_list
				= &mxd_scaler_function_scaler_function_list;

	scaler->record = record;
	scaler_function->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scaler_function_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_scaler_function_finish_record_initialization()";

	MX_RECORD **record_array;
	MX_RECORD *child_record;
	MX_RECORD_FIELD *record_field;
	MX_SCALER *scaler;
	MX_SCALER_FUNCTION *scaler_function;
	long i, num_records, num_variables, num_scalers;
	long ivar, iscaler;
	mx_status_type mx_status;

	char error_message[] =
	"Only scalers and double precision variables may be used in a scaler "
	"function's record list.  Record '%s' is not of either type.";

	scaler = (MX_SCALER *) (record->record_class_struct);

	mx_status = mxd_scaler_function_get_pointers( scaler,
					&scaler_function, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_scaler_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_records = scaler_function->num_records;
	record_array = scaler_function->record_array;

	/* Setup the master scaler record, if requested. */

	if ( strlen( scaler_function->master_scaler_name ) == 0 ) {
		scaler_function->master_scaler_record = NULL;
	} else {
		scaler_function->master_scaler_record = mx_get_record( record,
				scaler_function->master_scaler_name );

		if ( scaler_function->master_scaler_record == NULL ) {
			return mx_error( MXE_NOT_FOUND, fname,
		"Master scaler record '%s' for scaler '%s' does not exist.",
				scaler_function->master_scaler_name,
				record->name );
		}
	}

	/* Verify that only scalers and 0 or 1 dimensional double precision
	 * variable arrays are in the record_array above.  Along the way,
	 * find out how many records of each type exist.
	 */

	num_variables = 0L;
	num_scalers = 0L;

	for ( i = 0; i < num_records; i++ ) {
		child_record = record_array[i];

		switch ( child_record->mx_superclass ) {
		case MXR_VARIABLE:
			/* Verify that this is a 0 or 1 dimensional
			 * double precision array.
			 */

			mx_status = mx_find_record_field( child_record, "value",
							&record_field );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( record_field->datatype != MXFT_DOUBLE ) {
				return mx_error( MXE_TYPE_MISMATCH, fname,
		"The variable record '%s' is not a double precision variable.",
					child_record->name );
			}
			if ( (record_field->num_dimensions != 0)
			  && (record_field->num_dimensions != 1) )
			{
				return mx_error( MXE_TYPE_MISMATCH, fname,
			"Double precision variables used by this record type "
			"must be either 0 or 1 dimensions.  The record '%s' is "
			"a %ld dimensional variable.",
					child_record->name,
					record_field->num_dimensions );
			}

			num_variables++;

			break;

		case MXR_DEVICE:
			switch( child_record->mx_class ) {
			case MXC_SCALER:
				num_scalers++;

				break;

			default:
				return mx_error( MXE_TYPE_MISMATCH, fname,
					error_message, child_record->name );
				break;
			}
			break;
		default:
			return mx_error( MXE_TYPE_MISMATCH, fname,
				error_message, child_record->name );
			break;
		}
	}

	MX_DEBUG( 2,
	("%s: num_records = %ld, num_scalers = %ld, num_variables = %ld",
		fname, num_records, num_scalers, num_variables));

	/* Create the arrays for scaler parameters. */

	if ( num_scalers <= 0 ) {
		scaler_function->num_scalers = 0L;
		scaler_function->scaler_record_array = NULL;
		scaler_function->real_scaler_scale = NULL;
		scaler_function->real_scaler_offset = NULL;
	} else {
		scaler_function->num_scalers = num_scalers;
	
		scaler_function->scaler_record_array = (MX_RECORD **)
				malloc( num_scalers * sizeof(MX_RECORD *) );
	
		if ( scaler_function->scaler_record_array == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Cannot allocate scaler_record_array for %ld scalers.",
			num_scalers );
		}
	
		scaler_function->real_scaler_scale = (double *)
				malloc( num_scalers * sizeof(double) );
	
		if ( scaler_function->real_scaler_scale == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Cannot allocate real_scaler_scale for %ld scalers.",
			num_scalers );
		}
	
		scaler_function->real_scaler_offset = (double *)
				malloc( num_scalers * sizeof(double) );
	
		if ( scaler_function->real_scaler_offset == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Cannot allocate real_scaler_offset for %ld scalers.",
			num_scalers );
		}
	}

	/* Create the arrays for variable parameters. */

	if ( num_variables <= 0 ) {
		scaler_function->num_variables = 0L;
		scaler_function->variable_record_array = NULL;
		scaler_function->real_variable_scale = NULL;
		scaler_function->real_variable_offset = NULL;
	} else {
		scaler_function->num_variables = num_variables;
	
		scaler_function->variable_record_array = (MX_RECORD **)
				malloc( num_variables * sizeof(MX_RECORD *) );
	
		if ( scaler_function->variable_record_array == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate variable_record_array for %ld variables.",
			num_variables );
		}
	
		scaler_function->real_variable_scale = (double *)
				malloc( num_variables * sizeof(double) );
	
		if ( scaler_function->real_variable_scale == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate real_variable_scale for %ld variables.",
			num_variables );
		}
	
		scaler_function->real_variable_offset = (double *)
				malloc( num_variables * sizeof(double) );
	
		if ( scaler_function->real_variable_offset == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate real_variable_offset for %ld variables.",
			num_variables );
		}
	}

	/* Copy the values from the record arrays to the scaler and variable
	 * arrays.
	 */

	ivar = iscaler = 0L;

	for ( i = 0; i < num_records; i++ ) {
		child_record = record_array[i];

		if ( child_record->mx_superclass == MXR_DEVICE ) {
			if ( iscaler >= num_scalers ) {
				return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
					fname,
"iscaler value of %ld on second pass exceeds maximum value %ld from first pass.",
				iscaler, num_scalers+1 );
			}

			scaler_function->scaler_record_array[iscaler]
				= scaler_function->record_array[i];

			scaler_function->real_scaler_scale[iscaler]
				= scaler_function->real_scale[i];

			scaler_function->real_scaler_offset[iscaler]
				= scaler_function->real_offset[i];

			iscaler++;
		} else {
			if ( ivar >= num_variables ) {
				return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
					fname,
"ivar value of %ld on second pass exceeds maximum value %ld from first pass.",
				ivar, num_variables+1 );
			}

			scaler_function->variable_record_array[ivar]
				= scaler_function->record_array[i];

			scaler_function->real_variable_scale[ivar]
				= scaler_function->real_scale[i];

			scaler_function->real_variable_offset[ivar]
				= scaler_function->real_offset[i];

			ivar++;
		}
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scaler_function_delete_record( MX_RECORD *record )
{
	MX_SCALER_FUNCTION *scaler_function;

	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}
	if ( record->record_type_struct != NULL ) {
		scaler_function = (MX_SCALER_FUNCTION *)
						record->record_type_struct;

		if ( scaler_function->scaler_record_array != NULL )
			free( scaler_function->scaler_record_array );

		if ( scaler_function->real_scaler_scale != NULL )
			free( scaler_function->real_scaler_scale );

		if ( scaler_function->real_scaler_offset != NULL )
			free( scaler_function->real_scaler_offset );

		if ( scaler_function->variable_record_array != NULL )
			free( scaler_function->variable_record_array );

		if ( scaler_function->real_variable_scale != NULL )
			free( scaler_function->real_variable_scale );

		if ( scaler_function->real_variable_offset != NULL )
			free( scaler_function->real_variable_offset );

		free( scaler_function );

		record->record_type_struct = NULL;
	}
	if ( record->record_class_struct != NULL ) {
		free( record->record_class_struct );

		record->record_class_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scaler_function_print_scaler_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_scaler_function_print_scaler_structure()";

	MX_SCALER *scaler;
	MX_SCALER_FUNCTION *scaler_function;
	MX_RECORD **record_array;
	double *real_scale, *real_offset;
	long i, num_records, current_value;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	scaler = (MX_SCALER *) (record->record_class_struct);

	mx_status = mxd_scaler_function_get_pointers( scaler,
					&scaler_function, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_records = scaler_function->num_records;
	record_array = scaler_function->record_array;
	real_scale = scaler_function->real_scale;
	real_offset = scaler_function->real_offset;

	fprintf(file, "SCALER parameters for scaler '%s':\n", record->name);
	fprintf(file, "  Scaler type           = SCALER_FUNCTION.\n\n");

	mx_status = mx_scaler_read( record, &current_value );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of scaler '%s'",
			record->name );
	}

	fprintf(file, "  present value         = %ld.\n", current_value );
	fprintf(file, "  dark current          = %g counts per second.\n",
						scaler->dark_current);
	fprintf(file, "  scaler flags          = %#lx.\n",scaler->scaler_flags);

	fprintf(file, "  num records           = %ld\n", num_records);

	fprintf(file, "  record array          = (");

	if ( record_array == NULL ) {
		fprintf(file, "NULL");
	} else {
		for ( i = 0; i < num_records; i++ ) {
			fprintf(file, "%s", record_array[i]->name);

			if ( i != num_records-1 ) {
				fprintf(file, ", ");
			}
		}
	}
	fprintf(file, ")\n");

	fprintf(file, "  real scale           = (");

	if ( real_scale == NULL ) {
		fprintf(file, "NULL");
	} else {
		for ( i = 0; i < num_records; i++ ) {
			fprintf(file, "%g", real_scale[i]);

			if ( i != num_records-1 ) {
				fprintf(file, ", ");
			}
		}
	}
	fprintf(file, ")\n");

	fprintf(file, "  real offset          = (");

	if ( real_offset == NULL ) {
		fprintf(file, "NULL");
	} else {
		for ( i = 0; i < num_records; i++ ) {
			fprintf(file, "%g", real_offset[i]);

			if ( i != num_records-1 ) {
				fprintf(file, ", ");
			}
		}
	}
	fprintf(file, ")\n");

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scaler_function_clear( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_scaler_function_clear()";

	MX_SCALER_FUNCTION *scaler_function;
	MX_RECORD **scaler_record_array;
	MX_RECORD *child_scaler_record;
	long i, num_scalers;
	mx_status_type mx_status;

	mx_status = mxd_scaler_function_get_pointers( scaler,
					&scaler_function, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scaler_function->master_scaler_record != NULL ) {
		mx_status = mx_scaler_clear(
				scaler_function->master_scaler_record );
	} else {
		num_scalers = scaler_function->num_scalers;
		scaler_record_array = scaler_function->scaler_record_array;

		/* Only scalers can be cleared. */

		for ( i = 0; i < num_scalers; i++ ) {
			child_scaler_record = scaler_record_array[i];

			mx_status = mx_scaler_clear( child_scaler_record );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scaler_function_overflow_set( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_scaler_function_overflow_set()";

	MX_SCALER_FUNCTION *scaler_function;
	MX_RECORD **scaler_record_array;
	MX_RECORD *child_scaler_record;
	long i, num_scalers;
	mx_bool_type overflow_set;
	mx_status_type mx_status;

	mx_status = mxd_scaler_function_get_pointers( scaler,
					&scaler_function, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_scalers = scaler_function->num_scalers;
	scaler_record_array = scaler_function->scaler_record_array;

	/* Only scalers can have an overflow set. */

	scaler->overflow_set = FALSE;

	for ( i = 0; i < num_scalers; i++ ) {
	
		child_scaler_record = scaler_record_array[i];

		mx_status = mx_scaler_overflow_set( child_scaler_record,
							&overflow_set );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* If the child scaler has overflow set, we are done. */

		if ( overflow_set == TRUE ) {
			scaler->overflow_set = TRUE;

			return MX_SUCCESSFUL_RESULT;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scaler_function_read( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_scaler_function_read()";

	MX_SCALER_FUNCTION *scaler_function;
	MX_RECORD *child_record;
	long i, long_value;
	double sum, scaled_value, double_value;

	long num_scalers;
	MX_RECORD **scaler_record_array;
	double *real_scaler_scale;
	double *real_scaler_offset;

	long num_variables;
	MX_RECORD **variable_record_array;
	double *real_variable_scale;
	double *real_variable_offset;

	mx_status_type mx_status;

	mx_status = mxd_scaler_function_get_pointers( scaler,
					&scaler_function, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sum = 0.0;

	/* Add up the scaler values. */

	num_scalers = scaler_function->num_scalers;
	scaler_record_array = scaler_function->scaler_record_array;
	real_scaler_scale = scaler_function->real_scaler_scale;
	real_scaler_offset = scaler_function->real_scaler_offset;

	for ( i = 0; i < num_scalers; i++ ) {
	
		child_record = scaler_record_array[i];

		mx_status = mx_scaler_read( child_record, &long_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		scaled_value = real_scaler_offset[i] + real_scaler_scale[i]
							* (double) long_value;

		sum += scaled_value;

		MX_DEBUG( 2,("%s: scaler = '%s', sum = %g, scaled_value = %g",
			fname, child_record->name, sum, scaled_value));
	}

	/* Add up the variable values. */

	num_variables = scaler_function->num_variables;
	variable_record_array = scaler_function->variable_record_array;
	real_variable_scale = scaler_function->real_variable_scale;
	real_variable_offset = scaler_function->real_variable_offset;

	for ( i = 0; i < num_variables; i++ ) {
	
		child_record = variable_record_array[i];

		mx_status = mx_get_double_variable( child_record,
							&double_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		scaled_value = real_variable_offset[i] + real_variable_scale[i]
							* double_value;

		sum += scaled_value;

		MX_DEBUG( 2,("%s: variable = '%s', sum = %g, scaled_value = %g",
			fname, child_record->name, sum, scaled_value));
	}

	scaler->raw_value = mx_round( sum );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scaler_function_is_busy( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_scaler_function_is_busy()";

	MX_SCALER_FUNCTION *scaler_function;
	MX_RECORD **scaler_record_array;
	MX_RECORD *child_scaler_record;
	long i, num_scalers;
	mx_bool_type busy;
	mx_status_type mx_status;

	mx_status = mxd_scaler_function_get_pointers( scaler,
					&scaler_function, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_scalers = scaler_function->num_scalers;
	scaler_record_array = scaler_function->scaler_record_array;

	/* Only scalers can be busy. */

	scaler->busy = FALSE;

	for ( i = 0; i < num_scalers; i++ ) {
	
		child_scaler_record = scaler_record_array[i];

		mx_status = mx_scaler_is_busy( child_scaler_record, &busy );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* If the child scaler is busy, we are done. */

		if ( busy == TRUE ) {
			scaler->busy = TRUE;

			return MX_SUCCESSFUL_RESULT;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scaler_function_get_parameter( MX_SCALER *scaler )
{
	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		scaler->mode = MXCM_COUNTER_MODE;
		break;
	case MXLV_SCL_DARK_CURRENT:
		break;
	default:
		return mx_scaler_default_get_parameter_handler( scaler );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scaler_function_set_parameter( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_scaler_function_set_parameter()";

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		if ( scaler->mode != MXCM_COUNTER_MODE ) {
			return mx_error( MXE_UNSUPPORTED, fname,
				"Scaler modes other than preset time are not "
				"currently supported for scaler '%s'.",
				scaler->record->name );
		}
		break;
	case MXLV_SCL_DARK_CURRENT:
		break;
	default:
		return mx_scaler_default_set_parameter_handler( scaler );
	}

	return MX_SUCCESSFUL_RESULT;
}

