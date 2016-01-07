/*
 * Name:    i_dalsa_gev.c
 *
 * Purpose: MX driver for DALSA's Sapera LT camera interface.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_DALSA_GEV_DEBUG		FALSE

#define MXI_DALSA_GEV_DEBUG_OPEN	FALSE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "i_dalsa_gev.h"

MX_RECORD_FUNCTION_LIST mxi_dalsa_gev_record_function_list = {
	NULL,
	mxi_dalsa_gev_create_record_structures,
	mxi_dalsa_gev_finish_record_initialization,
	NULL,
	NULL,
	mxi_dalsa_gev_open,
	mxi_dalsa_gev_close
};

MX_RECORD_FIELD_DEFAULTS mxi_dalsa_gev_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_DALSA_GEV_STANDARD_FIELDS
};

long mxi_dalsa_gev_num_record_fields
		= sizeof( mxi_dalsa_gev_record_field_defaults )
			/ sizeof( mxi_dalsa_gev_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_dalsa_gev_rfield_def_ptr
			= &mxi_dalsa_gev_record_field_defaults[0];

static mx_status_type
mxi_dalsa_gev_get_pointers( MX_RECORD *record,
				MX_DALSA_GEV **dalsa_gev,
				const char *calling_fname )
{
	static const char fname[] = "mxi_dalsa_gev_get_pointers()";

	MX_DALSA_GEV *dalsa_gev_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	dalsa_gev_ptr = (MX_DALSA_GEV *) record->record_type_struct;

	if ( dalsa_gev_ptr == (MX_DALSA_GEV *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DALSA_GEV pointer for record '%s' is NULL.",
			record->name );
	}

	if ( dalsa_gev != (MX_DALSA_GEV **) NULL ) {
		*dalsa_gev = dalsa_gev_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxi_dalsa_gev_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_dalsa_gev_create_record_structures()";

	MX_DALSA_GEV *dalsa_gev;

	/* Allocate memory for the necessary structures. */

	dalsa_gev = (MX_DALSA_GEV *) malloc( sizeof(MX_DALSA_GEV) );

	if ( dalsa_gev == (MX_DALSA_GEV *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_DALSA_GEV structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = dalsa_gev;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	dalsa_gev->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_dalsa_gev_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_dalsa_gev_finish_record_initialization()";

	MX_DALSA_GEV *dalsa_gev;
	mx_status_type mx_status;

	mx_status = mxi_dalsa_gev_get_pointers( record, &dalsa_gev, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_dalsa_gev_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_dalsa_gev_open()";

	MX_DALSA_GEV *dalsa_gev;
	long i, gev_status;
	int num_cameras_found;
	mx_status_type mx_status;

	mx_status = mxi_dalsa_gev_get_pointers( record, &dalsa_gev, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_DALSA_GEV_DEBUG_OPEN
	MX_DEBUG(-2,("%s invoked for '%s'.", fname, record->name));
#endif
	/* Initialize the GigE-V Framework. */

	gev_status = GevApiInitialize();

	switch( gev_status ) {
	case GEVLIB_OK:
		break;
	case GEVLIB_ERROR_INSUFFICIENT_MEMORY:
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to initialize the GeV API "
		"Framework for record '%s'.",
			record->name );
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"GevApiInitialize() returned %ld for record '%s'.",
			gev_status, record->name );
		break;
	}

	/* How many cameras are available? */

	dalsa_gev->num_cameras = GevDeviceCount();

	/* Allocate memory for an array of GEV_CAMERA_INFO objects
	 * for each camera.
	 */

	dalsa_gev->camera_array = calloc( dalsa_gev->num_cameras,
					sizeof(GEV_CAMERA_INFO) );

	if ( dalsa_gev->camera_array == (GEV_CAMERA_INFO *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of GEV_CAMERA_INFO objects for record '%s'.",
			dalsa_gev->num_cameras, record->name );
	}

	/* Get the list of cameras. */

	gev_status = GevGetCameraList( dalsa_gev->camera_array,
					dalsa_gev->num_cameras,
					&num_cameras_found );
	switch( gev_status ) {
	case GEVLIB_OK:
		break;
	default:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"A call to GevGetCAmeraList() requesting a list of %ld "
		"cameras for record '%s' failed with status = %ld.",
			dalsa_gev->num_cameras,
			record->name, gev_status );
		break;
	}

	if ( num_cameras_found < dalsa_gev->num_cameras ) {
		dalsa_gev->num_cameras = num_cameras_found;
	}

	/* If requested, show all the cameras that were found. */

	if ( dalsa_gev->dalsa_gev_flags & MXF_DALSA_GEV_SHOW_CAMERA_LIST ) {

	    mx_info( "Interface '%s': num_cameras = %ld",
		record->name, dalsa_gev->num_cameras );

	    for ( i = 0; i < dalsa_gev->num_cameras; i++ ) {
		mx_info( "Camera %ld: IP address %lu, serial '%s'.",
			i, (unsigned long) dalsa_gev->camera_array[i].ipAddr,
			dalsa_gev->camera_array[i].serial );
	    }
	}

	/* Verify that this server is really present by using the
	 * SapManager::GetServerIndex() call.  If the server does not
	 * exist, then we should get an error.
	 */

	/* Allocate an array of MX_RECORD pointers to keep copies of the
	 * camera record pointers in.
	 */

	dalsa_gev->device_record_array = (MX_RECORD **)
			calloc( dalsa_gev->num_cameras, sizeof(MX_RECORD *) );

	if ( dalsa_gev->device_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of MX_RECORD pointers.", dalsa_gev->num_cameras );
	}

#if MXI_DALSA_GEV_DEBUG_OPEN
	MX_DEBUG(-2,("%s complete for '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_dalsa_gev_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_dalsa_gev_close()";

	MX_DALSA_GEV *dalsa_gev;
	mx_status_type mx_status;

	mx_status = mxi_dalsa_gev_get_pointers( record, &dalsa_gev, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_DALSA_GEV_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	(void) GevApiUninitialize();

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/
