/*
 * Name:    d_win32_system_clock.c
 *
 * Purpose: MX clock driver using Win32 GetSystemTimeAsFileTime() API.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_WIN32_SYSTEM_CLOCK_DEBUG	FALSE

#if defined( OS_WIN32 )

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_dynamic_library.h"
#include "mx_time.h"
#include "mx_clock.h"
#include "d_win32_system_clock.h"

/* Initialize the clock driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_win32_system_clock_record_function_list = {
	NULL,
	mxd_win32_system_clock_create_record_structures,
	mx_clock_finish_record_initialization
};

MX_CLOCK_FUNCTION_LIST mxd_win32_system_clock_clock_function_list = {
	mxd_win32_system_clock_get_timespec
};

/* Soft clock data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_win32_system_clock_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_CLOCK_STANDARD_FIELDS
};

long mxd_win32_system_clock_num_record_fields
		= sizeof( mxd_win32_system_clock_record_field_defaults )
		  / sizeof( mxd_win32_system_clock_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_win32_system_clock_rfield_def_ptr
			= &mxd_win32_system_clock_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_win32_system_clock_get_pointers( MX_CLOCK *clock,
			MX_WIN32_SYSTEM_CLOCK **win32_system_clock,
			const char *calling_fname )
{
	static const char fname[] = "mxd_win32_system_clock_get_pointers()";

	if ( clock == (MX_CLOCK *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The clock pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( clock->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for clock pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( clock->record->mx_type != MXT_CLK_WIN32_SYSTEM_CLOCK ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The clock '%s' passed by '%s' is not an OS clock.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			clock->record->name, calling_fname,
			clock->record->mx_superclass,
			clock->record->mx_class,
			clock->record->mx_type );
	}

	if ( win32_system_clock != (MX_WIN32_SYSTEM_CLOCK **) NULL ) {

		*win32_system_clock = (MX_WIN32_SYSTEM_CLOCK *)
				(clock->record->record_type_struct);

		if ( *win32_system_clock == (MX_WIN32_SYSTEM_CLOCK *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_WIN32_SYSTEM_CLOCK pointer for clock record '%s' passed by '%s' is NULL",
				clock->record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_win32_system_clock_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_win32_system_clock_create_record_structures()";

	MX_CLOCK *clock = NULL;
	MX_WIN32_SYSTEM_CLOCK *win32_system_clock = NULL;

	/* Allocate memory for the necessary structures. */

	clock = (MX_CLOCK *) malloc( sizeof(MX_CLOCK) );

	if ( clock == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_CLOCK structure." );
	}

	win32_system_clock = (MX_WIN32_SYSTEM_CLOCK *) malloc( sizeof(MX_WIN32_SYSTEM_CLOCK) );

	if ( win32_system_clock == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_WIN32_SYSTEM_CLOCK structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = clock;
	record->record_type_struct = win32_system_clock;
	record->class_specific_function_list
			= &mxd_win32_system_clock_clock_function_list;

	clock->record = record;

	return MX_SUCCESSFUL_RESULT;
}

typedef void (*GetSystemTimePreciseAsFileTime_ptr)( LPFILETIME );

static GetSystemTimePreciseAsFileTime_ptr
	ptrGetSystemTimePreciseAsFileTime = NULL;

MX_EXPORT mx_status_type
mxd_win32_system_clock_get_timespec( MX_CLOCK *clock )
{
	static const char fname[] = "mxd_win32_system_clock_get_timespec()";

	MX_WIN32_SYSTEM_CLOCK *win32_system_clock = NULL;
	FILETIME system_time_as_file_time;
	ULARGE_INTEGER system_time;
	uint64_t system_time_64bit;
	struct timespec system_timespec;
	mx_status_type mx_status;

	mx_status = mxd_win32_system_clock_get_pointers( clock,
						&win32_system_clock, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ptrGetSystemTimePreciseAsFileTime == NULL ) {

		mx_status = mx_dynamic_library_get_library_and_symbol_address(
					"KERNEL32.DLL",
					"GetSystemTimePreciseAsFileTime",
					NULL,
				(void **) &ptrGetSystemTimePreciseAsFileTime,
					0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( ptrGetSystemTimePreciseAsFileTime != NULL ) {
		/* Available for Window 8 and above. */

		ptrGetSystemTimePreciseAsFileTime( &system_time_as_file_time );
	} else {
		/* Said to be available for Windows 2000 and above. */

		GetSystemTimeAsFileTime( &system_time_as_file_time );
	}

	system_time.LowPart = system_time_as_file_time.dwLowDateTime;
	system_time.HighPart = system_time_as_file_time.dwHighDateTime;

	/* system_time_64bit contains the number of 100-nanosecond intervals
	 * since January 1, 1601 (UTC). Yay! Yet another time epoch convention.
	 */

	system_time_64bit = system_time.QuadPart;

	clock->timespec[0] = system_time_64bit / 10000000L;
	clock->timespec[1] = ( system_time_64bit % 10000000L ) * 100L;

	system_timespec.tv_sec = clock->timespec[0];
	system_timespec.tv_nsec = clock->timespec[1];

	clock->time = mx_convert_clock_time_to_seconds( system_timespec );

	return MX_SUCCESSFUL_RESULT;
}

#endif     /* OS_WIN32 */
