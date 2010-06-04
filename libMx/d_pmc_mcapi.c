/*
 * Name:    d_pmc_mcapi.c
 *
 * Purpose: MX driver for Precision MicroControl MCAPI controlled motors.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004, 2008, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PMC_MCAPI_MOTOR_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mxconfig.h"

#if HAVE_PMC_MCAPI

#if defined(OS_LINUX)
  #include "mcapi.h"

  #define HAVE_MCDLG	FALSE

#elif defined(OS_WIN32)
  #include "windows.h"

  #define HAVE_MCDLG	TRUE

  /* Vendor include files */

  #ifndef _WIN32
  #define _WIN32
  #endif

  #include "Mcapi.h"
#else
  #error The MX PMC MCAPI drivers have not been configured for this platform.
#endif

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "i_pmc_mcapi.h"
#include "d_pmc_mcapi.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_pmc_mcapi_record_function_list = {
	NULL,
	mxd_pmc_mcapi_create_record_structures,
	mxd_pmc_mcapi_finish_record_initialization,
	NULL,
	mxd_pmc_mcapi_print_structure,
	NULL,
	NULL,
	mxd_pmc_mcapi_open,
	NULL,
	NULL,
	mxd_pmc_mcapi_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_pmc_mcapi_motor_function_list = {
	NULL,
	mxd_pmc_mcapi_move_absolute,
	mxd_pmc_mcapi_get_position,
	mxd_pmc_mcapi_set_position,
	mxd_pmc_mcapi_soft_abort,
	mxd_pmc_mcapi_immediate_abort,
	NULL,
	NULL,
	mxd_pmc_mcapi_find_home_position,
	mxd_pmc_mcapi_constant_velocity_move,
	mxd_pmc_mcapi_get_parameter,
	mxd_pmc_mcapi_set_parameter,
#if 0
	mxd_pmc_mcapi_simultaneous_start,
#else
	NULL,
#endif
	mxd_pmc_mcapi_get_status
};

