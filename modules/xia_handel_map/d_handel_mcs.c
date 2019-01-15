/*
 * Name:    d_handel_mcs.c 
 *
 * Purpose: MX multichannel scaler driver for XIA Handel devices used
 *          in mapping mode.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_HANDEL_MCS_DEBUG_MONITOR_THREAD		TRUE

#define MXD_HANDEL_MCS_DEBUG_MONITOR_THREAD_BUFFERS	TRUE

#define MXD_HANDEL_MCS_DEBUG_BUSY			TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_mutex.h"
#include "mx_thread.h"
#include "mx_mca.h"
#include "mx_mcs.h"

#include <handel.h>
#include <handel_constants.h>
#include <handel_errors.h>
#include <handel_generic.h>

#include "i_handel.h"
#include "d_handel_mca.h"
#include "d_handel_mcs.h"

/* Initialize the mcs driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_handel_mcs_record_function_list = {
	mxd_handel_mcs_initialize_driver,
	mxd_handel_mcs_create_record_structures,
	mxd_handel_mcs_finish_record_initialization,
	NULL,
	NULL,
	mxd_handel_mcs_open
};

MX_MCS_FUNCTION_LIST mxd_handel_mcs_mcs_function_list = {
	mxd_handel_mcs_arm,
	mxd_handel_mcs_trigger,
	mxd_handel_mcs_stop,
	mxd_handel_mcs_clear,
	mxd_handel_mcs_busy,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_handel_mcs_get_parameter,
	mxd_handel_mcs_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_handel_mcs_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCS_STANDARD_FIELDS,
	MXD_HANDEL_MCS_STANDARD_FIELDS
};

long mxd_handel_mcs_num_record_fields
		= sizeof( mxd_handel_mcs_record_field_defaults )
			/ sizeof( mxd_handel_mcs_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_handel_mcs_rfield_def_ptr
			= &mxd_handel_mcs_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_handel_mcs_get_pointers( MX_MCS *mcs,
			MX_HANDEL_MCS **handel_mcs,
			MX_MCA **mca,
			MX_HANDEL_MCA **handel_mca,
			MX_HANDEL **handel,
			const char *calling_fname )
{
	static const char fname[] = "mxd_handel_mcs_get_pointers()";

	MX_RECORD *mcs_record = NULL;
	MX_HANDEL_MCS *handel_mcs_ptr = NULL;

	MX_RECORD *mca_record = NULL;
	MX_HANDEL_MCA *handel_mca_ptr = NULL;

	MX_RECORD *handel_record = NULL;

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	mcs_record = mcs->record;

	if ( mcs_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MCS pointer passed by '%s' is NULL.",
			calling_fname );
	}

	handel_mcs_ptr = (MX_HANDEL_MCS *) mcs_record->record_type_struct;

	if ( handel_mcs_ptr == (MX_HANDEL_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HANDEL_MCS pointer for MCS record '%s' is NULL.",
			mcs_record->name );
	}

	if ( handel_mcs != (MX_HANDEL_MCS **) NULL ) {
		*handel_mcs = handel_mcs_ptr;
	}

	mca_record = handel_mcs_ptr->mca_record;

	if ( mca_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The mca_record pointer for Handel MCS '%s' is NULL.",
			mcs_record->name );
	}

	if ( mca != (MX_MCA **) NULL ) {
		*mca = (MX_MCA *) mca_record->record_class_struct;

		if ( (*mca) == (MX_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MCA pointer for MCA '%s' used by "
			"Handel MCS '%s' is NULL.",
			mca_record->name, mcs_record->name );
		}
	}

	handel_mca_ptr = (MX_HANDEL_MCA *) mca_record->record_type_struct;

	if ( handel_mca_ptr == (MX_HANDEL_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_HANDEL_MCA pointer for MCA '%s' used by "
			"Handel MCS '%s' is NULL.",
			mca_record->name, mcs_record->name );
	}

	if ( handel_mca != (MX_HANDEL_MCA **) NULL ) {
		*handel_mca = handel_mca_ptr;
	}

	if ( handel != (MX_HANDEL **) NULL ) {
		handel_record = handel_mca_ptr->handel_record;

		if ( handel_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The handel_record pointer for MCA '%s' used by "
			"Handel MCS '%s' is NULL.",
			mca_record->name, mcs_record->name );
		}

		*handel = (MX_HANDEL *) handel_record->record_type_struct;

		if ( (*handel) == (MX_HANDEL *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_HANDEL pointer for Handel record '%s' is NULL.",
				handel_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

/* For successful operation, MX support for XIA Handel mapping mode _MUST_
 * be able to keep up with the rate that the XIA hardware generates new
 * spectra ("pixels") to read out.  Thus, we have a dedicated monitor thread
 * to do that.
 *
 * Note that the monitor thread only lasts until all of the data has been
 * read out or until an error occurs.
 */

