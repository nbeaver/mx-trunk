/*
 * Name:    d_spec_motor.c 
 *
 * Purpose: MX motor driver for motors controlled via a Spec server.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2004, 2006, 2010-2011, 2013, 2015-2016
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SPEC_MOTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_spec.h"
#include "d_spec_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_spec_motor_record_function_list = {
	NULL,
	mxd_spec_motor_create_record_structures,
	mxd_spec_motor_finish_record_initialization,
	NULL,
	mxd_spec_motor_print_motor_structure,
	mxd_spec_motor_open
};

MX_MOTOR_FUNCTION_LIST mxd_spec_motor_motor_function_list = {
	mxd_spec_motor_motor_is_busy,
	mxd_spec_motor_move_absolute,
	mxd_spec_motor_get_position,
	mxd_spec_motor_set_position,
	mxd_spec_motor_soft_abort,
	NULL,
	mxd_spec_motor_positive_limit_hit,
	mxd_spec_motor_negative_limit_hit,
	mxd_spec_motor_raw_home_command,
	NULL,
	mxd_spec_motor_get_parameter,
	mxd_spec_motor_set_parameter,
	mxd_spec_motor_simultaneous_start
};

/* Spec motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_spec_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_SPEC_MOTOR_STANDARD_FIELDS
};

long mxd_spec_motor_num_record_fields
		= sizeof( mxd_spec_motor_record_field_defaults )
			/ sizeof( mxd_spec_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_spec_motor_rfield_def_ptr
			= &mxd_spec_motor_record_field_defaults[0];

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_spec_motor_get_pointers( MX_MOTOR *motor,
			MX_SPEC_MOTOR **spec_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_spec_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The motor pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RECORD pointer for motor pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( motor->record->mx_type != MXT_MTR_SPEC ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The motor '%s' passed by '%s' is not a Spec motor.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			motor->record->name, calling_fname,
			motor->record->mx_superclass,
			motor->record->mx_class,
			motor->record->mx_type );
	}

	if ( spec_motor != (MX_SPEC_MOTOR **) NULL ) {

		*spec_motor = (MX_SPEC_MOTOR *)
				(motor->record->record_type_struct);

		if ( *spec_motor == (MX_SPEC_MOTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_SPEC_MOTOR pointer for motor record '%s' passed by '%s' is NULL",
				motor->record->name, calling_fname );
		}
	}

	if ( (*spec_motor)->spec_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX server record pointer for MX spec motor '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_spec_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_spec_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_SPEC_MOTOR *spec_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	spec_motor = (MX_SPEC_MOTOR *) malloc( sizeof(MX_SPEC_MOTOR) );

	if ( spec_motor == (MX_SPEC_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SPEC_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = spec_motor;
	record->class_specific_function_list
				= &mxd_spec_motor_motor_function_list;

	motor->record = record;

	/* A Spec motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	/* The Spec motor reports accelerations in counts/sec**2 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_spec_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_spec_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for record '%s' is NULL.", record->name);
	}

	motor->motor_flags |= MXF_MTR_IS_REMOTE_MOTOR;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_spec_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_spec_motor_print_motor_structure()";

	MX_MOTOR *motor;
	MX_SPEC_MOTOR *spec_motor;
	double position, move_deadband, speed;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MOTOR pointer for record '%s' is NULL.",
			record->name);
	}

	spec_motor = (MX_SPEC_MOTOR *) (record->record_type_struct);

	if ( spec_motor == (MX_SPEC_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SPEC_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type     = SPEC_MOTOR.\n\n");

	fprintf(file, "  name           = %s\n", record->name);
	fprintf(file, "  server         = %s\n",
					spec_motor->spec_server_record->name );
	fprintf(file, "  remote record  = %s\n",
					spec_motor->remote_motor_name );

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  position       = %g %s (%g)\n",
		motor->position, motor->units, motor->raw_position.analog );

	fprintf(file, "  scale          = %g %s per step.\n",
		motor->scale, motor->units );

	fprintf(file, "  offset         = %g %s.\n",
		motor->offset, motor->units );

	fprintf(file, "  backlash       = %g %s (%g)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog );

	fprintf(file, "  negative_limit = %g %s (%g)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog );

	fprintf(file, "  positive_limit = %g %s (%g)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog );

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband  = %g %s (%g)\n",
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
mxd_spec_motor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_spec_motor_open()";

	MX_MOTOR *motor;
	MX_SPEC_MOTOR *spec_motor;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_spec_motor_get_pointers( motor, &spec_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	spec_motor->end_of_busy_delay_tick = mx_current_clock_tick();

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_spec_motor_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_spec_motor_motor_is_busy()";

	MX_SPEC_MOTOR *spec_motor;
	char property_name[SV_NAME_LEN];
	long move_done, clock_tick_comparison;
	MX_CLOCK_TICK current_tick;
	mx_status_type mx_status;

	mx_status = mxd_spec_motor_get_pointers( motor, &spec_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( property_name, sizeof(property_name),
			"motor/%s/move_done",
			spec_motor->remote_motor_name );

	mx_status = mx_spec_get_number( spec_motor->spec_server_record,
				property_name, MXFT_LONG, &move_done );

	/* Sometimes, immediately after a move starts, spec will report
	 * the value of move_done as 0.  In order to reduce the chance
	 * of a move immediately being reported as done before it has
	 * started, we calculate in the move_absolute command an
         * end_of_busy_delay_tick time before which the motor will 
	 * unconditionally be reported as busy, regardless of the 
	 * value of move_done.
	 */

	current_tick = mx_current_clock_tick();

	clock_tick_comparison = mx_compare_clock_ticks(
			current_tick, spec_motor->end_of_busy_delay_tick );

