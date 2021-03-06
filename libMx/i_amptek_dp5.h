/*
 * Name:    i_amptek_dp5.h
 *
 * Purpose: Header file for Amptek MCAs that use the DP5 protocol.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2017-2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_AMPTEK_DP5_H__
#define __I_AMPTEK_DP5_H__

/* Values for the 'amptek_dp5_flags' field. */

#define MXF_AMPTEK_DP5_DEBUG_RAW			0x1
#define MXF_AMPTEK_DP5_DEBUG_ASCII			0x2

#define MXF_AMPTEK_DP5_RESET_TO_DEFAULTS		0x10
#define MXF_AMPTEK_DP5_FIND_BY_ORDER			0x20

/*---*/

#define MXU_AMPTEK_DP5_HEADER_LENGTH	6

#define MXU_AMPTEK_DP5_CHECKSUM_LENGTH	2

#define MXU_AMPTEK_DP5_HEADER_AND_CHECKSUM_LENGTH \
	( MXU_AMPTEK_DP5_HEADER_LENGTH + MXU_AMPTEK_DP5_CHECKSUM_LENGTH )

#define MXU_AMPTEK_DP5_MAX_READ_DATA_LENGTH	32767
#define MXU_AMPTEK_DP5_MAX_WRITE_DATA_LENGTH	512

#define MXU_AMPTEK_DP5_MAX_READ_PACKET_LENGTH \
  ( MXU_AMPTEK_DP5_HEADER_AND_CHECKSUM_LENGTH \
    + MXU_AMPTEK_DP5_MAX_READ_DATA_LENGTH )

#define MXU_AMPTEK_DP5_MAX_WRITE_PACKET_LENGTH \
  ( MXU_AMPTEK_DP5_HEADER_AND_CHECKSUM_LENGTH \
    + MXU_AMPTEK_DP5_MAX_WRITE_DATA_LENGTH )

/*---*/

#define MXT_AMPTEK_DP5_VENDOR_ID	0x10C4
#define MXT_AMPTEK_DP5_PRODUCT_ID	0x842A

#define MXT_AMPTEK_DP5_SETUP_GUID "{6A4E9A2D-9368-4f01-8E60-B3F9CDBAB5E8}"
#define MXT_AMPTEK_DP5_INTERFACE_GUID "{5A8ED6A1-7FC3-4b6a-A536-95DF35D03448}"

#define MXT_AMPTEK_DP5_CONTROL_ENDPOINT		0
#define MXT_AMPTEK_DP5_READ_ENDPOINT		1
#define MXT_AMPTEK_DP5_WRITE_ENDPOINT		2

/*---*/

#define MXU_AMPTEK_DP5_TYPE_NAME_LENGTH		20
#define MXU_AMPTEK_DP5_ARGUMENTS_LENGTH		80

typedef struct {
	int dummy;
} MX_AMPTEK_DP5_ETHERNET;

typedef struct {
	int dummy;
} MX_AMPTEK_DP5_RS232;

typedef struct {
	MX_USB_DEVICE *usb_device;
	char serial_number[20];
} MX_AMPTEK_DP5_USB;

typedef struct {
	MX_RECORD *record;

	char interface_type_name[MXU_AMPTEK_DP5_TYPE_NAME_LENGTH+1];
	long interface_type;
	char interface_arguments[MXU_AMPTEK_DP5_ARGUMENTS_LENGTH+1];
	unsigned long amptek_dp5_flags;
	double timeout;
	char load_filename[MXU_FILENAME_LENGTH+1];

	char save_filename[MXU_FILENAME_LENGTH+1];

	unsigned long firmware_version;
	unsigned long fpga_version;
	unsigned long serial_number;

	MX_RECORD *interface_record;

	union {
		MX_AMPTEK_DP5_ETHERNET ethernet;
		MX_AMPTEK_DP5_RS232 rs232;
		MX_AMPTEK_DP5_USB usb;
	} u;

	char get_configuration[MXU_AMPTEK_DP5_MAX_READ_PACKET_LENGTH+1];
	char set_configuration[MXU_AMPTEK_DP5_MAX_WRITE_PACKET_LENGTH+1];
	char save_configuration[MXU_AMPTEK_DP5_MAX_WRITE_PACKET_LENGTH+1];
} MX_AMPTEK_DP5;

