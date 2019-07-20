/*
 * Name:    i_amptek_dp4.h
 *
 * Purpose: Header file for Amptek MCAs that use the DP4 protocol.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_AMPTEK_DP4_H__
#define __I_AMPTEK_DP4_H__

#define MXU_AMPTEK_DP4_SERIAL_NUMBER_LENGTH 	20

/* Values for the 'amptek_dp4_flags' field. */

#define MXF_AMPTEK_DP4_DEBUG_RAW		0x1
#define MXF_AMPTEK_DP4_DEBUG_ASCII		0x2

#define MXF_AMPTEK_DP4_FIND_BY_ORDER		0x20

/*---*/

#define MXT_AMPTEK_DP4_VENDOR_ID		0x10E9
#define MXT_AMPTEK_DP4_PRODUCT_ID		0x0700

#define MXT_AMPTEK_DP4_CONTROL_ENDPOINT		0
#define MXT_AMPTEK_DP4_READ_ENDPOINT		1
#define MXT_AMPTEK_DP4_WRITE_ENDPOINT		2

/*---*/

typedef struct {
	MX_RECORD *record;

	MX_RECORD *usb_record;
	char serial_number[ MXU_AMPTEK_DP4_SERIAL_NUMBER_LENGTH + 1 ];
	unsigned long amptek_dp4_flags;

	MX_USB_DEVICE *usb_device;
} MX_AMPTEK_DP4;

#define MXLV_AMPTEK_DP4_FOO			87001

#define MXI_AMPTEK_DP4_STANDARD_FIELDS \
  {-1, -1, "usb_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP4, usb_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "serial_number", MXFT_STRING, NULL, \
		1, {MXU_AMPTEK_DP4_SERIAL_NUMBER_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP4, serial_number), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "amptek_dp4_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP4, amptek_dp4_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_amptek_dp4_command(
				MX_AMPTEK_DP4 *amptek_dp4,
       				unsigned long amptek_dp4_flags );

extern MX_RECORD_FUNCTION_LIST mxi_amptek_dp4_record_function_list;
extern MX_RECORD_FUNCTION_LIST mxi_amptek_dp4_record_function_list;

extern long mxi_amptek_dp4_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_amptek_dp4_rfield_def_ptr;

MX_API mx_status_type mxi_amptek_dp4_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_amptek_dp4_open( MX_RECORD *record );
MX_API mx_status_type mxi_amptek_dp4_special_processing_setup(
							MX_RECORD *record );

#endif /* __I_AMPTEK_DP4_H__ */

