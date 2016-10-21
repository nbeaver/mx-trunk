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

#define MXD_SIS3820_DEBUG_FIFO		FALSE

#define MXD_SIS3820_DEBUG_OPEN		TRUE

#define MXD_SIS3820_DEBUG_START		TRUE

#define MXD_SIS3820_DEBUG_STOP		TRUE

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
	NULL,
	NULL,
	NULL,
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

	MX_RECORD *record = NULL;
	MX_MCS *mcs = NULL;
	MX_SIS3820 *sis3820 = NULL;

	uint32_t interrupt_status = 0;
	uint32_t acquisition_count = 0;
	uint32_t fifo_wordcount = 0;
	long num_new_measurements = 0;
	long i_fifo, i_scaler, i_measurement = 0;
	long **dest_array = NULL;
	uint32_t *src_array = NULL;

	mx_bool_type lne_clock_shadow      = FALSE;
	mx_bool_type fifo_threshold        = FALSE;
	mx_bool_type acquisition_completed = FALSE;
	mx_bool_type overflow              = FALSE;
	mx_bool_type fifo_almost_full      = FALSE;

	mx_status_type mx_status;

#if MXD_SIS3820_DEBUG_FIFO
	MX_DEBUG(-2,("***** %s invoked *****", fname));
#endif

	sis3820 = (MX_SIS3820 *) callback_message->u.function.callback_args;

	if ( sis3820 == (MX_SIS3820 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SIS3820 callback args pointer was NULL." );
	}

	record = sis3820->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_SIS3820 structure %p is NULL.",
			sis3820 );
	}

	mcs = (MX_MCS *) record->record_class_struct;

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCS pointer for record '%s' is NULL.", record->name );
	}

	/* Get some MCS status information from the Interrupt Status Register.*/

	mx_status = mx_vme_in32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
			sis3820->base_address + MX_SIS3820_IRQ_CONTROL_REG,
				&interrupt_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( interrupt_status & 0x10000 ) {
		lne_clock_shadow = TRUE;
	}
	if ( interrupt_status & 0x20000 ) {
		fifo_threshold = TRUE;
	}
	if ( interrupt_status & 0x40000 ) {
		acquisition_completed = TRUE;
	}
	if ( interrupt_status & 0x80000 ) {
		overflow = TRUE;
	}
	if ( interrupt_status & 0x100000 ) {
		fifo_almost_full = TRUE;
	}

#if MXD_SIS3820_DEBUG_FIFO
	MX_DEBUG(-2,("FIFO: interrupt_status = %#lx",
		(unsigned long) interrupt_status ));
	MX_DEBUG(-2,("FIFO: lne_clock_shadow = %d, fifo_threshold = %d",
		(int) lne_clock_shadow, (int) fifo_threshold ));
	MX_DEBUG(-2,("FIFO: acquisition_completed = %d, overflow = %d",
		(int) acquisition_completed, (int) overflow ));
	MX_DEBUG(-2,("FIFO: fifo_almost_full = %d",
		(int) fifo_almost_full ));
#endif

	/* Find out how many acquisitions have occurred since the last
	 * time we checked.
	 */

	mx_status = mx_vme_in32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
		sis3820->base_address + MX_SIS3820_ACQUISITION_COUNT_REG,
				&acquisition_count );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_vme_in32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
		sis3820->base_address + MX_SIS3820_FIFO_WORDCOUNTER_REG,
				&fifo_wordcount );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_new_measurements = acquisition_count - mcs->measurement_number - 1;

	sis3820->unread_measurements_in_fifo = num_new_measurements;

#if MXD_SIS3820_DEBUG_FIFO
	MX_DEBUG(-2,
	("FIFO: measurement_number = %ld, acquisition_count = %lu, "
	"num_new_measurements = %ld, fifo_wordcount = %lu",
		mcs->measurement_number,
		(unsigned long) acquisition_count,
		num_new_measurements,
		(unsigned long) fifo_wordcount));
#endif

	/* Should we consider the MCS to be 'busy'.  We mark it as busy
	 * immediately after a start, no matter what 'acquisition_completed'
	 * value that we found.
	 */

	if ( sis3820->new_start ) {
		sis3820->new_start = FALSE;

		mcs->busy = TRUE;
	} else
	if ( sis3820->fifo_stop ) {
		sis3820->fifo_stop = FALSE;

		mcs->busy = FALSE;
	} else
	if ( acquisition_count > (mcs->measurement_number + 1) ) {
		mcs->busy = TRUE;
	} else 
	if ( acquisition_completed ) {
		mcs->busy = FALSE;
	} else {
		mcs->busy = TRUE;
	}