/* PMC motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_pmc_mcapi_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_PMC_MCAPI_MOTOR_STANDARD_FIELDS
};

long mxd_pmc_mcapi_num_record_fields
		= sizeof( mxd_pmc_mcapi_record_field_defaults )
			/ sizeof( mxd_pmc_mcapi_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pmc_mcapi_rfield_def_ptr
			= &mxd_pmc_mcapi_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_pmc_mcapi_get_pointers( MX_MOTOR *motor,
			MX_PMC_MCAPI_MOTOR **pmc_mcapi_motor,
			MX_PMC_MCAPI **pmc_mcapi,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pmc_mcapi_get_pointers()";

	MX_PMC_MCAPI_MOTOR *pmc_mcapi_motor_pointer;
	MX_RECORD *pmc_mcapi_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	pmc_mcapi_motor_pointer = (MX_PMC_MCAPI_MOTOR *)
				motor->record->record_type_struct;

	if ( pmc_mcapi_motor_pointer == (MX_PMC_MCAPI_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PMC_MCAPI_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( pmc_mcapi_motor != (MX_PMC_MCAPI_MOTOR **) NULL ) {
		*pmc_mcapi_motor = pmc_mcapi_motor_pointer;
	}

	if ( pmc_mcapi != (MX_PMC_MCAPI **) NULL ) {
		pmc_mcapi_record =
			pmc_mcapi_motor_pointer->pmc_mcapi_record;

		if ( pmc_mcapi_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The pmc_mcapi_record pointer for record '%s' is NULL.",
				motor->record->name );
		}

		*pmc_mcapi = (MX_PMC_MCAPI *)
				pmc_mcapi_record->record_type_struct;

		if ( *pmc_mcapi == (MX_PMC_MCAPI *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PMC_MCAPI pointer for record '%s' is NULL.",
				motor->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_pmc_mcapi_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_pmc_mcapi_create_record_structures()";

	MX_MOTOR *motor;
	MX_PMC_MCAPI_MOTOR *pmc_mcapi_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MOTOR structure." );
	}

	pmc_mcapi_motor = (MX_PMC_MCAPI_MOTOR *)
				malloc( sizeof(MX_PMC_MCAPI_MOTOR) );

	if ( pmc_mcapi_motor == (MX_PMC_MCAPI_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_PMC_MCAPI_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = pmc_mcapi_motor;
	record->class_specific_function_list
				= &mxd_pmc_mcapi_motor_function_list;

	motor->record = record;
	pmc_mcapi_motor->record = record;

	/* A PMC MCAPI motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	/* PMC MCAPI reports acceleration in units/sec**2. */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_pmc_mcapi_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_PMC_MCAPI *pmc_mcapi = NULL;
	MX_PMC_MCAPI_MOTOR *pmc_mcapi_motor = NULL;
	mx_status_type mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_pmc_mcapi_get_pointers( motor, &pmc_mcapi_motor,
						&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_pmc_mcapi_print_structure()";

	MCAXISCONFIG axis_config;
	MCMOTIONEX motion_config;
	MCFILTEREX filter_config;

	MX_MOTOR *motor;
	MX_PMC_MCAPI_MOTOR *pmc_mcapi_motor = NULL;
	MX_PMC_MCAPI *pmc_mcapi = NULL;
	double position, move_deadband, speed;
	long mcapi_status;
	char error_buffer[100];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_pmc_mcapi_get_pointers( motor, &pmc_mcapi_motor,
						&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type         = PMC_MCAPI_MOTOR motor.\n\n");

	fprintf(file, " MX parameters:\n" );
	fprintf(file, "  name               = %s\n", record->name);
	fprintf(file, "  controller name    = %s\n",
					pmc_mcapi->record->name);
	fprintf(file, "  axis number        = %ld\n",
					pmc_mcapi_motor->axis_number);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}
	
	fprintf(file, "  position           = %g %s  (%g raw units)\n",
			motor->position, motor->units,
			motor->raw_position.analog );
	fprintf(file, "  scale              = %g %s per step.\n",
			motor->scale, motor->units);
	fprintf(file, "  offset             = %g %s.\n",
			motor->offset, motor->units);
	
	fprintf(file, "  backlash           = %g %s  (%g raw units)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog );
	
	fprintf(file, "  negative limit     = %g %s  (%g raw units)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog );

	fprintf(file, "  positive limit     = %g %s  (%g raw units)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog );

	move_deadband = motor->scale * (double)motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband      = %g %s  (%g raw units)\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	mx_status = mx_motor_get_speed( record, &speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  speed              = %g %s/s  (%g steps/s)\n",
		speed, motor->units,
		motor->raw_speed );

	fprintf(file, "\n");

	/* Display MCAPI internal parameters for the motor. */

	/* Get the axis configuration parameters. */

	axis_config.cbSize = sizeof(MCAXISCONFIG);

	mcapi_status = MCGetAxisConfiguration( pmc_mcapi->binary_handle,
					pmc_mcapi_motor->axis_number,
					&axis_config );

	if ( mcapi_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( mcapi_status,
			error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to get the axis configuration for axis %lu of "
		"MCAPI controller '%s'.  MCAPI error message = '%s'",
			pmc_mcapi_motor->axis_number,
			pmc_mcapi->record->name,
			error_buffer );
	}

	/* If this axis is not actually a motor, then something is wrong. */

	if ( ( axis_config.MotorType != MC_TYPE_SERVO )
	  && ( axis_config.MotorType != MC_TYPE_STEPPER ) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"MCAPI says that record '%s' is not a motor, even though "
		"MX says that it is.\n",  record->name );
	}

	fprintf(file, " MCAXISCONFIG parameters:\n" );
	fprintf(file, "  ModuleType         = '%s'\n",
		mxi_pmc_mcapi_module_type_name( axis_config.ModuleType ) );

	switch( axis_config.MotorType ) {
	case MC_TYPE_SERVO:
		fprintf(file, "  MotorType          = 'servo'\n" );
		break;
	case MC_TYPE_STEPPER:
		fprintf(file, "  MotorType          = 'stepper'\n" );
		break;
	default:
		fprintf(file, "  MotorType          = 'unknown (%d)'\n",
					axis_config.MotorType );
		break;
	}

	fprintf(file, "  CaptureModes       = %#x\n",
					axis_config.CaptureModes );

	if ( axis_config.CaptureAndCompare ) {
		fprintf(file, "  CaptureAndCompare  is supported\n" );
	} else {
		fprintf(file, "  CaptureAndCompare  is not supported\n" );
	}

	fprintf(file, "  HighRate           = %f\n", axis_config.HighRate );
	fprintf(file, "  MediumRate         = %f\n", axis_config.MediumRate );
	fprintf(file, "  LowRate            = %f\n", axis_config.LowRate );
	fprintf(file, "  HighStepMin        = %f\n", axis_config.HighStepMin );
	fprintf(file, "  HighStepMax        = %f\n", axis_config.HighStepMax );
	fprintf(file, "  MediumStepMin      = %f\n", axis_config.MediumStepMin);
	fprintf(file, "  MediumStepMax      = %f\n", axis_config.MediumStepMax);
	fprintf(file, "  LowStepMin         = %f\n", axis_config.LowStepMin );
	fprintf(file, "  LowStepMax         = %f\n", axis_config.LowStepMax );

	fprintf(file, "\n");

	/* Get the motion parameters. */

	motion_config.cbSize = sizeof(MCMOTIONEX);

	mcapi_status = MCGetMotionConfigEx( pmc_mcapi->binary_handle,
				pmc_mcapi_motor->axis_number,
				&motion_config );

	if ( mcapi_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( mcapi_status,
			error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to get the motion configuration for axis %lu of "
		"MCAPI controller '%s'.  MCAPI error message = '%s'",
			pmc_mcapi_motor->axis_number,
			pmc_mcapi->record->name, error_buffer );
	}

	fprintf(file, " MCMOTIONEX parameters:\n" );
	fprintf(file, "  Acceleration       = %f\n",
					motion_config.Acceleration );
	fprintf(file, "  Deceleration       = %f\n",
					motion_config.Deceleration );
	fprintf(file, "  Velocity           = %f\n",
					motion_config.Velocity );
	fprintf(file, "  MinVelocity        = %f\n",
					motion_config.MinVelocity );

	switch( motion_config.Direction ) {
	case MC_DIR_POSITIVE:
		fprintf(file, "  Direction          = positive\n" );
		break;
	case MC_DIR_NEGATIVE:
		fprintf(file, "  Direction          = negative\n" );
		break;
	default:
		fprintf(file, "  Direction          = %d\n",
					motion_config.Direction );
		break;
	}

	fprintf(file, "  Torque             = %f\n",
					motion_config.Torque );
	fprintf(file, "  Deadband           = %f\n",
					motion_config.Deadband );
	fprintf(file, "  DeadbandDelay      = %f\n",
					motion_config.DeadbandDelay );

	switch( motion_config.StepSize ) {
	case MC_STEP_FULL:
		fprintf(file, "  StepSize           = full step\n" );
		break;
	case MC_STEP_HALF:
		fprintf(file, "  StepSize           = half step\n" );
		break;
	default:
		fprintf(file, "  StepSize           = %d\n",
					motion_config.StepSize );
		break;
	}

	switch( motion_config.Current ) {
	case MC_CURRENT_FULL:
		fprintf(file, "  Current            = full current\n" );
		break;
	case MC_CURRENT_HALF:
		fprintf(file, "  Current            = half current\n" );
		break;
	default:
		fprintf(file, "  Current            = %d\n",
					motion_config.Current );
		break;
	}

	fprintf(file, "  Current            = %d\n",
					motion_config.Current );
	fprintf(file, "  HardLimitMode      = %#x\n",
					motion_config.HardLimitMode );
	fprintf(file, "  SoftLimitMode      = %#x\n",
					motion_config.SoftLimitMode );
	fprintf(file, "  SoftLimitLow       = %f\n",
					motion_config.SoftLimitLow );
	fprintf(file, "  SoftLimitHigh      = %f\n",
					motion_config.SoftLimitHigh );

	if ( motion_config.EnableAmpFault ) {
		fprintf(file, "  EnableAmpFault     = on\n" );
	} else {
		fprintf(file, "  EnableAmpFault     = on\n" );
	}

	fprintf(file, "\n");

	/* Get the PID filter parameters. */

	filter_config.cbSize = sizeof(MCFILTEREX);

	mcapi_status = MCGetFilterConfigEx( pmc_mcapi->binary_handle,
					pmc_mcapi_motor->axis_number,
					&filter_config );

	if ( mcapi_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( mcapi_status,
			error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to get the PID filter configuration for axis %lu of "
		"MCAPI controller '%s'.  MCAPI error message = '%s'",
			pmc_mcapi_motor->axis_number,
			pmc_mcapi->record->name, error_buffer );
	}

	fprintf(file, " MCFILTEREX parameters:\n");
	fprintf(file, "  Gain               = %f\n", filter_config.Gain );
	fprintf(file, "  IntegralGain       = %f\n",
					filter_config.IntegralGain );
	fprintf(file, "  IntegrationLimit   = %f\n",
					filter_config.IntegrationLimit );

	switch( filter_config.IntegralOption ) {
	case MC_INT_NORMAL:
		fprintf(file, "  IntegralOption     = normal\n" );
		break;
	case MC_INT_FREEZE:
		fprintf(file, "  IntegralOption     = freeze\n" );
		break;
	case MC_INT_ZERO:
		fprintf(file, "  IntegralOption     = zero\n" );
		break;
	default:
		fprintf(file, "  IntegralOption     = %d\n",
					filter_config.IntegralOption );
		break;
	}

	fprintf(file, "  DerivativeGain     = %f\n",
					filter_config.DerivativeGain );
	fprintf(file, "  DerSamplePeriod    = %f\n",
					filter_config.DerSamplePeriod );
	fprintf(file, "  FollowingError     = %f\n",
					filter_config.FollowingError );
	fprintf(file, "  VelocityGain       = %f\n",
					filter_config.VelocityGain );
	fprintf(file, "  AccelGain          = %f\n", filter_config.AccelGain );
	fprintf(file, "  DecelGain          = %f\n", filter_config.DecelGain );
	fprintf(file, "  EncoderScaling     = %f\n",
					filter_config.EncoderScaling );

	switch( filter_config.UpdateRate ) {
	case MC_RATE_UNKNOWN:
		fprintf(file, "  UpdateRate         = unknown\n" );
		break;
	case MC_RATE_LOW:
		fprintf(file, "  UpdateRate         = low\n" );
		break;
	case MC_RATE_MEDIUM:
		fprintf(file, "  UpdateRate         = medium\n" );
		break;
	case MC_RATE_HIGH:
		fprintf(file, "  UpdateRate         = high\n" );
		break;
	default:
		fprintf(file, "  UpdateRate         = %d\n",
					filter_config.UpdateRate );
		break;
	}

	fprintf(file, "\n");

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pmc_mcapi_open()";

	MX_MOTOR *motor;
	MX_PMC_MCAPI_MOTOR *pmc_mcapi_motor = NULL;
	MX_PMC_MCAPI *pmc_mcapi = NULL;
	unsigned long axis_number;
	mx_status_type mx_status;

	long mcapi_status;
	char error_buffer[100];

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_pmc_mcapi_get_pointers( motor, &pmc_mcapi_motor,
						&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Add this motor record to the axis_array in the controller record. */

	axis_number = pmc_mcapi_motor->axis_number;

	if ( axis_number > pmc_mcapi->num_axes ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The axis number %lu requested for MCAPI motor '%s' is "
		"larger than the maximum allowed axis number (%lu) for "
		"MCAPI controller '%s'.",
			axis_number, record->name,
			pmc_mcapi->num_axes, pmc_mcapi->record->name );
	}

	if ( pmc_mcapi->axis_array[ axis_number-1 ] != (MX_RECORD *) NULL ) {
		return mx_error( MXE_ALREADY_EXISTS, fname,
		"Axis number %lu requested for MCAPI motor '%s' is already "
		"in use by record '%s'.",
			axis_number, record->name,
			pmc_mcapi->axis_array[ axis_number-1 ]->name );
	}

	pmc_mcapi->axis_array[ axis_number-1 ] = record;

	/* Save the axis configuration for later use. */

	pmc_mcapi_motor->configuration.cbSize = sizeof(MCAXISCONFIG);

	mcapi_status = MCGetAxisConfiguration( pmc_mcapi->binary_handle,
					pmc_mcapi_motor->axis_number,
					&(pmc_mcapi_motor->configuration) );

	if ( mcapi_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( mcapi_status,
			error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to get the axis configuration for MCAPI motor '%s'. "
		"MCAPI error message = '%s'",
			pmc_mcapi_motor->record->name, error_buffer );
	}

	/* Enable the motor axis. */

	MCEnableAxis( pmc_mcapi->binary_handle,
			pmc_mcapi_motor->axis_number,
			TRUE );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_pmc_mcapi_resynchronize()";

	MX_MOTOR *motor;
	MX_PMC_MCAPI_MOTOR *pmc_mcapi_motor = NULL;
	MX_PMC_MCAPI *pmc_mcapi = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_pmc_mcapi_get_pointers( motor, &pmc_mcapi_motor,
						&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MCReset( pmc_mcapi->binary_handle, pmc_mcapi_motor->axis_number );

	return MX_SUCCESSFUL_RESULT;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_pmc_mcapi_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmc_mcapi_move_absolute()";

	MX_PMC_MCAPI_MOTOR *pmc_mcapi_motor = NULL;
	MX_PMC_MCAPI *pmc_mcapi = NULL;
#if 0
	long mcapi_status;
	char error_buffer[100];
#endif
	mx_status_type mx_status;

	mx_status = mxd_pmc_mcapi_get_pointers( motor, &pmc_mcapi_motor,
						&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0 /****************************************************************/
	/* Start a block command sequence. */

#if MXD_PMC_MCAPI_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: Starting block command sequence for motor '%s'",
		fname, motor->record->name));
#endif

	mcapi_status = MCBlockBegin( pmc_mcapi->binary_handle,
					MC_BLOCK_COMPOUND, 0 );

	if ( mcapi_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( mcapi_status,
			error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Error invoking MCBlockBegin() for motor '%s'.  "
		"MCAPI error message = '%s'",
			motor->record->name,
			error_buffer );
	}
#endif /****************************************************************/

	/* The first command will be to switch to position mode. */

#if MXD_PMC_MCAPI_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: Motor '%s' will switch to position mode.",
		fname, motor->record->name));
#endif

	MCSetOperatingMode( pmc_mcapi->binary_handle,
				pmc_mcapi_motor->axis_number, 0,
				MC_MODE_POSITION );

	/* The second command will be to start the move. */

#if MXD_PMC_MCAPI_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: Motor '%s' will move to %g",
		fname, motor->record->name, motor->raw_destination.analog));
