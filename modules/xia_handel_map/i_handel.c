/*
 * Name:    i_handel.c
 *
 * Purpose: MX driver for the X-Ray Instrumentation Associates Handel library
 *          used by the DXP series of multichannel analyzer.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2006, 2009-2012, 2016-2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_HANDEL_DEBUG			FALSE

#define MXI_HANDEL_DEBUG_TIMING			FALSE

#define MXI_HANDEL_DEBUG_MONITOR_THREAD		TRUE

#define MXI_HANDEL_DEBUG_MONITOR_THREAD_BUFFERS	TRUE

#define MXI_HANDEL_DEBUG_WORK_FUNCTIONS		TRUE

#define MXI_HANDEL_DEBUG_MAPPING_PIXEL_NEXT	TRUE

#define MXI_HANDEL_CHANNELS_PER_MODULE		4

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>

#if defined(OS_WIN32)
#include <direct.h>
#endif

#include <handel.h>
#include <handel_errors.h>
#include <handel_generic.h>
#include <md_generic.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_stdint.h"
#include "mx_array.h"
#include "mx_unistd.h"
#include "mx_mutex.h"
#include "mx_thread.h"
#include "mx_atomic.h"
#include "mx_mca.h"
#include "mx_mcs.h"
#include "i_handel.h"
#include "d_handel_mca.h"
#include "d_handel_mcs.h"

#if MXI_HANDEL_DEBUG_TIMING
#  include "mx_hrt_debug.h"
#endif

MX_RECORD_FUNCTION_LIST mxi_handel_record_function_list = {
	mxi_handel_initialize_driver,
	mxi_handel_create_record_structures,
	mxi_handel_finish_record_initialization,
	NULL,
	NULL,
	mxi_handel_open,
	NULL,
	NULL,
	mxi_handel_resynchronize,
	mxi_handel_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_handel_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_HANDEL_STANDARD_FIELDS
};

long mxi_handel_num_record_fields
		= sizeof( mxi_handel_record_field_defaults )
			/ sizeof( mxi_handel_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_handel_rfield_def_ptr
			= &mxi_handel_record_field_defaults[0];

static mx_status_type mxi_handel_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );

extern const char *mxi_handel_strerror( int xia_status );

static mx_status_type
mxi_handel_get_pointers( MX_MCA *mca,
			MX_HANDEL_MCA **handel_mca,
			MX_HANDEL **handel,
			const char *calling_fname )
{
	static const char fname[] = "mxi_handel_get_pointers()";

	MX_RECORD *handel_record;

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( handel_mca == (MX_HANDEL_MCA **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_HANDEL_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( handel == (MX_HANDEL **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_HANDEL pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mca->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*handel_mca = (MX_HANDEL_MCA *) mca->record->record_type_struct;

	handel_record = (*handel_mca)->handel_record;

	if ( handel_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The handel_record pointer for MCA record '%s' is NULL.",
			mca->record->name );
	}

	*handel = (MX_HANDEL *) handel_record->record_type_struct;

	if ( (*handel) == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HANDEL pointer for XIA DXP record '%s' is NULL.",
			handel_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxi_handel_get_mcs_array( MX_HANDEL *handel,
				unsigned long num_mcs,
				 MX_MCS **mcs_array )
{
	static const char fname[] = "mxi_handel_get_mcs_array()";

	MX_RECORD *mca_record = NULL;
	MX_HANDEL_MCA *handel_mca = NULL;
	MX_RECORD *mcs_record = NULL;
	MX_MCS *mcs = NULL;
	unsigned long i;

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HANDEL pointer passed was NULL." );
	}
	if ( mcs_array == (MX_MCS **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The mcs_array pointer passed was NULL." );
	}

	MX_DEBUG(-2,("%s: **mcs_array = %p", fname, **mcs_array));

	for ( i = 0; i < num_mcs; i++ ) {
		mcs_array[i] = NULL;

		mca_record = handel->mca_record_array[i];

		if ( mca_record == (MX_RECORD *) NULL ) {
			/* Skip over MCA records that are not present. */
			continue;
		}

		handel_mca = (MX_HANDEL_MCA *) mca_record->record_type_struct;

		if ( handel_mca == (MX_HANDEL_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_HANDEL_MCA pointer for MCA '%s' belonging "
			"to Handel record '%s' is NULL.",
				mca_record->name, handel->record->name );
		}

		mcs_record = handel_mca->child_mcs_record;

		/* If mcs_record is NULL, then no child MCS record has
		 * been set up for this MCA in the MX database.  This
		 * means that we can skip over this mcs_array entry
		 * by setting the array element to NULL.
		 */

		if ( mcs_record == (MX_RECORD *) NULL ) {
			mcs_array[0] = NULL;

			/* Go on to the next entry in the MCA array. */
			continue;	
		}

		mcs = (MX_MCS *) mcs_record->record_class_struct;

		if ( mcs == (MX_MCS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MCS pointer for MCS record '%s' that depends "
			"on MCA '%s' and Handel record '%s' is NULL.",
				mcs_record->name, mca_record->name,
				handel->record->name );
		}

		mcs_array[i] = mcs;

		MX_DEBUG(-2,("%s: mcs_array[%d] = %p", fname, i, mcs_array[i]));
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----*/

static mx_status_type
mxi_handel_wait_for_buffers_full( MX_HANDEL *handel, char buffer_name )
{
	static const char fname[] = "mxi_handel_wait_for_buffers_full()";

	char run_data_name[20];
	unsigned short buffer_full;
	int xia_status, channel;

	unsigned long wait_for_buffer_sleep_ms = 100;  /* in milliseconds */

#if MXI_HANDEL_DEBUG_WORK_FUNCTIONS
	MX_DEBUG(-2,("%s invoked for record '%s', buffer_name = '%c'",
		fname, handel->record->name, buffer_name ));
#endif

	snprintf( run_data_name, sizeof(run_data_name),
		"buffer_full_%c", buffer_name );

	for ( channel = 0;
		channel < MXI_HANDEL_CHANNELS_PER_MODULE;
		channel++ )
	{
	    buffer_full = 0;

	    while ( buffer_full == 0 ) {
		MX_XIA_SYNC( xiaGetRunData( channel,
					run_data_name,
					&buffer_full ) );

		if ( xia_status != XIA_SUCCESS ) {
			handel->monitor_thread = NULL;

			return mx_error( MXE_DEVICE_ACTION_FAILED,fname,
			"The attempt to check the status of XIA buffer_%c "
			"for channel %d failed for Handel record '%s'.  "
			"Error code = %d, '%s'.",
				buffer_name, channel, handel->record->name,
				xia_status, mxi_handel_strerror(xia_status) );
		}

		mx_msleep( wait_for_buffer_sleep_ms );
	    }
	}

#if MXI_HANDEL_DEBUG_WORK_FUNCTIONS
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-----*/

static mx_status_type
mxi_handel_read_buffers( MX_HANDEL *handel,
			unsigned long measurement_number,
			char buffer_name,
			unsigned long num_buffers,
			MX_MCS **mcs_array )
{
	static const char fname[] = "mxi_handel_read_buffers()";

	MX_MCS *mcs = NULL;
	MX_HANDEL_MCS *handel_mcs = NULL;
	uint32_t *buffer_ptr = NULL;
	uint32_t buffer_scaler_value;
	char run_data_name[20];
	int xia_status, channel, scaler;

#if MXI_HANDEL_DEBUG_WORK_FUNCTIONS
	MX_DEBUG(-2,("%s invoked for record '%s', measurement_number = %lu, "
		"buffer_name = '%c', num_buffers = %lu, mcs_array = %p",
		fname, handel->record->name, measurement_number,
		buffer_name, num_buffers, mcs_array));
#endif

	snprintf( run_data_name, sizeof(run_data_name),
		"buffer_%c", buffer_name );

	for ( channel = 0;
		channel < MXI_HANDEL_CHANNELS_PER_MODULE;
		channel++ )
	{
	    mcs = mcs_array[channel];

	    /* We do not necessarily read from _all_ channels. */

	    if ( mcs != (MX_MCS *) NULL ) {
		if ( mcs->record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RECORD pointer for MCS channel %d is NULL.",
				channel );
		}

		handel_mcs = (MX_HANDEL_MCS *) mcs->record->record_type_struct;

		if ( handel_mcs == (MX_HANDEL_MCS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_HANDEL_MCS pointer for MCS '%s' is NULL.",
				mcs->record->name );
		}

		switch( buffer_name ) {
		case 'a':
			buffer_ptr = handel_mcs->buffer_a;
			break;
		case 'b':
			buffer_ptr = handel_mcs->buffer_b;
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal Handel buffer name 'buffer_%c' requested "
			"for Handel MCS '%s'.  The allowed names are "
			"'buffer_a' and 'buffer_b'.",
				buffer_name, mcs->record->name );
			break;
		}

		mx_mutex_lock( handel->mutex );

		xia_status = xiaGetRunData( channel, run_data_name, buffer_ptr);

		if ( xia_status != XIA_SUCCESS ) {
			mx_mutex_unlock( handel->mutex );

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to read XIA '%s' for MCS '%s' failed.  "
			"Error code = %d, '%s'.",
				run_data_name, mcs->record->name,
				xia_status, mxi_handel_strerror(xia_status) );
		}

		/* Copy the XIA buffer to the appropriate MCS buffer. */

		for ( scaler = 0; scaler < mcs->current_num_scalers; scaler++ )
		{
			buffer_scaler_value = buffer_ptr[ scaler ];

			mcs->data_array[ scaler ][ measurement_number ]
							= buffer_scaler_value;
		}

		mx_mutex_unlock( handel->mutex );
	    }
	}

