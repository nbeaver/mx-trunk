/*
 * Name:    i_amptek_dp5.h
 *
 * Purpose: Header file for Amptek MCAs that use the DP5 protocol.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_AMPTEK_DP5_H__
#define __I_AMPTEK_DP5_H__

/* Values for the 'amptek_dp5_flags' field. */

#define MXF_AMPTEK_DP5_DEBUG	0x1

/*---*/

#define MXF_AMPTEK_DP5_SETUP_GUID "{6A4E9A2D-9368-4f01-8E60-B3F9CDBAB5E8}"
#define MXF_AMPTEK_DP5_INTERFACE_GUID "{5A8ED6A1-7FC3-4b6a-A536-95DF35D03448}"

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
	int dummy;
} MX_AMPTEK_DP5_USB;

typedef struct {
	MX_RECORD *record;

	char interface_type_name[MXU_AMPTEK_DP5_TYPE_NAME_LENGTH+1];
	long interface_type;
	char interface_arguments[MXU_AMPTEK_DP5_ARGUMENTS_LENGTH+1];
	unsigned long amptek_dp5_flags;

	MX_RECORD *interface_record;

	union {
		MX_AMPTEK_DP5_ETHERNET ethernet;
		MX_AMPTEK_DP5_RS232 rs232;
		MX_AMPTEK_DP5_USB usb;
	} u;
} MX_AMPTEK_DP5;

#define MXLV_AMPTEK_DP5_FOO		86000

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
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_amptek_dp5_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxi_amptek_dp5_open( MX_RECORD *record );
MX_API mx_status_type mxi_amptek_dp5_special_processing_setup(
						MX_RECORD *record );

MX_API mx_status_type mxi_amptek_dp5_binary_command( MX_AMPTEK_DP5 *amptek_dp5,
					char *command,
					size_t command_length,
					char *response,
					size_t max_response_length,
					unsigned long amptek_dp5_flags );

MX_API mx_status_type mxi_amptek_dp5_ascii_command( MX_AMPTEK_DP5 *amptek_dp5,
					char *command,
					char *response,
					size_t max_response_length,
					unsigned long amptek_dp5_flags );

extern MX_RECORD_FUNCTION_LIST mxi_amptek_dp5_record_function_list;
extern MX_RECORD_FUNCTION_LIST mxi_amptek_dp5_record_function_list;

extern long mxi_amptek_dp5_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_amptek_dp5_rfield_def_ptr;

#endif /* __I_AMPTEK_DP5_H__ */
