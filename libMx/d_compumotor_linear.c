/*
 * Name:    d_compumotor_linear.c 
 *
 * Purpose: MX motor driver for a Compumotor 6K controller to perform
 *          linear interpolation moves of several motors at once.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2004, 2006-2007, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define COMPUMOTOR_LINEAR_DEBUG  FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_variable.h"
#include "d_compumotor.h"
#include "d_compumotor_linear.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_compumotor_linear_record_function_list = {
	mxd_compumotor_linear_initialize_type,
	mxd_compumotor_linear_create_record_structures,
	mxd_compumotor_linear_finish_record_initialization,
	mxd_compumotor_linear_delete_record,
	mxd_compumotor_linear_print_motor_structure,
	mxd_compumotor_linear_open,
	mxd_compumotor_linear_close
};

MX_MOTOR_FUNCTION_LIST mxd_compumotor_linear_motor_function_list = {
	mxd_compumotor_linear_motor_is_busy,
	mxd_compumotor_linear_move_absolute,
	mxd_compumotor_linear_get_position,
	mxd_compumotor_linear_set_position,
	mxd_compumotor_linear_soft_abort,
	mxd_compumotor_linear_immediate_abort,
	mxd_compumotor_linear_positive_limit_hit,
	mxd_compumotor_linear_negative_limit_hit,
	mxd_compumotor_linear_find_home_position,
	NULL,
	mx_motor_default_get_parameter_handler,
	mx_motor_default_set_parameter_handler
};

/* Linear function motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_compumotor_linear_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_COMPUMOTOR_LINEAR_STANDARD_FIELDS
};

long mxd_compumotor_linear_num_record_fields
		= sizeof(mxd_compumotor_linear_record_field_defaults)
		/ sizeof(mxd_compumotor_linear_record_field_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_compumotor_linear_rfield_def_ptr
			= &mxd_compumotor_linear_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_compumotor_linear_get_pointers( MX_MOTOR *motor,
			MX_COMPUMOTOR_LINEAR_MOTOR **compumotor_linear_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_compumotor_linear_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( compumotor_linear_motor == (MX_COMPUMOTOR_LINEAR_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	"The MX_COMPUMOTOR_LINEAR_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*compumotor_linear_motor = (MX_COMPUMOTOR_LINEAR_MOTOR *)
				(motor->record->record_type_struct);

	if ( *compumotor_linear_motor == (MX_COMPUMOTOR_LINEAR_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_COMPUMOTOR_LINEAR_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

#define NUM_COMPUMOTOR_LINEAR_FIELDS	4

MX_EXPORT mx_status_type
mxd_compumotor_linear_initialize_type( long type )
{
        static const char fname[] = "mxs_compumotor_linear_initialize_type()";

        const char field_name[NUM_COMPUMOTOR_LINEAR_FIELDS]
					[MXU_FIELD_NAME_LENGTH+1]
            = {
                "motor_record_array",
		"real_motor_scale",
		"real_motor_offset",
		"motor_move_fraction" };

        MX_DRIVER *driver;
        MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
        MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
        MX_RECORD_FIELD_DEFAULTS *field;
        int i;
        long num_record_fields;
	long referenced_field_index;
        long num_motors_varargs_cookie;
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
                        "num_motors", &referenced_field_index );

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

	mx_status = mx_construct_varargs_cookie(
                        referenced_field_index, 0, &num_motors_varargs_cookie);

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

        MX_DEBUG( 2,("%s: num_records varargs cookie = %ld",
                        fname, num_motors_varargs_cookie));

	for ( i = 0; i < NUM_COMPUMOTOR_LINEAR_FIELDS; i++ ) {
		mx_status = mx_find_record_field_defaults(
			record_field_defaults, num_record_fields,
			field_name[i], &field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		field->dimension[0] = num_motors_varargs_cookie;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_linear_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_compumotor_linear_create_record_structures()";

	MX_MOTOR *motor;
	MX_COMPUMOTOR_LINEAR_MOTOR *compumotor_linear;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	compumotor_linear = (MX_COMPUMOTOR_LINEAR_MOTOR *)
				malloc( sizeof(MX_COMPUMOTOR_LINEAR_MOTOR) );

	if ( compumotor_linear == (MX_COMPUMOTOR_LINEAR_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Can't allocate memory for MX_COMPUMOTOR_LINEAR_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = compumotor_linear;
	record->class_specific_function_list
				= &mxd_compumotor_linear_motor_function_list;

	motor->record = record;

	/* A Compumotor linear interpolation motor is treated as
	 * an analog motor.
	 */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_linear_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_compumotor_linear_finish_record_initialization()";

	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	MX_MOTOR *motor;
	MX_COMPUMOTOR_LINEAR_MOTOR *compumotor_linear_motor;
	MX_RECORD *compumotor_interface_record;
	MX_RECORD *first_compumotor_interface_record;
	MX_COMPUMOTOR *compumotor;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	long i, num_motors, axis_number;
	long controller_index;
	mx_status_type mx_status;

	compumotor_linear_motor = NULL;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_compumotor_linear_get_pointers( motor,
					&compumotor_linear_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = compumotor_linear_motor->num_motors;
	motor_record_array = compumotor_linear_motor->motor_record_array;

	/* Create the motor position and axis number arrays. */

	if ( num_motors <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		    "There must be at least one Compumotor motor record "
		    "specified for a Compumotor linear interpolation record.");
	} else {
		compumotor_linear_motor->motor_position_array = (double *)
				malloc( num_motors * sizeof(double) );
	
		if ( compumotor_linear_motor->motor_position_array == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Cannot allocate motor_position_array for %ld motors.",
			num_motors );
		}

		compumotor_linear_motor->index_to_axis_number = (long *)
				malloc( num_motors * sizeof(long) );
	
		if ( compumotor_linear_motor->index_to_axis_number == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Cannot allocate axis_number for %ld motors.",
			num_motors );
		}
	}

	/* Verify that only Compumotor motor records are in the motor
	 * record array.  Also initialize the recorded motor positions
	 * to 0.
	 */

	for ( i = 0; i < num_motors; i++ ) {
		child_motor_record = motor_record_array[i];

		if ( (child_motor_record->mx_superclass != MXR_DEVICE)
		  || (child_motor_record->mx_class != MXC_MOTOR)
		  || (child_motor_record->mx_type != MXT_MTR_COMPUMOTOR) )
		{
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"Record '%s' is not a Compumotor motor record.",
				child_motor_record->name );
		}

		compumotor_linear_motor->motor_position_array[i] = 0.0;
	}

	/* Verify that the Compumotor motors are all controlled via the
	 * same Compumotor interface and record their axis numbers in
	 * local arrays.
	 */

	for ( i = 0; i < MX_MAX_COMPUMOTOR_AXES; i++ ) {
		compumotor_linear_motor->axis_number_to_index[i] = -1;
	}

	first_compumotor_interface_record = NULL;

	for ( i = 0; i < num_motors; i++ ) {
		child_motor_record = motor_record_array[i];

		compumotor = (MX_COMPUMOTOR *)
				child_motor_record->record_type_struct;

		controller_index = compumotor->controller_index;

		if ( controller_index < 0 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Compumotor linear motor '%s' _must_ be listed in the MX database _after_ "
"motor '%s' which it depends on.", record->name, child_motor_record->name );
		}

		if ( i == 0 ) {
			first_compumotor_interface_record
				= compumotor->compumotor_interface_record;

			compumotor_linear_motor->controller_index
							= controller_index;
		} else {
			compumotor_interface_record
				= compumotor->compumotor_interface_record;

			if ( compumotor_interface_record
			  != first_compumotor_interface_record )
			{
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"All motors in a Compumotor linear interpolation record must "
	"be controlled by the same Compumotor controller, but '%s' and '%s' "
	"are controlled by different controllers.",
					motor_record_array[0]->name,
					child_motor_record->name );
			}
		}

		axis_number = compumotor->axis_number;

		if ( (axis_number < 1)
		  || (axis_number > MX_MAX_COMPUMOTOR_AXES) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal Compumotor axis number %ld for motor '%s'.",
				axis_number, motor_record_array[i]->name );
		}

		compumotor_linear_motor->index_to_axis_number[i] = axis_number;

		/* We are storing 1-based indices in a C 0-based array,
		 * so we must add and subtract appropriate offsets of 1.
		 */

		compumotor_linear_motor->axis_number_to_index[
						axis_number - 1 ] = i + 1;
	}

	compumotor_linear_motor->compumotor_interface_record
		= first_compumotor_interface_record;

	compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
			first_compumotor_interface_record->record_type_struct;

	if ( compumotor_interface == (MX_COMPUMOTOR_INTERFACE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_COMPUMOTOR_INTERFACE pointer for Compumotor interface '%s' is NULL.",
			first_compumotor_interface_record->name );
	}

	i = compumotor_linear_motor->controller_index;

	compumotor_linear_motor->num_axes = compumotor_interface->num_axes[i];