#if MXI_HANDEL_DEBUG_WORK_FUNCTIONS
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-----*/

static mx_status_type
mxi_handel_notify_buffers_read( MX_HANDEL *handel, char buffer_name )
{
	static const char fname[] = "mxi_handel_notify_buffers_read()";

	int xia_status, channel;

#if MXI_HANDEL_DEBUG_WORK_FUNCTIONS
	MX_DEBUG(-2,("%s invoked for record '%s', buffer_name = '%c'",
		fname, handel->record->name, buffer_name));
#endif

	for ( channel = 0;
		channel < MXI_HANDEL_CHANNELS_PER_MODULE;
		channel++ )
	{
		MX_XIA_SYNC( xiaBoardOperation( channel,
					"buffer_done", &buffer_name ) );

		if ( xia_status != XIA_SUCCESS ) {
			handel->monitor_thread = NULL;

			return mx_error( MXE_DEVICE_ACTION_FAILED,fname,
			"The attempt to signal completion of XIA buffer_%c "
			"for channel %d failed for Handel record '%s'.  "
			"Error code = %d, '%s'.",
				buffer_name, channel, handel->record->name,
				xia_status, mxi_handel_strerror(xia_status) );
		}
	}

#if MXI_HANDEL_DEBUG_WORK_FUNCTIONS
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-----*/

/*
 * mxi_handel_mcs_monitor_thread_fn() should only be running while an
 * MCS sequence is running.  It is started by mxd_handel_mcs_arm()
 * and exits when the MCS sequence is complete.
 */

MX_EXPORT mx_status_type
mxi_handel_mcs_monitor_thread_fn( MX_THREAD *thread, void *thread_args )
{
	static const char fname[] = "mxi_handel_mcs_monitor_thread_fn()";

	MX_HANDEL *handel = NULL;

	unsigned long num_mcs = 0;

	MX_MCS *mcs_array[MXI_HANDEL_CHANNELS_PER_MODULE];

	MX_MCS *master_mcs = NULL;

	unsigned long j;
	int xia_status;
	mx_status_type mx_status;

#if 0
	mx_breakpoint();
#endif

	if ( thread_args == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The thread_args pointer passed was NULL.  Warning: The "
		"handel->monitor_thread will now point to a memory block "
		"that has been freed.  However, there is no real way to do "
		"anything about this, since we were supposed to get the "
		"MX_HANDEL pointer from the thread_args pointer." );
	}

	handel = (MX_HANDEL *) thread_args;

#if MXI_HANDEL_DEBUG_MONITOR_THREAD
	MX_DEBUG(-2,("%s invoked for XIA Handel record '%s'.",
		fname, handel->record->name ));
#endif

	/* Get an array of all the MX_MCS data structures that the MX_HANDEL
	 * record knows about.
	 */

	num_mcs = handel->num_mcas;

	if ( num_mcs > MXI_HANDEL_CHANNELS_PER_MODULE ) {
		mx_warning( "The detector connected to XIA record '%s' "
		"has %ld MCAs, but this alpha-test version of MX MCS emulation "
		"only supports the first %d channels.",
			handel->record->name,
			handel->num_mcas,
			MXI_HANDEL_CHANNELS_PER_MODULE );

		num_mcs = MXI_HANDEL_CHANNELS_PER_MODULE;
	}

#if 0
	MX_DEBUG(-2,("%s: handel = %p '%s', handel->num_mcas = %ld",
		fname, handel, handel->record->name, handel->num_mcas));
	MX_DEBUG(-2,("%s: handel->mca_record_array = %p",
		fname, handel->mca_record_array));
	{
		int n;

		for ( n = 0; n < num_mcs; n++ ) {
			MX_DEBUG(-2,("%s: handel->mca_record_array[%d] = %p",
				fname, n, handel->mca_record_array[n]));

			if ( handel->mca_record_array[n] != NULL ) {
			 MX_DEBUG(-2,("%s: handel->mca_record_array[%d] = '%s'",
				fname, n, handel->mca_record_array[n]->name));
			}
		}
	}
#endif

	mx_status = mxi_handel_get_mcs_array( handel, num_mcs, mcs_array );

	if ( mx_status.code != MXE_SUCCESS ) {
		handel->monitor_thread = NULL;
		return mx_status;
	}

	/* We need to find the MX_HANDEL_MCS that started this sequence. */

	master_mcs = mcs_array[0];   /* FIXME: This is an oversimplfication. */

	/* Loop until all of the requested measurements have been taken. */

	j = 0;		/* j is the measurement number (starting at 0) */

	while (TRUE) {
		if ( j >= master_mcs->current_num_measurements ) {
			break;		/* Exit the while() loop. */
		}

#if MXI_HANDEL_DEBUG_MONITOR_THREAD_BUFFERS
	MX_DEBUG(-2,("%s: j = %lu, WAITING for buffer_a", fname, j ));
#endif
		mx_status = mxi_handel_wait_for_buffers_full( handel, 'a' );

		if ( mx_status.code != MXE_SUCCESS ) {
			handel->monitor_thread = NULL;
			return mx_status;
		}

#if MXI_HANDEL_DEBUG_MONITOR_THREAD_BUFFERS
	MX_DEBUG(-2,("%s: j = %lu, buffer_a AVAILABLE", fname, j ));
#endif
		mx_status = mxi_handel_read_buffers( handel, j,
							'a', 4, mcs_array );

		if ( mx_status.code != MXE_SUCCESS ) {
			handel->monitor_thread = NULL;
			return mx_status;
		}

#if MXI_HANDEL_DEBUG_MONITOR_THREAD_BUFFERS
	MX_DEBUG(-2,("%s: j = %lu, buffer_a COPIED", fname, j ));
#endif
		mx_status = mxi_handel_notify_buffers_read( handel, 'a' );

		if ( mx_status.code != MXE_SUCCESS ) {
			handel->monitor_thread = NULL;
			return mx_status;
		}

#if MXI_HANDEL_DEBUG_MONITOR_THREAD
		MX_DEBUG(-2,("%s: Handel '%s' MEASUREMENT %ld COMPLETE.",
			fname, handel->record->name, j ));
#endif
		j++;

		mx_atomic_increment32( &(handel->last_pixel_number) );

		mx_atomic_increment32( &(handel->total_num_pixels) );

		if ( j >= master_mcs->current_num_measurements ) {
			break;		/* Exit the while() loop. */
		}

#if MXI_HANDEL_DEBUG_MONITOR_THREAD_BUFFERS
	MX_DEBUG(-2,("%s: j = %lu, WAITING for buffer_b", fname, j ));
#endif
		mx_status = mxi_handel_wait_for_buffers_full( handel, 'b' );

		if ( mx_status.code != MXE_SUCCESS ) {
			handel->monitor_thread = NULL;
			return mx_status;
		}

#if MXI_HANDEL_DEBUG_MONITOR_THREAD_BUFFERS
	MX_DEBUG(-2,("%s: j = %lu, buffer_b AVAILABLE", fname, j ));
#endif
		mx_status = mxi_handel_read_buffers( handel, j,
			       				'b', 4, mcs_array );

		if ( mx_status.code != MXE_SUCCESS ) {
			handel->monitor_thread = NULL;
			return mx_status;
		}

#if MXI_HANDEL_DEBUG_MONITOR_THREAD_BUFFERS
	MX_DEBUG(-2,("%s: j = %lu, buffer_b COPIED", fname, j ));
#endif
		mx_status = mxi_handel_notify_buffers_read( handel, 'b' );

		if ( mx_status.code != MXE_SUCCESS ) {
			handel->monitor_thread = NULL;
			return mx_status;
		}

#if MXI_HANDEL_DEBUG_MONITOR_THREAD
		MX_DEBUG(-2,("%s: Handel '%s' MEASUREMENT %ld COMPLETE.",
			fname, handel->record->name, j ));
#endif
		j++;

		mx_atomic_increment32( &(handel->last_pixel_number) );

		mx_atomic_increment32( &(handel->total_num_pixels) );
	}

	MX_XIA_SYNC( xiaStopRun(-1) );

	if ( xia_status != XIA_SUCCESS ) {
		handel->monitor_thread = NULL;

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to stop a run for Handel '%s' failed.  "
		"Error code = %d, '%s'.",
			handel->record->name,
			xia_status, mxi_handel_strerror(xia_status) );
	}

