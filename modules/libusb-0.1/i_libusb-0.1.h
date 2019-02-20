/*
 * Name:    i_libusb_0.1.h
 *
 * Purpose: Header for MX interface driver for USB device control via the
 *          old libusb-0.1 interface.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2004, 2012-2013, 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_LIBUSB_01_H__
#define __I_LIBUSB_01_H__

typedef struct {
	MX_RECORD *record;
	struct usb_bus *usb_busses;
} MX_LIBUSB_01;

extern MX_RECORD_FUNCTION_LIST mxi_libusb_01_record_function_list;
extern MX_USB_FUNCTION_LIST mxi_libusb_01_usb_function_list;

extern long mxi_libusb_01_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_libusb_01_rfield_def_ptr;

#define MXI_LIBUSB_01_STANDARD_FIELDS

/* Define all of the interface functions. */

MX_API mx_status_type mxi_libusb_01_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_libusb_01_delete_record( MX_RECORD *record );
MX_API mx_status_type mxi_libusb_01_open( MX_RECORD *record );
MX_API mx_status_type mxi_libusb_01_close( MX_RECORD *record );

MX_API mx_status_type mxi_libusb_01_enumerate_devices( MX_USB *usb );
MX_API mx_status_type mxi_libusb_01_find_device( MX_USB *usb,
					MX_USB_DEVICE **usb_device );
MX_API mx_status_type mxi_libusb_01_delete_device( MX_USB_DEVICE *usb_device );
MX_API mx_status_type mxi_libusb_01_reset_device( MX_USB_DEVICE *usb_device );
MX_API mx_status_type mxi_libusb_01_control_transfer( MX_USB_DEVICE *usb_device,
						int request_type,
						int request,
						int value,
						int index_or_offset,
						char *transfer_buffer,
						int transfer_buffer_length,
						int *num_bytes_transferred,
						double timeout );
MX_API mx_status_type mxi_libusb_01_bulk_read( MX_USB_DEVICE *usb_device,
						int endpoint_number,
						char *transfer_buffer,
						int transfer_buffer_length,
						int *num_bytes_read,
						double timeout );
MX_API mx_status_type mxi_libusb_01_bulk_write( MX_USB_DEVICE *usb_device,
						int endpoint_number,
						char *transfer_buffer,
						int transfer_buffer_length,
						int *num_bytes_written,
						double timeout );

#endif /* __I_LIBUSB_01_H__ */

