/*
 * Name:    d_gated_backlash.c 
 *
 * Purpose: MX pseudomotor driver that arranges for a gate signal to be
 *          generated when a backlash correction is in progress.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2006-2007, 2009, 2010, 2013, 2020-2022
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_GATED_BACKLASH_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_time.h"
#include "mx_hrt.h"
#include "mx_analog_output.h"
#include "mx_digital_output.h"
#include "mx_relay.h"
#include "mx_motor.h"
#include "d_gated_backlash.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_gated_backlash_record_function_list = {
	NULL,
	mxd_gated_backlash_create_record_structures,
	mx_motor_finish_record_initialization,
	NULL,
	mxd_gated_backlash_print_motor_structure,
	mxd_gated_backlash_open,
	NULL,
	NULL,
	mxd_gated_backlash_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_gated_backlash_motor_function_list = {
	NULL,
	mxd_gated_backlash_move_absolute,
	mxd_gated_backlash_get_position,
	mxd_gated_backlash_set_position,
	mxd_gated_backlash_soft_abort,
	mxd_gated_backlash_immediate_abort,
	NULL,
	NULL,
	mxd_gated_backlash_raw_home_command,
	mxd_gated_backlash_constant_velocity_move,
	mxd_gated_backlash_get_parameter,
	mxd_gated_backlash_set_parameter,
	NULL,
	mxd_gated_backlash_get_status
};

/* Soft motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_gated_backlash_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_GATED_BACKLASH_STANDARD_FIELDS
};

long mxd_gated_backlash_num_record_fields
		= sizeof( mxd_gated_backlash_record_field_defaults )
			/ sizeof( mxd_gated_backlash_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_gated_backlash_rfield_def_ptr
			= &mxd_gated_backlash_record_field_defaults[0];

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_gated_backlash_get_pointers( MX_MOTOR *motor,
			MX_GATED_BACKLASH **gated_backlash,
			const char *calling_fname )
{
	static const char fname[] = "mxd_gated_backlash_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( gated_backlash == (MX_GATED_BACKLASH **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_GATED_BACKLASH pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_MOTOR pointer "
		"passed by '%s' is NULL.", calling_fname );
	}

	*gated_backlash = (MX_GATED_BACKLASH *)
				motor->record->record_type_struct;

	if ( *gated_backlash == (MX_GATED_BACKLASH *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_GATED_BACKLASH pointer for motor record '%s' passed by '%s' is NULL",
				motor->record->name, calling_fname );
	}

	if ( (*gated_backlash)->real_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX real_motor_record pointer for MX gated backlash motor '%s' is NULL.",
			motor->record->name );
	}

	if ( (*gated_backlash)->gate_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX gate_record pointer for MX gated backlash motor '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_gated_backlash_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_gated_backlash_create_record_structures()";

	MX_MOTOR *motor;
	MX_GATED_BACKLASH *gated_backlash;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	gated_backlash = (MX_GATED_BACKLASH *)
				malloc( sizeof(MX_GATED_BACKLASH) );

	if ( gated_backlash == (MX_GATED_BACKLASH *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_GATED_BACKLASH structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = gated_backlash;
	record->class_specific_function_list
				= &mxd_gated_backlash_motor_function_list;

	motor->record = record;
	gated_backlash->record = record;

	motor->motor_flags = MXF_MTR_IS_PSEUDOMOTOR;

	/* A gated backlash motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_gated_backlash_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[]
		= "mxd_gated_backlash_print_motor_structure()";

	MX_MOTOR *motor;
	MX_GATED_BACKLASH *gated_backlash;
	double position, move_deadband, speed;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_gated_backlash_get_pointers( motor,
						&gated_backlash, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type        = GATED_BACKLASH.\n\n");

	fprintf(file, "  name              = %s\n", record->name);
	fprintf(file, "  real motor record = %s\n",
				gated_backlash->real_motor_record->name );
	fprintf(file, "  gate record       = %s\n",
				gated_backlash->gate_record->name );
	fprintf(file, "  gate on           = %#lx\n", gated_backlash->gate_on );
	fprintf(file, "  gate off          = %#lx\n", gated_backlash->gate_off);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  position          = %g %s (%g)\n",
		motor->position, motor->units, motor->raw_position.analog );

	fprintf(file, "  scale             = %g %s per step.\n",
		motor->scale, motor->units );

	fprintf(file, "  offset            = %g %s.\n",
		motor->offset, motor->units );

	fprintf(file, "  backlash          = %g %s (%g)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog );

	fprintf(file, "  negative_limit    = %g %s (%g)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog );

	fprintf(file, "  positive_limit    = %g %s (%g)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog );

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband     = %g %s (%g)\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	mx_status = mx_motor_get_speed( record, &speed );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to get the speed of motor '%s'",
			record->name );
	}

	fprintf(file, "  speed             = %g %s/sec\n\n",
		speed, motor->units );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_gated_backlash_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_gated_backlash_open()";

	MX_MOTOR *motor;
	MX_GATED_BACKLASH *gated_backlash;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_gated_backlash_get_pointers( motor,
						&gated_backlash, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->real_motor_record = gated_backlash->real_motor_record;

	gated_backlash->gate_delay = mx_convert_seconds_to_timespec_time( 0.1 );

	gated_backlash->gate_delay_finish = mx_high_resolution_time();

	switch( gated_backlash->gate_record->mx_class ) {
	case MXC_ANALOG_OUTPUT:
	case MXC_DIGITAL_OUTPUT:
	case MXC_RELAY:
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The 'gate_record' for 'gated_backlash' record '%s' "
		"must be a record of type 'analog_output', 'digital_output', "
		"or 'relay'.", record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_gated_backlash_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_gated_backlash_resynchronize()";

	MX_MOTOR *motor;
	MX_GATED_BACKLASH *gated_backlash;
	mx_status_type mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_gated_backlash_get_pointers( motor,
						&gated_backlash, fname );

	if ( mx_status.code != MXE_SUCCESS ) {
		motor->busy = FALSE;

		return mx_status;
	}

	mx_status =
		mx_resynchronize_record( gated_backlash->real_motor_record );

	return mx_status;
}

static mx_status_type
mxd_gated_backlash_set_gate_value( MX_GATED_BACKLASH *gated_backlash,
					unsigned long gate_value )
{
	static const char fname[] = "mxd_gated_backlash_set_gate_value()";

	mx_status_type mx_status;

#if MXD_GATED_BACKLASH_DEBUG
	MX_DEBUG(-2,
	("%s: Setting gate record '%s' to %#lx for gated backlash motor '%s'.",
		fname, gated_backlash->gate_record->name,
		gate_value, gated_backlash->record->name ));
#endif

	switch( gated_backlash->gate_record->mx_class ) {
	case MXC_ANALOG_OUTPUT:
		mx_status = mx_analog_output_write( gated_backlash->gate_record,
							(double) gate_value );
		break;
	case MXC_DIGITAL_OUTPUT:
		mx_status = mx_digital_output_write(gated_backlash->gate_record,
							gate_value );
		break;
	case MXC_RELAY:
		mx_status = mx_relay_command( gated_backlash->gate_record,
							(long) gate_value );
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The 'gate_record' for 'gated_backlash' record '%s' "
		"must be a record of type 'analog_output', 'digital_output', "
		"or 'relay'.", gated_backlash->record->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gated_backlash_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_gated_backlash_move_absolute()";

	MX_GATED_BACKLASH *gated_backlash;
	mx_status_type mx_status;

	mx_status = mxd_gated_backlash_get_pointers( motor,
						&gated_backlash, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->backlash_move_in_progress ) {
		mx_status = mxd_gated_backlash_set_gate_value(
				gated_backlash, gated_backlash->gate_on );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		gated_backlash->gate_delay_finish =
			mx_add_timespec_times( mx_high_resolution_time(),
				gated_backlash->gate_delay );
	}

	mx_status = mx_motor_move_absolute( gated_backlash->real_motor_record,
					motor->raw_destination.analog, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gated_backlash_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_gated_backlash_get_position()";

	MX_GATED_BACKLASH *gated_backlash;
	mx_status_type mx_status;

	mx_status = mxd_gated_backlash_get_pointers( motor,
						&gated_backlash, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_position( gated_backlash->real_motor_record,
						&(motor->raw_position.analog) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gated_backlash_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_gated_backlash_set_position()";

	MX_GATED_BACKLASH *gated_backlash;
	mx_status_type mx_status;

	mx_status = mxd_gated_backlash_get_pointers( motor,
						&gated_backlash, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_set_position( gated_backlash->real_motor_record,
					motor->raw_set_position.analog );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gated_backlash_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_gated_backlash_soft_abort()";

	MX_GATED_BACKLASH *gated_backlash;
	mx_status_type mx_status;

	mx_status = mxd_gated_backlash_get_pointers( motor,
						&gated_backlash, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_soft_abort( gated_backlash->real_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gated_backlash_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_gated_backlash_immediate_abort()";

	MX_GATED_BACKLASH *gated_backlash;
	mx_status_type mx_status;

	mx_status = mxd_gated_backlash_get_pointers( motor,
						&gated_backlash, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status =
		mx_motor_immediate_abort( gated_backlash->real_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gated_backlash_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_gated_backlash_raw_home_command()";

	MX_GATED_BACKLASH *gated_backlash;
	mx_status_type mx_status;

	mx_status = mxd_gated_backlash_get_pointers( motor,
						&gated_backlash, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_raw_home_command(
				gated_backlash->real_motor_record,
				motor->raw_home_command );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gated_backlash_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] =
			"mxd_gated_backlash_constant_velocity_move()";

	MX_GATED_BACKLASH *gated_backlash;
	mx_status_type mx_status;

	mx_status = mxd_gated_backlash_get_pointers( motor,
						&gated_backlash, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_constant_velocity_move(
					gated_backlash->real_motor_record,
					motor->constant_velocity_move );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gated_backlash_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_gated_backlash_get_parameter()";

	MX_GATED_BACKLASH *gated_backlash;
	mx_status_type mx_status;

	mx_status = mxd_gated_backlash_get_pointers( motor,
						&gated_backlash, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status =
			mx_motor_get_speed( gated_backlash->real_motor_record,
						&(motor->raw_speed) );
		break;

	case MXLV_MTR_BASE_SPEED:
		mx_status = mx_motor_get_base_speed(
					gated_backlash->real_motor_record,
					&(motor->raw_base_speed) );
		break;

	case MXLV_MTR_MAXIMUM_SPEED:
		mx_status = mx_motor_get_maximum_speed(
					gated_backlash->real_motor_record,
					&(motor->raw_maximum_speed) );
		break;

	case MXLV_MTR_SYNCHRONOUS_MOTION_MODE:
		mx_status = mx_motor_get_synchronous_motion_mode(
					gated_backlash->real_motor_record,
					&(motor->synchronous_motion_mode) );
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		mx_status = mx_motor_get_raw_acceleration_parameters(
					gated_backlash->real_motor_record,
					motor->raw_acceleration_parameters );
		break;

	case MXLV_MTR_ACCELERATION_DISTANCE:
		mx_status = mx_motor_get_acceleration_distance(
					gated_backlash->real_motor_record,
					&(motor->acceleration_distance) );
		break;

	case MXLV_MTR_ACCELERATION_TIME:
		mx_status = mx_motor_get_acceleration_time(
					gated_backlash->real_motor_record,
					&(motor->acceleration_time) );
		break;

	case MXLV_MTR_COMPUTE_EXTENDED_SCAN_RANGE:
		mx_status = mx_motor_compute_extended_scan_range(
				gated_backlash->real_motor_record,
				motor->raw_compute_extended_scan_range[0],
				motor->raw_compute_extended_scan_range[1],
				&(motor->raw_compute_extended_scan_range[2]),
				&(motor->raw_compute_extended_scan_range[3]) );
		break;

	case MXLV_MTR_COMPUTE_PSEUDOMOTOR_POSITION:
		mx_status =
		    mx_motor_compute_pseudomotor_position_from_real_position(
				gated_backlash->real_motor_record,
				motor->compute_pseudomotor_position[0],
				&(motor->compute_pseudomotor_position[1]),
				TRUE );
		break;

	case MXLV_MTR_COMPUTE_REAL_POSITION:
		mx_status =
		    mx_motor_compute_real_position_from_pseudomotor_position(
				gated_backlash->real_motor_record,
				motor->compute_real_position[0],
				&(motor->compute_real_position[1]),
				TRUE );
		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gated_backlash_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_gated_backlash_set_parameter()";

	MX_GATED_BACKLASH *gated_backlash;
	mx_status_type mx_status;

	mx_status = mxd_gated_backlash_get_pointers( motor,
						&gated_backlash, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch ( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status =
			mx_motor_set_speed( gated_backlash->real_motor_record,
						motor->raw_speed );
		break;

	case MXLV_MTR_BASE_SPEED:
		mx_status = mx_motor_set_base_speed(
					gated_backlash->real_motor_record,
					motor->raw_base_speed );
		break;

	case MXLV_MTR_MAXIMUM_SPEED:
		mx_status = mx_motor_set_maximum_speed(
					gated_backlash->real_motor_record,
					motor->raw_maximum_speed );
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		mx_status = mx_motor_set_raw_acceleration_parameters(
					gated_backlash->real_motor_record,
					motor->raw_acceleration_parameters );
		break;

	case MXLV_MTR_SPEED_CHOICE_PARAMETERS:
		mx_status = mx_motor_set_speed_between_positions(
					gated_backlash->real_motor_record,
					motor->raw_speed_choice_parameters[0],
					motor->raw_speed_choice_parameters[1],
					motor->raw_speed_choice_parameters[2] );
		break;

	case MXLV_MTR_SAVE_SPEED:
		mx_status = mx_motor_save_speed(
					gated_backlash->real_motor_record );
		break;

	case MXLV_MTR_RESTORE_SPEED:
		mx_status = mx_motor_restore_speed(
					gated_backlash->real_motor_record );
		break;

	case MXLV_MTR_SYNCHRONOUS_MOTION_MODE:
		mx_status = mx_motor_set_synchronous_motion_mode(
					gated_backlash->real_motor_record,
					motor->synchronous_motion_mode );
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gated_backlash_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_gated_backlash_get_status()";

	MX_GATED_BACKLASH *gated_backlash;
	int comparison;
	mx_status_type mx_status;

	mx_status = mxd_gated_backlash_get_pointers( motor,
						&gated_backlash, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_status( gated_backlash->real_motor_record,
						&(motor->status) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->backlash_move_in_progress ) {
		if ( ( motor->status & MXSF_MTR_IS_BUSY ) == 0 ) {

			comparison = mx_compare_timespec_times(
					mx_high_resolution_time(),
					gated_backlash->gate_delay_finish );

			if ( comparison >= 0 ) {
				motor->backlash_move_in_progress = FALSE;

				mx_status = mxd_gated_backlash_set_gate_value(
				    gated_backlash, gated_backlash->gate_off );
			}
		}
	}

	return mx_status;
}

