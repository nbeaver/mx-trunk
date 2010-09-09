/*
 * Name:    d_monochromator.c 
 *
 * Purpose: MX motor driver to control X-ray monochromators.  This is a
 *          pseudomotor that pretends to be the monochromator 'theta' axis.
 *          The value written to this pseudo motor is used to control the
 *          real 'theta' axis and any other MX devices that are tied into
 *          this pseudomotor.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2006, 2008-2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MONOCHROMATOR_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_variable.h"
#include "v_position_select.h"
#include "mx_array.h"
#include "mx_constants.h"
#include "mx_motor.h"
#include "mx_unistd.h"
#include "d_monochromator.h"
#include "d_mono_dependencies.h"

#if MXD_MONOCHROMATOR_DEBUG_TIMING
#include "mx_hrt_debug.h"
#endif

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_monochromator_record_function_list = {
	mxd_monochromator_initialize_type,
	mxd_monochromator_create_record_structures,
	mxd_monochromator_finish_record_initialization,
	NULL,
	mxd_monochromator_print_motor_structure,
	mxd_monochromator_open,
	NULL
};

MX_MOTOR_FUNCTION_LIST mxd_monochromator_motor_function_list = {
	NULL,
	mxd_monochromator_move_absolute,
	mxd_monochromator_get_position,
	mxd_monochromator_set_position,
	mxd_monochromator_soft_abort,
	mxd_monochromator_immediate_abort,
	NULL,
	NULL,
	NULL,
	mxd_monochromator_constant_velocity_move,
	mxd_monochromator_get_parameter,
	mxd_monochromator_set_parameter,
	NULL,
	mxd_monochromator_get_status
};

/* Monochromator motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_monochromator_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_MONOCHROMATOR_STANDARD_FIELDS
};

long mxd_monochromator_num_record_fields
		= sizeof( mxd_monochromator_record_field_defaults )
			/ sizeof( mxd_monochromator_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_monochromator_rfield_def_ptr
			= &mxd_monochromator_record_field_defaults[0];

/* === */

static MX_MONOCHROMATOR_FUNCTION_LIST mxd_monochromator_function_list[] = {
	{ mxd_monochromator_get_theta_position,
	  mxd_monochromator_set_theta_position,
	  mxd_monochromator_move_theta},
	{ mxd_monochromator_get_energy_position,
	  mxd_monochromator_set_energy_position,
	  mxd_monochromator_move_energy},
	{ mxd_monochromator_get_polynomial_position,
	  mxd_monochromator_set_polynomial_position,
	  mxd_monochromator_move_polynomial},
	{ mxd_monochromator_get_insertion_device_energy_position,
	  mxd_monochromator_set_insertion_device_energy_position,
	  mxd_monochromator_move_insertion_device_energy},
	{ mxd_monochromator_get_bragg_normal_position,
	  mxd_monochromator_set_bragg_normal_position,
	  mxd_monochromator_move_bragg_normal},
	{ mxd_monochromator_get_bragg_parallel_position,
	  mxd_monochromator_set_bragg_parallel_position,
	  mxd_monochromator_move_bragg_parallel},
	{ mxd_monochromator_get_table_position,
	  mxd_monochromator_set_table_position,
	  mxd_monochromator_move_table},
	{ mxd_monochromator_get_diffractometer_theta_position,
	  mxd_monochromator_set_diffractometer_theta_position,
	  mxd_monochromator_move_diffractometer_theta},
	{ mxd_monochromator_get_e_polynomial_position,
	  mxd_monochromator_set_e_polynomial_position,
	  mxd_monochromator_move_e_polynomial},
	{ mxd_monochromator_get_option_selector_position,
	  mxd_monochromator_set_option_selector_position,
	  mxd_monochromator_move_option_selector},
	{ mxd_monochromator_get_theta_fn_position,
	  mxd_monochromator_set_theta_fn_position,
	  mxd_monochromator_move_theta_fn},
	{ mxd_monochromator_get_energy_fn_position,
	  mxd_monochromator_set_energy_fn_position,
	  mxd_monochromator_move_energy_fn},
};

/************** Internal use only functions. *************/

