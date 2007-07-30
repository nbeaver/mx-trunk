/*
 * Name:    i_epix_xclib.c
 *
 * Purpose: MX interface driver for cameras controlled through the
 *          EPIX, Inc. XCLIB library.
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

#define MXI_EPIX_XCLIB_DEBUG			TRUE

#define MXI_EPIX_XCLIB_DEBUG_SYSTEM_TICKS	FALSE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_EPIX_XCLIB

#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_unistd.h"
#include "mx_hrt.h"
#include "mx_inttypes.h"

#if defined(OS_WIN32)
#include <windows.h>
#endif

#include "xcliball.h"	/* Vendor include file. */

#include "i_epix_xclib.h"

MX_RECORD_FUNCTION_LIST mxi_epix_xclib_record_function_list = {
	NULL,
	mxi_epix_xclib_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_epix_xclib_open,
	mxi_epix_xclib_close,
	NULL,
	mxi_epix_xclib_resynchronize
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

/*---------------------------------------------------------------------------*/

static mx_bool_type mxi_epix_xclib_atexit_handler_installed = FALSE;

static void
mxi_epix_xclib_atexit_handler( void )
{
#if MXI_EPIX_XCLIB_DEBUG
	static char fname[] = "mxi_epix_xclib_atexit_handler()";

	MX_DEBUG(-2,("%s: atexit handler invoked.", fname));
#endif

	(void) pxd_PIXCIclose();
}

/*---------------------------------------------------------------------------*/

static mx_status_type
mxi_epix_xclib_get_timing_parameters( MX_EPIX_XCLIB *epix_xclib )
{
	static const char fname[] = "mxi_epix_xclib_get_timing_parameters()";

	struct pxvidstatus pxvstatus;
	mx_status_type mx_status;

	/* Get the current video status. */

	mx_status = mxi_epix_xclib_get_pxvidstatus( epix_xclib, 1, &pxvstatus,
							PXSTAT_UPDATED );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	epix_xclib->tick_frequency = pxvstatus.time.ticku[1];


	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

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

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_epix_xclib_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_epix_xclib_open()";

	char fault_message[80];
	MX_EPIX_XCLIB *epix_xclib;
	int os_major, os_minor, os_update;
	int i, length, epix_status;
	char error_message[80];
	uint original_exsync, original_prin;
	unsigned long requested_mask, actual_mask;
	unsigned long timeout_in_milliseconds, flags;
	double timeout_in_seconds;
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
	/* Check to see if we are running on a supported operating system. */

	mx_status = mx_get_os_version(&os_major, &os_minor, &os_update);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if defined(OS_LINUX)
	/* Please note that Linux 2.4.8 and after might be supportable.
	 * I just haven't put the effort into making that happen yet.
	 */

	if ( (os_major < 2) || (os_minor < 6) ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The MX epix_xclib driver does not support versions "
		"of the Linux kernel older than Linux 2.6.  "
		"You are running Linux %d.%d.%d",
			os_major, os_minor, os_update );
	}
#elif defined(OS_WIN32)
	{
		mx_bool_type supported;

		if ( os_major > 4 ) {
			supported = TRUE;
		} else
		if ( os_major < 4 ) {
			supported = FALSE;
		} else {
			switch( os_minor ) {
			case 1:
			case 3: /* These two cases are Windows NT 4 */

				supported = TRUE;

			default: /* All other cases are Windows 9x versions */

				supported = FALSE;
			}
		}

		if ( supported == FALSE ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"The MX epix_xclib driver for Windows only supports "
			"Windows NT 4, Windows 2000, Windows XP, "
			"and possibly Windows Server 2003 and Windows Vista.");
		}
	}
#endif

#if defined(OS_LINUX)
	{
		char pixci_filename[MXU_FILENAME_LENGTH];
		int os_status, saved_errno;

		/* Can we access the /dev/pixci device node? */

		strlcpy( pixci_filename, "/dev/pixci", sizeof(pixci_filename) );

		/* First check for existence of the file. */

		os_status = access( "/dev/pixci", F_OK );

		if ( os_status != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"The PIXCI device file '%s' does not exist. "
			"Have you run the command '/etc/init.d/pixci start' "
			"to create it?  "
			"Errno = %d, error message = '%s'.",
					pixci_filename,
					saved_errno, strerror(saved_errno) );
		}

		os_status = access( "/dev/pixci", R_OK );

		if ( os_status != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"This process not have read access for the PIXCI "
			"device file '%s'.  "
			"Errno = %d, error message = '%s'.",
					pixci_filename,
					saved_errno, strerror(saved_errno) );
		}
	}