#endif

	MCMoveAbsolute( pmc_mcapi->binary_handle,
				pmc_mcapi_motor->axis_number,
				motor->raw_destination.analog );

#if 0 /****************************************************************/
	/* Now send the command block. */

#if MXD_PMC_MCAPI_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: Sending motor command block for motor '%s'",
		fname, motor->record->name));
#endif

	mcapi_status = MCBlockEnd( pmc_mcapi->binary_handle, NULL );

	if ( mcapi_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( mcapi_status,
			error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Error sending command block for motor '%s'.  "
		"MCAPI error message = '%s'",
			motor->record->name,
			error_buffer );
	}
#endif /****************************************************************/

#if MXD_PMC_MCAPI_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: Move successfully begun for motor '%s'.",
		fname, motor->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmc_mcapi_get_position()";

	MX_PMC_MCAPI_MOTOR *pmc_mcapi_motor = NULL;
	MX_PMC_MCAPI *pmc_mcapi = NULL;
	long mcapi_status;
	mx_status_type mx_status;

	mx_status = mxd_pmc_mcapi_get_pointers( motor, &pmc_mcapi_motor,
						&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mcapi_status = MCGetPositionEx( pmc_mcapi->binary_handle,
					pmc_mcapi_motor->axis_number,
					&(motor->raw_position.analog) );

	if ( mcapi_status != MCERR_NOERROR ) {
		char error_buffer[100];

		mxi_pmc_mcapi_translate_error( mcapi_status,
			error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Error getting the position of motor '%s'.  "
		"MCAPI error message = '%s'",
			motor->record->name,
			error_buffer );
	}

#if MXD_PMC_MCAPI_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: Motor '%s' position is %g",
		fname, motor->record->name, motor->raw_position.analog));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmc_mcapi_set_position()";

	MX_PMC_MCAPI_MOTOR *pmc_mcapi_motor = NULL;
	MX_PMC_MCAPI *pmc_mcapi = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pmc_mcapi_get_pointers( motor, &pmc_mcapi_motor,
						&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MCSetPosition( pmc_mcapi->binary_handle,
			pmc_mcapi_motor->axis_number,
			motor->raw_set_position.analog );

#if MXD_PMC_MCAPI_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: Motor '%s' current position redefined to %g",
		fname, motor->record->name, motor->raw_set_position.analog));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmc_mcapi_soft_abort()";

	MX_PMC_MCAPI_MOTOR *pmc_mcapi_motor = NULL;
	MX_PMC_MCAPI *pmc_mcapi = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pmc_mcapi_get_pointers( motor, &pmc_mcapi_motor,
						&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MCStop( pmc_mcapi->binary_handle, pmc_mcapi_motor->axis_number );

#if MXD_PMC_MCAPI_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: Stop command sent to motor '%s'",
		fname, motor->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmc_mcapi_immediate_abort()";

	MX_PMC_MCAPI_MOTOR *pmc_mcapi_motor = NULL;
	MX_PMC_MCAPI *pmc_mcapi = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pmc_mcapi_get_pointers( motor, &pmc_mcapi_motor,
						&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MCAbort( pmc_mcapi->binary_handle, pmc_mcapi_motor->axis_number );

#if MXD_PMC_MCAPI_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: Abort command sent to motor '%s'",
		fname, motor->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmc_mcapi_find_home_position()";

	MX_PMC_MCAPI_MOTOR *pmc_mcapi_motor = NULL;
	MX_PMC_MCAPI *pmc_mcapi = NULL;
	char response[MXU_PMC_MCAPI_MAX_COMMAND_LENGTH+1];
	mx_status_type mx_status;

	mx_status = mxd_pmc_mcapi_get_pointers( motor, &pmc_mcapi_motor,
						&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the home search by spawning macro number 1
	 * as a background task.
	 */

	mx_status = mxi_pmc_mccl_command( pmc_mcapi, "GT1,TR0",
						response, sizeof(response),
						MXD_PMC_MCAPI_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: response = '%s'", fname, response));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmc_mcapi_constant_velocity_move()";

	MX_PMC_MCAPI_MOTOR *pmc_mcapi_motor = NULL;
	MX_PMC_MCAPI *pmc_mcapi = NULL;
	WORD direction;
	long mcapi_status;
	mx_status_type mx_status;

	mx_status = mxd_pmc_mcapi_get_pointers( motor, &pmc_mcapi_motor,
						&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Switch to velocity mode. */

	MCSetOperatingMode( pmc_mcapi->binary_handle,
				pmc_mcapi_motor->axis_number, 0,
				MC_MODE_VELOCITY );

#if MXD_PMC_MCAPI_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: Motor '%s' switched to velocity mode.",
		fname, motor->record->name));
#endif

	/* Set the direction of the move. */

	if ( motor->constant_velocity_move >= 0 ) {
		direction = MC_DIR_POSITIVE;
	} else {
		direction = MC_DIR_NEGATIVE;
	}

	MCDirection( pmc_mcapi->binary_handle,
			pmc_mcapi_motor->axis_number,
			direction );

#if MXD_PMC_MCAPI_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: Motor '%s' direction set to %d",
		fname, motor->record->name, (int) direction));
#endif

	/* Start the move. */

	mcapi_status = MCGoEx( pmc_mcapi->binary_handle,
				pmc_mcapi_motor->axis_number, 0.0 );

	if ( mcapi_status != MCERR_NOERROR ) {
		char error_buffer[100];

		mxi_pmc_mcapi_translate_error( mcapi_status,
			error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Error starting a velocity mode move for motor '%s'.  "
		"MCAPI error message = '%s'",
			motor->record->name,
			error_buffer );
	}

#if MXD_PMC_MCAPI_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: Constant velocity move begun for motor '%s'",
		fname, motor->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmc_mcapi_get_parameter()";

	MX_PMC_MCAPI_MOTOR *pmc_mcapi_motor = NULL;
	MX_PMC_MCAPI *pmc_mcapi = NULL;
	MCAXISCONFIG *axis_config = NULL;
	MCMOTIONEX motion_config;
	MCFILTEREX filter_config;
	long mcapi_status;
	char error_buffer[100];
	mx_status_type mx_status;

	mx_status = mxd_pmc_mcapi_get_pointers( motor, &pmc_mcapi_motor,
						&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
	case MXLV_MTR_BASE_SPEED:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		motion_config.cbSize = sizeof(motion_config);

		mcapi_status = MCGetMotionConfigEx(pmc_mcapi->binary_handle,
						pmc_mcapi_motor->axis_number,
						&motion_config );

		if ( mcapi_status != MCERR_NOERROR ) {
			mxi_pmc_mcapi_translate_error( mcapi_status,
				error_buffer, sizeof(error_buffer) );

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Error getting speed and acceleration configuration "
			"for motor '%s'.  MCAPI error message = '%s'",
				motor->record->name,
				error_buffer );
		}

		motor->raw_speed = motion_config.Velocity;
		motor->raw_base_speed = motion_config.MinVelocity;

		motor->raw_acceleration_parameters[0] =
						motion_config.Acceleration;
		motor->raw_acceleration_parameters[2] =
	 					motion_config.Deceleration;
		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;

#if MXD_PMC_MCAPI_MOTOR_DEBUG
		MX_DEBUG(-2,
  ("%s: motor '%s' raw_speed = %g, raw_base_speed = %g, accel = %g, decel = %g",
			fname, motor->record->name,
			motor->raw_speed, motor->raw_base_speed,
			motor->raw_acceleration_parameters[0],
			motor->raw_acceleration_parameters[1]));
#endif
		break;

	case MXLV_MTR_MAXIMUM_SPEED:

		/* First, get the PID filter configuration so that we can
		 * find out what update rate we are currently using.
		 */

		filter_config.cbSize = sizeof(filter_config);

		mcapi_status = MCGetFilterConfigEx(pmc_mcapi->binary_handle,
						pmc_mcapi_motor->axis_number,
						&filter_config );

		if ( mcapi_status != MCERR_NOERROR ) {
			mxi_pmc_mcapi_translate_error( mcapi_status,
				error_buffer, sizeof(error_buffer) );

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Error getting PID filter configuration "
			"for motor '%s'.  MCAPI error message = '%s'",
				motor->record->name,
				error_buffer );
		}

		/* The actual maximum speed will be found in the 
		 * motor's MCAXISCONFIG structure.
		 */

		axis_config = &(pmc_mcapi_motor->configuration);

		switch( filter_config.UpdateRate ) {
		case MC_RATE_LOW:
			motor->raw_maximum_speed = axis_config->LowStepMax;
			break;
		case MC_RATE_MEDIUM:
			motor->raw_maximum_speed = axis_config->MediumStepMax;
			break;
		case MC_RATE_HIGH:
			motor->raw_maximum_speed = axis_config->HighStepMax;
			break;
		default:
			return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
			"PMC MCAPI controller '%s' reported an illegal "
			"PID filter update rate %d",
				pmc_mcapi->record->name,
				filter_config.UpdateRate );
			break;
		}

