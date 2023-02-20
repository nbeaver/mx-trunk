/*
 * Name:    mx_clock.h
 *
 * Purpose: MX clock to read the current time from a Time-Of-Day clock.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2022-2023 Illinois Institute of Technology
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
#include "mx_osdef.h"
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
			MX_CLOCK **mx_clock,
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

	if ( mx_clock != (MX_CLOCK **) NULL ) {
		*mx_clock = (MX_CLOCK *) (clock_record->record_class_struct);

		if ( *mx_clock == (MX_CLOCK *) NULL ) {
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

	MX_CLOCK *mx_clock = NULL;
	mx_status_type mx_status;

	mx_status = mx_clock_get_pointers( clock_record,
					&mx_clock, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Zero out the various time fields. */

	mx_clock->timespec[0] = 0;
	mx_clock->timespec[1] = 0;

	mx_clock->timespec_offset[0] = 0;
	mx_clock->timespec_offset[1] = 0;

	mx_clock->time = 0.0;
	mx_clock->time_offset = 0.0;

	mx_clock->timespec_offset_struct.tv_sec = 0;
	mx_clock->timespec_offset_struct.tv_nsec = 0;

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

#if defined( OS_WIN32 )
#  define tzset		_tzset
#  define tzname	_tzname
#endif

MX_EXPORT mx_status_type
mx_clock_default_get_parameter_handler( MX_CLOCK *mx_clock )
{
	static const char fname[] = "mx_clock_default_get_parameter_handler()";

	time_t current_epoch_time_in_seconds;
	struct tm current_time_in_struct_tm;
	int saved_errno;

	mx_status_type mx_status = MX_SUCCESSFUL_RESULT;

	switch( mx_clock->parameter_type ) {
	case MXLV_CLK_TIMESTAMP:
		current_epoch_time_in_seconds = time( NULL );

		if ( current_epoch_time_in_seconds == (time_t) (-1) ) {
			saved_errno = errno;

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to read the current epoch time.  "
			"errno = %d, error message = '%s'",
				saved_errno, strerror( saved_errno ) );
		}


		(void) localtime_r( &current_epoch_time_in_seconds,
					&current_time_in_struct_tm );

		strftime( mx_clock->timestamp, MXU_CLK_STRING_LENGTH,
				"%b %d %H:%M:%S", &current_time_in_struct_tm );
		break;

	case MXLV_CLK_UTC_OFFSET:
		mx_clock->utc_offset = 0;
		break;

	case MXLV_CLK_TIMEZONE_NAME:
		tzset();	/* Sets the value of 'tzname' */

		strlcpy( mx_clock->timezone_name,
				tzname[0], MXU_CLK_STRING_LENGTH );
		break;

	/* Platform dependent epoch time names. */

#if defined( OS_WIN32 )

	/* January 1, 1601 00:00:00 UTC */
	case MXLV_CLK_EPOCH_NAME:
		strlcpy( mx_clock->epoch_name,
			"Windows", MXU_CLK_STRING_LENGTH );
		break;


#elif ( defined( OS_UNIX ) || defined( OS_CYGWIN ) )

	/* January 1, 1970 00:00:00 UTC */
	case MXLV_CLK_EPOCH_NAME:
		strlcpy( mx_clock->epoch_name, "UNIX", MXU_CLK_STRING_LENGTH );
		break;

#elif defined( OS_VMS )

	/* November 17, 1858 00:00:00 UTC */
	case MXLV_CLK_EPOCH_NAME:
		strlcpy( mx_clock->epoch_name, "VMS", MXU_CLK_STRING_LENGTH );
		break;
#else
#error Reporting the Epoch name is not yet implemented for this build target.
#endif
	/* -------- */

	default:
		break;
	}

	return mx_status;
}

/*----*/

MX_EXPORT mx_status_type
mx_clock_get_timespec( MX_RECORD *clock_record, uint64_t *timespec )
{
	static const char fname[] = "mx_clock_get_timespec()";

	MX_CLOCK *mx_clock = NULL;
	MX_CLOCK_FUNCTION_LIST *flist = NULL;
	mx_status_type ( *get_timespec_fn )( MX_CLOCK * ) = NULL;
	struct timespec timespec_struct, result_struct;
	mx_status_type mx_status;

	mx_status = mx_clock_get_pointers( clock_record,
					&mx_clock, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_timespec_fn = flist->get_timespec;

	if ( get_timespec_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_timespec function pointer for clock record '%s' is NULL.",
			clock_record->name );
	}

	mx_status = (*get_timespec_fn)( mx_clock );

	timespec_struct.tv_sec = mx_clock->timespec[0];
	timespec_struct.tv_nsec = mx_clock->timespec[1];

	result_struct = mx_subtract_timespec_times( timespec_struct,
					mx_clock->timespec_offset_struct );

	mx_clock->timespec[0] = result_struct.tv_sec;
	mx_clock->timespec[1] = result_struct.tv_nsec;

	if ( timespec != (uint64_t *) NULL ) {
		timespec[0] = mx_clock->timespec[0];
		timespec[1] = mx_clock->timespec[1];
	}

	mx_clock->time = mx_clock->timespec[0] + 1.0e-9 * mx_clock->timespec[1];

	return mx_status;
}

