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
#include "mx_image.h"
#include "mx_video_input.h"
#include "i_pleora_iport.h"
#include "d_pleora_iport_vinput.h"

MX_RECORD_FUNCTION_LIST mxi_pleora_iport_record_function_list = {
	NULL,
	mxi_pleora_iport_create_record_structures,
	mxi_pleora_iport_finish_record_initialization,
	NULL,
	NULL,
	mxi_pleora_iport_open,
	mxi_pleora_iport_close
};

MX_RECORD_FIELD_DEFAULTS mxi_pleora_iport_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_PLEORA_IPORT_STANDARD_FIELDS
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
mxi_pleora_iport_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_pleora_iport_finish_record_initialization()";

	MX_PLEORA_IPORT *pleora_iport;
	mx_status_type mx_status;

	mx_status = mxi_pleora_iport_get_pointers( record,
						&pleora_iport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The individual pleora_iport_vinput drivers will put copies
	 * of their record pointers into pleora_iport->device_record_array
	 * in their finish_record_initialization() routines, so we must
	 * initialize that array here before they try to use it.
	 */

#if MXI_PLEORA_IPORT_DEBUG
	MX_DEBUG(-2,("%s: pleora_iport->max_devices = %ld",
		fname, pleora_iport->max_devices));
#endif

	if ( pleora_iport->max_devices <= 0 ) {
		mx_warning( "'max_devices' for record '%s' is set to "
			"a value of %ld which is less than 1.  "
			"This is probably not what you want to do.",
			record->name, pleora_iport->max_devices );

		pleora_iport->device_record_array = NULL;
	} else {
		pleora_iport->device_record_array = (MX_RECORD **)
		    calloc( pleora_iport->max_devices, sizeof(MX_RECORD *) );

		if ( pleora_iport->device_record_array == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Unable to allocate memory for a %ld element array of "
			"MX_RECORD pointers for the device_record_array of "
			"record '%s'.",
				pleora_iport->max_devices,
				record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pleora_iport_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_pleora_iport_open()";

	MX_PLEORA_IPORT *pleora_iport;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput;
	MX_RECORD *device_record;
	long i, num_devices;
	mx_status_type mx_status;

	CyDeviceFinder finder;
	CyDeviceFinder::DeviceList ip_engine_list;
	CyResult cy_result;

	mx_status = mxi_pleora_iport_get_pointers( record,
						&pleora_iport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find IP engines available through the eBUS driver. */

	finder.Find( CY_DEVICE_ACCESS_MODE_EBUS,
			ip_engine_list,
			100,
			true );

	/* Find GigE Vision IP engines available through the eBUS driver. */

	finder.Find( CY_DEVICE_ACCESS_MODE_GEV_EBUS,
			ip_engine_list,
			100,
			true );

	/* Find IP engines available through the High Performance driver. */

	finder.Find( CY_DEVICE_ACCESS_MODE_DRV,
			ip_engine_list,
			100,
			true );

	/* Find IP engines available through the Network Stack. */

	finder.Find( CY_DEVICE_ACCESS_MODE_UDP,
			ip_engine_list,
			100,
			true );

	num_devices = ip_engine_list.size();

#if MXI_PLEORA_IPORT_DEBUG
	MX_DEBUG(-2,("%s: %d IP engines found for record '%s'.",
		fname, num_devices, record->name ));
#endif

	if ( num_devices == 0 ) {
		mx_warning( "No Pleora iPORT engines were found for "
		"record '%s', so no iPORT-based cameras will be initialized.",
			record->name );

		return MX_SUCCESSFUL_RESULT;
	}

	for ( i = 0; i < num_devices; i++ ) {
		const CyDeviceFinder::DeviceEntry &device_entry
				= ip_engine_list[i];

#if MXI_PLEORA_IPORT_DEBUG
		MX_DEBUG(-2,("%s: %ld>> MAC '%s', IP '%s'",
			fname, i, device_entry.mAddressMAC.c_str_ascii(),
			device_entry.mAddressIP.c_str_ascii() ));
#endif
	}

#if 0 && MXI_PLEORA_IPORT_DEBUG
	MX_DEBUG(-2,("%s: +++++++++++++++++++++", fname));

	for ( i = 0; i < num_devices; i++ ) {
		const CyDeviceFinder::DeviceEntry &device_entry
				= ip_engine_list[i];

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

	}
#endif

	/* Loop through all the records in device_record_array to find 
	 * the CyGrabber objects that correspond to them.
	 */

	for ( i = 0; i < pleora_iport->max_devices; i++ ) {

		int offset;

		device_record = pleora_iport->device_record_array[i];

		if ( device_record == (MX_RECORD *) NULL ) {
			/* There is no device in this slot, so go back
			 * and look in the next slot.
			 */
			continue;
		}

		pleora_iport_vinput = (MX_PLEORA_IPORT_VINPUT *)
					device_record->record_type_struct;

		if ( pleora_iport_vinput == (MX_PLEORA_IPORT_VINPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_PLEORA_IPORT_VINPUT pointer for record '%s' "
			"is NULL.", device_record->name );
		}

		pleora_iport_vinput->grabber = NULL;

#if MXI_PLEORA_IPORT_DEBUG
		MX_DEBUG(-2,("%s: device_record_array[%ld] = '%s'",
			fname, i, device_record->name ));
#endif

		const CyDeviceFinder::DeviceEntry &device_entry
				= ip_engine_list[i];

		/* Is this the device that we are looking for?
		 *
		 * Compare the MAC address string for this device to the
		 * MAC address specified in the pleora_iport_vinput record.
		 */

		const char *device_address_mac =
				device_entry.mAddressMAC.c_str_ascii();

#if MXI_PLEORA_IPORT_DEBUG
		MX_DEBUG(-2,("%s: device_address_mac = '%s'",
			fname, device_address_mac));
		MX_DEBUG(-2,
		("%s: pleora_iport_vinput->mac_address_string = '%s'",
			fname, pleora_iport_vinput->mac_address_string));
#endif

		if ( mx_strcasecmp( device_address_mac,
			pleora_iport_vinput->mac_address_string ) != 0 )
		{

#if MXI_PLEORA_IPORT_DEBUG
			MX_DEBUG(-2,("%s: skipping device %ld", fname, i));
#endif
			continue;	/* Go back for the next device. */
		}

		/* If the device already has an IP address and it is
		 * different from the one specified in the MX config file,
		 * then we print a warning message, but otherwise leave
		 * the situation alone.
		 */

		const char *device_address_ip =
				device_entry.mAddressIP.c_str_ascii();

		if ( strlen( device_address_ip ) > 0 ) {
			/* The device already has an IP address.  Do we
			 * have the same address?
			 */

			char ip_address_string[ MXU_HOSTNAME_LENGTH+1 ];

			if ( device_address_ip[0] == '[' ) {
				offset = 1;
			} else {
				offset = 0;
			}

			strlcpy( ip_address_string, device_address_ip + offset,
						sizeof(ip_address_string) );

			char *ptr = strchr( ip_address_string, ']' );

			if ( ptr != NULL ) {
				*ptr = '\0';
			}

#if MXI_PLEORA_IPORT_DEBUG
			MX_DEBUG(-2,("%s: ip_address_string = '%s'",
				fname, ip_address_string));
			MX_DEBUG(-2,
			("%s: pleora_iport_vinput->ip_address_string = '%s'",
				fname, pleora_iport_vinput->ip_address_string));
#endif

			if ( strcmp( ip_address_string,
				pleora_iport_vinput->ip_address_string ) != 0 )
			{

				mx_warning( "The IP address '%s' of the "
				"IP engine differs from the address '%s' "
				"specified in the MX configuration files.  "
				"We will use the IP address '%s'.",
					ip_address_string,
					pleora_iport_vinput->ip_address_string,
					ip_address_string );
			}

		} else {
			char formatted_ip_address[ MXU_HOSTNAME_LENGTH+1 ];

			/* The device does not have an IP address, so
			 * we assign one.
			 */

			snprintf( formatted_ip_address,
				sizeof(formatted_ip_address),
				"[%s]", pleora_iport_vinput->ip_address_string);

#if MXI_PLEORA_IPORT_DEBUG
			MX_DEBUG(-2,
			("%s: device %ld does not have an IP address, "
			"so we will assign it the address '%s'.",
				fname, i, formatted_ip_address ));
#endif
			cy_result = finder.SetIP( device_entry.mMode,
						device_entry.mAdapterID,
						device_entry.mAddressMAC,
						formatted_ip_address );

			if ( cy_result != CY_RESULT_OK ) {
				(void) mx_error( MXE_DEVICE_IO_ERROR, fname,
				"The attempt to set the IP address of "
				"ethernet MAC address '%s' failed.  "
				"cy_result = %d",
					device_entry.mAddressMAC,
					cy_result );

				continue;  /* Go back for the next device. */
			}
		}

		/* Create and setup a CyConfig object for the device. */

		CyConfig config;
		config.AddDevice();

		/* Configure the CyConfig object's connection parameters. */

		config.SetParameter( CY_CONFIG_PARAM_ACCESS_MODE,
					device_entry.mMode );
		config.SetParameter( CY_CONFIG_PARAM_ADDRESS_IP,
					device_entry.mAddressIP );
		config.SetParameter( CY_CONFIG_PARAM_ADDRESS_MAC,
					device_entry.mAddressMAC );
		config.SetParameter( CY_CONFIG_PARAM_ADAPTER_ID,
				device_entry.mAdapterID.GetIdentifier() );

		/* A packet payload size of 1440 bytes is safe
		 * for all connections.
		 */

		config.SetParameter( CY_CONFIG_PARAM_PACKET_SIZE, 1440 );

		/* Set the desired timeouts. */

		config.SetParameter( CY_CONFIG_PARAM_ANSWER_TIMEOUT, 1000 );
		config.SetParameter( CY_CONFIG_PARAM_FIRST_PACKET_TIMEOUT,
								1500 );
		config.SetParameter( CY_CONFIG_PARAM_PACKET_TIMEOUT, 500 );
		config.SetParameter( CY_CONFIG_PARAM_REQUEST_TIMEOUT, 5000 );

		/* Set the connection topology to unicast. */

		config.SetParameter( CY_CONFIG_PARAM_DATA_SENDING_MODE,
							CY_DEVICE_DSM_UNICAST );
		config.SetParameter( CY_CONFIG_PARAM_DATA_SENDING_MODE_MASTER,
							true );

		/* Create and connect a CyGrabber object to the IP Engine. */

		CyGrabber *grabber = new CyGrabber();

		cy_result = grabber->Connect( config );

		if ( cy_result != CY_RESULT_OK ) {
			mx_warning(
			"The attempt to connect to camera '%s' failed.  "
			"cy_result = %ld.", device_record->name, cy_result );

			continue;
		}

		/* Save a pointer to the grabber in the video input's
		 * record type structure.
		 */

		pleora_iport_vinput->grabber = grabber;

#if MXI_PLEORA_IPORT_DEBUG
		MX_DEBUG(-2,("%s: Saved grabber %p to record %p '%s'.",
			fname, grabber, pleora_iport_vinput,
			pleora_iport_vinput->record->name ));

		{
			CyDevice &device = grabber->GetDevice();

			unsigned char dev_id, mod_id, sub_id;
			unsigned char vendor_id, mac1, mac2, mac3, mac4;
			unsigned char mac5, mac6, ip1, ip2, ip3, ip4;
			unsigned char version_major, version_minor;
			unsigned char channel_count, version_sub;

			device.GetDeviceInfo( &dev_id, &mod_id, &sub_id,
				&vendor_id, &mac1, &mac2, &mac3, &mac4,
				&mac5, &mac6, &ip1, &ip2, &ip3, &ip4,
				&version_major, &version_minor,
				&channel_count, &version_sub );

			MX_DEBUG(-2,("%s: grabber IP address = %d.%d.%d.%d",
			fname, ip1, ip2, ip3, ip4));
		}

		MX_DEBUG(-2,("++++++++ %s complete ++++++++", fname));
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

#if MXI_PLEORA_IPORT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------*/

MX_EXPORT void
mxi_pleora_iport_display_parameter_array( CyDeviceExtension *extension,
					unsigned long num_parameters,
					unsigned long *parameter_array )
{
	unsigned long i, parameter_id;

	for ( i = 0; i < num_parameters; i++ ) {
		parameter_id = parameter_array[i];

		mxi_pleora_iport_display_parameter_info( extension,
							parameter_id );
	}

	return;
}

MX_EXPORT void
mxi_pleora_iport_display_parameter_info( CyDeviceExtension *extension,
					unsigned long parameter_id )
{
	CyString cy_string;
	unsigned long parameter_type;
	__int64 int64_min, int64_max, int64_value;
	double double_min, double_max, double_value;

	parameter_type = extension->GetParameterType( parameter_id );

	CyString parameter_name =
			extension->GetParameterName( parameter_id );

	fprintf( stderr, "Param (%lu) '%s', ",
			parameter_id, parameter_name.c_str_ascii() );

	switch( parameter_type ) {
	case CY_PARAMETER_STRING:
		extension->GetParameter( parameter_type, cy_string );

		fprintf( stderr, "string  '%s'\n",
			cy_string.c_str_ascii() );
		break;

	case CY_PARAMETER_INT:
		extension->GetParameter( parameter_id, int64_value );

		extension->GetParameterRange( parameter_id,
					int64_min, int64_max );

		fprintf( stderr, " int64  %lI64d (%lI64d to %lI64d)\n",
				int64_value, int64_min, int64_max );
		break;

	case CY_PARAMETER_DOUBLE:
		extension->GetParameter( parameter_id, double_value );

		extension->GetParameterRange( parameter_id,
					double_min, double_max );

		fprintf( stderr, "double  %g (%g to %g)\n",
					double_min, double_max );
		break;

	case CY_PARAMETER_BOOL:
		extension->GetParameter( parameter_id, int64_value );

		extension->GetParameterRange( parameter_id,
					int64_min, int64_max );

		fprintf( stderr, " bool  %lI64d (%lI64d to %lI64d)\n",
				int64_value, int64_min, int64_max );
		break;

	case CY_PARAMETER_ENUM:
		extension->GetParameter( parameter_id, int64_value );

		extension->GetParameterRange( parameter_id,
					int64_min, int64_max );

		fprintf( stderr, " enum  %lI64d (%lI64d to %lI64d)\n",
				int64_value, int64_min, int64_max );
		break;

	default:
		fprintf( stderr, "unrecognized type %lu\n",
					parameter_type );
		break;
	}

	return;
}