#if MXD_PMC_MCAPI_MOTOR_DEBUG
		MX_DEBUG(-2,
		("%s: Motor '%s' update rate = %g, raw_maximum_speed = %g",
			fname, motor->record->name,
			(double) filter_config.UpdateRate,
			motor->raw_maximum_speed));
#endif
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
	case MXLV_MTR_INTEGRAL_GAIN:
	case MXLV_MTR_DERIVATIVE_GAIN:
	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
	case MXLV_MTR_EXTRA_GAIN:
	case MXLV_MTR_INTEGRAL_LIMIT:
		filter_config.cbSize = sizeof(filter_config);

		mcapi_status = MCGetFilterConfigEx(pmc_mcapi->binary_handle,
						pmc_mcapi_motor->axis_number,
						&filter_config );

		if ( mcapi_status != MCERR_NOERROR ) {
			mxi_pmc_mcapi_translate_error( mcapi_status,
				error_buffer, sizeof(error_buffer) );

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Error getting PID filter configuration "
			"for motor '%s'.  MCAPI error message = '%s'",
				motor->record->name,
				error_buffer );
		}

		motor->proportional_gain = filter_config.Gain;
		motor->integral_gain = filter_config.IntegralGain;
		motor->derivative_gain = filter_config.DerivativeGain;
		motor->velocity_feedforward_gain = filter_config.VelocityGain;
		motor->acceleration_feedforward_gain = filter_config.AccelGain;
		motor->extra_gain = filter_config.DecelGain;
		motor->integral_limit = filter_config.IntegrationLimit;

