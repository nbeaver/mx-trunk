/*
 * Name:    d_e500.c 
 *
 * Purpose: MX motor driver for E500A CAMAC stepping motor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2006 Illinois Institute of Technology
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
#include "d_e500.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_e500_record_function_list = {
	NULL,
	mxd_e500_create_record_structures,
	mxd_e500_finish_record_initialization,
	NULL,
	mxd_e500_print_motor_structure,
	NULL,
	NULL,
	mxd_e500_open,
	mxd_e500_close
};

MX_MOTOR_FUNCTION_LIST mxd_e500_motor_function_list = {
	mxd_e500_motor_is_busy,
	mxd_e500_move_absolute,
	mxd_e500_get_position,
	mxd_e500_set_position,
	mxd_e500_soft_abort,
	mxd_e500_immediate_abort,
	mxd_e500_positive_limit_hit,
	mxd_e500_negative_limit_hit,
	mxd_e500_find_home_position
};

/* E500 motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_e500_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_E500_STANDARD_FIELDS
};

mx_length_type mxd_e500_num_record_fields
		= sizeof( mxd_e500_record_field_defaults )
			/ sizeof( mxd_e500_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_e500_rfield_def_ptr
			= &mxd_e500_record_field_defaults[0];

/* === */

MX_EXPORT mx_status_type
mxd_e500_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_e500_create_record_structures()";

	MX_MOTOR *motor;
	MX_E500 *e500;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	e500 = (MX_E500 *) malloc( sizeof(MX_E500) );

	if ( e500 == (MX_E500 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_E500 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = e500;
	record->class_specific_function_list
				= &mxd_e500_motor_function_list;

	motor->record = record;

	/* An E500 motor is treated as a stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_e500_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxd_e500_finish_record_initialization()";

	MX_E500 *e500;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	e500 = (MX_E500 *) record->record_type_struct;

	if ( e500 == (MX_E500 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
		"MX_E500 pointer for record '%s' is NULL.", record->name);
	}

	/* Verify that 'camac_record' is the correct type of record. */

	if ( e500->camac_record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"'%s' is not an interface record.", e500->camac_record->name );
	}
	if ( e500->camac_record->mx_class != MXI_CAMAC ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"'%s' is not a CAMAC crate.", e500->camac_record->name );
	}

	/* Check the CAMAC slot number. */

	if ( e500->slot < 1 || e500->slot > 23 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"CAMAC slot number %d is out of the allowed range 1-23.",
			e500->slot );
	}

	/* Check the subaddress. */

	if ( e500->subaddress < 1 || e500->subaddress > 8 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"E500 motor number %d is out of the allowed range 1-8.",
			e500->subaddress );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_e500_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_e500_print_motor_structure()";

	MX_MOTOR *motor;
	MX_CAMAC *crate;
	MX_E500 *e500;
	double position, backlash, negative_limit, positive_limit;
	double move_deadband;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
		"MX_MOTOR pointer for record '%s' is NULL.", record->name);
	}

	e500 = (MX_E500 *) (record->record_type_struct);

	if ( e500 == (MX_E500 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
		"MX_E500 pointer for record '%s' is NULL.", record->name);
	}

	crate  = (MX_CAMAC *) (e500->camac_record->record_class_struct);

	if ( crate == (MX_CAMAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
			"MX_CAMAC crate pointer for record '%s' is NULL.",
			record->name);
	}

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type  = E500.\n\n");

	fprintf(file, "  name        = %s\n", record->name);
	fprintf(file, "  crate       = %s\n", crate->record->name);
	fprintf(file, "  slot        = %d\n", e500->slot);
	fprintf(file, "  subaddress  = %d\n", e500->subaddress);

	position = motor->offset + motor->scale 
		* (double)(motor->raw_position.stepper);

	fprintf(file, "  position    = %ld steps (%g %s)\n",
		(long) motor->raw_position.stepper, position, motor->units );

	fprintf(file, "  scale       = %g %s per step.\n",
		motor->scale, motor->units );

	fprintf(file, "  offset      = %g %s.\n",
		motor->offset, motor->units );

	backlash = motor->scale 
		* (double)(motor->raw_backlash_correction.stepper);

	fprintf(file, "  backlash    = %ld steps (%g %s)\n",
		(long) motor->raw_backlash_correction.stepper,
		backlash, motor->units);

	negative_limit = motor->offset + motor->scale
		* (double)(motor->raw_negative_limit.stepper);

	fprintf(file, "  negative limit = %ld steps (%g %s)\n",
		(long) motor->raw_negative_limit.stepper,
		negative_limit, motor->units);

	positive_limit = motor->offset + motor->scale
		* (double)(motor->raw_positive_limit.stepper);

	fprintf(file, "  positive limit = %ld steps (%g %s)\n",
		(long) motor->raw_positive_limit.stepper,
		positive_limit, motor->units);

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  move deadband  = %ld steps (%g %s)\n\n",
		(long) motor->raw_move_deadband.stepper,
		move_deadband, motor->units );

	fprintf(file, "  base speed  = %hu\n", e500->e500_base_speed);
	fprintf(file, "  slew speed  = %u\n", e500->e500_slew_speed);
	fprintf(file, "  accel. time = %hu\n", e500->acceleration_time);
	fprintf(file, "  corr. limit = %hu\n", e500->correction_limit);
	fprintf(file, "  LAM mask    = %d\n", e500->lam_mask);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_e500_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_e500_open()";

	MX_MOTOR *motor;
	MX_E500 *e500;
	mx_status_type mx_status;
	int32_t accumulator;
	bool busy;

	MX_DEBUG(2, ("mxd_e500_write_parms_to_hardware() called."));

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	e500 = (MX_E500 *) (record->record_type_struct);

	if ( e500 == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_E500 structure is NULL!");
	}

	/* Is the motor currently moving?  If so, stop the motor. */

	mx_status = mx_e500_motor_busy( e500, &busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy == TRUE ) {
		mx_status = mx_e500_soft_abort( e500 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_msleep(2000);   /* Wait 2 seconds for the motor to stop. */
	}

	accumulator = (int32_t) (motor->raw_position.stepper);

	mx_status = mx_e500_write_absolute_accumulator( e500, accumulator );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_e500_write_baserate( e500 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_e500_write_pulse_parameter_reg( e500 );
		
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_e500_write_correction_limit( e500 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_e500_write_lam_mask( e500 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_e500_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_e500_close()";

	MX_MOTOR *motor;
	MX_E500 *e500;
	mx_status_type mx_status;
	int32_t accumulator;
	uint16_t actual_baserate;
	bool busy;

	MX_DEBUG( 2,("%s called.", fname));

	motor = (MX_MOTOR *) record->record_class_struct;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	e500 = (MX_E500 *) record->record_type_struct;

	if ( e500 == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_E500 pointer for record '%s' is NULL.", record->name);
	}

	/* Is the motor currently moving?  If so, stop the motor. */

	mx_status = mx_e500_motor_busy( e500, &busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy == TRUE ) {
		mx_status = mx_e500_soft_abort( e500 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_msleep(2000);   /* Wait 2 seconds for the motor to stop. */
	}

	mx_status = mx_e500_read_absolute_accumulator( e500, &accumulator );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->raw_position.stepper = accumulator;

	mx_status = mx_e500_read_baserate( e500, &actual_baserate );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_e500_read_pulse_parameter_reg( e500 );
		
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_e500_read_correction_limit( e500 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_e500_read_lam_mask( e500 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_e500_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_e500_motor_is_busy()";

	MX_E500 *e500;
	bool busy;
	mx_status_type mx_status;

	e500 = (MX_E500 *) (motor->record->record_type_struct);

	if ( e500 == (MX_E500 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_E500 pointer is NULL.");
	}

	mx_status = mx_e500_motor_busy( e500, &busy );

	motor->busy = busy;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_e500_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_e500_move_absolute_steps()";

	MX_E500 *e500;
	int32_t e500_motor_steps;
	mx_status_type mx_status;

	e500 = (MX_E500 *) (motor->record->record_type_struct);

	if ( e500 == (MX_E500 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_E500 pointer is NULL.");
	}

	e500_motor_steps = (int32_t) (motor->raw_destination.stepper);

	mx_status = mx_e500_move_absolute( e500, e500_motor_steps );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_e500_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_e500_get_position()";

	MX_E500 *e500;
	int32_t e500_motor_steps;
	mx_status_type mx_status;

	e500 = (MX_E500 *) (motor->record->record_type_struct);

	if ( e500 == (MX_E500 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_E500 pointer is NULL.");
	}

	mx_status = mx_e500_read_absolute_accumulator( e500, &e500_motor_steps );

	motor->raw_position.stepper = (long) e500_motor_steps;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_e500_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_e500_set_position_steps()";

	MX_E500 *e500;
	int32_t motor_steps;
	mx_status_type mx_status;

	e500 = (MX_E500 *) (motor->record->record_type_struct);

	if ( e500 == (MX_E500 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_E500 pointer is NULL.");
	}

	motor_steps = motor->raw_set_position.stepper;

	mx_status = mx_e500_write_absolute_accumulator( e500, motor_steps );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_e500_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_e500_soft_abort()";

	MX_E500 *e500;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("mxd_e500_soft_abort() called."));

	e500 = (MX_E500 *) (motor->record->record_type_struct);

	if ( e500 == (MX_E500 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_E500 pointer is NULL.");
	}

	mx_status = mx_e500_soft_abort( e500 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_e500_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_e500_immediate_abort()";

	MX_E500 *e500;
	mx_status_type mx_status;

	MX_DEBUG(-2, ("mxd_e500_immediate_abort() called."));

	e500 = (MX_E500 *) (motor->record->record_type_struct);

	if ( e500 == (MX_E500 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_E500 pointer is NULL.");
	}

	mx_status = mx_e500_immediate_abort( e500 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_e500_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_e500_positive_limit_hit()";

	MX_E500 *e500;
	bool limit_hit;
	mx_status_type mx_status;

	e500 = (MX_E500 *) (motor->record->record_type_struct);

	if ( e500 == (MX_E500 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_E500 pointer is NULL.");
	}

	mx_status = mx_e500_cw_limit( e500, &limit_hit );

	motor->positive_limit_hit = limit_hit;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_e500_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_e500_negative_limit_hit()";

	MX_E500 *e500;
	bool limit_hit;
	mx_status_type mx_status;

	e500 = (MX_E500 *) (motor->record->record_type_struct);

	if ( e500 == (MX_E500 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_E500 pointer is NULL.");
	}

	mx_status = mx_e500_ccw_limit( e500, &limit_hit );

	motor->negative_limit_hit = limit_hit;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_e500_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_e500_find_home_position()";

	MX_E500 *e500;
	mx_status_type mx_status;

	e500 = (MX_E500 *) (motor->record->record_type_struct);

	if ( e500 == (MX_E500 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_E500 pointer is NULL.");
	}

	mx_status = mx_e500_go_to_home( e500 );

	return mx_status;
}

/*************** Low-level E500 functions ***************/

/* mx_e500_preserve_csr_bitmap() is a utility function to construct
 * a template command and status register which preserves the previous
 * value of the programmed base rate, correction limit, and lam mask.
 */

MX_EXPORT mx_status_type
mx_e500_preserve_csr_bitmap( MX_E500 *e500, int32_t *csr )
{
	mx_status_type mx_status;
	uint8_t programmed_baserate;
	uint16_t actual_baserate;

	/* Preserve LAM mask, correction limit, and programmed base rate. */

	mx_status = mx_e500_read_command_status_reg( e500, csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	(*csr) &= MX_E500_UPDATE_MASK;

	mx_status = mx_e500_read_baserate(e500, &actual_baserate);

	programmed_baserate = e500->e500_base_speed;

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	programmed_baserate &= 0xFF;

	(*csr) += (programmed_baserate << 16);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_e500_accumulator_overflow( MX_E500 *e500, bool *overflow )
{
	mx_status_type mx_status;
	int32_t csr, overflow_temp;

	mx_status = mx_e500_read_command_status_reg( e500, &csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	overflow_temp = csr & MX_E500_ACCUMULATOR_OVERFLOW;

	if ( overflow_temp ) {
		*overflow = TRUE;
	} else {
		*overflow = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_e500_accumulator_overflow_reset( MX_E500 *e500 )
{
	mx_status_type mx_status;
	int32_t csr;

	mx_status = mx_e500_preserve_csr_bitmap( e500, &csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Or the CSR with the reset bit. */

	csr |= MX_E500_ACCUMULATOR_OVERFLOW;

	/* Reset the overflow. */

	mx_status = mx_e500_write_command_status_reg( e500, csr );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_e500_accumulator_underflow( MX_E500 *e500, bool *underflow )
{
	mx_status_type mx_status;
	int32_t csr, underflow_temp;

	mx_status = mx_e500_read_command_status_reg( e500, &csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	underflow_temp = csr & MX_E500_ACCUMULATOR_UNDERFLOW;

	if ( underflow_temp ) {
		*underflow = TRUE;
	} else {
		*underflow = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_e500_accumulator_underflow_reset( MX_E500 *e500 )
{
	mx_status_type mx_status;
	int32_t csr;

	mx_status = mx_e500_preserve_csr_bitmap( e500, &csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Or the CSR with the reset bit. */

	csr |= MX_E500_ACCUMULATOR_UNDERFLOW;

	/* Reset the underflow. */

	mx_status = mx_e500_write_command_status_reg( e500, csr );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_e500_ccw_limit( MX_E500 *e500, bool *limit_hit )
{
	mx_status_type mx_status;
	int32_t csr, ccw_limit;

	mx_status = mx_e500_read_command_status_reg( e500, &csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ccw_limit = csr & MX_E500_CCW_LIMIT;

	if ( ccw_limit ) {
		*limit_hit = TRUE;
	} else {
		*limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_e500_cw_limit( MX_E500 *e500, bool *limit_hit )
{
	mx_status_type mx_status;
	int32_t csr, cw_limit;

	mx_status = mx_e500_read_command_status_reg( e500, &csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	cw_limit = csr & MX_E500_CW_LIMIT;

	if ( cw_limit ) {
		*limit_hit = TRUE;
	} else {
		*limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_e500_build_file( MX_E500 *e500 )
{
	mx_status_type mx_status;
	int32_t csr;

	mx_status = mx_e500_preserve_csr_bitmap( e500, &csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Or the CSR with the build file bit. */

	csr |= MX_E500_BUILD_FILE;

	/* Start the build. */

	mx_status = mx_e500_write_command_status_reg( e500, csr );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_e500_correction_failure( MX_E500 *e500, bool *correction_failure )
{
	mx_status_type mx_status;
	int32_t csr, correction_failure_temp;

	mx_status = mx_e500_read_command_status_reg( e500, &csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	correction_failure_temp = csr & MX_E500_CORRECTION_FAILURE;

	if ( correction_failure_temp ) {
		*correction_failure = TRUE;
	} else {
		*correction_failure = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_e500_illegal_instruction( MX_E500 *e500, bool *illegal_instruction )
{
	mx_status_type mx_status;
	int32_t csr, illegal_instruction_temp;

	mx_status = mx_e500_read_command_status_reg( e500, &csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	illegal_instruction_temp = csr & MX_E500_ILLEGAL_INSTRUCTION;

	if ( illegal_instruction_temp ) {
		*illegal_instruction = TRUE;
	} else {
		*illegal_instruction = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_e500_immediate_abort( MX_E500 *e500 )
{
	mx_status_type mx_status;
	int32_t csr;

	mx_status = mx_e500_preserve_csr_bitmap( e500, &csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Or the CSR with the immediate abort bit. */

	csr |= MX_E500_IMMEDIATE_ABORT;

	/* Start the abort. */

	mx_status = mx_e500_write_command_status_reg( e500, csr );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_e500_motor_busy( MX_E500 *e500, bool *motor_busy )
{
	mx_status_type mx_status;
	int32_t csr;

	mx_status = mx_e500_read_command_status_reg( e500, &csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( csr & MX_E500_MOTOR_BUSY ) {
		*motor_busy = TRUE;
	} else {
		*motor_busy = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_e500_motor_start( MX_E500 *e500 )
{
	mx_status_type mx_status;
	int32_t csr;

	mx_status = mx_e500_preserve_csr_bitmap( e500, &csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Or the CSR with the motor start bit. */

	csr |= MX_E500_MOTOR_START;

	/* Start the motor. */

	mx_status = mx_e500_write_command_status_reg( e500, csr );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_e500_soft_abort( MX_E500 *e500 )
{
	mx_status_type mx_status;
	int32_t csr;

	mx_status = mx_e500_preserve_csr_bitmap( e500, &csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Or the CSR with the soft abort bit. */

	csr |= MX_E500_SOFT_ABORT;

	/* Start the abort. */

	mx_status = mx_e500_write_command_status_reg( e500, csr );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_e500_go_to_home( MX_E500 *e500 )
{
	static const char fname[] = "mx_e500_go_to_home()";

	int slot, motor, camac_X;
	int32_t data;

	slot  = e500->slot;
	motor = e500->subaddress;

	mx_camac_qwait( e500->camac_record,
		slot, (motor + MX_MOTORS_PER_E500), 25, &data, &camac_X );

	if ( camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"X = 0 for go to home command.");
	}
	return MX_SUCCESSFUL_RESULT;
}

#if 0

MX_EXPORT mx_status_type
mx_e500_simultaneous_start( MX_E500_MODULE *e500_module )
{
	static const char fname[] = "mx_e500_simultaneous_start()";

	int32_t slot, camac_X;
	int32_t data;

	crate = e500_module->crate;
	slot  = e500_module->slot;

	mx_camac_qwait( e500->camac_record, slot, 3, 25, &data, &camac_X );

	if ( camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"X = 0 for simultaneous start command.");
	}
	return MX_SUCCESSFUL_RESULT;
}

#endif

MX_EXPORT mx_status_type
mx_e500_move_absolute( MX_E500 *e500, int32_t steps )
{
	mx_status_type mx_status;
	int32_t accumulator, relative_steps;

	mx_status = mx_e500_read_absolute_accumulator( e500, &accumulator );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	relative_steps = steps - accumulator;

	mx_status = mx_e500_move_relative( e500, relative_steps );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_e500_move_relative( MX_E500 *e500, int32_t steps )
{
	mx_status_type mx_status;

	mx_status = mx_e500_write_relative_magnitude( e500, steps );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_e500_build_file( e500 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_msleep(5);	/* Put in a little time to avoid no-X errors. */

	mx_status = mx_e500_motor_start( e500 );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_e500_read_absolute_accumulator( MX_E500 *e500,
				int32_t *accumulator )
{
	static const char fname[] = "mx_e500_read_absolute_accumulator()";

	int32_t slot, motor, camac_X, data;

	slot  = e500->slot;
	motor = e500->subaddress;

	/* Load the absolute accumulators. */

	mx_camac_qwait( e500->camac_record, slot, 0, 25, &data, &camac_X );

	if ( camac_X == 0 ) {
		*accumulator = 0;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"X = 0 for load absolute accumulators.");
	}

	/* Read out the absolute accumulator. */

	mx_camac_qwait( e500->camac_record,
		slot, motor + MX_MOTORS_PER_E500, 0, &data, &camac_X );

	if ( camac_X == 0 ) {
		*accumulator = 0;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"X = 0 for read absolute accumulator.");
	}

	/* The value returned from camac_qwait() is a 24 bit signed integer.
	 * We need to convert this to a 32bit signed integer.
	 */

	*accumulator = MX_FROM_24_TO_32( data );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_e500_read_baserate( MX_E500 *e500, uint16_t *actual_rate )
{
	static const char fname[] = "mx_e500_read_baserate()";

	int32_t slot, motor, camac_X, data;
	uint8_t  programmed_rate;

	slot  = e500->slot;
	motor = e500->subaddress;

	/* Load the baserates. */

	mx_camac_qwait( e500->camac_record, slot, 4, 25, &data, &camac_X );

	if ( camac_X == 0 ) {
		e500->e500_base_speed = 0;
		*actual_rate = 0;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"X = 0 for load baserates.");
	}

	/* Read out the baserate. */

	mx_camac_qwait( e500->camac_record, slot, motor, 0, &data, &camac_X );

	if ( camac_X == 0 ) {
		e500->e500_base_speed = 0;
		*actual_rate = 0;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"X = 0 for read baserates.");
	}

	programmed_rate = (uint8_t) (data >> 16);   /* high byte */

	*actual_rate = (uint16_t) (data & 0xFFFF);  /* low word */

	/* Save the programmed rate in the E500 structure. */

	e500->e500_base_speed = programmed_rate;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_e500_read_command_status_reg( MX_E500 *e500, int32_t *csr )
{
	static const char fname[] = "mx_e500_read_command_status_reg()";

	int32_t slot, motor, camac_X, data;

	slot  = e500->slot;
	motor = e500->subaddress;

	/* Load the command and status registers. */

	mx_camac_qwait( e500->camac_record, slot, 1, 25, &data, &camac_X );

	if ( camac_X == 0 ) {
		*csr = 0;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"X = 0 for load command and status registers.");
	}

	/* Read out the command and status register. */

	mx_camac_qwait( e500->camac_record, slot, motor, 0, &data, &camac_X );

	if ( camac_X == 0 ) {
		*csr = 0;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"X = 0 for read command and status register.");
	}

	*csr = data;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_e500_read_correction_limit( MX_E500 *e500 )
{
	mx_status_type mx_status;
	int32_t csr, limit_temp;

	mx_status = mx_e500_read_command_status_reg( e500, &csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	limit_temp = csr & 0x00FF00;

	limit_temp >>= 8;

	e500->correction_limit = (uint8_t) limit_temp;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_e500_read_lam_mask( MX_E500 *e500 )
{
	mx_status_type mx_status;
	int32_t csr, lam_mask_temp;

	mx_status = mx_e500_read_command_status_reg( e500, &csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	lam_mask_temp = csr & MX_E500_LAM_MASK;

	if ( lam_mask_temp ) {
		e500->lam_mask = TRUE;
	} else {
		e500->lam_mask = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_e500_read_lam_status( MX_E500 *e500, bool *lam_status )
{
	mx_status_type mx_status;
	int32_t csr, lam_status_temp, motor;

	motor = e500->subaddress;

	mx_status = mx_e500_read_command_status_reg( e500, &csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	lam_status_temp = csr & 0xFF0000;

	lam_status_temp >>= ( 16 + motor );

	lam_status_temp &= 0x01;

	if ( lam_status_temp ) {
		*lam_status = TRUE;
	} else {
		*lam_status = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

#if 0

MX_EXPORT mx_status_type
mx_e500_read_module_id( MX_E500_MODULE *e500_module,
				uint16_t *id, uint8_t *revision )
{
	static const char fname[] = "mx_e500_read_module_id()";

	MX_CAMAC *crate;
	int32_t slot, camac_X, data;

	crate = e500_module->crate;
	slot  = e500_module->slot;

	/* Load the module ID */

	mx_camac_qwait( crate, slot, 2, 25, &data, &camac_X );

	if ( camac_X == 0 ) {
		*id = 0;
		*revision = 0;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Couldn't load E500 module id.");
	}

	/* Read the module ID out. */

	mx_camac_qwait( crate, slot, 0, 1, &data, &camac_X );

	if ( camac_X == 0 ) {
		*id = 0;
		*revision = 0;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Couldn't read out E500 module id.");
	}

	*id = (uint16_t) (data & 0xFFFF);

	*revision = (uint8_t) (data >> 16);

	return MX_SUCCESSFUL_RESULT;
}

#endif

MX_EXPORT mx_status_type
mx_e500_read_pulse_parameter_reg( MX_E500 *e500 )
{
	static const char fname[] = "mx_e500_read_pulse_parameter_reg()";

	int32_t slot, motor, camac_X, data;

	slot  = e500->slot;
	motor = e500->subaddress;

	/* Load the pulse parameter registers. */

	mx_camac_qwait( e500->camac_record, slot, 1, 25, &data, &camac_X );

	if ( camac_X == 0 ) {
		e500->e500_slew_speed = 0;
		e500->acceleration_time = 0;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"X = 0 for load pulse parameter registers.");
	}

	/* Read out the pulse parameter register. */

	mx_camac_qwait( e500->camac_record,
		slot, motor + MX_MOTORS_PER_E500, 0, &data, &camac_X );

	if ( camac_X == 0 ) {
		e500->e500_slew_speed = 0;
		e500->acceleration_time = 0;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"X = 0 for read pulse parameter register.");
	}

	e500->e500_slew_speed = (uint16_t) (data & 0xFFFF);    /* low word */

	e500->acceleration_time = (uint8_t) (data >> 16); /* high byte */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_e500_read_relative_magnitude( MX_E500 *e500,
					int32_t *relative_magnitude )
{
	static const char fname[] = "mx_e500_read_relative_magnitude()";

	int32_t slot, motor, camac_X, data;

	slot  = e500->slot;
	motor = e500->subaddress;

	/* Load the relative magnitudes. */

	mx_camac_qwait( e500->camac_record, slot, 0, 25, &data, &camac_X );

	if ( camac_X == 0 ) {
		*relative_magnitude = 0;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"X = 0 for load relative magnitudes.");
	}

	/* Read out the relative magnitude. */

	mx_camac_qwait( e500->camac_record, slot, motor, 0, &data, &camac_X );

	if ( camac_X == 0 ) {
		*relative_magnitude = 0;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"X = 0 for read magnitude register.");
	}

	/* The value returned from camac_qwait() is a 24 bit signed integer.
	 * We need to convert this to a 32bit signed integer.
	 */

	*relative_magnitude = MX_FROM_24_TO_32( data );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_e500_write_absolute_accumulator( MX_E500 *e500,
					int32_t accumulator )
{
	static const char fname[] = "mx_e500_write_absolute_accumulator()";

	int32_t slot, motor, camac_X;

	slot  = e500->slot;
	motor = e500->subaddress;

	/* Write the absolute accumulator. */

	mx_camac_qwait( e500->camac_record,
		slot, motor + MX_MOTORS_PER_E500, 16, &accumulator, &camac_X );

	if ( camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"X = 0 for write absolute accumulator.");
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_e500_write_baserate( MX_E500 *e500 )
{
	mx_status_type mx_status;
	int32_t csr, saved_correction_limit, saved_lam_mask, baserate;

	baserate = e500->e500_base_speed;

	/* Mask the base rate to 8 bits. */

	baserate &= 0xFF;

	/* Read the command and status register, so that we may preserve
	 * parts of it.
	 */

	mx_status = mx_e500_read_command_status_reg( e500, &csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	saved_correction_limit = csr & 0x00FF00;
	saved_lam_mask = csr & 0x000001;

	/* Create a new command and status register. */

	csr = (baserate << 16) + saved_correction_limit + saved_lam_mask;

	/* Write it to the hardware. */

	mx_status = mx_e500_write_command_status_reg( e500, csr );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_e500_write_command_status_reg( MX_E500 *e500, int32_t csr )
{
	static const char fname[] = "mx_e500_write_command_status_reg()";

	int32_t slot, motor, camac_X;

	slot  = e500->slot;
	motor = e500->subaddress;

	/* Write the command and status register. */

	mx_camac_qwait( e500->camac_record, slot, motor, 17, &csr, &camac_X );

	if ( camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"X = 0 for write command and status register.");
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_e500_write_correction_limit( MX_E500 *e500 )
{
	mx_status_type mx_status;
	int32_t csr, limit, saved_lam_mask;
	uint8_t programmed_baserate;
	uint16_t actual_baserate;

	/* Mask the correction limit to 8 bits and shift it up 8 bits.. */

	limit = e500->correction_limit;

	limit &= 0xFF;

	limit <<= 8;

	/* Read the command and status register, so that we may preserve
	 * the LAM mask.
	 */

	mx_status = mx_e500_read_command_status_reg( e500, &csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	saved_lam_mask = csr & 0x000001;

	/* Read the programmed base rate, so that we may preserve it. */

	mx_status = mx_e500_read_baserate( e500, &actual_baserate );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	programmed_baserate = e500->e500_base_speed;

	/* Create a new command and status register. */

	csr = programmed_baserate;

	csr <<= 16;

	csr += limit + saved_lam_mask;

	/* Write it to the hardware. */

	mx_status = mx_e500_write_command_status_reg( e500, csr );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_e500_write_lam_mask( MX_E500 *e500 )
{
	mx_status_type mx_status;
	bool mask;
	int32_t csr;

	mask = e500->lam_mask;

	mx_status = mx_e500_preserve_csr_bitmap( e500, &csr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Add LAM mask bit if requested. */

	if ( mask ) {
		csr |= MX_E500_LAM_MASK;
	}

	mx_status = mx_e500_write_command_status_reg( e500, csr );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_e500_write_pulse_parameter_reg( MX_E500 *e500 )
{
	static const char fname[] = "mx_e500_write_pulse_parameter_reg()";

	int32_t slot, motor, camac_X, data;
	int32_t slew_rate, accel_decel_time;

	slot  = e500->slot;
	motor = e500->subaddress;

	slew_rate = (int32_t) e500->e500_slew_speed;
	accel_decel_time = e500->acceleration_time;

	/* Mask off extra bits. */

	slew_rate &= 0xFFFF;

	accel_decel_time &= 0xFF;

	data = (accel_decel_time << 16) + slew_rate;

	/* Write the pulse parameter register. */

	mx_camac_qwait( e500->camac_record, 
		slot, motor + MX_MOTORS_PER_E500, 17, &data, &camac_X );

	if ( camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"X = 0 for write pulse parameter register.");
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_e500_write_relative_magnitude( MX_E500 *e500, int32_t magnitude )
{
	static const char fname[] = "mx_e500_write_relative_magnitude()";

	int32_t slot, motor, camac_X;

	slot  = e500->slot;
	motor = e500->subaddress;

	/* Write the relative magnitude. */

	mx_camac_qwait( e500->camac_record,
				slot, motor, 16, &magnitude, &camac_X );

	if ( camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"X = 0 for write magnitude register.");
	}
	return MX_SUCCESSFUL_RESULT;
}