#endif

	/* If requested, set the CPU affinity mask for the current process
	 * to enable running only on the first CPU.
	 */

	flags = epix_xclib->epix_xclib_flags;

	if ( flags & MXF_EPIX_SET_AFFINITY ) {

		/* Set affinity to the first CPU. */

		requested_mask = 0x1;

		mx_status = mx_set_process_affinity_mask( 0, requested_mask );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Verify that the affinity mask was set correctly. */

		mx_status = mx_get_process_affinity_mask( 0, &actual_mask );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( actual_mask != requested_mask ) {
			mx_warning( "The process affinity mask was set to %#lx "
			"rather than the requested value of %#lx",
				actual_mask, requested_mask );
		}
	}

	/* Initialize XCLIB. */

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: Loading EPIX configuration file '%s'.",
		fname, epix_xclib->format_file ));
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
	MX_DEBUG(-2,("%s: mxi_epix_xclib_atexit_handler_installed = %d",
			fname, mxi_epix_xclib_atexit_handler_installed ));
#endif
	/* Install the PIXCI atexit handler, if
	 * it has not already been installed.
	 */

	if ( mxi_epix_xclib_atexit_handler_installed == FALSE ) {
		int status;

		status = atexit( mxi_epix_xclib_atexit_handler );

		if ( status != 0 ) {
			return mx_error( MXE_FUNCTION_FAILED, fname,
    "Installing the EPIX PIXCI atexit handler failed for an unknown reason." );
		}

		mxi_epix_xclib_atexit_handler_installed = TRUE;

#if MXI_EPIX_XCLIB_DEBUG
		MX_DEBUG(-2,("%s: mxi_epix_xclib_atexit_handler() installed.",
			fname));
#endif
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
#endif

	/* Are we using high resolution time stamps? */

	epix_xclib->use_high_resolution_time_stamps = FALSE;

	/* Find out when this computer booted and the tick frequency
	 * for the EPIX system clock.
	 */

	mx_status = mx_get_system_boot_time(
				&(epix_xclib->system_boot_timespec) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_epix_xclib_get_timing_parameters( epix_xclib );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_epix_xclib_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_epix_xclib_close()";

	int i, length, epix_status;
	char fault_message[80];

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif
	epix_status = pxd_PIXCIclose();

	if ( epix_status < 0 ) {

		pxd_mesgFaultText(-1, fault_message, sizeof(fault_message) );

		length = strlen(fault_message);

		for ( i = 0; i < length; i++ ) {
			if ( fault_message[i] == '\n' )
				fault_message[i] = ' ';
		}

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Shutting down the PIXCI library failed for record '%s' "
		"with error code %d (%s).  Fault description = '%s'.", 
			record->name, epix_status,
			pxd_mesgErrorCode( epix_status ),
			fault_message );
	}

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_epix_xclib_resynchronize( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mxi_epix_xclib_close( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_epix_xclib_open( record );

	return mx_status;
}

/*---------------------------------------------------------------------------*/

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

/*---------------------------------------------------------------------------*/

MX_EXPORT struct timespec
mxi_epix_xclib_get_buffer_timestamp( MX_EPIX_XCLIB *epix_xclib,
					long unitmap,
					long buffer_number )
{
	static const char fname[] = "mxi_epix_xclib_get_buffer_timestamp()";

	struct timespec result, offset;
	struct pxbufstatus pxbstatus;
	uint64_t buffer64_ticks, frequency64, two_to_the_32nd_power;
	uint64_t remainder;
	mx_status_type mx_status;

	result.tv_sec = 0;
	result.tv_nsec = 0;

	if ( epix_xclib == (MX_EPIX_XCLIB *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPIX_XCLIB pointer passed was NULL." );

		return result;
	}

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: unitmap = %ld, buffer_number = %ld",
		fname, unitmap, buffer_number ));
#endif

	mx_status = mxi_epix_xclib_get_pxbufstatus( epix_xclib,
						unitmap, buffer_number,
						&pxbstatus );
	if ( mx_status.code != MXE_SUCCESS )
		return result;

	/* Use 64-bit integers to do the calculations. */

	two_to_the_32nd_power = ( 1ULL << 32 );

	frequency64 = epix_xclib->tick_frequency;

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,
	("%s: two_to_the_32nd_power = %" PRIu64 ", frequency64 = %" PRIu64,
		fname, two_to_the_32nd_power, frequency64));
#endif

	buffer64_ticks = 
		( two_to_the_32nd_power * (uint64_t) pxbstatus.captticks[1] )
			+ (uint64_t) pxbstatus.captticks[0];

#if defined( OS_LINUX )

	if ( epix_xclib->use_high_resolution_time_stamps == FALSE ) {

#if MXI_EPIX_XCLIB_DEBUG
		MX_DEBUG(-2,("%s: _Raw_ buffer64_ticks = %" PRIu64,
			fname, buffer64_ticks ));
#endif
		/* If EPIX system ticks use Linux kernel 'jiffies', then
		 * subtract the value of INITIAL_JIFFIES, so that system
		 * boot time becomes the zero time.
		 */

		buffer64_ticks = buffer64_ticks + 300000ULL;
	}
#endif

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: buffer64_ticks = %" PRIu64, fname, buffer64_ticks));

	MX_DEBUG(-2,("%s: system_boot_timespec = (%lu,%ld)", fname,
		epix_xclib->system_boot_timespec.tv_sec,
		epix_xclib->system_boot_timespec.tv_nsec));
#endif
	offset.tv_sec = (time_t) ( buffer64_ticks / frequency64 );

	remainder = buffer64_ticks % frequency64;

	offset.tv_nsec = (long) ( (1000000000ULL * remainder) / frequency64 );

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: offset = (%lu,%ld)", fname,
		offset.tv_sec, offset.tv_nsec));
