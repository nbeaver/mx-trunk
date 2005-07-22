/*
 * Name:    d_sis3801.c
 *
 * Purpose: MX MCS driver for the Struck SIS3801 multichannel scaler.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2001-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SIS3801_DEBUG		FALSE

#define MXD_SIS3801_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_types.h"
#include "mx_driver.h"
#include "mx_clock.h"
#include "mx_vme.h"
#include "mx_pulse_generator.h"
#include "mx_mcs.h"

#include "d_sis3801.h"

#if MXD_SIS3801_DEBUG_TIMING
#include "mx_hrt_debug.h"
#endif

MX_RECORD_FUNCTION_LIST mxd_sis3801_record_function_list = {
	mxd_sis3801_initialize_type,
	mxd_sis3801_create_record_structures,
	mxd_sis3801_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_sis3801_open,
	mxd_sis3801_close,
	NULL,
	mxd_sis3801_open
};

MX_MCS_FUNCTION_LIST mxd_sis3801_mcs_function_list = {
	mxd_sis3801_start,
	mxd_sis3801_stop,
	mxd_sis3801_clear,
	mxd_sis3801_busy,
	mxd_sis3801_read_all,
	mxd_sis3801_read_scaler,
	mxd_sis3801_read_measurement,
	NULL,
	mxd_sis3801_get_parameter,
	mxd_sis3801_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_sis3801_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCS_STANDARD_FIELDS,
	MXD_SIS3801_STANDARD_FIELDS
};

long mxd_sis3801_num_record_fields
		= sizeof( mxd_sis3801_record_field_defaults )
			/ sizeof( mxd_sis3801_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_sis3801_rfield_def_ptr
			= &mxd_sis3801_record_field_defaults[0];

#if MXD_SIS3801_DEBUG
#  define MXD_SIS3801_SHOW_STATUS \
	do {								\
		mx_uint32_type my_status_register;			\
									\
		mx_vme_in32( sis3801->vme_record,			\
			sis3801->crate_number,				\
			sis3801->address_mode,				\
			sis3801->base_address + MX_SIS3801_STATUS_REG,	\
			&my_status_register );				\
									\
		MX_DEBUG(-2,("%s: SHOW_STATUS - status_register = %#lx",\
			fname, (unsigned long) my_status_register));	\
	} while(0)
#else
#  define MXD_SIS3801_SHOW_STATUS
#endif

/* --- */

