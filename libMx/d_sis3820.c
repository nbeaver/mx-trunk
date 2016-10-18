/*
 * Name:    d_sis3820.c
 *
 * Purpose: MX MCS driver for the Struck SIS3820 multichannel scaler.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2001-2006, 2008, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SIS3820_DEBUG		FALSE

#define MXD_SIS3820_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_driver.h"
#include "mx_clock.h"
#include "mx_vme.h"
#include "mx_pulse_generator.h"
#include "mx_mcs.h"

#include "d_sis3820.h"

#if MXD_SIS3820_DEBUG_TIMING
#include "mx_hrt_debug.h"
#endif

MX_RECORD_FUNCTION_LIST mxd_sis3820_record_function_list = {
	mxd_sis3820_initialize_driver,
	mxd_sis3820_create_record_structures,
	mxd_sis3820_finish_record_initialization,
	NULL,
	NULL,
	mxd_sis3820_open,
	mxd_sis3820_close,
	NULL,
	mxd_sis3820_open
};

MX_MCS_FUNCTION_LIST mxd_sis3820_mcs_function_list = {
	mxd_sis3820_start,
	mxd_sis3820_stop,
	mxd_sis3820_clear,
	mxd_sis3820_busy,
	mxd_sis3820_read_all,
	mxd_sis3820_read_scaler,
	mxd_sis3820_read_measurement,
	NULL,
	NULL,
	mxd_sis3820_get_parameter,
	mxd_sis3820_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_sis3820_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCS_STANDARD_FIELDS,
	MXD_SIS3820_STANDARD_FIELDS
};

long mxd_sis3820_num_record_fields
		= sizeof( mxd_sis3820_record_field_defaults )
			/ sizeof( mxd_sis3820_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_sis3820_rfield_def_ptr
			= &mxd_sis3820_record_field_defaults[0];

#if MXD_SIS3820_DEBUG
#  define MXD_SIS3820_SHOW_STATUS \
	do {								\
		uint32_t my_status_register;			\
									\
		mx_vme_in32( sis3820->vme_record,			\
			sis3820->crate_number,				\
			sis3820->address_mode,				\
			sis3820->base_address + MX_SIS3820_STATUS_REG,	\
			&my_status_register );				\
									\
		MX_DEBUG(-2,("%s: SHOW_STATUS - status_register = %#lx",\
			fname, (unsigned long) my_status_register));	\
	} while(0)
#else
#  define MXD_SIS3820_SHOW_STATUS
#endif

/* --- */

static mx_status_type
mxd_sis3820_get_pointers( MX_MCS *mcs,
			MX_SIS3820 **sis3820,
			const char *calling_fname )
{
	static const char fname[] = "mxd_sis3820_get_pointers()";

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCS pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( sis3820 == (MX_SIS3820 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SIS3820 pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mcs->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for the MX_MCS structure passed by '%s' is NULL.",
			calling_fname );
	}

	*sis3820 = (MX_SIS3820 *) (mcs->record->record_type_struct);

	if ( *sis3820 == (MX_SIS3820 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SIS3820 pointer for record '%s' is NULL.",
			mcs->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_sis3820_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_scalers_varargs_cookie;
	long maximum_num_measurements_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mcs_initialize_driver( driver,
				&maximum_num_scalers_varargs_cookie,
				&maximum_num_measurements_varargs_cookie );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sis3820_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_sis3820_create_record_structures()";

	MX_MCS *mcs;
	MX_SIS3820 *sis3820 = NULL;

	mcs = (MX_MCS *) malloc( sizeof(MX_MCS) );

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_MCS structure." );
	}

	sis3820 = (MX_SIS3820 *) malloc( sizeof(MX_SIS3820) );

	if ( sis3820 == (MX_SIS3820 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_SIS3820 structure." );
	}

	record->record_class_struct = mcs;
	record->record_type_struct = sis3820;

	record->class_specific_function_list = &mxd_sis3820_mcs_function_list;

	mcs->record = record;
	sis3820->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3820_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_mcs_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sis3820_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_sis3820_open()";

	MX_MCS *mcs;
	MX_SIS3820 *sis3820 = NULL;
#if 0
	uint32_t module_id_register, control_register, status_register;
#endif
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mcs = (MX_MCS *) record->record_class_struct;

	mx_status = mxd_sis3820_get_pointers( mcs, &sis3820, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Parse the address mode string. */

#if MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s: sis3820->address_mode_name = '%s'",
			fname, sis3820->address_mode_name));
#endif

	mx_status = mx_vme_parse_address_mode( sis3820->address_mode_name,
						&(sis3820->address_mode) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s: sis3820->address_mode = %lu",
			fname, sis3820->address_mode));
#endif

#if 0 /* old 3801 code */

	/* Reset the SIS3820. */

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
				sis3820->base_address + MX_SIS3820_RESET_REG,
				1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the FIFO size. */

	mx_status = mx_vme_in32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
				sis3820->base_address + MX_SIS3820_STATUS_REG,
				&status_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( status_register ) {
	case 0x300:
#if MXD_SIS3820_DEBUG
		MX_DEBUG(-2,("%s: 64K FIFO installed.", fname));
#endif
		sis3820->fifo_size_in_kwords = 64;
		break;

	case 0x100:
#if MXD_SIS3820_DEBUG
		MX_DEBUG(-2,("%s: 256K FIFO installed.", fname));
#endif
		sis3820->fifo_size_in_kwords = 256;
		break;
	default:
		mx_warning( "Power up status register value %#lx is not "
		"either 0x300 (for 64K FIFO) or 0x100 (for 256K FIFO).",
			(unsigned long) status_register );

		sis3820->fifo_size_in_kwords = -1;
		break;
	}

	/* Read the module identification register. */

	mx_status = mx_vme_in32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
		sis3820->base_address + MX_SIS3820_MODULE_ID_IRQ_CONTROL_REG,
				&module_id_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sis3820->module_id = ( module_id_register >> 16 ) & 0xffff;
	sis3820->firmware_version = ( module_id_register >> 12 ) & 0xf;

#if MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s: module id = %#lx, firmware version = %lu",
		fname, sis3820->module_id, sis3820->firmware_version));
