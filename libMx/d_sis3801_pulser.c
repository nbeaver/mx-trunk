/*
 * Name:    d_sis3801_pulser.c
 *
 * Purpose: MX pulse generator driver for the Struck SIS3801.
 *
 *          This mode of operation is based on the suggestions in
 *          section 18.7.1 of the Struck SIS3801 manual.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_types.h"
#include "mx_driver.h"
#include "mx_clock.h"
#include "mx_vme.h"
#include "mx_mcs.h"
#include "mx_pulse_generator.h"

#include "d_sis3801_pulser.h"

MX_RECORD_FUNCTION_LIST mxd_sis3801_pulser_record_function_list = {
	NULL,
	mxd_sis3801_pulser_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_sis3801_pulser_open,
	NULL,
	NULL,
	NULL
};

MX_PULSE_GENERATOR_FUNCTION_LIST
mxd_sis3801_pulser_pulse_generator_function_list = {
	mxd_sis3801_pulser_busy,
	mxd_sis3801_pulser_start,
	mxd_sis3801_pulser_stop,
	mxd_sis3801_pulser_get_parameter,
	mxd_sis3801_pulser_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_sis3801_pulser_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PULSE_GENERATOR_STANDARD_FIELDS,
	MXD_SIS3801_PULSER_STANDARD_FIELDS
};

long mxd_sis3801_pulser_num_record_fields
		= sizeof( mxd_sis3801_pulser_record_field_defaults )
			/ sizeof( mxd_sis3801_pulser_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_sis3801_pulser_rfield_def_ptr
			= &mxd_sis3801_pulser_record_field_defaults[0];

/* --- */

