/*
 * Name:    i_phidget_old_stepper.c
 *
 * Purpose: MX interface driver for the old non-HID Phidget stepper motor
 *          controller.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004-2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_PHIDGET_OLD_STEPPER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_usb.h"

#include "i_phidget_old_stepper.h"

/* The following include file contains the Phidget firmware. */

#include "i_phidget_old_stepper_firmware.h"

MX_RECORD_FUNCTION_LIST mxi_phidget_old_stepper_record_function_list = {
	NULL,
	mxi_phidget_old_stepper_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_phidget_old_stepper_open,
	NULL,
	NULL,
	mxi_phidget_old_stepper_open
};

MX_RECORD_FIELD_DEFAULTS mxi_phidget_old_stepper_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_PHIDGET_OLD_STEPPER_STANDARD_FIELDS
};

long mxi_phidget_old_stepper_num_record_fields
		= sizeof( mxi_phidget_old_stepper_rf_defaults )
		/ sizeof( mxi_phidget_old_stepper_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_phidget_old_stepper_rfield_def_ptr
			= &mxi_phidget_old_stepper_rf_defaults[0];

/* --- */

static mx_status_type
mxi_phidget_old_stepper_get_pointers(
	    MX_RECORD *phidget_old_stepper_controller_record,
	    MX_PHIDGET_OLD_STEPPER_CONTROLLER **phidget_old_stepper_controller,
	    MX_RECORD **usb_record,
	    const char *calling_fname )
{
	static const char fname[] = "mxi_phidget_old_stepper_get_pointers()";

	MX_PHIDGET_OLD_STEPPER_CONTROLLER *phidget_old_stepper_controller_ptr;

	if ( phidget_old_stepper_controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	phidget_old_stepper_controller_ptr =
		(MX_PHIDGET_OLD_STEPPER_CONTROLLER *)
		    phidget_old_stepper_controller_record->record_type_struct;

	if ( phidget_old_stepper_controller_ptr ==
			(MX_PHIDGET_OLD_STEPPER_CONTROLLER *) NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_PHIDGET_OLD_STEPPER_CONTROLLER pointer for record '%s' is NULL.",
			phidget_old_stepper_controller_record->name );
	}

	if ( phidget_old_stepper_controller != NULL ) {
		*phidget_old_stepper_controller
			= phidget_old_stepper_controller_ptr;
	}

	if ( usb_record != (MX_RECORD **) NULL ) {
		*usb_record = phidget_old_stepper_controller_ptr->usb_record;

		if ( (*usb_record) == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The usb_record pointer for record '%s' is NULL.",
				phidget_old_stepper_controller_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxi_phidget_old_stepper_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_phidget_old_stepper_create_record_structures()";

	MX_PHIDGET_OLD_STEPPER_CONTROLLER *phidget_old_stepper_controller;

	phidget_old_stepper_controller = (MX_PHIDGET_OLD_STEPPER_CONTROLLER *)
			malloc( sizeof(MX_PHIDGET_OLD_STEPPER_CONTROLLER) );

	if ( phidget_old_stepper_controller ==
			(MX_PHIDGET_OLD_STEPPER_CONTROLLER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
    "Can't allocate memory for MX_PHIDGET_OLD_STEPPER_CONTROLLER structure." );
	}

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	record->record_type_struct = phidget_old_stepper_controller;

	phidget_old_stepper_controller->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_phidget_old_stepper_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_phidget_old_stepper_open()";

	MX_PHIDGET_OLD_STEPPER_CONTROLLER *phidget_old_stepper_controller;
	MX_RECORD *usb_record;
	MX_USB_DEVICE *usb_device, *usb_bootloader_device;
	int num_bytes_written, order_number;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	usb_record = NULL;
	phidget_old_stepper_controller = NULL;

	mx_status = mxi_phidget_old_stepper_get_pointers( record,
						&phidget_old_stepper_controller,
						&usb_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_PHIDGET_OLD_STEPPER_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	phidget_old_stepper_controller->usb_device = NULL;

	order_number = (int) phidget_old_stepper_controller->order_number;

	/* See if the firmware has already been uploaded by checking for the
	 * presence of the primary device (product ID 0x0046).
	 */

	mx_status = mx_usb_find_device_by_order( usb_record, &usb_device,
				MX_PHIDGET_OLD_STEPPER_VENDOR_ID,
				MX_PHIDGET_OLD_STEPPER_PRODUCT_ID,
				order_number,
				1, 0, 0, TRUE );

	/* If we found the primary device, we are done now. */

	if ( mx_status.code == MXE_SUCCESS ) {
		phidget_old_stepper_controller->usb_device = usb_device;

#if MXI_PHIDGET_OLD_STEPPER_DEBUG
		MX_DEBUG(-2,("%s: No firmware upload necessary.", fname));
#endif
		return mx_status;
	}

	/* If we got any error code other than MXE_NOT_FOUND, then we
	 * return the error code to the caller.
	 */

	if ( mx_status.code != MXE_SUCCESS ) {
		if ( mx_status.code != MXE_NOT_FOUND ) {
			return mx_status;
		}
	}

	/* If the stepper firmware has not already been uploaded,
	 * try to upload it now.
	 */

	/* Look for the bootloader device (product ID 0x0060). */

	mx_status = mx_usb_find_device_by_order( usb_record,
				&usb_bootloader_device,
				MX_PHIDGET_OLD_STEPPER_VENDOR_ID,
				MX_PHIDGET_OLD_STEPPER_BOOTLOADER_PRODUCT_ID,
				order_number,
				1, 0, 0, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Upload the firmware. */

	mx_status = mx_usb_bulk_write( usb_bootloader_device,
				MX_PHIDGET_OLD_STEPPER_BOOTLOADER_ENDPOINT,
				(char *) STEPPER1_FIRMWARE,
				STEPPER1_FIRMWARE_SIZE,
				&num_bytes_written, 1.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_PHIDGET_OLD_STEPPER_DEBUG
	MX_DEBUG(-2,("%s: Firmware uploaded, num_bytes_written = %d",
		fname, num_bytes_written));
#endif

	/* We no longer need the bootloader device. */

	mx_status = mx_usb_delete_device( usb_bootloader_device );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Give the controller a little while to digest the upload. */

	mx_msleep( 1000 );

	/* Uploading the firmware should have made the stepper device appear,
	 * so we must reenumerate the devices now.
	 */

	mx_status = mx_usb_enumerate_devices( usb_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now look for the primary device again. */

	mx_status = mx_usb_find_device_by_order( usb_record, &usb_device,
			MX_PHIDGET_OLD_STEPPER_VENDOR_ID,
			MX_PHIDGET_OLD_STEPPER_PRODUCT_ID,
			order_number,
			1, 0, 0, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	phidget_old_stepper_controller->usb_device = usb_device;

#if MXI_PHIDGET_OLD_STEPPER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

