/* 
 * Name:    d_amptek_dp4_mca.h
 *
 * Purpose: Header file for Amptek MCAs that use the DP4 protocol.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_AMPTEK_DP4_MCA_H__
#define __D_AMPTEK_DP4_MCA_H__

#define MXU_AMPTEK_DP4_SERIAL_NUMBER_LENGTH 		20

#define MXU_AMPTEK_DP4_CONFIGURATION_DATA_LENGTH	64

/* Values for the 'amptek_dp4_flags' field. */

#define MXF_AMPTEK_DP4_DEBUG_DATA			0x1

#define MXF_AMPTEK_DP4_FIND_BY_ORDER			0x20

/*---*/

#define MXT_AMPTEK_DP4_VENDOR_ID			0x10E9
#define MXT_AMPTEK_DP4_PRODUCT_ID			0x0700

/* Endpoint number definitions. */

#define MXEP_AMPTEK_DP4_CONTROL				0

#define MXEP_AMPTEK_DP4_CONFIGURATION_OUT		1	/* output */

#define MXEP_AMPTEK_DP4_BUFFER_A_STATUS			1	/* input */
#define MXEP_AMPTEK_DP4_BUFFER_A_SPECTRUM		2	/* input */
#define MXEP_AMPTEK_DP4_BUFFER_B_STATUS			3	/* input */
#define MXEP_AMPTEK_DP4_BUFFER_B_SPECTRUM		4	/* input */
#define MXEP_AMPTEK_DP4_CONFIGURATION_IN		5	/* input */
#define MXEP_AMPTEK_DP4_OSCILLOSCOPE_TRACE_DATA		6	/* input */

/*---*/

typedef struct {
	MX_RECORD *record;

	MX_RECORD *usb_record;
	char serial_number[ MXU_AMPTEK_DP4_SERIAL_NUMBER_LENGTH + 1 ];
	unsigned long amptek_dp4_flags;
	double timeout;

	MX_USB_DEVICE *usb_device;

	char configuration_data[ MXU_AMPTEK_DP4_CONFIGURATION_DATA_LENGTH ];
} MX_AMPTEK_DP4_MCA;

#define MXLV_AMPTEK_DP4_FOO			87001

#define MXD_AMPTEK_DP4_STANDARD_FIELDS \
  {-1, -1, "usb_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP4_MCA, usb_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "serial_number", MXFT_STRING, NULL, \
		1, {MXU_AMPTEK_DP4_SERIAL_NUMBER_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP4_MCA, serial_number), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "amptek_dp4_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP4_MCA, amptek_dp4_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "timeout", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP4_MCA, timeout), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

MX_API mx_status_type mxd_amptek_dp4_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_amptek_dp4_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_amptek_dp4_open( MX_RECORD *record );
MX_API mx_status_type mxd_amptek_dp4_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_amptek_dp4_trigger( MX_MCA *mca );
MX_API mx_status_type mxd_amptek_dp4_stop( MX_MCA *mca );
MX_API mx_status_type mxd_amptek_dp4_read( MX_MCA *mca );
MX_API mx_status_type mxd_amptek_dp4_clear( MX_MCA *mca );
MX_API mx_status_type mxd_amptek_dp4_get_status( MX_MCA *mca );
MX_API mx_status_type mxd_amptek_dp4_get_parameter( MX_MCA *mca );
MX_API mx_status_type mxd_amptek_dp4_set_parameter( MX_MCA *mca );

extern MX_RECORD_FUNCTION_LIST mxd_amptek_dp4_record_function_list;
extern MX_MCA_FUNCTION_LIST mxd_amptek_dp4_mca_function_list;

extern long mxd_amptek_dp4_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_amptek_dp4_rfield_def_ptr;

#endif /* __D_AMPTEK_DP4_MCA_H__ */

