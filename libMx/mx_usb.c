/*
 * Name:    mx_usb.c
 *
 * Purpose: MX function library for controlling a USB interface.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_types.h"
#include "mx_usb.h"
#include "mx_driver.h"

MX_EXPORT mx_status_type
mx_usb_get_pointers( MX_USB *usb,
			MX_USB_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_usb_get_pointers()";

	MX_RECORD *usb_record;

	if ( usb == (MX_USB *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The usb pointer passed by '%s' was NULL.",
			calling_fname );
	}

	usb_record = usb->record;

	if ( usb_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The record pointer for the MX_USB structure passed is NULL." );
	}

	if ( usb_record->mx_class != MXI_USB ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The record '%s' passed by '%s' is not a USB interface.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			usb_record->name, calling_fname,
			usb_record->mx_superclass,
			usb_record->mx_class,
			usb_record->mx_type );
	}

	if ( function_list_ptr == (MX_USB_FUNCTION_LIST **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The usb pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*function_list_ptr = (MX_USB_FUNCTION_LIST *)
				usb_record->class_specific_function_list;

	if ( *function_list_ptr == (MX_USB_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_USB_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
			usb_record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_usb_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mx_usb_finish_record_initialization()";

	MX_USB *usb;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	usb = (MX_USB *) record->record_class_struct;

	if ( usb == (MX_USB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_USB pointer for record '%s' is NULL.",
			record->name );
	}

	/* Create a handle table to store handles and pointers for
	 * MX_USB_DEVICE structures.  The table will be incremented
	 * in blocks of 100 handles.
	 */

	mx_status = mx_create_handle_table( &(usb->handle_table), 100, 1 );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_usb_enumerate_devices( MX_RECORD *usb_record )
{
	static const char fname[] = "mx_usb_enumerate_devices()";

	MX_USB *usb;
	MX_USB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_USB * );
	mx_status_type mx_status;

	if ( usb_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	usb = (MX_USB *) usb_record->record_class_struct;

	mx_status = mx_usb_get_pointers( usb, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->enumerate_devices;

	if ( fptr == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The 'enumerate_devices' function for the '%s' driver "
		"used by USB record '%s' is not yet implemented.",
			mx_get_driver_name( usb->record ),
			usb->record->name );
	}

	mx_status = (*fptr)( usb );

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mx_usb_find_device_by_order( MX_RECORD *usb_record,
				MX_USB_DEVICE **usb_device,
				unsigned long vendor_id,
				unsigned long product_id,
				int order_number,
				int configuration_number,
				int interface_number,
				int alternate_interface_number,
				int quiet_flag )
{
	static const char fname[] = "mx_usb_find_device_by_order()";

	MX_USB *usb;
	MX_USB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_USB *, MX_USB_DEVICE ** );
	mx_status_type mx_status;

	if ( usb_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}
	if ( usb_device == (MX_USB_DEVICE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_USB_DEVICE pointer passed was NULL." );
	}

	usb = (MX_USB *) usb_record->record_class_struct;

	mx_status = mx_usb_get_pointers( usb, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->find_device;

	if ( fptr == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The 'find_device' function for the '%s' driver "
		"used by USB record '%s' is not yet implemented.",
			mx_get_driver_name( usb->record ),
			usb->record->name );
	}

	usb->find_type = MXF_USB_FIND_TYPE_ORDER;

	usb->find.by_order.vendor_id = vendor_id;
	usb->find.by_order.product_id = product_id;
	usb->find.by_order.order_number = order_number;

	usb->configuration_number = configuration_number;
	usb->interface_number = interface_number;
	usb->alternate_interface_number = alternate_interface_number;

	usb->quiet_flag = quiet_flag;

	mx_status = (*fptr)( usb, usb_device );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_create_handle( &((*usb_device)->handle),
					usb->handle_table,
					*usb_device );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_usb_find_device_by_serial_number( MX_RECORD *usb_record,
				MX_USB_DEVICE **usb_device,
				unsigned long vendor_id,
				unsigned long product_id,
				char *serial_number,
				int configuration_number,
				int interface_number,
				int alternate_interface_number,
				int quiet_flag )
{
	static const char fname[] = "mx_usb_find_device_by_serial_number()";

	MX_USB *usb;
	MX_USB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_USB *, MX_USB_DEVICE ** );
	mx_status_type mx_status;

	if ( usb_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}
	if ( usb_device == (MX_USB_DEVICE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_USB_DEVICE pointer passed was NULL." );
	}

	usb = (MX_USB *) usb_record->record_class_struct;

	mx_status = mx_usb_get_pointers( usb, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->find_device;

	if ( fptr == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The 'find_device' function for the '%s' driver "
		"used by USB record '%s' is not yet implemented.",
			mx_get_driver_name( usb->record ),
			usb->record->name );
	}

	usb->find_type = MXF_USB_FIND_TYPE_SERIAL_NUMBER;

	usb->find.by_serial_number.vendor_id = vendor_id;
	usb->find.by_serial_number.product_id = product_id;
	usb->find.by_serial_number.serial_number = serial_number;

	usb->configuration_number = configuration_number;
	usb->interface_number = interface_number;
	usb->alternate_interface_number = alternate_interface_number;

	usb->quiet_flag = quiet_flag;

	mx_status = (*fptr)( usb, usb_device );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_create_handle( &((*usb_device)->handle),
					usb->handle_table,
					*usb_device );

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mx_usb_delete_device( MX_USB_DEVICE *usb_device )
{
	static const char fname[] = "mx_usb_delete_device()";

	MX_USB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_USB_DEVICE * );
	mx_status_type mx_status;

	if ( usb_device == (MX_USB_DEVICE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_USB_DEVICE pointer passed was NULL." );
	}

	mx_status = mx_usb_get_pointers( usb_device->usb, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->delete_device;

	if ( fptr == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The 'delete_device' function for the '%s' driver "
		"used by USB record '%s' is not yet implemented.",
			mx_get_driver_name( usb_device->usb->record ),
			usb_device->usb->record->name );
	}

	if ( usb_device->handle != MX_ILLEGAL_HANDLE ) {
		mx_status = mx_delete_handle( usb_device->handle,
						usb_device->usb->handle_table );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		usb_device->handle = MX_ILLEGAL_HANDLE;
	}

	mx_status = (*fptr)( usb_device );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_usb_reset_device( MX_USB_DEVICE *usb_device )
{
	static const char fname[] = "mx_usb_reset()";

	MX_USB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_USB_DEVICE * );
	mx_status_type mx_status;

	if ( usb_device == (MX_USB_DEVICE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_USB_DEVICE pointer passed was NULL." );
	}

	mx_status = mx_usb_get_pointers( usb_device->usb, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->reset_device;

	if ( fptr == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The 'reset' function for the '%s' driver "
		"used by USB record '%s' is not yet implemented.",
			mx_get_driver_name( usb_device->usb->record ),
			usb_device->usb->record->name );
	}

	mx_status = (*fptr)( usb_device );

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mx_usb_control_transfer( MX_USB_DEVICE *usb_device,
			int request_type,
			int request,
			int value,
			int index_or_offset,
			char *transfer_buffer,
			int transfer_buffer_length,
			int *num_bytes_transferred,
			double timeout )
{
	static const char fname[] = "mx_usb_control_transfer()";

	MX_USB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_USB_DEVICE *,
			int, int, int, int, char *, int, int *, double );
	mx_status_type mx_status;

	if ( usb_device == (MX_USB_DEVICE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_USB_DEVICE pointer passed was NULL." );
	}

	mx_status = mx_usb_get_pointers( usb_device->usb, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( transfer_buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The transfer_buffer pointer passed was NULL." );
	}

	fptr = fl_ptr->control_transfer;

	if ( fptr == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The 'control_transfer' function for the '%s' driver "
		"used by USB record '%s' is not yet implemented.",
			mx_get_driver_name( usb_device->usb->record ),
			usb_device->usb->record->name );
	}

	usb_device->request_type = request_type;
	usb_device->request = request;
	usb_device->value = value;
	usb_device->index_or_offset = index_or_offset;
	usb_device->transfer_buffer_length = transfer_buffer_length;
	usb_device->transfer_buffer = transfer_buffer;

	mx_status = (*fptr)( usb_device, request_type, request,
				value, index_or_offset,
				transfer_buffer, transfer_buffer_length,
				num_bytes_transferred, timeout );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_usb_bulk_read( MX_USB_DEVICE *usb_device,
			int endpoint_number,
			char *transfer_buffer,
			int transfer_buffer_length,
			int *num_bytes_read,
			double timeout )
{
	static const char fname[] = "mx_usb_bulk_read()";

	MX_USB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_USB_DEVICE *,
				int, char *, int, int *, double );
	mx_status_type mx_status;

	if ( usb_device == (MX_USB_DEVICE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_USB_DEVICE pointer passed was NULL." );
	}

	mx_status = mx_usb_get_pointers( usb_device->usb, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( transfer_buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The transfer_buffer pointer passed was NULL." );
	}

	fptr = fl_ptr->bulk_read;

	if ( fptr == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The 'bulk_read' function for the '%s' driver "
		"used by USB record '%s' is not yet implemented.",
			mx_get_driver_name( usb_device->usb->record ),
			usb_device->usb->record->name );
	}

	usb_device->transfer_buffer_length = transfer_buffer_length;

	usb_device->transfer_buffer = transfer_buffer;

	mx_status = (*fptr)( usb_device, endpoint_number,
			transfer_buffer, transfer_buffer_length,
			num_bytes_read, timeout );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_usb_bulk_write( MX_USB_DEVICE *usb_device,
			int endpoint_number,
			char *transfer_buffer,
			int transfer_buffer_length,
			int *num_bytes_written,
			double timeout )
{
	static const char fname[] = "mx_usb_bulk_write()";

	MX_USB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_USB_DEVICE *,
				int, char *, int, int *, double );
	mx_status_type mx_status;

	if ( usb_device == (MX_USB_DEVICE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_USB_DEVICE pointer passed was NULL." );
	}

	mx_status = mx_usb_get_pointers( usb_device->usb, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( transfer_buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The transfer_buffer pointer passed was NULL." );
	}

	fptr = fl_ptr->bulk_write;

	if ( fptr == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The 'bulk_write' function for the '%s' driver "
		"used by USB record '%s' is not yet implemented.",
			mx_get_driver_name( usb_device->usb->record ),
			usb_device->usb->record->name );
	}

	usb_device->transfer_buffer_length = transfer_buffer_length;

	usb_device->transfer_buffer = transfer_buffer;

	mx_status = (*fptr)( usb_device, endpoint_number,
			transfer_buffer, transfer_buffer_length,
			num_bytes_written, timeout );

	return mx_status;
}