#if MXD_PMC_MCAPI_MOTOR_DEBUG
		MX_DEBUG(-2,
		("%s: Motor '%s', Kp = %g, Ki = %g, Kd = %g, "
			"Kvff = %g, Kaff = %g, Ke = %g, Kil = %g",
			fname, motor->record->name,
			motor->proportional_gain,
			motor->integral_gain,
			motor->derivative_gain,
			motor->velocity_feedforward_gain,
			motor->acceleration_feedforward_gain,
			motor->extra_gain,
			motor->integral_limit));
#endif
		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
		break;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmc_mcapi_set_parameter()";

	MX_PMC_MCAPI_MOTOR *pmc_mcapi_motor = NULL;
	MX_PMC_MCAPI *pmc_mcapi = NULL;
	MCMOTIONEX motion_config;
	MCFILTEREX filter_config;
	long mcapi_status, mode;
	short state;
	char error_buffer[100];
	mx_status_type mx_status;

	mx_status = mxd_pmc_mcapi_get_pointers( motor, &pmc_mcapi_motor,
						&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
	case MXLV_MTR_BASE_SPEED:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		/* First, get the current settings. */

		motion_config.cbSize = sizeof(motion_config);

		mcapi_status = MCGetMotionConfigEx( pmc_mcapi->binary_handle,
						pmc_mcapi_motor->axis_number,
						&motion_config );

		if ( mcapi_status != MCERR_NOERROR ) {
			mxi_pmc_mcapi_translate_error( mcapi_status,
				error_buffer, sizeof(error_buffer) );

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Error getting speed and acceleration configuration "
			"for motor '%s'.  MCAPI error message = '%s'",
				motor->record->name,
				error_buffer );
		}

		/* Then, change the requested parameter. */

		switch( motor->parameter_type ) {
		case MXLV_MTR_SPEED:
			motion_config.Velocity = motor->raw_speed;
			break;
		case MXLV_MTR_BASE_SPEED:
			motion_config.MinVelocity = motor->raw_base_speed;
			break;
		case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
			motion_config.Acceleration =
					motor->raw_acceleration_parameters[0];
			motion_config.Deceleration =
					motor->raw_acceleration_parameters[1];
		}

		/* Finally, write back the changed settings. */

		mcapi_status = MCSetMotionConfigEx( pmc_mcapi->binary_handle,
						pmc_mcapi_motor->axis_number,
						&motion_config );

		if ( mcapi_status != MCERR_NOERROR ) {
			mxi_pmc_mcapi_translate_error( mcapi_status,
				error_buffer, sizeof(error_buffer) );

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Error setting speed and acceleration configuration "
			"for motor '%s'.  MCAPI error message = '%s'",
				motor->record->name,
				error_buffer );
		}

