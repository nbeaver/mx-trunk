/*
 * Name:    d_am9513_timer.c
 *
 * Purpose: MX timer driver to control Am9513 counters as timers.
 *
 *          At present, only configurations with one 16-bit counter
 *          is supported.  Also note that this driver relies on
 *          the output for this counter being connected to the gate
 *          for the counter.  That is, OUT(n) must be connected to
 *          GATE(n) for the counter to work.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "mxconfig.h"
#include "mx_constants.h"
#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_timer.h"
#include "i_am9513.h"
#include "d_am9513_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_am9513_timer_record_function_list = {
	mxd_am9513_timer_initialize_type,
	mxd_am9513_timer_create_record_structures,
	mxd_am9513_timer_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_am9513_timer_open,
	mxd_am9513_timer_close
};

MX_TIMER_FUNCTION_LIST mxd_am9513_timer_timer_function_list = {
	mxd_am9513_timer_is_busy,
	mxd_am9513_timer_start,
	mxd_am9513_timer_stop,
	NULL,
	NULL,
	mxd_am9513_timer_get_mode,
	mxd_am9513_timer_set_mode
};

/* Am9513 timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_am9513_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_AM9513_TIMER_STANDARD_FIELDS
};

mx_length_type mxd_am9513_timer_num_record_fields
		= sizeof( mxd_am9513_timer_record_field_defaults )
		  / sizeof( mxd_am9513_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_am9513_timer_rfield_def_ptr
			= &mxd_am9513_timer_record_field_defaults[0];

static MX_AM9513 *debug_am9513_ptr = &mxi_am9513_debug_struct;

static mx_status_type
mxd_am9513_timer_get_pointers( MX_TIMER *timer,
				MX_AM9513_TIMER **am9513_timer,
				mx_length_type *num_counters,
				MX_INTERFACE **am9513_interface_array,
				const char *calling_fname )
{
	static const char fname[] = "mxd_am9513_get_pointers()";

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_TIMER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( am9513_timer == (MX_AM9513_TIMER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AM9513_TIMER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( num_counters == (mx_length_type *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_counters pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( am9513_interface_array == (MX_INTERFACE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERFACE array pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*am9513_timer = (MX_AM9513_TIMER *)
				(timer->record->record_type_struct);

	if ( *am9513_timer == (MX_AM9513_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AM9513_TIMER pointer for record '%s' is NULL.",
			timer->record->name );
	}

	*num_counters = (*am9513_timer)->num_counters;

	*am9513_interface_array = (*am9513_timer)->am9513_interface_array;

	if ( *am9513_interface_array == (MX_INTERFACE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_INTERFACE array pointer for timer '%s' is NULL.",
			timer->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_timer_initialize_type( long type )
{
	static const char fname[] = "mxd_am9513_timer_initialize_type()";

	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults, *field;
	mx_length_type num_record_fields;
	mx_length_type num_counters_field_index;
	mx_length_type num_counters_varargs_cookie;
	mx_status_type mx_status;

	driver = mx_get_driver_by_type( type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record type %ld not found.", type );
	}

	record_field_defaults = *(driver->record_field_defaults_ptr);
	num_record_fields = *(driver->num_record_fields);

	mx_status = mx_find_record_field_defaults_index(
			record_field_defaults, num_record_fields,
			"num_counters", &num_counters_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie(
		num_counters_field_index, 0, &num_counters_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field_defaults(
			record_field_defaults, num_record_fields,
			"am9513_interface_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_counters_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_am9513_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_AM9513_TIMER *am9513_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	am9513_timer = (MX_AM9513_TIMER *)
				malloc( sizeof(MX_AM9513_TIMER) );

	if ( am9513_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AM9513_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = am9513_timer;
	record->class_specific_function_list
			= &mxd_am9513_timer_timer_function_list;

	timer->record = record;

	timer->mode = MXCM_PRESET_MODE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_timer_finish_record_initialization( MX_RECORD *record )
{
	return mx_timer_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxd_am9513_timer_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_am9513_timer_open()";

	MX_AM9513_TIMER *am9513_timer;
	MX_INTERFACE *am9513_interface_array;
	MX_RECORD *this_record;
	MX_AM9513 *this_am9513;
	uint16_t counter_mode_register;
	int n;
	mx_length_type num_counters, high_order_counter;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s called.", fname));

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL." );
	}

	mx_status = mxd_am9513_timer_get_pointers( record->record_class_struct,
			&am9513_timer, &num_counters,
			&am9513_interface_array, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_counters <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The number of counters (%ld) in record '%s' should be greater than zero.",
			(long) num_counters, record->name );
	}

	mx_status = mxi_am9513_grab_counters( record, num_counters,
						am9513_interface_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/** Initialize the mode register for the 16 least significant bits. **/

	this_record = am9513_interface_array[0].record;

	this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

	n = am9513_interface_array[0].address - 1;

	/*
	 * Initialize the counter mode register.
	 *
	 * Settings:
	 *    Gate high level N.
	 *    Count on rising edge.
	 *    Source = F1.
	 *    Disable special gate.
	 *    Reload from load.
	 *    Count once.
	 *    Binary count.
	 *    Count down.
	 */

	counter_mode_register  = 0x8000;	/* gate high level N */

	counter_mode_register |= 0x0b00;	/* SRC = F1 */

	if ( num_counters > 1 ) {
		/* active high terminal pulse count */

		counter_mode_register |= 0x0001;
	} else {
		/* TC toggled */

		counter_mode_register |= 0x0002;
	}

	this_am9513->counter_mode_register[n] = counter_mode_register;

	mx_status = mxi_am9513_set_counter_mode_register( this_am9513, n );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_AM9513_DEBUG( debug_am9513_ptr, 1 );

	/* Using timers built out of more than one Am9513 counter is not
	 * implemented yet.
	 */

	if ( num_counters > 1 ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Record '%s': Combining more than one Am9513 counter to form "
		"a timer is not yet implemented.", record->name );
	}

	/* Make sure that the gate generated by this counter is turned off. */

	high_order_counter = num_counters - 1;

	this_record = am9513_interface_array[ high_order_counter ].record;

	this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

	n = am9513_interface_array[ high_order_counter ].address - 1;

	mx_status = mxi_am9513_clear_tc( this_am9513, n );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_AM9513_DEBUG( debug_am9513_ptr, 1 );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_timer_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_am9513_timer_close()";

	MX_AM9513_TIMER *am9513_timer;
	MX_INTERFACE *am9513_interface_array;
	mx_length_type num_counters;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s called.", fname));

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL." );
	}

	mx_status = mxd_am9513_timer_get_pointers( record->record_class_struct,
			&am9513_timer, &num_counters,
			&am9513_interface_array, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_counters <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The number of counters (%ld) in record '%s' should be greater than zero.",
			(long) num_counters, record->name );
	}

	mx_status = mxi_am9513_release_counters( record, num_counters,
						am9513_interface_array );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_am9513_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_am9513_timer_is_busy()";

	MX_AM9513_TIMER *am9513_timer;
	MX_INTERFACE *am9513_interface_array;
	MX_RECORD *this_record;
	MX_AM9513 *this_am9513;
	uint8_t am9513_status, mask;
	mx_length_type num_counters;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s called.", fname));

	mx_status = mxd_am9513_timer_get_pointers( timer, &am9513_timer,
			&num_counters, &am9513_interface_array, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find out whether the lowest order counter is still counting. */

	this_record = am9513_interface_array[0].record;

	this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

	mx_status = mxi_am9513_get_status( this_am9513 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_AM9513_DEBUG( debug_am9513_ptr, 1 );

	am9513_status = this_am9513->status_register;

	MX_DEBUG( 2,("%s: am9513_status = 0x%x", fname, am9513_status));

	mask = 1 << am9513_interface_array[0].address;

	if ( am9513_status & mask ) {
		timer->busy = TRUE;
	} else {
		timer->busy = FALSE;
	}

	MX_DEBUG( 2,("%s: busy = %d", fname, (int) timer->busy));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_am9513_timer_start()";

	MX_AM9513_TIMER *am9513_timer;
	MX_INTERFACE *am9513_interface_array;
	MX_RECORD *this_record;
	MX_AM9513 *this_am9513;
	double seconds;
	double clock_ticks_double;
	double ulong_max_double;
	uint32_t clock_ticks_uint32;
	uint16_t counter_mode_register;
	uint16_t ticks_to_count_for;
	uint16_t frequency_scaler_ratio;
	int n;
	mx_length_type num_counters;
	mx_status_type mx_status;

	mx_status = mxd_am9513_timer_get_pointers( timer, &am9513_timer,
			&num_counters, &am9513_interface_array, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	seconds = timer->value;

	MX_DEBUG( 2,("%s invoked for %g seconds.", fname, seconds));

	/* Treat any time less than zero as zero. */

	if ( seconds < 0.0 ) {
		seconds = 0.0;
	}

	if ( num_counters > 1 ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Record '%s': Combining more than one Am9513 counter to form "
		"a timer is not yet implemented.", timer->record->name );
	}

	this_record = am9513_interface_array[0].record;

	this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

	n = am9513_interface_array[0].address - 1;

	/* Configure the counter to count down for the specified number
	 * of ticks.
	 */

	clock_ticks_double = seconds * am9513_timer->clock_frequency;

	MX_DEBUG( 2,("%s: seconds = %g, clock_ticks_double = %g",
		fname, seconds, clock_ticks_double));

	ulong_max_double = (double) MX_ULONG_MAX;

	if ( clock_ticks_double > ulong_max_double ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"%g seconds is too long a measurement time for a clock frequency of %g",
			seconds, am9513_timer->clock_frequency );
	}

	clock_ticks_uint32 = mx_round( clock_ticks_double );

	/* The timer does not count down reliably for less than 3 clock
	 * ticks, so we force 3 clock ticks to be the minimum.
	 */

	if ( clock_ticks_uint32 < 3 ) {
		clock_ticks_uint32 = 3;
	}

	MX_DEBUG( 2,("%s: clock_ticks_uint32 = %lu",
		fname, (unsigned long) clock_ticks_uint32));

	if ( clock_ticks_uint32 < 65536L ) {
		frequency_scaler_ratio = 0x0b00;	/* source = F1 */

		ticks_to_count_for = (uint16_t) clock_ticks_uint32;
	} else
	if ( clock_ticks_uint32 < 1048576L ) {
		frequency_scaler_ratio = 0x0c00;	/* source = F2 */

		ticks_to_count_for = (uint16_t)
					( clock_ticks_uint32 / 16L );
	} else
	if ( clock_ticks_uint32 < 16777216L ) {
		frequency_scaler_ratio = 0x0d00;	/* source = F3 */

		ticks_to_count_for = (uint16_t)
					( clock_ticks_uint32 / 256L );
	} else
	if ( clock_ticks_uint32 < 268435456L ) {
		frequency_scaler_ratio = 0x0e00;	/* source = F4 */

		ticks_to_count_for = (uint16_t)
					( clock_ticks_uint32 / 4096L );
	} else {
		frequency_scaler_ratio = 0x0f00;	/* source = F5 */

		ticks_to_count_for = (uint16_t)
					( clock_ticks_uint32 / 65536L );
	}

	MX_DEBUG( 2,
		("%s: frequency_scaler_ratio = %d, ticks_to_count_for = %hu", 
		fname, frequency_scaler_ratio, ticks_to_count_for));

	/* Set the timer preset value and the divisor
	 * for the clock frequency.
	 */

	counter_mode_register = this_am9513->counter_mode_register[n];

	counter_mode_register &= 0xf0ff;

	counter_mode_register |= frequency_scaler_ratio;

	this_am9513->counter_mode_register[n] = counter_mode_register;

	this_am9513->load_register[n] = ticks_to_count_for;

	mx_status = mxi_am9513_set_counter_mode_register( this_am9513, n );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_am9513_load_counter( this_am9513, n );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_AM9513_DEBUG( debug_am9513_ptr, 1 );

	/* Arm the timer */

	mx_status = mxi_am9513_arm_counter( this_am9513, n );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_AM9513_DEBUG( debug_am9513_ptr, 1 );

	/* Turn the gate signal on and start the timer.
	 *
	 * This step assumes that OUT(n) for the timer is connected
	 * to GATE(n).
	 */

	mx_status = mxi_am9513_set_tc( this_am9513, n );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_AM9513_DEBUG( debug_am9513_ptr, 1 );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_am9513_timer_stop()";

	MX_AM9513_TIMER *am9513_timer;
	MX_INTERFACE *am9513_interface_array;
	MX_RECORD *this_record;
	MX_AM9513 *this_am9513;
	uint16_t counter_mode_register;
	int n;
	double multiplier, result;
	mx_length_type i, num_counters;
	mx_status_type mx_status;

	mx_status = mxd_am9513_timer_get_pointers( timer, &am9513_timer,
			&num_counters, &am9513_interface_array, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Stop the counters from counting. */

	for ( i = 0; i < num_counters; i++ ) {

		this_record = am9513_interface_array[i].record;

		this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

		n = am9513_interface_array[i].address - 1;

		/* Disarm the counter. */

		mx_status = mxi_am9513_disarm_counter( this_am9513, n );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Turn off OUT(n). */

		mx_status = mxi_am9513_clear_tc( this_am9513, n );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_AM9513_DEBUG( debug_am9513_ptr, 1 );
	}

	/* Return the number of seconds left to count. */

	if ( num_counters > 1 ) {

		/* Haven't defined how this is to work yet. */

		timer->value = 0.0;
	} else {
		this_record = am9513_interface_array[0].record;

		this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

		n = am9513_interface_array[0].address - 1;

		counter_mode_register = this_am9513->counter_mode_register[n];

		MX_AM9513_DEBUG( debug_am9513_ptr, 1 );

		switch( counter_mode_register & 0x0f00 ) {
		case 0x0b00:	/* Source = F1 */

			multiplier = 1.0;
			break;

		case 0x0c00:	/* Source = F2 */

			multiplier = 16.0;
			break;

		case 0x0d00:	/* Source = F3 */

			multiplier = 256.0;
			break;

		case 0x0e00:	/* Source = F4 */

			multiplier = 4096.0;
			break;

		case 0x0f00:	/* Source = F5 */

			multiplier = 65536.0;
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal frequency divider found in counter mode register %#hx "
		"for counter '%s:%d' in timer '%s'",
				counter_mode_register,
				this_record->name, n,
				timer->record->name );
		}

		result = multiplier * (double) this_am9513->hold_register[n];

		timer->value = mx_divide_safely( result,
					am9513_timer->clock_frequency );

		MX_DEBUG( 2,("%s: seconds_left = %g", fname, timer->value ));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_timer_get_mode( MX_TIMER *timer )
{
	timer->mode = MXCM_PRESET_MODE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_timer_set_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_am9513_timer_set_mode()";

	if ( timer->mode != MXCM_PRESET_MODE ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"This operation is not yet implemented." );
	}

	return MX_SUCCESSFUL_RESULT;
}

