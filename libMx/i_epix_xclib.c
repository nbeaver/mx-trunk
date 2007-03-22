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
#include <stdlib.h>

#include "mxconfig.h"

#if HAVE_EPIX_XCLIB

#include "mx_util.h"
#include "mx_record.h"
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

	epix_xclib->open_time = time(NULL);

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

	MX_DEBUG(-2,("%s: open time = %lu seconds",
		fname, epix_xclib->open_time));
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

	return buffer;

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

#endif /* HAVE_EPIX_XCLIB */

