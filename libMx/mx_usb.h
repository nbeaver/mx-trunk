/*
 * Name:    mx_usb.h
 *
 * Purpose: Header file for USB interfaces.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_USB_H__
#define __MX_USB_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#include "mx_handle.h"

#define MXF_USB_FIND_TYPE_ORDER		1
#define MXF_USB_FIND_TYPE_SERIAL_NUMBER	2
#define MXF_USB_FIND_TYPE_BUS_ADDRESS	3

union mx_usb_find_device_union {
	struct {
		unsigned long vendor_id;
		unsigned long product_id;
		int order_number;
	} by_order;

	struct {
		unsigned long vendor_id;
		unsigned long product_id;
		char *serial_number;
	} by_serial_number;
};

typedef struct {
	MX_RECORD *record;

	unsigned long usb_flags;

	int find_type;
	int quiet_flag;
	union mx_usb_find_device_union find;

	int configuration_number;
	int interface_number;
	int alternate_interface_number;

	MX_HANDLE_TABLE *handle_table;

	void *bus_data;
} MX_USB;

typedef struct {
	MX_USB	*usb;
	unsigned long vendor_id;
	unsigned long product_id;
	int order_number;

	signed long handle;

	int configuration_number;
	int interface_number;
	int alternate_interface_number;

	int endpoint_number;

	int request_type;
	int request;
	int value;
	int index_or_offset;
	char *transfer_buffer;
	int transfer_buffer_length;

	void *device_data;
} MX_USB_DEVICE;

#define MX_USB_STANDARD_FIELDS \
  {-1, -1, "usb_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_USB, usb_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

typedef struct {
	mx_status_type ( *enumerate_devices )( MX_USB *usb );
	mx_status_type ( *find_device )( MX_USB *usb,
					MX_USB_DEVICE **usb_device );
	mx_status_type ( *delete_device )( MX_USB_DEVICE *usb_device );
	mx_status_type ( *reset_device )( MX_USB_DEVICE *usb_device );
	mx_status_type ( *control_transfer )( MX_USB_DEVICE *usb_device,
						int request_type,
						int request,
						int value,
						int index_or_offset,
						char *transfer_buffer,
						int transfer_buffer_length,
						int *num_bytes_transferred,
						double timeout );
	mx_status_type ( *bulk_read )( MX_USB_DEVICE *usb_device,
					int endpoint_number,
					char *transfer_buffer,
					int transfer_buffer_length,
					int *num_bytes_read,
					double timeout );
	mx_status_type ( *bulk_write )( MX_USB_DEVICE *usb_device,
					int endpoint_number,
					char *transfer_buffer,
					int transfer_buffer_length,
					int *num_bytes_written,
					double timeout );
} MX_USB_FUNCTION_LIST;

/* ============== Interface function prototypes. ============== */

MX_API mx_status_type mx_usb_get_pointers( MX_USB *usb,
					MX_USB_FUNCTION_LIST **fptr,
					const char *calling_fname );

MX_API mx_status_type mx_usb_finish_record_initialization( MX_RECORD *record );

MX_API mx_status_type mx_usb_enumerate_devices( MX_RECORD *usb_record );

/*---*/

MX_API mx_status_type mx_usb_find_device_by_order( MX_RECORD *usb_record,
					MX_USB_DEVICE **usb_device,
					unsigned long vendor_id,
					unsigned long product_id,
					int order_number,
					int configuration_number,
					int interface_number,
					int alternate_interface_number,
					int quiet_flag );

MX_API mx_status_type mx_usb_find_device_by_serial_number(MX_RECORD *usb_record,
					MX_USB_DEVICE **usb_device,
					unsigned long vendor_id,
					unsigned long product_id,
					char *serial_number,
					int configuration_number,
					int interface_number,
					int alternate_interface_number,
					int quiet_flag );

MX_API mx_status_type mx_usb_find_device_by_bus_address( MX_RECORD *usb_record,
					MX_USB_DEVICE **usb_device,
					int bus_number,
					int device_number,
					int configuration_number,
					int interface_number,
					int alternate_interface_number,
					int quiet_flag );

/*---*/

MX_API mx_status_type mx_usb_delete_device( MX_USB_DEVICE *usb_device );

MX_API mx_status_type mx_usb_reset_device( MX_USB_DEVICE *usb_device );

/*---*/

MX_API mx_status_type mx_usb_control_transfer( MX_USB_DEVICE *usb_device,
					int request_type,
					int request,
					int value,
					int index_or_offset,
					char *transfer_buffer,
					int transfer_buffer_length,
					int *num_bytes_transferred,
					double timeout );

MX_API mx_status_type mx_usb_bulk_read( MX_USB_DEVICE *usb_device,
					int endpoint_number,
					char *transfer_buffer,
					int transfer_buffer_length,
					int *num_bytes_read,
					double timeout );

MX_API mx_status_type mx_usb_bulk_write( MX_USB_DEVICE *usb_device,
					int endpoint_number,
					char *transfer_buffer,
					int transfer_buffer_length,
					int *num_bytes_written,
					double timeout );

#ifdef __cplusplus
}
#endif

#endif /* __MX_USB_H__ */
