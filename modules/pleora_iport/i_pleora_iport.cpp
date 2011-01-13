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

MX_EXPORT mx_status_type
mxi_pleora_iport_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_pleora_iport_open()";

	MX_PLEORA_IPORT *pleora_iport;
	long i, num_devices;
	mx_status_type mx_status;

	CyDeviceFinder finder;
	CyDeviceFinder::DeviceList *ip_engine_list;
	/* const CyDeviceFinder::DeviceEntry *device_entry; */
	CyResult cy_result;

	mx_status = mxi_pleora_iport_get_pointers( record,
						&pleora_iport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ip_engine_list = new CyDeviceFinder::DeviceList();

	/* Find IP engines available through the eBUS driver. */

	finder.Find( CY_DEVICE_ACCESS_MODE_EBUS,
			*ip_engine_list,
			100,
			true );

	/* Find GigE Vision IP engines available through the eBUS driver. */

	finder.Find( CY_DEVICE_ACCESS_MODE_GEV_EBUS,
			*ip_engine_list,
			100,
			true );

	/* Find IP engines available through the High Performance driver. */

	finder.Find( CY_DEVICE_ACCESS_MODE_DRV,
			*ip_engine_list,
			100,
			true );

	/* Find IP engines available through the Network Stack. */

	finder.Find( CY_DEVICE_ACCESS_MODE_UDP,
			*ip_engine_list,
			100,
			true );

	pleora_iport->ip_engine_list = ip_engine_list;

	num_devices = ip_engine_list->size();

	pleora_iport->num_devices = num_devices;

#if MXI_PLEORA_IPORT_DEBUG
	MX_DEBUG(-2,("%s: %d IP engines found for record '%s'.",
		fname, num_devices, record->name ));

	MX_DEBUG(-2,("%s: ip_engine_list = %p", fname, ip_engine_list));
#endif

	CyDeviceFinder::DeviceList ip_engine_list_obj = *ip_engine_list;

	for ( i = 0; i < num_devices; i++ ) {
		const CyDeviceFinder::DeviceEntry &device_entry
				= ip_engine_list_obj[i];

#if MXI_PLEORA_IPORT_DEBUG
		MX_DEBUG(-2,("%s: Entry %ld IP address = '%s'",
		    fname, i, device_entry.mAddressIP.c_str_ascii() ));
#endif
	}

#if MXI_PLEORA_IPORT_DEBUG
	for ( i = 0; i < num_devices; i++ ) {
		const CyDeviceFinder::DeviceEntry &device_entry
				= ip_engine_list_obj[i];

		MX_DEBUG(-2,("%s: Entry %ld:", fname, i));

		MX_DEBUG(-2,("%s:   Device Identification", fname));

		MX_DEBUG(-2,("%s:     DeviceName = '%s'",
		    fname, device_entry.mDeviceName.c_str_ascii() ));
		MX_DEBUG(-2,("%s:     ModelName = '%s'",
		    fname, device_entry.mModelName.c_str_ascii() ));
		MX_DEBUG(-2,("%s:     ManufacturerName = '%s'",
		    fname, device_entry.mManufacturerName.c_str_ascii() ));
		MX_DEBUG(-2,("%s:     SerialNumber = '%s'",
		    fname, device_entry.mSerialNumber.c_str_ascii() ));
		MX_DEBUG(-2,("%s:     DeviceInformation = '%s'",
		    fname, device_entry.mDeviceInformation.c_str_ascii() ));
		MX_DEBUG(-2,("%s:     DeviceVersion = '%s'",
		    fname, device_entry.mDeviceVersion.c_str_ascii() ));
		MX_DEBUG(-2,("%s:     DeviceID = %u",
		    fname, device_entry.mDeviceID ));
		MX_DEBUG(-2,("%s:     ModuleID = %u",
		    fname, device_entry.mModuleID ));
		MX_DEBUG(-2,("%s:     SubID = %u",
		    fname, device_entry.mSubID ));
		MX_DEBUG(-2,("%s:     VendorID = %u",
		    fname, device_entry.mVendorID ));
		MX_DEBUG(-2,("%s:     SoftVerMaj = %u",
		    fname, device_entry.mSoftVerMaj ));
		MX_DEBUG(-2,("%s:     SoftVerMin = %u",
		    fname, device_entry.mSoftVerMin ));
		MX_DEBUG(-2,("%s:     SoftVerSub = %u",
		    fname, device_entry.mSoftVerSub ));

		MX_DEBUG(-2,("%s:   Networking", fname));

		MX_DEBUG(-2,("%s:     Mode = %lu",
		    fname, device_entry.mMode));
		MX_DEBUG(-2,("%s:     ProtocolVerMaj = %u",
		    fname, device_entry.mProtocolVerMaj));
		MX_DEBUG(-2,("%s:     ProtocolVerMin = %u",
		    fname, device_entry.mProtocolVerMin));
		MX_DEBUG(-2,("%s:     AdapterID = \?\?\?", fname));
		MX_DEBUG(-2,("%s:     AddressIP = '%s'",
		    fname, device_entry.mAddressIP.c_str_ascii() ));
		MX_DEBUG(-2,("%s:     SubnetMask = '%s'",
		    fname, device_entry.mSubnetMask.c_str_ascii() ));
		MX_DEBUG(-2,("%s:     Gateway = '%s'",
		    fname, device_entry.mGateway.c_str_ascii() ));
		MX_DEBUG(-2,("%s:     AddressMAC = '%s'",
		    fname, device_entry.mAddressMAC.c_str_ascii() ));

		MX_DEBUG(-2,("%s:   Data Information", fname));

		MX_DEBUG(-2,("%s:     MulticastAddress = '%s'",
		    fname, device_entry.mMulticastAddress.c_str_ascii() ));
		MX_DEBUG(-2,("%s:     ChannelCount = %u",
		    fname, device_entry.mChannelCount));
		MX_DEBUG(-2,("%s:     SendingMode = %lu",
		    fname, device_entry.mSendingMode));

		MX_DEBUG(-2,("%s: +++++++++++++++++++++", fname));

		MX_DEBUG(-2,("%s: device_entry = %p", fname, device_entry));
		MX_DEBUG(-2,("%s: device_entry.mAddressIP = %p",
			fname, device_entry.mAddressIP));
		MX_DEBUG(-2,("%s: device_entry.mAddressIP.c_str_ascii = %p",
			fname, device_entry.mAddressIP.c_str_ascii));
		MX_DEBUG(-2,("%s: device_entry.mAddressIP.c_str_ascii() = '%s'",
			fname, device_entry.mAddressIP.c_str_ascii() ));
	}
#endif

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

#if MXI_PLEORA_IPORT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