#if MXD_SPEC_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: clock_tick_comparison = %d",
		fname, clock_tick_comparison));
#endif

	if ( clock_tick_comparison < 0 ) {
		motor->busy = TRUE;
	} else {
		if ( move_done == 0 ) {
			motor->busy = FALSE;
		} else {
			motor->busy = TRUE;
		}
	}

#if MXD_SPEC_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: '%s' = %d, motor->busy = %d",
		fname, property_name, move_done, motor->busy));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_spec_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_spec_motor_move_absolute()";

	MX_SPEC_MOTOR *spec_motor;
	char property_name[SV_NAME_LEN];
	double new_destination;
	MX_CLOCK_TICK current_tick, delay_ticks;
	mx_status_type mx_status;

	mx_status = mxd_spec_motor_get_pointers( motor, &spec_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	new_destination = motor->raw_destination.analog;

	snprintf( property_name, sizeof(property_name),
			"motor/%s/start_one",
			spec_motor->remote_motor_name );

#if MXD_SPEC_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: '%s' = %g", fname, property_name, new_destination));
#endif

	mx_status = mx_spec_put_number( spec_motor->spec_server_record,
				property_name, MXFT_DOUBLE, &new_destination );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We force the motor->busy flag to true for a fixed amount of time
	 * after the move stops by computing a time before which the motor
	 * always reported as being busy.
	 */

	motor->busy = TRUE;

	current_tick = mx_current_clock_tick();

	delay_ticks = mx_convert_seconds_to_clock_ticks(
					MX_SPEC_MOTOR_BUSY_DELAY );

	spec_motor->end_of_busy_delay_tick =
			mx_add_clock_ticks( current_tick, delay_ticks );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_spec_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_spec_motor_get_position()";

	MX_SPEC_MOTOR *spec_motor;
	char property_name[SV_NAME_LEN];
	double position;
	mx_status_type mx_status;

	mx_status = mxd_spec_motor_get_pointers( motor, &spec_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( property_name, sizeof(property_name),
			"motor/%s/position",
			spec_motor->remote_motor_name );

	mx_status = mx_spec_get_number( spec_motor->spec_server_record,
				property_name, MXFT_DOUBLE, &position );

#if MXD_SPEC_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: '%s' = %g", fname, property_name, position));
#endif

	motor->raw_position.analog = position;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_spec_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_spec_motor_set_position()";

	MX_SPEC_MOTOR *spec_motor;
	char property_name[SV_NAME_LEN];
	double new_set_position;
	mx_status_type mx_status;

	mx_status = mxd_spec_motor_get_pointers( motor, &spec_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	new_set_position = motor->raw_set_position.analog;

	snprintf( property_name, sizeof(property_name),
			"motor/%s/position",
			spec_motor->remote_motor_name );

#if MXD_SPEC_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: '%s' set position = %g",
		fname, property_name, new_set_position));
#endif

	mx_status = mx_spec_put_number( spec_motor->spec_server_record,
				property_name, MXFT_DOUBLE, &new_set_position );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_spec_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_spec_motor_soft_abort()";

	MX_SPEC_MOTOR *spec_motor;
	mx_status_type mx_status;

	mx_status = mxd_spec_motor_get_pointers( motor, &spec_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SPEC_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: 'motor/../abort_all'", fname));
#endif

	mx_status = mx_spec_put_string( spec_motor->spec_server_record,
					"motor/../abort_all", NULL );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_spec_motor_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_spec_motor_positive_limit_hit()";

	MX_SPEC_MOTOR *spec_motor;
	char property_name[SV_NAME_LEN];
	long limit_hit;
	mx_status_type mx_status;

	mx_status = mxd_spec_motor_get_pointers( motor, &spec_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( property_name, sizeof(property_name),
			"motor/%s/high_lim_hit",
			spec_motor->remote_motor_name );

	mx_status = mx_spec_get_number( spec_motor->spec_server_record,
				property_name, MXFT_LONG, &limit_hit );

	if ( limit_hit ) {
		motor->positive_limit_hit = TRUE;
	} else {
		motor->positive_limit_hit = FALSE;
	}

#if MXD_SPEC_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: '%s' = %d", fname, property_name, limit_hit));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_spec_motor_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_spec_motor_negative_limit_hit()";

	MX_SPEC_MOTOR *spec_motor;
	char property_name[SV_NAME_LEN];
	long limit_hit;
	mx_status_type mx_status;

	mx_status = mxd_spec_motor_get_pointers( motor, &spec_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( property_name, sizeof(property_name),
			"motor/%s/low_lim_hit",
			spec_motor->remote_motor_name );

	mx_status = mx_spec_get_number( spec_motor->spec_server_record,
				property_name, MXFT_LONG, &limit_hit );

	if ( limit_hit ) {
		motor->negative_limit_hit = TRUE;
	} else {
		motor->negative_limit_hit = FALSE;
	}

#if MXD_SPEC_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: '%s' = %d", fname, property_name, limit_hit));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_spec_motor_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_spec_motor_raw_home_command()";

	MX_SPEC_MOTOR *spec_motor;
	char property_name[SV_NAME_LEN];
	char home_search_string[20];
	mx_status_type mx_status;

	mx_status = mxd_spec_motor_get_pointers( motor,
						&spec_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->raw_home_command ) {
	case -2:
		strlcpy( home_search_string, "lim-",
			sizeof(home_search_string) );
		break;
	case -1:
		strlcpy( home_search_string, "home-",
			sizeof(home_search_string) );
		break;
	case 0:
		strlcpy( home_search_string, "home",
			sizeof(home_search_string) );
		break;
	case 1:
		strlcpy( home_search_string, "home+",
			sizeof(home_search_string) );
		break;
	case 2:
		strlcpy( home_search_string, "lim+",
			sizeof(home_search_string) );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal home search value %ld specified for motor '%s'.  "
		"The legal values are -1, 1 for home search, "
		"-2, 2 for limit search, 0 for ***FIXME***",
			motor->raw_home_command, motor->record->name );
	}

	snprintf( property_name, sizeof(property_name),
			"motor/%s/search",
			spec_motor->remote_motor_name );

#if MXD_SPEC_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: '%s' home_search_string = '%s'",
		fname, property_name, home_search_string));
#endif

	mx_status = mx_spec_put_string( spec_motor->spec_server_record,
					property_name, home_search_string );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_spec_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_spec_motor_get_parameter()";

	MX_SPEC_MOTOR *spec_motor;
	char property_name[SV_NAME_LEN];
	mx_status_type mx_status;

	mx_status = mxd_spec_motor_get_pointers( motor,
						&spec_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		snprintf( property_name, sizeof(property_name),
				"motor/%s/slew_rate",
				spec_motor->remote_motor_name );

		mx_status = mx_spec_get_number( spec_motor->spec_server_record,
						property_name, MXFT_DOUBLE,
						&(motor->raw_speed) );
		break;

	case MXLV_MTR_BASE_SPEED:
		snprintf( property_name, sizeof(property_name),
				"motor/%s/base_rate",
				spec_motor->remote_motor_name );

		mx_status = mx_spec_get_number( spec_motor->spec_server_record,
						property_name, MXFT_DOUBLE,
						&(motor->raw_base_speed) );
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		snprintf( property_name, sizeof(property_name),
				"motor/%s/acceleration",
				spec_motor->remote_motor_name );

		motor->raw_acceleration_parameters[1] = 0.0;
		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;

		mx_status = mx_spec_get_number( spec_motor->spec_server_record,
						property_name, MXFT_DOUBLE,
				&(motor->raw_acceleration_parameters[0]) );
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		snprintf( property_name, sizeof(property_name),
				"motor/%s/dc_proportional_gain",
				spec_motor->remote_motor_name );

		mx_status = mx_spec_get_number( spec_motor->spec_server_record,
						property_name, MXFT_DOUBLE,
						&(motor->proportional_gain) );
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		snprintf( property_name, sizeof(property_name),
				"motor/%s/dc_integral_gain",
				spec_motor->remote_motor_name );

		mx_status = mx_spec_get_number( spec_motor->spec_server_record,
						property_name, MXFT_DOUBLE,
						&(motor->integral_gain) );
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		snprintf( property_name, sizeof(property_name),
				"motor/%s/dc_derivative_gain",
				spec_motor->remote_motor_name );

		mx_status = mx_spec_get_number( spec_motor->spec_server_record,
						property_name, MXFT_DOUBLE,
						&(motor->derivative_gain) );
		break;

	case MXLV_MTR_INTEGRAL_LIMIT:
		snprintf( property_name, sizeof(property_name),
				"motor/%s/dc_integration_limit",
				spec_motor->remote_motor_name );

		mx_status = mx_spec_get_number( spec_motor->spec_server_record,
						property_name, MXFT_DOUBLE,
						&(motor->integral_limit) );
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %ld is not supported by this driver.",
			motor->parameter_type );
	}