static mx_status_type
mxd_handel_mcs_monitor_thread_fn( MX_THREAD *thread, void *args )
{
	static const char fname[] = "mxd_handel_mcs_monitor_thread_fn()";

	MX_RECORD *mcs_record = NULL;
	MX_MCS *mcs = NULL;
	MX_HANDEL_MCS *handel_mcs = NULL;
	MX_MCA *mca = NULL;
	MX_HANDEL_MCA *handel_mca = NULL;
	MX_HANDEL *handel = NULL;
	unsigned long i, j;
	int xia_status;
	mx_status_type mx_status;

	char current_buffer = '\0';
	unsigned short buffer_full = 0;
	unsigned long wait_for_buffer_sleep_ms = 1000;	/* in milliseconds */

#if 0
	mx_breakpoint();
#endif

	if ( args == NULL ) {
		handel->monitor_thread = NULL;

		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The args pointer passed was NULL." );
	}

	mcs_record = (MX_RECORD *) args;

#if MXD_HANDEL_MCS_DEBUG_MONITOR_THREAD
	MX_DEBUG(-2,("%s invoked for MCS record '%s'.",
		fname, mcs_record->name ));
#endif

	mcs = (MX_MCS *) mcs_record->record_class_struct;

	mx_status = mxd_handel_mcs_get_pointers( mcs, &handel_mcs,
						&mca, &handel_mca,
						&handel, fname );

	if ( mx_status.code != MXE_SUCCESS ) {
		handel->monitor_thread = NULL;
		return mx_status;
	}

	j = 0;		/* j is the measurement number (starting at 0) */

	while (TRUE) {
	    if ( j >= mcs->current_num_measurements ) {
		    break;	/* Exit the outer while() loop. */
	    }

	    /********* Wait for 'buffer_a' to be full. *********/

#if MXD_HANDEL_MCS_DEBUG_MONITOR_THREAD_BUFFERS
	    MX_DEBUG(-2,("%s: j = %lu, waiting for buffer_a", fname, j));
#endif

	    buffer_full = 0;

	    while ( buffer_full == 0 ) {
		MX_XIA_SYNC( xiaGetRunData(0, "buffer_full_a", &buffer_full ) );

		if ( xia_status != XIA_SUCCESS ) {
			handel->monitor_thread = NULL;

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to check the status of XIA buffer_a "
			"failed for MCS '%s'.  Error code = %d, '%s'.",
				mcs_record->name,
				xia_status, mxi_handel_strerror(xia_status) );
		}

		mx_msleep( wait_for_buffer_sleep_ms );
	    }

#if MXD_HANDEL_MCS_DEBUG_MONITOR_THREAD_BUFFERS
	    MX_DEBUG(-2,("%s: j = %lu, buffer_a available.", fname, j));
#endif

	    mx_mutex_lock( handel->mutex );

	    xia_status = xiaGetRunData( 0, "buffer_a", handel_mcs->buffer_a );

	    if ( xia_status != XIA_SUCCESS ) {
		mx_mutex_unlock( handel->mutex );

		handel->monitor_thread = NULL;

	    	return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to read XIA 'buffer_a' for MCS '%s' failed.  "
		"Error code = %d, '%s'.",
			mcs_record->name,
			xia_status, mxi_handel_strerror(xia_status) );
	    }

	    /* Copy 'buffer_a' to the appropriate MCS buffer. */

	    for ( i = 0; i < mcs->current_num_scalers; i++ ) {
		mcs->data_array[i][j] = handel_mcs->buffer_a[i];
	    }

	    mx_mutex_unlock( handel->mutex );

