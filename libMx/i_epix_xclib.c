/*
 * Name:    i_epix_xclib.c
 *
 * Purpose: MX interface driver for cameras controlled through the
 *          EPIX, Inc. EPIX_XCLIB library.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_EPIX_XCLIB_DEBUG	TRUE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_EPIX_XCLIB

#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_hrt.h"
#include "i_epix_xclib.h"

#if defined(OS_WIN32)
#include <windows.h>
#endif

#include "xcliball.h"

MX_RECORD_FUNCTION_LIST mxi_epix_xclib_record_function_list = {
	NULL,
	mxi_epix_xclib_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_epix_xclib_open
};

MX_RECORD_FIELD_DEFAULTS mxi_epix_xclib_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_EPIX_XCLIB_STANDARD_FIELDS
};

long mxi_epix_xclib_num_record_fields
		= sizeof( mxi_epix_xclib_record_field_defaults )
			/ sizeof( mxi_epix_xclib_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_epix_xclib_rfield_def_ptr
			= &mxi_epix_xclib_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_epix_xclib_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_epix_xclib_create_record_structures()";

	MX_EPIX_XCLIB *epix_xclib;

	/* Allocate memory for the necessary structures. */

	epix_xclib = (MX_EPIX_XCLIB *) malloc( sizeof(MX_EPIX_XCLIB) );

	if ( epix_xclib == (MX_EPIX_XCLIB *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_EPIX_XCLIB structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = epix_xclib;

	record->record_function_list = &mxi_epix_xclib_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	epix_xclib->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epix_xclib_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_epix_xclib_open()";

	char fault_message[80];
	MX_EPIX_XCLIB *epix_xclib;
	int i, length, epix_status;
	char error_message[80];
	uint original_exsync, original_prin;
	unsigned long timeout_in_milliseconds;
	double timeout_in_seconds;
	uint32_t epix_system_time;
	struct timespec epix_system_timespec, os_timespec;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	epix_xclib = (MX_EPIX_XCLIB *) record->record_type_struct;

	if ( epix_xclib == (MX_EPIX_XCLIB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_EPIX_XCLIB pointer for record '%s' is NULL.", record->name);
	}

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif

	epix_status = pxd_PIXCIopen( NULL, NULL, epix_xclib->format_file );

	if ( epix_status < 0 ) {

		pxd_mesgFaultText(-1, fault_message, sizeof(fault_message) );

		length = strlen(fault_message);

		for ( i = 0; i < length; i++ ) {
			if ( fault_message[i] == '\n' )
				fault_message[i] = ' ';
		}

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Loading PIXCI configuration '%s' failed for record '%s' "
		"with error code %d (%s).  Fault description = '%s'.", 
			epix_xclib->format_file, record->name,
			epix_status, pxd_mesgErrorCode( epix_status ),
			fault_message );
	}

#if MXI_EPIX_XCLIB_DEBUG
	/* Display some statistics. */

	MX_DEBUG(-2,("%s: Library Id = '%s'", fname, pxd_infoLibraryId() ));

	MX_DEBUG(-2,("%s: Include Id = '%s'", fname, pxd_infoIncludeId() ));

	MX_DEBUG(-2,("%s: Driver Id  = '%s'", fname, pxd_infoDriverId() ));

	MX_DEBUG(-2,("%s: Image frame buffer memory size = %lu bytes",
					fname, pxd_infoMemsize(-1) ));

	MX_DEBUG(-2,("%s: Number of boards = %d", fname, pxd_infoUnits() ));

	MX_DEBUG(-2,("%s: Number of frame buffers per board= %d",
					fname, pxd_imageZdim() ));

	MX_DEBUG(-2,("%s: X dimension = %d, Y dimension = %d",
				fname, pxd_imageXdim(), pxd_imageYdim() ));

	MX_DEBUG(-2,("%s: %d bits per pixel component",
					fname, pxd_imageBdim() ));

	MX_DEBUG(-2,("%s: %d components per pixel", fname, pxd_imageCdim() ));

	MX_DEBUG(-2,("%s: %d fields per frame buffer", fname, pxd_imageIdim()));

	MX_DEBUG(-2,("%s: Getting the zero for EPIX system time.", fname));
#endif

	/* We need to be able to convert the EPIX system time as reported
	 * by pxd_buffersSysTicks() and pxd_capturedSysTicks() into an
	 * absolute timestamp.  This requires us to determine the absolute
	 * time at which the EPIX system time was zero.
	 *
	 * Our current method for doing this is to take a frame and then
	 * invoke time() as soon as possible after the frame ends.  This
	 * means that there is an uncertainty on the order of a second or
	 * so in the absolute zero for the EPIX system time.  However, the
	 * difference in system time between consecutive frames should 
	 * be as accurate as the EPIX software can make it.
	 */

	/* Save the original values of EXSYNC (CC1 pulse width) and
	 * PRIN (time between the end of one CC1 pulse and the start
	 * of the next CC1 pulse).
	 */

#if 0
	original_exsync = pxd_getExsync(1);
	original_prin   = pxd_getPrin(1);

	/* Set the EXSYNC and PRIN counts to be very small.  This makes
	 * the exposure time for the following test frame as short as
	 * is possible.
	 */

	epix_status = pxd_setExsyncPrin(1,1,1);

	if ( epix_status != 0 ) {
		mxi_epix_xclib_error_message( 1, epix_status,
					error_message, sizeof(error_message) );
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to read the EXSYNC and PRIN registers "
			"for record '%s' failed.  %s",
				record->name, error_message );
	}
#endif

	/* Take a frame.  Time out if the frame takes longer than 1 second. */

	timeout_in_milliseconds = 1000;

	epix_status = pxd_doSnap(1,1,timeout_in_milliseconds);

	/* Get the EPIX system time. */

	epix_system_time = pxd_capturedSysTicks(1);

	/* Get the current wall clock time. */

	os_timespec = mx_current_os_time();

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: epix_system_time = %lu", fname,
					(unsigned long) epix_system_time));

	MX_DEBUG(-2,("%s: os_timespec = (%lu,%ld)", fname,
					os_timespec.tv_sec,
					os_timespec.tv_nsec));
