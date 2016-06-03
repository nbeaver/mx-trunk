/*
 * Name:    d_wavelength.c 
 *
 * Purpose: MX motor driver to control an MX motor in units of X-ray wavelength.
 *          This is generally used to control an X-ray monochromator.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006-2007, 2010, 2013, 2016
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
#include "mx_variable.h"
#include "mx_motor.h"
#include "d_wavelength.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_wavelength_motor_record_function_list = {
	NULL,
	mxd_wavelength_motor_create_record_structures,
	mxd_wavelength_motor_finish_record_initialization,
	NULL,
	mxd_wavelength_motor_print_motor_structure,
	mxd_wavelength_motor_open
};

MX_MOTOR_FUNCTION_LIST mxd_wavelength_motor_motor_function_list = {
	NULL,
	mxd_wavelength_motor_move_absolute,
	mxd_wavelength_motor_get_position,
	mxd_wavelength_motor_set_position,
	mxd_wavelength_motor_soft_abort,
	mxd_wavelength_motor_immediate_abort,
	NULL,
	NULL,
	mxd_wavelength_motor_raw_home_command,
	mxd_wavelength_motor_constant_velocity_move,
	mxd_wavelength_motor_get_parameter,
	mxd_wavelength_motor_set_parameter,
	NULL,
	mxd_wavelength_motor_get_status,
	mxd_wavelength_motor_get_extended_status
};

/* Photon wavelength motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_wavelength_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_WAVELENGTH_MOTOR_STANDARD_FIELDS
};

long mxd_wavelength_motor_num_record_fields
		= sizeof( mxd_wavelength_motor_record_field_defaults )
		    / sizeof( mxd_wavelength_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_wavelength_motor_rfield_def_ptr
			= &mxd_wavelength_motor_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_wavelength_motor_get_pointers( MX_MOTOR *motor,
			MX_WAVELENGTH_MOTOR **wavelength_motor,
			MX_RECORD **dependent_motor_record,
			const char *calling_fname )
{
	static const char fname[] = "mxd_wavelength_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( wavelength_motor == (MX_WAVELENGTH_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The wavelength_motor pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*wavelength_motor = (MX_WAVELENGTH_MOTOR *)
					motor->record->record_type_struct;

	if ( *wavelength_motor == (MX_WAVELENGTH_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_WAVELENGTH_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( dependent_motor_record == (MX_RECORD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The dependent_motor_record pointer passed by '%s' was NULL.",
			motor->record->name );
	}

	*dependent_motor_record = (*wavelength_motor)->dependent_motor_record;

	if ( *dependent_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Dependent motor record pointer for record '%s' is NULL.",
			motor->record->name );
	}


	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_wavelength_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_wavelength_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_WAVELENGTH_MOTOR *wavelength_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	wavelength_motor = (MX_WAVELENGTH_MOTOR *)
				malloc( sizeof(MX_WAVELENGTH_MOTOR) );

	if ( wavelength_motor == (MX_WAVELENGTH_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_WAVELENGTH_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = wavelength_motor;
	record->class_specific_function_list
				= &mxd_wavelength_motor_motor_function_list;

	motor->record = record;

	/* An X-ray wavelength motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_wavelength_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_wavelength_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_WAVELENGTH_MOTOR *wavelength_motor;

	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	if ( motor == (MX_MOTOR *)NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	motor->motor_flags |= MXF_MTR_IS_PSEUDOMOTOR;

	wavelength_motor = (MX_WAVELENGTH_MOTOR *) record->record_type_struct;

	if ( wavelength_motor == (MX_WAVELENGTH_MOTOR *)NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_WAVELENGTH_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	motor->real_motor_record = wavelength_motor->dependent_motor_record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_wavelength_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_wavelength_motor_print_motor_structure()";

	MX_MOTOR *motor;
	MX_RECORD *dependent_motor_record;
	MX_MOTOR *dependent_motor;
	MX_WAVELENGTH_MOTOR *wavelength_motor;
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_wavelength_motor_get_pointers( motor, &wavelength_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

        dependent_motor = (MX_MOTOR *)
                                (dependent_motor_record->record_class_struct);

        if ( dependent_motor == (MX_MOTOR *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                        "MX_MOTOR pointer for record '%s' is NULL.",
                        dependent_motor_record->name );
        }

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type             = WAVELENGTH_MOTOR.\n\n");

	fprintf(file, "  name                   = %s\n", record->name);
	fprintf(file, "  dependent motor record = %s\n",
					dependent_motor_record->name);
	fprintf(file, "  d spacing              = %s\n",
				wavelength_motor->d_spacing_record->name);
	fprintf(file, "  angle scale            = %.*g radians per %s.\n",
					record->precision,
					wavelength_motor->angle_scale,
					dependent_motor->units);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

        fprintf(file, "  position               = %.*g %s  (%.*g)\n",
		record->precision,
                motor->position, motor->units,
		record->precision,
                motor->raw_position.analog );
	fprintf(file, "  wavelength scale       = %.*g %s per unscaled "
							"wavelength unit.\n",
		record->precision,
		motor->scale, motor->units);
	fprintf(file, "  wavelength offset      = %.*g %s.\n",
		record->precision,
		motor->offset, motor->units);
        fprintf(file, "  backlash               = %.*g %s  (%.*g).\n",
		record->precision,
                motor->backlash_correction, motor->units,
		record->precision,
                motor->raw_backlash_correction.analog);
        fprintf(file, "  negative limit         = %.*g %s  (%.*g).\n",
		record->precision,
                motor->negative_limit, motor->units,
		record->precision,
                motor->raw_negative_limit.analog);
        fprintf(file, "  positive limit         = %.*g %s  (%.*g).\n",
		record->precision,
		motor->positive_limit, motor->units,
		record->precision,
		motor->raw_positive_limit.analog);

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband          = %.*g %s  (%.*g).\n\n",
		record->precision,
		move_deadband, motor->units,
		record->precision,
		motor->raw_move_deadband.analog);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_wavelength_motor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_wavelength_motor_open()";

	MX_MOTOR *motor;
	MX_WAVELENGTH_MOTOR *wavelength_motor;
	MX_RECORD *dependent_motor_record;
	double d_spacing;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_wavelength_motor_get_pointers( motor, &wavelength_motor,
					&dependent_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the value of saved_d_spacing. */

	mx_status = mx_get_double_variable( wavelength_motor->d_spacing_record,
						&d_spacing );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	wavelength_motor->saved_d_spacing = d_spacing;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_wavelength_motor_get_d_spacing( MX_MOTOR *motor,
				MX_WAVELENGTH_MOTOR *wavelength_motor,
				double *d_spacing )
{
	mx_bool_type fast_mode;
	mx_status_type mx_status;

	mx_status = mx_get_fast_mode( motor->record, &fast_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( fast_mode ) {
		*d_spacing = wavelength_motor->saved_d_spacing;

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_get_double_variable( wavelength_motor->d_spacing_record,
						d_spacing );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	wavelength_motor->saved_d_spacing = *d_spacing;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_wavelength_motor_convert_theta_to_wavelength( MX_MOTOR *motor,
					MX_WAVELENGTH_MOTOR *wavelength_motor,
					double theta,
					double *wavelength )
{
	double d_spacing, theta_in_radians, sin_theta;
	mx_status_type mx_status;

	mx_status = mxd_wavelength_motor_get_d_spacing( motor,
					wavelength_motor, &d_spacing );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	theta_in_radians = theta * wavelength_motor->angle_scale;

	sin_theta = sin( theta_in_radians );

	*wavelength = 2.0 * d_spacing * sin_theta;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_wavelength_motor_convert_wavelength_to_theta( MX_MOTOR *motor,
					MX_WAVELENGTH_MOTOR *wavelength_motor,
					const char *operation,
					double wavelength,
					double *theta )
{
	static const char fname[] =
		"mxd_wavelength_motor_convert_wavelength_to_theta()";

	double d_spacing, maximum_wavelength;
	double theta_in_radians, sin_theta;
	mx_status_type mx_status;

	mx_status = mxd_wavelength_motor_get_d_spacing( motor,
					wavelength_motor, &d_spacing );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	maximum_wavelength = 2.0 * d_spacing;

	sin_theta = mx_divide_safely( wavelength, 2.0 * d_spacing );

	if ( fabs( sin_theta ) <= 1.0 ) {
		theta_in_radians = asin( sin_theta );
	} else {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"Attempted to %s an unreachable wavelength of %g %s for motor '%s'.  "
"The maximum allowed wavelength for a %g angstrom d spacing is %g angstroms.",
				operation,
				wavelength, motor->units,
				motor->record->name,
				d_spacing, maximum_wavelength );
	}

	*theta = mx_divide_safely( theta_in_radians,
					wavelength_motor->angle_scale );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_wavelength_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_wavelength_motor_move_absolute()";

	MX_WAVELENGTH_MOTOR *wavelength_motor;
	MX_RECORD *dependent_motor_record;
	double wavelength, theta;
	mx_status_type mx_status;

	mx_status = mxd_wavelength_motor_get_pointers( motor, &wavelength_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The unscaled wavelength is in angstroms. */

	wavelength = motor->raw_destination.analog;

	mx_status = mxd_wavelength_motor_convert_wavelength_to_theta( motor,
							wavelength_motor,
							"move to",
							wavelength, &theta );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_move_absolute( dependent_motor_record, theta, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_wavelength_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_wavelength_motor_get_position()";

	MX_WAVELENGTH_MOTOR *wavelength_motor;
	MX_RECORD *dependent_motor_record;
	double wavelength, theta;
	mx_status_type mx_status;

	wavelength = 0.0;

	mx_status = mxd_wavelength_motor_get_pointers( motor, &wavelength_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_position( dependent_motor_record, &theta );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_wavelength_motor_convert_theta_to_wavelength( motor,
							wavelength_motor,
							theta, &wavelength );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->raw_position.analog = wavelength;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_wavelength_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_wavelength_motor_set_position()";

	MX_WAVELENGTH_MOTOR *wavelength_motor;
	MX_RECORD *dependent_motor_record;
	double wavelength, theta;
	mx_status_type mx_status;

	mx_status = mxd_wavelength_motor_get_pointers( motor, &wavelength_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	wavelength = motor->raw_set_position.analog;

	mx_status = mxd_wavelength_motor_convert_wavelength_to_theta( motor,
							wavelength_motor,
							"set",
							wavelength, &theta );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_set_position( dependent_motor_record, theta );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_wavelength_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_wavelength_motor_soft_abort()";

	MX_WAVELENGTH_MOTOR *wavelength_motor;
	MX_RECORD *dependent_motor_record;
	mx_status_type mx_status;

	mx_status = mxd_wavelength_motor_get_pointers( motor, &wavelength_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_soft_abort( dependent_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_wavelength_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_wavelength_motor_immediate_abort()";

	MX_WAVELENGTH_MOTOR *wavelength_motor;
	MX_RECORD *dependent_motor_record;
	mx_status_type mx_status;

	mx_status = mxd_wavelength_motor_get_pointers( motor, &wavelength_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_immediate_abort( dependent_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_wavelength_motor_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_wavelength_motor_raw_home_command()";

	MX_WAVELENGTH_MOTOR *wavelength_motor;
	MX_RECORD *dependent_motor_record;
	long direction;
	mx_status_type mx_status;

	mx_status = mxd_wavelength_motor_get_pointers( motor, &wavelength_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	direction = - ( motor->raw_home_command );

	mx_status = mx_motor_raw_home_command( dependent_motor_record,
						direction );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_wavelength_motor_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_wavelength_motor_constant_velocity_move()";

	MX_WAVELENGTH_MOTOR *wavelength_motor;
	MX_RECORD *dependent_motor_record;
	long direction;
	mx_status_type mx_status;

	mx_status = mxd_wavelength_motor_get_pointers( motor, &wavelength_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	direction = - ( motor->constant_velocity_move );

	mx_status = mx_motor_constant_velocity_move( dependent_motor_record,
							direction );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_wavelength_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_wavelength_motor_get_parameter()";

	MX_WAVELENGTH_MOTOR *wavelength_motor;
	MX_RECORD *dependent_motor_record;
	double theta, wavelength, acceleration_time;
	double d_spacing, theta_in_radians;
	double theta_speed, theta_speed_in_radians_per_second;
	double theta_start, theta_end;
	double real_theta_start, real_theta_end;
	double wavelength_start, wavelength_end;
	double real_wavelength_start, real_wavelength_end;
	mx_status_type mx_status;

	wavelength = real_wavelength_start = real_wavelength_end = 0.0;

	mx_status = mxd_wavelength_motor_get_pointers( motor, &wavelength_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_BASE_SPEED:
	case MXLV_MTR_MAXIMUM_SPEED:
	case MXLV_MTR_ACCELERATION_TYPE:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
	case MXLV_MTR_ACCELERATION_DISTANCE:
		return mx_error( MXE_UNSUPPORTED, fname,
"Wavelength pseudomotor '%s' cannot report the value of parameter '%s' (%ld).",
			motor->record->name,
			mx_get_field_label_string( motor->record,
						motor->parameter_type ),
			motor->parameter_type );
		break;

	case MXLV_MTR_SPEED:
		mx_status = mx_motor_get_position(
				dependent_motor_record, &theta );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		theta_in_radians = theta * wavelength_motor->angle_scale;

		mx_status = mxd_wavelength_motor_get_d_spacing( motor,
						wavelength_motor, &d_spacing );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_motor_get_speed(
				dependent_motor_record, &theta_speed );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		theta_speed_in_radians_per_second =
			theta_speed * wavelength_motor->angle_scale;

		motor->raw_speed = 2.0 * d_spacing * cos( theta_in_radians )
					* theta_speed_in_radians_per_second;
		break;
	case MXLV_MTR_ACCELERATION_TIME:
		mx_status = mx_motor_get_acceleration_time(
					dependent_motor_record,
					&acceleration_time );

		motor->acceleration_time = acceleration_time;
		break;

	case MXLV_MTR_COMPUTE_EXTENDED_SCAN_RANGE:
		wavelength_start = motor->raw_compute_extended_scan_range[0];
		wavelength_end = motor->raw_compute_extended_scan_range[1];

		mx_status = mxd_wavelength_motor_convert_wavelength_to_theta(
					motor, wavelength_motor,
					"compute theta for",
					wavelength_start, &theta_start );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_wavelength_motor_convert_wavelength_to_theta(
					motor, wavelength_motor,
					"compute theta for",
					wavelength_end, &theta_end );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_motor_compute_extended_scan_range(
					dependent_motor_record,
					theta_start, theta_end,
					&real_theta_start, &real_theta_end );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_wavelength_motor_convert_theta_to_wavelength(
					motor, wavelength_motor,
					real_theta_start,
					&real_wavelength_start );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_wavelength_motor_convert_theta_to_wavelength(
					motor, wavelength_motor,
					real_theta_end, &real_wavelength_end );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_compute_extended_scan_range[2]
						= real_wavelength_start;

		motor->raw_compute_extended_scan_range[3]
						= real_wavelength_end;
		break;

	case MXLV_MTR_COMPUTE_PSEUDOMOTOR_POSITION:
		theta = motor->compute_pseudomotor_position[0];

		mx_status = mxd_wavelength_motor_convert_theta_to_wavelength(
				motor, wavelength_motor, theta, &wavelength );

		motor->compute_pseudomotor_position[1] = wavelength;

		break;

	case MXLV_MTR_COMPUTE_REAL_POSITION:
		wavelength = motor->compute_real_position[0];

		mx_status = mxd_wavelength_motor_convert_wavelength_to_theta(
					motor, wavelength_motor,
					"compute theta for",
					wavelength, &theta );

		motor->compute_real_position[1] = theta;

		break;

	default:
		mx_status = mx_motor_default_get_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_wavelength_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_wavelength_motor_set_parameter()";

	MX_WAVELENGTH_MOTOR *wavelength_motor;
	MX_RECORD *dependent_motor_record;
	double real_position1, real_position2, time_for_move;
	mx_status_type mx_status;

	mx_status = mxd_wavelength_motor_get_pointers( motor, &wavelength_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
	case MXLV_MTR_BASE_SPEED:
	case MXLV_MTR_MAXIMUM_SPEED:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Wavelength pseudomotor '%s' cannot set the value of parameter '%s' (%ld).",
			motor->record->name,
			mx_get_field_label_string( motor->record,
						motor->parameter_type ),
			motor->parameter_type );

	case MXLV_MTR_SPEED_CHOICE_PARAMETERS:
		mx_status =
		    mx_motor_compute_real_position_from_pseudomotor_position(
			motor->record, motor->raw_speed_choice_parameters[0],
			&real_position1, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status =
		    mx_motor_compute_real_position_from_pseudomotor_position(
			motor->record, motor->raw_speed_choice_parameters[1],
			&real_position2, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		time_for_move = motor->raw_speed_choice_parameters[2];

		mx_status = mx_motor_set_speed_between_positions(
					dependent_motor_record,
					real_position1, real_position2,
					time_for_move );
		break;

	case MXLV_MTR_SAVE_SPEED:
		mx_status = mx_motor_save_speed( dependent_motor_record );
		break;

	case MXLV_MTR_RESTORE_SPEED:
		mx_status = mx_motor_restore_speed( dependent_motor_record );
		break;

	default:
		mx_status = mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_wavelength_motor_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_wavelength_motor_get_status()";

	MX_WAVELENGTH_MOTOR *wavelength_motor;
	MX_RECORD *dependent_motor_record;
	unsigned long motor_status;
	mx_status_type mx_status;

	mx_status = mxd_wavelength_motor_get_pointers( motor, &wavelength_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_status( dependent_motor_record, &motor_status);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Reverse the direction of the motor limit bits if needed. */

	motor->status = motor_status;

	if ( motor->scale < 0.0 ) {
		if ( motor_status & MXSF_MTR_POSITIVE_LIMIT_HIT ) {
			motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
		} else {
			motor->status &= ( ~ MXSF_MTR_NEGATIVE_LIMIT_HIT );
		}

		if ( motor_status & MXSF_MTR_NEGATIVE_LIMIT_HIT ) {
			motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
		} else {
			motor->status &= ( ~ MXSF_MTR_POSITIVE_LIMIT_HIT );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_wavelength_motor_get_extended_status( MX_MOTOR *motor )
{
	static const char fname[] =
		"mxd_wavelength_motor_get_extended_status()";

	MX_WAVELENGTH_MOTOR *wavelength_motor;
	MX_RECORD *dependent_motor_record;
	unsigned long motor_status;
	double theta;
	mx_status_type mx_status;

	mx_status = mxd_wavelength_motor_get_pointers( motor, &wavelength_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_extended_status( dependent_motor_record,
						&theta, &motor_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_wavelength_motor_convert_theta_to_wavelength( motor,
						wavelength_motor, theta,
						&(motor->raw_position.analog) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Reverse the direction of the motor limit bits if needed. */

	motor->status = motor_status;

	if ( motor->scale < 0.0 ) {
		if ( motor_status & MXSF_MTR_POSITIVE_LIMIT_HIT ) {
			motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
		} else {
			motor->status &= ( ~ MXSF_MTR_NEGATIVE_LIMIT_HIT );
		}

		if ( motor_status & MXSF_MTR_NEGATIVE_LIMIT_HIT ) {
			motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
		} else {
			motor->status &= ( ~ MXSF_MTR_POSITIVE_LIMIT_HIT );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