#if MXD_SPEC_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: property '%s'", fname, property_name));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_spec_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_spec_motor_set_parameter()";

	MX_SPEC_MOTOR *spec_motor;
	char property_name[SV_NAME_LEN];
	mx_status_type mx_status;

	mx_status = mxd_spec_motor_get_pointers( motor,
						&spec_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch ( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		snprintf( property_name, sizeof(property_name),
				"motor/%s/slew_rate",
				spec_motor->remote_motor_name );

		mx_status = mx_spec_put_number( spec_motor->spec_server_record,
						property_name, MXFT_DOUBLE,
						&(motor->raw_speed) );
		break;

	case MXLV_MTR_BASE_SPEED:
		snprintf( property_name, sizeof(property_name),
				"motor/%s/base_rate",
				spec_motor->remote_motor_name );

		mx_status = mx_spec_put_number( spec_motor->spec_server_record,
						property_name, MXFT_DOUBLE,
						&(motor->raw_base_speed) );
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		snprintf( property_name, sizeof(property_name),
				"motor/%s/acceleration",
				spec_motor->remote_motor_name );

		mx_status = mx_spec_put_number( spec_motor->spec_server_record,
						property_name, MXFT_DOUBLE,
				&(motor->raw_acceleration_parameters[0]) );
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		snprintf( property_name, sizeof(property_name),
				"motor/%s/dc_proportional_gain",
				spec_motor->remote_motor_name );

		mx_status = mx_spec_put_number( spec_motor->spec_server_record,
						property_name, MXFT_DOUBLE,
						&(motor->proportional_gain) );
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		snprintf( property_name, sizeof(property_name),
				"motor/%s/dc_integral_gain",
				spec_motor->remote_motor_name );

		mx_status = mx_spec_put_number( spec_motor->spec_server_record,
						property_name, MXFT_DOUBLE,
						&(motor->integral_gain) );
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		snprintf( property_name, sizeof(property_name),
				"motor/%s/dc_deriviative_gain",
				spec_motor->remote_motor_name );

		mx_status = mx_spec_put_number( spec_motor->spec_server_record,
						property_name, MXFT_DOUBLE,
						&(motor->derivative_gain) );
		break;

	case MXLV_MTR_INTEGRAL_LIMIT:
		snprintf( property_name, sizeof(property_name),
				"motor/%s/dc_integration_limit",
				spec_motor->remote_motor_name );

		mx_status = mx_spec_put_number( spec_motor->spec_server_record,
						property_name, MXFT_DOUBLE,
						&(motor->integral_limit) );
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %ld is not supported by this driver.",
			motor->parameter_type );
	}