#if MXD_PMC_MCAPI_MOTOR_DEBUG
		MX_DEBUG(-2,
  ("%s: motor '%s' raw_speed = %g, raw_base_speed = %g, accel = %g, decel = %g",
			fname, motor->record->name,
			motor->raw_speed, motor->raw_base_speed,
			motor->raw_acceleration_parameters[0],
			motor->raw_acceleration_parameters[1]));
#endif
		break;

	case MXLV_MTR_MAXIMUM_SPEED:
		return mx_error( MXE_UNSUPPORTED, fname,
			"The maximum motor speed cannot be set for motor '%s' "
			"belonging to PMC MCAPI controller '%s'.",
				motor->record->name,
				pmc_mcapi->record->name );
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
	case MXLV_MTR_INTEGRAL_GAIN:
	case MXLV_MTR_DERIVATIVE_GAIN:
	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
	case MXLV_MTR_EXTRA_GAIN:
	case MXLV_MTR_INTEGRAL_LIMIT:
		/* First, get the current settings. */

		filter_config.cbSize = sizeof(filter_config);

		mcapi_status = MCGetFilterConfigEx( pmc_mcapi->binary_handle,
						pmc_mcapi_motor->axis_number,
						&filter_config );

		if ( mcapi_status != MCERR_NOERROR ) {
			mxi_pmc_mcapi_translate_error( mcapi_status,
				error_buffer, sizeof(error_buffer) );

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Error getting PID filter configuration "
			"for motor '%s'.  MCAPI error message = '%s'",
				motor->record->name,
				error_buffer );
		}

		/* Then, change the requested parameter. */

		switch( motor->parameter_type ) {
		case MXLV_MTR_PROPORTIONAL_GAIN:
			filter_config.Gain = motor->proportional_gain;
			break;
		case MXLV_MTR_INTEGRAL_GAIN:
			filter_config.IntegralGain = motor->integral_gain;
			break;
		case MXLV_MTR_DERIVATIVE_GAIN:
			filter_config.DerivativeGain = motor->derivative_gain;
			break;
		case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
			filter_config.VelocityGain =
					motor->velocity_feedforward_gain;
			break;
		case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
			filter_config.AccelGain =
					motor->acceleration_feedforward_gain;
			break;
		case MXLV_MTR_EXTRA_GAIN:
			filter_config.DecelGain = motor->extra_gain;
			break;
		case MXLV_MTR_INTEGRAL_LIMIT:
			filter_config.IntegrationLimit = motor->integral_limit;
			break;
		}

		/* Finally, write back the changed settings. */

		mcapi_status = MCSetFilterConfigEx( pmc_mcapi->binary_handle,
						pmc_mcapi_motor->axis_number,
						&filter_config );

		if ( mcapi_status != MCERR_NOERROR ) {
			mxi_pmc_mcapi_translate_error( mcapi_status,
				error_buffer, sizeof(error_buffer) );

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Error setting PID filter configuration "
			"for motor '%s'.  MCAPI error message = '%s'",
				motor->record->name,
				error_buffer );
		}

#if MXD_PMC_MCAPI_MOTOR_DEBUG
		MX_DEBUG(-2,
		("%s: Motor '%s', Kp = %g, Ki = %g, Kd = %g, "
			"Kvff = %g, Kaff = %g, Ke = %g, Kil = %g",
			fname, motor->record->name,
			motor->proportional_gain,
			motor->integral_gain,
			motor->derivative_gain,
			motor->velocity_feedforward_gain,
			motor->acceleration_feedforward_gain,
			motor->extra_gain,
			motor->integral_limit));
