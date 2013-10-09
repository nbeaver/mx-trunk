/*
 * Name:    i_labjack_ux.c
 *
 * Purpose: MX interface driver for the LabJack U3, U6, and U9 devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_LABJACK_UX_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_record.h"
#include "i_labjack_ux.h"

MX_RECORD_FUNCTION_LIST mxi_labjack_ux_record_function_list = {
	NULL,
	mxi_labjack_ux_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_labjack_ux_open,
	mxi_labjack_ux_close
};

MX_RECORD_FIELD_DEFAULTS mxi_labjack_ux_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_LABJACK_UX_STANDARD_FIELDS
};

long mxi_labjack_ux_num_record_fields
		= sizeof( mxi_labjack_ux_record_field_defaults )
			/ sizeof( mxi_labjack_ux_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_labjack_ux_rfield_def_ptr
			= &mxi_labjack_ux_record_field_defaults[0];

static mx_status_type
mxi_labjack_ux_get_pointers( MX_RECORD *record,
				MX_LABJACK_UX **labjack_ux,
				const char *calling_fname )
{
	static const char fname[] = "mxi_labjack_ux_get_pointers()";

	MX_LABJACK_UX *labjack_ux_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	labjack_ux_ptr = (MX_LABJACK_UX *) record->record_type_struct;

	if ( labjack_ux_ptr == (MX_LABJACK_UX *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LABJACK_UX pointer for record '%s' is NULL.",
			record->name );
	}

	if ( labjack_ux != (MX_LABJACK_UX **) NULL ) {
		*labjack_ux = labjack_ux_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxi_labjack_ux_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_labjack_ux_create_record_structures()";

	MX_LABJACK_UX *labjack_ux;

	/* Allocate memory for the necessary structures. */

	labjack_ux = (MX_LABJACK_UX *) malloc( sizeof(MX_LABJACK_UX) );

	if ( labjack_ux == (MX_LABJACK_UX *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_LABJACK_UX structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = labjack_ux;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	labjack_ux->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_labjack_ux_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_labjack_ux_open()";

	MX_LABJACK_UX *labjack_ux;
	int i, length;
	mx_status_type mx_status;

	mx_status = mxi_labjack_ux_get_pointers( record, &labjack_ux, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	length = strlen( labjack_ux->product_name );

	for ( i = 0; i < length; i++ ) {
		if ( islower( labjack_ux->product_name[i] ) ) {
			labjack_ux->product_name[i] =
				toupper( labjack_ux->product_name[i] );
		}
	}

	if ( strcmp( labjack_ux->product_name, "U3" ) == 0 ) {
		labjack_ux->product_id = U3_PRODUCT_ID;
	} else
	if ( strcmp( labjack_ux->product_name, "U6" ) == 0 ) {
		labjack_ux->product_id = U6_PRODUCT_ID;
	} else
	if ( strcmp( labjack_ux->product_name, "UE9" ) == 0 ) {
		labjack_ux->product_id = UE9_PRODUCT_ID;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"LabJack product '%s' is not supported by this driver.",
			labjack_ux->product_name );
	}

#if defined(OS_WIN32)
#else /* NOT defined(OS_WIN32) */

	if ( labjack_ux->labjack_flags & MXF_LABJACK_UX_SHOW_INFO ) {
		mx_info( "LabJack library version = %f",
			LJUSB_GetLibraryVersion() );

		mx_info( "This computer has %u '%s' devices connected.",
			LJUSB_GetDevCount( labjack_ux->product_id ),
			labjack_ux->product_name );
	}

#endif /* NOT defined(OS_WIN32) */

#if defined(OS_WIN32)
#else
	labjack_ux->handle = LJUSB_OpenDevice( labjack_ux->device_number,
						0, labjack_ux->product_id );

	if ( labjack_ux->handle == NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"The attempt by record '%s' to connect to device %lu "
		"(product '%s') failed.",
			record->name,
			labjack_ux->device_number,
			labjack_ux->product_name );
	}

#if MXI_LABJACK_UX_DEBUG
	MX_DEBUG(-2,("%s: handle = %p", fname, labjack_ux->handle));
#endif

#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_labjack_ux_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_labjack_ux_close()";

	MX_LABJACK_UX *labjack_ux;
	mx_status_type mx_status;

	mx_status = mxi_labjack_ux_get_pointers( record, &labjack_ux, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if defined( OS_WIN32 )
	if ( labjack_ux->handle != INVALID_HANDLE ) {
		Close( labjack_ux->handle );
	}
#else
	if ( LJUSB_IsHandleValid( labjack_ux->handle ) ) {
		LJUSB_CloseDevice( labjack_ux->handle );
	}
#endif
	return mx_status;
}