#if MXI_HANDEL_DEBUG_MONITOR_THREAD
	MX_DEBUG(-2,("%s: Handel '%s' run stopped.",
		fname, handel->record->name));
#endif

#if MXI_HANDEL_DEBUG_MONITOR_THREAD
	MX_DEBUG(-2,("%s complete for XIA Handel record '%s'.",
		fname, handel->record->name ));
#endif
	handel->monitor_thread = NULL;

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_initialize_driver( MX_DRIVER *driver )
{
	static const char fname[] = "mxi_handel_initialize_driver()";

	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	long num_active_detector_channels_varargs_cookie;
	MX_RECORD_FIELD_DEFAULTS *field;
	long referenced_field_index;
	mx_status_type mx_status;

	record_field_defaults = *(driver->record_field_defaults_ptr);

	if ( record_field_defaults == (MX_RECORD_FIELD_DEFAULTS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults' for record type '%s' is NULL.",
			driver->name );
	}

	if ( driver->num_record_fields == (long *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'num_record_fields' pointer for record type '%s' is NULL.",
			driver->name );
	}

	/* Get varargs cookie for 'num_active_detector_channels'. */

	mx_status = mx_find_record_field_defaults_index( driver,
						"num_active_detector_channels",
						&referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
				&num_active_detector_channels_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set varargs cookie for 'active_detector_channel_array'. */

	mx_status = mx_find_record_field_defaults( driver,
						"active_detector_channel_array",
						&field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_active_detector_channels_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_handel_create_record_structures()";

	MX_HANDEL *handel;

	/* Allocate memory for the necessary structures. */

	handel = (MX_HANDEL *) malloc( sizeof(MX_HANDEL) );

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Can't allocate memory for MX_HANDEL structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;;
	record->record_type_struct = handel;

	record->class_specific_function_list = NULL;

	handel->record = record;

	handel->last_measurement_interval = -1.0;

#if MXI_HANDEL_DEBUG
	handel->debug_flag = TRUE;
#else
	handel->debug_flag = FALSE;
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_handel_finish_record_initialization()";

	MX_HANDEL *handel;

	handel = (MX_HANDEL *) record->record_type_struct;

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HANDEL pointer for record '%s' is NULL.",
			record->name );
	}

	/* We cannot allocate the following structures until
	 * mxi_handel_open() is invoked, so for now we
	 * initialize these data structures to all zeros.
	 */

	handel->num_mcas = 0;
	handel->mca_record_array = NULL;
	handel->save_filename[0] = '\0';

	return MX_SUCCESSFUL_RESULT;
}

/* mxi_handel_set_data_available_flags() sets mca->new_data_available
 * and handel_mca->new_statistics_available for all of the MCAs controlled
 * by this Handel interface.
 */

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_set_data_available_flags( MX_HANDEL *handel,
					int flag_value )
{
	static const char fname[] =
		"mxi_handel_set_data_available_flags()";

	MX_RECORD *mca_record, **mca_record_array;
	MX_MCA *mca;
	MX_HANDEL_MCA *handel_mca;
	unsigned long i;

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HANDEL pointer passed was NULL." );
	}

	MX_DEBUG( 2,("%s invoked for '%s', flag_value = %d",
		fname, handel->record->name, flag_value));

	mca_record_array = handel->mca_record_array;

	if ( mca_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The mca_record_array pointer for 'handel' record '%s'.",
			handel->record->name );
	}

	for ( i = 0; i < handel->num_mcas; i++ ) {

		mca_record = (handel->mca_record_array)[i];

		if ( mca_record == (MX_RECORD *) NULL ) {
			/* Skip this MCA slot. */

			continue;
		}

		mca = (MX_MCA *) mca_record->record_class_struct;

		if ( mca == (MX_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MCA pointer for MCA record '%s' is NULL.",
				mca_record->name );
		}

		handel_mca = (MX_HANDEL_MCA *) mca_record->record_type_struct;

		if ( handel_mca == (MX_HANDEL_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The MX_HANDEL_MCA pointer for MCA record '%s' is NULL.",
				mca_record->name );
		}

		mca->new_data_available = flag_value;
		handel_mca->new_statistics_available = flag_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxi_handel_load_config( MX_HANDEL *handel )
{
	static const char fname[] = "mxi_handel_load_config()";

	MX_RECORD *mca_record;
	MX_HANDEL_MCA *handel_mca;
	int i, xia_status;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	/* Load the configuration file. */

	mx_info("Loading Handel configuration '%s'.",
				handel->config_filename);

	mx_info("This may take a while...");

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	MX_XIA_SYNC( xiaLoadSystem( "handel_ini", handel->config_filename ) );

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"xiaLoadSystem(\"handel_ini\", \"%s\")",
		handel->config_filename );
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Unable to load the Handel detector configuration file '%s'.  "
		"Error code = %d, '%s'",
			handel->config_filename,
			xia_status, mxi_handel_strerror( xia_status ) );
	}

	errno = 0;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	MX_XIA_SYNC( xiaStartSystem() );

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, "xiaStartSystem()" );
#endif
	/* A note on checking the return value from xiaStartSystem().
	 *
	 * On Linux, if usb_set_configuration() fails due to the program
	 * not running as root, then usb_set_configuration() returns a 
	 * value of (-1) and sets errno to EPERM.  As it happens, the
	 * (-1) value gets discarded during the process of unwinding the
	 * stack back up to where we are now.  However, the EPERM errno
	 * seems to generally remain unscathed on the way back up.  Of
	 * course, this is not _guaranteed_ to happen, so we merely 
	 * "suggest" to the user that they _might_ need to run as root.
	 */

	if ( xia_status != XIA_SUCCESS ) {
		if ( errno == EPERM ) {
			mx_status =  mx_error(
				MXE_INTERFACE_ACTION_FAILED, fname,
				"Unable to start the MCA using Handel "
				"configuration file '%s'.  "
				"Errno was set to EPERM, so this _may_ "
				"mean that you need to run this program "
				"as root.  But this is not "
				"guaranteed to fix the problem.",
					handel->config_filename );
		} else {
			mx_status =  mx_error(
				MXE_INTERFACE_ACTION_FAILED, fname,
				"Unable to start the MCA using Handel "
				"configuration file '%s'.  "
				"Error code = %d, '%s'",
					handel->config_filename,
					xia_status,
					mxi_handel_strerror( xia_status ) );
		}
		return mx_status;
	}

	/* Invalidate the old presets for all of the MCAs by setting the
	 * old preset type to an illegal value.
	 */

	if ( handel->mca_record_array != NULL ) {
		for ( i = 0; i < handel->num_mcas; i++ ) {
			mca_record = handel->mca_record_array[i];

			if ( mca_record != NULL ) {
				handel_mca = (MX_HANDEL_MCA *)
					mca_record->record_type_struct;

				handel_mca->old_preset_type
					= (unsigned long) ULONG_MAX;
			}
		}
	}

	mx_info("Successfully loaded Handel configuration '%s'.",
				handel->config_filename);

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

/*
 * mxi_handel_load_new_config() encapsulates all of the operations that
 * must happen when a new configuration file is loaded by writing to the
 * MX record field 'config_filename'.
 */

static mx_status_type
mxi_handel_load_new_config( MX_HANDEL *handel )
{
	static const char fname[] = "mxi_handel_load_new_config()";

	MX_RECORD *mca_record;
	MX_HANDEL_MCA *handel_mca;
	int i, xia_status;
	double num_scas;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	/* Load the new configuration from the config file. */

	mx_status = mxi_handel_load_config( handel );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If possible, set the number of SCAs to 16 for each MCA. */

	for ( i = 0; i < handel->num_mcas; i++ ) {
		mca_record = handel->mca_record_array[i];

		if ( mca_record != (MX_RECORD *) NULL ) {
			handel_mca = (MX_HANDEL_MCA *)
					mca_record->record_type_struct;

			if ( handel_mca->hardware_scas_are_enabled ) {
				num_scas = 16.0;

				MX_DEBUG( 2,
				("%s: Setting 'number_of_scas' to %g",
					fname, num_scas));

				MX_XIA_SYNC( xiaSetAcquisitionValues(
					handel_mca->detector_channel,
					"number_of_scas",
					(void *) &num_scas ) );

				if ( xia_status != XIA_SUCCESS ) {
					return mx_error(
					    MXE_INTERFACE_ACTION_FAILED, fname,
		"Attempt to set the number of SCAs to 16 for MCA '%s' failed.  "
		"Error code = %d, '%s'", mca_record->name,
		xia_status, mxi_handel_strerror( xia_status ) );
				}
			}
		}
	}

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s'", handel->record->name );
#endif
	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxi_handel_save_config( MX_HANDEL *handel )
{
	static const char fname[] = "mxi_handel_save_config()";

	int xia_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	MX_XIA_SYNC( xiaSaveSystem( "handel_ini", handel->save_filename ) );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Attempt to save the Handel configuration to the file '%s' "
		"failed for MCA '%s'.  Error code = %d, '%s'",
		handel->save_filename, handel->record->name,
		xia_status, mxi_handel_strerror( xia_status ) );
	}

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s'", handel->record->name );
#endif
	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static void