#endif

	/* Compute the maximum allowed prescale factor for this module. */

	/* Other firmware versions may well support 28-bit prescale factors,
	 * but Struck does not give us a way to directly test for their
	 * presence.  All one can do is guess based on the firmware version
	 * number.  If you find that you have a version of the firmware that
	 * supports 28-bit ( or larger? ) prescale factors, then add it as
	 * a case in the switch() statement below.
	 */

	switch( sis3820->firmware_version ) {
	case 0x9:
	case 0xA:
		sis3820->maximum_prescale_factor = 268435456;   /* 2^28 */
		break;
	default:
		sis3820->maximum_prescale_factor = 16777216;	/* 2^24 */
		break;
	}

	if ( sis3820->sis3820_flags & MXF_SIS3820_USE_REFERENCE_PULSER ) {

		/* Enable reference pulses. */

		mx_status = mx_vme_out32( sis3820->vme_record,
					sis3820->crate_number,
					sis3820->address_mode,
	sis3820->base_address + MX_SIS3820_ENABLE_REFERENCE_PULSE_CHANNEL_1,
					1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Set the control input mode. */

	control_register =
		(uint32_t) ( ( sis3820->control_input_mode & 0x3 ) << 2 );

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
				sis3820->base_address + MX_SIS3820_CONTROL_REG,
				control_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Enable counting. */

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
				sis3820->base_address + MX_SIS3820_CONTROL_REG,
			MXF_SIS3820_CLEAR_SOFTWARE_DISABLE_COUNTING_BIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the measurement finish time to the current time. */

	sis3820->finish_time = mx_current_clock_tick();

	mcs->measurement_time = 0.0;

	/* We assume there are no counts remaining to be read in the FIFO. */

	sis3820->counts_available_in_fifo = FALSE;

	/* Turn on the user LED to show that we are initialized. */

#if MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s: Turning on user LED.", fname));
#endif

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
				sis3820->base_address + MX_SIS3820_CONTROL_REG,
				0x1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#endif /* old 3801 code */

#if MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3820_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_sis3820_close()";

	MX_MCS *mcs;
	MX_SIS3820 *sis3820 = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mcs = (MX_MCS *) record->record_class_struct;

	mx_status = mxd_sis3820_get_pointers( mcs, &sis3820, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off the user LED to show that we have shut down. */

#if MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s: Turning off user LED.", fname));
#endif

#if 0
	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
				sis3820->base_address + MX_SIS3820_CONTROL_REG,
				0x100 );
#endif

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_sis3820_start( MX_MCS *mcs )
{
	static const char fname[] = "mxd_sis3820_start()";

	MX_SIS3820 *sis3820 = NULL;
#if 0
	MX_CLOCK_TICK start_time, finish_time, measurement_ticks;
	double total_measurement_time, maximum_measurement_time;
	double pulse_period, clock_frequency;
	uint32_t prescale_factor, control_register;
	mx_bool_type busy;
#endif
	mx_status_type mx_status;

	mx_status = mxd_sis3820_get_pointers( mcs, &sis3820, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, mcs->record->name));
#endif

	/* Stop the MCS. */

	mx_status = mxd_sis3820_stop( mcs );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Clear the MCS. */

	mx_status = mxd_sis3820_clear( mcs );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0 /* old 3801 code */

	/* If external channel advance is disabled, enable the internal
	 * 10 MHz clock so that it can do the channel advance.
	 */

	if ( mcs->external_channel_advance_record == NULL ) {
		mcs->external_channel_advance = 0;
	}

	control_register = 0;

	if ( mcs->external_channel_advance ) {
		control_register |= MXF_SIS3820_DISABLE_10MHZ_TO_LNE_PRESCALER;
		control_register |= MXF_SIS3820_ENABLE_EXTERNAL_NEXT;
	} else {
		control_register |= MXF_SIS3820_ENABLE_10MHZ_TO_LNE_PRESCALER;
		control_register |= MXF_SIS3820_DISABLE_EXTERNAL_NEXT;
	}

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
				sis3820->base_address + MX_SIS3820_CONTROL_REG,
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
	"is currently configured for a negative pulse period.",
				mcs->record->name,
				mcs->external_channel_advance_record->name );
			}

			if ( pulse_period < 1.0e-30 ) {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Cannot start MCS '%s' since its external pulse generator '%s' "
	"is currently configured for an essentially zero pulse period.",
				mcs->record->name,
				mcs->external_channel_advance_record->name );
			}

			clock_frequency = mx_divide_safely( 1.0, pulse_period );

			prescale_factor = (uint32_t)
			    mx_round( clock_frequency * mcs->measurement_time );
