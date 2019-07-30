/*
 * Name:    i_libusb_01.c
 *
 * Purpose: MX driver for USB device control via the old libusb-0.1 interface.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004, 2007-2008, 2010, 2012-2013, 2017, 2019
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_LIBUSB_01_DEBUG		FALSE

#define MXI_LIBUSB_01_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_usb.h"
#include "i_libusb-0.1.h"

#if MXI_LIBUSB_01_DEBUG_TIMING
#include "mx_hrt_debug.h"
#endif

/* The following include file comes from 'libusb_01'.  On some, but not all,
 * platforms, it will be found as /usr/include/usb.h.
 */

#include "usb.h"

MX_RECORD_FUNCTION_LIST mxi_libusb_01_record_function_list = {
	NULL,
	mxi_libusb_01_create_record_structures,
	mx_usb_finish_record_initialization,
	mxi_libusb_01_delete_record,
	NULL,
	mxi_libusb_01_open,
	mxi_libusb_01_close
};

MX_USB_FUNCTION_LIST mxi_libusb_01_usb_function_list = {
	mxi_libusb_01_enumerate_devices,
	mxi_libusb_01_find_device,
	mxi_libusb_01_delete_device,
	NULL,
	mxi_libusb_01_control_transfer,
	mxi_libusb_01_bulk_read,
	mxi_libusb_01_bulk_write
};
MX_RECORD_FIELD_DEFAULTS mxi_libusb_01_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_USB_STANDARD_FIELDS,
	MXI_LIBUSB_01_STANDARD_FIELDS
};