#endif

	result = mx_add_high_resolution_times(
			epix_xclib->system_boot_timespec, offset );

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: result = (%lu,%ld)", fname,
		result.tv_sec, result.tv_nsec));
#endif

	return result;
}

MX_EXPORT mx_status_type
mxi_epix_xclib_get_pxvidstatus( MX_EPIX_XCLIB *epix_xclib,
					long unitmap,
					struct pxvidstatus *pxvstatus,
					int selection_mode )
{
	static const char fname[] = "mxi_epix_xclib_get_pxvidstatus()";

	struct xclibs *xc;
	int epix_status;
	mx_status_type mx_status;

	if ( epix_xclib == (MX_EPIX_XCLIB *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPIX_XCLIB pointer passed was NULL." );
	}

	if ( pxvstatus == (pxvidstatus_s *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The struct pxvidstatus pointer passed was NULL." );
	}

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'",
		fname, epix_xclib->record->name ));
	MX_DEBUG(-2,("%s: unitmap = %#lx, selection_mode = %d",
		fname, unitmap, selection_mode));
#endif
	/* Initialize the pxstatus structure. */

	memset( pxvstatus, 0, sizeof(struct pxvidstatus) );

	pxvstatus->ddch.len = sizeof(struct pxvidstatus);
	pxvstatus->ddch.mos = PXMOS_VIDSTATUS;

	/* Escape to the Structured Style Interface. */

	xc = pxd_xclibEscape(0, 0, 0);

	if ( xc == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The XCLIB library has not yet been initialized "
		"for record '%s' with pxd_PIXCIopen().",
			epix_xclib->record->name );
	}

	mx_status = MX_SUCCESSFUL_RESULT;

	epix_status = xc->pxdev.getVidStatus( &(xc->pxdev),
				unitmap, 0, pxvstatus, selection_mode );

	if ( epix_status != 0 ) {
		/* Do not return just yet since we need to call
		 * pxd_xclibEscaped() before returning.  Instead,
		 * we save the MX status and return it at the
		 * end of this function.
		 */

		mx_status = mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Error in xc->pxdev.getVidStatus() for record '%s'.  "
		"Error code = %d",
			epix_xclib->record->name, epix_status );
	}

	/* Return from the Structured Style Interface. */

	epix_status = pxd_xclibEscaped(0, 0, 0);

	if ( epix_status != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Error in pxd_xclibEscaped() for record '%s'.  "
		"Error code = %d",
			epix_xclib->record->name, epix_status );
	}

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: vcnt = %lu",
		fname, (unsigned long) pxvstatus->time.vcnt));
	MX_DEBUG(-2,("%s: hcnt = %lu",
		fname, (unsigned long) pxvstatus->time.hcnt));
	MX_DEBUG(-2,("%s: ticks = [%lu,%lu]", fname,
		(unsigned long) pxvstatus->time.ticks[0],
		(unsigned long) pxvstatus->time.ticks[1]));
	MX_DEBUG(-2,("%s: ticku = [%lu,%lu]", fname,
		(unsigned long) pxvstatus->time.ticku[0],
		(unsigned long) pxvstatus->time.ticku[1]));
	MX_DEBUG(-2,("%s: field = %lu",
		fname, (unsigned long) pxvstatus->time.field));
	MX_DEBUG(-2,("%s: fieldmod = %lu",
		fname, (unsigned long) pxvstatus->time.fieldmod));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_epix_xclib_get_pxbufstatus( MX_EPIX_XCLIB *epix_xclib,
					long unitmap,
					long buffer_number,
					struct pxbufstatus *pxbstatus )
{
	static const char fname[] = "mxi_epix_xclib_get_pxbufstatus()";

