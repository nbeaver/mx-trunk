/*
 * Name:    i_pleora_iport.c
 *
 * Purpose: MX driver for the Pleora iPORT camera interface.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_PLEORA_IPORT_DEBUG		TRUE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "i_pleora_iport.h"

MX_RECORD_FUNCTION_LIST mxi_pleora_iport_record_function_list = {
	NULL,
	mxi_pleora_iport_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_pleora_iport_open,
	mxi_pleora_iport_close
};

MX_RECORD_FIELD_DEFAULTS mxi_pleora_iport_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS
};

long mxi_pleora_iport_num_record_fields
		= sizeof( mxi_pleora_iport_record_field_defaults )
			/ sizeof( mxi_pleora_iport_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_pleora_iport_rfield_def_ptr
			= &mxi_pleora_iport_record_field_defaults[0];

static mx_status_type
mxi_pleora_iport_get_pointers( MX_RECORD *record,
				MX_PLEORA_IPORT **pleora_iport,
				const char *calling_fname )
{
	static const char fname[] = "mxi_pleora_iport_get_pointers()";

	MX_PLEORA_IPORT *pleora_iport_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	pleora_iport_ptr = (MX_PLEORA_IPORT *) record->record_type_struct;

	if ( pleora_iport_ptr == (MX_PLEORA_IPORT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PLEORA_IPORT pointer for record '%s' is NULL.",
			record->name );
	}

	if ( pleora_iport != (MX_PLEORA_IPORT **) NULL ) {
		*pleora_iport = pleora_iport_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxi_pleora_iport_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_pleora_iport_create_record_structures()";

	MX_PLEORA_IPORT *pleora_iport;

	/* Allocate memory for the necessary structures. */

	pleora_iport = (MX_PLEORA_IPORT *) malloc( sizeof(MX_PLEORA_IPORT) );

	if ( pleora_iport == (MX_PLEORA_IPORT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_PLEORA_IPORT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = pleora_iport;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	pleora_iport->record = record;

	return MX_SUCCESSFUL_RESULT;
}

#define MXI_PLEORA_IPORT_ARRAY_BLOCK_SIZE	10

static int
mxi_pleora_iport_finder_callback( struct CyDeviceEntry *entry,
					void *aContext )
{
	static const char fname[] = "mxi_pleora_iport_finder_callback()";

	MX_PLEORA_IPORT *pleora_iport;
	long new_max;
	void *new_ptr;

	if ( aContext == NULL ) {
		(void) mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The context pointer supplied was NULL." );
		return 0;
	}

	pleora_iport = (MX_PLEORA_IPORT *) aContext;

#if MXI_PLEORA_IPORT_DEBUG
	MX_DEBUG(-2,("%s invoked for Pleora iPORT record '%s'.",
		fname, pleora_iport->record->name ));

	MX_DEBUG(-2,("%s: entry device name = '%s'.",
		fname, entry->mDeviceName ));
#endif

	if ( pleora_iport->num_devices >= pleora_iport->max_num_devices ) {
		new_max = pleora_iport->max_num_devices
				+ MXI_PLEORA_IPORT_ARRAY_BLOCK_SIZE;

		new_ptr = realloc( pleora_iport->device_array,
				new_max * sizeof(struct CyDeviceEntry *) );

		if ( new_ptr == NULL ) {
			(void) mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to reallocate a %ld element "
			"array of CyDeviceEntry pointer for record '%s'.",
				new_max, pleora_iport->record->name );
			return 0;
		}

		pleora_iport->max_num_devices = new_max;
		pleora_iport->device_array = new_ptr;
	}

	pleora_iport->device_array[ pleora_iport->num_devices ] = entry;

	pleora_iport->num_devices++;

	return 1;
}

