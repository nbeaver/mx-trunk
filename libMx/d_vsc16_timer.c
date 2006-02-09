/*
 * Name:    d_vsc16_timer.c
 *
 * Purpose: MX driver for a Joerger VSC-16 counter channel used as a timer.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_stdint.h"
#include "mx_vme.h"
#include "mx_timer.h"
#include "i_vsc16.h"
#include "d_vsc16_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_vsc16_timer_record_function_list = {
	NULL,
	mxd_vsc16_timer_create_record_structures,
	mxd_vsc16_timer_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_vsc16_timer_open,
	NULL
};

MX_TIMER_FUNCTION_LIST mxd_vsc16_timer_timer_function_list = {
	mxd_vsc16_timer_is_busy,
	mxd_vsc16_timer_start,
	mxd_vsc16_timer_stop,
	mxd_vsc16_timer_clear,
	mxd_vsc16_timer_read,
	mxd_vsc16_timer_get_mode,
	mxd_vsc16_timer_set_mode,
	mxd_vsc16_timer_set_modes_of_associated_counters
};

/* EPICS timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_vsc16_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_VSC16_TIMER_STANDARD_FIELDS
};

mx_length_type mxd_vsc16_timer_num_record_fields
		= sizeof( mxd_vsc16_timer_record_field_defaults )
		  / sizeof( mxd_vsc16_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_vsc16_timer_rfield_def_ptr
			= &mxd_vsc16_timer_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_vsc16_timer_get_pointers( MX_TIMER *timer,
			MX_VSC16_TIMER **vsc16_timer,
			MX_VSC16 **vsc16,
			const char *calling_fname )
{
	static const char fname[] = "mxd_vsc16_timer_get_pointers()";

	MX_RECORD *vsc16_record;

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_TIMER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( vsc16_timer == (MX_VSC16_TIMER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VSC16_TIMER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( vsc16 == (MX_VSC16 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VSC16 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*vsc16_timer = (MX_VSC16_TIMER *) timer->record->record_type_struct;

	if ( *vsc16_timer == (MX_VSC16_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VSC16_TIMER pointer for record '%s' is NULL.",
			timer->record->name );
	}

	vsc16_record = (*vsc16_timer)->vsc16_record;

	if ( vsc16_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The vsc16_record pointer for record '%s' is NULL.",
			timer->record->name );
	}

	*vsc16 = (MX_VSC16 *) vsc16_record->record_type_struct;

	if ( *vsc16 == (MX_VSC16 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VSC16 pointer for record '%s' is NULL.",
			timer->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*======*/

