/*
 * Name:    d_mca_alt_time.c
 *
 * Purpose: MX scaler driver to read out alternate MCA time values.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2002, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_scaler.h"
#include "mx_timer.h"
#include "mx_mca.h"
#include "d_mca_timer.h"
#include "d_mca_alt_time.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_mca_alt_time_record_function_list = {
	NULL,
	mxd_mca_alt_time_create_record_structures,
	mxd_mca_alt_time_finish_record_initialization
};

MX_SCALER_FUNCTION_LIST mxd_mca_alt_time_scaler_function_list = {
	NULL,
	NULL,
	mxd_mca_alt_time_read
};

/* MX MCA alternate time scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_mca_alt_time_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_MCA_ALT_TIME_STANDARD_FIELDS
};

long mxd_mca_alt_time_num_record_fields
		= sizeof( mxd_mca_alt_time_record_field_defaults )
		  / sizeof( mxd_mca_alt_time_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mca_alt_time_rfield_def_ptr
			= &mxd_mca_alt_time_record_field_defaults[0];

/*=======================================================================*/

/* Note: The fact that this routine returns an MX_MCA_TIMER structure rather
 *       than an MX_MCA_ALT_TIME structure is intentional.  This is not
 *       a mistake.  After all, the MX_MCA_TIMER structure is the one that
 *       has everything we need in it.
 */

static mx_status_type
mxd_mca_alt_time_get_pointers( MX_SCALER *scaler,
			MX_MCA_ALT_TIME **mca_alt_time,
			MX_MCA_TIMER **mca_timer,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mca_alt_time_get_pointers()";

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The scaler pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( scaler->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for scaler pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( scaler->record->mx_type != MXT_SCL_MCA_ALTERNATE_TIME ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The scaler '%s' passed by '%s' is not an MCA alternate time scaler.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			scaler->record->name, calling_fname,
			scaler->record->mx_superclass,
			scaler->record->mx_class,
			scaler->record->mx_type );
	}

	if ( mca_alt_time != (MX_MCA_ALT_TIME **) NULL ) {
		*mca_alt_time = (MX_MCA_ALT_TIME *)
					scaler->record->record_type_struct;

		if ( (*mca_alt_time) == (MX_MCA_ALT_TIME *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_MCA_ALT_TIME pointer for scaler record '%s' passed by '%s' is NULL",
				scaler->record->name, calling_fname );
		}
	}

	if ( mca_timer != (MX_MCA_TIMER **) NULL ) {

		if ( scaler->timer_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The associated timer_record pointer for scaler '%s' passed by '%s' is NULL.",
				scaler->record->name, calling_fname );
		}

		if ( scaler->timer_record->mx_type != MXT_TIM_MCA ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
"The associated timer record '%s' for scaler '%s' is not an MCA timer record.",
				scaler->timer_record->name,
				scaler->record->name );
		}

		*mca_timer = (MX_MCA_TIMER *)
				scaler->timer_record->record_type_struct;

		if ( (*mca_timer) == (MX_MCA_TIMER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCA_TIMER pointer for MCA timer record '%s' is NULL.",
				scaler->timer_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_mca_alt_time_create_record_structures( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_mca_alt_time_create_record_structures()";

	MX_SCALER *scaler;
	MX_MCA_ALT_TIME *mca_alt_time;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER structure." );
	}

	mca_alt_time = (MX_MCA_ALT_TIME *) malloc( sizeof(MX_MCA_ALT_TIME) );

	if ( mca_alt_time == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCA_ALT_TIME structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = mca_alt_time;
	record->class_specific_function_list
			= &mxd_mca_alt_time_scaler_function_list;

	scaler->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_alt_time_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_mca_alt_time_finish_record_initialization()";

	MX_SCALER *scaler;
	mx_status_type mx_status;

	mx_status = mx_scaler_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scaler = (MX_SCALER *) record->record_class_struct;

	if ( scaler->timer_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"mca_alt_time scaler record '%s' does not have a timer record associated "
"with it.  You must specify an associated timer record in the database.",
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_alt_time_read( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_mca_alt_time_read()";

	MX_MCA_ALT_TIME *mca_alt_time;
	MX_MCA_TIMER *mca_timer;
	double alternate_time;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	mca_timer = NULL;
	mca_alt_time = NULL;

	mx_status = mxd_mca_alt_time_get_pointers( scaler,
					&mca_alt_time, &mca_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( mca_alt_time->time_type ) {
	case MXF_MCA_ALT_TIME_LIVE_TIME:
		mx_status = mx_mca_get_live_time( mca_timer->mca_record,
							&alternate_time );
		break;

	case MXF_MCA_ALT_TIME_REAL_TIME:
		mx_status = mx_mca_get_real_time( mca_timer->mca_record,
							&alternate_time );
		break;

	case MXF_MCA_ALT_TIME_COMPLEMENTARY_TIME:
		if ( mca_timer->use_real_time ) {
			mx_status = mx_mca_get_live_time( mca_timer->mca_record,
							&alternate_time );
		} else {
			mx_status = mx_mca_get_real_time( mca_timer->mca_record,
							&alternate_time );
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal 'time_type' value %d for record '%s'.  "
		"The allowed values are 0, 1, and 2.",
			mca_alt_time->time_type,
			scaler->record->name );
	}

	scaler->raw_value
		= mx_round( mca_alt_time->timer_scale * alternate_time );

	return mx_status;
}