static mx_status_type
mxd_monochromator_get_pointers( MX_MOTOR *motor,
				MX_MONOCHROMATOR **monochromator,
				const char *calling_fname )
{
	static const char fname[] = "mxd_monochromator_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MOTOR pointer passed by '%s' is NULL.",
			calling_fname );
	}
	if ( monochromator == (MX_MONOCHROMATOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MONOCHROMATOR indirect pointer passed by '%s' is NULL.",
			calling_fname);
	}
	if ( motor->record->mx_type != MXT_MTR_MONOCHROMATOR ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"Record '%s' is not a monochromator record.  Instead, it is of type %ld.",
			motor->record->name, motor->record->mx_type );
	}
	*monochromator = (MX_MONOCHROMATOR *)
				(motor->record->record_type_struct);

	if ( *monochromator == (MX_MONOCHROMATOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MONOCHROMATOR pointer for motor '%s' passed by '%s' is NULL.",
			motor->record->name, calling_fname );
	}
	if ( (*monochromator)->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"record pointer for monochromator record '%s' passed by '%s' is NULL.",
			motor->record->name, calling_fname );
	}
	if ( (*monochromator)->list_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"list_array pointer for monochromator record '%s' passed by '%s' is NULL.",
			motor->record->name, calling_fname );
	}
	if ( (*monochromator)->speed_change_permitted == (mx_bool_type *) NULL){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"speed_change_permitted pointer for monochromator record '%s' "
		"passed by '%s' is NULL.",
			motor->record->name, calling_fname );
	}
	if ( (*monochromator)->speed_changed == (mx_bool_type *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"speed_changed pointer for monochromator record '%s' passed by '%s' is NULL.",
			motor->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_monochromator_get_enable_status( MX_RECORD *list_record,
					mx_bool_type *enable_status )
{
	static const char fname[] = "mxd_monochromator_get_enable_status()";

	MX_RECORD **record_array;
	MX_RECORD *enable_status_record;
	void *pointer_to_value;
	mx_bool_type fast_mode;
	mx_status_type mx_status;

	mx_status = mx_get_variable_pointer( list_record, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_fast_mode( list_record, &fast_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	record_array = (MX_RECORD **) pointer_to_value;

	enable_status_record = record_array[MXFP_MONO_ENABLED];

	if ( fast_mode == FALSE ) {
		mx_status = mx_receive_variable( enable_status_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_get_variable_pointer( enable_status_record,
					&pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*enable_status = *( int * ) pointer_to_value;

	MX_DEBUG( 2,("%s: '%s' enable status = %d",
		fname, list_record->name, (int) *enable_status));

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_monochromator_get_dependency_type( MX_RECORD *list_record,
					long *dependency_type )
{
	MX_RECORD **record_array;
	MX_RECORD *dependency_type_record;
	void *pointer_to_value;
	mx_status_type mx_status;

	mx_status = mx_get_variable_pointer( list_record, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	record_array = (MX_RECORD **) pointer_to_value;

	dependency_type_record = record_array[MXFP_MONO_DEPENDENCY_TYPE];

	mx_status = mx_receive_variable( dependency_type_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_variable_pointer( dependency_type_record,
					&pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*dependency_type = *( int * ) pointer_to_value;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_monochromator_get_param_array_from_list( MX_RECORD *list_record,
					long *num_parameters,
					double **parameter_array )
{
	static const char fname[]
		= "mxd_monochromator_get_param_array_from_list()";

	MX_RECORD_FIELD *field;
	MX_RECORD **record_array;
	MX_RECORD *parameter_array_record;
	void *pointer_to_value;
	mx_status_type mx_status;

	mx_status = mx_get_variable_pointer( list_record, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	record_array = (MX_RECORD **) pointer_to_value;

	parameter_array_record = record_array[MXFP_MONO_PARAMETER_ARRAY];

	mx_status = mx_receive_variable( parameter_array_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field( parameter_array_record,
							"value", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pointer_to_value = mx_get_field_value_pointer( field );

	if ( pointer_to_value == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"Field value pointer is NULL for record field '%s' in record '%s'.",
			field->name, list_record->name );
	}

	*num_parameters = (int) (field->dimension[0]);

	*parameter_array = ( double * ) pointer_to_value;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_monochromator_get_record_array_from_list( MX_RECORD *list_record,
					long *num_records,
					MX_RECORD ***returned_record_array )
{
	static const char fname[]
		= "mxd_monochromator_get_record_array_from_list()";

	MX_RECORD_FIELD *field;
	MX_RECORD **record_array;
	MX_RECORD *record_array_record;
	void *pointer_to_value;
	mx_status_type mx_status;

	mx_status = mx_get_variable_pointer( list_record, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	record_array = (MX_RECORD **) pointer_to_value;

	record_array_record = record_array[MXFP_MONO_RECORD_ARRAY];

	mx_status = mx_find_record_field(record_array_record, "value", &field);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pointer_to_value = mx_get_field_value_pointer( field );

	if ( pointer_to_value == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"Field value pointer is NULL for record field '%s' in record '%s'.",
			field->name, list_record->name );
	}

	*num_records = field->dimension[0];

	*returned_record_array = ( MX_RECORD ** ) pointer_to_value;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_monochromator_check_for_quick_scan_ability( MX_RECORD *record,
					MX_RECORD *monochromator_record,
					MX_RECORD *list_record )
{
	static const char fname[]
		= "mxd_monochromator_check_for_quick_scan_ability()";

	MX_RECORD **record_array, *enable_status_record;
	MX_MOTOR *motor;
	void *pointer_to_value;
	int flags;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	flags = MXF_MTR_CANNOT_CHANGE_SPEED | MXF_MTR_CANNOT_QUICK_SCAN;

	if ( motor->motor_flags & flags ) {

		mx_status = mx_get_variable_pointer( list_record,
							&pointer_to_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		record_array = (MX_RECORD **) pointer_to_value;

		enable_status_record = record_array[MXFP_MONO_ENABLED];

		return mx_error( MXE_UNSUPPORTED, fname,
"The motor '%s' used by monochromator '%s' cannot quick scan but is being "
"asked to quick scan anyway.  To fix this, either set the enable variable '%s' "
"to 0 or disable synchronous motion mode.", record->name,
					monochromator_record->name,
					enable_status_record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_monochromator_restore_speeds( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_monochromator_restore_speeds()";

	MX_MONOCHROMATOR *monochromator;
	MX_RECORD *list_record;
	MX_RECORD **record_array;
	MX_RECORD *record;
	long i, j, num_records;
	mx_status_type mx_status;

	mx_status = mxd_monochromator_get_pointers( motor,
						&monochromator, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 0; i < monochromator->num_dependencies; i++ ) {

		MX_DEBUG( 2,("%s: monochromator->speed_changed[%ld] = %d",
			fname, i, (int) monochromator->speed_changed[i]));

		if ( monochromator->speed_changed[i] ) {

			list_record = monochromator->list_array[i];

			mx_status
			    = mxd_monochromator_get_record_array_from_list(
				list_record, &num_records, &record_array );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			MX_DEBUG( 2,
				("%s: list_record = '%s', num_records = %ld",
				fname, list_record->name, num_records));

			for ( j = 0; j < num_records; j++ ) {

				record = record_array[j];

				MX_DEBUG( 2,("%s: record_array[%ld] = '%s'",
					fname, j, record->name));

				if ( (record->mx_superclass == MXR_DEVICE)
				  && (record->mx_class == MXC_MOTOR) )
				{
					MX_DEBUG( 2,
				("%s: restoring motor speed for record '%s'",
						fname, record->name));

					(void) mx_motor_restore_speed( record );
				}
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/********************************************************/

#define NUM_MONO_FIELDS   3

MX_EXPORT mx_status_type
mxd_monochromator_initialize_type( long record_type )
{
	static const char fname[] = "mxd_monochromator_initialize_type()";

	const char field_name[NUM_MONO_FIELDS][MXU_FIELD_NAME_LENGTH+1]
	   = {
		"list_array",
		"speed_change_permitted",
		"speed_changed"
	     };

	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
	MX_RECORD_FIELD_DEFAULTS *field;
	int i;
	long num_record_fields;
	long num_dependencies_field_index;
	long num_dependencies_varargs_cookie;
	mx_status_type mx_status;

	driver = mx_get_driver_by_type( record_type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record type %ld not found.",
			record_type );
	}

	record_field_defaults_ptr
			= driver->record_field_defaults_ptr;

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
			"num_dependencies", &num_dependencies_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( num_dependencies_field_index,
				0, &num_dependencies_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 0; i < NUM_MONO_FIELDS; i++ ) {
		mx_status = mx_find_record_field_defaults(
			record_field_defaults, num_record_fields,
			field_name[i], &field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		field->dimension[0] = num_dependencies_varargs_cookie;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_monochromator_create_record_structures( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_monochromator_create_record_structures()";

	MX_MOTOR *motor;
	MX_MONOCHROMATOR *monochromator;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	monochromator = (MX_MONOCHROMATOR *) malloc( sizeof(MX_MOTOR) );

	if ( monochromator == (MX_MONOCHROMATOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_MONOCHROMATOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = monochromator;
	record->class_specific_function_list
				= &mxd_monochromator_motor_function_list;

	motor->record = record;
	monochromator->record = record;

	/* The monochromator is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_monochromator_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_monochromator_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_MONOCHROMATOR *monochromator;
	MX_RECORD *list_record;
	long i;
	long dependency_type = -1;
	mx_bool_type theta_dependency_found, energy_dependency_found;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_monochromator_get_pointers( motor,
						&monochromator, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->motor_flags |= MXF_MTR_IS_PSEUDOMOTOR;

	/* Find out whether we have a theta dependency
	 * or an energy dependency.
	 */

	theta_dependency_found = FALSE;
	energy_dependency_found = FALSE;

	for ( i = 0; i < monochromator->num_dependencies; i++ ) {

		list_record = monochromator->list_array[i];

		mx_status = mxd_monochromator_get_dependency_type( list_record,
							&dependency_type );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( dependency_type ) {
		case MXF_MONO_THETA:
			theta_dependency_found = TRUE;
			break;
		case MXF_MONO_ENERGY:
			energy_dependency_found = TRUE;
			break;
		}
	}

	if ( theta_dependency_found && energy_dependency_found )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Monochromator '%s' has both a theta dependency (type 0) and "
		"an energy dependency (type 1) defined.  You may not have "
		"both of them at the same time.  In all likelihood, you "
		"want to have a theta dependency.",  record->name );

	} else if ( ( theta_dependency_found  == FALSE )
		 && ( energy_dependency_found == FALSE ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Monochromator '%s' does not have either a theta dependency "
		"(type 0) or an energy dependency (type 1) defined.  You need "
		"to have one and only one of them defined in order to use "
		"this monochromator pseudomotor.  In all likelihood, you "
		"want to have a theta dependency.",  record->name );

	} else if ( theta_dependency_found ) {

		motor->motor_flags
			|= MXF_MTR_PSEUDOMOTOR_RECURSION_IS_NOT_NECESSARY;
	}

	/* Arrange for the value of motor->real_motor_record to be initialized
	 * by getting the MXLV_MTR_GET_REAL_MOTOR_FROM_PSEUDOMOTOR parameter.
	 */

	mx_status = mx_motor_get_parameter( record,
			MXLV_MTR_GET_REAL_MOTOR_FROM_PSEUDOMOTOR );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_monochromator_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_monochromator_print_motor_structure()";

	MX_MOTOR *motor;
	MX_MONOCHROMATOR *monochromator;
	double position, move_deadband, speed;
	long i, num_dependencies;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer for record '%s' is NULL.", record->name );
	}

	monochromator = (MX_MONOCHROMATOR *) (record->record_type_struct);

	if ( monochromator == (MX_MONOCHROMATOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MONOCHROMATOR pointer for record '%s' is NULL.",
			record->name );
	}

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type     = MONOCHROMATOR.\n\n");

	fprintf(file, "  name           = %s\n", record->name);

	num_dependencies = monochromator->num_dependencies;

	fprintf(file, "  list array     = (");

	if ( monochromator->list_array == NULL ) {
		fprintf(file, "NULL");
	} else {
		for ( i = 0; i < num_dependencies; i++ ) {
			fprintf(file, "%s",
				monochromator->list_array[i]->name);

			if ( i != (num_dependencies - 1) ) {
				fprintf(file, ", ");
			}
		}
	}
	fprintf(file, ")\n");

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	fprintf(file, "  position       = %g %s  (%g).\n",
		motor->position, motor->units,
		motor->raw_position.analog );
	fprintf(file, "  theta scale    = %g %s "
					"per unscaled theta unit.\n",
		motor->scale, motor->units);
	fprintf(file, "  theta offset   = %g %s.\n",
		motor->offset, motor->units);
	fprintf(file, "  backlash       = %g %s  (%g).\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog );
	fprintf(file, "  negative limit = %g %s  (%g).\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog );
	fprintf(file, "  positive limit = %g %s  (%g).\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog );

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband  = %g %s  (%g).\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	mx_status = mx_motor_get_speed( record, &speed );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to get the speed of motor '%s'",
			record->name );
	}

	fprintf(file, "  speed          = %g %s/sec\n\n",
		speed, motor->units );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_monochromator_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_monochromator_open()";

	MX_MONOCHROMATOR *monochromator;
	MX_RECORD *list_record, *dependent_record;
	MX_RECORD **record_array;
	MX_MOTOR *motor, *dependent_motor;
	long i, j, num_records, flags;
	mx_bool_type speed_change_permitted;
	mx_status_type mx_status;

#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_monochromator_get_pointers( motor,
						&monochromator, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;


#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_DEBUG(-2,("%s: about to sleep for 100 usec.", fname));

	MX_HRT_START( measurement );
	mx_usleep(100);
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, motor->record->name );
#endif

	

	monochromator->move_in_progress = FALSE;

	/* Make sure that the speed change flags are initialized. */

	for ( i = 0; i < monochromator->num_dependencies; i++ ) {
		list_record = monochromator->list_array[i];

		mx_status = mxd_monochromator_get_record_array_from_list(
				list_record, &num_records, &record_array );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		speed_change_permitted = TRUE;

		for ( j = 0; j < num_records; j++ ) {

			dependent_record = record_array[j];

			if ( (dependent_record->mx_superclass == MXR_DEVICE)
			  && (dependent_record->mx_class == MXC_MOTOR) )
			{
				dependent_motor = (MX_MOTOR *)
					dependent_record->record_class_struct;

				flags = MXF_MTR_CANNOT_CHANGE_SPEED
						| MXF_MTR_CANNOT_QUICK_SCAN;

				if ( dependent_motor->motor_flags & flags ) {
					speed_change_permitted = FALSE;
				}
			}
		}
		monochromator->speed_change_permitted[i]
						= speed_change_permitted;

		monochromator->speed_changed[i] = FALSE;

	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_monochromator_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_monochromator_move_absolute()";

	MX_MONOCHROMATOR *monochromator;
	MX_RECORD *list_record;
	MX_MONOCHROMATOR_FUNCTION_LIST *flist;
	mx_status_type (*fptr)( MX_MONOCHROMATOR *, MX_RECORD *, long,
					double, double, int );
	double raw_destination, old_raw_position, dummy;
	long i;
	long dependency_type = -1;
	mx_status_type mx_status;

#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif

	mx_status = mxd_monochromator_get_pointers( motor,
						&monochromator, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	raw_destination = motor->raw_destination.analog;

	/* Get the old position. */

	mx_status = mx_motor_get_position( motor->record, &dummy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	old_raw_position = motor->raw_position.analog;

	/* Check the software limits for each of the dependencies. */

	MX_DEBUG( 2,("%s: ***** Checking software limits.", fname));

	for ( i = 0; i < monochromator->num_dependencies; i++ ) {

		list_record = monochromator->list_array[i];

		if ( list_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"Record pointer for element %ld of list_array in monochromator '%s' is NULL.",
				i, motor->record->name );
		}

		mx_status = mxd_monochromator_get_dependency_type( list_record,
							&dependency_type );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		flist = &mxd_monochromator_function_list[ dependency_type ];

		if ( flist == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"Monochromator function list entry for dependency type %ld is NULL.",
				dependency_type );
		}

		fptr = flist->move;

		if ( fptr == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"move function pointer for monochromator dependency type %ld is NULL.",
				dependency_type );
		}

		mx_status = (*fptr)( monochromator, list_record, i,
					raw_destination, old_raw_position,
					MXF_MTR_ONLY_CHECK_LIMITS );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Now start the move for each of the dependencies. */

	MX_DEBUG( 2,("%s: ***** Now starting the move.", fname));

	MX_DEBUG( 2,("%s: motor->synchronous_motion_mode = %d",
			fname, (int) motor->synchronous_motion_mode));

	monochromator->move_in_progress = TRUE;

	for ( i = 0; i < monochromator->num_dependencies; i++ ) {

		/* Having tested for NULL pointers on the first pass, we
		 * do not need to test for them the second time through.
		 */

		list_record = monochromator->list_array[i];

		mx_status = mxd_monochromator_get_dependency_type( list_record,
							&dependency_type );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		flist = &mxd_monochromator_function_list[ dependency_type ];

		fptr = flist->move;

		mx_status = (*fptr)( monochromator, list_record, i,
					raw_destination, old_raw_position, 0 );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mxd_monochromator_soft_abort( motor );

			return mx_status;
		}
	}

#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, motor->record->name );
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_monochromator_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_monochromator_get_position()";

	MX_MONOCHROMATOR *monochromator;
	MX_RECORD *list_record;
	MX_MONOCHROMATOR_FUNCTION_LIST *flist;
	mx_status_type (*fptr)(MX_MONOCHROMATOR *, MX_RECORD *, long, double *);
	double position;
	long dependency_type = -1;
	mx_status_type mx_status;

#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif

	mx_status = mxd_monochromator_get_pointers( motor,
						&monochromator, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the position using the first entry in the list array. */

	list_record = monochromator->list_array[0];

	if ( list_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"First record pointer for list_array in monochromator '%s' is NULL.",
			motor->record->name );
	}

	mx_status = mxd_monochromator_get_dependency_type( list_record,
							&dependency_type );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	flist = &mxd_monochromator_function_list[ dependency_type ];

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"Monochromator function list entry for dependency type %ld is NULL.",
			dependency_type );
	}

	fptr = flist->get_position;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"get_position pointer for monochromator dependency type %ld is NULL.",
			dependency_type );
	}

	mx_status = (*fptr)( monochromator, list_record, 0, &position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->raw_position.analog = position;

#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, motor->record->name );
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_monochromator_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_monochromator_set_position()";

	MX_MONOCHROMATOR *monochromator;
	MX_RECORD *list_record;
	MX_MONOCHROMATOR_FUNCTION_LIST *flist;
	mx_status_type (*fptr)( MX_MONOCHROMATOR *, MX_RECORD *, long, double );
	double raw_set_position;
	long dependency_type = -1;
	mx_status_type mx_status;

#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif

	mx_status = mxd_monochromator_get_pointers( motor,
						&monochromator, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	raw_set_position = motor->raw_set_position.analog;

	/* Set the position using the first entry in the list array. */

	list_record = monochromator->list_array[0];

	if ( list_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"First record pointer for list_array in monochromator '%s' is NULL.",
			motor->record->name );
	}

	mx_status = mxd_monochromator_get_dependency_type( list_record,
							&dependency_type );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	flist = &mxd_monochromator_function_list[ dependency_type ];

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"Monochromator function list entry for dependency type %ld is NULL.",
			dependency_type );
	}

	fptr = flist->set_position;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"get_position pointer for monochromator dependency type %ld is NULL.",
			dependency_type );
	}

	mx_status = (*fptr)( monochromator, list_record, 0, raw_set_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, motor->record->name );
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_monochromator_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_monochromator_soft_abort()";

	MX_MONOCHROMATOR *monochromator;
	MX_RECORD *list_record;
	MX_RECORD **record_array;
	MX_RECORD *record;
	long i, j, num_records;
	mx_status_type mx_status;

#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif

	MX_DEBUG( 2,("%s invoked.",fname));

	mx_status = mxd_monochromator_get_pointers( motor,
						&monochromator, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 0; i < monochromator->num_dependencies; i++ ) {
		list_record = monochromator->list_array[i];

		mx_status = mxd_monochromator_get_record_array_from_list(
				list_record, &num_records, &record_array );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		for ( j = 0; j < num_records; j++ ) {

			record = record_array[j];

			if ( (record->mx_superclass == MXR_DEVICE)
			  && (record->mx_class == MXC_MOTOR) )
			{
				(void) mx_motor_soft_abort( record );
			}
		}
	}

	monochromator->move_in_progress = FALSE;

	(void) mxd_monochromator_restore_speeds( motor );

#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, motor->record->name );
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_monochromator_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_monochromator_immediate_abort()";

	MX_MONOCHROMATOR *monochromator;
	MX_RECORD *list_record;
	MX_RECORD **record_array;
	MX_RECORD *record;
	long i, j, num_records;
	mx_status_type mx_status;

#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif

	MX_DEBUG( 2,("%s invoked.",fname));

	mx_status = mxd_monochromator_get_pointers( motor,
						&monochromator, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 0; i < monochromator->num_dependencies; i++ ) {
		list_record = monochromator->list_array[i];

		mx_status = mxd_monochromator_get_record_array_from_list(
				list_record, &num_records, &record_array );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		for ( j = 0; j < num_records; j++ ) {

			record = record_array[j];

			if ( (record->mx_superclass == MXR_DEVICE)
			  && (record->mx_class == MXC_MOTOR) )
			{
				(void) mx_motor_immediate_abort( record );
			}
		}
	}

#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, motor->record->name );
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_monochromator_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[]
		= "mxd_monochromator_constant_velocity_move()";

	MX_MONOCHROMATOR *monochromator;
	MX_RECORD *list_record;
	MX_RECORD **record_array;
	MX_RECORD *record;
	long num_records;
	mx_status_type mx_status;

#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif

	mx_status = mxd_monochromator_get_pointers( motor,
						&monochromator, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the first entry in the list array. */

	list_record = monochromator->list_array[0];

	if ( list_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"First record pointer for list_array in monochromator '%s' is NULL.",
			motor->record->name );
	}

	mx_status = mxd_monochromator_get_record_array_from_list(
				list_record, &num_records, &record_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_records <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The first dependency for monochromator motor '%s' has no associated records.",
			motor->record->name );
	}

	/* Get the first entry in the record array. */

	record = record_array[0];

	if ( (record->mx_superclass != MXR_DEVICE)
	  || (record->mx_class != MXC_MOTOR) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The first record '%s' in the first dependency list for "
		"monochromator motor '%s' is not a motor.",
			record->name, motor->record->name );
	}

	/* Start the move using this record. */

	mx_status = mx_motor_constant_velocity_move( record,
				motor->constant_velocity_move );

#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, motor->record->name );
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_monochromator_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_monochromator_get_parameter()";

	MX_MONOCHROMATOR *monochromator;
	MX_RECORD *list_record;
	MX_RECORD **record_array;
	MX_RECORD *theta_record;
	long num_records;
	double double_value;
	double start_position, end_position;
	double real_start_position, real_end_position;
	mx_status_type mx_status;

#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif

	mx_status = mxd_monochromator_get_pointers( motor,
						&monochromator, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: motor '%s', parameter_type = %ld",
		fname, motor->record->name, motor->parameter_type));

	/* Get the first entry in the list array. */

	list_record = monochromator->list_array[0];

	if ( list_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"First record pointer for list_array in monochromator '%s' is NULL.",
			motor->record->name );
	}

	mx_status = mxd_monochromator_get_record_array_from_list(
				list_record, &num_records, &record_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_records <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The first dependency for monochromator motor '%s' has no associated records.",
			motor->record->name );
	}

	/* Get the first entry in the record array. */

	theta_record = record_array[0];

	if ( (theta_record->mx_superclass != MXR_DEVICE)
	  || (theta_record->mx_class != MXC_MOTOR) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The first record '%s' in the first dependency list for "
		"monochromator motor '%s' is not a motor.",
			theta_record->name, motor->record->name );
	}

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status = mx_motor_get_speed( theta_record, &double_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_speed = double_value;
		break;

	case MXLV_MTR_BASE_SPEED:
		mx_status = mx_motor_get_base_speed( theta_record,
							&double_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_base_speed = double_value;
		break;

	case MXLV_MTR_MAXIMUM_SPEED:
		mx_status = mx_motor_get_maximum_speed( theta_record,
							&double_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_maximum_speed = double_value;
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		mx_status = mx_motor_get_raw_acceleration_parameters(
					theta_record,
					motor->raw_acceleration_parameters );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->acceleration_time = double_value;
		break;

	case MXLV_MTR_ACCELERATION_TIME:
		mx_status = mx_motor_get_acceleration_time( theta_record,
							&double_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->acceleration_time = double_value;
		break;

	case MXLV_MTR_ACCELERATION_DISTANCE:
		mx_status = mx_motor_get_acceleration_distance( theta_record,
							&double_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->acceleration_distance = double_value;
		break;

	case MXLV_MTR_COMPUTE_EXTENDED_SCAN_RANGE:
		start_position = motor->raw_compute_extended_scan_range[0];
		end_position = motor->raw_compute_extended_scan_range[1];

		mx_status = mx_motor_compute_extended_scan_range( theta_record,
							start_position,
							end_position,
							&real_start_position,
							&real_end_position );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_compute_extended_scan_range[2] = real_start_position;
		motor->raw_compute_extended_scan_range[3] = real_end_position;
		break;

	case MXLV_MTR_COMPUTE_PSEUDOMOTOR_POSITION:
		motor->compute_pseudomotor_position[1]
				= motor->compute_pseudomotor_position[0];
		break;

	case MXLV_MTR_COMPUTE_REAL_POSITION:
		motor->compute_real_position[1]
				= motor->compute_real_position[0];
		break;

	case MXLV_MTR_GET_REAL_MOTOR_FROM_PSEUDOMOTOR:
		motor->real_motor_record = theta_record;
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Parameter type '%s' (%ld) is not supported by the "
			"monochromator driver for motor '%s'.",
			mx_get_field_label_string( motor->record,
						motor->parameter_type ),
			motor->parameter_type,
			motor->record->name );
	}

#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, motor->record->name );
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_monochromator_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_monochromator_set_parameter()";

	MX_MONOCHROMATOR *monochromator;
	MX_RECORD *list_record;
	MX_RECORD **record_array;
	MX_RECORD *theta_record;
	long num_records;
	double start_position, end_position, time_for_move;
	mx_status_type mx_status;

#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif

	mx_status = mxd_monochromator_get_pointers( motor,
						&monochromator, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: motor '%s', parameter_type = %ld",
		fname, motor->record->name, motor->parameter_type));

	/* Get the first entry in the list array. */

	list_record = monochromator->list_array[0];

	if ( list_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"First record pointer for list_array in monochromator '%s' is NULL.",
			motor->record->name );
	}

	mx_status = mxd_monochromator_get_record_array_from_list(
				list_record, &num_records, &record_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_records <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The first dependency for monochromator motor '%s' has no associated records.",
			motor->record->name );
	}

	/* Get the first entry in the record array. */

	theta_record = record_array[0];

	if ( (theta_record->mx_superclass != MXR_DEVICE)
	  || (theta_record->mx_class != MXC_MOTOR) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The first record '%s' in the first dependency list for "
		"monochromator motor '%s' is not a motor.",
			theta_record->name, motor->record->name );
	}

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status = mx_motor_set_speed( theta_record, motor->raw_speed );
		break;

	case MXLV_MTR_BASE_SPEED:
		mx_status = mx_motor_set_base_speed( theta_record,
						motor->raw_base_speed );
		break;

	case MXLV_MTR_MAXIMUM_SPEED:
		mx_status = mx_motor_set_maximum_speed( theta_record,
						motor->raw_maximum_speed );
		break;

	case MXLV_MTR_SAVE_SPEED:
		mx_status = mx_motor_save_speed( theta_record );
		break;

	case MXLV_MTR_RESTORE_SPEED:
		mx_status = mx_motor_restore_speed( theta_record );
		break;

	case MXLV_MTR_SPEED_CHOICE_PARAMETERS:
		start_position = motor->raw_speed_choice_parameters[0];
		end_position = motor->raw_speed_choice_parameters[1];
		time_for_move = motor->raw_speed_choice_parameters[2];

		mx_status = mx_motor_set_speed_between_positions( theta_record,
							start_position,
							end_position,
							time_for_move );
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Parameter type '%s' (%ld) is not supported by the "
			"monochromator driver for motor '%s'.",
			mx_get_field_label_string( motor->record,
						motor->parameter_type ),
			motor->parameter_type,
			motor->record->name );
	}

#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, motor->record->name );
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_monochromator_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_monochromator_get_status()";

	MX_MONOCHROMATOR *monochromator;
	MX_POSITION_SELECT *position_select;
	MX_RECORD *list_record;
	MX_RECORD **record_array;
	MX_RECORD *record;
	long i, j, num_records, dependency_type;
	unsigned long motor_status;
	mx_bool_type enabled;
	mx_status_type mx_status;

#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif
	dependency_type = -1;

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor->record->name));

	mx_status = mxd_monochromator_get_pointers( motor,
						&monochromator, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->status = 0;

	for ( i = 0; i < monochromator->num_dependencies; i++ ) {
	    list_record = monochromator->list_array[i];

	    mx_status = mxd_monochromator_get_enable_status(
						list_record, &enabled );

	    if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	    if ( enabled ) {
		mx_status = mxd_monochromator_get_record_array_from_list(
				list_record, &num_records, &record_array );

		if ( mx_status.code != MXE_SUCCESS )
		    return mx_status;

		mx_status = mxd_monochromator_get_dependency_type( list_record,
							&dependency_type );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		for ( j = 0; j < num_records; j++ ) {

		    record = record_array[j];

		    if ( dependency_type == MXF_MONO_OPTION_SELECTOR ) {
#if 0
			if ( ( j == 0 )
			  && ( record->mx_type == MXV_CAL_POSITION_SELECT ) )
#else
			if ( record->mx_type == MXV_CAL_POSITION_SELECT )
#endif
			{
			    /* For this record type, we really want to check
			     * the motor record attached to the position select
			     * variable record.
			     */

			    position_select = (MX_POSITION_SELECT *)
				    record->record_type_struct;

			    record = position_select->motor_record;
			}
		    }

		    if ( (record->mx_superclass == MXR_DEVICE)
		      && (record->mx_class == MXC_MOTOR) )
		    {
			mx_status = mx_motor_get_status(record, &motor_status);

			if ( mx_status.code != MXE_SUCCESS )
			    return mx_status;

			if ( motor_status & MXSF_MTR_IS_BUSY ) {
			    motor->status |= MXSF_MTR_IS_BUSY;
			}
			/* The monochromator pseudomotor geometry can be
			 * nonintuitive, so if one limit flag is set, we
			 * set both limit flags.
			 */
			if ( motor_status & MXSF_MTR_POSITIVE_LIMIT_HIT ) {
			    motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
			    motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
			}
			if ( motor_status & MXSF_MTR_NEGATIVE_LIMIT_HIT ) {
			    motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
			    motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
			}
			/* The home search flag is explicitly ignored. */

			if ( motor_status & MXSF_MTR_FOLLOWING_ERROR ) {
			    motor->status |= MXSF_MTR_FOLLOWING_ERROR;
			}
			if ( motor_status & MXSF_MTR_DRIVE_FAULT ) {
			    motor->status |= MXSF_MTR_DRIVE_FAULT;
			}
			if ( motor_status & MXSF_MTR_AXIS_DISABLED ) {
			    motor->status |= MXSF_MTR_AXIS_DISABLED;
			}
		    }
		}
	    }
	}

	mx_motor_set_traditional_status( motor );

	if ( ( motor->busy == FALSE ) && monochromator->move_in_progress ) {

		monochromator->move_in_progress = FALSE;

		(void) mxd_monochromator_restore_speeds( motor );
	}

	MX_DEBUG( 2,("%s returning.", fname));

#if MXD_MONOCHROMATOR_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, motor->record->name );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/****************** Private function definitions. *****************/

static mx_status_type
mxd_monochromator_get_theta_position( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double *theta )
{
	MX_RECORD **theta_record_array;
	MX_RECORD *theta_record;
	long num_records;
	mx_status_type mx_status;

	mx_status = mxd_monochromator_get_record_array_from_list(
			list_record, &num_records, &theta_record_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	theta_record = theta_record_array[0];

	mx_status = mx_motor_get_position( theta_record, theta );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_monochromator_set_theta_position( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta )
{
	MX_RECORD **theta_record_array;
	MX_RECORD *theta_record;
	long num_records;
	mx_status_type mx_status;

	mx_status = mxd_monochromator_get_record_array_from_list(
			list_record, &num_records, &theta_record_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	theta_record = theta_record_array[0];

	mx_status = mx_motor_set_position( theta_record, theta );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_monochromator_move_theta( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta,
					double old_theta,
					int flags )
{
	static const char fname[] = "mxd_monochromator_move_theta()";

	MX_RECORD **theta_record_array;
	MX_RECORD *theta_record;
	long num_records;
	double theta_speed;
	mx_bool_type enabled;
	mx_status_type mx_status;

	/* Is this dependency enabled? */

	mx_status = mxd_monochromator_get_enable_status( list_record, &enabled);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( enabled == FALSE )
		return MX_SUCCESSFUL_RESULT;

	mx_status = mxd_monochromator_get_record_array_from_list(
			list_record, &num_records, &theta_record_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	theta_record = theta_record_array[0];

	/* Get the motor speed so that we can compute the move time. */

	mx_status = mx_motor_get_speed( theta_record, &theta_speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	monochromator->move_time = mx_divide_safely( theta - old_theta,
							theta_speed );

	/* We are enabled, so move the motor. */

	if ( flags & MXF_MTR_ONLY_CHECK_LIMITS ) {
		MX_DEBUG( 2,("%s: Checking '%s' limits for %g",
				fname, theta_record->name, theta));

		mx_status = mx_motor_check_position_limits(
						theta_record, theta, flags );
	} else {
		MX_DEBUG( 2,("%s: Moving '%s' to %g",
				fname, theta_record->name, theta));

		mx_status = mx_motor_move_absolute( theta_record, theta, flags);
	}

	return mx_status;
}

/******************************************************************/

static mx_status_type
mxd_mono_get_energy_pointers( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					MX_RECORD **energy_record,
					double *radians_per_engineering_unit,
					double *d_spacing )
{
	static const char fname[] = "mxd_mono_get_energy_pointers()";

	MX_RECORD **record_array;
	double *parameter_array;
	long num_records, num_parameters, num_elements;
	void *pointer_to_value;
	mx_status_type mx_status;

	mx_status = mxd_monochromator_get_param_array_from_list(
			list_record, &num_parameters, &parameter_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_parameters != 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The parameter array for list record '%s' does not have 1 parameter.  "
"Instead, it has %ld parameters.", list_record->name, num_parameters );
	}

	*radians_per_engineering_unit = parameter_array[0];

#if 0
	if ( fabs(*radians_per_engineering_unit) < 1.0e-100 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The first element (radians per engineering unit) of the parameter array for "
"list record '%s' is too small.  Current value = %g",
			list_record->name, *radians_per_engineering_unit );
	}
#endif

	mx_status = mxd_monochromator_get_record_array_from_list(
			list_record, &num_records, &record_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_records != 2 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The record array for list record '%s' should have 2 records in it.  "
	"Instead there are %ld records in it.",
			list_record->name, num_records );
	}

	/* The first record is the energy motor record. */

	*energy_record = record_array[0];

	/* The second record in the record array is the monochromator
	 * crystal d spacing.
	 */

	mx_status = mx_get_1d_array( record_array[1], MXFT_DOUBLE,
					&num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*d_spacing = *( double *) pointer_to_value;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_monochromator_get_energy_position( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double *theta )
{
	MX_RECORD *energy_record;
	double energy, d_spacing, sin_theta, denominator;
	double angle_in_radians, radians_per_engineering_unit;
	mx_status_type mx_status;

	mx_status = mxd_mono_get_energy_pointers( monochromator,
			list_record, &energy_record,
			&radians_per_engineering_unit,
			&d_spacing );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_position( energy_record, &energy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	denominator = 2.0 * d_spacing * energy;

	if ( fabs(denominator) >= 1.0 ) {
		sin_theta = MX_HC / denominator;
	} else {
		if ( MX_HC < fabs(denominator * DBL_MAX) ) {
			sin_theta = MX_HC / denominator;
		} else {
			sin_theta = DBL_MAX;
		}
	}

	if ( fabs(sin_theta) > 1.0 ) {
		angle_in_radians = MX_PI / 2.0;
	} else {
		angle_in_radians = asin( sin_theta );
	}

	*theta = mx_divide_safely( angle_in_radians,
				radians_per_engineering_unit );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_monochromator_set_energy_position( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta )
{
	MX_RECORD *energy_record;
	double energy, d_spacing, denominator;
	double angle_in_radians, radians_per_engineering_unit;
	mx_status_type mx_status;

	mx_status = mxd_mono_get_energy_pointers( monochromator,
			list_record, &energy_record,
			&radians_per_engineering_unit,
			&d_spacing );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the energy. */

	angle_in_radians = theta * radians_per_engineering_unit;

	denominator = 2.0 * d_spacing * sin( angle_in_radians );

	if ( fabs(denominator) >= 1.0 ) {
		energy = MX_HC / denominator;
	} else {
		if ( MX_HC < fabs(denominator * DBL_MAX) ) {
			energy = MX_HC / denominator;
		} else {
			energy = DBL_MAX;
		}
	}

	/* Reset the monochromator energy. */

	mx_status = mx_motor_set_position( energy_record, energy );

	return mx_status;
}

static mx_status_type
mxd_monochromator_move_energy( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta,
					double old_theta,
					int flags )
{
	static const char fname[] = "mxd_monochromator_move_energy()";

	MX_RECORD *energy_record;
	double energy, old_energy, d_spacing, denominator;
	double angle_in_radians, old_angle_in_radians;
	double energy_speed, radians_per_engineering_unit;
	mx_bool_type enabled;
	mx_status_type mx_status;

	mx_status = mxd_monochromator_get_enable_status( list_record, &enabled);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( enabled == FALSE )
		return MX_SUCCESSFUL_RESULT;

	mx_status = mxd_mono_get_energy_pointers( monochromator,
			list_record, &energy_record,
			&radians_per_engineering_unit,
			&d_spacing );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the old energy. */

	old_angle_in_radians = old_theta * radians_per_engineering_unit;

	denominator = 2.0 * d_spacing * sin( old_angle_in_radians );

	old_energy = mx_divide_safely( MX_HC, denominator );

	/* Compute the new energy. */

	angle_in_radians = theta * radians_per_engineering_unit;

	denominator = 2.0 * d_spacing * sin( angle_in_radians );

	energy = mx_divide_safely( MX_HC, denominator );

	/* Get the current speed in energy units.  This is probably just
	 * an instantaneous approximation.
	 */

	mx_status = mx_motor_get_speed( energy_record, &energy_speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute a rough approximation of the move time. */

	monochromator->move_time = mx_divide_safely( old_energy,
				energy_speed * tan( old_angle_in_radians ) );

	/* Move the energy motor to the requested position. */

	if ( flags & MXF_MTR_ONLY_CHECK_LIMITS ) {
		MX_DEBUG( 2,("%s: Checking '%s' limits for %g",
				fname, energy_record->name, energy));

		mx_status = mx_motor_check_position_limits(
					energy_record, energy, flags );
	} else {
		MX_DEBUG( 2,("%s: Moving '%s' to %g",
				fname, energy_record->name, energy));

		mx_status = mx_motor_move_absolute( energy_record,
							energy, flags );
	}

	return mx_status;
}

/******************************************************************/

static mx_status_type
mxd_monochromator_get_polynomial_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double *theta )
{
	return MX_SUCCESSFUL_RESULT;
}
static mx_status_type
mxd_monochromator_set_polynomial_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta )
{
	return MX_SUCCESSFUL_RESULT;
}
static mx_status_type
mxd_monochromator_move_polynomial( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta,
					double old_theta,
					int flags )
{
	static const char fname[] = "mxd_monochromator_move_polynomial()";

	MX_RECORD **record_array;
	MX_RECORD *dependent_motor_record;
	double *parameter_array;
	long i, num_records, num_parameters;
	double dependent_value;
	mx_bool_type enabled;
	mx_status_type mx_status;

	mx_status = mxd_monochromator_get_enable_status( list_record, &enabled);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( enabled == FALSE )
		return MX_SUCCESSFUL_RESULT;

	mx_status = mxd_monochromator_get_param_array_from_list(
			list_record, &num_parameters, &parameter_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_monochromator_get_record_array_from_list(
			list_record, &num_records, &record_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_records != 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The record array for list record '%s' should have 1 record in it.  "
	"Instead there are %ld records in it.",
			list_record->name, num_records );
	}

	/* This record should be the dependent motor record. */

	if ( (record_array[0]->mx_superclass != MXR_DEVICE)
	  || (record_array[0]->mx_class != MXC_MOTOR) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The first record '%s' in the record array for list record '%s' "
	"is not a motor record.",
			record_array[0]->name, list_record->name );
	}

	dependent_motor_record = record_array[0];

	/* Compute the dependent motor position and move the motor there. */

	dependent_value = parameter_array[ num_parameters - 1 ];

	for ( i = num_parameters - 2; i >= 0; i-- ) {
		dependent_value = parameter_array[i] + theta * dependent_value;
	}

	if ( flags & MXF_MTR_ONLY_CHECK_LIMITS ) {
		MX_DEBUG( 2,("%s: Checking '%s' limits for %g",
			fname, dependent_motor_record->name, dependent_value));
	} else {
		MX_DEBUG( 2,("%s: Moving '%s' to %g",
			fname, dependent_motor_record->name, dependent_value));
	}

	mx_status = mx_motor_move_absolute( dependent_motor_record,
						dependent_value, flags );

	return mx_status;
}

/******************************************************************/

static mx_status_type
mxd_monochromator_get_insertion_device_energy_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double *theta_position )
{
	return MX_SUCCESSFUL_RESULT;
}
static mx_status_type
mxd_monochromator_set_insertion_device_energy_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta )
{
	return MX_SUCCESSFUL_RESULT;
}
static mx_status_type
mxd_monochromator_move_insertion_device_energy(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta,
					double old_theta,
					int flags )
{
	static const char fname[]
		= "mxd_monochromator_move_insertion_device_energy()";

	MX_RECORD **record_array;
	MX_RECORD *id_motor_record;
	double *parameter_array;
	long num_parameters, num_records, num_elements, gap_harmonic;
	double gap_offset, d_spacing, denominator, mono_energy, id_energy;
	void *pointer_to_value;
	mx_bool_type enabled;
	mx_status_type mx_status;

	mx_status = mxd_monochromator_get_enable_status( list_record, &enabled);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( enabled == FALSE )
		return MX_SUCCESSFUL_RESULT;

	mx_status = mxd_monochromator_get_param_array_from_list(
			list_record, &num_parameters, &parameter_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_parameters != 2 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The parameter array for list record '%s' does not have 2 parameters.  "
"Instead, it has %ld parameters.", list_record->name, num_parameters );
	}

	gap_harmonic = mx_round( parameter_array[0] );

	if ( gap_harmonic <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal gap harmonic number %ld.  It should be greater than "
		"or equal to 1.", gap_harmonic );
	}

	gap_offset = parameter_array[1];

	mx_status = mxd_monochromator_get_record_array_from_list(
			list_record, &num_records, &record_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_records != 2 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The record array for list record '%s' does not have 2 records.  "
	"Instead, it has %ld records.", list_record->name, num_records );
	}

	/* The first record is the insertion device motor record. */

	id_motor_record = record_array[0];

	/* The second record contains the value of the monochromator
	 * crystal d-spacing.
	 */

	mx_status = mx_get_1d_array( record_array[1], MXFT_DOUBLE,
					&num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	d_spacing = *( double * ) pointer_to_value;

	/* Compute the insertion device energy to request. */

	denominator = 2.0 * d_spacing * sin( theta * MX_RADIANS_PER_DEGREE );

	mono_energy = mx_divide_safely( MX_HC, denominator );

	id_energy = ( mono_energy + gap_offset ) / (double) gap_harmonic;

	/* Move the insertion device. */

	if ( flags & MXF_MTR_ONLY_CHECK_LIMITS ) {
		MX_DEBUG( 2,("%s: Checking '%s' limits for %g",
				fname, id_motor_record->name, id_energy));
	} else {
		MX_DEBUG( 2,("%s: Moving '%s' to %g",
				fname, id_motor_record->name, id_energy));
	}

	mx_status = mx_motor_move_absolute( id_motor_record, id_energy, flags );

	return mx_status;
}

/******************************************************************/

/* The following function is used by the Bragg normal motor, the Bragg
 * parallel motor, and the table motor below.
 */

static mx_status_type
mxd_monochromator_get_double_crystal_parameters(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					MX_MOTOR **monochromator_motor,
					MX_RECORD **dependent_motor_record,
					long num_parameters,
					double *tuning_parameters )
{
	static const char fname[]
		= "mxd_monochromator_get_double_crystal_parameters()";

	MX_RECORD **record_array;
	long i, num_variable_elements, num_records;
	void *pointer_to_value;
	double *variable_array;
	mx_status_type mx_status;

	if ( monochromator == (MX_MONOCHROMATOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MONOCHROMATOR pointer passed was NULL." );
	}
	if ( monochromator->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MONOCHROMATOR pointer %p was NULL.",
			monochromator );
	}

	if ( monochromator_motor != (MX_MOTOR **) NULL ) {
		*monochromator_motor = (MX_MOTOR *)
				monochromator->record->record_class_struct;

		if ( *monochromator_motor == (MX_MOTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
			"The MX_MOTOR pointer for record '%s' is NULL.",
				monochromator->record->name );
		}
	}

	mx_status = mxd_monochromator_get_record_array_from_list(
			list_record, &num_records, &record_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_records != 2 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The record array for list record '%s' should have 2 records in it.  "
	"Instead there are %ld records in it.",
			list_record->name, num_records );
	}

	/* The first record is the dependent motor record. */

	if ( (record_array[0]->mx_superclass != MXR_DEVICE)
	  || (record_array[0]->mx_class != MXC_MOTOR) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The first record '%s' in the record array for list record '%s' "
	"is not a motor record.",
			record_array[0]->name, list_record->name );
	}

	if ( dependent_motor_record != (MX_RECORD **) NULL ) {
		*dependent_motor_record = record_array[0];
	}

	/* The second record should be a variable which contains
	 * the desired tuning parameters.
	 */

	mx_status = mx_get_1d_array( record_array[1], MXFT_DOUBLE,
				&num_variable_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_variable_elements != num_parameters ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The monochromator pseudomotor '%s' expects the parameter variable '%s' "
"to have %ld elements, but it actually has %ld elements.",
			monochromator->record->name,
			record_array[1]->name,
			num_parameters,
			num_variable_elements );
	}

	if ( tuning_parameters != (double *) NULL ) {

		variable_array = (double *) pointer_to_value;

		for ( i = 0; i < num_parameters; i++ ) {
			tuning_parameters[i] = variable_array[i];
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/******************************************************************/

static mx_status_type
mxd_monochromator_get_bragg_normal_position( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double *theta )
{
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_monochromator_set_bragg_normal_position( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta )
{
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_monochromator_move_bragg_normal( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta,
					double old_theta,
					int flags )
{
	static const char fname[] = "mxd_monochromator_move_bragg_normal()";

	MX_MOTOR *monochromator_motor;
	MX_RECORD *normal_record;
	double old_bragg_normal, new_bragg_normal, beam_offset, denominator;
	mx_bool_type enabled;
	mx_status_type mx_status;

	mx_status = mxd_monochromator_get_enable_status( list_record, &enabled);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( enabled == FALSE )
		return MX_SUCCESSFUL_RESULT;

	mx_status = mxd_monochromator_get_double_crystal_parameters(
					monochromator, list_record,
					&monochromator_motor, &normal_record,
					1, &beam_offset );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the old monochromator crystal separation. */

	denominator = 2.0 * cos( old_theta * MX_RADIANS_PER_DEGREE );

	old_bragg_normal = mx_divide_safely( beam_offset, denominator );

	/* Compute the new monochromator crystal separation. */

	denominator = 2.0 * cos( theta * MX_RADIANS_PER_DEGREE );

	new_bragg_normal = mx_divide_safely( beam_offset, denominator );

	if ( flags & MXF_MTR_ONLY_CHECK_LIMITS ) {
		MX_DEBUG( 2,("%s: Checking '%s' limits for %g",
				fname, normal_record->name, new_bragg_normal));

		mx_status = mx_motor_check_position_limits( normal_record,
						new_bragg_normal, flags );
	} else {
		MX_DEBUG( 2,("%s: Moving '%s' to %g",
				fname, normal_record->name, new_bragg_normal));

		/* Change the motor speed for 'normal' if synchronous motion 
		 * is enabled.
		 */

		if ( monochromator_motor->synchronous_motion_mode ) {

			mx_status =
				mxd_monochromator_check_for_quick_scan_ability(
							normal_record,
							monochromator->record,
							list_record );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

#if 0
			{
				double speed;
				mx_status = mx_motor_get_speed( normal_record,
								&speed );
				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
				MX_DEBUG(-2,("%s: motor '%s' speed before = %g",
					fname, normal_record->name, speed));
			}
#endif
			mx_status = mx_motor_save_speed( normal_record );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_motor_set_speed_between_positions(
					normal_record,
					old_bragg_normal, new_bragg_normal,
					monochromator->move_time );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

#if 0
			{
				double speed;
				mx_status = mx_motor_get_speed( normal_record,
								&speed );
				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
				MX_DEBUG(-2,("%s: motor '%s' speed after = %g",
					fname, normal_record->name, speed));
			}
#endif
			monochromator->speed_changed[dependency_number] = TRUE;
		}

		mx_status = mx_motor_move_absolute( normal_record,
						new_bragg_normal, flags );
	}

	return mx_status;
}

/******************************************************************/

static mx_status_type
mxd_monochromator_get_bragg_parallel_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double *theta )
{
	return MX_SUCCESSFUL_RESULT;
}
static mx_status_type
mxd_monochromator_set_bragg_parallel_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta )
{
	return MX_SUCCESSFUL_RESULT;
}
static mx_status_type
mxd_monochromator_move_bragg_parallel( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta,
					double old_theta,
					int flags )
{
	static const char fname[] = "mxd_monochromator_move_bragg_parallel()";

	MX_RECORD *parallel_record;
	double new_bragg_parallel, beam_offset, denominator;
	mx_bool_type enabled;
	mx_status_type mx_status;

	mx_status = mxd_monochromator_get_enable_status( list_record, &enabled);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( enabled == FALSE )
		return MX_SUCCESSFUL_RESULT;

	mx_status = mxd_monochromator_get_double_crystal_parameters(
					monochromator, list_record,
					NULL, &parallel_record,
					1, &beam_offset );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the new monochromator crystal separation and move
	 * the Bragg normal motor there.
	 */

	denominator = 2.0 * sin( theta * MX_RADIANS_PER_DEGREE );

	new_bragg_parallel = mx_divide_safely( beam_offset, denominator );

	if ( flags & MXF_MTR_ONLY_CHECK_LIMITS ) {
		MX_DEBUG( 2,("%s: Checking '%s' limits for %g",
		    fname, parallel_record->name, new_bragg_parallel));
	} else {
		MX_DEBUG( 2,("%s: Moving '%s' to %g",
		    fname, parallel_record->name, new_bragg_parallel));
	}

	mx_status = mx_motor_move_absolute( parallel_record,
					new_bragg_parallel, flags );

	return mx_status;
}

/******************************************************************/

static mx_status_type
mxd_monochromator_get_table_position( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double *theta )
{
	return MX_SUCCESSFUL_RESULT;
}
static mx_status_type
mxd_monochromator_set_table_position( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta )
{
	return MX_SUCCESSFUL_RESULT;
}
static mx_status_type
mxd_monochromator_move_table( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta,
					double old_theta,
					int flags )
{
	static const char fname[] = "mxd_monochromator_move_table()";

	MX_RECORD *table_record;
	double constant, crystal_separation, new_table_position;
	double table_parameters[2];
	mx_bool_type enabled;
	mx_status_type mx_status;

	mx_status = mxd_monochromator_get_enable_status( list_record, &enabled);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( enabled == FALSE )
		return MX_SUCCESSFUL_RESULT;

	mx_status = mxd_monochromator_get_double_crystal_parameters(
					monochromator, list_record,
					NULL, &table_record,
					2, table_parameters );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	constant = table_parameters[0];
	crystal_separation = table_parameters[1];

	/* Compute the new table height and move the table there. */

	new_table_position = constant + 2.0 * crystal_separation
				* cos( theta * MX_RADIANS_PER_DEGREE );

	if ( flags & MXF_MTR_ONLY_CHECK_LIMITS ) {
		MX_DEBUG( 2,("%s: Checking '%s' limits for %g",
			fname, table_record->name, new_table_position));
	} else {
		MX_DEBUG( 2,("%s: Moving '%s' to %g",
			fname, table_record->name, new_table_position));
	}

	mx_status = mx_motor_move_absolute( table_record,
					new_table_position, flags );

	return mx_status;
}

/******************************************************************/

static mx_status_type
mxd_monochromator_get_diffractometer_theta_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double *theta )
{
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_monochromator_set_diffractometer_theta_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta )
{
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_monochromator_move_diffractometer_theta(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta,
					double old_theta,
					int flags )
{
	static const char fname[]
		= "mxd_monochromator_move_diffractometer_theta()";

	MX_RECORD **record_array;
	MX_RECORD *diffractometer_theta_record;
	double *parameter_array;
	double monochromator_d_spacing;
	double diffractometer_d_spacing;
	double diffractometer_scale, diffractometer_offset;
	double numerator, sin_theta, diffractometer_theta;
	void *pointer_to_value;
	long num_parameters, num_records, num_elements;
	mx_bool_type enabled;
	mx_status_type mx_status;

	mx_status = mxd_monochromator_get_enable_status( list_record, &enabled);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( enabled == FALSE )
		return MX_SUCCESSFUL_RESULT;

	mx_status = mxd_monochromator_get_param_array_from_list(
			list_record, &num_parameters, &parameter_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_parameters != 2 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The parameter array for list record '%s' should have 2 parameters in it.  "
"Instead, there are %ld parameters in it.", list_record->name, num_parameters );
	}

	diffractometer_scale = parameter_array[0];
	diffractometer_offset = parameter_array[1];

	mx_status = mxd_monochromator_get_record_array_from_list(
			list_record, &num_records, &record_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_records != 3 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The record array for list record '%s' should have 3 records in it.  "
"Instead, there are %ld records in it.", list_record->name, num_records );
	}

	/* The first record in the record array is the motor record
	 * for the diffractometer theta.
	 */

	diffractometer_theta_record = record_array[0];

	/* The second record is the monochromator d-spacing. */

	mx_status = mx_get_1d_array( record_array[1], MXFT_DOUBLE,
					&num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	monochromator_d_spacing = *( double * ) pointer_to_value;

	/* The third record is the diffractometer d-spacing. */

	mx_status = mx_get_1d_array( record_array[2], MXFT_DOUBLE,
					&num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	diffractometer_d_spacing = *( double * ) pointer_to_value;

	numerator = monochromator_d_spacing
			* sin( theta * MX_RADIANS_PER_DEGREE );

	sin_theta = mx_divide_safely( numerator, diffractometer_d_spacing );

	if ( fabs(sin_theta) > 1.0 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"sin(diffractometer_theta) = %g", sin_theta );
	}

	diffractometer_theta = asin( sin_theta ) / MX_RADIANS_PER_DEGREE;

	diffractometer_theta = diffractometer_scale * diffractometer_theta
					+ diffractometer_offset;

	/* Now we may move the motor. */

	if ( flags & MXF_MTR_ONLY_CHECK_LIMITS ) {
		MX_DEBUG( 2,("%s: Checking '%s' limits for %g",
				fname, diffractometer_theta_record->name,
				diffractometer_theta));
	} else {
		MX_DEBUG( 2,("%s: Moving '%s' to %g",
				fname, diffractometer_theta_record->name,
				diffractometer_theta));
	}

	mx_status = mx_motor_move_absolute( diffractometer_theta_record,
						diffractometer_theta, flags );

	return mx_status;
}

/******************************************************************/

/* These functions use the term 'e_polynomial' rather than 'energy_polynomial'
 * since the latter choice would create function names that have the same
 * first 31 characters as other already existing function names.
 */

static mx_status_type
mxd_monochromator_get_e_polynomial_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double *theta_position )
{
	return MX_SUCCESSFUL_RESULT;
}
static mx_status_type
mxd_monochromator_set_e_polynomial_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta )
{
	return MX_SUCCESSFUL_RESULT;
}
static mx_status_type
mxd_monochromator_move_e_polynomial(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta,
					double old_theta,
					int flags )
{
	static const char fname[] = "mxd_monochromator_move_e_polynomial()";

	MX_RECORD **record_array;
	MX_RECORD *energy_polynomial_record;
	double *parameter_array;
	long i, num_parameters, num_records, num_elements;
	double d_spacing, denominator, mono_energy, energy_polynomial;
	void *pointer_to_value;
	mx_bool_type enabled;
	mx_status_type mx_status;

	mx_status = mxd_monochromator_get_enable_status( list_record, &enabled);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( enabled == FALSE )
		return MX_SUCCESSFUL_RESULT;

	mx_status = mxd_monochromator_get_param_array_from_list(
			list_record, &num_parameters, &parameter_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_monochromator_get_record_array_from_list(
			list_record, &num_records, &record_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_records != 2 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The record array for list record '%s' does not have 2 records.  "
	"Instead, it has %ld records.", list_record->name, num_records );
	}

	/* The first record is the energy polynomial motor record. */

	energy_polynomial_record = record_array[0];

	/* The second record contains the value of the monochromator
	 * crystal d-spacing.
	 */

	mx_status = mx_get_1d_array( record_array[1], MXFT_DOUBLE,
					&num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	d_spacing = *( double * ) pointer_to_value;

	/* Compute the monochromator energy. */

	denominator = 2.0 * d_spacing * sin( theta * MX_RADIANS_PER_DEGREE );

	mono_energy = mx_divide_safely( MX_HC, denominator );

	/* Compute the energy polynomial position to request. */

	energy_polynomial = parameter_array[ num_parameters - 1 ];

	for ( i = num_parameters - 2; i >= 0; i-- ) {
		energy_polynomial = parameter_array[i]
					+ mono_energy * energy_polynomial;
	}

	/* Move the energy polynomial motor. */

	if ( flags & MXF_MTR_ONLY_CHECK_LIMITS ) {
		MX_DEBUG( 2,("%s: Checking '%s' limits for %g",
		fname, energy_polynomial_record->name, energy_polynomial));
	} else {
		MX_DEBUG( 2,("%s: Moving '%s' to %g",
		fname, energy_polynomial_record->name, energy_polynomial));
	}

	mx_status = mx_motor_move_absolute( energy_polynomial_record,
						energy_polynomial, flags );

	return mx_status;
}

/******************************************************************/

static mx_status_type
mxd_monochromator_get_option_selector_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double *theta_position )
{
	return MX_SUCCESSFUL_RESULT;
}
static mx_status_type
mxd_monochromator_set_option_selector_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta )
{
	return MX_SUCCESSFUL_RESULT;
}
static mx_status_type
mxd_monochromator_move_option_selector(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta,
					double old_theta,
					int flags )
{
	static const char fname[] = "mxd_monochromator_move_option_selector()";

	MX_RECORD **record_array;
	MX_RECORD *option_selector_record;
	MX_RECORD *option_range_record;
	void *pointer_to_value;
	double **option_range_array;
	double lower_limit, upper_limit;
	mx_bool_type enabled, exit_loop;
	long old_selection, new_selection;
	long num_records, num_dimensions, field_type, num_ranges;
	long *dimension_array;
	mx_status_type mx_status;

	if ( flags & MXF_MTR_ONLY_CHECK_LIMITS ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mxd_monochromator_get_enable_status( list_record, &enabled);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( enabled == FALSE )
		return MX_SUCCESSFUL_RESULT;

	mx_status = mxd_monochromator_get_record_array_from_list(
			list_record, &num_records, &record_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_records != 2 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The record array for list record '%s' does not have 2 records.  "
	"Instead, it has %ld records.", list_record->name, num_records );
	}

	/* The first record is the option selector record. */

	option_selector_record = record_array[0];

	/* The second record lists the option ranges. */

	option_range_record = record_array[1];

	/* Get the current value of the option selector. */

	mx_status = mx_get_long_variable( option_selector_record,
					&old_selection );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the option range array. */

	mx_status = mx_get_variable_parameters( option_range_record,
						&num_dimensions,
						&dimension_array,
						&field_type,
						&pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ( num_dimensions != 2 )
	  || ( dimension_array[1] != 2 )
	  || ( field_type != MXFT_DOUBLE ) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The option range variable '%s' used by monochromator '%s' "
	"is not a Nx2 array of doubles.", option_range_record->name,
				monochromator->record->name );
	}

	num_ranges = dimension_array[0];

	option_range_array = (double **) pointer_to_value;

#if 0
	{
		int i;

		for ( i = 0; i < num_ranges; i++ ) {
			MX_DEBUG(-2,("%s: range %d, %g to %g",
				fname, i, option_range_array[i][0],
				option_range_array[i][1]));
		}
	}
#endif

	if ( old_selection < 0 ) {
		new_selection = 0;

	} else if ( old_selection >= num_ranges ) {
		new_selection = num_ranges - 1;
	} else {
		new_selection = old_selection;
	}

	exit_loop = FALSE;

	while( exit_loop == FALSE ) {

		lower_limit = option_range_array[ new_selection ][0];
		upper_limit = option_range_array[ new_selection ][1];

		if ( theta < lower_limit ) {
			if ( new_selection > 0 ) {
				new_selection--;
			} else {
				new_selection = 0;

				exit_loop = TRUE;
			}
		} else if ( theta > upper_limit ) {
			if ( new_selection < ( num_ranges - 1 ) ) {
				new_selection++;
			} else {
				new_selection = num_ranges - 1;

				exit_loop = TRUE;
			}
		} else {
			exit_loop = TRUE;
		}
	}

	/* Change the option selector variable if necessary. */

	MX_DEBUG( 2,("%s: new_selection = %ld, old_selection = %ld",
		fname, new_selection, old_selection));

	if ( new_selection != old_selection ) {

		mx_status = mx_set_long_variable( option_selector_record,
					new_selection );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/******************************************************************/

static mx_status_type
mxd_monochromator_get_theta_fn_position( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double *theta_fn )
{
	return MX_SUCCESSFUL_RESULT;
}
static mx_status_type
mxd_monochromator_set_theta_fn_position( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta_fn )
{
	return MX_SUCCESSFUL_RESULT;
}
static mx_status_type
mxd_monochromator_move_theta_fn( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta,
					double old_theta,
					int flags )
{
	static const char fname[] = "mxd_monochromator_move_theta_fn()";

	MX_RECORD **record_array;
	MX_RECORD *dependent_motor_record;
	double *parameter_array;
	long num_records, num_parameters;
	mx_bool_type enabled;
	mx_status_type mx_status;

	mx_status = mxd_monochromator_get_enable_status( list_record, &enabled);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( enabled == FALSE )
		return MX_SUCCESSFUL_RESULT;

	mx_status = mxd_monochromator_get_param_array_from_list(
			list_record, &num_parameters, &parameter_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_monochromator_get_record_array_from_list(
			list_record, &num_records, &record_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_records != 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The record array for list record '%s' should have 1 record in it.  "
	"Instead there are %ld records in it.",
			list_record->name, num_records );
	}

	/* This record should be the dependent motor record. */

	if ( (record_array[0]->mx_superclass != MXR_DEVICE)
	  || (record_array[0]->mx_class != MXC_MOTOR) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The first record '%s' in the record array for list record '%s' "
	"is not a motor record.",
			record_array[0]->name, list_record->name );
	}

	dependent_motor_record = record_array[0];

	/* Move the dependent motor.  The dependent motor record is
	 * responsible for performing any transformation on the position.
	 */

	mx_status = mx_motor_move_absolute( dependent_motor_record,
						theta, flags );

	return mx_status;
}

/******************************************************************/

static mx_status_type
mxd_monochromator_get_energy_fn_position( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double *theta_position )
{
	return MX_SUCCESSFUL_RESULT;
}
static mx_status_type
mxd_monochromator_set_energy_fn_position( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta )
{
	return MX_SUCCESSFUL_RESULT;
}
static mx_status_type
mxd_monochromator_move_energy_fn( MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record,
					long dependency_number,
					double theta,
					double old_theta,
					int flags )
{
	static const char fname[] = "mxd_monochromator_move_energy_fn()";

	MX_RECORD **record_array;
	MX_RECORD *dependent_motor_record;
	double *parameter_array;
	long num_parameters, num_records, num_elements;
	double d_spacing, denominator, mono_energy;
	void *pointer_to_value;
	mx_bool_type enabled;
	mx_status_type mx_status;

	mx_status = mxd_monochromator_get_enable_status( list_record, &enabled);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( enabled == FALSE )
		return MX_SUCCESSFUL_RESULT;

	mx_status = mxd_monochromator_get_param_array_from_list(
			list_record, &num_parameters, &parameter_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_monochromator_get_record_array_from_list(
			list_record, &num_records, &record_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_records != 2 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The record array for list record '%s' does not have 2 records.  "
	"Instead, it has %ld records.", list_record->name, num_records );
	}

	/* The first record is the dependent motor record. */

	dependent_motor_record = record_array[0];

	/* The second record contains the value of the monochromator
	 * crystal d-spacing.
	 */

	mx_status = mx_get_1d_array( record_array[1], MXFT_DOUBLE,
					&num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	d_spacing = *( double * ) pointer_to_value;

	/* Compute the monochromator energy. */

	denominator = 2.0 * d_spacing * sin( theta * MX_RADIANS_PER_DEGREE );

	mono_energy = mx_divide_safely( MX_HC, denominator );

	/* Move the dependent motor.  The dependent motor record is
	 * responsible for computing the spline function.
	 */

	mx_status = mx_motor_move_absolute( dependent_motor_record,
						mono_energy, flags );

	return mx_status;
}

