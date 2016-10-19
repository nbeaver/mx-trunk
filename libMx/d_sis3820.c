/*
 * Name:    d_sis3820.c
 *
 * Purpose: MX MCS driver for the Struck SIS3820 multichannel scaler.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SIS3820_DEBUG		TRUE

#define MXD_SIS3820_DEBUG_OPEN		TRUE

#define MXD_SIS3820_DEBUG_START		TRUE

#define MXD_SIS3820_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_driver.h"
#include "mx_clock.h"
#include "mx_callback.h"
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

/*=========================================================================*/

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

/*=========================================================================*/

static mx_status_type
mxd_sis3820_fifo_callback_function( MX_CALLBACK_MESSAGE *callback_message )
{
	static const char fname[] = "mxd_sis3820_fifo_callback_function()";

	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	/* Restart the virtual timer to arrange for the next callback. */

	mx_status = mx_virtual_timer_start(
			callback_message->u.function.oneshot_timer,
			callback_message->u.function.callback_interval );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

/*=========================================================================*/

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
		"Cannot allocate memory for MX_MCS structure." );
	}

	sis3820 = (MX_SIS3820 *) malloc( sizeof(MX_SIS3820) );

	if ( sis3820 == (MX_SIS3820 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_SIS3820 structure." );
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
	MX_LIST_HEAD *list_head = NULL;
	MX_VIRTUAL_TIMER *oneshot_timer = NULL;
	uint32_t module_id_register = 0;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mcs = (MX_MCS *) record->record_class_struct;

	mx_status = mxd_sis3820_get_pointers( mcs, &sis3820, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if callbacks are enabled. */

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for record '%s' is NULL.",
			record->name );
	}

	if ( list_head->callback_pipe == NULL ) {
		sis3820->use_callback = FALSE;

		mx_warning("MX callbacks are not enabled for this process.  "
		"In this configuration, the FIFO callback for driver '%s' "
		"of record '%s' will never be called, so data from the MCS "
		"will never be read out.",
			mx_get_driver_name(record), record->name );
	} else {
		sis3820->use_callback = TRUE;
	}

	/* Parse the address mode string. */

#if MXD_SIS3820_DEBUG_OPEN
	MX_DEBUG(-2,("%s: sis3820->address_mode_name = '%s'",
			fname, sis3820->address_mode_name));
#endif

	mx_status = mx_vme_parse_address_mode( sis3820->address_mode_name,
						&(sis3820->address_mode) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3820_DEBUG_OPEN
	MX_DEBUG(-2,("%s: sis3820->address_mode = %lu",
			fname, sis3820->address_mode));
#endif

	/* Reset the SIS3820. */

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
			sis3820->base_address + MX_SIS3820_KEY_RESET_REG,
				1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the module identification register. */

	mx_status = mx_vme_in32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
				sis3820->base_address + MX_SIS3820_MODID_REG,
				&module_id_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sis3820->module_id = ( module_id_register >> 16 ) & 0xffff;
	sis3820->firmware_version = module_id_register & 0xffff;

#if MXD_SIS3820_DEBUG_OPEN
	MX_DEBUG(-2,("%s: module id = %#lx, firmware version = %#lx",
		fname, sis3820->module_id, sis3820->firmware_version));
#endif

	/* Setup default acquisition mode for MCS operation. */

	if ( sis3820->sis3820_flags & MXF_SIS3820_USE_REFERENCE_PULSER ) {
		mcs->external_channel_advance = FALSE;
	} else {
		mcs->external_channel_advance = TRUE;
	}

	/*-----------------------------------------------------------------*/

	/* Create a callback message structure that will be used to handle
	 * reading out counts from the SIS3820 FIFO.
	 *
	 * The same callback message structure will be used over and over
	 * again to send messages to the server's main event loop.  It may
	 * not be safe to call malloc() in the context (such as a signal
	 * handler) where the message is put into the main loop's event pipe,
	 * so we do that here where we know that it is safe.
	 */

	sis3820->callback_message = malloc( sizeof(MX_CALLBACK_MESSAGE) );

	if ( sis3820->callback_message == (MX_CALLBACK_MESSAGE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_CALLBACK_MESSAGE "
		"structure for MCS '%s'.", record->name );
	}

	sis3820->callback_message->callback_type = MXCBT_FUNCTION;
	sis3820->callback_message->list_head = list_head;

	sis3820->callback_message->u.function.callback_function =
					mxd_sis3820_fifo_callback_function;

	sis3820->callback_message->u.function.callback_args = sis3820;

	/* Specify the callback interval in seconds. */

	sis3820->callback_message->u.function.callback_interval = 0.1;

	/* Create a one-shot interval timer that will arrange for the
	 * MCS's callback function to be called later.
	 *
	 * We use a one-shot timer here to avoid filling the callback pipe.
	 */

	mx_status = mx_virtual_timer_create(
				&oneshot_timer,
				list_head->master_timer,
				MXIT_ONE_SHOT_TIMER,
				mx_callback_standard_vtimer_handler,
				sis3820->callback_message );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sis3820->callback_message->u.function.oneshot_timer = oneshot_timer;

	/* The callback timer will not be started until mxd_sis3820_open()
	 * is called.
	 */

	/*-----------------------------------------------------------------*/

	/* Turn on the user LED to show that we are initialized. */

#if MXD_SIS3820_DEBUG_OPEN
	MX_DEBUG(-2,("%s: Turning on user LED.", fname));
#endif

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
			sis3820->base_address + MX_SIS3820_CONTROL_STATUS_REG,
				MXF_SIS3820_CTRL_USER_LED_ON );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3820_DEBUG_OPEN
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

	/* Stop the MCS. */

	mx_status = mxd_sis3820_stop( mcs );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off the user LED to show that we have shut down. */

#if MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s: Turning off user LED.", fname));
#endif

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
			sis3820->base_address + MX_SIS3820_CONTROL_STATUS_REG,
				MXF_SIS3820_CTRL_USER_LED_OFF );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_sis3820_start( MX_MCS *mcs )
{
	static const char fname[] = "mxd_sis3820_start()";

	MX_SIS3820 *sis3820 = NULL;
	uint32_t reference_pulser = 0;
	uint32_t acq_op_mode = 0;
	uint32_t lne_prescale_factor = 0;
	mx_status_type mx_status;

	mx_status = mxd_sis3820_get_pointers( mcs, &sis3820, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3820_DEBUG_START
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

	/*------------ Configure the MCS for this sequence. ------------*/

	/* Configure the number of measurements. */

#if MXD_SIS3820_DEBUG_START
	MX_DEBUG(-2,("%s: mcs->current_num_measurements = %lu",
		fname, mcs->current_num_measurements));
#endif

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
		sis3820->base_address + MX_SIS3820_ACQUISITION_PRESET_REG,
				mcs->current_num_measurements );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If we are using the internal 10 MHz pulse generator, then we
	 * must calculate the LNE prescaler factor to get the time interval
	 * in seconds that we want.  If we are using an external channel
	 * advance signal, then we currently ignore the LNE prescaler factor.
	 */

	if ( mcs->external_channel_advance ) {
		lne_prescale_factor = 0;
	} else {
		lne_prescale_factor =
	    mx_round( MX_SIS3820_10MHZ_INTERNAL_CLOCK * mcs->measurement_time );
	}

#if MXD_SIS3820_DEBUG_START
	MX_DEBUG(-2,("%s: lne_prescale_factor = %lu",
		fname, (unsigned long) lne_prescale_factor));
#endif

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
			sis3820->base_address + MX_SIS3820_LNE_PRESCALE_REG,
				lne_prescale_factor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure channel advance. */

#if MXD_SIS3820_DEBUG_START
	MX_DEBUG(-2,("%s: mcs->external_channel_advance = %d",
		fname, (int) mcs->external_channel_advance));
#endif

	if ( mcs->external_channel_advance ) {

		/* External channel advance mode. */

		/* FIFO mode, MCS mode, Input mode 1, LNE as enable. */

		acq_op_mode  = MXF_SIS3820_FIFO_MODE;
		acq_op_mode |= MXF_SIS3820_OP_MODE_MULTI_CHANNEL_SCALER;
		acq_op_mode |= MXF_SIS3820_CONTROL_INPUT_MODE1;
		acq_op_mode |= MXF_SIS3820_ARM_ENABLE_CONTROL_SIGNAL;

		/* Use front panel control. */

		acq_op_mode |= MXF_SIS3820_LNE_SOURCE_CONTROL_SIGNAL;
	} else {
		/* Internal channel advance mode. */

		acq_op_mode  = MXF_SIS3820_SDRAM_MODE;
		acq_op_mode |= MXF_SIS3820_OP_MODE_MULTI_CHANNEL_SCALER;
		acq_op_mode |= MXF_SIS3820_CONTROL_INPUT_MODE0;
		acq_op_mode |= MXF_SIS3820_ARM_ENABLE_CONTROL_SIGNAL;

		/* Use internal pulse generator at 10 MHz. */

		acq_op_mode |= MXF_SIS3820_LNE_SOURCE_INTERNAL_10MHZ;
	}

#if MXD_SIS3820_DEBUG_START
	MX_DEBUG(-2,("%s: acq_op_mode = %#lx",
		fname, (unsigned long) acq_op_mode));
#endif

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
			sis3820->base_address + MX_SIS3820_OPERATION_MODE_REG,
				acq_op_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Enable the internal pulse generator if necessary. */

	if ( mcs->external_channel_advance ) {
		reference_pulser = MXF_SIS3820_CTRL_REFERENCE_CH1_DISABLE;
	} else {
		reference_pulser = MXF_SIS3820_CTRL_REFERENCE_CH1_ENABLE;
	}

#if MXD_SIS3820_DEBUG_START
	MX_DEBUG(-2,("%s: reference_pulser = %#lx",
		fname, (unsigned long) reference_pulser));
#endif

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
			sis3820->base_address + MX_SIS3820_CONTROL_STATUS_REG,
				reference_pulser );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* FIXME: Enable the background processing callback. */

	/* Arm the MCS. */

#if MXD_SIS3820_DEBUG_START
	MX_DEBUG(-2,("%s: Calling KEY_OPERATION_ARM", fname));
#endif

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
		sis3820->base_address + MX_SIS3820_KEY_OPERATION_ARM_REG,
				0x1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
		sis3820->base_address + MX_SIS3820_KEY_COUNTER_CLEAR_REG,
				0x1 );

	/* FIXME: Perhaps we should also memset() the MX_MCS data structure. */

	return mx_status;
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
	MX_DEBUG(-2,("%s invoked for MCS '%s', parameter type '%s' (%ld)",
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
	MX_DEBUG(-2,("%s invoked for MCS '%s', parameter type '%s' (%ld)",
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

