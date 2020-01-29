/*
 * Name:    d_smc24.c
 *
 * Purpose: MX motor driver for Joerger SMC24 CAMAC stepping motor controller.
 *
 * This controller does not have an internal register to record its 
 * current position, so it needs the assistance of an external device
 * to keep track of the motor's absolute position.  Traditionally,
 * a Kinetic Systems 3640 CAMAC up/down counter is used as the external
 * device, but any device capable of acting as an encoder-like device
 * may be used as long as there is an MX_ENCODER driver for it.
 *
 * The driver can also emulate in software a 32-bit step counter using
 * a 16-bit hardware encoder by setting the bit in the "flags" variable
 * called MXF_SMC24_USE_32BIT_SOFTWARE_COUNTER.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003-2006, 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_camac.h"
#include "mx_encoder.h"
#include "d_smc24.h"

/*** The prototypes of several static helper functions are included here. ***/

static mx_status_type smc24_read_status_register(
	MX_SMC24 *smc24, int32_t *status_register );

/* smc24_update_32bit_software_encoder_position() is used to aid in the 
 * update of the 32-bit software counter (if used).
 */

static mx_status_type smc24_update_32bit_software_encoder_position(
	MX_SMC24 *smc24, int32_t *software_encoder_position );

/* smc24_cw_ccw_pulses_soft_abort() is used to handle a bug in the SMC24
 * hardware that occurs when the SMC24 is put into pause mode.  The bug
 * is that pulses continue to be sent out the CW and CCW lines at the base
 * speed even though the motor is not actually moving.
 */
static mx_status_type smc24_cw_ccw_pulses_soft_abort( MX_MOTOR *motor );
static mx_status_type smc24_normal_soft_abort( MX_MOTOR *motor );

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_smc24_record_function_list = {
	NULL,
	mxd_smc24_create_record_structures,
	mxd_smc24_finish_record_initialization,
	NULL,
	mxd_smc24_print_motor_structure
};

MX_MOTOR_FUNCTION_LIST mxd_smc24_motor_function_list = {
	mxd_smc24_motor_is_busy,
	mxd_smc24_move_absolute,
	mxd_smc24_get_position,
	mxd_smc24_set_position,
	mxd_smc24_soft_abort,
	mxd_smc24_immediate_abort,
	mxd_smc24_positive_limit_hit,
	mxd_smc24_negative_limit_hit,
	NULL,
	NULL,
	mx_motor_default_get_parameter_handler,
	mx_motor_default_set_parameter_handler
};

MX_RECORD_FIELD_DEFAULTS mxd_smc24_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_SMC24_STANDARD_FIELDS
};

