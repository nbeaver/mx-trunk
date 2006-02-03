/*
 * Name:    i_libusb.h
 *
 * Purpose: Header for MX interface driver for USB device control via the
 *          libusb or libusb-win32 packages.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_LIBUSB_H__
#define __I_LIBUSB_H__

typedef struct {
	MX_RECORD *record;

#if HAVE_LIBUSB
	struct usb_bus *usb_busses;
#endif

} MX_LIBUSB;

extern MX_RECORD_FUNCTION_LIST mxi_libusb_record_function_list;
extern MX_USB_FUNCTION_LIST mxi_libusb_usb_function_list;

extern mx_length_type mxi_libusb_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_libusb_rfield_def_ptr;

#define MXI_LIBUSB_STANDARD_FIELDS

/* Define all of the interface functions. */

MX_API mx_status_type mxi_libusb_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_libusb_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_libusb_delete_record( MX_RECORD *record );
MX_API mx_status_type mxi_libusb_open( MX_RECORD *record );
MX_API mx_status_type mxi_libusb_close( MX_RECORD *record );

MX_API mx_status_type mxi_libusb_enumerate_devices( MX_USB *usb );
MX_API mx_status_type mxi_libusb_find_device( MX_USB *usb,
					MX_USB_DEVICE **usb_device );
MX_API mx_status_type mxi_libusb_delete_device( MX_USB_DEVICE *usb_device );
MX_API mx_status_type mxi_libusb_reset_device( MX_USB_DEVICE *usb_device );
MX_API mx_status_type mxi_libusb_control_transfer( MX_USB_DEVICE *usb_device,
						int request_type,
						int request,
						int value,
						int index_or_offset,
						char *transfer_buffer,
						int transfer_buffer_length,
						int *num_bytes_transferred,
						double timeout );
MX_API mx_status_type mxi_libusb_bulk_read( MX_USB_DEVICE *usb_device,
						int endpoint_number,
						char *transfer_buffer,
						int transfer_buffer_length,
						int *num_bytes_read,
						double timeout );
MX_API mx_status_type mxi_libusb_bulk_write( MX_USB_DEVICE *usb_device,
						int endpoint_number,
						char *transfer_buffer,
						int transfer_buffer_length,
						int *num_bytes_written,
						double timeout );

#endif /* __I_LIBUSB_H__ */