#if MXD_SIS3820_DEBUG
			MX_DEBUG(-2,
  ("%s: Using pulse generator, pulse_period = %g, clock = %g, prescale = %lu",
   				fname, pulse_period, clock_frequency,
				prescale_factor));
#endif

			if ( prescale_factor >=
					sis3820->maximum_prescale_factor )
			{
			    maximum_measurement_time = mx_divide_safely(
				(double) sis3820->maximum_prescale_factor,
					clock_frequency );

			    return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested measurement time of %g seconds for MCS '%s' "
		"is larger than the maximum allowed measurement time "
		"per point of %g seconds.",
				mcs->measurement_time,
				sis3820->record->name, 
				maximum_measurement_time );
			}

			/* Is the pulse generator running? */

			mx_status = mx_pulse_generator_is_busy( 
					mcs->external_channel_advance_record,
					&busy );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( busy == FALSE ) {
				/* If not, then start the pulse generator. */

				mx_status = mx_pulse_generator_start(
					mcs->external_channel_advance_record );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
			break;
		default:
			prescale_factor = (uint32_t) mcs->external_prescale;
			break;
		}

#if MXD_SIS3820_DEBUG
		MX_DEBUG(-2,
		("%s: Using external channel advance, prescale_factor = %lu",
		 	fname, prescale_factor));
#endif

		if ( prescale_factor >= sis3820->maximum_prescale_factor ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The external prescale factor %ld for MCS '%s' is too large.  "
		"The largest allowed external prescale factor is %lu.",
			(long) prescale_factor, sis3820->record->name,
			sis3820->maximum_prescale_factor );
		}
	} else {
		/* Use internal clock. */

		prescale_factor = (uint32_t)
				mx_round( MX_SIS3820_10MHZ_INTERNAL_CLOCK
					* mcs->measurement_time );

#if MXD_SIS3820_DEBUG
		MX_DEBUG(-2,("%s: Using internal clock", fname));
#endif

		if ( prescale_factor >= sis3820->maximum_prescale_factor ) {

			maximum_measurement_time = mx_divide_safely(
				(double) sis3820->maximum_prescale_factor,
				MX_SIS3820_10MHZ_INTERNAL_CLOCK );

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested measurement time of %g seconds for MCS '%s' "
		"is larger than the maximum allowed measurement time "
		"per point of %g seconds.",
				mcs->measurement_time,
				sis3820->record->name, 
				maximum_measurement_time );
		}
	}

	prescale_factor -= 1;