#if MXD_HANDEL_MCS_DEBUG_MONITOR_THREAD_BUFFERS
	    MX_DEBUG(-2,("%s: j = %lu, buffer_a copied.", fname, j));
#endif

	    /* Notify the XIA hardware that the buffer has been read. */

	    current_buffer = 'a';

	    MX_XIA_SYNC( xiaBoardOperation(0, "buffer_done", &current_buffer) );

	    if ( xia_status != XIA_SUCCESS ) {
		handel->monitor_thread = NULL;

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to signal completion of the 'buffer_a' read "
		"failed for MCS '%s'.  Error code = %d, '%s'.",
			mcs_record->name,
			xia_status, mxi_handel_strerror(xia_status) );
	    }

#if MXD_HANDEL_MCS_DEBUG_MONITOR_THREAD
	    MX_DEBUG(-2,("%s: MCS '%s' measurement %ld complete.",
		fname, mcs_record->name, j ));
#endif

	    j++;

	    if ( j >= mcs->current_num_measurements ) {
		    break;	/* Exit the outer while() loop. */
	    }

	    /********* Wait for 'buffer_b' to be full. *********/

#if MXD_HANDEL_MCS_DEBUG_MONITOR_THREAD_BUFFERS
	    MX_DEBUG(-2,("%s: j = %lu, waiting for buffer_b", fname, j));
#endif

	    buffer_full = 0;

	    while ( buffer_full == 0 ) {
		MX_XIA_SYNC( xiaGetRunData(0, "buffer_full_b", &buffer_full ) );

		if ( xia_status != XIA_SUCCESS ) {
			handel->monitor_thread = NULL;

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to check the status of XIA buffer_b "
			"failed for MCS '%s'.  Error code = %d, '%s'.",
				mcs_record->name,
				xia_status, mxi_handel_strerror(xia_status) );
		}

		mx_msleep( wait_for_buffer_sleep_ms );
	    }

#if MXD_HANDEL_MCS_DEBUG_MONITOR_THREAD_BUFFERS
	    MX_DEBUG(-2,("%s: j = %lu, buffer_b available.", fname, j));
#endif

	    mx_mutex_lock( handel->mutex );

	    xia_status = xiaGetRunData( 0, "buffer_b", handel_mcs->buffer_b );

	    if ( xia_status != XIA_SUCCESS ) {
		mx_mutex_unlock( handel->mutex );

		handel->monitor_thread = NULL;

	    	return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to read XIA 'buffer_b' for MCS '%s' failed.  "
		"Error code = %d, '%s'.",
			mcs_record->name,
			xia_status, mxi_handel_strerror(xia_status) );
	    }

	    /* Copy 'buffer_b' to the appropriate MCS buffer. */

	    for ( i = 0; i < mcs->current_num_scalers; i++ ) {
		mcs->data_array[i][j] = handel_mcs->buffer_b[i];
	    }

	    mx_mutex_unlock( handel->mutex );

#if MXD_HANDEL_MCS_DEBUG_MONITOR_THREAD_BUFFERS
	    MX_DEBUG(-2,("%s: j = %lu, buffer_b copied.", fname, j));
#endif

	    /* Notify the XIA hardware that the buffer has been read. */

	    current_buffer = 'b';

	    MX_XIA_SYNC( xiaBoardOperation(0, "buffer_done", &current_buffer) );

	    if ( xia_status != XIA_SUCCESS ) {
		handel->monitor_thread = NULL;

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to signal completion of the 'buffer_a' read "
		"failed for MCS '%s'.  Error code = %d, '%s'.",
			mcs_record->name,
			xia_status, mxi_handel_strerror(xia_status) );
	    }

