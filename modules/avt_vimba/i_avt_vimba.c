/*
 * Name:    i_avt_vimba.c
 *
 * Purpose: MX interface driver for the Vimba C API used by
 *          cameras from Allied Vision Technologies.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013-2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_AVT_VIMBA_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"

#include "i_avt_vimba.h"

MX_RECORD_FUNCTION_LIST mxi_avt_vimba_record_function_list = {
	NULL,
	mxi_avt_vimba_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_avt_vimba_open,
	mxi_avt_vimba_close
};

MX_RECORD_FIELD_DEFAULTS mxi_avt_vimba_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_AVT_VIMBA_STANDARD_FIELDS
};

long mxi_avt_vimba_num_record_fields
		= sizeof( mxi_avt_vimba_record_field_defaults )
			/ sizeof( mxi_avt_vimba_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_avt_vimba_rfield_def_ptr
			= &mxi_avt_vimba_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_avt_vimba_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_avt_vimba_create_record_structures()";

	MX_AVT_VIMBA *avt_vimba;

	/* Allocate memory for the necessary structures. */

	avt_vimba = (MX_AVT_VIMBA *) malloc( sizeof(MX_AVT_VIMBA) );

	if ( avt_vimba == (MX_AVT_VIMBA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AVT_VIMBA structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = avt_vimba;

	record->record_function_list = &mxi_avt_vimba_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	avt_vimba->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_avt_vimba_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_avt_vimba_open()";

	MX_AVT_VIMBA *avt_vimba = NULL;
	unsigned long camera_array_size;

	VmbVersionInfo_t version_info;
	VmbUint32_t num_cameras = 0;
	VmbCameraInfo_t *camera_info = NULL;
	VmbBool_t using_gigabit_ethernet = FALSE;
	VmbUint32_t i;
	VmbError_t vmb_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	avt_vimba = (MX_AVT_VIMBA *) record->record_type_struct;

	if ( avt_vimba == (MX_AVT_VIMBA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_AVT_VIMBA pointer for record '%s' is NULL.", record->name);
	}

#if MXI_AVT_VIMBA_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif
	vmb_status = VmbVersionQuery( &version_info, sizeof(version_info) );

	if ( vmb_status != VmbErrorSuccess ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The attempt to get the version number of the "
		"Vimba C API failed with error code %lu.",
			(unsigned long) vmb_status );
	}

#if MXI_AVT_VIMBA_DEBUG
	MX_DEBUG(-2,("%s: Using Vimba C API version %lu.%lu.%lu", fname,
		(unsigned long) version_info.major,
		(unsigned long) version_info.minor,
		(unsigned long) version_info.patch ));
#endif
	avt_vimba->vimba_version = 1000000L * version_info.major
				+ 1000L * version_info.minor
				+ version_info.patch;

	/* Initialize the Vimba C API. */

	vmb_status = VmbStartup();

	switch( vmb_status ) {
	case VmbErrorSuccess:
		break;
	case VmbErrorInternalFault:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"VmbStartup() failed with Internal fault." );
		break;
	case VmbErrorNoTL:
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"No transport layers were found by the Vimba C API." );
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"VmbStartup() failed with error %d",
			(int) vmb_status );
		break;
	}

	/*--- Get and display the list of cameras. ---*/

	/* Are we using Gigabit Ethernet? */

	vmb_status = VmbFeatureBoolGet( gVimbaHandle, "GeVTLIsPresent",
					&using_gigabit_ethernet );

	if ( vmb_status != VmbErrorSuccess ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"An attempt to detect the presence of the GigE Vision "
		"transport layer failed with error %d",
			(int) vmb_status );
	}

	if ( using_gigabit_ethernet == FALSE ) {
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"The GigE Vision transport layer is not present." );
	}

	/* Start network discovery to find all conected GigE Vision cameras. */

	vmb_status = VmbFeatureCommandRun(gVimbaHandle, "GeVDiscoveryAllOnce");

	if ( vmb_status != VmbErrorSuccess ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"An attempt to start GigE Vision camera discovery "
		"failed with error code %d", (int) vmb_status );
	}

	/* Wait 0.2 seconds for the cameras to respond. */

	mx_msleep( 200 );

	/* Find out how many cameras are available. */

	vmb_status = 
	    VmbCamerasList( NULL, 0, &num_cameras, sizeof(*camera_info) );

	if ( vmb_status != VmbErrorSuccess ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"An attempt to get the number of discovered cameras "
		"failed with error code %d.",
				(int) vmb_status );
	}

	avt_vimba->num_cameras = num_cameras;

#if MXI_AVT_VIMBA_DEBUG
	MX_DEBUG(-2,("%s: %lu cameras found.", fname, avt_vimba->num_cameras ));
#endif
	if ( avt_vimba->num_cameras == 0 ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"No AVT Vimba cameras were found for record '%s'.",
			record->name );
	}

	camera_array_size = num_cameras * sizeof( *camera_info );

	camera_info = (VmbCameraInfo_t *) malloc(camera_array_size);

	if ( camera_info == (VmbCameraInfo_t *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a "
		"%d camera info array for record '%s'.",
			num_cameras, record->name );
	}

	/* Fill in the camera info array. */

	vmb_status = VmbCamerasList( camera_info,
					avt_vimba->num_cameras,
					&num_cameras,
					camera_array_size );

	if ( vmb_status != VmbErrorSuccess ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"An attempt to get a list of discovered cameras "
		"failed with error code %d.",
				(int) vmb_status );
	}

	/* The number of cameras might have changed between the two calls
	 * to VmbCameraList().
	 */

	avt_vimba->num_cameras = num_cameras;

	avt_vimba->camera_info = camera_info;

	/* Print the camera info out. */

	for ( i = 0; i < num_cameras; i++ ) {
		MX_DEBUG(-2,("%s: Camera %d", fname, i));
		MX_DEBUG(-2,("%s:   Camera Name: '%s'",
			fname, avt_vimba->camera_info[i].cameraName));
		MX_DEBUG(-2,("%s:   Model Name: '%s'",
			fname, avt_vimba->camera_info[i].modelName));
		MX_DEBUG(-2,("%s:   Camera ID: '%s'",
			fname, avt_vimba->camera_info[i].cameraIdString));
		MX_DEBUG(-2,("%s:   Serial Number: '%s'",
			fname, avt_vimba->camera_info[i].serialString));
		MX_DEBUG(-2,("%s:   Permitted Access: %#x",
			fname, avt_vimba->camera_info[i].permittedAccess));
		MX_DEBUG(-2,("%s:   @ Interface ID: '%s'",
			fname, avt_vimba->camera_info[i].interfaceIdString));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_avt_vimba_close( MX_RECORD *record )
{
#if MXI_AVT_VIMBA_DEBUG
	static const char fname[] = "mxi_avt_vimba_close()";

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif

	VmbShutdown();

	return MX_SUCCESSFUL_RESULT;
}