mxi_handel_redirect_log_output( MX_HANDEL *handel )
{
	char log_filename[MXU_FILENAME_LENGTH+1];
	char *mxdir;
	int xia_status;

	if ( strlen( handel->log_filename ) > 0 ) {
		strlcpy( log_filename, handel->log_filename,
						MXU_FILENAME_LENGTH );
	} else {
		mxdir = getenv("MXDIR");

		if ( mxdir == NULL ) {
			strlcpy( log_filename,
				"/opt/mx/log/handel.log",
				sizeof(log_filename)  );
		} else {
			snprintf( log_filename, sizeof(log_filename),
				"%s/log/handel.log", mxdir );
		}
	}

	mx_info( "Redirecting Handel log output to '%s'.", log_filename );

	/* Redirect the log output. */

	MX_XIA_SYNC( xiaSetLogOutput( log_filename ) );

	if ( xia_status != XIA_SUCCESS ) {
		mx_warning(
		    "Unable to redirect XIA Handel log output to file '%s'.  "
		    "Error code = %d, '%s'", log_filename, xia_status,
			mxi_handel_strerror( xia_status ) );
	}

	return;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_handel_open()";

	MX_HANDEL *handel;
	char detector_alias[MAXALIAS_LEN+1];
	char version_string[80];
	int xia_status, display_config;
	unsigned int i, j, num_detectors, num_modules;
	unsigned int num_mcas, total_num_mcas;
	void *void_ptr;
	long dimension_array[2];
	size_t size_array[2];
	int os_status, saved_errno;
	mx_status_type mx_status;

#if MX_IGNORE_XIA_NULL_STRING
	char *ignore_this_pointer;
#endif

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	/* The following statement exist only to suppress the GCC warning
	 * "warning: `XIA_NULL_STRING' defined but not used".  You should
	 * not actually use the variable 'ignore_this_pointer' for anything.
	 */

#if MX_IGNORE_XIA_NULL_STRING
	ignore_this_pointer = XIA_NULL_STRING;
#endif

	MX_DEBUG( 2,("%s invoked.", fname));

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	handel = (MX_HANDEL *) (record->record_type_struct);

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_HANDEL pointer for record '%s' is NULL.", record->name);
	}

	/* Set up the MX Handel mutex. */

	mx_status = mx_mutex_create( &(handel->mutex) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

	display_config = handel->handel_flags &
			MXF_HANDEL_DISPLAY_CONFIGURATION_AT_STARTUP;

#if 0
	/* Redirect Handel and Xerxes output to the MX output functions. */

	mx_status = mxu_xia_dxp_replace_output_functions();

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	/* Initialize mca_record_array to NULL, so that load_config
	 * below knows not to try to change the MCA presets.
	 */

	handel->mca_record_array = NULL;

	/* Check to see if the XIA .ini file is readable. */

	os_status = access( handel->config_filename, R_OK );

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_PERMISSION_DENIED, fname,
		"This process does not have the permissions required to "
		"read the Handel configuration file '%s'.  "
		"errno = %d, error message = '%s'.",
			handel->config_filename,
			saved_errno, strerror( saved_errno ) );
	}

	/* Initialize Handel. */

	MX_XIA_SYNC( xiaInitHandel() );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to initialize Handel.  "
		"Error code = %d, '%s'",
			xia_status, mxi_handel_strerror( xia_status ) );
	}

	/* If requested, redirect log output to a file. */

	if (handel->handel_flags & MXF_HANDEL_WRITE_LOG_OUTPUT_TO_FILE)
	{
		mxi_handel_redirect_log_output( handel );
	}

	/* Set the logging level. */

	if ( handel->handel_log_level < MD_ERROR ) {
		handel->handel_log_level = MD_ERROR;
	}
	if ( handel->handel_log_level > MD_DEBUG ) {
		handel->handel_log_level = MD_DEBUG;
	}

	MX_XIA_SYNC( xiaSetLogLevel( handel->handel_log_level ) );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"An attempt to set the Handel log level to %lu failed.  "
		"Error code = %d, '%s'", handel->handel_log_level,
			xia_status, mxi_handel_strerror( xia_status ) );
	}

	/* Display the version of Handel/Xerxes. */

	xiaGetVersionInfo( NULL, NULL, NULL, version_string );

	mx_info("MX is using Handel %s", version_string);

	/* Load the detector configuration. */

	mx_status = mxi_handel_load_config( handel );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* How many detectors are there? */

	MX_XIA_SYNC( xiaGetNumDetectors( &num_detectors ) );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to find out how many detectors are in "
		"the current configuration '%s'.  "
		"Error code = %d, '%s'",
			handel->config_filename,
			xia_status, mxi_handel_strerror( xia_status ) );
	}

	handel->num_detectors = num_detectors;

	total_num_mcas = 0;

	/* Find out how many detector channels (MCAs) there are
	 * for all detectors.
	 */

	for ( i = 0; i < num_detectors; i++ ) {

		MX_XIA_SYNC( xiaGetDetectors_VB( i, detector_alias ) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to get the detector alias for XIA detector %d.  "
		"Error code = %d, '%s'", i,
			xia_status, mxi_handel_strerror( xia_status ) );
		}

		num_mcas = 0;

		void_ptr = (void *) &num_mcas;

		MX_XIA_SYNC( xiaGetDetectorItem( detector_alias,
						"number_of_channels",
						void_ptr ) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
	"Unable to get the number of channels for XIA detector %d '%s'.  "
	"Error code = %d, '%s'", i, detector_alias,
			xia_status, mxi_handel_strerror( xia_status ) );
		}

		total_num_mcas += num_mcas;
	}

	handel->num_mcas = total_num_mcas;

	/* Allocate and initialize an array to store pointers to all of
	 * the MCA records used by this interface.
	 */

	handel->mca_record_array = ( MX_RECORD ** ) malloc(
					num_mcas * sizeof( MX_RECORD * ) );

	if ( handel->mca_record_array == ( MX_RECORD ** ) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of MX_RECORD pointers for the mca_record_array data "
		"structure of record '%s'.",
			handel->num_mcas, record->name );
	}

#if 1
	MX_DEBUG(-2,("%s: handel->mca_record_array = %p",
		fname, handel->mca_record_array));
#endif

	for ( i = 0; i < handel->num_mcas; i++ ) {
		handel->mca_record_array[i] = NULL;
	}

	/* Find out how many modules there are. */

	MX_XIA_SYNC( xiaGetNumModules( &num_modules ) );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to find out how many modules are in "
		"the current configuration '%s'.  "
		"Error code = %d, '%s'",
			handel->config_filename,
			xia_status, mxi_handel_strerror( xia_status ) );
	}

	handel->num_modules = num_modules;

	/* Allocate a two-dimensional module pointer array. */

	handel->mcas_per_module = 4;	/* FIXME: For the DXP-XMAP. */

	dimension_array[0] = handel->num_modules;
	dimension_array[1] = handel->mcas_per_module;

	size_array[0] = sizeof(MX_RECORD *);
	size_array[1] = sizeof(MX_RECORD **);

	handel->module_array = mx_allocate_array( MXFT_RECORD,
				2, dimension_array, size_array );

	if ( handel->module_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a ( %ld x %ld ) "
		"module array for Handel interface '%s'.",
			handel->num_modules, handel->mcas_per_module,
			record->name );
	}

	for ( i = 0; i < handel->num_modules; i++ ) {
		for ( j = 0; j < handel->mcas_per_module; j++ ) {
			handel->module_array[i][j] = NULL;
		}
	}

	handel->use_module_statistics_2 = TRUE;

	/* Initialize atomic counter variables. */

	mx_atomic_write32( &(handel->last_pixel_number), -1 );
	mx_atomic_write32( &(handel->total_num_pixels), 0 );
	mx_atomic_write32( &(handel->total_num_pixels_at_start), 0 );

#if 1
	MX_DEBUG(-2,("%s: last_pixel_number = %d, total_num_pixels = %d, "
	"total_num_pixels_at_start = %d",
		fname, (int) handel->last_pixel_number,
		(int) handel->total_num_pixels,
		(int) handel->total_num_pixels_at_start ));