static mx_status_type
mxd_sis3801_pulser_get_pointers( MX_PULSE_GENERATOR *pulse_generator,
			MX_SIS3801_PULSER **sis3801_pulser,
			const char *calling_fname )
{
	const char fname[] = "mxd_sis3801_pulser_get_pointers()";

	MX_RECORD *sis3801_pulser_record;

	if ( pulse_generator == (MX_PULSE_GENERATOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PULSE_GENERATOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( sis3801_pulser == (MX_SIS3801_PULSER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SIS3801_PULSER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	sis3801_pulser_record = pulse_generator->record;

	if ( sis3801_pulser_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_PULSE_GENERATOR pointer "
		"passed by '%s' is NULL.", calling_fname );
	}

	*sis3801_pulser = (MX_SIS3801_PULSER *)
			sis3801_pulser_record->record_type_struct;

	if ( *sis3801_pulser == (MX_SIS3801_PULSER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SIS3801_PULSER pointer for 'sis3801_pulser' "
		"record '%s' passed by '%s' is NULL.",
				sis3801_pulser_record->name,
				calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_sis3801_pulser_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_sis3801_pulser_create_record_structures()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_SIS3801_PULSER *sis3801_pulser;

	pulse_generator = (MX_PULSE_GENERATOR *)
				malloc( sizeof(MX_PULSE_GENERATOR) );

	if ( pulse_generator == (MX_PULSE_GENERATOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for a MX_PULSE_GENERATOR structure "
		"for record '%s'.", record->name );
	}

	sis3801_pulser = (MX_SIS3801_PULSER *)
				malloc( sizeof(MX_SIS3801_PULSER) );

	if ( sis3801_pulser == (MX_SIS3801_PULSER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannott allocate memory for a MX_SIS3801_PULSER structure"
		"for record '%s'.", record->name );
	}

	record->record_class_struct = pulse_generator;
	record->record_type_struct = sis3801_pulser;

	record->class_specific_function_list =
			&mxd_sis3801_pulser_pulse_generator_function_list;

	pulse_generator->record = record;
	sis3801_pulser->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3801_pulser_open( MX_RECORD *record )
{
	const char fname[] = "mxd_sis3801_pulser_open()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_SIS3801_PULSER *sis3801_pulser;
	mx_uint32_type module_id_register, control_register;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	pulse_generator = (MX_PULSE_GENERATOR *) record->record_class_struct;

	mx_status = mxd_sis3801_pulser_get_pointers( pulse_generator,
						&sis3801_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.", fname, record->name));

	/* Parse the address mode string. */

	MX_DEBUG( 2,("%s: sis3801_pulser->address_mode_name = '%s'",
			fname, sis3801_pulser->address_mode_name));

	mx_status = mx_vme_parse_address_mode(
				sis3801_pulser->address_mode_name,
				&(sis3801_pulser->address_mode) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: sis3801_pulser->address_mode = %lu",
			fname, sis3801_pulser->address_mode));

	/* Reset the SIS3801. */

	mx_status = mx_vme_out32( sis3801_pulser->vme_record,
				sis3801_pulser->crate_number,
				sis3801_pulser->address_mode,
			sis3801_pulser->base_address + MX_SIS3801_RESET_REG,
				1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the module identification register. */

	mx_status = mx_vme_in32( sis3801_pulser->vme_record,
				sis3801_pulser->crate_number,
				sis3801_pulser->address_mode,
	sis3801_pulser->base_address + MX_SIS3801_MODULE_ID_IRQ_CONTROL_REG,
				&module_id_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sis3801_pulser->module_id = ( module_id_register >> 16 ) & 0xffff;
	sis3801_pulser->firmware_version = ( module_id_register >> 12 ) & 0xf;

	MX_DEBUG( 2,("%s: module id = %#lx, firmware version = %lu",
	  fname, sis3801_pulser->module_id, sis3801_pulser->firmware_version));

	/* Compute the maximum allowed prescale factor for this module. */

	/* Other firmware versions may well support 28-bit prescale factors,
	 * but Struck does not give us a way to directly test for their
	 * presence.  All one can do is guess based on the firmware version
	 * number.  If you find that you have a version of the firmware that
	 * supports 28-bit ( or larger? ) prescale factors, then add it as
	 * a case in the switch() statement below.
	 */

	switch( sis3801_pulser->firmware_version ) {
	case 0x9:
	case 0xA:
		sis3801_pulser->maximum_prescale_factor = 268435456; /* 2^28 */
		break;
	default:
		sis3801_pulser->maximum_prescale_factor = 16777216;  /* 2^24 */
		break;
	}

	/* Write the value 1 to the copy disable register so that the SIS3801
	 * does not copy any values to the FIFO.  This ensures that the
	 * SIS3801 will never have a FIFO full condition.
	 */

	mx_status = mx_vme_out32( sis3801_pulser->vme_record,
				sis3801_pulser->crate_number,
				sis3801_pulser->address_mode,
		sis3801_pulser->base_address + MX_SIS3801_COPY_DISABLE_REG,
				1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Enable the 10 MHz to LNE pulser and make sure that the SIS3801
	 * is getting its next event from the internal 10 MHz signal rather
	 * than the external next event logic.
	 */

	control_register = MXF_SIS3801_ENABLE_10MHZ_TO_LNE_PRESCALER;
	control_register |= MXF_SIS3801_DISABLE_EXTERNAL_NEXT;

	mx_status = mx_vme_out32( sis3801_pulser->vme_record,
				sis3801_pulser->crate_number,
				sis3801_pulser->address_mode,
			sis3801_pulser->base_address + MX_SIS3801_CONTROL_REG,
				control_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the control input mode. */

	control_register = ( sis3801_pulser->control_input_mode & 0x3 ) << 2;

	mx_status = mx_vme_out32( sis3801_pulser->vme_record,
				sis3801_pulser->crate_number,
				sis3801_pulser->address_mode,
			sis3801_pulser->base_address + MX_SIS3801_CONTROL_REG,
				control_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Enable counting. */

	mx_status = mx_vme_out32( sis3801_pulser->vme_record,
				sis3801_pulser->crate_number,
				sis3801_pulser->address_mode,
			sis3801_pulser->base_address + MX_SIS3801_CONTROL_REG,
			MXF_SIS3801_CLEAR_SOFTWARE_DISABLE_COUNTING_BIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the pulse train finish time to the current time. */

	sis3801_pulser->finish_time = mx_current_clock_tick();

	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_sis3801_pulser_busy( MX_PULSE_GENERATOR *pulse_generator )
{
	const char fname[] = "mxd_sis3801_pulser_busy()";

	MX_SIS3801_PULSER *sis3801_pulser;
	MX_CLOCK_TICK current_time;
	mx_status_type mx_status;

	mx_status = mxd_sis3801_pulser_get_pointers( pulse_generator,
						&sis3801_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, pulse_generator->record->name));

	/* If busy is set to FALSE, then leave it that way. */

	if ( pulse_generator->busy == FALSE )
		return MX_SUCCESSFUL_RESULT;

	/* If we are configured to count forever, then leave the busy flag
	 * alone and return.
	 */

	if ( pulse_generator->num_pulses == MXF_PGN_FOREVER )
		return MX_SUCCESSFUL_RESULT;

	/* See if we have reached the expected finish time of the pulse train.*/

	current_time = mx_current_clock_tick();

	if ( mx_compare_clock_ticks( current_time,
				sis3801_pulser->finish_time ) >= 0 )
	{
		pulse_generator->busy = FALSE;
	} else {
		pulse_generator->busy = TRUE;
	}

	if ( pulse_generator->busy == FALSE ) {
		MX_DEBUG( 2,
	("%s: busy = %d, current_time = (%lu,%lu), finish_time = (%lu,%lu)",
		fname, pulse_generator->busy, current_time.high_order,
		(unsigned long) current_time.low_order,
		sis3801_pulser->finish_time.high_order,
		(unsigned long) sis3801_pulser->finish_time.low_order));

		/* Stop the pulse train. */

		mx_status = mxd_sis3801_pulser_stop( pulse_generator );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3801_pulser_start( MX_PULSE_GENERATOR *pulse_generator )
{
	const char fname[] = "mxd_sis3801_pulser_start()";

	MX_SIS3801_PULSER *sis3801_pulser;
	MX_CLOCK_TICK start_time, finish_time, countdown_ticks;
	double total_countdown_time;
	mx_status_type mx_status;

	mx_status = mxd_sis3801_pulser_get_pointers( pulse_generator,
						&sis3801_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, pulse_generator->record->name));

	/* Enable the next clock logic. */

	mx_status = mx_vme_out32( sis3801_pulser->vme_record,
				sis3801_pulser->crate_number,
				sis3801_pulser->address_mode,
	sis3801_pulser->base_address + MX_SIS3801_ENABLE_NEXT_CLOCK_LOGIC,
				1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Enable the LNE prescaler.
	 *
	 * This will start the measurement if we are using the SIS3801's own
	 * internal clock.  Otherwise, it makes the SIS3801 ready to start
	 * counting when the first external next clock comes in.
	 */

	mx_status = mx_vme_out32( sis3801_pulser->vme_record,
				sis3801_pulser->crate_number,
				sis3801_pulser->address_mode,
			sis3801_pulser->base_address + MX_SIS3801_CONTROL_REG,
				MXF_SIS3801_ENABLE_LNE_PRESCALER );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pulse_generator->busy = TRUE;

	/* Return if the pulse generator is configured to count forever. */

	if ( pulse_generator->num_pulses == MXF_PGN_FOREVER )
		return MX_SUCCESSFUL_RESULT;

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

	MX_DEBUG( 2,
		("%s: total_countdown_time = %g, countdown_ticks = (%lu,%lu)",
		fname, total_countdown_time, countdown_ticks.high_order,
			(unsigned long) countdown_ticks.low_order ));

	MX_DEBUG( 2,("%s: start_time = (%lu,%lu), finish_time = (%lu,%lu)",
		fname, start_time.high_order,
		(unsigned long) start_time.low_order,
		finish_time.high_order,
		(unsigned long) finish_time.low_order));

	sis3801_pulser->finish_time = finish_time;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3801_pulser_stop( MX_PULSE_GENERATOR *pulse_generator )
{
	const char fname[] = "mxd_sis3801_pulser_stop()";

	MX_SIS3801_PULSER *sis3801_pulser;
	mx_status_type mx_status;

	mx_status = mxd_sis3801_pulser_get_pointers( pulse_generator,
						&sis3801_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, pulse_generator->record->name));

	/* Disable the LNE prescaler. */

	mx_status = mx_vme_out32( sis3801_pulser->vme_record,
				sis3801_pulser->crate_number,
				sis3801_pulser->address_mode,
			sis3801_pulser->base_address + MX_SIS3801_CONTROL_REG,
				MXF_SIS3801_DISABLE_LNE_PRESCALER );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Disable next clock logic. */

	mx_status = mx_vme_out32( sis3801_pulser->vme_record,
				sis3801_pulser->crate_number,
				sis3801_pulser->address_mode,
	sis3801_pulser->base_address + MX_SIS3801_DISABLE_NEXT_CLOCK_LOGIC,
				1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pulse_generator->busy = FALSE;

	/* Set the pulse train finish time to the current time. */

	sis3801_pulser->finish_time = mx_current_clock_tick();

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3801_pulser_get_parameter( MX_PULSE_GENERATOR *pulse_generator )
{
	const char fname[] = "mxd_sis3801_pulser_get_parameter()";

	MX_SIS3801_PULSER *sis3801_pulser;
	mx_uint32_type prescale_factor;
	mx_status_type mx_status;

	mx_status = mxd_sis3801_pulser_get_pointers( pulse_generator,
						&sis3801_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%d)",
		fname, pulse_generator->record->name,
		mx_get_field_label_string( pulse_generator->record,
					pulse_generator->parameter_type ),
		pulse_generator->parameter_type));

	switch( pulse_generator->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:

		/* The SIS3801 does not have a countdown register, so just
		 * report back the value that we last set.
		 */

		break;
	case MXLV_PGN_PULSE_WIDTH:
		/* The SIS3801 does not provide a way to change the width
		 * of a pulse so report back the overhead time of the FIFO
		 * copy operation
		 */

		pulse_generator->pulse_width = 260.0e-9;
		break;
	case MXLV_PGN_PULSE_DELAY:

		/* Pulse delays are not supported for the SIS3801, so
		 * always return 0.
		 */

		pulse_generator->pulse_delay = 0.0;
		break;
	case MXLV_PGN_MODE:
		/* Only pulse mode is supported, so return that. */

		pulse_generator->mode = MXF_PGN_PULSE;
		break;
	case MXLV_PGN_PULSE_PERIOD:
		/* Read the prescale factor and then compute the pulse period.*/

		mx_status = mx_vme_in32( sis3801_pulser->vme_record,
				sis3801_pulser->crate_number,
				sis3801_pulser->address_mode,
		sis3801_pulser->base_address + MX_SIS3801_PRESCALE_FACTOR_REG,
				&prescale_factor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		pulse_generator->pulse_period = mx_divide_safely(
			prescale_factor, MX_SIS3801_10MHZ_INTERNAL_CLOCK );

		break;
	default:
		return mx_pulse_generator_default_get_parameter_handler(
							pulse_generator );
		break;
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3801_pulser_set_parameter( MX_PULSE_GENERATOR *pulse_generator )
{
	const char fname[] = "mxd_sis3801_pulser_set_parameter()";

	MX_SIS3801_PULSER *sis3801_pulser;
	double maximum_pulse_period;
	mx_uint32_type prescale_factor, control_register_value;
	mx_status_type mx_status;

	mx_status = mxd_sis3801_pulser_get_pointers( pulse_generator,
						&sis3801_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	control_register_value = 0;

	MX_DEBUG( 2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%d)",
		fname, pulse_generator->record->name,
		mx_get_field_label_string( pulse_generator->record,
					pulse_generator->parameter_type ),
		pulse_generator->parameter_type));

	switch( pulse_generator->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:
		/* Just store the number of pulses locally since the SIS3801
		 * itself does not have a countdown register.
		 */

		break;
	case MXLV_PGN_PULSE_WIDTH:
		/* The SIS3801 does not provide a way to change the width
		 * of a pulse so silently ignore this request and set
		 * the pulse width to a plausible value.
		 */

		pulse_generator->pulse_width = 260.0e-9;
		break;
	case MXLV_PGN_PULSE_DELAY:

		/* Pulse delays are not supported for the SIS3801_PULSER, so
		 * silently ignore the request.
		 */

		pulse_generator->pulse_delay = 0.0;
		break;
	case MXLV_PGN_MODE:
		if ( pulse_generator->mode != MXF_PGN_PULSE ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Pulse generator mode %d is not supported for "
			"pulse generator '%s'.  "
			"Only pulse mode (1) is supported.",
				pulse_generator->mode,
				pulse_generator->record->name );
		}
		break;
	case MXLV_PGN_PULSE_PERIOD:

		/* Set the prescale factor for the requested pulse period. */

		MX_DEBUG( 2,("%s: pulse_period = %g seconds",
				fname, pulse_generator->pulse_period));

		prescale_factor = mx_round( MX_SIS3801_10MHZ_INTERNAL_CLOCK
					* pulse_generator->pulse_period );

		if ( prescale_factor >=
				sis3801_pulser->maximum_prescale_factor )
		{

			maximum_pulse_period = mx_divide_safely(
			    (double) sis3801_pulser->maximum_prescale_factor,
			    MX_SIS3801_10MHZ_INTERNAL_CLOCK );

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The requested pulse period of %g seconds for pulse generator '%s' "
	"is larger than the maximum allowed pulse period of %g seconds.",
			pulse_generator->pulse_period,
			sis3801_pulser->record->name, 
			maximum_pulse_period );
		}

		prescale_factor -= 1;

		MX_DEBUG( 2,("%s: prescale_factor = %lu",
					fname, prescale_factor));

		mx_status = mx_vme_out32( sis3801_pulser->vme_record,
				sis3801_pulser->crate_number,
				sis3801_pulser->address_mode,
		sis3801_pulser->base_address + MX_SIS3801_PRESCALE_FACTOR_REG,
				prescale_factor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;
	default:
		return mx_pulse_generator_default_set_parameter_handler(
							pulse_generator );
		break;
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