#endif

	if ( epix_status == 0 ) {
		/* The function succeeded, so we do nothing here. */
	} else
	if ( epix_status == PXERTIMEOUT ) {
		pxd_goUnLive(1);

		timeout_in_seconds = 0.001 * (double) timeout_in_milliseconds;

		return mx_error( MXE_TIMED_OUT, fname,
		"Detector '%s' timed out after waiting %g seconds to get the "
		"current EPIX system clock time by taking a frame.  This could "
		"happen if the default exposure time set by the Video "
		"Configuration for your camera is longer than %g seconds.", 
			record->name, timeout_in_seconds, timeout_in_seconds );
	} else {
		mxi_epix_xclib_error_message( 1, epix_status,
					error_message, sizeof(error_message) );
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to calibrate the current EPIX system time "
			"for record '%s' failed.  %s",
				record->name, error_message );
	}

#if 0
	/* Restore the original EXSYNC and PRIN values. */

	epix_status = pxd_setExsyncPrin(1, original_exsync, original_prin);

	if ( epix_status != 0 ) {
		mxi_epix_xclib_error_message( 1, epix_status,
					error_message, sizeof(error_message) );
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to restore the EXSYNC and PRIN registers "
			"for record '%s' failed.  %s",
				record->name, error_message );
	}
#endif

	/* Compute the EPIX zero time. */

	epix_system_timespec =
	    mxi_epix_xclib_convert_system_time_to_timespec( epix_system_time );

	epix_xclib->epix_zero_time = mx_subtract_high_resolution_times(
					os_timespec, epix_system_timespec );

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: epix_system_timespec = (%lu,%ld)", fname,
				epix_system_timespec.tv_sec,
				epix_system_timespec.tv_nsec));

	MX_DEBUG(-2,("%s: epix_zero_time       = (%lu,%ld)", fname,
				epix_xclib->epix_zero_time.tv_sec,
				epix_xclib->epix_zero_time.tv_nsec));