#if MXD_HANDEL_MCS_DEBUG_MONITOR_THREAD
	    MX_DEBUG(-2,("%s: MCS '%s' measurement %ld complete.",
		fname, mcs_record->name, j ));
#endif

	    j++;
	}

	(void) mxd_handel_mcs_stop( mcs );

#if MXD_HANDEL_MCS_DEBUG_MONITOR_THREAD
	MX_DEBUG(-2,("%s complete for MCS record '%s'.",
		fname, mcs_record->name ));
#endif
	handel->monitor_thread = NULL;

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_handel_mcs_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_scalers_varargs_cookie;
	long maximum_num_measurements_varargs_cookie;
	mx_status_type status;

	status = mx_mcs_initialize_driver( driver,
				&maximum_num_scalers_varargs_cookie,
				&maximum_num_measurements_varargs_cookie );

	return status;
}

MX_EXPORT mx_status_type
mxd_handel_mcs_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_handel_mcs_create_record_structures()";

	MX_MCS *mcs;
	MX_HANDEL_MCS *handel_mcs = NULL;

	/* Allocate memory for the necessary structures. */

	mcs = (MX_MCS *) malloc( sizeof(MX_MCS) );

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCS structure." );
	}

	handel_mcs = (MX_HANDEL_MCS *) malloc( sizeof(MX_HANDEL_MCS) );

	if ( handel_mcs == (MX_HANDEL_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_HANDEL_MCS structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mcs;
	record->record_type_struct = handel_mcs;
	record->class_specific_function_list =
		&mxd_handel_mcs_mcs_function_list;

	mcs->record = record;

	handel_mcs->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mcs_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_handel_mcs_finish_record_initialization()";

	MX_MCS *mcs;
	mx_status_type mx_status;

	mx_status = mx_mcs_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mcs = (MX_MCS *) (record->record_class_struct);

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCS pointer for MCS record '%s' is NULL.",
			record->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mcs_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_handel_mcs_open()";

	MX_MCS *mcs = NULL;
	MX_HANDEL_MCS *handel_mcs = NULL;
	MX_MCA *mca = NULL;
	MX_HANDEL_MCA *handel_mca = NULL;
	MX_HANDEL *handel = NULL;
	double num_map_pixels, num_map_pixels_per_buffer;
	int xia_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mcs = (MX_MCS *) (record->record_class_struct);

	mx_status = mxd_handel_mcs_get_pointers( mcs, &handel_mcs,
					&mca, &handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: MCA = '%s'", fname, mca->record->name));

	/* Most of the work was already done by the MCA record that we
	 * point to.  We just need to set up mapping-specific stuff here
	 * that an ordinary XIA MCA will not understand.
	 */

	handel_mca->has_mapping_firmware = TRUE;

	handel_mcs->buffer_length = 0;
	handel_mcs->buffer_a = NULL;
	handel_mcs->buffer_b = NULL;

	/* Initialize the MCS to 1 measurement. */

	mcs->current_num_measurements = 1;

	num_map_pixels = mcs->current_num_measurements;

	MX_XIA_SYNC( xiaSetAcquisitionValues( -1, "num_map_pixels",
						(void *) &num_map_pixels ) );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to set the number of measurements to 1 "
		"for MCS '%s' failed.  Error code = %d, '%s'",
		mcs->record->name,
		xia_status, mxi_handel_strerror(xia_status) );
	}

	/* Let handel decide the number of pixels per buffer. */

	num_map_pixels_per_buffer = -1.0;

	MX_XIA_SYNC( xiaSetAcquisitionValues( -1, "num_map_pixels_per_buffer",
					(void *) &num_map_pixels_per_buffer ) );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to set 'num_map_pixels_per_buffer' to -1 "
		"for MCS '%s' failed.  Error code = %d, '%s'",
		mcs->record->name,
		xia_status, mxi_handel_strerror(xia_status) );
	}

	/* Default to external trigger. */

	mcs->trigger_mode = MXF_DEV_EXTERNAL_TRIGGER;

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_handel_mcs_arm( MX_MCS *mcs )
{
	static const char fname[] = "mxd_handel_mcs_arm()";

	MX_HANDEL_MCS *handel_mcs = NULL;
	MX_MCA *mca = NULL;
	MX_HANDEL_MCA *handel_mca = NULL;
	MX_HANDEL *handel = NULL;
	double mapping_mode;
	double pixel_advance_mode, gate_master, sync_master, sync_count;
	unsigned long old_buffer_length;
	int xia_status;
	mx_status_type mx_status;

	mx_status = mxd_handel_mcs_get_pointers( mcs, &handel_mcs,
					&mca, &handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_handel_mcs_stop( mcs );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn on mapping mode. */

	switch( handel->mapping_mode ) {
	case MXF_HANDEL_MAP_MCA_MODE:
		mapping_mode = handel->mapping_mode;
		break;
	case MXF_HANDEL_MAP_NORMAL_MODE:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Normal mode (0) is not supported for XIA MCS '%s'.",
			mcs->record->name );
		break;
	case MXF_HANDEL_MAP_SCA_MODE:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"SCA mode (2) is not yet implemented for XIA MCS '%s'.",
			mcs->record->name );
		break;
	case MXF_HANDEL_MAP_LIST_MODE:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"LIST mode (3) is not yet implemented for XIA MCS '%s'.",
			mcs->record->name );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Mapping mode (%ld) is not supported for XIA MCS '%s'.",
			handel->mapping_mode, mcs->record->name );
		break;
	}

	MX_XIA_SYNC( xiaSetAcquisitionValues( -1,
				"mapping_mode", &mapping_mode ) );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to set 'mapping_mode' to %f "
		"for MCS '%s' failed.  Error code = %d, '%s'",
		mapping_mode, mcs->record->name,
		xia_status, mxi_handel_strerror(xia_status) );
	}

	/* Configure the hardware pixel_advance_mode based on the MX variable.*/

	switch( mcs->trigger_mode ) {
	case MXF_DEV_INTERNAL_TRIGGER:
		gate_master = 0.0;
		sync_master = 0.0;
		pixel_advance_mode = 0.0;
		sync_count = 0.0;
		break;
	case MXF_DEV_EXTERNAL_TRIGGER:
		switch( handel->pixel_advance_mode ) {
		case MXF_HANDEL_ADV_GATE_MODE:
			gate_master = 1.0;
			sync_master = 0.0;
			pixel_advance_mode = XIA_MAPPING_CTL_GATE;
			sync_count = 0.0;
			break;
		case MXF_HANDEL_ADV_SYNC_MODE:
			gate_master = 0.0;
			sync_master = 1.0;
			pixel_advance_mode = XIA_MAPPING_CTL_SYNC;
			sync_count = handel->sync_count;
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"pixel_advance_mode (%ld) is not supported for "
			"XIA MCS '%s' in external trigger mode.  "
			"The only supported pixel advance modes are "
			"GATE (1) and SYNC (2).",
				handel->pixel_advance_mode,
				mcs->record->name );
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"MCS trigger mode (%ld) is not supported for XIA MCS '%s'.  "
		"The only supported trigger modes are INTERNAL (1) "
		"and EXTERNAL (2).",
			mcs->trigger_mode, mcs->record->name );
		break;
	}

