/*
 * Name:    d_linear_function.c 
 *
 * Purpose: MX motor driver for a pseudomotor that is a linear function
 *          of several real motors.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2004, 2006-2007, 2010, 2013-2015
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_variable.h"
#include "d_linear_function.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_linear_function_record_function_list = {
	mxd_linear_function_initialize_driver,
	mxd_linear_function_create_record_structures,
	mxd_linear_function_finish_record_initialization,
	mxd_linear_function_delete_record,
	mxd_linear_function_print_motor_structure
};

MX_MOTOR_FUNCTION_LIST mxd_linear_function_motor_function_list = {
	mxd_linear_function_motor_is_busy,
	mxd_linear_function_move_absolute,
	mxd_linear_function_get_position,
	NULL,
	mxd_linear_function_soft_abort,
	mxd_linear_function_immediate_abort,
	mxd_linear_function_positive_limit_hit,
	mxd_linear_function_negative_limit_hit,
	mxd_linear_function_raw_home_command,
	NULL,
	mxd_linear_function_get_parameter,
	mxd_linear_function_set_parameter,
};

/* Linear function motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_linear_function_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_LINEAR_FUNCTION_STANDARD_FIELDS
};

long mxd_linear_function_num_record_fields
		= sizeof(mxd_linear_function_record_field_defaults)
			/ sizeof(mxd_linear_function_record_field_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_linear_function_rfield_def_ptr
			= &mxd_linear_function_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_linear_function_get_pointers( MX_MOTOR *motor,
			MX_LINEAR_FUNCTION_MOTOR **linear_function_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_linear_function_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( linear_function_motor == (MX_LINEAR_FUNCTION_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_LINEAR_FUNCTION_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*linear_function_motor = (MX_LINEAR_FUNCTION_MOTOR *)
				(motor->record->record_type_struct);

	if ( *linear_function_motor == (MX_LINEAR_FUNCTION_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LINEAR_FUNCTION_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

#define NUM_LINEAR_FUNCTION_FIELDS	4

MX_EXPORT mx_status_type
mxd_linear_function_initialize_driver( MX_DRIVER *driver )
{
        static const char fname[] = "mxs_linear_function_initialize_driver()";

        const char field_name[NUM_LINEAR_FUNCTION_FIELDS]
					[MXU_FIELD_NAME_LENGTH+1]
            = {
                "record_array",
		"real_scale",
		"real_offset",
		"move_fraction" };

        MX_RECORD_FIELD_DEFAULTS *field;
        int i;
	long referenced_field_index;
        long num_records_varargs_cookie;
        mx_status_type mx_status;

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DRIVER pointer passed was NULL." );
	}

	mx_status = mx_find_record_field_defaults_index( driver,
                        			"num_records",
						&referenced_field_index );

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

	mx_status = mx_construct_varargs_cookie(
                        referenced_field_index, 0, &num_records_varargs_cookie);

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

        MX_DEBUG( 2,("%s: num_records varargs cookie = %ld",
                        fname, num_records_varargs_cookie));

	for ( i = 0; i < NUM_LINEAR_FUNCTION_FIELDS; i++ ) {
		mx_status = mx_find_record_field_defaults( driver,
						field_name[i], &field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		field->dimension[0] = num_records_varargs_cookie;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linear_function_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_linear_function_create_record_structures()";

	MX_MOTOR *motor;
	MX_LINEAR_FUNCTION_MOTOR *linear_function;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	linear_function = (MX_LINEAR_FUNCTION_MOTOR *)
				malloc( sizeof(MX_LINEAR_FUNCTION_MOTOR) );

	if ( linear_function == (MX_LINEAR_FUNCTION_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Can't allocate memory for MX_LINEAR_FUNCTION_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = linear_function;
	record->class_specific_function_list
				= &mxd_linear_function_motor_function_list;

	motor->record = record;
	linear_function->record = record;

	/* A linear function motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linear_function_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_linear_function_finish_record_initialization()";

	MX_RECORD **record_array;
	MX_RECORD *child_record;
	MX_RECORD_FIELD *record_field;
	MX_MOTOR *motor;
	MX_LINEAR_FUNCTION_MOTOR *linear_function_motor;
	long i, num_records, num_variables, num_motors;
	long ivar, imotor;
	mx_status_type mx_status;

	char error_message[] =
	"Only motors and double precision variables may be used in a linear "
	"function's record list.  Record '%s' is not of either type.";

	linear_function_motor = NULL;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_linear_function_get_pointers( motor,
					&linear_function_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->motor_flags |= MXF_MTR_IS_PSEUDOMOTOR;
	motor->motor_flags |= MXF_MTR_CANNOT_QUICK_SCAN;

	num_records = linear_function_motor->num_records;
	record_array = linear_function_motor->record_array;

	/* Verify that only motors and 0 or 1 dimensional double precision
	 * variable arrays are in the record_array above.  Along the way,
	 * find out how many records of each type exist.
	 */

	num_variables = 0L;
	num_motors = 0L;

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
			case MXC_MOTOR:
				num_motors++;

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
	("%s: num_records = %ld, num_motors = %ld, num_variables = %ld",
		fname, num_records, num_motors, num_variables));

	/* Create the arrays for motor parameters. */

	if ( num_motors <= 0 ) {
		linear_function_motor->num_motors = 0L;
		linear_function_motor->motor_record_array = NULL;
		linear_function_motor->motor_position_array = NULL;
		linear_function_motor->real_motor_scale = NULL;
		linear_function_motor->real_motor_offset = NULL;
		linear_function_motor->motor_move_fraction = NULL;
	} else {
		linear_function_motor->num_motors = num_motors;
	
		linear_function_motor->motor_record_array = (MX_RECORD **)
				malloc( num_motors * sizeof(MX_RECORD *) );
	
		if ( linear_function_motor->motor_record_array == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Cannot allocate motor_record_array for %ld motors.",
			num_motors );
		}
	
		linear_function_motor->motor_position_array = (double *)
				malloc( num_motors * sizeof(double) );
	
		if ( linear_function_motor->motor_position_array == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Cannot allocate motor_position_array for %ld motors.",
			num_motors );
		}
	
		linear_function_motor->real_motor_scale = (double *)
				malloc( num_motors * sizeof(double) );
	
		if ( linear_function_motor->real_motor_scale == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Cannot allocate real_motor_scale for %ld motors.",
			num_motors );
		}
	
		linear_function_motor->real_motor_offset = (double *)
				malloc( num_motors * sizeof(double) );
	
		if ( linear_function_motor->real_motor_offset == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Cannot allocate real_motor_offset for %ld motors.",
			num_motors );
		}
	
		linear_function_motor->motor_move_fraction = (double *)
				malloc( num_motors * sizeof(double) );
	
		if ( linear_function_motor->motor_move_fraction == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Cannot allocate motor_move_fraction for %ld motors.",
			num_motors );
		}
	}

	/* Create the arrays for variable parameters. */

	if ( num_variables <= 0 ) {
		linear_function_motor->num_variables = 0L;
		linear_function_motor->variable_record_array = NULL;
		linear_function_motor->variable_value_array = NULL;
		linear_function_motor->real_variable_scale = NULL;
		linear_function_motor->real_variable_offset = NULL;
		linear_function_motor->variable_move_fraction = NULL;
	} else {
		linear_function_motor->num_variables = num_variables;
	
		linear_function_motor->variable_record_array = (MX_RECORD **)
				malloc( num_variables * sizeof(MX_RECORD *) );
	
		if ( linear_function_motor->variable_record_array == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate variable_record_array for %ld variables.",
			num_variables );
		}
	
		linear_function_motor->variable_value_array = (double *)
				malloc( num_variables * sizeof(double) );
	
		if ( linear_function_motor->variable_value_array == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate variable_value_array for %ld variables.",
			num_variables );
		}
	
		linear_function_motor->real_variable_scale = (double *)
				malloc( num_variables * sizeof(double) );
	
		if ( linear_function_motor->real_variable_scale == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate real_variable_scale for %ld variables.",
			num_variables );
		}
	
		linear_function_motor->real_variable_offset = (double *)
				malloc( num_variables * sizeof(double) );
	
		if ( linear_function_motor->real_variable_offset == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate real_variable_offset for %ld variables.",
			num_variables );
		}
	
		linear_function_motor->variable_move_fraction = (double *)
				malloc( num_variables * sizeof(double) );
	
		if ( linear_function_motor->variable_move_fraction == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate variable_move_fraction for %ld variables.",
			num_variables );
		}
	}

	/* Copy the values from the record arrays to the motor and variable
	 * arrays.
	 */

	ivar = imotor = 0L;

	for ( i = 0; i < num_records; i++ ) {
		child_record = record_array[i];

		if ( child_record->mx_superclass == MXR_DEVICE ) {
			if ( imotor >= num_motors ) {
				return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
					fname,
"imotor value of %ld on second pass exceeds maximum value %ld from first pass.",
				imotor, num_motors+1 );
			}

			linear_function_motor->motor_record_array[imotor]
				= linear_function_motor->record_array[i];

			linear_function_motor->motor_position_array[imotor]
				= 0.0;

			linear_function_motor->real_motor_scale[imotor]
				= linear_function_motor->real_scale[i];

			linear_function_motor->real_motor_offset[imotor]
				= linear_function_motor->real_offset[i];

			linear_function_motor->motor_move_fraction[imotor]
				= linear_function_motor->move_fraction[i];

			imotor++;
		} else {
			if ( ivar >= num_variables ) {
				return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
					fname,
"ivar value of %ld on second pass exceeds maximum value %ld from first pass.",
				ivar, num_variables+1 );
			}

			linear_function_motor->variable_record_array[ivar]
				= linear_function_motor->record_array[i];

			linear_function_motor->variable_value_array[ivar] = 0.0;

			linear_function_motor->real_variable_scale[ivar]
				= linear_function_motor->real_scale[i];

			linear_function_motor->real_variable_offset[ivar]
				= linear_function_motor->real_offset[i];

			linear_function_motor->variable_move_fraction[ivar]
				= linear_function_motor->move_fraction[i];

			ivar++;
		}
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linear_function_delete_record( MX_RECORD *record )
{
	MX_LINEAR_FUNCTION_MOTOR *linear_function_motor;

	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}
	if ( record->record_type_struct != NULL ) {
		linear_function_motor = (MX_LINEAR_FUNCTION_MOTOR *)
						record->record_type_struct;

		if ( linear_function_motor->motor_record_array != NULL )
			free( linear_function_motor->motor_record_array );

		if ( linear_function_motor->motor_position_array != NULL )
			free( linear_function_motor->motor_position_array );

		if ( linear_function_motor->real_motor_scale != NULL )
			free( linear_function_motor->real_motor_scale );

		if ( linear_function_motor->real_motor_offset != NULL )
			free( linear_function_motor->real_motor_offset );

		if ( linear_function_motor->motor_move_fraction != NULL )
			free( linear_function_motor->motor_move_fraction );

		if ( linear_function_motor->variable_record_array != NULL )
			free( linear_function_motor->variable_record_array );

		if ( linear_function_motor->variable_value_array != NULL )
			free( linear_function_motor->variable_value_array );

		if ( linear_function_motor->real_variable_scale != NULL )
			free( linear_function_motor->real_variable_scale );

		if ( linear_function_motor->real_variable_offset != NULL )
			free( linear_function_motor->real_variable_offset );

		if ( linear_function_motor->variable_move_fraction != NULL )
			free( linear_function_motor->variable_move_fraction );

		free( linear_function_motor );

		record->record_type_struct = NULL;
	}
	if ( record->record_class_struct != NULL ) {
		free( record->record_class_struct );

		record->record_class_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linear_function_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] =
			"mxd_linear_function_print_motor_structure()";

	MX_MOTOR *motor;
	MX_LINEAR_FUNCTION_MOTOR *linear_function_motor;
	MX_RECORD **record_array;
	double *real_scale, *real_offset, *move_fraction;
	long i, num_records;
	double position, move_deadband;
	mx_status_type mx_status;

	linear_function_motor = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_linear_function_get_pointers( motor,
					&linear_function_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_records = linear_function_motor->num_records;
	record_array = linear_function_motor->record_array;
	real_scale = linear_function_motor->real_scale;
	real_offset = linear_function_motor->real_offset;
	move_fraction = linear_function_motor->move_fraction;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type           = LINEAR_FUNCTION_MOTOR.\n\n");

	fprintf(file, "  name                 = %s\n", record->name);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	fprintf(file, "  position             = %g %s  (%g).\n",
		motor->position, motor->units, motor->raw_position.analog );
	fprintf(file, "  scale                = %g %s per raw unit.\n",
		motor->scale, motor->units);
	fprintf(file, "  offset               = %g %s.\n",
		motor->offset, motor->units);
	fprintf(file, "  backlash             = %g %s  (%g).\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog);
	fprintf(file, "  negative limit       = %g %s  (%g).\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog);
	fprintf(file, "  positive limit       = %g %s  (%g).\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog);

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband        = %g %s  (%g).\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	fprintf(file, "  num records          = %ld\n", num_records);

	fprintf(file, "  record array         = (");

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

	fprintf(file, "  move fraction        = (");

	if ( move_fraction == NULL ) {
		fprintf(file, "NULL");
	} else {
		for ( i = 0; i < num_records; i++ ) {
			fprintf(file, "%g", move_fraction[i]);

			if ( i != num_records-1 ) {
				fprintf(file, ", ");
			}
		}
	}
	fprintf(file, ")\n");

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linear_function_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_linear_function_motor_is_busy()";

	MX_LINEAR_FUNCTION_MOTOR *linear_function_motor;
	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	long i, num_motors;
	mx_bool_type busy;
	mx_status_type mx_status;

	linear_function_motor = NULL;

	mx_status = mxd_linear_function_get_pointers( motor,
					&linear_function_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = linear_function_motor->num_motors;
	motor_record_array = linear_function_motor->motor_record_array;

	/* Only motors can be busy. */

	for ( i = 0; i < num_motors; i++ ) {
	
		child_motor_record = motor_record_array[i];

		mx_status = mx_motor_is_busy( child_motor_record, &busy );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* If the child motor is busy, we are done. */

		if ( busy == TRUE ) {
			motor->busy = TRUE;

			return MX_SUCCESSFUL_RESULT;
		}
	}

	motor->busy = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linear_function_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_linear_function_move_absolute()";

	MX_LINEAR_FUNCTION_MOTOR *linear_function_motor;
	MX_RECORD **motor_record_array, **variable_record_array;
	MX_RECORD *child_motor_record, *child_variable_record;
	void *pointer_to_value;
	double *double_pointer;
	double *motor_position_array, *variable_value_array;
	double *real_motor_scale, *real_motor_offset;
	double *real_variable_scale, *real_variable_offset;
	double *motor_move_fraction, *variable_move_fraction;
	int move_flags;
	long i, num_motors, num_variables, linear_flags;
	double new_pseudomotor_position, old_pseudomotor_position;
	double pseudomotor_difference, motor_position_difference;
	double variable_value_difference;
	double numerator, denominator;
	double old_motor_position_array, old_variable_value_array;
	mx_status_type mx_status;

	linear_function_motor = NULL;

	mx_status = mxd_linear_function_get_pointers( motor,
					&linear_function_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = linear_function_motor->num_motors;
	motor_record_array = linear_function_motor->motor_record_array;
	motor_position_array = linear_function_motor->motor_position_array;
	real_motor_scale = linear_function_motor->real_motor_scale;
	real_motor_offset = linear_function_motor->real_motor_offset;
	motor_move_fraction = linear_function_motor->motor_move_fraction;

	num_variables = linear_function_motor->num_variables;
	variable_record_array = linear_function_motor->variable_record_array;
	variable_value_array = linear_function_motor->variable_value_array;
	real_variable_scale = linear_function_motor->real_variable_scale;
	real_variable_offset = linear_function_motor->real_variable_offset;
	variable_move_fraction = linear_function_motor->variable_move_fraction;

	new_pseudomotor_position = motor->raw_destination.analog;

	/* Check the limits on this request. */

	if ( new_pseudomotor_position > motor->raw_positive_limit.analog
	  || new_pseudomotor_position < motor->raw_negative_limit.analog )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Requested position %g is outside the allowed range of %g to %g",
		new_pseudomotor_position, motor->raw_negative_limit.analog,
			motor->raw_positive_limit.analog );
	}

	/* Get the old positions. */

	old_pseudomotor_position = 0.0;

	for ( i = 0; i < num_motors; i++ ) {
		child_motor_record = motor_record_array[i];

		mx_status = mx_motor_get_position( child_motor_record,
						&motor_position_array[i] );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		old_pseudomotor_position
			+= ( motor_position_array[i] * real_motor_scale[i]
					+ real_motor_offset[i] );
	}

	for ( i = 0; i < num_variables; i++ ) {
		child_variable_record = variable_record_array[i];

		mx_status = mx_receive_variable( child_variable_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_get_variable_pointer( child_variable_record,
					&pointer_to_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		variable_value_array[i] = *(double *) pointer_to_value;

		old_pseudomotor_position
			+= ( variable_value_array[i] * real_variable_scale[i]
					+ real_variable_offset[i] );
	}

	pseudomotor_difference = new_pseudomotor_position
					- old_pseudomotor_position;

	MX_DEBUG( 2,
	("%s: old_pseudomotor_position = %g, new_pseudomotor_position = %g",
		fname, old_pseudomotor_position, new_pseudomotor_position));
	MX_DEBUG( 2,("%s: pseudomotor_difference = %g",
			fname, pseudomotor_difference));

	/* Now compute the new positions. */

	for ( i = 0; i < num_motors; i++ ) {
		numerator = pseudomotor_difference * motor_move_fraction[i];
		denominator = real_motor_scale[i];

		MX_DEBUG( 2,
	("%s: real_motor_scale[%ld] = %g, motor_move_fraction[%ld] = %g",
		fname, i, real_motor_scale[i], i, motor_move_fraction[i]));

		MX_DEBUG( 2,("%s: numerator = %g, denominator = %g",
			fname, numerator, denominator));

		if ( fabs(denominator) >= 1.0 ) {
			motor_position_difference = numerator / denominator;
		} else {
			if ( numerator < fabs(denominator * DBL_MAX) ) {
				motor_position_difference
					= numerator / denominator;
			} else {
				motor_position_difference = DBL_MAX;
			}
		}
		old_motor_position_array = motor_position_array[i];

		motor_position_array[i] += motor_position_difference;

		MX_DEBUG( 2,("%s: motor_position_difference = %g",
			fname, motor_position_difference));
		MX_DEBUG( 2,
("%s: Old motor position array[%ld] = %g, New motor position array[%ld] = %g",
fname, i, old_motor_position_array, i, motor_position_array[i]));
	}

	for ( i = 0; i < num_variables; i++ ) {
		numerator = pseudomotor_difference * variable_move_fraction[i];
		denominator = real_variable_scale[i];

		MX_DEBUG( 2,
	("%s: real_variable_scale[%ld] = %g, variable_move_fraction[%ld] = %g",
	fname, i, real_variable_scale[i], i, variable_move_fraction[i]));

		MX_DEBUG( 2,("%s: numerator = %g, denominator = %g",
			fname, numerator, denominator));

		if ( fabs(denominator) >= 1.0 ) {
			variable_value_difference = numerator / denominator;
		} else {
			if ( numerator < fabs(denominator * DBL_MAX) ) {
				variable_value_difference
					= numerator / denominator;
			} else {
				variable_value_difference = DBL_MAX;
			}
		}
		old_variable_value_array = variable_value_array[i];

		variable_value_array[i] += variable_value_difference;

		MX_DEBUG( 2,("%s: variable_value_difference = %g",
			fname, variable_value_difference));
		MX_DEBUG( 2,
("%s: Old variable value array[%ld] = %g, New variable value array[%ld] = %g",
fname, i, old_variable_value_array, i, variable_value_array[i]));
	}

	/* Perform the motor moves. */

	linear_flags = linear_function_motor->linear_function_flags;

	if ( linear_flags & MXF_LINEAR_FUNCTION_MOTOR_SIMULTANEOUS_START ) {
		move_flags = MXF_MTR_NOWAIT | MXF_MTR_SIMULTANEOUS_START;
	} else {
		move_flags = MXF_MTR_NOWAIT;
	}

	mx_status = mx_motor_array_move_absolute(
			num_motors, motor_record_array, motor_position_array,
			move_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Change the variables. */

	for ( i = 0; i < num_variables; i++ ) {

		/* Nominally setting variable_move_fraction[i] to zero
		 * should suppress changes to the value of the variable.
		 * However, since we do not, in general, know how the
		 * remote server is implemented, it is best to avoid sending
		 * a new variable value at all if variable_move_fraction[i]
		 * is extremely small.  This is to try to avoid having the
		 * value of a variable that is really supposed to be constant
		 * gradually change over time due to floating point roundoff
		 * errors.
		 *
		 * Another reason to suppress the change is that sometimes the
		 * value of a variable that you can read is write protected.
		 */

		if ( fabs( variable_move_fraction[i] ) > 1.0e-30 ) {

			child_variable_record = variable_record_array[i];

			mx_status = mx_get_variable_pointer(
						child_variable_record,
						&pointer_to_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			double_pointer = (double *) pointer_to_value;

			*double_pointer = variable_value_array[i];

			mx_status = mx_send_variable( child_variable_record );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linear_function_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_linear_function_get_position()";

	MX_RECORD **motor_record_array, **variable_record_array;
	MX_RECORD *child_motor_record, *child_variable_record;
	double *real_motor_scale, *real_motor_offset;
	double *real_variable_scale, *real_variable_offset;
	void *pointer_to_value;
	long i, num_motors, num_variables;
	MX_LINEAR_FUNCTION_MOTOR *linear_function_motor;
	double pseudomotor_position, motor_position, variable_value;
	mx_status_type mx_status;

	linear_function_motor = NULL;

	mx_status = mxd_linear_function_get_pointers( motor,
					&linear_function_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = linear_function_motor->num_motors;
	motor_record_array = linear_function_motor->motor_record_array;
	real_motor_scale = linear_function_motor->real_motor_scale;
	real_motor_offset = linear_function_motor->real_motor_offset;

	num_variables = linear_function_motor->num_variables;
	variable_record_array = linear_function_motor->variable_record_array;
	real_variable_scale = linear_function_motor->real_variable_scale;
	real_variable_offset = linear_function_motor->real_variable_offset;

	/* Get the current positions. */

	pseudomotor_position = 0.0;

	for ( i = 0; i < num_motors; i++ ) {
		child_motor_record = motor_record_array[i];

		mx_status = mx_motor_get_position( child_motor_record,
						&motor_position );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		pseudomotor_position += ( motor_position * real_motor_scale[i]
						+ real_motor_offset[i] );
	}

	for ( i = 0; i < num_variables; i++ ) {
		child_variable_record = variable_record_array[i];

		mx_status = mx_receive_variable( child_variable_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_get_variable_pointer( child_variable_record,
					&pointer_to_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		variable_value = *(double *) pointer_to_value;

		pseudomotor_position
			+= ( variable_value * real_variable_scale[i]
					+ real_variable_offset[i] );
	}

	motor->raw_position.analog = pseudomotor_position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linear_function_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_linear_function_soft_abort()";

	MX_RECORD **motor_record_array;
	long i, num_motors;
	MX_LINEAR_FUNCTION_MOTOR *linear_function_motor;
	mx_status_type mx_status;

	linear_function_motor = NULL;

	mx_status = mxd_linear_function_get_pointers( motor,
					&linear_function_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = linear_function_motor->num_motors;
	motor_record_array = linear_function_motor->motor_record_array;

	mx_status = MX_SUCCESSFUL_RESULT;

	/* Only motor records are aborted. */

	for ( i = 0; i < num_motors; i++ ) {
		(void) mx_motor_soft_abort( motor_record_array[i] );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linear_function_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_linear_function_immediate_abort()";

	MX_RECORD **motor_record_array;
	long i, num_motors;
	MX_LINEAR_FUNCTION_MOTOR *linear_function_motor;
	mx_status_type mx_status;

	linear_function_motor = NULL;

	mx_status = mxd_linear_function_get_pointers( motor,
					&linear_function_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = linear_function_motor->num_motors;
	motor_record_array = linear_function_motor->motor_record_array;

	mx_status = MX_SUCCESSFUL_RESULT;

	/* Only motor records are aborted. */

	for ( i = 0; i < num_motors; i++ ) {
		(void) mx_motor_immediate_abort( motor_record_array[i] );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_linear_function_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_linear_function_positive_limit_hit()";

	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	MX_LINEAR_FUNCTION_MOTOR *linear_function_motor;
	long i, num_motors;
	mx_bool_type limit_hit;
	mx_status_type mx_status;

	linear_function_motor = NULL;

	mx_status = mxd_linear_function_get_pointers( motor,
					&linear_function_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = linear_function_motor->num_motors;
	motor_record_array = linear_function_motor->motor_record_array;

	limit_hit = FALSE;

	/* Only motor records are checked. */

	for ( i = 0; i < num_motors; i++ ) {
		child_motor_record = motor_record_array[i];

		mx_status = mx_motor_positive_limit_hit(
				child_motor_record, &limit_hit );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( limit_hit == TRUE ) {
			(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"Positive limit hit for motor '%s'",
				child_motor_record->name );

			break;             /* exit the for() loop */
		}
	}

	motor->positive_limit_hit = limit_hit;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linear_function_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_linear_function_negative_limit_hit()";

	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	MX_LINEAR_FUNCTION_MOTOR *linear_function_motor;
	long i, num_motors;
	mx_bool_type limit_hit;
	mx_status_type mx_status;

	linear_function_motor = NULL;

	mx_status = mxd_linear_function_get_pointers( motor,
					&linear_function_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = linear_function_motor->num_motors;
	motor_record_array = linear_function_motor->motor_record_array;

	limit_hit = FALSE;

	/* Only motor records are checked. */

	for ( i = 0; i < num_motors; i++ ) {
		child_motor_record = motor_record_array[i];

		mx_status = mx_motor_negative_limit_hit(
				child_motor_record, &limit_hit );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( limit_hit == TRUE ) {
			(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"Negative limit hit for motor '%s'",
				child_motor_record->name );

			break;             /* exit the for() loop */
		}
	}

	motor->negative_limit_hit = limit_hit;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linear_function_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_linear_function_raw_home_command()";

	MX_RECORD **motor_record_array = NULL;
	MX_RECORD *child_motor_record = NULL;
	MX_LINEAR_FUNCTION_MOTOR *linear_function_motor = NULL;
	MX_MOTOR *child_motor;
	long num_motors;
	double total_scale_factor;
	mx_status_type mx_status;

	mx_status = mxd_linear_function_get_pointers( motor,
					&linear_function_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = linear_function_motor->num_motors;
	motor_record_array = linear_function_motor->motor_record_array;

	if ( num_motors != 1 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The '%s' driver for pseudomotor '%s' only supports homing "
		"if the pseudomotor depends on only one real motor.  "
		"Unfortunately, this pseudomotor depends on %ld motors.  "
		"You will need to separately home each of the dependent "
		"motors instead.",
			mx_get_driver_name( motor->record ),
			motor->record->name,
			num_motors );
	}

	child_motor_record = motor_record_array[0];

	if ( child_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The child motor record pointer for pseudomotor '%s' is NULL.",
			motor->record->name );
	}

	child_motor = (MX_MOTOR *) child_motor_record->record_class_struct;

	if ( child_motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for motor '%s' used by "
		"pseudomotor '%s' is NULL.",
			child_motor_record->name,
			motor->record->name );
	}

	total_scale_factor = linear_function_motor->real_motor_scale[0]
				* child_motor->scale;

	if ( total_scale_factor >= 0 ) {
		mx_status = mx_motor_home_search( child_motor_record, 1, 0 );
	} else {
		mx_status = mx_motor_home_search( child_motor_record, -1, 0 );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_linear_function_get_parameter( MX_MOTOR *motor )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linear_function_set_parameter( MX_MOTOR *motor )
{
	return MX_SUCCESSFUL_RESULT;
}