#endif
	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT char *
mxi_epix_xclib_error_message( int unitmap,
				int epix_error_code,
				char *buffer,
				size_t buffer_length )
{
	char fault_message[500];
	int i, string_length, fault_string_length;

	MX_DEBUG(-2,("debug: unitmap = %d, epix_error_code = %d",
			unitmap, epix_error_code ));

	if ( buffer == NULL )
		return NULL;

	snprintf( buffer, buffer_length, "PIXCI error code = %d (%s).  ",
			epix_error_code, pxd_mesgErrorCode( epix_error_code ) );

	string_length = strlen( buffer );

	MX_DEBUG(-2,("debug: string_length = %d, buffer = %p, buffer = '%s'",
			string_length, buffer, buffer));

	return buffer;

	pxd_mesgFaultText( unitmap, fault_message, sizeof(fault_message) );

	MX_DEBUG(-2,("debug: fault_message #1 = '%s'", fault_message));

	fault_string_length = strlen( fault_message );

	MX_DEBUG(-2,("debug: fault_string_length #1 = %d",
			fault_string_length));

	if ( fault_message[fault_string_length - 1] == '\n' ) {
		fault_message[fault_string_length - 1] = '\0';

		fault_string_length--;
	}

	MX_DEBUG(-2,("debug: fault_message #2 = '%s'", fault_message));

	MX_DEBUG(-2,("debug: fault_string_length #2 = %d",
			fault_string_length));

	for ( i = 0; i < fault_string_length; i++ ) {
		if ( fault_message[i] == '\n' )
			fault_message[i] = ' ';
	}

	MX_DEBUG(-2,("debug: fault_message #3 = '%s'", fault_message));

	MX_DEBUG(-2,("debug: buffer = '%s'", buffer));

	return buffer;
}

#define MXP_ONE_NANOSECOND	1000000000L

MX_EXPORT struct timespec
mxi_epix_xclib_convert_system_time_to_timespec( unsigned long epix_system_time )
{
	struct timespec result;
	double epix_ticks_per_second, fp_result, fp_remainder;

#if defined(OS_LINUX)
	epix_ticks_per_second = 1000.0;

#elif defined(OS_WIN32)
	{
		int os_major, os_minor, os_update;

		(void) mx_get_os_version(&os_major, &os_minor, &os_update);

		if ( os_major == 4 ) {
			switch( os_minor ) {
			case 0:		/* Windows 95 */
			case 10:	/* Windows 98 */
			case 90:	/* Windows ME */
				epix_ticks_per_second = 1000.0;
				break;
			default:
				epix_ticks_per_second = 1.0e7;
			}
		} else {
				epix_ticks_per_second = 1.0e7;
		}
	}
#else
#error This platform is not supported for EPIX XCLIB.
#endif

	fp_result = ((double) epix_system_time) / epix_ticks_per_second;

	result.tv_sec = (time_t) fp_result;

	fp_remainder = fp_result - (double) result.tv_sec;

	result.tv_nsec = mx_round( fp_remainder * (double) MXP_ONE_NANOSECOND );

	/* Guard against floating point roundoff errors. */

	if ( result.tv_nsec < 0 ) {
		result.tv_nsec = 0;
	} else
	if ( result.tv_nsec >= MXP_ONE_NANOSECOND ) {
		result.tv_sec  += 1;
		result.tv_nsec -= MXP_ONE_NANOSECOND;
	}

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("EPIX: system time = %lu", epix_system_time));
	MX_DEBUG(-2,("EPIX: ticks per second = %g", epix_ticks_per_second));
	MX_DEBUG(-2,("EPIX: fp_result = %g, fp_remainder = %g",
				fp_result, fp_remainder));
	MX_DEBUG(-2,("EPIX: result = (%lu,%ld)",
				(unsigned long) result.tv_sec,
				result.tv_nsec));
#endif

	return result;
}

#endif /* HAVE_EPIX_XCLIB */

