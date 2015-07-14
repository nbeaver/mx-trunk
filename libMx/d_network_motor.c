/*
 * Name:    d_network_motor.c 
 *
 * Purpose: MX motor driver for motors controlled via an MX network server.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2006, 2009-2011, 2013-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NETWORK_MOTOR_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_hrt_debug.h"
#include "mx_motor.h"
#include "d_network_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_network_motor_record_function_list = {
	NULL,
	mxd_network_motor_create_record_structures,
	mxd_network_motor_finish_record_initialization,
	NULL,
	mxd_network_motor_print_motor_structure,
	NULL,
	NULL,
	NULL,
	mxd_network_motor_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_network_motor_motor_function_list = {
	mxd_network_motor_motor_is_busy,
	mxd_network_motor_move_absolute,
	mxd_network_motor_get_position,
	mxd_network_motor_set_position,
	mxd_network_motor_soft_abort,
	mxd_network_motor_immediate_abort,
	mxd_network_motor_positive_limit_hit,
	mxd_network_motor_negative_limit_hit,
	mxd_network_motor_raw_home_command,
	mxd_network_motor_constant_velocity_move,
	mxd_network_motor_get_parameter,
	mxd_network_motor_set_parameter,
	NULL,
	mxd_network_motor_get_status,
	mxd_network_motor_get_extended_status
};

/* Soft motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_network_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_NETWORK_MOTOR_STANDARD_FIELDS
};

long mxd_network_motor_num_record_fields
		= sizeof( mxd_network_motor_record_field_defaults )
			/ sizeof( mxd_network_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_motor_rfield_def_ptr
			= &mxd_network_motor_record_field_defaults[0];

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_network_motor_get_pointers( MX_MOTOR *motor,
			MX_NETWORK_MOTOR **network_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( network_motor == (MX_NETWORK_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_NETWORK_MOTOR pointer passed by '%s' was NULL",
			calling_fname );
	}

	*network_motor = (MX_NETWORK_MOTOR *) motor->record->record_type_struct;

	if ( *network_motor == (MX_NETWORK_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_NETWORK_MOTOR pointer for motor record '%s' passed by '%s' is NULL",
				motor->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_network_motor_get_remote_record_information( MX_MOTOR *motor,
					MX_NETWORK_MOTOR *network_motor )
{
	static const char fname[]
		= "mxd_network_motor_get_remote_record_information()";

	MX_DRIVER *driver;
	long dimension[1];
	mx_status_type mx_status;

	network_motor->need_to_get_remote_record_information = FALSE;

	/* Get the 'motor_flags' value for the remote record. */

	mx_status = mx_get( &(network_motor->motor_flags_nf),
			MXFT_ULONG, &(network_motor->remote_motor_flags) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: network_motor '%s', remote_motor_flags = %lx",
	fname, motor->record->name, network_motor->remote_motor_flags ));

	/* Get the name of the driver for the remote record. */

	dimension[0] = MXU_DRIVER_NAME_LENGTH;

	mx_status = mx_get_array( &(network_motor->mx_type_nf),
				MXFT_STRING, 1, dimension,
				network_motor->remote_driver_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: network_motor '%s', remote_driver_name = '%s'",
	fname, motor->record->name, network_motor->remote_driver_name ));

	/* See if the remote driver is listed in the local client's
	 * driver list.  If so, record that value in remote_driver_type.
	 * If not, set remote_driver_type to -1.
	 */

	driver = mx_get_driver_by_name( network_motor->remote_driver_name );

	if ( driver == (MX_DRIVER *) NULL ) {
		network_motor->remote_driver_type = -1;
	} else {
		network_motor->remote_driver_type = driver->mx_type;
	}

	MX_DEBUG( 2,("%s: network_motor '%s', remote_driver_type = %ld",
	fname, motor->record->name, network_motor->remote_driver_type ));

	/* Get the remote acceleration type. */

	mx_status = mx_get(&(network_motor->acceleration_type_nf),
			MXFT_LONG, &(motor->acceleration_type) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: network_motor '%s', acceleration_type = %ld",
		fname, motor->record->name, motor->acceleration_type));

	/* Get the remote scale factor. */

	mx_status = mx_get(&(network_motor->scale_nf),
			MXFT_DOUBLE, &(network_motor->remote_scale) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: network_motor '%s', remote_scale = %g",
		fname, motor->record->name, network_motor->remote_scale));

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_network_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_network_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_NETWORK_MOTOR *network_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	network_motor = (MX_NETWORK_MOTOR *) malloc( sizeof(MX_NETWORK_MOTOR) );

	if ( network_motor == (MX_NETWORK_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NETWORK_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = network_motor;
	record->class_specific_function_list
				= &mxd_network_motor_motor_function_list;

	motor->record = record;
	network_motor->record = record;

	network_motor->need_to_get_remote_record_information = TRUE;

	network_motor->remote_driver_type = -1;

	strlcpy( network_motor->remote_driver_name, "",
		sizeof(network_motor->remote_driver_name) );

	network_motor->remote_motor_flags = 0;

	/* If we need the acceleration type later, then we will need
	 * to explicitly fetch it from the server at that time.
	 */

	motor->acceleration_type = MXF_MTR_ACCEL_NONE;

	/* A network motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_NETWORK_MOTOR *network_motor;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_network_motor_get_pointers( motor,
						&network_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy(record->network_type_name, "mx", MXU_NETWORK_TYPE_NAME_LENGTH);

	motor->motor_flags |= MXF_MTR_IS_REMOTE_MOTOR;

	/* Initialize network fields. */

	mx_network_field_init( &(network_motor->acceleration_distance_nf),
		network_motor->server_record,
	  "%s.acceleration_distance", network_motor->remote_record_name );

	mx_network_field_init(
		&(network_motor->acceleration_feedforward_gain_nf),
		network_motor->server_record,
    "%s.acceleration_feedforward_gain", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->acceleration_time_nf),
		network_motor->server_record,
		"%s.acceleration_time", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->acceleration_type_nf),
		network_motor->server_record,
		"%s.acceleration_type", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->axis_enable_nf),
		network_motor->server_record,
		"%s.axis_enable", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->base_speed_nf),
		network_motor->server_record,
		"%s.base_speed", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->busy_nf),
		network_motor->server_record,
		"%s.busy", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->closed_loop_nf),
		network_motor->server_record,
		"%s.closed_loop", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->compute_extended_scan_range_nf),
		network_motor->server_record,
    "%s.compute_extended_scan_range", network_motor->remote_record_name);

	mx_network_field_init(
		&(network_motor->compute_pseudomotor_position_nf),
		network_motor->server_record,
    "%s.compute_pseudomotor_position", network_motor->remote_record_name);

	mx_network_field_init( &(network_motor->compute_real_position_nf),
		network_motor->server_record,
	  "%s.compute_real_position", network_motor->remote_record_name);

	mx_network_field_init( &(network_motor->constant_velocity_move_nf),
		network_motor->server_record,
		"%s.constant_velocity_move", network_motor->remote_record_name);

	mx_network_field_init( &(network_motor->derivative_gain_nf),
		network_motor->server_record,
		"%s.derivative_gain", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->destination_nf),
		network_motor->server_record,
		"%s.destination", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->extended_status_nf),
		network_motor->server_record,
		"%s.extended_status", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->extra_gain_nf),
		network_motor->server_record,
		"%s.extra_gain", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->fault_reset_nf),
		network_motor->server_record,
		"%s.fault_reset", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->home_search_nf),
		network_motor->server_record,
		"%s.home_search", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->immediate_abort_nf),
		network_motor->server_record,
		"%s.immediate_abort", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->integral_gain_nf),
		network_motor->server_record,
		"%s.integral_gain", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->integral_limit_nf),
		network_motor->server_record,
		"%s.integral_limit", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->limit_switch_as_home_switch_nf),
		network_motor->server_record,
		"%s.limit_switch_as_home_switch",
					network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->maximum_speed_nf),
		network_motor->server_record,
		"%s.maximum_speed", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->motor_flags_nf),
		network_motor->server_record,
		"%s.motor_flags", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->mx_type_nf),
		network_motor->server_record,
		"%s.mx_type", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->negative_limit_hit_nf),
		network_motor->server_record,
		"%s.negative_limit_hit", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->position_nf),
		network_motor->server_record,
		"%s.position", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->positive_limit_hit_nf),
		network_motor->server_record,
		"%s.positive_limit_hit", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->proportional_gain_nf),
		network_motor->server_record,
		"%s.proportional_gain", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->raw_acceleration_parameters_nf),
		network_motor->server_record,
	  "%s.raw_acceleration_parameters", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->restore_speed_nf),
		network_motor->server_record,
		"%s.restore_speed", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->resynchronize_nf),
		network_motor->server_record,
		"%s.resynchronize", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->save_speed_nf),
		network_motor->server_record,
		"%s.save_speed", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->save_start_positions_nf),
		network_motor->server_record,
		"%s.save_start_positions", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->scale_nf),
		network_motor->server_record,
		"%s.scale", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->set_position_nf),
		network_motor->server_record,
		"%s.set_position", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->soft_abort_nf),
		network_motor->server_record,
		"%s.soft_abort", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->speed_nf),
		network_motor->server_record,
		"%s.speed", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->speed_choice_parameters_nf),
		network_motor->server_record,
	  "%s.speed_choice_parameters", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->status_nf),
		network_motor->server_record,
		"%s.status", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->synchronous_motion_mode_nf),
		network_motor->server_record,
	  "%s.synchronous_motion_mode", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->use_start_positions_nf),
		network_motor->server_record,
		"%s.use_start_positions", network_motor->remote_record_name);

	mx_network_field_init( &(network_motor->use_window_nf),
		network_motor->server_record,
		"%s.use_window", network_motor->remote_record_name);

	mx_network_field_init( &(network_motor->velocity_feedforward_gain_nf),
		network_motor->server_record,
	  "%s.velocity_feedforward_gain", network_motor->remote_record_name );

	mx_network_field_init( &(network_motor->window_nf),
		network_motor->server_record,
		"%s.window", network_motor->remote_record_name);

	mx_network_field_init( &(network_motor->window_is_available_nf),
		network_motor->server_record,
		"%s.window_is_available", network_motor->remote_record_name);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[]
		= "mxd_network_motor_print_motor_structure()";

	MX_MOTOR *motor;
	MX_NETWORK_MOTOR *network_motor;
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

	network_motor = (MX_NETWORK_MOTOR *) (record->record_type_struct);

	if ( network_motor == (MX_NETWORK_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_NETWORK_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	if ( network_motor->need_to_get_remote_record_information ) {

		mx_status = mxd_network_motor_get_remote_record_information(
				motor, network_motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type     = NETWORK_MOTOR.\n\n");

	fprintf(file, "  name           = %s\n", record->name);
	fprintf(file, "  server         = %s\n",
					network_motor->server_record->name );
	fprintf(file, "  remote record  = %s\n",
					network_motor->remote_record_name );

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
mxd_network_motor_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_network_motor_resynchronize()";

	MX_MOTOR *motor;
	MX_NETWORK_MOTOR *network_motor;
	mx_bool_type resynchronize;
	mx_status_type mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_network_motor_get_pointers( motor,
						&network_motor, fname );

	if ( mx_status.code != MXE_SUCCESS ) {
		motor->busy = FALSE;

		return mx_status;
	}

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor->record->name));

	resynchronize = TRUE;

	mx_status = mx_put( &(network_motor->resynchronize_nf),
				MXFT_BOOL, &resynchronize );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_network_motor_get_remote_record_information(
			motor, network_motor );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_motor_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_network_motor_motor_is_busy()";

	MX_NETWORK_MOTOR *network_motor;
	mx_bool_type busy;
	mx_status_type mx_status;

#if MXD_NETWORK_MOTOR_DEBUG_TIMING
	MX_HRT_TIMING busy_measurement;
#endif

	mx_status = mxd_network_motor_get_pointers( motor,
						&network_motor, fname );

	if ( mx_status.code != MXE_SUCCESS ) {
		motor->busy = FALSE;

		return mx_status;
	}

#if MXD_NETWORK_MOTOR_DEBUG_TIMING
	MX_HRT_START( busy_measurement );
#endif

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor->record->name));

	if ( network_motor->need_to_get_remote_record_information ) {

		mx_status = mxd_network_motor_get_remote_record_information(
				motor, network_motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_get( &(network_motor->busy_nf), MXFT_BOOL, &busy );

	if ( busy ) {
		motor->busy = TRUE;
	} else {
		motor->busy = FALSE;
	}

#if MXD_NETWORK_MOTOR_DEBUG_TIMING
	MX_HRT_END( busy_measurement );

	MX_HRT_RESULTS( busy_measurement, fname, "busy" );
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_network_motor_move_absolute()";

	MX_NETWORK_MOTOR *network_motor;
	double new_destination;
	mx_status_type mx_status;

#if MXD_NETWORK_MOTOR_DEBUG_TIMING
	MX_HRT_TIMING move_measurement;
#endif

	mx_status = mxd_network_motor_get_pointers( motor,
						&network_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_MOTOR_DEBUG_TIMING
	MX_HRT_START( move_measurement );
#endif

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor->record->name));

	if ( network_motor->need_to_get_remote_record_information ) {

		mx_status = mxd_network_motor_get_remote_record_information(
				motor, network_motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	new_destination = motor->raw_destination.analog;

	mx_status = mx_put( &(network_motor->destination_nf),
				MXFT_DOUBLE, &new_destination );

#if MXD_NETWORK_MOTOR_DEBUG_TIMING
	MX_HRT_END( move_measurement );

	MX_HRT_RESULTS( move_measurement, fname, "move" );
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_network_motor_get_position()";

	MX_NETWORK_MOTOR *network_motor;
	double position;
	mx_status_type mx_status;

#if MXD_NETWORK_MOTOR_DEBUG_TIMING
	MX_HRT_TIMING get_position_measurement;
#endif

	mx_status = mxd_network_motor_get_pointers( motor,
						&network_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_MOTOR_DEBUG_TIMING
	MX_HRT_START( get_position_measurement );
#endif

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor->record->name));

	if ( network_motor->need_to_get_remote_record_information ) {

		mx_status = mxd_network_motor_get_remote_record_information(
				motor, network_motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_get( &(network_motor->position_nf),
				MXFT_DOUBLE, &position );

	motor->raw_position.analog = position;

#if MXD_NETWORK_MOTOR_DEBUG_TIMING
	MX_HRT_END( get_position_measurement );

	MX_HRT_RESULTS( get_position_measurement, fname, "get position" );
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_network_motor_set_position()";

	MX_NETWORK_MOTOR *network_motor;
	double new_set_position;
	mx_status_type mx_status;

	mx_status = mxd_network_motor_get_pointers( motor,
						&network_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor->record->name));

	if ( network_motor->need_to_get_remote_record_information ) {

		mx_status = mxd_network_motor_get_remote_record_information(
				motor, network_motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	new_set_position = motor->raw_set_position.analog;

	mx_status = mx_put( &(network_motor->set_position_nf),
				MXFT_DOUBLE, &new_set_position );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_network_motor_soft_abort()";

	MX_NETWORK_MOTOR *network_motor;
	mx_bool_type abort_flag;
	mx_status_type mx_status;

	mx_status = mxd_network_motor_get_pointers( motor,
						&network_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor->record->name));

	if ( network_motor->need_to_get_remote_record_information ) {

		mx_status = mxd_network_motor_get_remote_record_information(
				motor, network_motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	abort_flag = TRUE;

	mx_status = mx_put( &(network_motor->soft_abort_nf),
				MXFT_BOOL, &abort_flag );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_network_motor_immediate_abort()";

	MX_NETWORK_MOTOR *network_motor;
	mx_bool_type abort_flag;
	mx_status_type mx_status;

	mx_status = mxd_network_motor_get_pointers( motor,
						&network_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor->record->name));

	if ( network_motor->need_to_get_remote_record_information ) {

		mx_status = mxd_network_motor_get_remote_record_information(
				motor, network_motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	abort_flag = TRUE;

	mx_status = mx_put( &(network_motor->immediate_abort_nf),
				MXFT_BOOL, &abort_flag );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_motor_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_network_motor_positive_limit_hit()";

	MX_NETWORK_MOTOR *network_motor;
	mx_bool_type limit_hit;
	mx_status_type mx_status;

	mx_status = mxd_network_motor_get_pointers( motor,
						&network_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor->record->name));

	if ( network_motor->need_to_get_remote_record_information ) {

		mx_status = mxd_network_motor_get_remote_record_information(
				motor, network_motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_get( &(network_motor->positive_limit_hit_nf),
				MXFT_BOOL, &limit_hit );

	motor->positive_limit_hit = limit_hit;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_motor_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_network_motor_negative_limit_hit()";

	MX_NETWORK_MOTOR *network_motor;
	mx_bool_type limit_hit;
	mx_status_type mx_status;

	mx_status = mxd_network_motor_get_pointers( motor,
						&network_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor->record->name));

	if ( network_motor->need_to_get_remote_record_information ) {

		mx_status = mxd_network_motor_get_remote_record_information(
				motor, network_motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_get( &(network_motor->negative_limit_hit_nf),
				MXFT_BOOL, &limit_hit );

	motor->negative_limit_hit = limit_hit;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_motor_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_network_motor_raw_home_command()";

	MX_NETWORK_MOTOR *network_motor;
	long home_search;
	mx_status_type mx_status;

	mx_status = mxd_network_motor_get_pointers( motor,
						&network_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor->record->name));

	if ( network_motor->need_to_get_remote_record_information ) {

		mx_status = mxd_network_motor_get_remote_record_information(
				motor, network_motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	home_search = motor->raw_home_command;

	mx_status = mx_put( &(network_motor->home_search_nf),
				MXFT_BOOL, &home_search );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_motor_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[]
		= "mxd_network_motor_constant_velocity_move()";

	MX_NETWORK_MOTOR *network_motor;
	long constant_velocity_move;
	mx_status_type mx_status;

	mx_status = mxd_network_motor_get_pointers( motor,
						&network_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor->record->name));

	if ( network_motor->need_to_get_remote_record_information ) {

		mx_status = mxd_network_motor_get_remote_record_information(
				motor, network_motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	constant_velocity_move = motor->constant_velocity_move;

	mx_status = mx_put( &(network_motor->constant_velocity_move_nf),
				MXFT_LONG, &constant_velocity_move );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_network_motor_get_parameter()";

	MX_NETWORK_MOTOR *network_motor;
	long dimension_array[1];
	unsigned long flags;
	int is_pseudomotor;
	mx_status_type mx_status;

	mx_status = mxd_network_motor_get_pointers( motor,
						&network_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	if ( network_motor->need_to_get_remote_record_information ) {

		mx_status = mxd_network_motor_get_remote_record_information(
				motor, network_motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status = mx_get( &(network_motor->speed_nf),
				MXFT_DOUBLE, &(motor->raw_speed) );
		break;

	case MXLV_MTR_BASE_SPEED:
		mx_status = mx_get( &(network_motor->base_speed_nf),
				MXFT_DOUBLE, &(motor->raw_base_speed) );
		break;

	case MXLV_MTR_MAXIMUM_SPEED:
		mx_status = mx_get( &(network_motor->maximum_speed_nf),
				MXFT_DOUBLE, &(motor->raw_maximum_speed) );
		break;

	case MXLV_MTR_SYNCHRONOUS_MOTION_MODE:
		mx_status = mx_get(&(network_motor->synchronous_motion_mode_nf),
				MXFT_BOOL, &(motor->synchronous_motion_mode) );
		break;

	case MXLV_MTR_ACCELERATION_TYPE:
		mx_status = mx_get(&(network_motor->acceleration_type_nf),
				MXFT_LONG, &(motor->acceleration_type) );
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		dimension_array[0] = MX_MOTOR_NUM_ACCELERATION_PARAMS;

		mx_status = mx_get_array(
			&(network_motor->raw_acceleration_parameters_nf),
				MXFT_DOUBLE, 1, dimension_array,
				&(motor->raw_acceleration_parameters) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( motor->acceleration_type == MXF_MTR_ACCEL_RATE ) {
			motor->raw_acceleration_parameters[0]
				*= fabs(network_motor->remote_scale);

			motor->raw_acceleration_parameters[1]
				*= fabs(network_motor->remote_scale);
		}
		break;

	case MXLV_MTR_ACCELERATION_DISTANCE:
		mx_status = mx_get( &(network_motor->acceleration_distance_nf),
				MXFT_DOUBLE, &(motor->acceleration_distance) );
		break;

	case MXLV_MTR_ACCELERATION_TIME:
		mx_status = mx_get( &(network_motor->acceleration_time_nf),
				MXFT_DOUBLE, &(motor->acceleration_time) );
		break;

	case MXLV_MTR_COMPUTE_EXTENDED_SCAN_RANGE:
		dimension_array[0] = MX_MOTOR_NUM_SCAN_RANGE_PARAMS;

		/* Send the original scan range limits. */

		mx_status = mx_put_array(
			&(network_motor->compute_extended_scan_range_nf),
				MXFT_DOUBLE, 1, dimension_array,
				&(motor->raw_compute_extended_scan_range) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Then, get the extended scan range limits. */

		mx_status = mx_get_array(
			&(network_motor->compute_extended_scan_range_nf),
				MXFT_DOUBLE, 1, dimension_array,
				&(motor->raw_compute_extended_scan_range) );
		break;

	case MXLV_MTR_COMPUTE_PSEUDOMOTOR_POSITION:

		flags = network_motor->remote_motor_flags;

		is_pseudomotor = (int) ( flags & MXF_MTR_IS_PSEUDOMOTOR );

		if ( (is_pseudomotor == FALSE)
		  || (flags & MXF_MTR_PSEUDOMOTOR_RECURSION_IS_NOT_NECESSARY) )
		{
			/* If the remote motor _is_ the real motor, or has
			 * a value that is equal to the real motor's position,
			 * then it is not necessary to send a message across
			 * the network.
			 */

			motor->compute_pseudomotor_position[1]
				= motor->compute_pseudomotor_position[0];

			return MX_SUCCESSFUL_RESULT;
		}

		dimension_array[0] = MX_MOTOR_NUM_POSITION_PARAMS;

		/* Send the real motor position. */

		mx_status = mx_put_array(
			&(network_motor->compute_pseudomotor_position_nf),
				MXFT_DOUBLE, 1, dimension_array,
				&(motor->compute_pseudomotor_position) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Get the pseudomotor position. */

		mx_status = mx_get_array(
			&(network_motor->compute_pseudomotor_position_nf),
				MXFT_DOUBLE, 1, dimension_array,
				&(motor->compute_pseudomotor_position) );
		break;

	case MXLV_MTR_COMPUTE_REAL_POSITION:

		flags = network_motor->remote_motor_flags;

		is_pseudomotor = (int) ( flags & MXF_MTR_IS_PSEUDOMOTOR );

		if ( (is_pseudomotor == FALSE)
		  || (flags & MXF_MTR_PSEUDOMOTOR_RECURSION_IS_NOT_NECESSARY) )
		{
			/* If the remote motor _is_ the real motor, or has
			 * a value that is equal to the real motor's position,
			 * then it is not necessary to send a message across
			 * the network.
			 */

			motor->compute_real_position[1]
				= motor->compute_real_position[0];

			return MX_SUCCESSFUL_RESULT;
		}

		dimension_array[0] = MX_MOTOR_NUM_POSITION_PARAMS;

		/* Send the pseudomotor position. */

		mx_status = mx_put_array(
				&(network_motor->compute_real_position_nf),
				MXFT_DOUBLE, 1, dimension_array,
				&(motor->compute_real_position) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Get the real motor position. */

		mx_status = mx_get_array(
				&(network_motor->compute_real_position_nf),
				MXFT_DOUBLE, 1, dimension_array,
				&(motor->compute_real_position) );
		break;

	case MXLV_MTR_LIMIT_SWITCH_AS_HOME_SWITCH:
		mx_status = mx_get(
			&(network_motor->limit_switch_as_home_switch_nf),
			MXFT_LONG, &(motor->limit_switch_as_home_switch) );
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		mx_status = mx_get( &(network_motor->proportional_gain_nf),
				MXFT_DOUBLE, &(motor->proportional_gain) );
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		mx_status = mx_get( &(network_motor->integral_gain_nf),
				MXFT_DOUBLE, &(motor->integral_gain) );
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		mx_status = mx_get( &(network_motor->derivative_gain_nf),
				MXFT_DOUBLE, &(motor->derivative_gain) );
		break;

	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		mx_status = mx_get(
			&(network_motor->velocity_feedforward_gain_nf),
			MXFT_DOUBLE, &(motor->velocity_feedforward_gain) );
		break;

	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		mx_status = mx_get(
			&(network_motor->acceleration_feedforward_gain_nf),
			MXFT_DOUBLE, &(motor->acceleration_feedforward_gain) );
		break;

	case MXLV_MTR_INTEGRAL_LIMIT:
		mx_status = mx_get( &(network_motor->integral_limit_nf),
			MXFT_DOUBLE, &(motor->integral_limit) );
		break;

	case MXLV_MTR_EXTRA_GAIN:
		mx_status = mx_get( &(network_motor->extra_gain_nf),
			MXFT_DOUBLE, &(motor->extra_gain) );
		break;

	case MXLV_MTR_USE_WINDOW:
		mx_status = mx_get( &(network_motor->use_window_nf),
				MXFT_BOOL, &(motor->use_window) );
		break;

	case MXLV_MTR_WINDOW:
		dimension_array[0] = 2;	/* window start, window end */

		mx_status = mx_get_array( &(network_motor->window_nf),
			MXFT_DOUBLE, 1, dimension_array, motor->window );
		break;

	case MXLV_MTR_WINDOW_IS_AVAILABLE:
		mx_status = mx_get( &(network_motor->window_is_available_nf),
				MXFT_BOOL, &(motor->window_is_available) );
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %ld is not supported by this driver.",
			motor->parameter_type );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_network_motor_set_parameter()";

	MX_NETWORK_MOTOR *network_motor;
	long dimension_array[1];
	mx_bool_type bool_value;
	mx_status_type mx_status;

	mx_status = mxd_network_motor_get_pointers( motor,
						&network_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	if ( network_motor->need_to_get_remote_record_information ) {

		mx_status = mxd_network_motor_get_remote_record_information(
				motor, network_motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	switch ( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status = mx_put( &(network_motor->speed_nf),
				MXFT_DOUBLE, &(motor->raw_speed) );
		break;

	case MXLV_MTR_BASE_SPEED:
		mx_status = mx_put( &(network_motor->base_speed_nf),
				MXFT_DOUBLE, &(motor->raw_base_speed) );
		break;

	case MXLV_MTR_MAXIMUM_SPEED:
		mx_status = mx_put( &(network_motor->maximum_speed_nf),
				MXFT_DOUBLE, &(motor->raw_maximum_speed) );
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		dimension_array[0] = MX_MOTOR_NUM_ACCELERATION_PARAMS;

		mx_status = mx_put_array(
			&(network_motor->raw_acceleration_parameters_nf),
				MXFT_DOUBLE, 1, dimension_array,
				&(motor->raw_acceleration_parameters) );
		break;

	case MXLV_MTR_SPEED_CHOICE_PARAMETERS:
		dimension_array[0] = MX_MOTOR_NUM_SPEED_CHOICE_PARAMS;

		mx_status = mx_put_array(
				&(network_motor->speed_choice_parameters_nf),
				MXFT_DOUBLE, 1, dimension_array,
				&(motor->raw_speed_choice_parameters) );
		break;

	case MXLV_MTR_SAVE_SPEED:
		bool_value = TRUE;

		mx_status = mx_put( &(network_motor->save_speed_nf),
					MXFT_BOOL, &bool_value );
		break;

	case MXLV_MTR_RESTORE_SPEED:
		bool_value = TRUE;

		mx_status = mx_put( &(network_motor->restore_speed_nf),
				MXFT_BOOL, &bool_value );
		break;

	case MXLV_MTR_SYNCHRONOUS_MOTION_MODE:
		mx_status = mx_put(&(network_motor->synchronous_motion_mode_nf),
				MXFT_BOOL, &(motor->synchronous_motion_mode) );
		break;

	case MXLV_MTR_LIMIT_SWITCH_AS_HOME_SWITCH:
		mx_status = mx_put(
			&(network_motor->limit_switch_as_home_switch_nf),
			MXFT_LONG, &(motor->limit_switch_as_home_switch) );
		break;

	case MXLV_MTR_SAVE_START_POSITIONS:
		mx_status = mx_put( &(network_motor->save_start_positions_nf),
				MXFT_DOUBLE,&(motor->raw_saved_start_position));
		break;

	case MXLV_MTR_USE_START_POSITIONS:
		mx_status = mx_put( &(network_motor->use_start_positions_nf),
				MXFT_BOOL, &(motor->use_start_positions) );
		break;

	case MXLV_MTR_AXIS_ENABLE:
		mx_status = mx_put( &(network_motor->axis_enable_nf),
				MXFT_BOOL, &(motor->axis_enable) );
		break;

	case MXLV_MTR_CLOSED_LOOP:
		mx_status = mx_put( &(network_motor->closed_loop_nf),
				MXFT_BOOL, &(motor->closed_loop) );
		break;

	case MXLV_MTR_FAULT_RESET:
		mx_status = mx_put( &(network_motor->fault_reset_nf),
				MXFT_BOOL, &(motor->fault_reset) );
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		mx_status = mx_put( &(network_motor->proportional_gain_nf),
				MXFT_DOUBLE, &(motor->proportional_gain) );
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		mx_status = mx_put( &(network_motor->integral_gain_nf),
				MXFT_DOUBLE, &(motor->integral_gain) );
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		mx_status = mx_put( &(network_motor->derivative_gain_nf),
				MXFT_DOUBLE, &(motor->derivative_gain) );
		break;

	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		mx_status = mx_put(
			&(network_motor->velocity_feedforward_gain_nf),
			MXFT_DOUBLE, &(motor->velocity_feedforward_gain) );
		break;

	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		mx_status = mx_put(
			&(network_motor->acceleration_feedforward_gain_nf),
			MXFT_DOUBLE, &(motor->acceleration_feedforward_gain) );
		break;

	case MXLV_MTR_INTEGRAL_LIMIT:
		mx_status = mx_put( &(network_motor->integral_limit_nf),
			MXFT_DOUBLE, &(motor->integral_limit) );
		break;

	case MXLV_MTR_EXTRA_GAIN:
		mx_status = mx_put( &(network_motor->extra_gain_nf),
			MXFT_DOUBLE, &(motor->extra_gain) );
		break;

	case MXLV_MTR_USE_WINDOW:
		mx_status = mx_put( &(network_motor->use_window_nf),
				MXFT_BOOL, &(motor->use_window) );
		break;

	case MXLV_MTR_WINDOW:
		dimension_array[0] = 2;	/* window start, window end */

		mx_status = mx_put_array( &(network_motor->window_nf),
			MXFT_DOUBLE, 1, dimension_array, motor->window );
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %ld is not supported by this driver.",
			motor->parameter_type );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_motor_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_network_motor_get_status()";

	MX_NETWORK_SERVER *network_server;
	MX_NETWORK_MOTOR *network_motor;
	unsigned long remote_mx_version;
	mx_status_type mx_status;

#if MXD_NETWORK_MOTOR_DEBUG_TIMING
	MX_HRT_TIMING get_status_measurement;
#endif

	mx_status = mxd_network_motor_get_pointers( motor,
						&network_motor, fname );

	if ( mx_status.code != MXE_SUCCESS ) {
		motor->status = 0;

		return mx_status;
	}

#if MXD_NETWORK_MOTOR_DEBUG_TIMING
	MX_HRT_START( get_status_measurement );
#endif

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor->record->name));

	network_server = (MX_NETWORK_SERVER *)
			network_motor->server_record->record_class_struct;

	if ( network_server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_SERVER pointer for MX server '%s' "
		"used by record '%s' is NULL.",
			network_motor->server_record->name,
			motor->record->name );
	}

	if ( network_motor->need_to_get_remote_record_information ) {

		mx_status = mxd_network_motor_get_remote_record_information(
				motor, network_motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	remote_mx_version = network_server->remote_mx_version;

	if ( remote_mx_version < MX_VERSION_HAS_MOTOR_GET_STATUS ) {

		/* Use traditional motor status. */

		mx_status = mx_motor_get_traditional_status( motor );
	} else {
		mx_status = mx_get( &(network_motor->status_nf),
					MXFT_HEX, &( motor->status ) );
	}

#if MXD_NETWORK_MOTOR_DEBUG_TIMING
	MX_HRT_END( get_status_measurement );

	MX_HRT_RESULTS( get_status_measurement, fname, "get status" );
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_motor_get_extended_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_network_motor_get_extended_status()";

	MX_NETWORK_SERVER *network_server;
	MX_NETWORK_MOTOR *network_motor;
	long dimension[1];
	int num_items;
	unsigned long remote_mx_version;
	mx_status_type mx_status;

#if MXD_NETWORK_MOTOR_DEBUG_TIMING
	MX_HRT_TIMING get_extended_status_measurement;
#endif

	mx_status = mxd_network_motor_get_pointers( motor,
						&network_motor, fname );

	if ( mx_status.code != MXE_SUCCESS ) {
		motor->status = 0;

		return mx_status;
	}

#if MXD_NETWORK_MOTOR_DEBUG_TIMING
	MX_HRT_START( get_extended_status_measurement );
#endif

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor->record->name));

	network_server = (MX_NETWORK_SERVER *)
			network_motor->server_record->record_class_struct;

	if ( network_server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_SERVER pointer for MX server '%s' "
		"used by record '%s' is NULL.",
			network_motor->server_record->name,
			motor->record->name );
	}

	if ( network_motor->need_to_get_remote_record_information ) {

		mx_status = mxd_network_motor_get_remote_record_information(
				motor, network_motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	remote_mx_version = network_server->remote_mx_version;

	if ( remote_mx_version < MX_VERSION_HAS_MOTOR_GET_STATUS ) {

		/* Use traditional motor status. */

		mx_status = mx_motor_get_traditional_status( motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_motor_get_position( motor->record, NULL );
	} else {
		dimension[0] = MXU_EXTENDED_STATUS_STRING_LENGTH;

		mx_status = mx_get_array( &(network_motor->extended_status_nf),
					MXFT_STRING, 1, dimension,
					motor->extended_status );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( motor->extended_status, "%lg %lx",
				&(motor->raw_position.analog),
				&(motor->status ) );

		if ( num_items != 2 ) {
			return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The string returned by server '%s' for record field '%s' "
		"was not parseable as an extended status string.  "
		"Returned string = '%s'",
				network_motor->server_record->name,
				"extended_status", motor->extended_status );
		}
	}

#if MXD_NETWORK_MOTOR_DEBUG_TIMING
	MX_HRT_END( get_extended_status_measurement );

	MX_HRT_RESULTS( get_extended_status_measurement, fname,
				"get extended status" );
#endif

	return MX_SUCCESSFUL_RESULT;
}

