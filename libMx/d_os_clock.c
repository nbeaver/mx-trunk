/*
 * Name:    d_os_clock.c
 *
 * Purpose: MX clock driver for OS-based time-of-day clocks.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2022-2023 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_OS_CLOCK_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_time.h"
#include "mx_clock.h"
#include "d_os_clock.h"

/* Initialize the clock driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_os_clock_record_function_list = {
	NULL,
	mxd_os_clock_create_record_structures,
	mx_clock_finish_record_initialization
};

MX_CLOCK_FUNCTION_LIST mxd_os_clock_clock_function_list = {
	mxd_os_clock_get_timespec
};

/* Soft clock data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_os_clock_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_CLOCK_STANDARD_FIELDS
};

long mxd_os_clock_num_record_fields
		= sizeof( mxd_os_clock_record_field_defaults )
		  / sizeof( mxd_os_clock_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_os_clock_rfield_def_ptr
			= &mxd_os_clock_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_os_clock_get_pointers( MX_CLOCK *mx_clock,
			MX_OS_CLOCK **os_clock,
			const char *calling_fname )
{
	static const char fname[] = "mxd_os_clock_get_pointers()";

	if ( mx_clock == (MX_CLOCK *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The clock pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( mx_clock->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for clock pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( mx_clock->record->mx_type != MXT_CLK_OS_CLOCK ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The clock '%s' passed by '%s' is not an OS clock.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			mx_clock->record->name, calling_fname,
			mx_clock->record->mx_superclass,
			mx_clock->record->mx_class,
			mx_clock->record->mx_type );
	}

	if ( os_clock != (MX_OS_CLOCK **) NULL ) {

		*os_clock = (MX_OS_CLOCK *)
				(mx_clock->record->record_type_struct);

		if ( *os_clock == (MX_OS_CLOCK *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_OS_CLOCK pointer for clock record '%s' passed by '%s' is NULL",
				mx_clock->record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_os_clock_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_os_clock_create_record_structures()";

	MX_CLOCK *mx_clock = NULL;
	MX_OS_CLOCK *os_clock = NULL;

	/* Allocate memory for the necessary structures. */

	mx_clock = (MX_CLOCK *) malloc( sizeof(MX_CLOCK) );

	if ( mx_clock == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_CLOCK structure." );
	}

	os_clock = (MX_OS_CLOCK *) malloc( sizeof(MX_OS_CLOCK) );

	if ( os_clock == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_OS_CLOCK structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mx_clock;
	record->record_type_struct = os_clock;
	record->class_specific_function_list
			= &mxd_os_clock_clock_function_list;

	mx_clock->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_os_clock_get_timespec( MX_CLOCK *mx_clock )
{
	static const char fname[] = "mxd_os_clock_get_timespec()";

	MX_OS_CLOCK *os_clock = NULL;
	struct timespec os_timespec;
	mx_status_type mx_status;

	mx_status = mxd_os_clock_get_pointers( mx_clock, &os_clock, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	os_timespec = mx_current_os_time();

	mx_clock->timespec[0] = os_timespec.tv_sec;
	mx_clock->timespec[1] = os_timespec.tv_nsec;

	mx_clock->time = mx_convert_clock_time_to_seconds( os_timespec );

#if 0
	MX_DEBUG(-2,("%s: timespec = ( %ld, %ld ), time = %.6lf",
	  fname, mx_clock->timespec[0], mx_clock->timespec[1], mx_clock->time));
#endif

	return MX_SUCCESSFUL_RESULT;
}