long mxd_smc24_num_record_fields
		= sizeof( mxd_smc24_record_field_defaults )
			/ sizeof( mxd_smc24_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_smc24_rfield_def_ptr
			= &mxd_smc24_record_field_defaults[0];

static mx_status_type mxd_smc24_steps_to_go( MX_MOTOR *motor,
							long *steps_to_go );
static mx_status_type mxd_smc24_update_position( MX_MOTOR *motor );

MX_EXPORT mx_status_type
mxd_smc24_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_smc24_create_record_structures()";

	MX_MOTOR *motor;
	MX_SMC24 *smc24;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	smc24 = (MX_SMC24 *) malloc( sizeof(MX_SMC24) );

	if ( smc24 == (MX_SMC24 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SMC24 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = smc24;
	record->class_specific_function_list
				= &mxd_smc24_motor_function_list;

	motor->record = record;

	/* An SMC24 motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_smc24_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxd_smc24_finish_record_initialization()";

	MX_SMC24 *smc24;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	smc24 = (MX_SMC24 *) record->record_type_struct;

	if ( smc24->slot < 1 || smc24->slot > 23 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"CAMAC slot number %ld is out of allowed range 1-23.",
			smc24->slot );
	}
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_smc24_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_smc24_print_motor_structure()";

	MX_MOTOR *motor;
	MX_SMC24 *smc24;
	long motor_steps, encoder_ticks;
	double position, backlash;
	double negative_limit, positive_limit, move_deadband;
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

	smc24 = (MX_SMC24 *) (record->record_type_struct);

	if ( smc24 == (MX_SMC24 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SMC24 pointer for record '%s' is NULL.", record->name );
	}

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type  = SMC24.\n\n");

	fprintf(file, "  name        = %s\n", record->name);
	fprintf(file, "  crate       = %s\n", smc24->crate_record->name);
	fprintf(file, "  slot        = %ld\n", smc24->slot);
	fprintf(file, "  encoder     = %s\n", smc24->encoder_record->name);

	mx_status = mx_encoder_read( smc24->encoder_record, &encoder_ticks );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Unable to read encoder value for encoder '%s'",
			smc24->encoder_record->name );
	}

	motor_steps = mx_round( smc24->motor_steps_per_encoder_tick
		* (double) encoder_ticks );

	position = motor->offset + motor->scale
		* smc24->motor_steps_per_encoder_tick
		* (double) encoder_ticks;

	fprintf(file, "  position    = %ld steps (%g %s)\n",
		motor_steps, position, motor->units);
	fprintf(file, "  scale       = %g %s per step.\n",
		motor->scale, motor->units);
	fprintf(file, "  offset      = %g %s.\n",
		motor->offset, motor->units);

	backlash = motor->scale
		* (double) (motor->raw_backlash_correction.stepper);

	fprintf(file, "  backlash    = %ld steps (%g %s)\n",
		motor->raw_backlash_correction.stepper,
		backlash, motor->units);

	negative_limit = motor->offset + motor->scale
		* (double)(motor->raw_negative_limit.stepper);

	fprintf(file, "  negative limit = %ld steps (%g %s)\n",
	    motor->raw_negative_limit.stepper, negative_limit, motor->units);

	positive_limit = motor->offset + motor->scale
		* (double)(motor->raw_positive_limit.stepper);

	fprintf(file, "  positive limit = %ld steps (%g %s)\n",
	    motor->raw_positive_limit.stepper, positive_limit, motor->units);

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  move deadband  = %ld steps (%g %s)\n",
		motor->raw_move_deadband.stepper, move_deadband, motor->units);

	fprintf(file, "  flags       = 0x%lx\n", smc24->flags);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_smc24_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smc24_motor_is_busy()";

	MX_SMC24 *smc24;
	int32_t status_register, not_active;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MOTOR pointer passed is NULL." );
	}

	smc24 = (MX_SMC24 *) (motor->record->record_type_struct);

	if ( smc24 == (MX_SMC24 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SMC24 pointer for motor is NULL." );
	}

	/* Read the SMC24 motor status register. */

	mx_status = smc24_read_status_register( smc24, &status_register );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}
	
	/* Select the active bit from the status register. */

	not_active = status_register & 0x8;

	if ( not_active ) {
		motor->busy = FALSE;
	} else {
		motor->busy = TRUE;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_smc24_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smc24_move_absolute()";

	MX_SMC24 *smc24;
	int32_t relative_steps;
	long new_position;
	long current_position;
	int camac_Q, camac_X;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MOTOR pointer passed is NULL." );
	}

	smc24 = (MX_SMC24 *) (motor->record->record_type_struct);

	if ( smc24 == (MX_SMC24 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SMC24 pointer for motor is NULL." );
	}

	new_position = motor->raw_destination.stepper;

	/* Get the current position. */

	mx_status = mx_motor_get_position_steps(
				motor->record, &current_position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}

	relative_steps = (int32_t) ( new_position - current_position );

	/* Send the move relative command. */

	mx_camac( smc24->crate_record,
		smc24->slot, 0, 16, &relative_steps, &camac_Q, &camac_X );

	if ( camac_X != 1 || camac_Q != 1 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	"Attempt to move SMC24 motor failed.  N = %ld, Q = %d, X = %d",
			smc24->slot, camac_Q, camac_X );
	}
	
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_smc24_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smc24_get_position_steps()";

	MX_SMC24 *smc24;
	long motor_steps, encoder_ticks;
	mx_status_type mx_status;

	smc24 = (MX_SMC24 *) (motor->record->record_type_struct);

	if ( smc24 == (MX_SMC24 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SMC24 pointer for record '%s' is NULL.",
			motor->record->name );
	}

#if 1
	mx_status = mxd_smc24_update_position( motor );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}
#endif

	mx_status = mx_encoder_read( smc24->encoder_record, &encoder_ticks );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}

	motor_steps = mx_round( smc24->motor_steps_per_encoder_tick
			* (double) encoder_ticks );

	motor->raw_position.stepper = motor_steps;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_smc24_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smc24_set_position_steps()";

	MX_SMC24 *smc24;
	long motor_steps, encoder_ticks;
	mx_status_type mx_status;

	smc24 = (MX_SMC24 *) (motor->record->record_type_struct);

	if ( smc24 == (MX_SMC24 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SMC24 pointer for record '%s' is NULL.",
			motor->record->name );
	}

	motor_steps = motor->raw_set_position.stepper;

	if (fabs( smc24->motor_steps_per_encoder_tick ) < MX_MOTOR_STEP_FUZZ){
		encoder_ticks = 0;
	} else {
		encoder_ticks = mx_round( ( (double) motor_steps ) /
			smc24->motor_steps_per_encoder_tick );
	}

	mx_status = mx_encoder_write( smc24->encoder_record, encoder_ticks );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_smc24_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smc24_soft_abort()";

	MX_SMC24 *smc24;
	mx_bool_type busy;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MOTOR pointer passed is NULL." );
	}

	smc24 = (MX_SMC24 *) (motor->record->record_type_struct);

	if ( smc24 == (MX_SMC24 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SMC24 pointer for motor is NULL." );
	}

	/* Is the motor currently moving? */

	mx_status = mx_motor_is_busy( motor->record, &busy );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}

	/* Don't need to do anything if the motor is not moving. */

	if ( busy == FALSE ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Otherwise, we have to stop the motor. */

	if ( (smc24->flags & MXF_SMC24_USE_CW_CCW_MOTOR_PULSES) == FALSE ) {

		mx_status = smc24_normal_soft_abort( motor );
	} else {

		mx_status = smc24_cw_ccw_pulses_soft_abort( motor );
	}
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_smc24_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smc24_immediate_abort()";

	MX_SMC24 *smc24;
	int32_t data;
	int camac_Q, camac_X;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MOTOR pointer passed is NULL." );
	}

	smc24 = (MX_SMC24 *) (motor->record->record_type_struct);

	if ( smc24 == (MX_SMC24 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SMC24 pointer for motor is NULL." );
	}

	mx_camac( smc24->crate_record,
			smc24->slot, 0, 25, &data, &camac_Q, &camac_X );

	if ( camac_X != 1 || camac_Q != 1 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Immediate abort of SMC24 was unsuccessful." );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_smc24_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smc24_positive_limit_hit()";

	MX_SMC24 *smc24;
	int32_t status_register;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MOTOR pointer passed is NULL." );
	}

	smc24 = (MX_SMC24 *) (motor->record->record_type_struct);

	if ( smc24 == (MX_SMC24 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SMC24 pointer for motor is NULL." );
	}

	mx_status = smc24_read_status_register( smc24, &status_register );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}

	if ( status_register & 0x2 ) {
		motor->positive_limit_hit = TRUE;
	} else {
		motor->positive_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_smc24_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smc24_negative_limit_hit()";

	MX_SMC24 *smc24;
	int32_t status_register;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MOTOR pointer passed is NULL." );
	}

	smc24 = (MX_SMC24 *) (motor->record->record_type_struct);

	if ( smc24 == (MX_SMC24 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SMC24 pointer for motor is NULL." );
	}

	mx_status = smc24_read_status_register( smc24, &status_register );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}

	if ( status_register & 0x4 ) {
		motor->negative_limit_hit = TRUE;
	} else {
		motor->negative_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ======== Private internal functions of the SMC-24 driver ======== */

static mx_status_type
mxd_smc24_steps_to_go( MX_MOTOR *motor, long *steps_to_go )
{
	static const char fname[] = "mxd_smc24_steps_to_go()";

	MX_SMC24 *smc24;
	int32_t data;
	int camac_Q, camac_X;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MOTOR pointer passed is NULL." );
	}

	smc24 = (MX_SMC24 *) (motor->record->record_type_struct);

	if ( smc24 == (MX_SMC24 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SMC24 pointer for motor is NULL." );
	}

	/* Read the SMC-24's counter register. */

	mx_camac( smc24->crate_record,
			smc24->slot, 0, 0, &data, &camac_Q, &camac_X );

	if ( camac_X != 1 || camac_Q != 1 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	"Unable to read SMC24's counter register.  N = %ld, Q = %d, X = %d",
			smc24->slot, camac_Q, camac_X );
	}

	/* The value returned from mx_camac() is a 24 bit signed integer.
	 * We need to convert this to a 32bit signed integer.
	 */

	data = MX_FROM_24_TO_32( data );

	*steps_to_go = (long) data;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_smc24_update_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smc24_update_position()";

	MX_SMC24 *smc24;
	int32_t software_encoder_position;
	double position;
	long motor_steps, encoder_ticks;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MOTOR pointer passed is NULL." );
	}

	smc24 = (MX_SMC24 *) (motor->record->record_type_struct);

	if ( smc24 == (MX_SMC24 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SMC24 pointer for motor is NULL." );
	}

	if ( (smc24->flags) & MXF_SMC24_USE_32BIT_SOFTWARE_COUNTER ) {
		if ( fabs( smc24->motor_steps_per_encoder_tick )
			< MX_MOTOR_STEP_FUZZ ) {
			software_encoder_position = 0;
		} else {
			software_encoder_position = (int32_t)
				( ((double) motor->raw_position.stepper)
				  / smc24->motor_steps_per_encoder_tick );
		}

		mx_status = smc24_update_32bit_software_encoder_position(
					smc24, &software_encoder_position );

		if ( mx_status.code != MXE_SUCCESS ) {
			return mx_status;
		}

		motor->raw_position.stepper
			= mx_round( smc24->motor_steps_per_encoder_tick
				* (double) software_encoder_position );

	} else {
		if ( fabs( smc24->motor_steps_per_encoder_tick )
						< MX_MOTOR_STEP_FUZZ ) {
			motor_steps = 0;
		} else {
			mx_status = mx_encoder_read( smc24->encoder_record,
							&encoder_ticks );

			if ( mx_status.code != MXE_SUCCESS ) {
				return mx_status;
			}

			position = motor->offset
				+ motor->scale
				* (smc24->motor_steps_per_encoder_tick)
				* (double) encoder_ticks;

			motor_steps = mx_round( position );
		}
		motor->raw_position.stepper = motor_steps;
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
smc24_read_status_register( MX_SMC24 *smc24, int32_t *status_register )
{
	static const char fname[] = "smc24_read_status_register()";

	int camac_Q, camac_X;

	/* Read the SMC24 motor status register. */

	mx_camac( smc24->crate_record,
		smc24->slot, 1, 0, status_register, &camac_Q, &camac_X );

	if ( camac_X != 1 || camac_Q != 1 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	"Failed to get SMC24 motor status register.  N = %ld, Q = %d, X = %d",
			smc24->slot, camac_Q, camac_X );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* If software position counters are being used for SMC24's, the function
 * smc24_update_32bit_software_encoder_position() is called to attempt to 
 * reconcile the software position with the value in the corresponding 
 * 16 bit up/down counter.  This reconciliation will not do the right thing
 * if one of the following has happened since the last reconciliation:
 *
 * 1.  The up/down counter has overflowed more than once.
 *
 * 2.  The up/down counter has underflowed more than once.
 *
 * 3.  The up/down counter has both overflowed and underflowed.
 *
 * The software has no way of knowing if any of these three situations
 * has happened, so you are generally better off if you can avoid using
 * 16 bit up/down counters.
 */

static mx_status_type
smc24_update_32bit_software_encoder_position(
	MX_SMC24 *smc24, int32_t *software_encoder_position )
{
	long hardware_encoder_position;
	int32_t hardware_encoder_32bit_position;
	int overflow, underflow;
	mx_status_type mx_status;

	mx_status = mx_encoder_read( smc24->encoder_record,
					&hardware_encoder_position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}

	hardware_encoder_32bit_position = (int32_t) hardware_encoder_position;

	/* Force the least significant 16 bits to agree. */

	*software_encoder_position -= ( *software_encoder_position & 0xFFFF );

	*software_encoder_position += hardware_encoder_32bit_position;

	/* Check for overflow and underflow. */

	mx_status = mx_encoder_get_overflow_status( smc24->encoder_record,
						&underflow, &overflow );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}

	if ( overflow && underflow ) {
		/* Can't handle this situation, so do nothing. */
	} else if ( overflow ) {
		*software_encoder_position += 65536;
	} else if ( underflow ) {
		*software_encoder_position -= 65536;
	}

	/* Clear the underflow and overflow status for this encoder. */

	mx_status = mx_encoder_reset_overflow_status( smc24->encoder_record );

	return mx_status;
}

/*
 *  smc24_soft_abort() attempts to abort the motion of an SMC24 controlled
 *  motor without losing calibration in the following way:
 *
 *  1.  If the motor is not moving, just return.
 *
 *  2.  If the motor is moving, see how far it has left to move.
 *
 *  3.  If the motor has less than ABORT_RAMPDOWN_DISTANCE steps
 *      left to move, do nothing and let the motor stop normally.
 *  
 *  4.  If the motor has more than ABORT_RAMPDOWN_DISTANCE steps
 *      left to move, send the motor a new move command of 
 *      ABORT_RAMPDOWN_DISTANCE steps in the same direction that
 *      it was already going in.  The new move command will override
 *      the old move command and the motor will stop after 
 *      ABORT_RAMPDOWN_DISTANCE steps.  Joerger says that this
 *      should only lose or gain about 1 step.
 */

#define ABORT_RAMPDOWN_DISTANCE       1000	/* in motor steps */

#define ABORT_RAMPDOWN_THINKING_TIME     0	/* in motor steps */

static mx_status_type
smc24_cw_ccw_pulses_soft_abort( MX_MOTOR *motor )
{
	long steps_to_go;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warning. */

	steps_to_go = 0L;

	/* How far does the motor have left to go? */

	mx_status = mxd_smc24_steps_to_go( motor, &steps_to_go );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Do not do the abort if the motor was going to stop soon anyway.
	 * ABORT_RAMPDOWN_THINKING_TIME reflects how far the motor moves 
	 * between the time we do the mxd_smc24_steps_to_go() function and 
	 * the time we do the mx_motor_move_relative_steps_with_report()
	 * function.
	 */

	if ( labs(steps_to_go) 
		< (ABORT_RAMPDOWN_DISTANCE + ABORT_RAMPDOWN_THINKING_TIME)) {

		return MX_SUCCESSFUL_RESULT;	/* No need to do anything. */
	}

	if ( steps_to_go >= 0 ) {
		mx_status = mx_motor_move_relative_steps_with_report(
			motor->record, ABORT_RAMPDOWN_DISTANCE, NULL, 0 );
	} else {
		mx_status = mx_motor_move_relative_steps_with_report(
			motor->record, - ABORT_RAMPDOWN_DISTANCE, NULL, 0 );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
smc24_normal_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "smc24_normal_soft_abort()";

	MX_SMC24 *smc24;
	long old_encoder_position, encoder_position;
	int num_constant_measurements, timeout, naptime;
	int32_t data;
	int camac_Q, camac_X;
	mx_status_type mx_status;

	smc24 = (MX_SMC24 *) (motor->record->record_type_struct);

	/* Tell the motor to begin pause mode. */

	mx_camac( smc24->crate_record,
			smc24->slot, 0, 24, &data, &camac_Q, &camac_X );

	if ( camac_X != 1 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Attempt to enter pause mode failed." );
	}

	/* Wait for the motor to stop moving. */

	timeout = 10000;	/* milliseconds */
	naptime = 10;		/* milliseconds */

	num_constant_measurements = 0;

	mx_status = mx_encoder_read( smc24->encoder_record, &encoder_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	old_encoder_position = encoder_position;

	while ( num_constant_measurements < 3 && timeout > 0 ) {

		mx_msleep( naptime );	/* Wait 0.1 seconds */

		timeout -= naptime;

		mx_status = mx_encoder_read( smc24->encoder_record,
						&encoder_position );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( encoder_position == old_encoder_position ) {
			num_constant_measurements++;
		} else {
			num_constant_measurements = 0;
		}

		old_encoder_position = encoder_position;
	}

	/* Tell the motor to move 0 steps. */

	mx_status = mx_motor_move_relative_steps_with_report(
					motor->record, 0, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( timeout <= 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Timed out waiting for motor to stop moving." );
	}

	return MX_SUCCESSFUL_RESULT;
}