#if 0
	fprintf(stderr, "\nindex_to_axis_number = ");

	for ( i = 0; i < compumotor_linear_motor->num_motors; i++ ) {
		fprintf( stderr, "%d ",
			compumotor_linear_motor->index_to_axis_number[i] );
	}
	fprintf(stderr, "\naxis_number_to_index = ");

	for ( i = 0; i < MX_MAX_COMPUMOTOR_AXES; i++ ) {
		fprintf( stderr, "%d ",
			compumotor_linear_motor->axis_number_to_index[i] );
	}
	fprintf(stderr, "\n");
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_linear_delete_record( MX_RECORD *record )
{
	MX_COMPUMOTOR_LINEAR_MOTOR *compumotor_linear_motor;

	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}
	if ( record->record_type_struct != NULL ) {
		compumotor_linear_motor = (MX_COMPUMOTOR_LINEAR_MOTOR *)
						record->record_type_struct;

		if ( compumotor_linear_motor->motor_position_array != NULL )
			free( compumotor_linear_motor->motor_position_array );

		if ( compumotor_linear_motor->index_to_axis_number != NULL )
			free( compumotor_linear_motor->index_to_axis_number );

		free( compumotor_linear_motor );

		record->record_type_struct = NULL;
	}
	if ( record->record_class_struct != NULL ) {
		free( record->record_class_struct );

		record->record_class_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_linear_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] =
			"mxd_compumotor_linear_print_motor_structure()";

	MX_MOTOR *motor;
	MX_COMPUMOTOR_LINEAR_MOTOR *compumotor_linear_motor;
	MX_RECORD **motor_record_array;
	double *real_motor_scale, *real_motor_offset, *motor_move_fraction;
	long i, num_motors;
	double position, move_deadband;
	mx_status_type mx_status;

	compumotor_linear_motor = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_compumotor_linear_get_pointers( motor,
					&compumotor_linear_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors          = compumotor_linear_motor->num_motors;
	motor_record_array  = compumotor_linear_motor->motor_record_array;
	real_motor_scale    = compumotor_linear_motor->real_motor_scale;
	real_motor_offset   = compumotor_linear_motor->real_motor_offset;
	motor_move_fraction = compumotor_linear_motor->motor_move_fraction;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type           = COMPUMOTOR_LINEAR_MOTOR.\n\n");

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

	fprintf(file, "  flags                = %#lx\n", 
			compumotor_linear_motor->flags );

	fprintf(file, "  num motors           = %ld\n", num_motors);

	fprintf(file, "  motor record array   = (");

	if ( motor_record_array == NULL ) {
		fprintf(file, "NULL");
	} else {
		for ( i = 0; i < num_motors; i++ ) {
			fprintf(file, "%s", motor_record_array[i]->name);

			if ( i != num_motors-1 ) {
				fprintf(file, ", ");
			}
		}
	}
	fprintf(file, ")\n");

	fprintf(file, "  real motor scale     = (");

	if ( real_motor_scale == NULL ) {
		fprintf(file, "NULL");
	} else {
		for ( i = 0; i < num_motors; i++ ) {
			fprintf(file, "%g", real_motor_scale[i]);

			if ( i != num_motors-1 ) {
				fprintf(file, ", ");
			}
		}
	}
	fprintf(file, ")\n");

	fprintf(file, "  real motor offset    = (");

	if ( real_motor_offset == NULL ) {
		fprintf(file, "NULL");
	} else {
		for ( i = 0; i < num_motors; i++ ) {
			fprintf(file, "%g", real_motor_offset[i]);

			if ( i != num_motors-1 ) {
				fprintf(file, ", ");
			}
		}
	}
	fprintf(file, ")\n");

	fprintf(file, "  motor move fraction  = (");

	if ( motor_move_fraction == NULL ) {
		fprintf(file, "NULL");
	} else {
		for ( i = 0; i < num_motors; i++ ) {
			fprintf(file, "%g", motor_move_fraction[i]);

			if ( i != num_motors-1 ) {
				fprintf(file, ", ");
			}
		}
	}
	fprintf(file, ")\n");

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_linear_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_compumotor_linear_open()";

	MX_MOTOR *motor;
	MX_COMPUMOTOR_LINEAR_MOTOR *compumotor_linear_motor;
	MX_RECORD *compumotor_interface_record;
	mx_status_type mx_status;

	compumotor_linear_motor = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_compumotor_linear_get_pointers( motor,
					&compumotor_linear_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	compumotor_interface_record
		= compumotor_linear_motor->compumotor_interface_record;

	if ( compumotor_interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The compumotor_interface_record pointer for motor '%s' is NULL.",
			motor->record->name );
	}
#if 1
	/* Use the interface record's event time manager. */

	record->event_time_manager =
			compumotor_interface_record->event_time_manager;
#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_linear_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_linear_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_linear_motor_is_busy()";

	MX_COMPUMOTOR_LINEAR_MOTOR *compumotor_linear_motor;
	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	long i, num_motors;
	mx_bool_type busy;
	mx_status_type mx_status;

	compumotor_linear_motor = NULL;

	mx_status = mxd_compumotor_linear_get_pointers( motor,
					&compumotor_linear_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = compumotor_linear_motor->num_motors;
	motor_record_array = compumotor_linear_motor->motor_record_array;

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
mxd_compumotor_linear_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_linear_move_absolute()";

	MX_COMPUMOTOR_LINEAR_MOTOR *compumotor_linear_motor;
	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	MX_RECORD *compumotor_interface_record;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	double *motor_position_array;
	double *real_motor_scale, *real_motor_offset, *motor_move_fraction;
	double new_pseudomotor_position, old_pseudomotor_position;
	double pseudomotor_difference, motor_position_difference;
	double numerator, denominator;
	double old_motor_position_array;
	long i, num_motors;
	int simultaneous_start;
	mx_status_type mx_status;

	compumotor_linear_motor = NULL;

	mx_status = mxd_compumotor_linear_get_pointers( motor,
					&compumotor_linear_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = compumotor_linear_motor->num_motors;
	motor_record_array = compumotor_linear_motor->motor_record_array;
	motor_position_array = compumotor_linear_motor->motor_position_array;
	real_motor_scale = compumotor_linear_motor->real_motor_scale;
	real_motor_offset = compumotor_linear_motor->real_motor_offset;
	motor_move_fraction = compumotor_linear_motor->motor_move_fraction;

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

	/* Get the MX_COMPUMOTOR_INTERFACE structure so that we can send
	 * commands directly to the controller.
	 */

	compumotor_interface_record
		= compumotor_linear_motor->compumotor_interface_record;

	if ( compumotor_interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The compumotor_interface_record pointer for motor '%s' is NULL.",
			motor->record->name );
	}

	compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
			compumotor_interface_record->record_type_struct;

	if ( compumotor_interface == (MX_COMPUMOTOR_INTERFACE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_COMPUMOTOR_INTERFACE pointer for Compumotor interface '%s' is NULL.",
			compumotor_interface_record->name );
	}

	/* Are we using a simultaneous start or are we using the 
	 * linear interpolation functionality?
	 */

	if (compumotor_linear_motor->flags & MXF_COMPUMOTOR_SIMULTANEOUS_START)
	{
		simultaneous_start = TRUE;
	} else {
		simultaneous_start = FALSE;
	}

	mx_status = mxi_compumotor_multiaxis_move( compumotor_interface,
			compumotor_linear_motor->controller_index,
			num_motors, motor_record_array, motor_position_array,
			simultaneous_start );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_compumotor_linear_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_linear_get_position()";

	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	MX_MOTOR *child_motor;
	MX_RECORD *compumotor_interface_record;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	double *real_motor_scale, *real_motor_offset;
	long i, j, n, motor_index, num_motors;
	int num_items;
	MX_COMPUMOTOR_LINEAR_MOTOR *compumotor_linear_motor;
	double motor_position, raw_motor_position;
	double pseudomotor_position, pseudomotor_change;
	double pseudomotor_child_scale, pseudomotor_child_offset;
	char command[80], response[80];
	char *ptr;
	mx_status_type mx_status;

	compumotor_linear_motor = NULL;

	mx_status = mxd_compumotor_linear_get_pointers( motor,
					&compumotor_linear_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = compumotor_linear_motor->num_motors;
	motor_record_array = compumotor_linear_motor->motor_record_array;
	real_motor_scale = compumotor_linear_motor->real_motor_scale;
	real_motor_offset = compumotor_linear_motor->real_motor_offset;

	compumotor_interface_record
		= compumotor_linear_motor->compumotor_interface_record;

	compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
    			compumotor_interface_record->record_type_struct;

	/* Get the current positions. */

	n = compumotor_linear_motor->controller_index;

	snprintf( command, sizeof(command), "%ld_!TPE",
				compumotor_interface->controller_number[n] );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
			response, sizeof(response), COMPUMOTOR_LINEAR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pseudomotor_position = 0.0;

	ptr = response;

	for ( i = 0; i < MX_MAX_COMPUMOTOR_AXES; i++ ) {

		raw_motor_position = 0.0;

		num_items = sscanf( ptr, "%lg", &raw_motor_position );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"The response string for the TPE command was unparseable.  "
		"Response = '%s'", response );
		}

		motor_index = compumotor_linear_motor->axis_number_to_index[i];

		if ( motor_index > 0 ) {
			j = motor_index - 1;

			child_motor_record = motor_record_array[j];

			child_motor = (MX_MOTOR *)
				child_motor_record->record_class_struct;

			motor_position = child_motor->offset
				+ child_motor->scale * raw_motor_position;

			pseudomotor_child_scale
			    = compumotor_linear_motor->real_motor_scale[j];

			pseudomotor_child_offset
			    = compumotor_linear_motor->real_motor_offset[j];

			pseudomotor_change
			    = pseudomotor_child_scale * motor_position
					+ pseudomotor_child_offset;

			pseudomotor_position += pseudomotor_change;
		}

		ptr = strchr( ptr, ',' );

		if ( ptr == NULL ) {
			break;		/* Exit the for() loop. */
		}

		ptr++;
	}
		
	motor->raw_position.analog = pseudomotor_position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_linear_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_linear_set_position()";

	return mx_error( MXE_UNSUPPORTED, fname,
"'set position' is not valid for a Compumotor linear interpolation motor." );
}