MX_EXPORT mx_status_type
mxd_vsc16_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_vsc16_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_VSC16_TIMER *vsc16_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for an MX_TIMER structure." );
	}

	vsc16_timer = (MX_VSC16_TIMER *) malloc( sizeof(MX_VSC16_TIMER) );

	if ( vsc16_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for an MX_VSC16_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = vsc16_timer;
	record->class_specific_function_list
			= &mxd_vsc16_timer_timer_function_list;

	timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vsc16_timer_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_timer_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_vsc16_timer_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_vsc16_timer_open()";

	MX_TIMER *timer;
	MX_VSC16_TIMER *vsc16_timer;
	MX_VSC16 *vsc16;
	int counter_number;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	timer = (MX_TIMER *) (record->record_class_struct);

	mx_status = mxd_vsc16_timer_get_pointers( timer,
					&vsc16_timer, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

        /* Check that the counter number is valid. */

        counter_number = vsc16_timer->counter_number;

        if ( counter_number < 1 || counter_number > vsc16->num_counters ) {
                return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
        "VSC16 counter number %d for timer '%s' is out of allowed range 1-%lu",
                        counter_number, record->name,
			(unsigned long) vsc16->num_counters );
        }

	vsc16->counter_record[ counter_number - 1 ] = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vsc16_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_vsc16_timer_is_busy()";

	MX_VSC16_TIMER *vsc16_timer;
	MX_VSC16 *vsc16;
	uint16_t control_register;
	mx_status_type mx_status;

	mx_status = mxd_vsc16_timer_get_pointers( timer,
					&vsc16_timer, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_vme_in16( vsc16->vme_record,
				vsc16->crate_number,
				MXF_VME_A32,
				vsc16->base_address + MX_VSC16_CONTROL_REGISTER,
				&control_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if the ARM bit is set.  The counter will disarm at the 
	 * end of the counting cycle.
	 */

	if ( control_register & 0x1 ) {
		timer->busy = TRUE;
	} else {
		timer->busy = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vsc16_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_vsc16_timer_start()";

	MX_VSC16_TIMER *vsc16_timer;
	MX_VSC16 *vsc16;
	MX_RECORD *vme_record;
	uint32_t preset;
	unsigned long crate, base, preset_address;
	int counter_index;
	double seconds;
	mx_status_type mx_status;

	mx_status = mxd_vsc16_timer_get_pointers( timer,
					&vsc16_timer, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	counter_index = vsc16_timer->counter_number - 1;

	vme_record = vsc16->vme_record;
	crate      = vsc16->crate_number;
	base       = vsc16->base_address;

	/* Set the timer preset. */

	seconds = timer->value;

	preset = mx_round( seconds * vsc16_timer->clock_frequency );

	preset_address = base + MX_VSC16_PRESET_BASE + 4 * counter_index;

	mx_status = mx_vme_out32( vme_record, crate, MXF_VME_A32,
					preset_address, preset );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the timer counting:
	 *
	 *    Configure the counter to disarm on internal IRQ and 
	 *    set the arm bit.
	 */

	mx_status = mx_vme_out16( vme_record, crate, MXF_VME_A32,
					base + MX_VSC16_CONTROL_REGISTER,
					0x1 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_vsc16_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_vsc16_timer_stop()";

	MX_VSC16_TIMER *vsc16_timer;
	MX_VSC16 *vsc16;
	mx_status_type mx_status;

	mx_status = mxd_vsc16_timer_get_pointers( timer,
					&vsc16_timer, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Stop the timer from counting by disarming it. */

	mx_status = mx_vme_out16( vsc16->vme_record,
				vsc16->crate_number,
				MXF_VME_A32,
				vsc16->base_address + MX_VSC16_CONTROL_REGISTER,
				0x0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->value = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vsc16_timer_clear( MX_TIMER *timer )
{
	static const char fname[] = "mxd_vsc16_timer_clear()";

	MX_VSC16_TIMER *vsc16_timer;
	MX_VSC16 *vsc16;
	unsigned long preset_address;
	mx_status_type mx_status;

	mx_status = mxd_vsc16_timer_get_pointers( timer,
					&vsc16_timer, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Clear the counter by overwriting its preset. */

	preset_address = vsc16->base_address + MX_VSC16_PRESET_BASE
				+ 4 * ( vsc16_timer->counter_number - 1 );

	mx_status = mx_vme_out32( vsc16->vme_record,
				vsc16->crate_number,
				MXF_VME_A32,
				preset_address,
				0x0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->value = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vsc16_timer_read( MX_TIMER *timer )
{
	static const char fname[] = "mxd_vsc16_timer_read()";

	MX_VSC16_TIMER *vsc16_timer;
	MX_VSC16 *vsc16;
	uint32_t counter_value;
	unsigned long read_address;
	mx_status_type mx_status;

	mx_status = mxd_vsc16_timer_get_pointers( timer,
					&vsc16_timer, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the counter. */

	read_address = vsc16->base_address + MX_VSC16_READ_BASE
				+ 4 * ( vsc16_timer->counter_number - 1 );

	mx_status = mx_vme_in32( vsc16->vme_record,
				vsc16->crate_number,
				MXF_VME_A32,
				read_address,
				&counter_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->value = mx_divide_safely( (double) counter_value,
					vsc16_timer->clock_frequency );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vsc16_timer_get_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_vsc16_timer_get_mode()";

	MX_VSC16_TIMER *vsc16_timer;
	MX_VSC16 *vsc16;
	uint16_t direction_register, mask;
	mx_status_type mx_status;

	mx_status = mxd_vsc16_timer_get_pointers( timer,
					&vsc16_timer, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The counter mode may be found by reading the direction register. */

	mx_status = mx_vme_in16( vsc16->vme_record,
				vsc16->crate_number,
				MXF_VME_A32,
			vsc16->base_address + MX_VSC16_DIRECTION_REGISTER,
				&direction_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mask = 1 << ( vsc16_timer->counter_number - 1 );

	/* If the mask bit is set, the counter is counting down and is
	 * serving as a preset.  Otherwise, it is counting up.
	 */

	if ( direction_register & mask ) {
		timer->mode = MXCM_PRESET_MODE;
	} else {
		timer->mode = MXCM_COUNTER_MODE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vsc16_timer_set_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_vsc16_timer_set_mode()";

	MX_VSC16_TIMER *vsc16_timer;
	MX_VSC16 *vsc16;
	MX_RECORD *vme_record;
	uint16_t direction_register, mask;
	unsigned long crate, base;
	int existing_mode;
	mx_status_type mx_status;

	mx_status = mxd_vsc16_timer_get_pointers( timer,
					&vsc16_timer, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vme_record = vsc16->vme_record;
	crate      = vsc16->crate_number;
	base       = vsc16->base_address;

	/* See if the counter is already in the correct mode by reading
	 * the direction register.
	 */

	mx_status = mx_vme_in16( vme_record, crate, MXF_VME_A32,
					base + MX_VSC16_DIRECTION_REGISTER,
					&direction_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mask = 1 << ( vsc16_timer->counter_number - 1 );

	/* If the mask bit is set, the counter is counting down and is
	 * serving as a preset.  Otherwise, it is counting up.
	 */

	if ( direction_register & mask ) {
		existing_mode = MXCM_PRESET_MODE;
	} else {
		existing_mode = MXCM_COUNTER_MODE;
	}

	/* If we are already in the correct mode, then we can exit now. */

	if ( existing_mode == timer->mode )
		return MX_SUCCESSFUL_RESULT;

	/* Otherwise, we must reprogram the direction register. */

	switch( timer->mode ) {
	case MXCM_PRESET_MODE:
		direction_register |= mask;
		break;

	case MXCM_COUNTER_MODE:
		direction_register &= ~mask;
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Unsupported preset mode %d requested for VSC16 timer '%s'.  "
	"Only preset mode and counter mode are supported.",
			timer->mode, timer->record->name );
		break;
	}

	mx_status = mx_vme_out16( vme_record, crate, MXF_VME_A32,
					base + MX_VSC16_DIRECTION_REGISTER,
					direction_register );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_vsc16_timer_set_modes_of_associated_counters( MX_TIMER *timer )
{
	static const char fname[]
		= "mxd_vsc16_timer_set_modes_of_associated_counters()";

	MX_VSC16_TIMER *vsc16_timer;
	MX_VSC16 *vsc16;
	uint16_t direction_register;
	mx_status_type mx_status;

	mx_status = mxd_vsc16_timer_get_pointers( timer,
					&vsc16_timer, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We must set the bit for this counter to 1 (count down) and
	 * set the bits for all the other counters to 0 (count up).
	 */

	direction_register = 1 << ( vsc16_timer->counter_number - 1 );

	mx_status = mx_vme_out16( vsc16->vme_record,
				vsc16->crate_number,
				MXF_VME_A32,
			vsc16->base_address + MX_VSC16_DIRECTION_REGISTER,
				direction_register );

	return mx_status;
}