MX_EXPORT mx_status_type
mxi_pleora_iport_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_pleora_iport_open()";

	MX_PLEORA_IPORT *pleora_iport;
	unsigned int timeout_ms;
	mx_status_type mx_status;

	CyResult cy_result;

	unsigned long access_mode[] = {
		CY_DEVICE_ACCESS_MODE_DRV,
		CY_DEVICE_ACCESS_MODE_EBUS,
		CY_DEVICE_ACCESS_MODE_GEV_EBUS };

	unsigned long num_access_modes = sizeof(access_mode)
					/ sizeof(access_mode[0]);
	unsigned long i;

	mx_status = mxi_pleora_iport_get_pointers( record,
						&pleora_iport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pleora_iport->max_num_devices = MXI_PLEORA_IPORT_ARRAY_BLOCK_SIZE;
	pleora_iport->num_devices = 0;

	pleora_iport->device_array = (struct CyDeviceEntry **)
    malloc( pleora_iport->max_num_devices * sizeof(struct CyDeviceEntry *) );

	if ( pleora_iport->device_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for a %ld element array of "
		"CyDeviceEntry pointers for record '%s'.",
			pleora_iport->max_num_devices, record->name );
	}

	pleora_iport->finder_handle = CyDeviceFinder_Init( 0, 0 );

#if MXI_PLEORA_IPORT_DEBUG
	MX_DEBUG(-2,("%s: finder_handle = %p",
		fname, pleora_iport->finder_handle));
#endif
	timeout_ms = 10;	/* in milliseconds */

	for ( i = 0; i < num_access_modes; i++ ) {

#if MXI_PLEORA_IPORT_DEBUG
		MX_DEBUG(-2,("%s: access_mode[%lu] = %lu",
			fname, i, access_mode[i]));
		MX_DEBUG(-2,("%s: ***  num_devices (before) = %ld",
			fname, pleora_iport->num_devices));
#endif
		cy_result = CyDeviceFinder_Find(
					pleora_iport->finder_handle,
					access_mode[i],
					timeout_ms,
					mxi_pleora_iport_finder_callback,
					pleora_iport );

#if MXI_PLEORA_IPORT_DEBUG
		MX_DEBUG(-2,("%s: cy_result = %d", fname, cy_result));
		MX_DEBUG(-2,("%s: ***  num_devices (after) = %ld",
			fname, pleora_iport->num_devices));
#endif
		switch( cy_result ) {
		case CY_RESULT_OK:
		case CY_RESULT_NO_AVAILABLE_DATA:
		case CY_RESULT_NOT_SUPPORTED:
			break;
		case CY_RESULT_NETWORK_ERROR:
			return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"A network error occurred while looking for "
			"Pleora iPORT devices on behalf of record '%s'.",
				record->name );
			break;
		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"An unknown error occurred while looking for "
			"Pleora iPORT devices on behalf of record '%s'.",
				record->name );
			break;
		}
#if MXI_PLEORA_IPORT_DEBUG
		MX_DEBUG(-2,("%s: End of loop, i = %lu", fname, i));
#endif
	}

#if MXI_PLEORA_IPORT_DEBUG
	MX_DEBUG(-2,("%s: %ld devices found for '%s'",
		fname, pleora_iport->num_devices, record->name));
#endif

	if ( pleora_iport->num_devices ) {
		mx_warning(
		"No Pleora iPORT cameras were found on the network." );

#if MXI_PLEORA_IPORT_DEBUG
	} else {
		struct CyDeviceEntry *device_entry;

		MX_DEBUG(-2,("%s: Pleora iPORT cameras:", fname));

		for ( i = 0; i < pleora_iport->num_devices; i++ ) {
			device_entry = pleora_iport->device_array[i];

			MX_DEBUG(-2,("%s:   %s, %s, %s, %s",
				fname,
				device_entry->mDeviceName,
				device_entry->mModelName,
				device_entry->mManufacturerName,
				device_entry->mAddressIP ));
		}
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pleora_iport_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_pleora_iport_close()";

	MX_PLEORA_IPORT *pleora_iport;
	mx_status_type mx_status;

	mx_status = mxi_pleora_iport_get_pointers( record,
						&pleora_iport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pleora_iport->finder_handle != NULL ) {
		CyDeviceFinder_Destroy( pleora_iport->finder_handle );

		pleora_iport->finder_handle = NULL;
	}

	return MX_SUCCESSFUL_RESULT;
}

