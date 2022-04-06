/*
 * Name:    mx_clock.h
 *
 * Purpose: MX clock to read the current time from a Time-Of-Day clock.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_clock.h"

/*=======================================================================*/

/* This function is a utility function to consolidate all of the pointer
 * mangling that often has to happen at the beginning of an MX clock
 * function.  The parameter 'calling_fname' is passed so that error messages
 * will appear with the name of the calling function.
 */

MX_EXPORT mx_status_type
mx_clock_get_pointers( MX_RECORD *clock_record,
			MX_CLOCK **clock,
			MX_CLOCK_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_clock_get_pointers()";

	if ( clock_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The clock_record pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( clock_record->mx_class != MXC_CLOCK ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"The record '%s' passed by '%s' is not a clock.",
			clock_record->name, calling_fname );
	}

	if ( clock != (MX_CLOCK **) NULL ) {
		*clock = (MX_CLOCK *) (clock_record->record_class_struct);

		if ( *clock == (MX_CLOCK *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_CLOCK pointer for record '%s' passed by '%s' is NULL",
				clock_record->name, calling_fname );
		}
	}

	if ( function_list_ptr != (MX_CLOCK_FUNCTION_LIST **) NULL ) {
		*function_list_ptr = (MX_CLOCK_FUNCTION_LIST *)
				(clock_record->class_specific_function_list);

		if ( *function_list_ptr == (MX_CLOCK_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_CLOCK_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
				clock_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_clock_get_timespec( MX_RECORD *clock_record, uint64_t *timespec )
{
	static const char fname[] = "mx_clock_get_timespec()";

	MX_CLOCK *clock;
	MX_CLOCK_FUNCTION_LIST *function_list;
	mx_status_type ( *get_timespec_fn )( MX_CLOCK * );
	mx_status_type mx_status;

	mx_status = mx_clock_get_pointers( clock_record,
					&clock, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_timespec_fn = function_list->get_timespec;

	if ( get_timespec_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_timespec function pointer for clock record '%s' is NULL.",
			clock_record->name );
	}

	mx_status = (*get_timespec_fn)( clock );

	if ( timespec != (uint64_t *) NULL ) {
		timespec[0] = clock->timespec[0];
		timespec[1] = clock->timespec[1];
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_clock_get_seconds( MX_RECORD *clock_record, double *seconds )
{
	static const char fname[] = "mx_clock_get_seconds()";

	MX_CLOCK *clock;
	uint64_t timespec[2];
	mx_status_type mx_status;

	mx_status = mx_clock_get_pointers( clock_record, &clock, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_clock_get_timespec( clock_record, timespec );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	clock->seconds = timespec[0] + 1.0e-9 * timespec[1];

	if ( seconds != (double *) NULL ) {
		*seconds = clock->seconds;
	}

	return mx_status;
}

/*=======================================================================*/

#if defined( OS_LINUX )

/* For Posix clocks */

MX_EXPORT mx_status_type
mx_clock_get_time( struct timespec *timespec )
{
	static const char fname[] = "mx_clock_get_time()";

	int os_status, saved_errno;

	os_status = clock_gettime( CLOCK_REALTIME, timespec );

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"A call to clock_gettime( CLOCK_REALTIME, ... ) failed.  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror(saved_errno) );
	}

	return MX_SUCCESSFUL_RESULT;
}

#else
#  error mx_clock_get_time() is not yet implemented for this build target.
#endif