long mxi_libusb_01_num_record_fields
	= sizeof( mxi_libusb_01_record_field_defaults )
	/ sizeof( mxi_libusb_01_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_libusb_01_rfield_def_ptr
			= &mxi_libusb_01_record_field_defaults[0];

/* ---- */

/* A private function for the use of the driver. */

static mx_status_type
mxi_libusb_01_get_pointers( MX_USB *mx_usb,
			MX_LIBUSB_01 **libusb_01,
			const char *calling_fname )
{
	static const char fname[] = "mxi_libusb_01_get_pointers()";

	MX_RECORD *libusb_01_record;

	if ( mx_usb == (MX_USB *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_USB pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( libusb_01 == (MX_LIBUSB_01 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_LIBUSB_01 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	libusb_01_record = mx_usb->record;

	if ( libusb_01_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The libusb_01_record pointer for the "
			"MX_USB pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*libusb_01 = (MX_LIBUSB_01 *) libusb_01_record->record_type_struct;

	if ( *libusb_01 == (MX_LIBUSB_01 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIBUSB_01 pointer for record '%s' is NULL.",
			libusb_01_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_libusb_01_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_libusb_01_create_record_structures()";

	MX_USB *mx_usb;
	MX_LIBUSB_01 *libusb_01;

	/* Allocate memory for the necessary structures. */

	mx_usb = (MX_USB *) malloc( sizeof(MX_USB) );

	if ( mx_usb == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_USB structure." );
	}

	libusb_01 = (MX_LIBUSB_01 *) malloc( sizeof(MX_LIBUSB_01) );

	if ( libusb_01 == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_LIBUSB_01 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mx_usb;
	record->record_type_struct = libusb_01;
	record->class_specific_function_list = &mxi_libusb_01_usb_function_list;

	mx_usb->record = record;
	libusb_01->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_libusb_01_delete_record( MX_RECORD *record )
{
	static const char fname[] = "mxi_libusb_01_delete_record()";

	MX_USB *mx_usb;
	MX_LIBUSB_01 *libusb_01 = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

#if MXI_LIBUSB_01_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	mx_usb = (MX_USB *) record->record_class_struct;

	mx_status = mxi_libusb_01_get_pointers( mx_usb, &libusb_01, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_delete_handle_table( mx_usb->handle_table );


	mx_status = mx_default_delete_record_handler( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_libusb_01_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_libusb_01_open()";

	MX_USB *mx_usb;
	MX_LIBUSB_01 *libusb_01 = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

#if MXI_LIBUSB_01_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	mx_usb = (MX_USB *) record->record_class_struct;

	mx_status = mxi_libusb_01_get_pointers( mx_usb, &libusb_01, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the libusb_01 library. */

	usb_init();

	/* Now ask libusb_01 to find all of the devices. */

	mx_status = mxi_libusb_01_enumerate_devices( mx_usb );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_libusb_01_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_libusb_01_close()";

	MX_USB *mx_usb;
	MX_LIBUSB_01 *libusb_01 = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

#if MXI_LIBUSB_01_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	mx_usb = (MX_USB *) record->record_class_struct;

	mx_status = mxi_libusb_01_get_pointers( mx_usb, &libusb_01, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_libusb_01_enumerate_devices( MX_USB *mx_usb )
{
	static const char fname[] = "mxi_libusb_01_enumerate_devices()";

	MX_LIBUSB_01 *libusb_01 = NULL;
	int num_bus_changes, num_device_changes;
	mx_status_type mx_status;

	mx_status = mxi_libusb_01_get_pointers( mx_usb, &libusb_01, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_LIBUSB_01_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, mx_usb->record->name));
#endif

	num_bus_changes = usb_find_busses();

	num_device_changes = usb_find_devices();

	/* Suppress GCC warnings. */

	MXW_UNUSED( num_bus_changes );
	MXW_UNUSED( num_device_changes );

#if defined(OS_WIN32)
	libusb_01->usb_busses = usb_get_busses();
#else
	libusb_01->usb_busses = usb_busses;
#endif

#if MXI_LIBUSB_01_DEBUG
	MX_DEBUG(-2,("%s: num_bus_changes = %d, num_device_changes = %d",
		fname, num_bus_changes, num_device_changes));

	MX_DEBUG(-2,("%s: libusb_01->usb_busses = %p", fname, libusb_01->usb_busses));

#if 1
	/* Look and see what was found on the USB busses. */

	{
		struct usb_bus *current_bus;
		struct usb_device *current_device;

		current_bus = libusb_01->usb_busses;

		while ( current_bus != NULL ) {
			MX_DEBUG(-2,("%s: current_bus = %p, dirname = '%s'",
				fname, current_bus, current_bus->dirname));

			current_device = current_bus->devices;

			while ( current_device != NULL ) {
				MX_DEBUG(-2,
("%s: current_device = %p, filename = '%s', vendor = %#x, product = %#x",
					fname, current_device,
					current_device->filename,
					current_device->descriptor.idVendor,
					current_device->descriptor.idProduct));

				current_device = current_device->next;
			}

			current_bus = current_bus->next;
		}
	}
#endif

#endif /* MXI_LIBUSB_01_DEBUG */

	return mx_status;
}

static mx_status_type
mxi_libusb_01_setup_device_structures( MX_USB *mx_usb,
				MX_USB_DEVICE **mx_device,
				struct usb_device *libusb_01_device )
{
	static const char fname[] = "mxi_libusb_01_setup_device_structures()";

	MX_USB_DEVICE *mx_usb_device;
	usb_dev_handle *libusb_01_device_handle;
	int usb_status;

#if MXI_LIBUSB_01_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	/* Allocate memory for an MX_USB_DEVICE structure. */

	mx_usb_device = (MX_USB_DEVICE *) malloc( sizeof( MX_USB_DEVICE ) );

	if ( mx_usb_device == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for a new MX_USB_DEVICE structure." );
	}

	/* Initialize the MX_USB_DEVICE structure. */

	mx_usb_device->usb = mx_usb;
	mx_usb_device->vendor_id = libusb_01_device->descriptor.idVendor;
	mx_usb_device->product_id = libusb_01_device->descriptor.idProduct;

#if MXI_LIBUSB_01_DEBUG
	MX_DEBUG(-2,("%s: setting up device (vendor %#lx, product %#lx)",
		fname, mx_usb_device->vendor_id, mx_usb_device->product_id));
#endif

	/* Open a libusb_01 handle for the device. */

	libusb_01_device_handle = usb_open( libusb_01_device );

	if ( libusb_01_device_handle == NULL ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unable to create a libusb_01 device handle for device %p",
			libusb_01_device );
	}

	/* Save the handle in the generic MX_USB_DEVICE structure. */

	mx_usb_device->device_data = libusb_01_device_handle;

	*mx_device = mx_usb_device;

	/* Set the configuration. */

	mx_usb_device->configuration_number = mx_usb->configuration_number;

	usb_status = usb_set_configuration( libusb_01_device_handle,
					mx_usb_device->configuration_number );

	switch( usb_status ) {
	case 0:
		break;
	case -EPERM:
		return mx_error( MXE_PERMISSION_DENIED, fname,
			"You do not have permission to set configuration %d "
			"for device (%#lx, %#lx) on USB bus '%s'.",
				mx_usb_device->configuration_number,
				mx_usb_device->vendor_id,
				mx_usb_device->product_id,
				mx_usb_device->usb->record->name );
		break;
	default:
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to set configuration %d for device "
			"(%#lx, %#lx) on USB bus '%s' failed.  "
			"USB status = %d, error message = '%s'",
				mx_usb_device->configuration_number,
				mx_usb_device->vendor_id,
				mx_usb_device->product_id,
				mx_usb_device->usb->record->name,
				usb_status, usb_strerror() );
		break;
	}

#if MXI_LIBUSB_01_DEBUG
	MX_DEBUG(-2,("%s: configuration %d set.",
		fname, mx_usb_device->configuration_number));
#endif

	/* Claim the interface. */

	mx_usb_device->interface_number = mx_usb->interface_number;

	usb_status = usb_claim_interface( libusb_01_device_handle,
					mx_usb_device->interface_number );

	switch( usb_status ) {
	case 0:
		break;
	case -EPERM:
		return mx_error( MXE_PERMISSION_DENIED, fname,
			"You do not have permission to claim interface %d "
			"for device (%#lx, %#lx) on USB bus '%s'.",
				mx_usb_device->interface_number,
				mx_usb_device->vendor_id,
				mx_usb_device->product_id,
				mx_usb_device->usb->record->name );
		break;
	default:
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to claim interface %d for device "
			"(%#lx, %#lx) on USB bus '%s' failed.  "
			"USB status = %d, error message = '%s'",
				mx_usb_device->interface_number,
				mx_usb_device->vendor_id,
				mx_usb_device->product_id,
				mx_usb_device->usb->record->name,
				usb_status, usb_strerror() );
		break;
	}

#if MXI_LIBUSB_01_DEBUG
	MX_DEBUG(-2,("%s: interface %d claimed.",
		fname, mx_usb_device->interface_number));
#endif

	/* Select the alternate interface. */

	mx_usb_device->alternate_interface_number =
				mx_usb->alternate_interface_number;

	usb_status = usb_set_altinterface( libusb_01_device_handle,
				mx_usb_device->alternate_interface_number );

	switch( usb_status ) {
	case 0:
		break;
	case -EINVAL:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested alternate interface number %d for "
			"device (%#lx, %#lx) on USB bus '%s' is not valid.",
				mx_usb_device->alternate_interface_number,
				mx_usb_device->vendor_id,
				mx_usb_device->product_id,
				mx_usb_device->usb->record->name );
		break;
	default:
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to set alternate interface %d for device "
			"(%#lx, %#lx) on USB bus '%s' failed.  "
			"USB status = %d, error message = '%s'",
				mx_usb_device->alternate_interface_number,
				mx_usb_device->vendor_id,
				mx_usb_device->product_id,
				mx_usb_device->usb->record->name,
				usb_status, usb_strerror() );
		break;
	}

#if MXI_LIBUSB_01_DEBUG
	MX_DEBUG(-2,("%s: alternate interface %d set.",
		fname, mx_usb_device->alternate_interface_number));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_libusb_01_find_device( MX_USB *mx_usb, MX_USB_DEVICE **mx_usb_device )
{
	static const char fname[] = "mxi_libusb_01_find_device()";

	struct usb_bus *current_bus;
	struct usb_device *current_device;
	MX_LIBUSB_01 *libusb_01 = NULL;
	unsigned long vendor_id, product_id;
	int order_number, num_matching_devices_found, product_id_in_use;
	int quiet_flag;
	long mx_error_code;
	char *serial_number;
	mx_status_type mx_status;

	if ( mx_usb_device == (MX_USB_DEVICE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_USB_DEVICE pointer passed was NULL." );
	}

	mx_status = mxi_libusb_01_get_pointers( mx_usb, &libusb_01, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_LIBUSB_01_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, mx_usb->record->name));
#endif

	quiet_flag = mx_usb->quiet_flag;

	mx_usb->quiet_flag = FALSE;

	num_matching_devices_found = 0;

	switch( mx_usb->find_type ) {
	case MXF_USB_FIND_TYPE_ORDER:
		vendor_id = mx_usb->find.by_order.vendor_id;
		product_id = mx_usb->find.by_order.product_id;
		order_number = mx_usb->find.by_order.order_number;
		serial_number = NULL;
		product_id_in_use = TRUE;
		break;

	case MXF_USB_FIND_TYPE_SERIAL_NUMBER:
		vendor_id = mx_usb->find.by_serial_number.vendor_id;
		product_id = mx_usb->find.by_serial_number.product_id;
		order_number = -1;
		serial_number = mx_usb->find.by_serial_number.serial_number;
		product_id_in_use = TRUE;

		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		    "Device lookup by serial number is not yet implemented "
		    "for USB bus record '%s'.", mx_usb->record->name );
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported device find type %d for USB bus record '%s'.",
			mx_usb->find_type, mx_usb->record->name );
		break;
	}

	MXW_UNUSED( serial_number );

	current_bus = libusb_01->usb_busses;

	*mx_usb_device = NULL;

	while ( current_bus != NULL ) {

#if MXI_LIBUSB_01_DEBUG
	    MX_DEBUG(-2,("%s: current_bus = %p, dirname = '%s'",
				fname, current_bus, current_bus->dirname));
#endif

	    current_device = current_bus->devices;

	    while ( current_device != NULL ) {

#if MXI_LIBUSB_01_DEBUG
		MX_DEBUG(-2,
("%s: current_device = %p, filename = '%s', vendor = %#x, product = %#x",
				fname, current_device,
				current_device->filename,
				current_device->descriptor.idVendor,
				current_device->descriptor.idProduct));
#endif

		if ( product_id_in_use ) {
		    if ( ( vendor_id == current_device->descriptor.idVendor )
		      && ( product_id == current_device->descriptor.idProduct ))
		    {
			switch( mx_usb->find_type ) {
			case MXF_USB_FIND_TYPE_ORDER:

#if MXI_LIBUSB_01_DEBUG
			    MX_DEBUG(-2,
		("%s: order_number = %d, num_matching_devices_found = %d",
		 		fname, order_number,
				num_matching_devices_found));
#endif

			    if ( order_number == num_matching_devices_found ) {

#if MXI_LIBUSB_01_DEBUG
				    MX_DEBUG(-2,("%s: Device found!", fname));
#endif

				    return
					mxi_libusb_01_setup_device_structures(
				    	    mx_usb, mx_usb_device,
					    current_device );
			    }
			    break;
			case MXF_USB_FIND_TYPE_SERIAL_NUMBER:
			    /* Not yet implemented */
			    break;
			}
			
			num_matching_devices_found++;
		    }
		}
		current_device = current_device->next;
	    }
	    current_bus = current_bus->next;
	}

	if ( product_id_in_use ) {
		if ( mx_usb->find_type == MXF_USB_FIND_TYPE_ORDER ) {
			if ( quiet_flag ) {
				mx_error_code = (MXE_NOT_FOUND | MXE_QUIET);
			} else {
				mx_error_code = MXE_NOT_FOUND;
			}

			return mx_error( mx_error_code, fname,
			"The requested device (vendor %#lx, product %#lx, "
			"order number %d) for USB record '%s' was not found.",
				vendor_id, product_id, order_number,
				mx_usb->record->name);
		} else {
			if ( quiet_flag ) {
				mx_error_code = (MXE_NOT_FOUND | MXE_QUIET);
			} else {
				mx_error_code = MXE_NOT_FOUND;
			}

			return mx_error( mx_error_code, fname,
			"The requested device (vendor %#lx, product %#lx) "
			"for USB record '%s' was not found.",
				vendor_id, product_id, mx_usb->record->name );
		}
	} else {
		if ( quiet_flag ) {
			mx_error_code = (MXE_NOT_FOUND | MXE_QUIET);
		} else {
			mx_error_code = MXE_NOT_FOUND;
		}

		return mx_error( mx_error_code, fname,
		"The requested device for USB record '%s' was not found.",
			mx_usb->record->name );
	}
}

MX_EXPORT mx_status_type
mxi_libusb_01_delete_device( MX_USB_DEVICE *mx_usb_device )
{
	static const char fname[] = "mxi_libusb_01_delete_device()";

	MX_LIBUSB_01 *libusb_01 = NULL;
	int usb_status;
	mx_status_type mx_status;

	if ( mx_usb_device == (MX_USB_DEVICE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_USB_DEVICE pointer passed was NULL." );
	}

	mx_status = mxi_libusb_01_get_pointers( mx_usb_device->usb,
							&libusb_01, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_LIBUSB_01_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname,
				mx_usb_device->usb->record->name));
#endif

	if ( mx_usb_device->interface_number >= 0 ) {
		usb_status = usb_release_interface(
			( usb_dev_handle *) mx_usb_device->device_data,
			mx_usb_device->interface_number );

		if ( usb_status != 0 ) {
			mx_status = mx_error( MXE_DEVICE_ACTION_FAILED, fname,
				"The attempt to release interface %d for "
				"device (%#lx, %#lx) on USB bus '%s' failed.  "
				"USB status = %d, error message = '%s'",
					mx_usb_device->interface_number,
					mx_usb_device->vendor_id,
					mx_usb_device->product_id,
					mx_usb_device->usb->record->name,
					usb_status, usb_strerror() );
		}
	}

	usb_status = usb_close(( usb_dev_handle *) mx_usb_device->device_data );

	if ( usb_status != 0 ) {
		mx_status = mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to close device "
			"(%#lx, %#lx) on USB bus '%s' failed.  "
			"USB status = %d, error message = '%s'",
				mx_usb_device->vendor_id,
				mx_usb_device->product_id,
				mx_usb_device->usb->record->name,
				usb_status, usb_strerror() );
	}

	mx_free( mx_usb_device->device_data );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_libusb_01_control_transfer( MX_USB_DEVICE *mx_usb_device,
				int request_type,
				int request,
				int value,
				int index_or_offset,
				char *transfer_buffer,
				int transfer_buffer_length,
				int *num_bytes_transferred,
				double timeout )
{
	static const char fname[] = "mxi_libusb_01_control_transfer()";

	MX_LIBUSB_01 *libusb_01 = NULL;
	int usb_result;
	mx_status_type mx_status;

#if MXI_LIBUSB_01_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	if ( mx_usb_device == (MX_USB_DEVICE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_USB_DEVICE pointer passed was NULL." );
	}

	mx_status = mxi_libusb_01_get_pointers( mx_usb_device->usb,
							&libusb_01, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_LIBUSB_01_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s', timeout = %g sec", fname,
				mx_usb_device->usb->record->name, timeout ));
#endif

#if MXI_LIBUSB_01_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	usb_result = usb_control_msg(
			( usb_dev_handle *) mx_usb_device->device_data,
			request_type, request, value, index_or_offset,
			transfer_buffer, transfer_buffer_length,
			mx_round( 1000.0 * timeout ) );

#if MXI_LIBUSB_01_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, mx_usb_device->usb->record->name );
#endif

	if ( usb_result >= 0 ) {
		if ( num_bytes_transferred != (int *) NULL ) {
			*num_bytes_transferred = usb_result;
		}
	} else {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to send a control message to device "
			"(%#lx, %#lx) on USB bus '%s' failed.  "
			"USB status = %d, error message = '%s'",
				mx_usb_device->vendor_id,
				mx_usb_device->product_id,
				mx_usb_device->usb->record->name,
				usb_result, usb_strerror() );
	}

#if MXI_LIBUSB_01_DEBUG
	MX_DEBUG(-2,
	("%s: USB device (%#lx, %#lx), bytes transferred = %d", fname,
	 	mx_usb_device->vendor_id,
		mx_usb_device->product_id,
		usb_result ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_libusb_01_bulk_read( MX_USB_DEVICE *mx_usb_device,
				int endpoint_number,
				char *transfer_buffer,
				int transfer_buffer_length,
				int *num_bytes_read,
				double timeout )
{
	static const char fname[] = "mxi_libusb_01_bulk_read()";

	MX_LIBUSB_01 *libusb_01 = NULL;
	int usb_result;
	mx_status_type mx_status;

#if MXI_LIBUSB_01_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	if ( mx_usb_device == (MX_USB_DEVICE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_USB_DEVICE pointer passed was NULL." );
	}

	mx_status = mxi_libusb_01_get_pointers( mx_usb_device->usb,
							&libusb_01, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_LIBUSB_01_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s', timeout = %g sec", fname,
				mx_usb_device->usb->record->name, timeout ));
#endif

#if MXI_LIBUSB_01_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	usb_result = usb_bulk_read(
		( usb_dev_handle *) mx_usb_device->device_data,
			endpoint_number,
			transfer_buffer, transfer_buffer_length,
			mx_round( 1000.0 * timeout ) );

#if MXI_LIBUSB_01_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, mx_usb_device->usb->record->name );
#endif

	if ( usb_result >= 0 ) {
		if ( num_bytes_read != (int *) NULL ) {
			*num_bytes_read = usb_result;
		}
	} else
	if ( usb_result == (-110) ) {
		return mx_error( MXE_TIMED_OUT, fname,
			"The attempt to perform a bulk read of %d bytes "
			"from endpoint %#x "
			"of device (%#lx, %#lx) on USB bus '%s' has "
			"timed out after %g seconds.",
				transfer_buffer_length,
				endpoint_number,
				mx_usb_device->vendor_id,
				mx_usb_device->product_id,
				mx_usb_device->usb->record->name,
				timeout );
	} else {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to perform a bulk read of %d bytes "
			"from endpoint %#x "
			"of device (%#lx, %#lx) on USB bus '%s' failed.  "
			"USB status = %d, error message = '%s'",
				transfer_buffer_length,
				endpoint_number,
				mx_usb_device->vendor_id,
				mx_usb_device->product_id,
				mx_usb_device->usb->record->name,
				usb_result, usb_strerror() );
	}

#if MXI_LIBUSB_01_DEBUG
	MX_DEBUG(-2,
	("%s: USB device (%#lx, %#lx), endpoint %#x, bytes read = %d", fname,
	 	mx_usb_device->vendor_id, mx_usb_device->product_id,
		endpoint_number, usb_result ));

	{
		int i;

		fprintf( stderr, "%s:", fname );

		for ( i = 0; i < *num_bytes_read; i++ ) {
			fprintf( stderr, " %#02x",
				(unsigned char) transfer_buffer[i] );
		}

		fprintf( stderr, "\n" );
	}
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_libusb_01_bulk_write( MX_USB_DEVICE *mx_usb_device,
				int endpoint_number,
				char *transfer_buffer,
				int transfer_buffer_length,
				int *num_bytes_written,
				double timeout )
{
	static const char fname[] = "mxi_libusb_01_bulk_write()";

	MX_LIBUSB_01 *libusb_01 = NULL;
	int usb_result;
	mx_status_type mx_status;

#if MXI_LIBUSB_01_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	if ( mx_usb_device == (MX_USB_DEVICE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_USB_DEVICE pointer passed was NULL." );
	}

	mx_status = mxi_libusb_01_get_pointers( mx_usb_device->usb,
							&libusb_01, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_LIBUSB_01_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s', timeout = %g sec", fname,
				mx_usb_device->usb->record->name, timeout ));
	{
		int i;

		fprintf( stderr, "%s:", fname );

		for ( i = 0; i < transfer_buffer_length; i++ ) {
			fprintf( stderr, " %#02x",
				(unsigned char) transfer_buffer[i] );
		}

		fprintf( stderr, "\n" );
	}
#endif

#if MXI_LIBUSB_01_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	usb_result = usb_bulk_write(
		( usb_dev_handle *) mx_usb_device->device_data,
			endpoint_number,
			transfer_buffer, transfer_buffer_length,
			mx_round( 1000.0 * timeout ) );

#if MXI_LIBUSB_01_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, mx_usb_device->usb->record->name );
#endif

	if ( usb_result >= 0 ) {
		if ( num_bytes_written != (int *) NULL ) {
			*num_bytes_written = usb_result;
		}
	} else {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to perform a bulk write of %d bytes "
			"to endpoint %#x "
			"of device (%#lx, %#lx) on USB bus '%s' failed.  "
			"USB status = %d, error message = '%s'",
				transfer_buffer_length,
				endpoint_number,
				mx_usb_device->vendor_id,
				mx_usb_device->product_id,
				mx_usb_device->usb->record->name,
				usb_result, usb_strerror() );
	}

#if MXI_LIBUSB_01_DEBUG
	MX_DEBUG(-2,
	("%s: USB device (%#lx, %#lx), endpoint %#x, bytes written = %d", fname,
	 	mx_usb_device->vendor_id, mx_usb_device->product_id,
		endpoint_number, usb_result ));
#endif

	return mx_status;
}