#endif
		break;

	case MXLV_MTR_AXIS_ENABLE:
		if ( motor->axis_enable ) {
			state = TRUE;
		} else {
			state = FALSE;
		}

		MCEnableAxis( pmc_mcapi->binary_handle,
				pmc_mcapi_motor->axis_number,
				state );

#if MXD_PMC_MCAPI_MOTOR_DEBUG
		MX_DEBUG(-2,("%s: Motor '%s' enable state = %ld",
			fname, motor->record->name, (long) state));
#endif
		break;

	case MXLV_MTR_CLOSED_LOOP:
		if ( motor->closed_loop ) {
			mode = MC_IM_CLOSEDLOOP;
		} else {
			mode = MC_IM_OPENLOOP;
		}

		mcapi_status = MCSetModuleInputMode(
					pmc_mcapi->binary_handle,
					pmc_mcapi_motor->axis_number,
					mode );

		if ( mcapi_status != MCERR_NOERROR ) {
			mxi_pmc_mcapi_translate_error( mcapi_status,
				error_buffer, sizeof(error_buffer) );

			if ( motor->closed_loop ) {
				return mx_error(MXE_DEVICE_ACTION_FAILED, fname,
				"Error enabling the servo loop for motor '%s'."
				"  MCAPI error message = '%s'",
					motor->record->name,
					error_buffer );
			} else {
				return mx_error(MXE_DEVICE_ACTION_FAILED, fname,
				"Error disabling the servo loop for motor '%s'."
				"  MCAPI error message = '%s'",
					motor->record->name,
					error_buffer );
			}
		}

#if MXD_PMC_MCAPI_MOTOR_DEBUG
		MX_DEBUG(-2,("%s: Motor '%s' servo loop mode = %ld",
			fname, motor->record->name, mode));
#endif
		break;

	case MXLV_MTR_FAULT_RESET:
		if ( motor->fault_reset ) {
			MCReset( pmc_mcapi->binary_handle,
				pmc_mcapi_motor->axis_number );

#if MXD_PMC_MCAPI_MOTOR_DEBUG
			MX_DEBUG(-2,("%s: Motor '%s' reset sent",
				fname, motor->record->name));
#endif
		}
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return mx_status;
}

#if 0

