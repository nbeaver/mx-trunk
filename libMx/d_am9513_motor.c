/*
 * Name:    d_am9513_motor.c
 *
 * Purpose: MX motor driver to use Am9513 counters as motor controllers.
 *
 *          This driver assumes that two contiguous counters in an Am9513
 *          chip are used to implement this motor controller.  The low
 *          order counter is used to set the motor speed, while the high
 *          order counter generates the motor step pulses.  This setup
 *          assumes that the output of the high order counter is connected
 *          to the external gate for the low order counter.
 *
 *          Please note that this driver always runs the motor at a 
 *          configurable speed.  There is no ramp up acceleration or
 *          ramp down deceleration.  It is equivalent to running a
 *          motor always at the 'base speed'.
 *
 * Author:  William Lavender
 *
 *          Modified by Carlo Segre to lengthen motor step pulses.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>

#include "mxconfig.h"
#include "mx_constants.h"
#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_motor.h"
#include "mx_digital_output.h"
#include "d_am9513_motor.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_am9513_motor_record_function_list = {
	mxd_am9513_motor_initialize_type,
	mxd_am9513_motor_create_record_structures,
	mxd_am9513_motor_finish_record_initialization,
	mxd_am9513_motor_delete_record,
	mxd_am9513_motor_print_structure,
	mxd_am9513_motor_read_parms_from_hardware,
	mxd_am9513_motor_write_parms_to_hardware,
	mxd_am9513_motor_open,
	mxd_am9513_motor_close,
	NULL,
	mxd_am9513_motor_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_am9513_motor_motor_function_list = {
	mxd_am9513_motor_motor_is_busy,
	mxd_am9513_motor_move_absolute,
	mxd_am9513_motor_get_position,
	mxd_am9513_motor_set_position,
	mxd_am9513_motor_soft_abort,
	mxd_am9513_motor_immediate_abort,
	mxd_am9513_motor_positive_limit_hit,
	mxd_am9513_motor_negative_limit_hit,
	mxd_am9513_motor_find_home_position
};

/* Am9513 motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_am9513_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_AM9513_MOTOR_STANDARD_FIELDS
};

long mxd_am9513_motor_num_record_fields
		= sizeof( mxd_am9513_motor_record_field_defaults )
			/ sizeof( mxd_am9513_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_am9513_motor_rfield_def_ptr
			= &mxd_am9513_motor_record_field_defaults[0];

static MX_AM9513 *debug_am9513_ptr = &mxi_am9513_debug_struct;

/* A private function for the use of the driver. */