MX_EXPORT mx_status_type
mxd_compumotor_linear_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_linear_soft_abort()";

	MX_RECORD **motor_record_array;
	long i, num_motors;
	MX_COMPUMOTOR_LINEAR_MOTOR *compumotor_linear_motor;
	MX_RECORD *compumotor_interface_record;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	char command[80];
	char *ptr;
	size_t length, buffer_left;
	long n;
	mx_bool_type will_be_stopped;
	mx_status_type mx_status;

	compumotor_linear_motor = NULL;

	mx_status = mxd_compumotor_linear_get_pointers( motor,
					&compumotor_linear_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = compumotor_linear_motor->num_motors;
	motor_record_array = compumotor_linear_motor->motor_record_array;

	compumotor_interface_record
		= compumotor_linear_motor->compumotor_interface_record;

	compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
			compumotor_interface_record->record_type_struct;

	/*
	 * Construct a stop command for all the motors used by
	 * this pseudomotor.
	 */

	n = compumotor_linear_motor->controller_index;

	snprintf( command, sizeof(command), "%ld_!S",
				compumotor_interface->controller_number[n] );

	for ( i = 0; i < compumotor_linear_motor->num_axes; i++ ) {

		if ( compumotor_linear_motor->axis_number_to_index[i] > 0 ) {
			will_be_stopped = TRUE;
		} else {
			will_be_stopped = FALSE;
		}

		length = strlen( command );

		ptr = command + length;

		buffer_left = sizeof( command ) - length;

		if ( will_be_stopped ) {
			strlcat( ptr, "1", buffer_left );
		} else {
			strlcat( ptr, "0", buffer_left );
		}
	}

	/* Stop the motion. */

	mx_status = mxi_compumotor_command( compumotor_interface, command,
			NULL, 0, COMPUMOTOR_LINEAR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_compumotor_linear_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_linear_immediate_abort()";

	MX_COMPUMOTOR_LINEAR_MOTOR *compumotor_linear_motor;
	MX_RECORD *compumotor_interface_record;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	char command[80];
	long n;
	mx_status_type mx_status;
 
	compumotor_linear_motor = NULL;

	mx_status = mxd_compumotor_linear_get_pointers( motor,
					&compumotor_linear_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	compumotor_interface_record
		= compumotor_linear_motor->compumotor_interface_record;

	compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
		compumotor_interface_record->record_type_struct;

	/* Send a kill motion command to halt all axes attached to
	 * this controller.
	 */

	n = compumotor_linear_motor->controller_index;

	snprintf( command, sizeof(command), "%ld_!K",
				compumotor_interface->controller_number[n] );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, COMPUMOTOR_LINEAR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_compumotor_linear_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] =
		"mxd_compumotor_linear_positive_limit_hit()";

	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	MX_COMPUMOTOR_LINEAR_MOTOR *compumotor_linear_motor;
	long i, num_motors;
	mx_bool_type limit_hit;
	mx_status_type mx_status;

	compumotor_linear_motor = NULL;

	mx_status = mxd_compumotor_linear_get_pointers( motor,
					&compumotor_linear_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = compumotor_linear_motor->num_motors;
	motor_record_array = compumotor_linear_motor->motor_record_array;

	limit_hit = FALSE;

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
mxd_compumotor_linear_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] =
		"mxd_compumotor_linear_negative_limit_hit()";

	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	MX_COMPUMOTOR_LINEAR_MOTOR *compumotor_linear_motor;
	long i, num_motors;
	mx_bool_type limit_hit;
	mx_status_type mx_status;

	compumotor_linear_motor = NULL;

	mx_status = mxd_compumotor_linear_get_pointers( motor,
					&compumotor_linear_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = compumotor_linear_motor->num_motors;
	motor_record_array = compumotor_linear_motor->motor_record_array;

	limit_hit = FALSE;

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
mxd_compumotor_linear_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_linear_find_home_position()";

	return mx_error( MXE_UNSUPPORTED, fname,
"Finding home for a Compumotor linear interpolation motor is _not_ allowed.  "
"Motor name = '%s'",
		motor->record->name );
}