#if 1
	MX_DEBUG(-2,("%s: MCS '%s' trigger_mode = %ld, mapping_mode = %ld, "
		"handel->pixel_advance_mode = %ld, pixel_advance_mode = %f, "
		"gate_master = %f, sync_master = %f, sync_count = %f",
		fname, mcs->record->name, mcs->trigger_mode,
		handel->mapping_mode, handel->pixel_advance_mode,
		pixel_advance_mode, gate_master, sync_master, sync_count));
#endif

	MX_XIA_SYNC( xiaSetAcquisitionValues(0, "gate_master", &gate_master ));

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to set 'gate_master' to %f "
		"for MCS '%s' failed.  Error code = %d, '%s'",
		gate_master, mcs->record->name,
		xia_status, mxi_handel_strerror(xia_status) );
	}

	MX_XIA_SYNC( xiaSetAcquisitionValues(0, "sync_master", &sync_master ));

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to set 'sync_master' to %f "
		"for MCS '%s' failed.  Error code = %d, '%s'",
		sync_master, mcs->record->name,
		xia_status, mxi_handel_strerror(xia_status) );
	}

	if ( mcs->trigger_mode == MXF_DEV_EXTERNAL_TRIGGER ) {
	    switch( handel->pixel_advance_mode ) {
	    case MXF_HANDEL_ADV_GATE_MODE:
	    case MXF_HANDEL_ADV_SYNC_MODE:
		MX_XIA_SYNC( xiaSetAcquisitionValues(0, "pixel_advance_mode",
							&pixel_advance_mode ));

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to set 'pixel_advance_mode' to %f "
			"for MCS '%s' failed.  Error code = %d, '%s'",
			pixel_advance_mode, mcs->record->name,
			xia_status, mxi_handel_strerror(xia_status) );
		}
		break;
	    default:
		break;
	    }
	}

	MX_XIA_SYNC( xiaSetAcquisitionValues(0, "sync_count", &sync_count ));

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to set 'sync_count' to %f "
		"for MCS '%s' failed.  Error code = %d, '%s'",
		sync_count, mcs->record->name,
		xia_status, mxi_handel_strerror(xia_status) );
	}

	/* Apply the settings to all channels. */

	mx_status = mxi_handel_apply_to_all_channels( handel );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if we need to change the lengths of the Handel
	 * 'buffer_a' and 'buffer_b' arrays and reallocate them
	 * if necessary.
	 */

	old_buffer_length = handel_mcs->buffer_length;

	MX_XIA_SYNC( xiaGetRunData( 0,
				"buffer_len", &(handel_mcs->buffer_length) ) );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to get the 'buffer_len' run data "
		"for MCS '%s' failed.  Error code = %d, '%s'",
		mcs->record->name,
		xia_status, mxi_handel_strerror(xia_status) );
	}

	if ( old_buffer_length != handel_mcs->buffer_length ) {
		/* Free the old buffers. */

		mx_free( handel_mcs->buffer_a );
		mx_free( handel_mcs->buffer_b );

		/* Allocate the new buffers. */

		handel_mcs->buffer_a = malloc( handel_mcs->buffer_length
						* sizeof(long) );

		if ( handel_mcs->buffer_a == (uint16_t *) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a "
			"%lu element array of unsigned longs for "
			"'buffer_a' belonging to XIA MCS '%s'.",
				handel_mcs->buffer_length,
				mcs->record->name );
		}

		handel_mcs->buffer_b = malloc( handel_mcs->buffer_length
						* sizeof(long) );

		if ( handel_mcs->buffer_b == (uint16_t *) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a "
			"%lu element array of unsigned longs for "
			"'buffer_b' belonging to XIA MCS '%s'.",
				handel_mcs->buffer_length,
				mcs->record->name );
		}
	}

	/* Create a thread that handles reading out 'buffer_a' and
	 * 'buffer_b' fast enough to keep up with the rate that the
	 * XIA hardware generates it.
	 */

	mx_status = mx_thread_create( &(handel->monitor_thread),
					mxd_handel_mcs_monitor_thread_fn,
					mcs->record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If we are in external trigger mode, then start the MCS. */

	if ( mcs->trigger_mode == MXF_DEV_EXTERNAL_TRIGGER ) {
		/* Start the MCS. */

		mx_status = mxd_handel_mca_start_run( mca, 1 );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mcs_trigger( MX_MCS *mcs )
{
	static const char fname[] = "mxd_handel_mcs_trigger()";

	MX_MCA *mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mcs_get_pointers( mcs, NULL,
					&mca, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mcs->trigger_mode == MXF_DEV_INTERNAL_TRIGGER ) {
		/* Start the MCS. */

		mx_status = mxd_handel_mca_start_run( mca, 1 );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mcs_stop( MX_MCS *mcs )
{
	static const char fname[] = "mxd_handel_mcs_stop()";

	MX_MCA *mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mcs_get_pointers( mcs, NULL,
					&mca, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_handel_mca_stop_run( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mcs_clear( MX_MCS *mcs )
{
	static const char fname[] = "mxd_handel_mcs_clear()";

	MX_HANDEL_MCS *handel_mcs = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mcs_get_pointers( mcs, &handel_mcs,
					NULL, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mcs_busy( MX_MCS *mcs )
{
	static const char fname[] = "mxd_handel_mcs_busy()";

	MX_HANDEL_MCS *handel_mcs = NULL;
	MX_HANDEL *handel = NULL;
	unsigned long run_active = 0;
	int xia_status;
	mx_status_type mx_status;

#if MXD_HANDEL_MCS_DEBUG_BUSY
	static mx_bool_type old_busy = FALSE;
#endif

	mx_status = mxd_handel_mcs_get_pointers( mcs, &handel_mcs,
					NULL, NULL, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_XIA_SYNC( xiaGetRunData( 0, "run_active", &run_active ) );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to get 'run_active' for MCS '%s' failed.  "
		"Error code = %d, '%s'",
		mcs->record->name,
		xia_status, mxi_handel_strerror(xia_status) );
	}

	if ( run_active ) {
		mcs->busy = TRUE;
		mcs->status = 0x1;
	} else {
		mcs->busy = FALSE;
		mcs->status = 0x0;
	}

#if MXD_HANDEL_MCS_DEBUG_BUSY
	if ( old_busy != mcs->busy ) {
		MX_DEBUG(-2,("%s: MCS '%s' busy = %d",
		fname, mcs->record->name, (int) mcs->busy ));
	}

	old_busy = mcs->busy;
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mcs_get_parameter( MX_MCS *mcs )
{
	static const char fname[] = "mxd_handel_mcs_get_parameter()";

	MX_HANDEL_MCS *handel_mcs = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mcs_get_pointers( mcs, &handel_mcs,
					NULL, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCS '%s', parameter type '%s' (%ld)",
		fname, mcs->record->name,
		mx_get_field_label_string( mcs->record, mcs->parameter_type ),
		mcs->parameter_type));

	switch( mcs->parameter_type ) {
	case MXLV_MCS_MEASUREMENT_NUMBER:
		break;
	case MXLV_MCS_DARK_CURRENT:
		break;
	case MXLV_MCS_TRIGGER_MODE:
		break;
	default:
		return mx_mcs_default_get_parameter_handler( mcs );
		break;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mcs_set_parameter( MX_MCS *mcs )
{
	static const char fname[] = "mxd_handel_mcs_set_parameter()";

	MX_HANDEL_MCS *handel_mcs = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mcs_get_pointers( mcs, &handel_mcs,
					NULL, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCS '%s', parameter type '%s' (%ld)",
		fname, mcs->record->name,
		mx_get_field_label_string( mcs->record, mcs->parameter_type ),
		mcs->parameter_type));

	switch( mcs->parameter_type ) {
	case MXLV_MCS_COUNTING_MODE:
		break;

	case MXLV_MCS_MEASUREMENT_TIME:
		break;

	case MXLV_MCS_CURRENT_NUM_MEASUREMENTS:
		break;

	case MXLV_MCS_DARK_CURRENT:
		break;

	case MXLV_MCS_TRIGGER_MODE:
		break;
	default:
		return mx_mcs_default_set_parameter_handler( mcs );
		break;
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