#if MXD_SPEC_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: property '%s'", fname, property_name));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_spec_motor_simultaneous_start( long num_motor_records,
				MX_RECORD **motor_record_array,
				double *destination_array,
				unsigned long flags )
{
	static const char fname[] = "mxd_spec_motor_simultaneous_start()";

	MX_RECORD *motor_record, *spec_server_record;
	MX_MOTOR *motor;
	MX_SPEC_MOTOR *spec_motor;
	int i;
	double raw_destination;
	char property_name[SV_NAME_LEN];
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	if ( num_motor_records <= 0 )
		return MX_SUCCESSFUL_RESULT;

	/* Find the Spec server record. */

	spec_motor = (MX_SPEC_MOTOR *)motor_record_array[0]->record_type_struct;

	spec_server_record = spec_motor->spec_server_record;

	/* Tell Spec to prepare for a simultaneous start. */

	mx_status = mx_spec_put_string( spec_server_record,
					"motor/../prestart_all", NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 0; i < num_motor_records; i++ ) {
		motor_record = motor_record_array[i];

		motor = (MX_MOTOR *) motor_record->record_class_struct;

		if ( motor_record->mx_type != MXT_MTR_SPEC ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"Cannot perform a simultaneous start since motors "
			"'%s' and '%s' are not the same type of motors.",
				motor_record_array[0]->name,
				motor_record->name );
		}

		spec_motor = (MX_SPEC_MOTOR *) motor_record->record_type_struct;

		if ( spec_server_record != spec_motor->spec_server_record ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Cannot perform a simultaneous tart since motors "
			"'%s' and '%s' are not controlled by the same "
			"Spec server.", motor_record_array[0]->name,
					motor_record->name );
		}

		/* Compute the new position in raw units. */

		raw_destination =
			mx_divide_safely( destination_array[i] - motor->offset,
						motor->scale );

		/* Add the move command for this motor. */

		mx_status = mx_spec_put_number( spec_server_record,
						property_name, MXFT_DOUBLE,
						&raw_destination );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Now perform the simultaneous start. */

	mx_status = mx_spec_put_string( spec_server_record,
					"motor/../start_all", NULL );

	return mx_status;
}