#endif

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s'", record->name );
#endif

	if ( display_config ) {
		mx_info( "XIA Handel library initialized for MX record '%s'.",
			record->name );
		mx_info(
		"num detectors = %ld, num channels = %ld, num modules = %ld",
			handel->num_detectors,
			handel->num_mcas,
			handel->num_modules );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_handel_resynchronize()";

	MX_HANDEL *handel;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	handel = (MX_HANDEL *) (record->record_type_struct);

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_HANDEL pointer for record '%s' is NULL.", record->name);
	}

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mxi_handel_load_config( handel );

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, "for record '%s'", record->name );
#endif

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_set_acquisition_values_for_all_channels( MX_HANDEL *handel,
						char *value_name,
						double *value_ptr,
						mx_bool_type apply_flag )
{
	static const char fname[] =
		"mxi_handel_set_acquisition_values_for_all_channels()";

	int xia_status;
	mx_status_type mx_status;

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HANDEL pointer passed was NULL." );
	}

	if ( value_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'value_name' argument passed for "
		"Handel interface '%s' was NULL.",
			handel->record->name );
	}
	if ( value_ptr == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'value_ptr' argument passed for "
		"Handel interface '%s' was NULL.",
			handel->record->name );
	}

	if ( handel->debug_flag ) {
		MX_DEBUG(-2,("%s: setting acquisition value '%s' for "
			"Handel interface '%s' to %g.",
			fname, value_name, handel->record->name, *value_ptr ));
	}

	MX_XIA_SYNC( xiaSetAcquisitionValues( -1,
				value_name, (void *) value_ptr ) );

	switch( xia_status ) {
	case XIA_SUCCESS:
		break;		/* Everything is OK. */
	case XIA_UNKNOWN_VALUE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested acquisition value '%s' for Handel "
			"interface '%s' does not exist.",
				value_name, handel->record->name );
		break;
	default:
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Cannot set acquisition value '%s' for "
		"Handel interface '%s' to %g.  "
		"Error code = %d, '%s'", 
					value_name,
					handel->record->name,
					*value_ptr,
					xia_status,
					mxi_handel_strerror( xia_status ) );
	}

	if ( apply_flag ) {
		mx_status = mxi_handel_apply_to_all_channels( handel );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_apply_to_all_channels( MX_HANDEL *handel )
{
	static const char fname[] = "mxi_handel_apply_to_all_channels()";

	int m, xia_status, ignored;
	MX_RECORD *first_mca_record;
	MX_HANDEL_MCA *first_handel_mca;

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HANDEL pointer passed was NULL." );
	}

	if ( handel->debug_flag ) {
		MX_DEBUG(-2,("%s for Handel record '%s'",
			fname, handel->record->name ));
	}

	for ( m = 0; m < handel->num_modules; m++ ) {
		/* Find the first MCA for each module and then
		 * do an apply to it.
		 */

		first_mca_record = handel->module_array[m][0];

		if ( first_mca_record == NULL ) {
			continue;  /* Go back to the top of the loop. */
		}

		first_handel_mca = first_mca_record->record_type_struct;

		if ( first_handel_mca == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
			fname, "The MX_HANDEL_MCA pointer for Handel "
			"MCA '%s' is NULL.",
				first_mca_record->name );
		}

		if ( handel->debug_flag ) {
			MX_DEBUG(-2,("%s: applying to MCA '%s'",
				fname, first_mca_record->name ));
		}

		ignored = 0;

		MX_XIA_SYNC( xiaBoardOperation(
				first_handel_mca->detector_channel,
				"apply", (void *) &ignored ) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED,
			fname, "Cannot apply acquisition values for "
			"MCA '%s' on behalf of Handel record '%s'.  "
			"Error code = %d, '%s'",
				first_mca_record->name,
				handel->record->name,
				xia_status,
				mxi_handel_strerror( xia_status ) );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_read_parameter( MX_MCA *mca,
			char *parameter_name,
			unsigned long *value_ptr )
{
	static const char fname[] = "mxi_handel_read_parameter()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	unsigned short short_value;
	int xia_status;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,
		("%s: Record '%s', channel %ld, parameter '%s'.",
			fname, mca->record->name,
			handel_mca->detector_channel, parameter_name));
	}

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	MX_XIA_SYNC( xiaGetParameter( handel_mca->detector_channel,
					parameter_name, &short_value ) );

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s', channel %ld, parameter '%s'",
		mca->record->name,
		handel_mca->detector_channel,
		parameter_name);
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot read DXP parameter '%s' for detector channel %ld "
		"of XIA DXP interface '%s'.  "
		"Error code = %d, '%s'",
			parameter_name, handel_mca->detector_channel,
			mca->record->name, xia_status,
			mxi_handel_strerror( xia_status ) );
	}

	*value_ptr = (unsigned long) short_value;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s: value = %lu", fname, *value_ptr));
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_write_parameter( MX_MCA *mca,
			char *parameter_name,
			unsigned long value )
{
	static const char fname[] = "mxi_handel_write_parameter()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	unsigned short short_value;
	int xia_status;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,
		("%s: Record '%s', channel %ld, parameter '%s', value = %lu.",
			fname, mca->record->name, handel_mca->detector_channel,
			parameter_name, (unsigned long) value));
	}

	/* Send the parameter value. */

	short_value = (unsigned short) value;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	MX_XIA_SYNC( xiaSetParameter( handel_mca->detector_channel,
					parameter_name, short_value ) );

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s', channel %ld, parameter '%s'",
		mca->record->name,
		handel_mca->detector_channel,
		parameter_name);
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot write the value %hu to DXP parameter '%s' "
		"for detector channel %ld of XIA DXP interface '%s'.  "
		"Error code = %d, '%s'", short_value,
			parameter_name, handel_mca->detector_channel,
			mca->record->name, xia_status,
			mxi_handel_strerror( xia_status ) );
	}

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,
		("%s: Write to parameter '%s' succeeded.",
			fname, parameter_name));
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_write_parameter_to_all_channels( MX_HANDEL *handel,
					char *parameter_name,
					unsigned long value )
{
	static const char fname[] =
		"mxi_handel_write_parameter_to_all_channels()";

	unsigned short short_value;
	int xia_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	if ( handel->debug_flag ) {
		MX_DEBUG(-2,("%s: Record '%s', parameter '%s', value = %lu.",
			fname, handel->record->name,
			parameter_name, (unsigned long) value));
	}

	short_value = (unsigned short) value;

#if 0
	/* I don't know yet whether there is a Handel function for this. */

#else
	/* A temporary kludge which does work. */

	{
		MX_RECORD *mca_record;
		MX_HANDEL_MCA *local_dxp_mca;
		unsigned long i;

		for ( i = 0; i < handel->num_mcas; i++ ) {

			mca_record = handel->mca_record_array[i];

			if ( mca_record == (MX_RECORD *) NULL ) {

				continue;    /* Skip this one. */
			}

			local_dxp_mca = (MX_HANDEL_MCA *)
					mca_record->record_type_struct;

			MX_DEBUG( 2,("%s: writing to detector channel %ld.",
				fname, local_dxp_mca->detector_channel));

#if MXI_HANDEL_DEBUG_TIMING
			MX_HRT_START( measurement );
#endif
			MX_XIA_SYNC( xiaSetParameter(
					local_dxp_mca->detector_channel,
					parameter_name, short_value ) );

#if MXI_HANDEL_DEBUG_TIMING
			MX_HRT_END( measurement );

			MX_HRT_RESULTS( measurement, fname,
				"for record '%s', channel %ld, parameter '%s'",
				mca_record->name,
				local_dxp_mca->detector_channel,
				parameter_name );
#endif

			if ( xia_status != XIA_SUCCESS ) {
				return mx_error( MXE_DEVICE_ACTION_FAILED,fname,
			"Cannot write the value %hu to DXP parameter '%s' "
			"for detector channel %ld of XIA DXP interface '%s'.  "
			"Error code = %d, '%s'", short_value,
				parameter_name, local_dxp_mca->detector_channel,
				handel->record->name, xia_status,
				mxi_handel_strerror( xia_status ) );
			}
		}
	}
#endif
	if ( handel->debug_flag ) {
		MX_DEBUG(-2,
		("%s: Write parameter '%s' to all channels succeeded.",
			fname, parameter_name));
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] = "mxi_handel_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_HANDEL_ACQUISITION_VALUE_NAME:
		case MXLV_HANDEL_CONFIG_FILENAME:
		case MXLV_HANDEL_MAPPING_MODE:
		case MXLV_HANDEL_MAPPING_PIXEL_NEXT:
		case MXLV_HANDEL_PARAMETER_NAME:
		case MXLV_HANDEL_PIXEL_ADVANCE_MODE:
		case MXLV_HANDEL_SAVE_FILENAME:
		case MXLV_HANDEL_SYNC_COUNT:
			record_field->process_function
					    = mxi_handel_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_reallocate_buffers( MX_HANDEL *handel,
				size_t buffer_size )
{
	static const char fname[] = "mxi_handel_reallocate_buffers()";

	MX_MCS *mcs_array[MXI_HANDEL_CHANNELS_PER_MODULE];
	MX_MCS *mcs = NULL;
	MX_HANDEL_MCS *handel_mcs = NULL;
	int channel;
	mx_status_type mx_status;

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HANDEL pointer passed was NULL." );
	}

	if (1) {
	    int i;

	    MX_DEBUG(-2,("%s: *** MARKER BEFORE ***",fname));

	    mx_status = mxi_handel_get_mcs_array( handel,
						handel->num_mcas,
						mcs_array );

	    MX_DEBUG(-2,("%s: *** MARKER AFTER ***",fname));

	    if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	    MX_DEBUG(-2,("%s: mcs_array = %p", fname, mcs_array));

	    for ( i = 0; i < handel->num_mcas; i++ ) {
		    MX_DEBUG(-2,("%s: mcs_array[%d] = %p",
			fname, i, mcs_array[i]));
	    }
	}

	for ( channel = 0;
		channel < MXI_HANDEL_CHANNELS_PER_MODULE;
		channel++ )
	{
		mcs = mcs_array[channel];

		handel_mcs = (MX_HANDEL_MCS *) mcs->record->record_type_struct;

		/* Free the old buffers. */

		mx_free( handel_mcs->buffer_a );
		mx_free( handel_mcs->buffer_b );

		/* Allocate the new buffers. */

		handel_mcs->buffer_a = malloc( handel_mcs->buffer_length
						* sizeof(long) );

		if ( handel_mcs->buffer_a == (uint32_t *) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a "
			"%lu element array of unsigned longs for "
			"'buffer_a' belonging to XIA MCS '%s'.",
				handel_mcs->buffer_length,
				mcs->record->name );
		}

		handel_mcs->buffer_b = malloc( handel_mcs->buffer_length
						* sizeof(long) );

		if ( handel_mcs->buffer_b == (uint32_t *) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a "
			"%lu element array of unsigned longs for "
			"'buffer_b' belonging to XIA MCS '%s'.",
				handel_mcs->buffer_length,
				mcs->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_start_run( MX_HANDEL *handel,
			mx_bool_type resume_run )
{
	static const char fname[] = "mxi_handel_start_run()";

	int m, xia_status;
	unsigned short resume_flag;
	MX_RECORD *first_mca_record;
	MX_HANDEL_MCA *first_handel_mca;

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HANDEL pointer passed was NULL." );
	}

	if ( handel->debug_flag ) {
		MX_DEBUG(-2,("%s for Handel interface '%s', resume_run = %d",
			fname, handel->record->name, (int) resume_run));
	}

	/* Make sure that resume_flag can only have the values 0 or 1. */

	if ( resume_run ) {
		resume_flag = 1;
	} else {
		resume_flag = 0;
	}

	/* Walk through the modules to start the run. */

	for ( m = 0; m < handel->num_modules; m++ ) {

		first_mca_record = handel->module_array[m][0];

		if ( first_mca_record == (MX_RECORD *) NULL ) {
			/* Skip modules that are not installed. */

			continue;
		}

		first_handel_mca = first_mca_record->record_type_struct;

		if ( first_handel_mca == (MX_HANDEL_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The MX_HANDEL_MCA pointer for Handel MCA '%s' is NULL.",
				first_mca_record->name );
		}

		if ( handel->debug_flag ) {
			MX_DEBUG(-2,("%s: starting MCA '%s'.",
				fname, first_mca_record->name ));
		}

		MX_XIA_SYNC( xiaStartRun( first_handel_mca->detector_channel,
							resume_flag ) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot start a run for MCA '%s' on behalf of "
			"Handel interface '%s'.  "
			"Error code = %d, '%s'",
				first_mca_record->name,
				handel->record->name,
				xia_status,
				mxi_handel_strerror( xia_status ) );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_stop_run( MX_HANDEL *handel )
{
	static const char fname[] = "mxi_handel_stop_run()";

	int m, xia_status;
	MX_RECORD *first_mca_record;
	MX_HANDEL_MCA *first_handel_mca;

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HANDEL pointer passed was NULL." );
	}

	if ( handel->debug_flag ) {
		MX_DEBUG(-2,("%s for Handel interface '%s'",
			fname, handel->record->name ));
	}

	/* Walk through the modules to stop the run. */

	for ( m = 0; m < handel->num_modules; m++ ) {

		first_mca_record = handel->module_array[m][0];

		if ( first_mca_record == (MX_RECORD *) NULL ) {
			/* Skip modules that are not installed. */

			continue;
		}

		first_handel_mca = first_mca_record->record_type_struct;

		if ( first_handel_mca == (MX_HANDEL_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The MX_HANDEL_MCA pointer for Handel MCA '%s' is NULL.",
				first_mca_record->name );
		}

		if ( handel->debug_flag ) {
			MX_DEBUG(-2,("%s: stopping MCA '%s'.",
				fname, first_mca_record->name ));
		}

		MX_XIA_SYNC( xiaStopRun( first_handel_mca->detector_channel ) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot stop a run for MCA '%s' on behalf of "
			"Handel interface '%s'.  "
			"Error code = %d, '%s'",
				first_mca_record->name,
				handel->record->name,
				xia_status,
				mxi_handel_strerror( xia_status ) );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_is_busy( MX_HANDEL *handel,
			mx_bool_type *busy )
{
	static const char fname[] = "mxi_handel_is_busy()";

	int i, xia_status;
	MX_RECORD *mca_record;
	MX_HANDEL_MCA *handel_mca;
	unsigned long run_active;

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HANDEL pointer passed was NULL." );
	}
	if ( busy == (mx_bool_type *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The busy pointer passed was NULL." );
	}

	*busy = FALSE;

	if ( handel->debug_flag ) {
		MX_DEBUG(-2,("%s for Handel interface '%s'",
			fname, handel->record->name ));
	}

	/* Walk through the mca_channels to see if they are busy. */

	for ( i = 0; i < handel->num_mcas; i++ ) {

		mca_record = handel->mca_record_array[i];

		if ( mca_record == (MX_RECORD *) NULL ) {
			/* Skip MCAs that are not installed. */

			continue;
		}

		handel_mca = mca_record->record_type_struct;

		if ( handel_mca == (MX_HANDEL_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The MX_HANDEL_MCA pointer for Handel MCA '%s' is NULL.",
				mca_record->name );
		}

		if ( handel->debug_flag ) {
			MX_DEBUG(-2,("%s: checking run_active for MCA '%s'.",
				fname, mca_record->name ));
		}

		MX_XIA_SYNC( xiaGetRunData( handel_mca->detector_channel,
						"run_active", &run_active ) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot get 'run_active' for MCA '%s' on behalf of "
			"Handel interface '%s'.  "
			"Error code = %d, '%s'",
				mca_record->name,
				handel->record->name,
				xia_status,
				mxi_handel_strerror( xia_status ) );
		}

		if ( run_active & XIA_RUN_HARDWARE ) {
			/* If any of the channels are busy, then we consider
			 * the Handel system as a whole to be busy, so there
			 * is no need to check the state of the channels
			 * after this one.
			 */

			*busy = TRUE;

			if ( handel->debug_flag ) {
				MX_DEBUG(-2,
				("%s for Handel interface '%s', busy = %d",
				fname, handel->record->name, *busy ));
			}

			return MX_SUCCESSFUL_RESULT;
		}
	}

	if ( handel->debug_flag ) {
		MX_DEBUG(-2,("%s for Handel interface '%s', busy = %d",
			fname, handel->record->name, *busy ));
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_set_preset( MX_HANDEL *handel,
			double preset_type,
			double preset_value )
{
	static const char fname[] = "mxi_handel_set_preset()";

	int m, xia_status;
	MX_RECORD *first_mca_record;
	MX_HANDEL_MCA *first_handel_mca;

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HANDEL pointer passed was NULL." );
	}

	if ( handel->debug_flag ) {
		MX_DEBUG(-2,
		("%s for Handel interface '%s', "
		"preset_type = %g, preset_value = %g",
			fname, handel->record->name,
			preset_type, preset_value ));
	}

	/* Walk through the modules to set the preset in each one. */

	for ( m = 0; m < handel->num_modules; m++ ) {

		first_mca_record = handel->module_array[m][0];

		if ( first_mca_record == (MX_RECORD *) NULL ) {
			/* Skip modules that are not installed. */

			continue;
		}

		first_handel_mca = first_mca_record->record_type_struct;

		if ( first_handel_mca == (MX_HANDEL_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The MX_HANDEL_MCA pointer for Handel MCA '%s' is NULL.",
				first_mca_record->name );
		}

		if ( handel->debug_flag ) {
			MX_DEBUG(-2,("%s: setting the preset for MCA '%s'.",
				fname, first_mca_record->name ));
		}

		/* First set the preset type. */

		MX_XIA_SYNC( xiaSetAcquisitionValues(
				first_handel_mca->detector_channel,
				"preset_type", &preset_type ) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot set the preset type for MCA '%s' on behalf of "
			"Handel interface '%s'.  "
			"Error code = %d, '%s'",
				first_mca_record->name,
				handel->record->name,
				xia_status,
				mxi_handel_strerror( xia_status ) );
		}

		/* Then set the preset value. */

		MX_XIA_SYNC( xiaSetAcquisitionValues(
				first_handel_mca->detector_channel,
				"preset_value", &preset_value ) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot set the preset value for MCA '%s' on behalf of "
			"Handel interface '%s'.  "
			"Error code = %d, '%s'",
				first_mca_record->name,
				handel->record->name,
				xia_status,
				mxi_handel_strerror( xia_status ) );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_show_parameter( MX_HANDEL *handel )
{
	static const char fname[] = "mxi_handel_show_parameter()";

	MX_RECORD *mca_record;
	MX_HANDEL_MCA *handel_mca;
	unsigned long i;
	unsigned short parameter_value;
	int xia_status;

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HANDEL pointer passed was NULL." );
	}

	mx_info( "-------------------------------------------------------" );
	mx_info( "Parameter '%s' values:", handel->parameter_name );
	mx_info( " " );

	for ( i = 0; i < handel->num_mcas; i++ ) {
		mca_record = handel->mca_record_array[i];

		if ( mca_record == (MX_RECORD *) NULL )
			continue;

		handel_mca = (MX_HANDEL_MCA *) mca_record->record_type_struct;

		if ( handel_mca == (MX_HANDEL_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_HANDEL_MCA pointer for MCA '%s' is NULL.",
				mca_record->name );
		}

		MX_XIA_SYNC( xiaGetParameter( handel_mca->detector_channel,
						handel->parameter_name,
						&parameter_value ) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot get the value of parameter '%s' for MCA '%s'.  "
			"Error code = %d, '%s'",
				handel->parameter_name,
				mca_record->name,
				xia_status,
				mxi_handel_strerror( xia_status ) );
		}

		mx_info( "  '%s' = %hu", mca_record->name, parameter_value );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_show_acquisition_value( MX_HANDEL *handel )
{
	static const char fname[] = "mxi_handel_show_acquisition_value()";

	MX_RECORD *mca_record;
	MX_HANDEL_MCA *handel_mca;
	unsigned long i;
	double acquisition_value;
	int xia_status;

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HANDEL pointer passed was NULL." );
	}

	mx_info( "-------------------------------------------------------" );
	mx_info( "Parameter '%s' values:", handel->parameter_name );
	mx_info( " " );

	for ( i = 0; i < handel->num_mcas; i++ ) {
		mca_record = handel->mca_record_array[i];

		if ( mca_record == (MX_RECORD *) NULL )
			continue;

		handel_mca = (MX_HANDEL_MCA *) mca_record->record_type_struct;

		if ( handel_mca == (MX_HANDEL_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_HANDEL_MCA pointer for MCA '%s' is NULL.",
				mca_record->name );
		}

		MX_XIA_SYNC( xiaGetAcquisitionValues(
					handel_mca->detector_channel,
					handel->acquisition_value_name,
					(void *) &acquisition_value ) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot get acquisition value '%s' for MCA '%s'.  "
			"Error code = %d, '%s'",
				handel->parameter_name,
				mca_record->name,
				xia_status,
				mxi_handel_strerror( xia_status ) );
		}

		mx_info( "  '%s' = %g", mca_record->name, acquisition_value );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_get_acq_value_as_long( MX_HANDEL *handel,
				char *value_name,
				long *acq_value,
				mx_bool_type apply_flag )
{
	static const char fname[] = "mxi_handel_get_acq_value_as_long()";

	double acq_value_as_double;
	int xia_status;
	mx_status_type mx_status;

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HANDEL pointer passed was NULL." );
	}

	MX_XIA_SYNC( xiaGetAcquisitionValues( -1, value_name,
						&acq_value_as_double ) );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Cannot get acquisition value '%s' for Handel record '%s'.  "
		"Error code = %d, '%s'",
			value_name,
			handel->record->name,
			xia_status,
			mxi_handel_strerror( xia_status ) );
	}

	if ( apply_flag ) {
		mx_status = mxi_handel_apply_to_all_channels( handel );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( acq_value != (long *) NULL ) {
		*acq_value = mx_round( acq_value_as_double );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_set_acq_value_as_long( MX_HANDEL *handel,
				char *value_name,
				long acq_value,
				mx_bool_type apply_flag )
{
	static const char fname[] = "mxi_handel_set_acq_value_as_long()";

	double acq_value_as_double;
	int xia_status;
	mx_status_type mx_status;

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HANDEL pointer passed was NULL." );
	}

	acq_value_as_double = acq_value;

	MX_XIA_SYNC( xiaSetAcquisitionValues( -1, value_name,
						&acq_value_as_double ) );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Cannot set acquisition value '%s' for Handel record '%s'.  "
		"Error code = %d, '%s'",
			value_name,
			handel->record->name,
			xia_status,
			mxi_handel_strerror( xia_status ) );
	}

	if ( apply_flag ) {
		mx_status = mxi_handel_apply_to_all_channels( handel );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

const char *
mxi_handel_strerror( int handel_status )
{
	static struct {
		int handel_error_code;
		const char handel_error_message[140];
	} error_code_table[] = {
		{   0, "XIA_SUCCESS" },
		{   1, "XIA_OPEN_FILE" },
		{   2, "XIA_FILEERR" },
		{   3, "XIA_NOSECTION" },
		{   4, "XIA_FORMAT_ERROR" },
		{   5, "XIA_ILLEGAL_OPERATION - "
			"Attempted to configure options in an illegal order" },
		{   6, "XIA_FILE_RA - "
			"File random access unable to find name-value pair" },
		{ 201, "XIA_UNKNOWN_DECIMATION - "
	"The decimation read from the hardware does not match a known value" },
		{ 202, "XIA_SLOWLEN_OOR - "
			"Calculated SLOWLEN value is out-of-range" },
		{ 203, "XIA_SLOWGAP_OOR - "
			"Calculated SLOWGAP value is out-of-range" },
		{ 204, "XIA_SLOWFILTER_OOR - "
			"Attempt to set the Peaking or Gap time s.t. P+G>31" },
		{ 205, "XIA_FASTLEN_OOR - "
			"Calculated FASTLEN value is out-of-range" },
		{ 206, "XIA_FASTGAP_OOR - "
			"Calculated FASTGAP value is out-of-range" },
		{ 207, "XIA_FASTFILTER_OOR - "
			"Attempt to set the Peaking or Gap time s.t. P+G>31" },
		{ 301, "XIA_INITIALIZE" },
		{ 302, "XIA_NO_ALIAS" },
		{ 303, "XIA_ALIAS_EXISTS" },
		{ 304, "XIA_BAD_VALUE" },
		{ 305, "XIA_INFINITE_LOOP" },
		{ 306, "XIA_BAD_NAME - Specified name is not valid" },
		{ 307, "XIA_BAD_PTRR - "
			"Specified PTRR is not valid for this alias" },
		{ 308, "XIA_ALIAS_SIZE - Alias name has too many characters" },
		{ 309, "XIA_NO_MODULE - "
			"Must define at least one module before" },
		{ 310, "XIA_BAD_INTERFACE - "
			"The specified interface does not exist" },
		{ 311, "XIA_NO_INTERFACE - "
	"An interface must be defined before more information is specified" },
		{ 312, "XIA_WRONG_INTERFACE - "
	"Specified information doesn't apply to this interface" },
		{ 313, "XIA_NO_CHANNELS - "
			"Number of channels for this module is set to 0" },
		{ 314, "XIA_BAD_CHANNEL - "
			"Specified channel index is invalid or out-of-range" },
		{ 315, "XIA_NO_MODIFY - "
			"Specified name cannot be modified once set" },
		{ 316, "XIA_INVALID_DETCHAN - "
			"Specified detChan value is invalid" },
		{ 317, "XIA_BAD_TYPE - "
			"The DetChanElement type specified is invalid" },
		{ 318, "XIA_WRONG_TYPE - "
		"This routine only operates on detChans that are sets" },
		{ 319, "XIA_UNKNOWN_BOARD - Board type is unknown" },
		{ 320, "XIA_NO_DETCHANS - No detChans are currently defined" },
		{ 321, "XIA_NOT_FOUND - "
			"Unable to locate the Acquisition value requested" },
		{ 322, "XIA_PTR_CHECK - "
			"Pointer is out of synch when it should be valid" },
		{ 323, "XIA_LOOKING_PTRR - "
	"FirmwareSet has a FDD file defined and this only works with PTRRs" },
		{ 324, "XIA_NO_FILENAME - "
			"Requested filename information is set to NULL" },
		{ 325, "XIA_BAD_INDEX - "
			"User specified an alias index that doesn't exist" },
		{ 350, "XIA_FIRM_BOTH - "
"A FirmwareSet may not contain both an FDD and seperate Firmware definitions" },
		{ 351, "XIA_PTR_OVERLAP - "
	"Peaking time ranges in the Firmware definitions may not overlap" },
		{ 352, "XIA_MISSING_FIRM - "
	"Either the FiPPI or DSP file is missing from a Firmware element" },
		{ 353, "XIA_MISSING_POL - "
	"A polarity value is missing from a Detector element" },
		{ 354, "XIA_MISSING_GAIN - "
	"A gain value is missing from a Detector element" },
		{ 355, "XIA_MISSING_INTERFACE - "
	"The interface this channel requires is missing" },
		{ 356, "XIA_MISSING_ADDRESS - "
	"The epp_address information is missing for this channel" },
		{ 357, "XIA_INVALID_NUMCHANS - "
	"The wrong number of channels are assigned to this module" },
		{ 358, "XIA_INCOMPLETE_DEFAULTS - "
	"Some of the required defaults are missing" },
		{ 359, "XIA_BINS_OOR - "
	"There are too many or too few bins for this module type" },
		{ 360, "XIA_MISSING_TYPE - "
	"The type for the current detector is not specified properly" },
		{ 361, "XIA_NO_MMU - "
	"No MMU defined and/or required for this module" },
		{ 401, "XIA_NOMEM - Unable to allocate memory" },
		{ 402, "XIA_XERXES - XerXes returned an error" },
		{ 403, "XIA_MD - MD layer returned an error" },
		{ 404, "XIA_EOF - EOF encountered" },
		{ 405, "XIA_XERXES_NORMAL_RUN_ACTIVE - "
			"XerXes says a normal run is still active" },
		{ 406, "XIA_HARDWARE_RUN_ACTIVE - "
			"The hardware says a control run is still active" },
		{ 501, "XIA_UNKNOWN" },
		{ 502, "XIA_LOG_LEVEL - Log level invalid" },
		{ 503, "XIA_NO_LIST - List size is zero" },
		{ 504, "XIA_NO_ELEM - No data to remove" },
		{ 505, "XIA_DATA_DUP - Data already in table" },
		{ 506, "XIA_REM_ERR - Unable to remove entry from hash table" },
		{ 507, "XIA_FILE_TYPE - Improper file type specified" },
		{ 508, "XIA_END - "
	"There are no more instances of the name specified. Pos set to end" },
		{ 601, "XIA_NOSUPPORT_FIRM - "
	"The specified firmware is not supported by this board type" },
		{ 602, "XIA_UNKNOWN_FIRM - "
			"The specified firmware type is unknown" },
		{ 603, "XIA_NOSUPPORT_VALUE - "
			"The specified acquisition value is not supported" },
		{ 604, "XIA_UNKNOWN_VALUE - "
			"The specified acquisition value is unknown" },
		{ 605, "XIA_PEAKINGTIME_OOR - "
	"The specified peaking time is out-of-range for this product" },
		{ 606, "XIA_NODEFINE_PTRR - "
	"The specified peaking time does not have a PTRR associated with it" },
		{ 607, "XIA_THRESH_OOR - "
			"The specified treshold is out-of-range" },
		{ 608, "XIA_ERROR_CACHE - "
			"The data in the values cache is out-of-sync" },
		{ 609, "XIA_GAIN_OOR - "
			"The specified gain is out-of-range for this product" },
		{ 610, "XIA_TIMEOUT - Timeout waiting for BUSY" },
		{ 611, "XIA_BAD_SPECIAL - "
	"The specified special run is not supported for this module" },
		{ 612, "XIA_TRACE_OOR - "
	"The specified value of tracewait (in ns) is out-of-range" },
		{ 613, "XIA_DEFAULTS - "
	"The PSL layer encountered an error creating a Defaults element" },
		{ 614, "XIA_BAD_FILTER - Error loading filter info "
	"from either a FDD file or the Firmware configuration" },
		{ 615, "XIA_NO_REMOVE - Specified acquisition value "
	"is required for this product and can't be removed" },
		{ 616, "XIA_NO_GAIN_FOUND - "
	"Handel was unable to converge on a stable gain value" },
		{ 617, "XIA_UNDEFINED_RUN_TYPE - "
	"Handel does not recognize this run type" },
		{ 618, "XIA_INTERNAL_BUFFER_OVERRUN - "
	"Handel attempted to overrun an internal buffer boundry" }, 
		{ 619, "XIA_EVENT_BUFFER_OVERRUN - "
	"Handel attempted to overrun the event buffer boundry" }, 
		{ 620, "XIA_BAD_DATA_LENGTH - "
	"Handel was asked to set a Data length to zero for readout" },
		{ 621, "XIA_NO_LINEAR_FIT - "
	"Handel was unable to perform a linear fit to some data" },
		{ 622, "XIA_MISSING_PTRR - Required PTRR is missing" },
		{ 623, "XIA_PARSE_DSP - Error parsing DSP" },
		{ 624, "XIA_UDXPS", },
		{ 625, "XIA_BIN_WIDTH - Specified bin width is out-of-range" },
		{ 626, "XIA_NO_VGA - "
"An attempt was made to set the gaindac on a board that doesn't have a VGA" },
		{ 627, "XIA_TYPEVAL_OOR - "
	"Specified detector type value is out-of-range" },
		{ 628, "XIA_LOW_LIMIT_OOR - "
	"Specified low MCA limit is out-of-range" },
		{ 629, "XIA_BPB_OOR - bytes_per_bin is out-of-range" },
		{ 630, "XIA_FIP_OOR - Specified FiPPI is out-fo-range" },
		{ 701, "XIA_XUP_VERSION - XUP version is not supported" },
		{ 702, "XIA_CHKSUM - checksum mismatch in the XUP" },
		{ 703, "XIA_BAK_MISSING - Requested BAK file cannot be opened"},
		{ 704, "XIA_SIZE_MISMATCH - "
			"Size read from file is incorrect" },
		{ 1001,"XIA_DEBUG" }
	};

	static const char unrecognized_error_message[]
		= "Unrecognized Handel error code";

	static int num_error_codes = sizeof( error_code_table )
				/ sizeof( error_code_table[0] );

	unsigned int i;

	for ( i = 0; i < num_error_codes; i++ ) {

		if ( handel_status == error_code_table[i].handel_error_code )
			break;		/* Exit the for() loop. */
	}

	if ( i >= num_error_codes ) {
		return &unrecognized_error_message[0];
	}

	return &( error_code_table[i].handel_error_message[0] );
}

/*-------------------------------------------------------------------------*/

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif

static mx_status_type
mxi_handel_process_function( void *record_ptr,
			void *record_field_ptr,
			void *socket_handler_ptr,
			int operation )
{
	static const char fname[] = "mxi_handel_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_HANDEL *handel;
	int xia_status, ignored;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	handel = (MX_HANDEL *) (record->record_type_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_HANDEL_CONFIG_FILENAME:
			/* Nothing to do since the necessary filename is
			 * already stored in the 'config_filename' field.
			 */

			break;
		case MXLV_HANDEL_SAVE_FILENAME:
			/* Nothing to do since the necessary filename is
			 * already stored in the 'save_filename' field.
			 */

			break;

		/* Note: The following are commented out since they appear to
		 * be write-only variables.  Trying to read from them returns
		 * an XIA_BAD_YPTE error saying that they are invalid.
		 */
#if 0
		case MXLV_HANDEL_MAPPING_MODE:
			mx_status = mxi_handel_get_acq_value_as_long( handel,
							"mapping_mode",
							&(handel->mapping_mode),
							TRUE );
			break;
		case MXLV_HANDEL_PIXEL_ADVANCE_MODE:
			mx_status = mxi_handel_get_acq_value_as_long( handel,
							"pixel_advance_mode",
						&(handel->pixel_advance_mode),
							TRUE );
			break;
		case MXLV_HANDEL_SYNC_COUNT:
			mx_status = mxi_handel_get_acq_value_as_long( handel,
							"sync_count",
							&(handel->sync_count),
							TRUE );
			break;
#endif
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_HANDEL_CONFIG_FILENAME:
			mx_status = mxi_handel_load_new_config( handel );

			break;
		case MXLV_HANDEL_SAVE_FILENAME:
			mx_status = mxi_handel_save_config( handel );

			break;
		case MXLV_HANDEL_PARAMETER_NAME:
			mx_status = mxi_handel_show_parameter( handel );
			break;
		case MXLV_HANDEL_ACQUISITION_VALUE_NAME:
			mx_status = mxi_handel_show_acquisition_value(handel);
			break;
		case MXLV_HANDEL_MAPPING_MODE:
			mx_status = mxi_handel_set_acq_value_as_long( handel,
							"mapping_mode",
							handel->mapping_mode,
							TRUE );
			break;
		case MXLV_HANDEL_PIXEL_ADVANCE_MODE:
			mx_status = mxi_handel_set_acq_value_as_long( handel,
							"pixel_advance_mode",
						handel->pixel_advance_mode,
							TRUE );
			break;
		case MXLV_HANDEL_SYNC_COUNT:
			mx_status = mxi_handel_set_acq_value_as_long( handel,
							"sync_count",
							handel->sync_count,
							TRUE );
			break;
		case MXLV_HANDEL_MAPPING_PIXEL_NEXT:

#if MXI_HANDEL_DEBUG_MAPPING_PIXEL_NEXT
			MX_DEBUG(-2,
			("%s: WRITING TO 'mapping_pixel_next' for record '%s'.",
				fname, handel->record->name ));
#endif
			ignored = 0;

			MX_XIA_SYNC( xiaBoardOperation(
				0, "mapping_pixel_next", (void *) &ignored ) );

			if ( xia_status != XIA_SUCCESS ) {
				mx_status = mx_error( MXE_DEVICE_ACTION_FAILED,
				"Writing to 'mapping_pixel_next' for XIA "
				"Handel controller '%s' failed.  "
				"Error code = %d, '%s'.",
					record->name,
					xia_status,
					mxi_handel_strerror(xia_status) );
			}
			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
		break;
	}

	return mx_status;
}

