/*
 * Name:    d_sis3807.c
 *
 * Purpose: MX pulse generator driver for a single output channel of
 *          the Struck SIS3807.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002, 2005-2008, 2010, 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SIS3807_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_driver.h"
#include "mx_clock.h"
#include "mx_vme.h"
#include "mx_pulse_generator.h"

#include "i_sis3807.h"
#include "d_sis3807.h"

MX_RECORD_FUNCTION_LIST mxd_sis3807_record_function_list = {
	NULL,
	mxd_sis3807_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_sis3807_open
};

MX_PULSE_GENERATOR_FUNCTION_LIST mxd_sis3807_pulse_generator_function_list = {
	mxd_sis3807_busy,
	mxd_sis3807_start,
	mxd_sis3807_stop,
	mxd_sis3807_get_parameter,
	mxd_sis3807_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_sis3807_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PULSE_GENERATOR_STANDARD_FIELDS,
	MXD_SIS3807_STANDARD_FIELDS
};

long mxd_sis3807_num_record_fields
		= sizeof( mxd_sis3807_record_field_defaults )
			/ sizeof( mxd_sis3807_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_sis3807_rfield_def_ptr
			= &mxd_sis3807_record_field_defaults[0];

/* --- */

static mx_status_type
mxd_sis3807_get_pointers( MX_PULSE_GENERATOR *pulse_generator,
			MX_SIS3807_PULSER **sis3807_pulser,
			MX_SIS3807 **sis3807,
			const char *calling_fname )
{
	static const char fname[] = "mxd_sis3807_get_pointers()";

	MX_RECORD *sis3807_pulser_record, *sis3807_record;
	MX_SIS3807_PULSER *local_sis3807_pulser;

	if ( pulse_generator == (MX_PULSE_GENERATOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PULSE_GENERATOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	sis3807_pulser_record = pulse_generator->record;

	if ( sis3807_pulser_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_PULSE_GENERATOR pointer "
		"passed by '%s' is NULL.", calling_fname );
	}

	local_sis3807_pulser = (MX_SIS3807_PULSER *)
				sis3807_pulser_record->record_type_struct;

	if ( sis3807_pulser != (MX_SIS3807_PULSER **) NULL ) {
		*sis3807_pulser = local_sis3807_pulser;
	}

	if ( sis3807 != (MX_SIS3807 **) NULL ) {
		sis3807_record = local_sis3807_pulser->sis3807_record;

		if ( sis3807_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The sis3807_record pointer for 'sis3807_pulser' "
			"record '%s' passed by '%s' is NULL.",
				sis3807_pulser_record->name,
				calling_fname );
		}

		*sis3807 = (MX_SIS3807 *) sis3807_record->record_type_struct;

		if ( *sis3807 == (MX_SIS3807 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_SIS3807 pointer for sis3807 record '%s' "
			"used by 'sis3807_pulser' record '%s' "
			"passed by '%s' is NULL.",
				sis3807_record->name,
				sis3807_pulser_record->name,
				calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_sis3807_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_sis3807_create_record_structures()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_SIS3807_PULSER *sis3807_pulser = NULL;

	pulse_generator = (MX_PULSE_GENERATOR *)
				malloc( sizeof(MX_PULSE_GENERATOR) );

	if ( pulse_generator == (MX_PULSE_GENERATOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for a MX_PULSE_GENERATOR structure "
		"for record '%s'.", record->name );
	}

	sis3807_pulser = (MX_SIS3807_PULSER *)
				malloc( sizeof(MX_SIS3807_PULSER) );

	if ( sis3807_pulser == (MX_SIS3807_PULSER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannott allocate memory for a MX_SIS3807_PULSER structure"
		"for record '%s'.", record->name );
	}

	record->record_class_struct = pulse_generator;
	record->record_type_struct = sis3807_pulser;

	record->class_specific_function_list =
				&mxd_sis3807_pulse_generator_function_list;

	pulse_generator->record = record;
	sis3807_pulser->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3807_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_sis3807_open()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_SIS3807_PULSER *sis3807_pulser = NULL;
	MX_SIS3807 *sis3807 = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	pulse_generator = (MX_PULSE_GENERATOR *) record->record_class_struct;

	mx_status = mxd_sis3807_get_pointers( pulse_generator, &sis3807_pulser,
						&sis3807, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3807_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	/* Are we configured for a legal channel number? */

	if ( ( sis3807_pulser->channel_number < 1 )
	  || ( sis3807_pulser->channel_number > sis3807->num_channels ) )
	{
		if ( sis3807->num_channels == 1 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Channel number %ld for 'sis3807_pulser' '%s' is illegal.  "
	"The only allowed channel number for 'sis3807' record '%s' is 1.",
				sis3807_pulser->channel_number, record->name,
				sis3807->record->name );
		} else {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Channel number %ld for 'sis3807_pulser' '%s' is illegal.  "
	"The allowed range of channel numbers for 'sis3807' record '%s' "
	"is from 1 to %ld.",
				sis3807_pulser->channel_number, record->name,
				sis3807->record->name, sis3807->num_channels );
		}
	}

	/* Set the pulse train finish time to the current time. */

	if ( sis3807->burst_register_available == FALSE ) {
		sis3807_pulser->finish_time = mx_current_clock_tick();
	}

	/* Initialize the pulse period, width, etc. */

	mx_status = mx_pulse_generator_initialize( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_sis3807_busy( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_sis3807_busy()";

	MX_SIS3807_PULSER *sis3807_pulser = NULL;
	MX_SIS3807 *sis3807 = NULL;
	MX_CLOCK_TICK current_time;
	uint32_t status_register;
	mx_status_type mx_status;

	mx_status = mxd_sis3807_get_pointers( pulse_generator, &sis3807_pulser,
							&sis3807, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3807_DEBUG
	MX_DEBUG(-2,("%s: record '%s', burst_register_available = %d.",
			fname, pulse_generator->record->name,
			sis3807->burst_register_available ));
#endif

	if ( sis3807->burst_register_available ) {

		/* If the burst register is implemented as in firmware
		 * version 3, then we just need to read the SIS3807 status
		 * register to see if the pulser is still enabled.
		 */

		mx_status = mx_vme_in32( sis3807->vme_record,
				sis3807->crate_number,
				sis3807->address_mode,
				sis3807->base_address + MX_SIS3807_STATUS_REG,
				&status_register );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXD_SIS3807_DEBUG
		MX_DEBUG(-2,("%s: SIS3807 '%s' status register = %#lx",
			fname, sis3807->record->name,
			(unsigned long) status_register ));
#endif

		if ( status_register & MXF_SIS3807_STATUS_ENABLE_PULSER_1 ) {
			pulse_generator->busy = TRUE;
		} else {
			pulse_generator->busy = FALSE;
		}
	} else {
		/* Otherwise, we must use the elapsed time to estimate
		 * whether or not it is time to disable the pulser.
		 */

		/* If busy is set to FALSE, then leave it that way. */

		if ( pulse_generator->busy == FALSE )
			return MX_SUCCESSFUL_RESULT;

		/* If we are configured to count forever, then leave
		 * the busy flag alone and return.
		 */

		if ( pulse_generator->num_pulses == MXF_PGN_FOREVER )
			return MX_SUCCESSFUL_RESULT;

		/* See if we have reached the expected finish time
		 * of the pulse train.
		 */

		current_time = mx_current_clock_tick();

		if ( mx_compare_clock_ticks( current_time,
				sis3807_pulser->finish_time ) >= 0 )
		{
			pulse_generator->busy = FALSE;
		} else {
			pulse_generator->busy = TRUE;
		}

		if ( pulse_generator->busy == FALSE ) {

#if MXD_SIS3807_DEBUG
			MX_DEBUG(-2,
	("%s: busy = %d, current_time = (%lu,%lu), finish_time = (%lu,%lu)",
		fname, pulse_generator->busy, current_time.high_order,
		(unsigned long) current_time.low_order,
		sis3807_pulser->finish_time.high_order,
		(unsigned long) sis3807_pulser->finish_time.low_order));
#endif

			/* Stop the pulse train. */

			mx_status = mxd_sis3807_stop( pulse_generator );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_sis3807_send_single_pulse( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_sis3807_send_single_pulse()";

	MX_SIS3807_PULSER *sis3807_pulser = NULL;
	MX_SIS3807 *sis3807 = NULL;
	uint32_t pulse_register;
	mx_status_type mx_status;

	mx_status = mxd_sis3807_get_pointers( pulse_generator, &sis3807_pulser,
						&sis3807, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3807_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, pulse_generator->record->name));
#endif

	pulse_register = (uint32_t) sis3807->base_address;

	switch( sis3807_pulser->channel_number ) {
	case 1:
		pulse_register += MX_SIS3807_PULSE_CHANNEL_1;
		break;
	case 2:
		pulse_register += MX_SIS3807_PULSE_CHANNEL_2;
		break;
	case 3:
		pulse_register += MX_SIS3807_PULSE_CHANNEL_3;
		break;
	case 4:
		pulse_register += MX_SIS3807_PULSE_CHANNEL_4;
		break;
	}

	/* Send the pulse. */

	mx_status = mx_vme_out32( sis3807->vme_record,
					sis3807->crate_number,
					sis3807->address_mode,
					pulse_register,
					1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3807_DEBUG
	MX_DEBUG(-2,("%s: single pulse sent.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_sis3807_start_with_burst_register( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_sis3807_start_with_burst_register()";

	MX_SIS3807_PULSER *sis3807_pulser = NULL;
	MX_SIS3807 *sis3807 = NULL;
	uint32_t enable_channel_value;
	mx_status_type mx_status;

	mx_status = mxd_sis3807_get_pointers( pulse_generator, &sis3807_pulser,
						&sis3807, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Since the SIS3807 has special hardware for generating a
	 * single pulse, we must treat pulse_generator->num_pulses == 1
	 * as a special case.
	 */

	if ( pulse_generator->num_pulses == 1 ) {
		mx_status = mxd_sis3807_send_single_pulse( pulse_generator );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/***** If we get here, then pulse_generator->num_pulses != 1. *****/

	/* Enable the channel. */

	enable_channel_value = 1 << ( 3 + sis3807_pulser->channel_number );

#if MXD_SIS3807_DEBUG
	MX_DEBUG(-2,("%s: pulser '%s', enable_channel_value = %#lx",
			fname, pulse_generator->record->name,
			(unsigned long) enable_channel_value ));
#endif

	mx_status = mx_vme_out32( sis3807->vme_record,
				sis3807->crate_number,
				sis3807->address_mode,
				sis3807->base_address + MX_SIS3807_CONTROL_REG,
				enable_channel_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pulse_generator->busy = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_sis3807_start_without_burst_register( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[]
		= "mxd_sis3807_start_without_burst_register()";

	MX_SIS3807_PULSER *sis3807_pulser = NULL;
	MX_SIS3807 *sis3807 = NULL;
	MX_CLOCK_TICK start_time, finish_time, countdown_ticks;
	MX_CLOCK_TICK pulse_duration_ticks;
	double total_countdown_time, pulse_time;
	uint32_t enable_channel_value;
	mx_status_type mx_status;

	mx_status = mxd_sis3807_get_pointers( pulse_generator, &sis3807_pulser,
						&sis3807, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3807_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, pulse_generator->record->name));
#endif

	/* Since the SIS3807 has special hardware for generating a
	 * single pulse, we must treat pulse_generator->num_pulses == 1
	 * as a special case.
	 */

	if ( pulse_generator->num_pulses == 1 ) {
		mx_status = mxd_sis3807_send_single_pulse( pulse_generator );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		pulse_generator->busy = TRUE;

		/* Compute when the pulse will be over. */

		start_time = mx_current_clock_tick();

		if ( pulse_generator->mode != MXF_PGN_PULSE ) {
			sis3807_pulser->finish_time = start_time;
		} else {
			pulse_time = 80.0e-9;

			pulse_duration_ticks =
				mx_convert_seconds_to_clock_ticks( pulse_time );

			sis3807_pulser->finish_time = mx_add_clock_ticks(
					start_time, pulse_duration_ticks );
		}

		return MX_SUCCESSFUL_RESULT;
	}

	/***** If we get here, then pulse_generator->num_pulses != 1. *****/

	/* Enable the channel. */

	enable_channel_value = 1 << ( 3 + sis3807_pulser->channel_number );

	mx_status = mx_vme_out32( sis3807->vme_record,
				sis3807->crate_number,
				sis3807->address_mode,
				sis3807->base_address + MX_SIS3807_CONTROL_REG,
				enable_channel_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pulse_generator->busy = TRUE;

	/* Return if the pulse generator is configured to count forever. */

	if ( pulse_generator->num_pulses == MXF_PGN_FOREVER ) {

#if MXD_SIS3807_DEBUG
		MX_DEBUG(-2,("%s: pulser '%s' will count until stopped.",
			fname, pulse_generator->record->name ));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	/* Record the time when the measurement should be finished. */

	total_countdown_time = pulse_generator->pulse_period
				* (double) pulse_generator->num_pulses;

	/* Give a little extra delay time to try to avoid turning off the
	 * pulse generator early.
	 */

	total_countdown_time += 0.02;		/* Delay time in seconds */

	countdown_ticks
		= mx_convert_seconds_to_clock_ticks( total_countdown_time );

	start_time = mx_current_clock_tick();

	finish_time = mx_add_clock_ticks( start_time, countdown_ticks );

#if MXD_SIS3807_DEBUG
	MX_DEBUG(-2,
		("%s: total_countdown_time = %g, countdown_ticks = (%lu,%lu)",
		fname, total_countdown_time, countdown_ticks.high_order,
			(unsigned long) countdown_ticks.low_order ));

	MX_DEBUG(-2,("%s: start_time = (%lu,%lu), finish_time = (%lu,%lu)",
		fname, start_time.high_order,
		(unsigned long) start_time.low_order,
		finish_time.high_order,
		(unsigned long) finish_time.low_order));
#endif

	sis3807_pulser->finish_time = finish_time;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3807_start( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_sis3807_start()";

	MX_SIS3807_PULSER *sis3807_pulser = NULL;
	MX_SIS3807 *sis3807 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sis3807_get_pointers( pulse_generator, &sis3807_pulser,
						&sis3807, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sis3807->burst_register_available ) {
		mx_status =
		    mxd_sis3807_start_with_burst_register( pulse_generator );
	} else {
		mx_status =
		    mxd_sis3807_start_without_burst_register( pulse_generator );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sis3807_stop( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_sis3807_stop()";

	MX_SIS3807_PULSER *sis3807_pulser = NULL;
	MX_SIS3807 *sis3807 = NULL;
	uint32_t disable_channel_value;
	mx_status_type mx_status;

	mx_status = mxd_sis3807_get_pointers( pulse_generator, &sis3807_pulser,
						&sis3807, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3807_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, pulse_generator->record->name));
#endif

	/* Disable the channel. */

	disable_channel_value = 1 << ( 11 + sis3807_pulser->channel_number );

	mx_status = mx_vme_out32( sis3807->vme_record,
				sis3807->crate_number,
				sis3807->address_mode,
				sis3807->base_address + MX_SIS3807_CONTROL_REG,
				disable_channel_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pulse_generator->busy = FALSE;

	/* Set the pulse train finish time to the current time. */

	if ( sis3807->burst_register_available == FALSE ) {
		sis3807_pulser->finish_time = mx_current_clock_tick();
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3807_get_parameter( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_sis3807_get_parameter()";

	MX_SIS3807_PULSER *sis3807_pulser = NULL;
	MX_SIS3807 *sis3807 = NULL;
	uint32_t status_register, mask;
	mx_status_type mx_status;

	mx_status = mxd_sis3807_get_pointers( pulse_generator, &sis3807_pulser,
						&sis3807, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3807_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%d)",
		fname, pulse_generator->record->name,
		mx_get_field_label_string( pulse_generator->record,
					pulse_generator->parameter_type ),
		pulse_generator->parameter_type));
#endif

	switch( pulse_generator->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:

		/* The burst register, if present, is write only, so just
		 * report back the most recently set value.
		 */

		break;
	case MXLV_PGN_PULSE_WIDTH:

		/* The pulse width register, if present, is write only
		 * so we can only report back the most recently set value.
		 *
		 * If the pulse width register is _not_ present, then we
		 * attempt to figure out the width of the pulse from the
		 * rest of the configuration of the pulser.
		 */

		if ( sis3807->pulse_width_available == FALSE ) {
			if ( pulse_generator->mode == MXF_PGN_PULSE ) {
				if ( pulse_generator->num_pulses == 1 ) {
					pulse_generator->pulse_width = 80.0e-9;
				} else {
					pulse_generator->pulse_width = 60.0e-9;
				}
			} else {
				if ( pulse_generator->num_pulses == 1 ) {
					pulse_generator->pulse_width = DBL_MAX;
				} else {
					pulse_generator->pulse_width =
					  0.5 * pulse_generator->pulse_period;
				}
			}
		}
		break;
	case MXLV_PGN_PULSE_DELAY:

		/* Pulse delays are not supported for the SIS3807, so
		 * always return 0.
		 */

		pulse_generator->pulse_delay = 0.0;
		break;
	case MXLV_PGN_MODE:
		/* Read the SIS3807 status register. */

		mx_status = mx_vme_in32( sis3807->vme_record,
				sis3807->crate_number,
				sis3807->address_mode,
				sis3807->base_address + MX_SIS3807_STATUS_REG,
				&status_register );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mask = 1 << ( 15 + sis3807_pulser->channel_number );

		if ( status_register & mask ) {
			pulse_generator->mode = MXF_PGN_SQUARE_WAVE;
		} else {
			pulse_generator->mode = MXF_PGN_PULSE;
		}
		break;
	case MXLV_PGN_PULSE_PERIOD:
		/* The pulse preset registers are write only, so just report
		 * the value that we last set.
		 */

		break;
	default:
		return mx_pulse_generator_default_get_parameter_handler(
							pulse_generator );
	}

#if MXD_SIS3807_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

#define TWO_TO_THE_32ND_POWER_MINUS_ONE    4294967295UL

#define TWO_TO_THE_8TH_POWER_MINUS_ONE     255

#define PULSE_WIDTH_STEP_SIZE              20.0e-9       /* in seconds */

MX_EXPORT mx_status_type
mxd_sis3807_set_parameter( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_sis3807_set_parameter()";

	MX_SIS3807_PULSER *sis3807_pulser = NULL;
	MX_SIS3807 *sis3807 = NULL;
	double time_quantum, real_preset_value_plus_one;
	uint32_t preset_value_plus_one, control_register;
	uint32_t preset_value, preset_register;
	uint32_t pulse_width_register, num_pulses;
	unsigned long flags;
	mx_status_type mx_status;

	mx_status = mxd_sis3807_get_pointers( pulse_generator, &sis3807_pulser,
						&sis3807, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	control_register = 0;

#if MXD_SIS3807_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%d)",
		fname, pulse_generator->record->name,
		mx_get_field_label_string( pulse_generator->record,
					pulse_generator->parameter_type ),
		pulse_generator->parameter_type));
#endif

	switch( pulse_generator->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:
		if ( sis3807->burst_register_available ) {

#if ( TWO_TO_THE_32ND_POWER_MINUS_ONE < ULONG_MAX )

			if ( pulse_generator->num_pulses
				> TWO_TO_THE_32ND_POWER_MINUS_ONE )
			{
				/* This case can't happen on a machine where
				 * unsigned long is 32 bits.
				 */

				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Requested number of pulses %lu for pulse generator '%s' is greater "
	"than the maximum allowed number of pulses %lu.",
					pulse_generator->num_pulses,
					pulse_generator->record->name,
					TWO_TO_THE_32ND_POWER_MINUS_ONE );
			}

#endif /* TWO_TO_THE_32ND_POWER_MINUS_ONE < ULONG_MAX */

			num_pulses = (uint32_t) pulse_generator->num_pulses;

/* FIXME: (WML) At the moment (Oct. 4, 2002), the last pulse generated by
 *              the SIS3807 is a runt pulse with an extremely narrow width
 *              that is useless for many purposes.  Temporarily, I am
 *              working around this by adding one extra pulse to take
 *              the place of the runt pulse.  However, num_pulses equal
 *              to 0 or 1 have special meanings for the SIS3807, so the
 *              extra pulse is only added for the case of num_pulses > 1.
 */
			flags = sis3807_pulser->sis3807_flags;

			if ( flags & MXF_SIS3807_RUNT_PULSE_FIX ) {
				if ( num_pulses > 1 ) {
					num_pulses++;
				}
			}

#if MXD_SIS3807_DEBUG
			MX_DEBUG(-2,
			    ("%s: num_pulses = %lu", fname, num_pulses));
#endif

			mx_status = mx_vme_out32( sis3807->vme_record,
					sis3807->crate_number,
					sis3807->address_mode,
			sis3807->base_address + MX_SIS3807_BURST_REGISTER,
					num_pulses );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
		break;
	case MXLV_PGN_PULSE_DELAY:

		/* Pulse delays are not supported for the SIS3807, so
		 * silently ignore the request.
		 */

		pulse_generator->pulse_delay = 0.0;
		break;
	case MXLV_PGN_MODE:
		switch( pulse_generator->mode ) {
		case MXF_PGN_PULSE:
			control_register =
				1 << ( 23 + sis3807_pulser->channel_number );
			break;
		case MXF_PGN_SQUARE_WAVE:
			control_register =
				1 << ( 15 + sis3807_pulser->channel_number );
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
		"Pulse mode %ld is not supported for pulse generator '%s'.  "
		"Only pulse mode (1) and square wave mode (2) are supported.",
				pulse_generator->mode,
				pulse_generator->record->name );
		}
		/* Reprogram the waveform mode. */

		mx_status = mx_vme_out32( sis3807->vme_record,
				sis3807->crate_number,
				sis3807->address_mode,
				sis3807->base_address + MX_SIS3807_CONTROL_REG,
				control_register );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	case MXLV_PGN_PULSE_WIDTH:
		if ( sis3807->pulse_width_available ) {

			if ( pulse_generator->pulse_width
					< PULSE_WIDTH_STEP_SIZE )
			{
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Requested pulse width %g for pulse generator '%s' is less "
		"than the minimum allowed pulse width %g.",
					pulse_generator->pulse_width,
					pulse_generator->record->name,
					PULSE_WIDTH_STEP_SIZE );
			}

			/* The pulse width register specifies the pulse
			 * width in units of 20 nanoseconds.
			 */

			pulse_width_register =
					(uint32_t) mx_round( mx_divide_safely(
						pulse_generator->pulse_width,
						PULSE_WIDTH_STEP_SIZE ) );

			if ( pulse_width_register >
					TWO_TO_THE_8TH_POWER_MINUS_ONE )
			{
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Requested pulse width %g for pulse generator '%s' is greater "
		"than the maximum allowed pulse width %g.",
					pulse_generator->pulse_width,
					pulse_generator->record->name,
	      PULSE_WIDTH_STEP_SIZE * (double) TWO_TO_THE_8TH_POWER_MINUS_ONE );
			}

			mx_status = mx_vme_out32( sis3807->vme_record,
						sis3807->crate_number,
						sis3807->address_mode,
				sis3807->base_address + MX_SIS3807_PULSE_WIDTH,
						pulse_width_register );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Save the actual pulse width. */

			pulse_generator->pulse_width =
			  PULSE_WIDTH_STEP_SIZE * (double) pulse_width_register;
		} else {
			/* This version of the SIS3807 firmware does not
			 * provide a way to change the width of a pulse
			 * so silently ignore this request and set
			 * the pulse width to a plausible value.
			 */

			if ( pulse_generator->mode == MXF_PGN_PULSE ) {
				if ( pulse_generator->num_pulses == 1 ) {
					pulse_generator->pulse_width = 80.0e-9;
				} else {
					pulse_generator->pulse_width = 60.0e-9;
				}
			} else {
				/* Square wave */

				pulse_generator->pulse_width =
					  0.5 * pulse_generator->pulse_period;
			}
		}

#if MXD_SIS3807_DEBUG
		MX_DEBUG(-2,("%s: real pulse_width = %g",
			fname, pulse_generator->pulse_width));
#endif
		break;

	case MXLV_PGN_PULSE_PERIOD:
		/* Compute the preset register value from the pulse period. */

		if ( sis3807->enable_factor_8_predivider ) {
			time_quantum = 800.0e-9;
		} else {
			time_quantum = 100.0e-9;
		}
		if ( pulse_generator->mode == MXF_PGN_SQUARE_WAVE ) {
			time_quantum *= 2.0;
		}

		real_preset_value_plus_one = mx_divide_safely(
				pulse_generator->pulse_period, time_quantum );

		preset_value_plus_one =
			(uint32_t) mx_round( real_preset_value_plus_one );

#if MXD_SIS3807_DEBUG
		MX_DEBUG(-2,("%s: pulse_period = %g, time_quantum = %g",
			fname, pulse_generator->pulse_period, time_quantum));

		MX_DEBUG(-2,("%s: real_preset_value_plus_one = %g",
			fname, real_preset_value_plus_one));

		MX_DEBUG(-2,("%s: preset_value_plus_one = %lu",
			fname, preset_value_plus_one));
#endif

		if ( preset_value_plus_one == 0 ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The pulse period (%g seconds) requested for "
			"pulse generator '%s' is shorter than the "
			"minimum period of %g seconds.",
				pulse_generator->pulse_period,
				pulse_generator->record->name,
				time_quantum );
		}
		if ( preset_value_plus_one >= MX_SIS3807_MAXIMUM_PRESET ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The pulse period (%g seconds) requested for "
			"pulse generator '%s' is longer than the "
			"maximum period of %g seconds.",
				pulse_generator->pulse_period,
				pulse_generator->record->name,
				time_quantum * (double)
					( 1 + MX_SIS3807_MAXIMUM_PRESET ) );
		}

		preset_value = preset_value_plus_one - 1;

		/* Figure out which register to write the preset_value to. */

		preset_register = (uint32_t) sis3807->base_address;

		switch( sis3807_pulser->channel_number ) {
		case 1:
			preset_register += MX_SIS3807_PRESET_FACTOR_PULSER_1;
			break;
		case 2:
			preset_register += MX_SIS3807_PRESET_FACTOR_PULSER_2;
			break;
		case 3:
			preset_register += MX_SIS3807_PRESET_FACTOR_PULSER_3;
			break;
		case 4:
			preset_register += MX_SIS3807_PRESET_FACTOR_PULSER_4;
			break;
		}

		/* Update the preset. */

		mx_status = mx_vme_out32( sis3807->vme_record,
					sis3807->crate_number,
					sis3807->address_mode,
					preset_register,
					preset_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Save the actual pulse period. */

		pulse_generator->pulse_period =
			time_quantum * (double) preset_value_plus_one;

#if MXD_SIS3807_DEBUG
		MX_DEBUG(-2,("%s: real pulse_period = %g",
		 	fname, pulse_generator->pulse_period ));
#endif
		break;
	default:
		return mx_pulse_generator_default_set_parameter_handler(
							pulse_generator );
	}

#if MXD_SIS3807_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

