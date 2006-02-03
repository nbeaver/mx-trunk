/*
 * Name:    i_phidget_old_stepper.h
 *
 * Purpose: Header for the MX interface driver for the old non-HID Phidget
 *          stepper motor controller.
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

#ifndef __I_PHIDGET_OLD_STEPPER_H__
#define __I_PHIDGET_OLD_STEPPER_H__

#define MX_PHIDGET_OLD_STEPPER_VENDOR_ID		0x06c2

#define MX_PHIDGET_OLD_STEPPER_PRODUCT_ID		0x0046
#define MX_PHIDGET_OLD_STEPPER_BOOTLOADER_PRODUCT_ID	0x0060

#define MX_PHIDGET_OLD_STEPPER_BOOTLOADER_ENDPOINT	0x01

#define MX_PHIDGET_OLD_STEPPER_READ_ENDPOINT		0x82
#define MX_PHIDGET_OLD_STEPPER_WRITE_ENDPOINT		0x01

typedef struct {
	MX_RECORD *record;

	MX_RECORD *usb_record;
	int order_number;

	MX_USB_DEVICE *usb_device;
} MX_PHIDGET_OLD_STEPPER_CONTROLLER;

#define MXI_PHIDGET_OLD_STEPPER_STANDARD_FIELDS \
  {-1, -1, "usb_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PHIDGET_OLD_STEPPER_CONTROLLER, usb_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "order_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PHIDGET_OLD_STEPPER_CONTROLLER, order_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_phidget_old_stepper_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_phidget_old_stepper_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_phidget_old_stepper_record_function_list;

extern mx_length_type mxi_phidget_old_stepper_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_phidget_old_stepper_rfield_def_ptr;

#endif /* __I_PHIDGET_OLD_STEPPER_H__ */
