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
#include "mx_time.h"
#include "mx_hrt.h"
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
mx_clock_finish_record_initialization( MX_RECORD *clock_record )
{
	static const char fname[] = "mx_clock_finish_record_initialization()";

	MX_CLOCK *clock = NULL;
	mx_status_type mx_status;

	mx_status = mx_clock_get_pointers( clock_record, &clock, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Zero out the various time fields. */

	clock->timespec[0] = 0;
	clock->timespec[1] = 0;

	clock->timespec_offset[0] = 0;
	clock->timespec_offset[1] = 0;

	clock->seconds = 0.0;
	clock->offset = 0.0;

	clock->timespec_offset_struct.tv_sec = 0;
	clock->timespec_offset_struct.tv_nsec = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_clock_get_timespec( MX_RECORD *clock_record, uint64_t *timespec )
{
	static const char fname[] = "mx_clock_get_timespec()";

	MX_CLOCK *clock;
	MX_CLOCK_FUNCTION_LIST *function_list;
	mx_status_type ( *get_timespec_fn )( MX_CLOCK * );
	struct timespec timespec_struct, result_struct;
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

	timespec_struct.tv_sec = clock->timespec[0];
	timespec_struct.tv_nsec = clock->timespec[0];

	result_struct = mx_subtract_high_resolution_times( timespec_struct,
						clock->timespec_offset_struct );

	clock->timespec[0] = result_struct.tv_sec;
	clock->timespec[1] = result_struct.tv_nsec;

	if ( timespec != (uint64_t *) NULL ) {
		timespec[0] = clock->timespec[0];
		timespec[1] = clock->timespec[1];
	}

	clock->seconds = timespec[0] + 1.0e-9 * timespec[1];

	return mx_status;
}

MX_EXPORT mx_status_type
mx_clock_set_timespec_offset( MX_RECORD *clock_record,
				uint64_t *timespec_offset )
{
	static const char fname[] = "mx_clock_set_timespec_offset()";

	MX_CLOCK *clock = NULL;
	mx_status_type mx_status;

	mx_status = mx_clock_get_pointers( clock_record, &clock, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	clock->timespec_offset[0] = timespec_offset[0];
	clock->timespec_offset[1] = timespec_offset[1];

	clock->timespec_offset_struct.tv_sec = timespec_offset[0];
	clock->timespec_offset_struct.tv_nsec = timespec_offset[1];

	clock->offset = mx_convert_high_resolution_time_to_seconds(
						clock->timespec_offset_struct );

	return MX_SUCCESSFUL_RESULT;
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

	if ( seconds != (double *) NULL ) {
		*seconds = clock->seconds;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_clock_set_offset( MX_RECORD *clock_record, double offset )
{
	static const char fname[] = "mx_clock_set_offset()";

	MX_CLOCK *clock = NULL;
	mx_status_type mx_status;

	mx_status = mx_clock_get_pointers( clock_record, &clock, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( offset < 0.0 ) {
		clock->timespec_offset_struct = mx_current_os_time();

		clock->offset = mx_convert_high_resolution_time_to_seconds(
						clock->timespec_offset_struct );
	} else {
		clock->offset = offset;

		clock->timespec_offset_struct =
			mx_convert_seconds_to_high_resolution_time( offset );
	}

	clock->timespec_offset[0] = clock->timespec_offset_struct.tv_sec;
	clock->timespec_offset[1] = clock->timespec_offset_struct.tv_nsec;

	return MX_SUCCESSFUL_RESULT;
}