#if MXD_SIS3820_DEBUG_FIFO
	MX_DEBUG(-2,("FIFO: mcs->busy = %d", (int) mcs->busy));
#endif

#if MXD_SIS3820_DEBUG_FIFO
	MX_DEBUG(-2,("FIFO: MCS readout."));
#endif
	/* Readout all new measurements from the MCS.
	 *
	 * FIXME: The data are read out one measurement at a time, which
	 *        is very slow.
	 */

	dest_array = mcs->data_array;
	src_array = sis3820->measurement_buffer;

	for ( i_fifo = 0; i_fifo < num_new_measurements; i_fifo++ ) {

		mx_status = mx_vme_multi_in32(
				sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
				sizeof( uint32_t ),
			sis3820->base_address + MX_SIS3820_FIFO_BASE_REG,
				mcs->maximum_num_scalers,
				src_array );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		i_measurement = i_fifo + mcs->measurement_number + 1;

		/* Take the modulo of the measurement number so that it
		 * wraps back to the beginning of each scaler's measurement
		 * buffer when mcs->maximum_num_measurements is exceeded.
		 */

		i_measurement = i_measurement % (mcs->maximum_num_measurements);

		/* Loop over the scalers. */

		for ( i_scaler = 0;
			i_scaler < mcs->maximum_num_scalers;
			i_scaler++ )
		{
			dest_array[i_scaler][i_measurement]
					= src_array[i_scaler];
		}
#if MXD_SIS3820_DEBUG_FIFO
		MX_DEBUG(-2,("FIFO: ((%ld)) data = "
			"%ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, "
			"%ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, "
			"%ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, "
			"%ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld",
				i_measurement,
			dest_array[0][i_measurement],
			dest_array[1][i_measurement],
			dest_array[2][i_measurement],
			dest_array[3][i_measurement],
			dest_array[4][i_measurement],
			dest_array[5][i_measurement],
			dest_array[6][i_measurement],
			dest_array[7][i_measurement],
			dest_array[8][i_measurement],
			dest_array[9][i_measurement],
			dest_array[10][i_measurement],
			dest_array[11][i_measurement],
			dest_array[12][i_measurement],
			dest_array[13][i_measurement],
			dest_array[14][i_measurement],
			dest_array[15][i_measurement],
			dest_array[16][i_measurement],
			dest_array[17][i_measurement],
			dest_array[18][i_measurement],
			dest_array[19][i_measurement],
			dest_array[20][i_measurement],
			dest_array[21][i_measurement],
			dest_array[22][i_measurement],
			dest_array[23][i_measurement],
			dest_array[24][i_measurement],
			dest_array[25][i_measurement],
			dest_array[26][i_measurement],
			dest_array[27][i_measurement],
			dest_array[28][i_measurement],
			dest_array[29][i_measurement],
			dest_array[30][i_measurement],
			dest_array[31][i_measurement] ));
#endif
	}

	/* Update the measurement counter */

	mcs->measurement_number = acquisition_count - 1;

	/* If we are still busy, start the virtual timer to arrange for
	 * the next callback.
	 */

	if ( mcs->busy ) {

#if MXD_SIS3820_DEBUG_FIFO
		MX_DEBUG(-2,("FIFO: restarting FIFO timer."));
#endif
		mx_status = mx_virtual_timer_start(
			callback_message->u.function.oneshot_timer,
			callback_message->u.function.callback_interval );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if MXD_SIS3820_DEBUG_FIFO
	MX_DEBUG(-2,("***** %s complete *****", fname));
#endif

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
	uint32_t module_id_register;
	uint32_t interrupt_disable_mask;
	uint32_t sdram_page_register;
	uint32_t memory_size_bit;
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

	mcs->readout_preference = MXF_MCS_PREFER_READ_MEASUREMENT;

	sis3820->new_start = FALSE;

	/* Allocate a buffer for measurements that are being read
	 * out of the FIFO.
	 */

	sis3820->measurement_buffer = (uint32_t *)
			malloc( mcs->maximum_num_scalers * sizeof(uint32_t) );

	if ( sis3820->measurement_buffer == (uint32_t *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element array "
		"for the single measurement buffer of MCS '%s'.",
			mcs->maximum_num_scalers, record->name );
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

	mcs->busy = FALSE;
	sis3820->fifo_stop = FALSE;

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
	/* How big is the FIFO (in megabytes)? */

	mx_status = mx_vme_in32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
			sis3820->base_address + MX_SIS3820_SDRAM_PAGE_REG,
				&sdram_page_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	memory_size_bit = ( sdram_page_register >> 8 ) & 0x1;

#if MXD_SIS3820_DEBUG_OPEN
	MX_DEBUG(-2,("%s: sdram_page_register =  %#lx, memory_size_bit = %#lx",
		fname, (unsigned long) sdram_page_register,
		(unsigned long) memory_size_bit ));
#endif

	if ( memory_size_bit == 0 ) {
		sis3820->fifo_size = 64;	/* in megabytes */
	} else {
		sis3820->fifo_size = 512;
	}

	sis3820->fifo_size_in_measurements = mx_round( mx_divide_safely(
			1048576 * sis3820->fifo_size,
		sizeof(uint32_t) * mcs->maximum_num_scalers ) );
		
#if MXD_SIS3820_DEBUG_OPEN
	MX_DEBUG(-2,
	("%s: fifo_size = %lu megabytes, fifo_size_in_measurements = %lu",
		fname, sis3820->fifo_size, sis3820->fifo_size_in_measurements));
#endif

	/* Setup default acquisition mode for MCS operation. */

	if ( sis3820->sis3820_flags & MXF_SIS3820_USE_REFERENCE_PULSER ) {
		mcs->external_channel_advance = FALSE;
	} else {
		mcs->external_channel_advance = TRUE;
	}

	/* Disable interrupts 0 to 4, since we want to read the status
	 * conditions from the Interrupt Control/Status register instead.
	 */

	sis3820->disabled_interrupts = 0x1F;

	interrupt_disable_mask = (sis3820->disabled_interrupts) << 8;

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
			sis3820->base_address + MX_SIS3820_IRQ_CONTROL_REG,
				interrupt_disable_mask );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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
	uint32_t reference_pulser;
	uint32_t acq_op_mode;
	uint32_t lne_prescale_factor;
	double pulse_period;
	double clock_frequency;
	double maximum_measurement_time;
	double maximum_possible_prescale_factor;
	double desired_lne_prescale_factor;
	mx_bool_type pulse_generator_busy = FALSE;
	unsigned long ctl_input_mode, ctl_output_mode;
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

	if ( mcs->external_channel_advance == FALSE ) {
		/* Use the internal 10 MHz pulse generator. */

		lne_prescale_factor =
	    mx_round( MX_SIS3820_10MHZ_INTERNAL_CLOCK * mcs->measurement_time );

	} else {
		/* Use the external channel advance. */

		if ( mcs->external_channel_advance_record == NULL ) {

			return mx_error(MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
			"External channel advance has been requested for "
			"MCS '%s', but no external source of channel "
			"advance pulses has been set up.", mcs->record->name );
		}

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

			lne_prescale_factor = (uint32_t)
			    mx_round( clock_frequency * mcs->measurement_time );

			desired_lne_prescale_factor =
				clock_frequency * mcs->measurement_time;

			/* The maximum possible LNE prescale factor is
			 * (2^32 - 1).  If the user requests a larger
			 * prescale factor, then tell the user that
			 * this will not work.
			 */

			maximum_possible_prescale_factor = 4294967295.0;

			if ( desired_lne_prescale_factor >=
					maximum_possible_prescale_factor )
			{
			    maximum_measurement_time = mx_divide_safely(
				maximum_possible_prescale_factor,
					clock_frequency );

			    return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested measurement time of %g seconds for MCS '%s' "
		"is larger than the maximum allowed measurement time "
		"per point of %g seconds.",
				mcs->measurement_time,
				sis3820->record->name, 
				maximum_measurement_time );
			}

#if MXD_SIS3820_DEBUG_START
			MX_DEBUG(-2,("%s: Using pulse generator, "
			"pulse_period = %g, clock = %g, prescale = %lu",
   				fname, pulse_period, clock_frequency,
				(unsigned long) lne_prescale_factor));
#endif

			/* Is the pulse generator running? */

			mx_status = mx_pulse_generator_is_busy( 
					mcs->external_channel_advance_record,
					&pulse_generator_busy );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( pulse_generator_busy == FALSE ) {
				/* If not, then start the pulse generator. */

				mx_status = mx_pulse_generator_start(
					mcs->external_channel_advance_record );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
			break;
		default:
			lne_prescale_factor = (uint32_t) mcs->external_prescale;
			break;
		}

#if MXD_SIS3820_DEBUG_START
		MX_DEBUG(-2,
	    ("%s: Using external channel advance, lne_prescale_factor = %lu",
		 	fname, (unsigned long) lne_prescale_factor));
#endif
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
	ctl_input_mode  = sis3820->control_input_mode;
	ctl_output_mode = sis3820->control_output_mode;

	acq_op_mode  = MXF_SIS3820_FIFO_MODE;
	acq_op_mode |= MXF_SIS3820_OP_MODE_MULTI_CHANNEL_SCALER;
	acq_op_mode |= MXF_SIS3820_ARM_ENABLE_CONTROL_SIGNAL;

	if ( ctl_output_mode < 4 ) {
		acq_op_mode |= ( ctl_output_mode << 20 );
	}

	if ( mcs->external_channel_advance ) {

		/* External channel advance mode (front panel control) */

		acq_op_mode |= MXF_SIS3820_LNE_SOURCE_CONTROL_SIGNAL;

		if ( ctl_input_mode < 8 ) {
			acq_op_mode |= ( ctl_input_mode << 16 );
		} else {
			/* Mode 1 is the default control input mode. */

			acq_op_mode |= MXF_SIS3820_CONTROL_INPUT_MODE1;
		}
	} else {
		/* Use internal pulse generator at 10 MHz. */

		acq_op_mode |= MXF_SIS3820_LNE_SOURCE_INTERNAL_10MHZ;

		if ( ctl_input_mode < 8 ) {
			acq_op_mode |= ( ctl_input_mode << 16 );
		} else {
			/* Mode 0 is the default control input mode. */

			acq_op_mode |= MXF_SIS3820_CONTROL_INPUT_MODE0;
		}
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

	/* Prepare the MCS for counting. */

	if ( mcs->external_channel_advance ) {

		/* For external channel advance, we arm the MCS. */

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
	} else {
		/* If using the internal 10 MHz pulser, we enable the MCS. */

#if MXD_SIS3820_DEBUG_START
		MX_DEBUG(-2,("%s: Calling KEY_OPERATION_ENABLE", fname));
#endif

		mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
		sis3820->base_address + MX_SIS3820_KEY_OPERATION_ENABLE_REG,
				0x1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mcs->measurement_number = -1;
	mcs->busy = TRUE;

	sis3820->new_start = TRUE;
	sis3820->fifo_stop = FALSE;

	/* Start the FIFO callback timer. */

	mx_status = mx_virtual_timer_start(
		sis3820->callback_message->u.function.oneshot_timer,
		sis3820->callback_message->u.function.callback_interval );

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

#if MXD_SIS3820_DEBUG_STOP
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, mcs->record->name));
#endif

	/* Disable counting. */
	
#if MXD_SIS3820_DEBUG_STOP
	MX_DEBUG(-2,("%s: Calling KEY_OPERATION_DISABLE", fname));
#endif
	mcs->busy = FALSE;

	sis3820->fifo_stop = TRUE;

	mx_status = mx_vme_out32( sis3820->vme_record,
			sis3820->crate_number,
			sis3820->address_mode,
		sis3820->base_address + MX_SIS3820_KEY_OPERATION_DISABLE_REG,
			0x1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Stop the FIFO callback timer. */

#if MXD_SIS3820_DEBUG_STOP
	MX_DEBUG(-2,("%s: Stopping the FIFO callback", fname));
#endif

	mx_status = mx_virtual_timer_stop(
		sis3820->callback_message->u.function.oneshot_timer, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sis3820_clear( MX_MCS *mcs )
{
	static const char fname[] = "mxd_sis3820_clear()";

	MX_SIS3820 *sis3820 = NULL;
	long **data_array = NULL;
	long i, j;
	uint32_t interrupt_disable_mask = 0;
	uint32_t status_flags_to_clear = 0;
	uint32_t mask = 0;
	mx_status_type mx_status;

	mx_status = mxd_sis3820_get_pointers( mcs, &sis3820, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, mcs->record->name));
#endif
	mcs->busy = FALSE;

	/* Clear the local data array. */

	data_array = mcs->data_array;

	if ( data_array != NULL ) {
		for ( i = 0; i < mcs->maximum_num_scalers; i++ ) {
			for ( j = 0; j < mcs->maximum_num_measurements; j++ ) {
				data_array[i][j] = 0;
			}
		}
	}

	/* Clear the FIFO. */

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
		sis3820->base_address + MX_SIS3820_KEY_COUNTER_CLEAR_REG,
				0x1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Clear latched IRQ status flags. */

	interrupt_disable_mask = (sis3820->disabled_interrupts) << 8;

	status_flags_to_clear = (sis3820->disabled_interrupts) << 16;

	mask = ( interrupt_disable_mask | status_flags_to_clear );

	MX_DEBUG(-2,("%s: clearing and disabling IRQ sources, mask = %#lx",
		fname, (unsigned long) mask ));

	mx_status = mx_vme_out32( sis3820->vme_record,
				sis3820->crate_number,
				sis3820->address_mode,
		sis3820->base_address + MX_SIS3820_IRQ_CONTROL_REG,
				mask );

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

#if 0 && MXD_SIS3820_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, mcs->record->name));
#endif

	/* The FIFO callback saves this information. */

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
	case MXLV_MCS_MEASUREMENT_NUMBER:
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