static mx_status_type
mxd_am9513_motor_get_pointers( MX_MOTOR *motor,
				MX_AM9513_MOTOR **am9513_motor,
				long *num_counters,
				MX_INTERFACE **am9513_interface_array,
				const char *calling_fname )
{
	const char fname[] = "mxd_am9513_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( am9513_motor == (MX_AM9513_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AM9513_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( num_counters == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_counters pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( am9513_interface_array == (MX_INTERFACE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERFACE array pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*am9513_motor = (MX_AM9513_MOTOR *)
				(motor->record->record_type_struct);

	if ( *am9513_motor == (MX_AM9513_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AM9513_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	*num_counters = (*am9513_motor)->num_counters;

	*am9513_interface_array = (*am9513_motor)->am9513_interface_array;

	if ( *am9513_interface_array == (MX_INTERFACE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_INTERFACE array pointer for motor '%s' is NULL.",
			motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_am9513_motor_initialize_type( long type )
{
	const char fname[] = "mxd_am9513_motor_initialize_type()";

	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults, *field;
	long num_record_fields;
	long num_counters_field_index;
	long num_counters_varargs_cookie;
	mx_status_type status;

	driver = mx_get_driver_by_type( type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record type %ld not found.", type );
	}

	record_field_defaults = *(driver->record_field_defaults_ptr);
	num_record_fields = *(driver->num_record_fields);

	status = mx_find_record_field_defaults_index(
			record_field_defaults, num_record_fields,
			"num_counters", &num_counters_field_index );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_construct_varargs_cookie(
		num_counters_field_index, 0, &num_counters_varargs_cookie );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_find_record_field_defaults(
			record_field_defaults, num_record_fields,
			"am9513_interface_array", &field );

	if ( status.code != MXE_SUCCESS )
		return status;

	field->dimension[0] = num_counters_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_motor_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_am9513_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_AM9513_MOTOR *am9513_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	am9513_motor = (MX_AM9513_MOTOR *) malloc( sizeof(MX_AM9513_MOTOR) );

	if ( am9513_motor == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AM9513_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = am9513_motor;
	record->class_specific_function_list
			= &mxd_am9513_motor_motor_function_list;

	motor->record = record;

	/* An elapsed time motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_motor_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type status;

	status = mx_motor_finish_record_initialization( record );

	return status;
}

MX_EXPORT mx_status_type
mxd_am9513_motor_delete_record( MX_RECORD *record )
{
	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}
	if ( record->record_type_struct != NULL ) {
		free( record->record_type_struct );

		record->record_type_struct = NULL;
	}
	if ( record->record_class_struct != NULL ) {
		free( record->record_class_struct );

		record->record_class_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_motor_print_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxd_am9513_motor_print_structure()";

	MX_MOTOR *motor;
	MX_AM9513_MOTOR *am9513_motor;
	MX_INTERFACE *am9513_interface_array;
	long i, num_counters;
	double position, move_deadband;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MOTOR pointer for record '%s' is NULL.", record->name );
	}

	am9513_motor = (MX_AM9513_MOTOR *) (record->record_type_struct);

	if ( am9513_motor == (MX_AM9513_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_AM9513_MOTOR pointer for record '%s' is NULL.", record->name );
	}

	num_counters = am9513_motor->num_counters;

	am9513_interface_array = am9513_motor->am9513_interface_array;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type        = AM9513 motor.\n\n");

	fprintf(file, "  name              = %s\n", record->name);
	fprintf(file, "  num counters      = %ld\n", num_counters);
	fprintf(file, "  counter array     = (");

	for ( i = 0; i < num_counters; i++ ) {
		fprintf(file, "%s:%s", am9513_interface_array[i].record->name,
				am9513_interface_array[i].address_name);

		if ( i != num_counters-1 ) {
			fprintf(file, ", ");
		}
	}
	fprintf(file, ")\n");
	fprintf(file, "  direction record  = %s\n",
					am9513_motor->direction_record->name );
	fprintf(file, "  direction bit     = %ld\n",
					am9513_motor->direction_bit );
	fprintf(file, "  step frequency    = %g\n",
					am9513_motor->step_frequency );

	status = mx_motor_get_position( record, &position );

	if ( status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
		}
	
	fprintf(file, "  position          = %g %s  (%ld steps)\n",
			motor->position, motor->units,
			motor->raw_position.stepper );
	fprintf(file, "  scale             = %g %s per step.\n",
			motor->scale, motor->units);
	fprintf(file, "  offset            = %g %s.\n",
			motor->offset, motor->units);
	
	fprintf(file, "  backlash          = %g %s  (%ld steps)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.stepper );
	
	fprintf(file, "  negative limit    = %g %s  (%ld steps)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.stepper );

	fprintf(file, "  positive limit    = %g %s  (%ld steps)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.stepper );

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  move deadband     = %g %s  (%ld steps)\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.stepper );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_motor_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_motor_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_motor_open( MX_RECORD *record )
{
	const char fname[] = "mxd_am9513_motor_open()";

	MX_MOTOR *motor;
	MX_AM9513_MOTOR *am9513_motor;
	MX_INTERFACE *am9513_interface_array;
	MX_RECORD *low_record, *high_record;
	MX_AM9513 *low_am9513, *high_am9513;
	uint16_t counter_mode_register;
	long num_counters;
	int m, n, frequency_scaler_ratio;
	unsigned long clock_ticks_ulong;
	uint16_t ticks_to_count_for;
	double seconds_per_step, external_clock_ticks_per_step;
	double ulong_max_double;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s called.", fname));

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_am9513_motor_get_pointers( motor,
			&am9513_motor, &num_counters,
			&am9513_interface_array, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_counters != 2 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The number of counters (%ld) for motor '%s' should be equal to 2.",
			num_counters, record->name );
	}

	mx_status = mxi_am9513_grab_counters( record, num_counters,
						am9513_interface_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set status variables. */

	am9513_motor->last_destination = motor->raw_position.stepper;
	am9513_motor->last_direction = 1;
	am9513_motor->busy_retries_left = 0;

	/* Get pointers to the counter data structures. */

	low_record  = am9513_interface_array[0].record;
	high_record = am9513_interface_array[1].record;

	if ( low_record != high_record ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Error configuring motor '%s'.  The counters used for this "
		"driver must be from the same chip.", record->name );
	}

	m = (int) am9513_interface_array[0].address - 1;
	n = (int) am9513_interface_array[1].address - 1;

	if ( m+1 != n ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Error configuring motor '%s'.  The low order counter '%s:%d' "
		"is not immediately below the high order counter '%s:%d'",
			record->name, low_record->name, m+1,
			high_record->name, n+1 );
	}

	low_am9513  = (MX_AM9513 *) low_record->record_type_struct;
	high_am9513 = (MX_AM9513 *) high_record->record_type_struct;

	/*
	 * Initialize the mode register for the lower order counter.
	 * This counter is used to set the speed of the motor.
         *
	 * Settings:
	 *    High level gate N (hardwired to OUT of high order counter)
	 *    Count on rising edge.
	 *    ( skip setting the source value here )
	 *    Disable special gate.
	 *    Reload from load.
	 *    Count repetitively.
	 *    Binary count.
	 *    Count Down.
	 *    Active high terminal pulse count.
	 */

	counter_mode_register  = 0x8000;	/* High level gate N */
	counter_mode_register |= 0x0020;	/* Count repetitively */
	counter_mode_register |= 0x0001;	/* Active high term. pulse */

	/* Compute the source and load register value needed for
	 * the requested speed.
	 */

	if ( fabs( am9513_motor->step_frequency ) <= 0.0 ) {

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested motor step frequency %g Hz for motor '%s' "
		"must be a positive number",
			am9513_motor->step_frequency, record->name );
	}

	seconds_per_step = mx_divide_safely(1.0, am9513_motor->step_frequency);

	external_clock_ticks_per_step = seconds_per_step
					* am9513_motor->clock_frequency;

	MX_DEBUG( 2,("%s: step_frequency = %g",
				fname, am9513_motor->step_frequency));
	MX_DEBUG( 2,("%s: seconds_per_step = %g", fname, seconds_per_step));
	MX_DEBUG( 2,("%s: external_clock_ticks_per_step = %g",
				fname, external_clock_ticks_per_step));

	ulong_max_double = (double) MX_ULONG_MAX;

	if ( external_clock_ticks_per_step > ulong_max_double ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"%g Hz is too slow a motor step frequency for a clock frequency of %g Hz.",
			am9513_motor->step_frequency,
			am9513_motor->clock_frequency );
	}

	clock_ticks_ulong = mx_round( external_clock_ticks_per_step );

	MX_DEBUG( 2,("%s: clock_ticks_ulong = %lu",
				fname, clock_ticks_ulong));

/* Replaced with alternative model below since this has a tendency
 * to result in the shortest possible step pulse length (basically 
 * the TC pulse is just one clock cycle).  This may cause some 
 * problems with motor response when there is a long cable and a 
 * pull-up resistor at the other end. (CUS 990628)
	if ( clock_ticks_ulong < 65536L ) {
		frequency_scaler_ratio = 0x0b00;		F1 
		ticks_to_count_for = (uint16_t) clock_ticks_ulong;
	} else
	if ( clock_ticks_ulong < 1048576L ) {
		frequency_scaler_ratio = 0x0c00;		F2
		ticks_to_count_for = (uint16_t)
					( clock_ticks_ulong / 16L );
	} else
	if ( clock_ticks_ulong < 16777216L ) {
		frequency_scaler_ratio = 0x0d00;		F3
		ticks_to_count_for = (uint16_t)
					( clock_ticks_ulong / 256L );
	} else
	if ( clock_ticks_ulong < 268435456L ) {
		frequency_scaler_ratio = 0x0e00;		F4
		ticks_to_count_for = (uint16_t)
					( clock_ticks_ulong / 4096L );
	} else {
		frequency_scaler_ratio = 0x0f00;		F5
		ticks_to_count_for = (uint16_t)
					( clock_ticks_ulong / 65536L );
	}
*/

/* This model for determining the optimal clock frequency and delay
 * sacrifices accuracy in timing for a better step pulse duty cycle.
 * The idea is that a slower motor will require a longer step pulse.
 * In this algorithm, we shoot for between a 1:8 and 1:128 step pulse 
 * to inter-step delay time.  (CUS 990628)
 */

	if ( clock_ticks_ulong > 524288L ) {
		frequency_scaler_ratio = 0x0f00;		/* F5 */
	        ticks_to_count_for = (uint16_t)
			( clock_ticks_ulong / 65536L );
	} else 
	if ( clock_ticks_ulong > 32768L ) {
		frequency_scaler_ratio = 0x0e00;		/* F4 */
	        ticks_to_count_for = (uint16_t)
			( clock_ticks_ulong / 4096L );
	} else 
	if ( clock_ticks_ulong > 2048L ) {
		frequency_scaler_ratio = 0x0d00;		/* F3 */
	        ticks_to_count_for = (uint16_t)
			( clock_ticks_ulong / 256L );
	} else 
	if ( clock_ticks_ulong > 128L ) {
		frequency_scaler_ratio = 0x0c00;		/* F2 */
	        ticks_to_count_for = (uint16_t)
			( clock_ticks_ulong / 16L );
	} else {
		frequency_scaler_ratio = 0x0b00;		/* F1 */
	        ticks_to_count_for = (uint16_t) clock_ticks_ulong;
	} 

	MX_DEBUG( 2,
		("%s: frequency_scaler_ratio = %d, ticks_to_count_for = %hu", 
		fname, frequency_scaler_ratio, ticks_to_count_for));

	/* Set the timer preset value and the divisor
	 * for the clock frequency.
	 */

	counter_mode_register |= frequency_scaler_ratio;

	low_am9513->counter_mode_register[m] = counter_mode_register;

	low_am9513->load_register[m] = ticks_to_count_for;

	/* Configure the low order counter. */

	mx_status = mxi_am9513_set_counter_mode_register( low_am9513, m );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_am9513_load_counter( low_am9513, m );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_AM9513_DEBUG( debug_am9513_ptr, 1 );

	/*
	 * Initialize the mode register for the higher order counter so
	 * that it counts down once for each time the low order counter
	 * counts down to zero.  This counter is used to generate the
	 * steps sent to the motor driver by the low order counter and
	 * turn off the lower counter when it reaches 0 and it's OUT line
	 * ( connected to the GATE of the lower order counter ) disables
	 * further step pulses from being generated.
	 *
	 * Settings:
	 *    No gating.
	 *    Count on trailing edge.
	 *    Source TCN-1.
	 *    Disable special gate.
	 *    Reload from load.
	 *    Count once.
	 *    Binary count.
	 *    Count Down.
	 *    TC toggled.
	 */

	counter_mode_register  = 0;		/* No gating */
	counter_mode_register |= 0x1000;	/* Count on trailing edge */
	counter_mode_register |= 0x0002;	/* TC toggled */

	/* Configure the high order counter mode register. */

	high_am9513->counter_mode_register[n] = counter_mode_register;

	mx_status = mxi_am9513_set_counter_mode_register( low_am9513, n );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the load register to 0xffff.  When the 'get_position'
	 * function adds 1 to this 16-bit value, it will roll over to 0.
	 */

	high_am9513->load_register[n] = 0xffff;

	mx_status = mxi_am9513_load_counter( high_am9513, n );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure that the output (OUTn) for the high order counter
	 * is currently set low.
	 */

	mx_status = mxi_am9513_clear_tc( high_am9513, n );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_AM9513_DEBUG( debug_am9513_ptr, 1 );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_motor_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_motor_resynchronize( MX_RECORD *record )
{
	const char fname[] = "mxd_am9513_motor_resynchronize()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Resynchronize is not yet implemented for Am9513 controlled motors." );
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_am9513_motor_motor_is_busy( MX_MOTOR *motor )
{
	const char fname[] = "mxd_am9513_motor_motor_is_busy()";

	MX_AM9513_MOTOR *am9513_motor;
	MX_INTERFACE *am9513_interface_array;
	MX_RECORD *this_record;
	MX_AM9513 *this_am9513;
	long num_counters;
	uint8_t am9513_status;
	int mask, busy;
	mx_status_type mx_status;

	mx_status = mxd_am9513_motor_get_pointers( motor, &am9513_motor,
			&num_counters, &am9513_interface_array, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find out whether either of the counters are still counting. */

	this_record = am9513_interface_array[0].record;

	this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

	do {
		MX_DEBUG( 2,("%s: attempting to get busy status.", fname));

		mx_status = mxi_am9513_get_status( this_am9513 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_AM9513_DEBUG( debug_am9513_ptr, 1 );

		am9513_status = this_am9513->status_register;

		MX_DEBUG( 2,("%s: am9513_status = 0x%x, retries = %ld",
					fname, am9513_status,
					am9513_motor->busy_retries_left));

		mask = 1 << am9513_interface_array[1].address;

		if ( am9513_status & mask ) {
			busy = TRUE;
		} else {
			busy = FALSE;
		}

		if ( busy ) {
			MX_DEBUG( 2,("%s: retries were zeroed.", fname));

			am9513_motor->busy_retries_left = 0;

			break;		/* exit the do while loop. */
		} else {
			if ( am9513_motor->busy_retries_left > 0 ) {
				MX_DEBUG( 2,
				("%s: retries were decremented.",fname));

				(am9513_motor->busy_retries_left)--;
			} else {
				MX_DEBUG( 2,
				("%s: retries were _not_ decremented.",fname));
			}
		}

		MX_AM9513_DELAY;

	} while ( am9513_motor->busy_retries_left > 0 );

	motor->busy = busy;

	MX_DEBUG( 2,("%s: busy = %d", fname, (int) motor->busy));

	return MX_SUCCESSFUL_RESULT;
}

/*
 * mxd_am9513_motor_move_single_step() moves an Am9513 controlled motor
 * by one step using only stone knives and bearskins.
 */

static mx_status_type
mxd_am9513_motor_move_single_step( MX_AM9513_MOTOR *am9513_motor,
					MX_AM9513 *low_am9513, int m )
{
	uint16_t saved_counter_mode_register, saved_load_register;

	mx_status_type mx_status;

	saved_counter_mode_register = low_am9513->counter_mode_register[m];
	saved_load_register = low_am9513->load_register[m];

	/* Turn off gating and the count repetitively settings. */

	low_am9513->counter_mode_register[m]
				= saved_counter_mode_register & 0x1fcf;

	mx_status = mxi_am9513_set_counter_mode_register( low_am9513, m );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Load a 4 into the counter.  A 0 here will not put out any
	 * steps at all.  A 1 or 2 results in an anomalously long step
	 * pulse (not known why!) and 3 or more gives the desired short
	 * step pulse.  (CUS 050615) */

	low_am9513->load_register[m] = 4;

	mx_status = mxi_am9513_load_counter( low_am9513, m );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Arm the counter.  This should cause the step to occur. */

	mx_status = mxi_am9513_arm_counter( low_am9513, m );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the step pulse to complete.  Unless the delay is
	 * present, a fast computer could reset the Load and Command
	 * registers before the step pulse is complete, resulting in
	 * flaky behavior.  It is not clear that the 10ms delay is
	 * optimal but it works and is not too long ;). (CUS 050615) */

	mx_msleep( 10 );

	/* Now restore the counter parameters. */

	low_am9513->counter_mode_register[m] = saved_counter_mode_register;

	mx_status = mxi_am9513_set_counter_mode_register( low_am9513, m );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	low_am9513->load_register[m] = saved_load_register;

	mx_status = mxi_am9513_load_counter( low_am9513, m );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Modify the recorded position of the motor. */

	if ( am9513_motor->last_direction < 0 ) {
		am9513_motor->last_destination--;
	} else {
		am9513_motor->last_destination++;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*
 * mxd_am9513_motor_move_absolute() is the normal entry point for
 * moving an Am9513 controlled motor.
 */

MX_EXPORT mx_status_type
mxd_am9513_motor_move_absolute( MX_MOTOR *motor )
{
	const char fname[] = "mxd_am9513_motor_move_absolute()";

	MX_AM9513_MOTOR *am9513_motor;
	MX_INTERFACE *am9513_interface_array;
	MX_RECORD *high_record, *low_record, *this_record;
	MX_AM9513 *high_am9513, *low_am9513, *this_am9513;
	int i, j, m, n;
	long num_counters, relative_steps;
	unsigned long output_value, mask;
	mx_status_type mx_status;

	mx_status = mxd_am9513_motor_get_pointers( motor, &am9513_motor,
			&num_counters, &am9513_interface_array, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure the counters are disarmed before we start.
	 * This is because we do not want any of the counters to
	 * start counting before the time we arm them later.
	 */

	for ( i = 0; i <= 1; i++ ) {

		this_record = am9513_interface_array[i].record;

		this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

		j = (int) am9513_interface_array[i].address - 1;

		mx_status = mxi_am9513_disarm_counter( this_am9513, j );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_AM9513_DEBUG( debug_am9513_ptr, 1 );
	}

	low_record = am9513_interface_array[0].record;

	low_am9513 = (MX_AM9513 *) low_record->record_type_struct;

	m = (int) am9513_interface_array[0].address - 1;

	high_record = am9513_interface_array[1].record;

	high_am9513 = (MX_AM9513 *) high_record->record_type_struct;

	n = (int) am9513_interface_array[1].address - 1;

	/*** Read the current position and compute the relative motion. ***/

	mx_status = mxd_am9513_motor_get_position( motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: raw_destination = %ld, raw_position = %ld",
		fname, motor->raw_destination.stepper,
		motor->raw_position.stepper));

	relative_steps = motor->raw_destination.stepper
					- motor->raw_position.stepper;

	MX_DEBUG( 2,("%s: relative_steps = %ld", fname, relative_steps));

	if ( ( relative_steps < -65534 ) || ( relative_steps > 65534 ) ) {

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"A move of motor '%s' to %g would require a relative move of %ld steps.  "
"The maximum move currently allowed is 65534 steps or less.",
			motor->record->name, motor->destination,
			relative_steps );
	}

	/* If the number of relative steps is 0, then return immediately
	 * with a success status.  This is to avoid loading the value 0
	 * into an Am9513 counter, since the Am9513 will immediately roll
	 * over from 0 to 0xffff which is not what we want at all.
	 */

	if ( relative_steps == 0 ) {
		MX_DEBUG( 2,(
		"%s: relative_steps = 0, so we are returning immediately.",
			fname ));

		return MX_SUCCESSFUL_RESULT;
	}

	/************** Set the direction bit. ***************/

	mx_status = mx_digital_output_read(
			am9513_motor->direction_record, &output_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: old output_value = %lu", fname, output_value));

	if ( relative_steps >= 0 ) {
		mask = 1 << am9513_motor->direction_bit;

		output_value |= mask;

		am9513_motor->last_direction = 1;
	} else {
		mask = 1 << am9513_motor->direction_bit;

		output_value &= ~mask;

		am9513_motor->last_direction = -1;
	}

	mx_status = mx_digital_output_write(
			am9513_motor->direction_record, output_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: new output_value = %lu", fname, output_value));
	MX_DEBUG( 2,("%s: last_direction = %ld",
			fname, am9513_motor->last_direction));

	/* We must handle the case of moving 1 step specially.  This
	 * is because for this case 'abs(relative_steps) - 1' below
	 * would be equal to 0 and loading a 0 into the counter will
	 * cause it to roll over to 0xffff, which is undesirable.
	 */

	if ( abs( (int) relative_steps ) == 1 ) {
		mx_status = mxd_am9513_motor_move_single_step(
					am9513_motor, low_am9513, m );

		return mx_status;
	}

	/************** Load the distance to move. *****************/

	/* We subtract 1 from the value since the Am9513 will generate
	 * one more step than the number you put in the load register.
	 */

	high_am9513->load_register[n] = abs( (int) relative_steps ) - 1;

	MX_DEBUG( 2,("%s: loading value %hu",
			fname, high_am9513->load_register[n] ));

	mx_status = mxi_am9513_load_counter( high_am9513, n );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_AM9513_DEBUG( debug_am9513_ptr, 1 );

	/* Now that the load register has been copied into the counter,
	 * put a 0 in the load register so that 'reload from load' at
	 * the end of a move will result in the counter rolling over
	 * to a value of 0xffff.  We can then detect this value of 0xffff
	 * in the get position routine and reinterpret it as a value
	 * of zero.  This makes the reading of the current position 
	 * easier to handle.
	 */

	high_am9513->load_register[n] = 0;

	mx_status = mxi_am9513_set_load_register( high_am9513, n );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_AM9513_DEBUG( debug_am9513_ptr, 1 );

	/* Make sure that the output (OUTn) for the high order counter
	 * is currently set high.
	 */

	mx_status = mxi_am9513_set_tc( high_am9513, n );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_AM9513_DEBUG( debug_am9513_ptr, 1 );

	/************* Start the move by arming the counters. *************/

	am9513_motor->last_destination = motor->raw_destination.stepper;
	am9513_motor->busy_retries_left = 1 + am9513_motor->busy_retries;

	MX_DEBUG( 2,("%s: last_destination = %ld",
			fname, am9513_motor->last_destination));

	for ( i = 1; i >= 0; i-- ) {

		this_record = am9513_interface_array[i].record;

		this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

		j = (int) am9513_interface_array[i].address - 1;

		mx_status = mxi_am9513_arm_counter( this_am9513, j );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_AM9513_DEBUG( debug_am9513_ptr, 1 );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_motor_get_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_am9513_motor_get_position()";

	MX_AM9513_MOTOR *am9513_motor;
	MX_INTERFACE *am9513_interface_array;
	MX_RECORD *high_record;
	MX_AM9513 *high_am9513;
	long num_counters;
	int n;
	uint16_t hold_register, step_offset;
	mx_status_type mx_status;

	mx_status = mxd_am9513_motor_get_pointers( motor, &am9513_motor,
			&num_counters, &am9513_interface_array, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the hold register of the high order counter. */

	high_record = am9513_interface_array[1].record;

	high_am9513 = (MX_AM9513 *) high_record->record_type_struct;

	n = (int) am9513_interface_array[1].address - 1;

	MX_AM9513_DEBUG( high_am9513, 0 );
	MX_AM9513_DEBUG( debug_am9513_ptr, 1 );

	mx_status = mxi_am9513_read_counter( high_am9513, n );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_AM9513_DEBUG( debug_am9513_ptr, 1 );

	hold_register = high_am9513->hold_register[n];

	MX_DEBUG( 2,("%s: busy_retries_left = %ld",
			fname, am9513_motor->busy_retries_left));

	/* If the hold register contains 0xffff, this means the Am9513
	 * reloaded 0 from the load register and then rolled over
	 * to 0xffff.
	 *
	 * If the hold register contains some other value, we must
	 * add 1 to it to compensate for the fact that we subtracted
	 * 1 from the relative steps to move in the 'move_absolute'
	 * routine.
	 */

	if ( hold_register == 0xffff ) {
		step_offset = 0;
	} else {
		step_offset = hold_register + 1;
	}

	MX_DEBUG( 2,("%s: hold = %hu, step_offset = %hu",
		fname, hold_register, step_offset));

	MX_DEBUG( 2,
	("%s: last_destination = %ld, last_direction = %ld",
		fname, am9513_motor->last_destination,
		am9513_motor->last_direction ));

	motor->raw_position.stepper = am9513_motor->last_destination
			- am9513_motor->last_direction * step_offset;

	MX_DEBUG( 2,("%s: step_offset = %hu, raw_position = %ld",
		fname, step_offset, motor->raw_position.stepper));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_motor_set_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_am9513_motor_set_position()";

	MX_AM9513_MOTOR *am9513_motor;
	MX_INTERFACE *am9513_interface_array;
	MX_RECORD *high_record;
	MX_AM9513 *high_am9513;
	long num_counters;
	int n;
	mx_status_type mx_status;

	mx_status = mxd_am9513_motor_get_pointers( motor, &am9513_motor,
			&num_counters, &am9513_interface_array, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->raw_position.stepper = motor->raw_set_position.stepper;

	am9513_motor->last_destination = motor->raw_set_position.stepper;

	high_record = am9513_interface_array[1].record;

	high_am9513 = (MX_AM9513 *) high_record->record_type_struct;

	n = (int) am9513_interface_array[1].address - 1;

	/* Set the load register to 0xffff.  When the 'get_position'
	 * function adds 1 to this 16-bit value, it will roll over to 0.
	 */

	high_am9513->load_register[n] = 0xffff;

	mx_status = mxi_am9513_load_counter( high_am9513, n );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_AM9513_DEBUG( debug_am9513_ptr, 1 );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_motor_soft_abort( MX_MOTOR *motor )
{
	const char fname[] = "mxd_am9513_motor_soft_abort()";

	MX_AM9513_MOTOR *am9513_motor;
	MX_INTERFACE *am9513_interface_array;
	MX_RECORD *this_record;
	MX_AM9513 *this_am9513;
	int i, n;
	long num_counters;
	mx_status_type mx_status;

	mx_status = mxd_am9513_motor_get_pointers( motor, &am9513_motor,
			&num_counters, &am9513_interface_array, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Keep GCC from generating warnings about possibly
	 * uninitialized variables.
	 */

	this_am9513 = NULL;
	n = 0;

	/* Disarm the counters. */

	for ( i = 0; i < num_counters; i++ ) {

		this_record = am9513_interface_array[i].record;

		this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

		n = (int) am9513_interface_array[i].address - 1;

		mx_status = mxi_am9513_disarm_counter( this_am9513, n );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_AM9513_DEBUG( debug_am9513_ptr, 1 );
	}

	/* Make sure that the outputs (OUTn) for the counters are set low. */

	for ( i = 0; i < num_counters; i++ ) {

		this_record = am9513_interface_array[i].record;

		this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

		n = (int) am9513_interface_array[i].address - 1;

		mx_status = mxi_am9513_clear_tc( this_am9513, n );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_AM9513_DEBUG( debug_am9513_ptr, 1 );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_motor_immediate_abort( MX_MOTOR *motor )
{
	return mxd_am9513_motor_soft_abort( motor );
}

MX_EXPORT mx_status_type
mxd_am9513_motor_positive_limit_hit( MX_MOTOR *motor )
{
	motor->positive_limit_hit = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_motor_negative_limit_hit( MX_MOTOR *motor )
{
	motor->negative_limit_hit = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_motor_find_home_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_am9513_motor_find_home_position()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"The Am9513 motor driver does not support home searches." );
}