#if MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s: prescale_factor = %lu", fname, prescale_factor));
#endif

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
			sis3820->base_address + MX_SIS3820_PRESCALE_FACTOR_REG,
				prescale_factor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Enable the next clock logic. */

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
		sis3820->base_address + MX_SIS3820_ENABLE_NEXT_CLOCK_LOGIC,
				1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 1
	/* The SIS3820 treats a prescale factor of 0 as if it were the same
	 * as a prescale factor of 0xffffffff.  Thus, the prescale register
	 * will count down for a very long time.  However, if all we want
	 * to do is cause the 3820 to do a channel advance on each incoming
	 * pulse, then we can get the same effect by merely disabling the
	 * LNE prescaler.
	 */

	if ( prescale_factor == 0 ) {
		control_register = MXF_SIS3820_DISABLE_LNE_PRESCALER;
	} else {
		control_register = MXF_SIS3820_ENABLE_LNE_PRESCALER;
	}
#else
	control_register = MXF_SIS3820_ENABLE_LNE_PRESCALER;
#endif

	/* Enable the LNE prescaler.
	 *
	 * This will start the measurement if we are using the SIS3820's own
	 * internal clock.  Otherwise, it makes the SIS3820 ready to start
	 * counting when the first external next clock comes in.
	 */

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
				sis3820->base_address + MX_SIS3820_CONTROL_REG,
				control_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sis3820->counts_available_in_fifo = TRUE;

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

#if MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s: measurement_time = %g, measurement_ticks = (%lu,%lu)",
		fname, total_measurement_time, measurement_ticks.high_order,
			(unsigned long) measurement_ticks.low_order ));

	MX_DEBUG(-2,("%s: start_time = (%lu,%lu), finish_time = (%lu,%lu)",
		fname, start_time.high_order,
		(unsigned long) start_time.low_order,
		finish_time.high_order,
		(unsigned long) finish_time.low_order));
#endif

	sis3820->finish_time = finish_time;

	MXD_SIS3820_SHOW_STATUS;

#endif /* old 3801 code */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3820_stop( MX_MCS *mcs )
{
	static const char fname[] = "mxd_sis3820_stop()";

	MX_SIS3820 *sis3820 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sis3820_get_pointers( mcs, &sis3820, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, mcs->record->name));
#endif

	sis3820->finish_time = mx_current_clock_tick();

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3820_clear( MX_MCS *mcs )
{
	static const char fname[] = "mxd_sis3820_clear()";

	MX_SIS3820 *sis3820 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sis3820_get_pointers( mcs, &sis3820, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, mcs->record->name));
#endif

	/*FIXME: Place VME code here. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3820_busy( MX_MCS *mcs )
{
	static const char fname[] = "mxd_sis3820_busy()";

	MX_SIS3820 *sis3820 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sis3820_get_pointers( mcs, &sis3820, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, mcs->record->name));
#endif

	/*FIXME: Place VME code here. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3820_read_all( MX_MCS *mcs )
{
	static const char fname[] = "mxd_sis3820_read_all()";

	MX_SIS3820 *sis3820 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sis3820_get_pointers( mcs, &sis3820, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, mcs->record->name));
#endif

	/*FIXME: Place VME code here. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3820_read_scaler( MX_MCS *mcs )
{
	static const char fname[] = "mxd_sis3820_read_scaler()";

	MX_SIS3820 *sis3820 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sis3820_get_pointers( mcs, &sis3820, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, mcs->record->name));
#endif

	/*FIXME: Place VME code here. */

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sis3820_read_measurement( MX_MCS *mcs )
{
	static const char fname[] = "mxd_sis3820_read_measurement()";

	MX_SIS3820 *sis3820 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sis3820_get_pointers( mcs, &sis3820, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, mcs->record->name));
#endif

	/*FIXME: Place VME code here. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3820_get_parameter( MX_MCS *mcs )
{
	static const char fname[] = "mxd_sis3820_get_parameter()";

	MX_SIS3820 *sis3820 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sis3820_get_pointers( mcs, &sis3820, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3820_DEBUG
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
	}

#if MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3820_set_parameter( MX_MCS *mcs )
{
	static const char fname[] = "mxd_sis3820_set_parameter()";

	MX_SIS3820 *sis3820 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sis3820_get_pointers( mcs, &sis3820, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3820_DEBUG
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
	}

#if MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