MX_EXPORT mx_status_type
mx_clock_set_timespec_offset( MX_RECORD *clock_record,
				uint64_t *timespec_offset )
{
	static const char fname[] = "mx_clock_set_timespec_offset()";

	MX_CLOCK *mx_clock = NULL;
	mx_status_type mx_status;

	mx_status = mx_clock_get_pointers( clock_record,
						&mx_clock, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( timespec_offset == (uint64_t *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The timespec_offset pointer passed for clock '%s' was NULL.",
			clock_record->name );
	}

	mx_clock->timespec_offset[0] = timespec_offset[0];
	mx_clock->timespec_offset[1] = timespec_offset[1];

	mx_clock->timespec_offset_struct.tv_sec = timespec_offset[0];
	mx_clock->timespec_offset_struct.tv_nsec = timespec_offset[1];

	mx_clock->time_offset = mx_convert_timespec_time_to_seconds(
					mx_clock->timespec_offset_struct );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_clock_get_time( MX_RECORD *clock_record, double *time_in_seconds )
{
	static const char fname[] = "mx_clock_get_seconds()";

	MX_CLOCK *mx_clock;
	uint64_t timespec[2];
	mx_status_type mx_status;

	mx_status = mx_clock_get_pointers( clock_record,
						&mx_clock, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_clock_get_timespec( clock_record, timespec );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( time_in_seconds != (double *) NULL ) {
		*time_in_seconds = mx_clock->time;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_clock_set_time_offset( MX_RECORD *clock_record, double offset )
{
	static const char fname[] = "mx_clock_set_time_offset()";

	MX_CLOCK *mx_clock = NULL;
	mx_status_type mx_status;

	mx_status = mx_clock_get_pointers( clock_record,
						&mx_clock, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( offset < 0.0 ) {
		mx_clock->timespec_offset_struct = mx_current_os_time();

		mx_clock->time_offset = mx_convert_timespec_time_to_seconds(
					mx_clock->timespec_offset_struct );
	} else {
		mx_clock->time_offset = offset;

		mx_clock->timespec_offset_struct =
			mx_convert_seconds_to_timespec_time( offset );
	}

	mx_clock->timespec_offset[0] = mx_clock->timespec_offset_struct.tv_sec;
	mx_clock->timespec_offset[1] = mx_clock->timespec_offset_struct.tv_nsec;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_clock_set_offset_to_current_time( MX_RECORD *clock_record )
{
	uint64_t current_time[2];
	mx_status_type mx_status;

	mx_status = mx_clock_get_timespec( clock_record, current_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_clock_set_timespec_offset( clock_record, current_time );

	return mx_status;
}

/*----*/

MX_EXPORT mx_status_type
mx_clock_get_timestamp( MX_RECORD *clock_record,
			char *timestamp, size_t timestamp_length )
{
	static const char fname[] = "mx_clock_get_timestamp()";

	MX_CLOCK *mx_clock = NULL;
	MX_CLOCK_FUNCTION_LIST *flist = NULL;
	mx_status_type ( *get_parameter_fn )( MX_CLOCK * ) = NULL;
	mx_status_type mx_status;

	mx_status = mx_clock_get_pointers( clock_record,
						&mx_clock, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_clock_default_get_parameter_handler;
	}

	mx_clock->parameter_type = MXLV_CLK_TIMESTAMP;

	mx_status = (*get_parameter_fn)( mx_clock );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( timestamp != (char *) NULL ) {
		strlcpy( timestamp, mx_clock->timestamp, timestamp_length );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_clock_get_utc_offset( MX_RECORD *clock_record, long *utc_offset )
{
	static const char fname[] = "mx_clock_get_utc_offset()";

	MX_CLOCK *mx_clock = NULL;
	MX_CLOCK_FUNCTION_LIST *flist = NULL;
	mx_status_type ( *get_parameter_fn )( MX_CLOCK * ) = NULL;
	mx_status_type mx_status;

	mx_status = mx_clock_get_pointers( clock_record,
						&mx_clock, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_clock_default_get_parameter_handler;
	}

	mx_clock->parameter_type = MXLV_CLK_UTC_OFFSET;

	mx_status = (*get_parameter_fn)( mx_clock );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( utc_offset != (long *) NULL ) {
		*utc_offset = mx_clock->utc_offset;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_clock_get_timezone_name( MX_RECORD *clock_record,
			char *timezone_name, size_t timezone_name_length )
{
	static const char fname[] = "mx_clock_get_timezone_name()";

	MX_CLOCK *mx_clock = NULL;
	MX_CLOCK_FUNCTION_LIST *flist = NULL;
	mx_status_type ( *get_parameter_fn )( MX_CLOCK * ) = NULL;
	mx_status_type mx_status;

	mx_status = mx_clock_get_pointers( clock_record,
						&mx_clock, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_clock_default_get_parameter_handler;
	}

	mx_clock->parameter_type = MXLV_CLK_TIMEZONE_NAME;

	mx_status = (*get_parameter_fn)( mx_clock );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( timezone_name != (char *) NULL ) {
		strlcpy( timezone_name,
			mx_clock->timezone_name,
			timezone_name_length );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_clock_get_epoch_name( MX_RECORD *clock_record,
			char *epoch_name, size_t epoch_name_length )
{
	static const char fname[] = "mx_clock_get_epoch_name()";

	MX_CLOCK *mx_clock = NULL;
	MX_CLOCK_FUNCTION_LIST *flist = NULL;
	mx_status_type ( *get_parameter_fn )( MX_CLOCK * ) = NULL;
	mx_status_type mx_status;

	mx_status = mx_clock_get_pointers( clock_record,
						&mx_clock, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_clock_default_get_parameter_handler;
	}

	mx_clock->parameter_type = MXLV_CLK_EPOCH_NAME;

	mx_status = (*get_parameter_fn)( mx_clock );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( epoch_name != (char *) NULL ) {
		strlcpy( epoch_name,
			mx_clock->epoch_name,
			epoch_name_length );
	}

	return MX_SUCCESSFUL_RESULT;
}

