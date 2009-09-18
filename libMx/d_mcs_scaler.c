/*
 * Name:    d_mcs_scaler.c
 *
 * Purpose: MX scaler driver to control MCS channels as individual scalers.
 *
 *          By convention, the scaler number for an MCS scaler should match
 *          the labeling of scaler channel numbers on the front panel (if any)
 *          of the MCS.  It is the responsibility of the underlying MCS
 *          record to put the data at an appropriate place in mcs->data_array.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2002, 2004, 2006, 2008-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_scaler.h"
#include "mx_timer.h"
#include "mx_mcs.h"
#include "d_mcs_scaler.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_mcs_scaler_record_function_list = {
	NULL,
	mxd_mcs_scaler_create_record_structures,
	mxd_mcs_scaler_finish_record_initialization,
	NULL,
	mxd_mcs_scaler_print_structure
};

MX_SCALER_FUNCTION_LIST mxd_mcs_scaler_scaler_function_list = {
	mxd_mcs_scaler_clear,
	NULL,
	mxd_mcs_scaler_read,
	mxd_mcs_scaler_read_raw,
	mxd_mcs_scaler_is_busy,
	mxd_mcs_scaler_start,
	mxd_mcs_scaler_stop,
	mxd_mcs_scaler_get_parameter,
	mxd_mcs_scaler_set_parameter,
	mxd_mcs_scaler_set_modes_of_associated_counters
};

/* MCS scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_mcs_scaler_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_MCS_SCALER_STANDARD_FIELDS
};

long mxd_mcs_scaler_num_record_fields
		= sizeof( mxd_mcs_scaler_record_field_defaults )
		  / sizeof( mxd_mcs_scaler_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mcs_scaler_rfield_def_ptr
			= &mxd_mcs_scaler_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_mcs_scaler_get_pointers( MX_SCALER *scaler,
			MX_MCS_SCALER **mcs_scaler,
			MX_MCS **mcs,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mcs_scaler_get_pointers()";

	MX_MCS_SCALER *mcs_scaler_ptr;
	MX_RECORD *mcs_record;

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SCALER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	mcs_scaler_ptr = (MX_MCS_SCALER *) (scaler->record->record_type_struct);

	if ( mcs_scaler_ptr == (MX_MCS_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCS_SCALER pointer for record '%s' is NULL.",
			scaler->record->name );
	}

	if ( mcs_scaler != (MX_MCS_SCALER **) NULL ) {
		*mcs_scaler = mcs_scaler_ptr;
	}

	if ( mcs != (MX_MCS **) NULL ) {
		mcs_record = mcs_scaler_ptr->mcs_record;

		if ( mcs_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The mcs_record pointer for MCS scaler '%s' is NULL.",
				scaler->record->name );
		}

		*mcs = (MX_MCS *) mcs_record->record_class_struct;

		if ( *mcs == (MX_MCS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_MCS pointer for MCS record '%s' used by record '%s' is NULL.",
				mcs_record->name, scaler->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_mcs_scaler_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_mcs_scaler_create_record_structures()";

	MX_SCALER *scaler;
	MX_MCS_SCALER *mcs_scaler = NULL;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_SCALER structure." );
	}

	mcs_scaler = (MX_MCS_SCALER *) malloc( sizeof(MX_MCS_SCALER) );

	if ( mcs_scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MCS_SCALER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = mcs_scaler;
	record->class_specific_function_list
			= &mxd_mcs_scaler_scaler_function_list;

	scaler->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcs_scaler_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_mcs_scaler_finish_record_initialization()";

	MX_MCS *mcs = NULL;
	MX_SCALER *scaler;
	MX_MCS_SCALER *mcs_scaler = NULL;
	long scaler_index;
	mx_status_type mx_status;

	mx_status = mx_scaler_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scaler = (MX_SCALER *) record->record_class_struct;

	mx_status = mxd_mcs_scaler_get_pointers( scaler,
						&mcs_scaler, &mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mcs->scaler_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The scaler_record_array pointer for MCS record '%s' is NULL.",
			mcs->record->name );
	}

	scaler_index = mcs_scaler->scaler_number;

	mcs->scaler_record_array[ scaler_index ] = record;

#if 0
	MX_DEBUG(-2,("%s: record = %p", fname, record));
	MX_DEBUG(-2,("%s: record->name = '%s'", fname, record->name));
	MX_DEBUG(-2,("%s: mcs_scaler = %p", fname, mcs_scaler));
	MX_DEBUG(-2,("%s: mcs_scaler->mcs_record = %p",
					fname, mcs_scaler->mcs_record));
	MX_DEBUG(-2,("%s: mcs_scaler->mcs_record->name = '%s'",
					fname, mcs_scaler->mcs_record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcs_scaler_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_mcs_scaler_print_structure()";

	MX_SCALER *scaler;
	MX_MCS_SCALER *mcs_scaler = NULL;
	long current_value;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	scaler = (MX_SCALER *) (record->record_class_struct);

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SCALER pointer for record '%s' is NULL.", record->name);
	}

	mcs_scaler = (MX_MCS_SCALER *) (record->record_type_struct);

	if ( mcs_scaler == (MX_MCS_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MCS_SCALER pointer for record '%s' is NULL.", record->name);
	}

	fprintf(file, "SCALER parameters for scaler '%s':\n", record->name);

	fprintf(file, "  Scaler type           = MCS_SCALER.\n\n");
	fprintf(file, "  MCS record            = %s\n",
					mcs_scaler->mcs_record->name);
	fprintf(file, "  scaler number         = %ld\n",
					mcs_scaler->scaler_number);

	mx_status = mx_scaler_read( record, &current_value );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read value of scaler '%s'",
			record->name );
	}

	fprintf(file, "  present value         = %ld\n", current_value);

	mx_status = mx_scaler_get_dark_current( record, NULL );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read the dark current for scaler '%s'",
			record->name );
	}

	fprintf(file, "  dark current          = %g counts per second.\n",
						scaler->dark_current);
	fprintf(file, "  scaler flags          = %#lx\n", scaler->scaler_flags);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcs_scaler_clear( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_mcs_scaler_clear()";

	MX_MCS_SCALER *mcs_scaler = NULL;
	mx_status_type mx_status;

	mx_status = mxd_mcs_scaler_get_pointers( scaler,
						&mcs_scaler, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for scaler '%s'",
			fname, scaler->record->name));

#if 0
	/* NOTE: The 'start' routines for MCS scalers and timers all set
	 * the number of measurements to 1, so it should be unnecessary
	 * to set it to 1 here as well.
	 */

	mx_status = mx_mcs_set_num_measurements( mcs_scaler->mcs_record, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	mx_status = mx_mcs_clear( mcs_scaler->mcs_record );

	MX_DEBUG( 2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcs_scaler_read( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_mcs_scaler_read()";

	MX_MCS_SCALER *mcs_scaler = NULL;
	MX_MCS *mcs = NULL;
	MX_RECORD *timer_record;
	long scaler_index, offset;
	mx_bool_type fast_mode;
	double dark_current, last_measurement_time;
	mx_status_type mx_status;

	mx_status = mxd_mcs_scaler_get_pointers( scaler,
						&mcs_scaler, &mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for scaler '%s'",
			fname, scaler->record->name));

	if ( mcs->data_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"data_array pointer for MCS record '%s' is NULL.",
			mcs_scaler->mcs_record->name );
	}

	mx_status = mx_get_fast_mode( scaler->record, &fast_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mcs_set_num_measurements( mcs_scaler->mcs_record, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scaler_index = mcs_scaler->scaler_number;

	mx_status = mx_mcs_read_scaler( mcs_scaler->mcs_record,
						scaler_index, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	MX_DEBUG(-2,("%s: mcs = %p", fname, mcs));
	MX_DEBUG(-2,("%s: mcs->data_array = %p", fname, mcs->data_array));
#endif

	scaler->raw_value = mcs->data_array[ scaler_index ][0];

	/* The MCS record plays the role of the "server" in this case.
	 * Thus, if the "server" is supposed to subtract the dark current,
	 * we must ask the MCS record for the value of the dark current
	 * and subtract it ourselves.
	 */

	if ( scaler->scaler_flags & MXF_SCL_SERVER_SUBTRACTS_DARK_CURRENT ) {

		timer_record = scaler->timer_record;

		if ( timer_record == (MX_RECORD *) NULL ) {
			return mx_error(MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"Scaler record '%s' is configured to subtract a dark current, "
		"but no associated timer has been defined in the scaler's "
		"database entry.  You must either configure an associated "
		"timer record for this scaler, or turn the subtract dark "
		"current flag off.", scaler->record->name );
		}

		/* Only read the dark current if fast mode is turned off.
		 * In other words, don't read the dark current on each step
		 * of a step scan.
		 */

		if ( fast_mode == FALSE ) {
			mx_status = mx_mcs_get_dark_current(
					mcs_scaler->mcs_record,
					scaler_index,
					&dark_current );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			scaler->dark_current = dark_current;
		}

		mx_status = mx_timer_get_last_measurement_time( timer_record,
							&last_measurement_time);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( last_measurement_time >= 0.0 ) {
			offset = mx_round( scaler->dark_current
						* last_measurement_time );

			scaler->raw_value -= offset;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcs_scaler_read_raw( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_mcs_scaler_read_raw()";

	MX_MCS_SCALER *mcs_scaler = NULL;
	MX_MCS *mcs = NULL;
	long scaler_index;
	mx_status_type mx_status;

	mx_status = mxd_mcs_scaler_get_pointers( scaler,
						&mcs_scaler, &mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mcs->data_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"data_array pointer for MCS record '%s' is NULL.",
			mcs_scaler->mcs_record->name );
	}

	mx_status = mx_mcs_set_num_measurements( mcs_scaler->mcs_record, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scaler_index = mcs_scaler->scaler_number;

	mx_status = mx_mcs_read_scaler( mcs_scaler->mcs_record,
						scaler_index, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	MX_DEBUG(-2,("%s: mcs = %p", fname, mcs));
	MX_DEBUG(-2,("%s: mcs->data_array = %p", fname, mcs->data_array));
#endif

	scaler->raw_value = mcs->data_array[ scaler_index ][0];

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcs_scaler_is_busy( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_mcs_scaler_is_busy()";

	MX_MCS_SCALER *mcs_scaler = NULL;
	mx_bool_type busy;
	mx_status_type mx_status;

	mx_status = mxd_mcs_scaler_get_pointers( scaler,
						&mcs_scaler, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for scaler '%s'", fname, scaler->record->name));

	mx_status = mx_mcs_is_busy( mcs_scaler->mcs_record, &busy );

	scaler->busy = busy;

	MX_DEBUG( 2,("%s complete.  busy = %d", fname, (int) scaler->busy));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcs_scaler_start( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_mcs_scaler_start()";

	MX_MCS_SCALER *mcs_scaler = NULL;
	mx_status_type mx_status;

	mx_status = mxd_mcs_scaler_get_pointers( scaler,
						&mcs_scaler, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for scaler '%s' for %ld counts.",
			fname, scaler->record->name, scaler->raw_value));

	mx_status = mx_mcs_set_measurement_counts( mcs_scaler->mcs_record,
							scaler->raw_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: measurement counts have been set.", fname));

	mx_status = mx_mcs_set_num_measurements( mcs_scaler->mcs_record, 1L );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: num measurements has been set.", fname));

	mx_status = mx_mcs_start( mcs_scaler->mcs_record );

	MX_DEBUG( 2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcs_scaler_stop( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_mcs_scaler_stop()";

	MX_MCS_SCALER *mcs_scaler = NULL;
	mx_status_type mx_status;

	mx_status = mxd_mcs_scaler_get_pointers( scaler,
						&mcs_scaler, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for scaler '%s'", fname, scaler->record->name));

	mx_status = mx_mcs_stop( mcs_scaler->mcs_record );

	MX_DEBUG( 2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcs_scaler_get_parameter( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_mcs_scaler_get_parameter()";

	MX_MCS_SCALER *mcs_scaler = NULL;
	unsigned long scaler_index;
	long mode;
	double dark_current;
	mx_status_type mx_status;

	mx_status = mxd_mcs_scaler_get_pointers( scaler,
						&mcs_scaler, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		mx_status = mx_mcs_get_mode( mcs_scaler->mcs_record, &mode );

		scaler->mode = mode;
		break;
	case MXLV_SCL_DARK_CURRENT:
		if ( scaler->scaler_flags
			& MXF_SCL_SERVER_SUBTRACTS_DARK_CURRENT )
		{
			scaler_index = mcs_scaler->scaler_number;

			mx_status = mx_mcs_get_dark_current(
						mcs_scaler->mcs_record,
						scaler_index,
						&dark_current );

			scaler->dark_current = dark_current;
		}
		break;
	default:
		mx_status = mx_scaler_default_get_parameter_handler( scaler );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcs_scaler_set_parameter( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_mcs_scaler_set_parameter()";

	MX_MCS_SCALER *mcs_scaler = NULL;
	unsigned long scaler_index;
	mx_status_type mx_status;

	mx_status = mxd_mcs_scaler_get_pointers( scaler,
						&mcs_scaler, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		/* The following is currently disabled since it is not clear
		 * that running an MCS in a non-preset time mode makes any
		 * sense.
		 */
#if 0
		mx_status = mx_mcs_set_mode( mcs_scaler->mcs_record,
						scaler->mode );
#else
		mx_status = MX_SUCCESSFUL_RESULT;
#endif
		break;
	case MXLV_SCL_DARK_CURRENT:
		if ( scaler->scaler_flags
			& MXF_SCL_SERVER_SUBTRACTS_DARK_CURRENT )
		{
			scaler_index = mcs_scaler->scaler_number;

			mx_status = mx_mcs_set_dark_current(
						mcs_scaler->mcs_record,
						scaler_index,
						scaler->dark_current );
		}
		break;
	default:
		mx_status = mx_scaler_default_set_parameter_handler( scaler );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcs_scaler_set_modes_of_associated_counters( MX_SCALER *scaler )
{
	/* This function is unnecessary if all the counters for a measurement
	 * belong to the same multichannel scaler (which is true at present).
	 */

	return MX_SUCCESSFUL_RESULT;
}