static mx_status_type
mxd_sis3801_get_pointers( MX_MCS *mcs,
			MX_SIS3801 **sis3801,
			const char *calling_fname )
{
	static const char fname[] = "mxd_sis3801_get_pointers()";

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCS pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( sis3801 == (MX_SIS3801 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SIS3801 pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mcs->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for the MX_MCS structure passed by '%s' is NULL.",
			calling_fname );
	}

	*sis3801 = (MX_SIS3801 *) (mcs->record->record_type_struct);

	if ( *sis3801 == (MX_SIS3801 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SIS3801 pointer for record '%s' is NULL.",
			mcs->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_sis3801_initialize_type( long record_type )
{
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	long num_record_fields;
	long maximum_num_scalers_varargs_cookie;
	long maximum_num_measurements_varargs_cookie;
	mx_status_type status;

	status = mx_mcs_initialize_type( record_type,
				&num_record_fields,
				&record_field_defaults,
				&maximum_num_scalers_varargs_cookie,
				&maximum_num_measurements_varargs_cookie );

	return status;
}

MX_EXPORT mx_status_type
mxd_sis3801_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_sis3801_create_record_structures()";

	MX_MCS *mcs;
	MX_SIS3801 *sis3801;

	mcs = (MX_MCS *) malloc( sizeof(MX_MCS) );

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_MCS structure." );
	}

	sis3801 = (MX_SIS3801 *) malloc( sizeof(MX_SIS3801) );

	if ( sis3801 == (MX_SIS3801 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_SIS3801 structure." );
	}

	record->record_class_struct = mcs;
	record->record_type_struct = sis3801;

	record->class_specific_function_list = &mxd_sis3801_mcs_function_list;

	mcs->record = record;
	sis3801->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3801_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_mcs_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sis3801_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_sis3801_open()";

	MX_MCS *mcs;
	MX_SIS3801 *sis3801;
	mx_uint32_type module_id_register, control_register, status_register;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mcs = (MX_MCS *) record->record_class_struct;

	mx_status = mxd_sis3801_get_pointers( mcs, &sis3801, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Parse the address mode string. */

#if MXD_SIS3801_DEBUG
	MX_DEBUG(-2,("%s: sis3801->address_mode_name = '%s'",
			fname, sis3801->address_mode_name));
#endif

	mx_status = mx_vme_parse_address_mode( sis3801->address_mode_name,
						&(sis3801->address_mode) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3801_DEBUG
	MX_DEBUG(-2,("%s: sis3801->address_mode = %lu",
			fname, sis3801->address_mode));
#endif

	/* Reset the SIS3801. */

	mx_status = mx_vme_out32( sis3801->vme_record,
				sis3801->crate_number,
				sis3801->address_mode,
				sis3801->base_address + MX_SIS3801_RESET_REG,
				1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the FIFO size. */

	mx_status = mx_vme_in32( sis3801->vme_record,
				sis3801->crate_number,
				sis3801->address_mode,
				sis3801->base_address + MX_SIS3801_STATUS_REG,
				&status_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( status_register ) {
	case 0x300:
#if MXD_SIS3801_DEBUG
		MX_DEBUG(-2,("%s: 64K FIFO installed.", fname));
#endif
		sis3801->fifo_size_in_kwords = 64;
		break;

	case 0x100:
#if MXD_SIS3801_DEBUG
		MX_DEBUG(-2,("%s: 256K FIFO installed.", fname));
#endif
		sis3801->fifo_size_in_kwords = 256;
		break;
	default:
		mx_warning( "Power up status register value %#lx is not "
		"either 0x300 (for 64K FIFO) or 0x100 (for 256K FIFO).",
			status_register );

		sis3801->fifo_size_in_kwords = -1;
		break;
	}

	/* Read the module identification register. */

	mx_status = mx_vme_in32( sis3801->vme_record,
				sis3801->crate_number,
				sis3801->address_mode,
		sis3801->base_address + MX_SIS3801_MODULE_ID_IRQ_CONTROL_REG,
				&module_id_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sis3801->module_id = ( module_id_register >> 16 ) & 0xffff;
	sis3801->firmware_version = ( module_id_register >> 12 ) & 0xf;

#if MXD_SIS3801_DEBUG
	MX_DEBUG(-2,("%s: module id = %#lx, firmware version = %lu",
		fname, sis3801->module_id, sis3801->firmware_version));
#endif

	/* Compute the maximum allowed prescale factor for this module. */

	/* Other firmware versions may well support 28-bit prescale factors,
	 * but Struck does not give us a way to directly test for their
	 * presence.  All one can do is guess based on the firmware version
	 * number.  If you find that you have a version of the firmware that
	 * supports 28-bit ( or larger? ) prescale factors, then add it as
	 * a case in the switch() statement below.
	 */

	switch( sis3801->firmware_version ) {
	case 0x9:
	case 0xA:
		sis3801->maximum_prescale_factor = 268435456;   /* 2^28 */
		break;
	default:
		sis3801->maximum_prescale_factor = 16777216;	/* 2^24 */
		break;
	}

	if ( sis3801->sis3801_flags & MXF_SIS3801_USE_REFERENCE_PULSER ) {

		/* Enable reference pulses. */

		mx_status = mx_vme_out32( sis3801->vme_record,
					sis3801->crate_number,
					sis3801->address_mode,
	sis3801->base_address + MX_SIS3801_ENABLE_REFERENCE_PULSE_CHANNEL_1,
					1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Set the control input mode. */

	control_register = ( sis3801->control_input_mode & 0x3 ) << 2;

	mx_status = mx_vme_out32( sis3801->vme_record,
				sis3801->crate_number,
				sis3801->address_mode,
				sis3801->base_address + MX_SIS3801_CONTROL_REG,
				control_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Enable counting. */

	mx_status = mx_vme_out32( sis3801->vme_record,
				sis3801->crate_number,
				sis3801->address_mode,
				sis3801->base_address + MX_SIS3801_CONTROL_REG,
			MXF_SIS3801_CLEAR_SOFTWARE_DISABLE_COUNTING_BIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the measurement finish time to the current time. */

	sis3801->finish_time = mx_current_clock_tick();

	mcs->measurement_time = 0.0;

	/* We assume there are no counts remaining to be read in the FIFO. */

	sis3801->counts_available_in_fifo = FALSE;

	/* Turn on the user LED to show that we are initialized. */

#if MXD_SIS3801_DEBUG
	MX_DEBUG(-2,("%s: Turning on user LED.", fname));
#endif

	mx_status = mx_vme_out32( sis3801->vme_record,
				sis3801->crate_number,
				sis3801->address_mode,
				sis3801->base_address + MX_SIS3801_CONTROL_REG,
				0x1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3801_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3801_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_sis3801_close()";

	MX_MCS *mcs;
	MX_SIS3801 *sis3801;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mcs = (MX_MCS *) record->record_class_struct;

	mx_status = mxd_sis3801_get_pointers( mcs, &sis3801, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off the user LED to show that we have shut down. */

#if MXD_SIS3801_DEBUG
	MX_DEBUG(-2,("%s: Turning off user LED.", fname));
#endif

	mx_status = mx_vme_out32( sis3801->vme_record,
				sis3801->crate_number,
				sis3801->address_mode,
				sis3801->base_address + MX_SIS3801_CONTROL_REG,
				0x100 );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_sis3801_start( MX_MCS *mcs )
{
	static const char fname[] = "mxd_sis3801_start()";

	MX_SIS3801 *sis3801;
	MX_CLOCK_TICK start_time, finish_time, measurement_ticks;
	double total_measurement_time, maximum_measurement_time;
	double pulse_period, clock_frequency;
	mx_uint32_type prescale_factor, control_register;
	mx_status_type mx_status;

	mx_status = mxd_sis3801_get_pointers( mcs, &sis3801, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3801_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, mcs->record->name));
#endif

	/* Stop the MCS. */

	mx_status = mxd_sis3801_stop( mcs );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Clear the MCS. */

	mx_status = mxd_sis3801_clear( mcs );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If external channel advance is disabled, enable the internal
	 * 10 MHz clock so that it can do the channel advance.
	 */

	if ( mcs->external_channel_advance_record == NULL ) {
		mcs->external_channel_advance = 0;
	}

	control_register = 0;

	if ( mcs->external_channel_advance ) {
		control_register |= MXF_SIS3801_DISABLE_10MHZ_TO_LNE_PRESCALER;
		control_register |= MXF_SIS3801_ENABLE_EXTERNAL_NEXT;
	} else {
		control_register |= MXF_SIS3801_ENABLE_10MHZ_TO_LNE_PRESCALER;
		control_register |= MXF_SIS3801_DISABLE_EXTERNAL_NEXT;
	}

	mx_status = mx_vme_out32( sis3801->vme_record,
				sis3801->crate_number,
				sis3801->address_mode,
				sis3801->base_address + MX_SIS3801_CONTROL_REG,
				control_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the prescale factor. */

	if ( mcs->external_channel_advance ) {

		/* Use external channel advance. */

		switch( mcs->external_channel_advance_record->mx_class ) {
		case MXC_PULSE_GENERATOR:
			mx_status = mx_pulse_generator_get_pulse_period(
					mcs->external_channel_advance_record,
					&pulse_period );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( pulse_period < 0.0 ) {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Cannot start MCS '%s' since its external pulse generator '%s' "
	"is currently configured for a negative pulse frequency.",
				mcs->record->name,
				mcs->external_channel_advance_record->name );
			}

			if ( pulse_period < 1.0e-30 ) {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Cannot start MCS '%s' since its external pulse generator '%s' "
	"is currently configured for an essentially infinite pulse frequency.",
				mcs->record->name,
				mcs->external_channel_advance_record->name );
			}

			clock_frequency = mx_divide_safely( 1.0, pulse_period );

			prescale_factor = mx_round( clock_frequency
						* mcs->measurement_time );

			if ( prescale_factor >=
					sis3801->maximum_prescale_factor )
			{
			    maximum_measurement_time = mx_divide_safely(
				(double) sis3801->maximum_prescale_factor,
					clock_frequency );

			    return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested measurement time of %g seconds for MCS '%s' "
		"is larger than the maximum allowed measurement time "
		"per point of %g seconds.",
				mcs->measurement_time,
				sis3801->record->name, 
				maximum_measurement_time );
			}
			break;
		default:
			prescale_factor = mcs->external_prescale;
			break;
		}

#if MXD_SIS3801_DEBUG
		MX_DEBUG(-2,("%s: Using external channel advance", fname));
#endif

		if ( prescale_factor >= sis3801->maximum_prescale_factor ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The external prescale factor %ld for MCS '%s' is too large.  "
		"The largest allowed external prescale factor is %lu.",
			prescale_factor, sis3801->record->name,
			sis3801->maximum_prescale_factor );
		}
	} else {
		/* Use internal clock. */

		prescale_factor = mx_round( MX_SIS3801_10MHZ_INTERNAL_CLOCK
					* mcs->measurement_time );

#if MXD_SIS3801_DEBUG
		MX_DEBUG(-2,("%s: Using internal clock", fname));
#endif

		if ( prescale_factor >= sis3801->maximum_prescale_factor ) {

			maximum_measurement_time = mx_divide_safely(
				(double) sis3801->maximum_prescale_factor,
				MX_SIS3801_10MHZ_INTERNAL_CLOCK );

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested measurement time of %g seconds for MCS '%s' "
		"is larger than the maximum allowed measurement time "
		"per point of %g seconds.",
				mcs->measurement_time,
				sis3801->record->name, 
				maximum_measurement_time );
		}
	}

	prescale_factor -= 1;

#if MXD_SIS3801_DEBUG
	MX_DEBUG(-2,("%s: prescale_factor = %lu", fname, prescale_factor));
#endif

	mx_status = mx_vme_out32( sis3801->vme_record,
				sis3801->crate_number,
				sis3801->address_mode,
			sis3801->base_address + MX_SIS3801_PRESCALE_FACTOR_REG,
				prescale_factor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Enable the next clock logic. */

	mx_status = mx_vme_out32( sis3801->vme_record,
				sis3801->crate_number,
				sis3801->address_mode,
		sis3801->base_address + MX_SIS3801_ENABLE_NEXT_CLOCK_LOGIC,
				1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 1
	/* The SIS3801 treats a prescale factor of 0 as if it were the same
	 * as a prescale factor of 0xffffffff.  Thus, the prescale register
	 * will count down for a very long time.  However, if all we want
	 * to do is cause the 3801 to do a channel advance on each incoming
	 * pulse, then we can get the same effect by merely disabling the
	 * LNE prescaler.
	 */

	if ( prescale_factor == 0 ) {
		control_register = MXF_SIS3801_DISABLE_LNE_PRESCALER;
	} else {
		control_register = MXF_SIS3801_ENABLE_LNE_PRESCALER;
	}
#else
	control_register = MXF_SIS3801_ENABLE_LNE_PRESCALER;
#endif

	/* Enable the LNE prescaler.
	 *
	 * This will start the measurement if we are using the SIS3801's own
	 * internal clock.  Otherwise, it makes the SIS3801 ready to start
	 * counting when the first external next clock comes in.
	 */

	mx_status = mx_vme_out32( sis3801->vme_record,
				sis3801->crate_number,
				sis3801->address_mode,
				sis3801->base_address + MX_SIS3801_CONTROL_REG,
				control_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sis3801->counts_available_in_fifo = TRUE;

	/* Record the time when the measurement should be finished. */

	total_measurement_time = mcs->measurement_time
				* (double) mcs->current_num_measurements;

	/* Give a little extra delay time to try to avoid reading out the
	 * FIFO before all the measurements have been taken.
	 */

	total_measurement_time += 0.02;		/* Delay time in seconds */

	measurement_ticks
		= mx_convert_seconds_to_clock_ticks( total_measurement_time );

	start_time = mx_current_clock_tick();

	finish_time = mx_add_clock_ticks( start_time, measurement_ticks );

#if MXD_SIS3801_DEBUG
	MX_DEBUG(-2,("%s: measurement_time = %g, measurement_ticks = (%lu,%lu)",
		fname, total_measurement_time, measurement_ticks.high_order,
			(unsigned long) measurement_ticks.low_order ));

	MX_DEBUG(-2,("%s: start_time = (%lu,%lu), finish_time = (%lu,%lu)",
		fname, start_time.high_order,
		(unsigned long) start_time.low_order,
		finish_time.high_order,
		(unsigned long) finish_time.low_order));
#endif

	sis3801->finish_time = finish_time;

	MXD_SIS3801_SHOW_STATUS;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3801_stop( MX_MCS *mcs )
{
	static const char fname[] = "mxd_sis3801_stop()";

	MX_SIS3801 *sis3801;
	mx_status_type mx_status;

	mx_status = mxd_sis3801_get_pointers( mcs, &sis3801, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3801_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, mcs->record->name));
#endif

	/* Disable the LNE prescaler. */

	mx_status = mx_vme_out32( sis3801->vme_record,
				sis3801->crate_number,
				sis3801->address_mode,
				sis3801->base_address + MX_SIS3801_CONTROL_REG,
				MXF_SIS3801_DISABLE_LNE_PRESCALER );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Disable next clock logic. */

	mx_status = mx_vme_out32( sis3801->vme_record,
				sis3801->crate_number,
				sis3801->address_mode,
		sis3801->base_address + MX_SIS3801_DISABLE_NEXT_CLOCK_LOGIC,
				1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the measurement finish time to the current time. */

	sis3801->finish_time = mx_current_clock_tick();

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3801_clear( MX_MCS *mcs )
{
	static const char fname[] = "mxd_sis3801_clear()";

	MX_SIS3801 *sis3801;
	unsigned long i, j;
	mx_status_type mx_status;

	mx_status = mxd_sis3801_get_pointers( mcs, &sis3801, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3801_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, mcs->record->name));
#endif

	/* Clear the local data array. */

	for ( i = 0; i < mcs->maximum_num_scalers; i++ ) {
		for ( j = 0; j < mcs->maximum_num_measurements; j++ ) {
			mcs->data_array[i][j] = 0;
		}
	}

	/* Clear the FIFO. */

	mx_status = mx_vme_out32( sis3801->vme_record,
				sis3801->crate_number,
				sis3801->address_mode,
			sis3801->base_address + MX_SIS3801_CLEAR_FIFO_REG,
				0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sis3801->counts_available_in_fifo = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3801_busy( MX_MCS *mcs )
{
	static const char fname[] = "mxd_sis3801_busy()";

	MX_SIS3801 *sis3801;
	MX_CLOCK_TICK current_time;
	mx_status_type mx_status;

	mx_status = mxd_sis3801_get_pointers( mcs, &sis3801, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3801_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, mcs->record->name));
#endif

	/* See if we have reached the expected finish time of the measurement.*/

	current_time = mx_current_clock_tick();

	if ( mx_compare_clock_ticks( current_time, sis3801->finish_time ) >= 0 )
	{
		mcs->busy = FALSE;
	} else {
		mcs->busy = TRUE;
	}

	if ( mcs->busy == FALSE ) {

#if MXD_SIS3801_DEBUG
		MX_DEBUG(-2,
	("%s: busy = %d, current_time = (%lu,%lu), finish_time = (%lu,%lu)",
		fname, mcs->busy, current_time.high_order,
		(unsigned long) current_time.low_order,
		sis3801->finish_time.high_order,
		(unsigned long) sis3801->finish_time.low_order));
#endif

		/* Stop any counting that may still be going on. */

		mx_status = mxd_sis3801_stop( mcs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	MXD_SIS3801_SHOW_STATUS;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3801_read_all( MX_MCS *mcs )
{
	static const char fname[] = "mxd_sis3801_read_all()";

	MX_SIS3801 *sis3801;
	long i, j, num_measurements;
	mx_uint32_type fifo_value, status_register;
	mx_status_type mx_status;

#if MXD_SIS3801_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_sis3801_get_pointers( mcs, &sis3801, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3801_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, mcs->record->name));
#endif

	/* Don't try to read new counts if there aren't any to read. */

	if ( sis3801->counts_available_in_fifo == FALSE ) {
		return MX_SUCCESSFUL_RESULT;
	}

	sis3801->counts_available_in_fifo = FALSE;

	num_measurements = (long) mcs->current_num_measurements;

#if MXD_SIS3801_DEBUG
	MX_DEBUG(-2,("%s: num_measurements = %ld, num_scalers = %ld",
		fname, num_measurements, mcs->current_num_scalers));
#endif

#if MXD_SIS3801_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	for ( j = 0; j < num_measurements; j++ ) {

		for ( i = 0; i < mcs->current_num_scalers; i++ ) {

			/* Check the FIFO status. */

			mx_status = mx_vme_in32( sis3801->vme_record,
				sis3801->crate_number,
				sis3801->address_mode,
				sis3801->base_address + MX_SIS3801_STATUS_REG,
				&status_register );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

#if MXD_SIS3801_DEBUG
			MX_DEBUG(-2,("%s: status_register = %#lx",
				fname, (unsigned long) status_register));
#endif

			/* If the FIFO is full, the SIS3801 will have locked
			 * up and needs to be reset.
			 */

			if ( status_register & MXF_SIS3801_FIFO_FLAG_FULL ) {
				return mx_error( MXE_LIMIT_WAS_EXCEEDED, fname,
"The MCS '%s' FIFO reports it is full at measurement %ld, scaler %ld.  "
"This is an unrecoverable error and will require a software reset of the MCS.",
					mcs->record->name, j, i );
			}

			/* If the FIFO is empty, we have run out of data
			 * before we should have.
			 */

			if ( status_register & MXF_SIS3801_FIFO_FLAG_EMPTY ) {
				mx_warning(
	"%s: The MCS '%s' FIFO ran out of data at measurement %ld, scaler %ld "
	"when there should have been %lu measurements for the %lu scalers.",
		fname, mcs->record->name,
		j, i, mcs->current_num_measurements, mcs->current_num_scalers);

				return MX_SUCCESSFUL_RESULT;
			}

			/* Read the next data value. */

			mx_status = mx_vme_in32( sis3801->vme_record,
				sis3801->crate_number,
				sis3801->address_mode,
				sis3801->base_address + MX_SIS3801_READ_FIFO,
				&fifo_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( j >= 0 ) {
				mcs->data_array[i][j] = (long) fifo_value;
			}
		}
	}

#if MXD_SIS3801_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, "for MCS %s",
			sis3801->record->name );
#endif

#if MXD_SIS3801_DEBUG
	MX_DEBUG(-2,
	("%s: %lu measurements for %lu scalers were successfully read.",
	fname, mcs->current_num_measurements, mcs->current_num_scalers));
#endif

#if 0
	/* Check the FIFO status one more time. */

	mx_status = mx_vme_in32( sis3801->vme_record,
				sis3801->crate_number,
				sis3801->address_mode,
				sis3801->base_address + MX_SIS3801_STATUS_REG,
				&status_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3801_read_scaler( MX_MCS *mcs )
{
	mx_status_type mx_status;

	/* Read the data from the FIFO. */

	mx_status = mxd_sis3801_read_all( mcs );

	/* Since mcs->data_array has the scaler number as its leading
	 * array index, the data is already laid out in mcs->data_array
	 * in such a way that it can be passed back by mx_mcs_read_scaler()
	 * to the application program by merely passing back a pointer
	 * to mcs->data_array[scaler_number][0].
	 *
	 * Thus, we do not need to copy any data, since it is already laid
	 * out in memory the way we want it to be.
	 */

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sis3801_read_measurement( MX_MCS *mcs )
{
	static const char fname[] = "mxd_sis3801_read_measurement()";

	MX_SIS3801 *sis3801;
	unsigned long i, j;
	mx_status_type mx_status;

	mx_status = mxd_sis3801_get_pointers( mcs, &sis3801, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3801_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, mcs->record->name));
#endif

	/* Read the data from the FIFO. */

	mx_status = mxd_sis3801_read_all( mcs );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Copy the data to the measurement_data array. */

	j = mcs->measurement_index;

	for ( i = 0; i < mcs->current_num_scalers; i++ ) {
		mcs->measurement_data[i] = mcs->data_array[i][j];
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3801_get_parameter( MX_MCS *mcs )
{
	static const char fname[] = "mxd_sis3801_get_parameter()";

	MX_SIS3801 *sis3801;
	mx_status_type mx_status;

	mx_status = mxd_sis3801_get_pointers( mcs, &sis3801, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3801_DEBUG
	MX_DEBUG(-2,("%s invoked for MCS '%s', parameter type '%s' (%d)",
		fname, mcs->record->name,
		mx_get_field_label_string( mcs->record, mcs->parameter_type ),
		mcs->parameter_type));
#endif

	switch( mcs->parameter_type ) {
	case MXLV_MCS_MODE:
	case MXLV_MCS_MEASUREMENT_TIME:
	case MXLV_MCS_CURRENT_NUM_MEASUREMENTS:
	case MXLV_MCS_DARK_CURRENT:
		/* Just return the values in the local data structure. */

		break;
	default:
		return mx_mcs_default_get_parameter_handler( mcs );
		break;
	}

#if MXD_SIS3801_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3801_set_parameter( MX_MCS *mcs )
{
	static const char fname[] = "mxd_sis3801_set_parameter()";

	MX_SIS3801 *sis3801;
	mx_status_type mx_status;

	mx_status = mxd_sis3801_get_pointers( mcs, &sis3801, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3801_DEBUG
	MX_DEBUG(-2,("%s invoked for MCS '%s', parameter type '%s' (%d)",
		fname, mcs->record->name,
		mx_get_field_label_string( mcs->record, mcs->parameter_type ),
		mcs->parameter_type));
#endif

	switch( mcs->parameter_type ) {
	case MXLV_MCS_MODE:
	case MXLV_MCS_MEASUREMENT_TIME:
	case MXLV_MCS_CURRENT_NUM_MEASUREMENTS:
	case MXLV_MCS_DARK_CURRENT:
		/* Just store the values in the local data structure. */

		break;
	default:
		return mx_mcs_default_set_parameter_handler( mcs );
		break;
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

