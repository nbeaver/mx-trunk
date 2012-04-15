/*
 * Name:    i_sapera_lt.c
 *
 * Purpose: MX driver for DALSA's Sapera LT camera interface.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011-2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_SAPERA_LT_DEBUG		FALSE

#define MXI_SAPERA_LT_DEBUG_OPEN	TRUE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "i_sapera_lt.h"

MX_RECORD_FUNCTION_LIST mxi_sapera_lt_record_function_list = {
	NULL,
	mxi_sapera_lt_create_record_structures,
	mxi_sapera_lt_finish_record_initialization,
	NULL,
	NULL,
	mxi_sapera_lt_open,
	mxi_sapera_lt_close
};

MX_RECORD_FIELD_DEFAULTS mxi_sapera_lt_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_SAPERA_LT_STANDARD_FIELDS
};

long mxi_sapera_lt_num_record_fields
		= sizeof( mxi_sapera_lt_record_field_defaults )
			/ sizeof( mxi_sapera_lt_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_sapera_lt_rfield_def_ptr
			= &mxi_sapera_lt_record_field_defaults[0];

static mx_status_type
mxi_sapera_lt_get_pointers( MX_RECORD *record,
				MX_SAPERA_LT **sapera_lt,
				const char *calling_fname )
{
	static const char fname[] = "mxi_sapera_lt_get_pointers()";

	MX_SAPERA_LT *sapera_lt_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	sapera_lt_ptr = (MX_SAPERA_LT *) record->record_type_struct;

	if ( sapera_lt_ptr == (MX_SAPERA_LT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SAPERA_LT pointer for record '%s' is NULL.",
			record->name );
	}

	if ( sapera_lt != (MX_SAPERA_LT **) NULL ) {
		*sapera_lt = sapera_lt_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

void
mxi_sapera_lt_error_callback( SapManCallbackInfo *info )
{
	const char *sapera_error_message = info->GetErrorMessage();

	fprintf( stderr, "SAPERA LT ERROR: %s\n", sapera_error_message );
}

/*------*/

MX_EXPORT mx_status_type
mxi_sapera_lt_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_sapera_lt_create_record_structures()";

	MX_SAPERA_LT *sapera_lt;

	/* Allocate memory for the necessary structures. */

	sapera_lt = (MX_SAPERA_LT *) malloc( sizeof(MX_SAPERA_LT) );

	if ( sapera_lt == (MX_SAPERA_LT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_SAPERA_LT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = sapera_lt;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	sapera_lt->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sapera_lt_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_sapera_lt_finish_record_initialization()";

	MX_SAPERA_LT *sapera_lt;
	BOOL sapera_status;
	mx_status_type mx_status;

	mx_status = mxi_sapera_lt_get_pointers( record, &sapera_lt, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure Sapera LT error reporting so that it invokes
	 * a callback.
	 */

	sapera_status = SapManager::SetDisplayStatusMode(
						SapManager::StatusCallback,
						mxi_sapera_lt_error_callback,
						record );

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to set up a Sapera LT error reporting callback "
		"for record '%s' failed.", record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sapera_lt_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_sapera_lt_open()";

	MX_SAPERA_LT *sapera_lt;
	mx_status_type mx_status;

	mx_status = mxi_sapera_lt_get_pointers( record, &sapera_lt, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_SAPERA_LT_DEBUG_OPEN
	MX_DEBUG(-2,("%s invoked for '%s'.", fname, record->name));
#endif
	/* How many servers are available?  This number will always be
	 * at least 1, since this counts the System server.
	 */

	int num_servers = SapManager::GetServerCount();

#if MXI_SAPERA_LT_DEBUG_OPEN
	MX_DEBUG(-2,("%s: num_servers = %d", fname, num_servers));

	int i;
	BOOL result;
	char server_name[80];

	for ( i = 0; i < num_servers; i++ ) {
		result = SapManager::GetServerName( i, server_name );

		if ( result == FALSE ) {
			MX_DEBUG(-2,("%s: server %d = Error", fname, i));
		} else {
			MX_DEBUG(-2,("%s: server %d = '%s'",
					fname, i, server_name));
		}
	}
#endif

	/* Verify that this server is really present by using the
	 * SapManager::GetServerIndex() call.  If the server does not
	 * exist, then we should get an error.
	 */

	sapera_lt->server_index =
		SapManager::GetServerIndex( sapera_lt->server_name );

#if MXI_SAPERA_LT_DEBUG_OPEN
	MX_DEBUG(-2,("%s: server_index = %d", fname, sapera_lt->server_index ));
#endif

	if ( sapera_lt->server_index == SapLocation::ServerUnknown ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"Sapera server '%s' could not be found.",
			sapera_lt->server_name );
	}

	sapera_lt->num_frame_grabbers =
		SapManager::GetResourceCount( sapera_lt->server_name,
						SapManager::ResourceAcq );
#if MXI_SAPERA_LT_DEBUG_OPEN
	MX_DEBUG(-2,("%s: server '%s' num_frame_grabbers = %ld",
		fname, sapera_lt->server_name, sapera_lt->num_frame_grabbers));

	char resource_name[80];

	for ( i = 0; i < sapera_lt->num_frame_grabbers; i++ ) {
		result = SapManager::GetResourceName( sapera_lt->server_index,
						SapManager::ResourceAcq,
						i, resource_name );
		if ( result == FALSE ) {
			MX_DEBUG(-2,
			("%s: server '%s', frame grabber %d = Error",
				fname, sapera_lt->server_name, i));
		} else {
			MX_DEBUG(-2,("%s: server '%s', frame grabber %d = '%s'",
				fname, sapera_lt->server_name,
				i, resource_name ));
		}
	}
#endif

	sapera_lt->num_cameras =
		SapManager::GetResourceCount( sapera_lt->server_name,
						SapManager::ResourceAcqDevice );
#if MXI_SAPERA_LT_DEBUG_OPEN
	MX_DEBUG(-2,("%s: server '%s' num_cameras = %ld",
		fname, sapera_lt->server_name, sapera_lt->num_cameras));

	for ( i = 0; i < sapera_lt->num_cameras; i++ ) {
		result = SapManager::GetResourceName( sapera_lt->server_index,
						SapManager::ResourceAcqDevice,
						i, resource_name );
		if ( result == FALSE ) {
			MX_DEBUG(-2,("%s: server '%s', camera %d = Error",
				fname, sapera_lt->server_name, i));
		} else {
			MX_DEBUG(-2,("%s: server '%s', camera %d = '%s'",
				fname, sapera_lt->server_name,
				i, resource_name ));
		}
	}
#endif

	sapera_lt->max_devices =
		sapera_lt->num_frame_grabbers + sapera_lt->num_cameras;

	if ( sapera_lt->max_devices <= 0 ) {
		return mx_error( MXE_HARDWARE_CONFIGURATION_ERROR, fname,
		"Sapera server '%s' does not have any acquisition devices!",
			sapera_lt->server_name );
	}

	sapera_lt->device_record_array = (MX_RECORD **)
			calloc( sapera_lt->max_devices, sizeof(MX_RECORD *) );

	if ( sapera_lt->device_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of MX_RECORD pointers.", sapera_lt->max_devices );
	}

#if MXI_SAPERA_LT_DEBUG_OPEN
	MX_DEBUG(-2,("%s complete for '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sapera_lt_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_sapera_lt_close()";

	MX_SAPERA_LT *sapera_lt;
	mx_status_type mx_status;

	mx_status = mxi_sapera_lt_get_pointers( record,
						&sapera_lt, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_SAPERA_LT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/