MX_EXPORT mx_status_type
mxd_pmc_mcapi_simultaneous_start( int num_motor_records,
				MX_RECORD **motor_record_array,
				double *position_array,
				int flags )
{
	static const char fname[] = "mxd_pmc_mcapi_simultaneous_start()";

	MX_RECORD *current_motor_record, *current_controller_record;
	MX_MOTOR *current_motor;
	MX_PMC_MCAPI *pmc_mcapi = NULL;
	MX_PMC_MCAPI *current_pmc_mcapi;
	MX_PMC_MCAPI_MOTOR *current_pmc_mcapi_motor;
	int i;
	char command[80];

	mx_status_type mx_status;

	pmc_mcapi = NULL;

	for ( i = 0; i < num_motor_records; i++ ) {
		current_motor_record = motor_record_array[i];

		current_motor = (MX_MOTOR *)
			current_motor_record->record_class_struct;

		if ( current_motor_record->mx_type != MXT_MTR_PMC_MCAPI ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"Cannot perform a simultaneous start since motors "
			"'%s' and '%s' are not the same type of motors.",
				motor_record_array[0]->name,
				current_motor_record->name );
		}

		current_pmc_mcapi_motor = (MX_PMC_MCAPI_MOTOR *)
				current_motor_record->record_type_struct;

		current_controller_record
			= current_pmc_mcapi_motor->pmc_mcapi_record;

		current_pmc_mcapi = (MX_PMC_MCAPI *)
			current_controller_record->record_type_struct;

		if ( pmc_mcapi == (MX_PMC_MCAPI *) NULL )
		{
			pmc_mcapi = current_pmc_mcapi;
		}

		if ( pmc_mcapi != current_pmc_mcapi ) {
			return mx_error( MXE_UNSUPPORTED, fname,
		"Cannot perform a simultaneous start for motors '%s' and '%s' "
		"since they are controlled by different PMC "
		"interfaces, namely '%s' and '%s'.",
				motor_record_array[0]->name,
				current_motor_record->name,
				pmc_mcapi->record->name,
				current_controller_record->name );
		}
	}

	/* Start the move. */

	for ( i = 0; i < num_motor_records; i++ ) {
		current_motor_record = motor_record_array[i];

		current_motor = (MX_MOTOR *)
			current_motor_record->record_class_struct;

		current_pmc_mcapi_motor = (MX_PMC_MCAPI_MOTOR *)
				current_motor_record->record_type_struct;

		/* FIXME - Start the motor here. */
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif

MX_EXPORT mx_status_type
mxd_pmc_mcapi_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmc_mcapi_get_status()";

	MX_PMC_MCAPI_MOTOR *pmc_mcapi_motor = NULL;
	MX_PMC_MCAPI *pmc_mcapi = NULL;
	MCSTATUSEX status_struct;
	long mcapi_status;
	int is_at_target, is_stopped;
	int mc_stat_amp_fault, mc_stat_dir, mc_stat_error;
	int mc_stat_following, mc_stat_homed;
	int mc_stat_inp_home, mc_stat_inp_index;
	int mc_stat_mlim_trip, mc_stat_plim_trip;
	int mc_stat_mtr_enable, mc_stat_traj;
	int mc_stat_edge_found, mc_stat_index_found;
	char error_buffer[100];
	mx_status_type mx_status;

	mx_status = mxd_pmc_mcapi_get_pointers( motor, &pmc_mcapi_motor,
						&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	is_at_target = MCIsAtTarget( pmc_mcapi->binary_handle,
				pmc_mcapi_motor->axis_number, 0.0 );

	is_stopped = MCIsStopped( pmc_mcapi->binary_handle,
				pmc_mcapi_motor->axis_number, 0.0 );

	mcapi_status = MCGetStatusEx( pmc_mcapi->binary_handle,
					pmc_mcapi_motor->axis_number,
					&status_struct );

	if ( mcapi_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( mcapi_status,
			error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Error the status of motor '%s'.  "
			"MCAPI error message = '%s'",
				motor->record->name,
				error_buffer );
	}

	/* Check all of the status bits that we are interested in. */

	motor->status = 0;

	/* Shown in alphabetical order. */

	mc_stat_amp_fault = MCDecodeStatusEx( pmc_mcapi->binary_handle,
					&status_struct, MC_STAT_AMP_FAULT );

	mc_stat_dir = MCDecodeStatusEx( pmc_mcapi->binary_handle,
					&status_struct, MC_STAT_DIR );

	mc_stat_edge_found = MCDecodeStatusEx( pmc_mcapi->binary_handle,
					&status_struct, MC_STAT_EDGE_FOUND );

	mc_stat_error = MCDecodeStatusEx( pmc_mcapi->binary_handle,
					&status_struct, MC_STAT_ERROR );

	mc_stat_following = MCDecodeStatusEx( pmc_mcapi->binary_handle,
					&status_struct, MC_STAT_FOLLOWING );

	mc_stat_homed = MCDecodeStatusEx( pmc_mcapi->binary_handle,
					&status_struct, MC_STAT_HOMED );

	mc_stat_index_found = MCDecodeStatusEx( pmc_mcapi->binary_handle,
					&status_struct, MC_STAT_INDEX_FOUND );

	mc_stat_inp_home = MCDecodeStatusEx( pmc_mcapi->binary_handle,
					&status_struct, MC_STAT_INP_HOME );

	mc_stat_inp_index = MCDecodeStatusEx( pmc_mcapi->binary_handle,
					&status_struct, MC_STAT_INP_INDEX );

	mc_stat_mlim_trip = MCDecodeStatusEx( pmc_mcapi->binary_handle,
					&status_struct, MC_STAT_MLIM_TRIP );

	mc_stat_mtr_enable = MCDecodeStatusEx( pmc_mcapi->binary_handle,
					&status_struct, MC_STAT_MTR_ENABLE );

	mc_stat_plim_trip = MCDecodeStatusEx( pmc_mcapi->binary_handle,
					&status_struct, MC_STAT_PLIM_TRIP );

	mc_stat_traj = MCDecodeStatusEx( pmc_mcapi->binary_handle,
					&status_struct, MC_STAT_TRAJ );

#if MXD_PMC_MCAPI_MOTOR_DEBUG
	/* Shown in the order displayed in the vendor's CWDemo program. */

	MX_DEBUG(-2,("%s: Motor '%s' MC_STAT_MTR_ENABLE bit = %d",
		fname, motor->record->name, mc_stat_mtr_enable));
	MX_DEBUG(-2,("%s: Motor '%s' MC_STAT_TRAJ bit = %d",
		fname, motor->record->name, mc_stat_traj));
	MX_DEBUG(-2,("%s: Motor '%s' MC_STAT_DIR = %d",
		fname, motor->record->name, mc_stat_dir));
	MX_DEBUG(-2,("%s: Motor '%s' MC_STAT_INP_HOME = %d",
		fname, motor->record->name, mc_stat_inp_home));
	MX_DEBUG(-2,("%s: Motor '%s' MC_STAT_INP_INDEX = %d",
		fname, motor->record->name, mc_stat_inp_index));
	MX_DEBUG(-2,("%s: Motor '%s' MC_STAT_ERROR = %d",
		fname, motor->record->name, mc_stat_error));
	MX_DEBUG(-2,("%s: Motor '%s' MC_STAT_PLIM_TRIP bit = %d",
		fname, motor->record->name, mc_stat_plim_trip));
	MX_DEBUG(-2,("%s: Motor '%s' MC_STAT_MLIM_TRIP bit = %d",
		fname, motor->record->name, mc_stat_mlim_trip));
	MX_DEBUG(-2,("%s: Motor '%s' MC_STAT_AMP_FAULT = %d",
		fname, motor->record->name, mc_stat_amp_fault));
	MX_DEBUG(-2,("%s:----------------------------------", fname));
	MX_DEBUG(-2,("%s: Motor '%s' MC_STAT_EDGE_FOUND = %d",
		fname, motor->record->name, mc_stat_edge_found));
	MX_DEBUG(-2,("%s: Motor '%s' MC_STAT_FOLLOWING = %d",
		fname, motor->record->name, mc_stat_following));
	MX_DEBUG(-2,("%s: Motor '%s' MC_STAT_HOMED = %d",
		fname, motor->record->name, mc_stat_homed));
	MX_DEBUG(-2,("%s: Motor '%s' MC_STAT_INDEX_FOUND = %d",
		fname, motor->record->name, mc_stat_index_found));
	MX_DEBUG(-2,("%s:----------------------------------", fname));
	MX_DEBUG(-2,("%s: Motor '%s' MCIsAtTarget = %ld",
		fname, motor->record->name, (long) is_at_target));
	MX_DEBUG(-2,("%s: Motor '%s' MCIsStopped = %ld",
		fname, motor->record->name, (long) is_stopped));
#endif

	/* Shown in the order of MX status bits. */

#if 0
	if ( mc_stat_traj == 0 ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}
#else
	if ( is_stopped == 0 ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}
#endif
	if ( mc_stat_plim_trip ) {
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
	}
	if ( mc_stat_mlim_trip ) {
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
	}
	if ( mc_stat_homed ) {
		motor->status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;
	}
	if ( mc_stat_following ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}
	if ( mc_stat_amp_fault ) {
		motor->status |= MXSF_MTR_DRIVE_FAULT;
	}
#if 0
	if ( mc_stat_mtr_enable == 0 ) {
		motor->status |= MXSF_MTR_AXIS_DISABLED;
	}
#endif
	if ( mc_stat_error ) {
		motor->status |= MXSF_MTR_ERROR;
	}

#if MXD_PMC_MCAPI_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: Motor '%s', MX motor->status = %#lx",
		fname, motor->record->name, motor->status));
#endif

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_PMC_MCAPI && defined(OS_WIN32) */