	struct xclibs *xc;
	int epix_status;
	mx_status_type mx_status;

	if ( epix_xclib == (MX_EPIX_XCLIB *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPIX_XCLIB pointer passed was NULL." );
	}

	if ( pxbstatus == (pxbufstatus_s *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The struct pxvidstatus pointer passed was NULL." );
	}

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'",
		fname, epix_xclib->record->name ));
	MX_DEBUG(-2,("%s: unitmap = %#lx, buffer_number = %ld",
		fname, unitmap, buffer_number));
#endif
	/* Initialize the pxbstatus structure. */

	memset( pxbstatus, 0, sizeof(struct pxbufstatus) );

	pxbstatus->ddch.len = sizeof(struct pxbufstatus);
	pxbstatus->ddch.mos = PXMOS_BUFSTATUS;

#if 1
	MX_DEBUG(-2,("%s: BEFORE pxd_goneLive() = %d",
		fname, pxd_goneLive(1,0)));
#endif

	/* Escape to the Structured Style Interface. */

	xc = pxd_xclibEscape(0, 0, 0);

	if ( xc == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The XCLIB library has not yet been initialized "
		"for record '%s' with pxd_PIXCIopen().",
			epix_xclib->record->name );
	}

	mx_status = MX_SUCCESSFUL_RESULT;

	epix_status = xc->pxlib.goingBufStatus( &(xc->pxlib),
				0, unitmap, buffer_number, pxbstatus );

	if ( epix_status != 0 ) {
		/* Do not return just yet since we need to call
		 * pxd_xclibEscaped() before returning.  Instead,
		 * we save the MX status and return it at the
		 * end of this function.
		 */

		mx_status = mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Error in xc->pxlib.goingBufStatus() for record '%s'.  "
		"Error code = %d",
			epix_xclib->record->name, epix_status );
	}

#if 0      /* WML - calling pxd_xclibEscaped() is not necessary
            *       or useful here.
            */

	/* Return from the Structured Style Interface. */

	epix_status = pxd_xclibEscaped(0, 0, 0);

	if ( epix_status != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Error in pxd_xclibEscaped() for record '%s'.  "
		"Error code = %d",
			epix_xclib->record->name, epix_status );
	}
#endif     /* WML */

#if 1
	MX_DEBUG(-2,("%s: AFTER pxd_goneLive() = %d",
		fname, pxd_goneLive(1,0)));
#endif

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: stateid = %d", fname, pxbstatus->stateid));
	MX_DEBUG(-2,("%s: tracker = %d", fname, pxbstatus->tracker));
	MX_DEBUG(-2,("%s: captvcnt = %d", fname, (int) pxbstatus->captvcnt));
	MX_DEBUG(-2,("%s: captticks = [%lu,%lu]", fname,
		(unsigned long) pxbstatus->captticks[0],
		(unsigned long) pxbstatus->captticks[1]));
	MX_DEBUG(-2,("%s: captgpin = %d", fname, pxbstatus->captgpin));
#endif

	return mx_status;
}

#endif /* HAVE_EPIX_XCLIB */