#define MXLV_AMPTEK_DP5_LOAD_FILENAME			86001
#define MXLV_AMPTEK_DP5_SAVE_FILENAME			86002
#define MXLV_AMPTEK_DP5_GET_CONFIGURATION		86003
#define MXLV_AMPTEK_DP5_SET_CONFIGURATION		86004
#define MXLV_AMPTEK_DP5_SAVE_CONFIGURATION		86005

#define MXI_AMPTEK_DP5_STANDARD_FIELDS \
  {-1, -1, "interface_type_name", MXFT_STRING, NULL, \
	  			1, {MXU_AMPTEK_DP5_TYPE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP5, interface_type_name), \
	{sizeof(char)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY)}, \
  \
  {-1, -1, "interface_arguments", MXFT_STRING, NULL, \
	  			1, {MXU_AMPTEK_DP5_ARGUMENTS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP5, interface_arguments), \
	{sizeof(char)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY)}, \
  \
  {-1, -1, "amptek_dp5_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP5, amptek_dp5_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "timeout", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP5,timeout), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_AMPTEK_DP5_LOAD_FILENAME, -1, "load_filename", MXFT_STRING, NULL, \
	  					1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP5, load_filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_AMPTEK_DP5_SAVE_FILENAME, -1, "save_filename", MXFT_STRING, NULL, \
	  					1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP5, save_filename), \
	{sizeof(char)}, NULL, 0}, \
  \
  {-1, -1, "firmware_version", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP5, firmware_version), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "fpga_version", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP5, fpga_version), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "serial_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP5, serial_number), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AMPTEK_DP5_GET_CONFIGURATION, -1, "get_configuration", MXFT_STRING, \
	  		NULL, 1, {MXU_AMPTEK_DP5_MAX_READ_PACKET_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP5, get_configuration), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AMPTEK_DP5_SET_CONFIGURATION, -1, "set_configuration", MXFT_STRING, \
	  		NULL, 1, {MXU_AMPTEK_DP5_MAX_READ_PACKET_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP5, set_configuration), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AMPTEK_DP5_SAVE_CONFIGURATION, -1, "save_configuration", MXFT_STRING, \
	  		NULL, 1, {MXU_AMPTEK_DP5_MAX_READ_PACKET_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP5, save_configuration), \
	{sizeof(char)}, NULL, 0}, \

MX_API mx_status_type mxi_amptek_dp5_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxi_amptek_dp5_open( MX_RECORD *record );
MX_API mx_status_type mxi_amptek_dp5_special_processing_setup(
						MX_RECORD *record );

MX_API mx_status_type mxi_amptek_dp5_ascii_command(
				MX_AMPTEK_DP5 *amptek_dp5,
				char *ascii_command,
				char *ascii_response,
				unsigned long max_ascii_response_length,
				mx_bool_type change_default,
				unsigned long amptek_dp5_flags );

MX_API mx_status_type mxi_amptek_dp5_binary_command(
				MX_AMPTEK_DP5 *amptek_dp5,
				long command_pid1, long command_pid2,
				long *response_pid1, long *response_pid2,
				char *binary_command,
				unsigned long binary_command_length,
				char *binary_response,
				unsigned long max_binary_response_length,
				unsigned long *actual_binary_response_length,
       				unsigned long amptek_dp5_flags );

MX_API mx_status_type mxi_amptek_dp5_raw_command(
				MX_AMPTEK_DP5 *amptek_dp5,
				char *raw_command,
				char *raw_response,
				unsigned long max_raw_response_length,
				unsigned long *actual_raw_response_length,
				unsigned long amptek_dp5_flags );

extern MX_RECORD_FUNCTION_LIST mxi_amptek_dp5_record_function_list;
extern MX_RECORD_FUNCTION_LIST mxi_amptek_dp5_record_function_list;

extern long mxi_amptek_dp5_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_amptek_dp5_rfield_def_ptr;

#endif /* __I_AMPTEK_DP5_H__ */

