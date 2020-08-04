/*
 * Name:    i_zwo_efw.h
 *
 * Purpose: Header file for ZWO Electronic Filter Wheels
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_ZWO_EFW_H__
#define __I_ZWO_EFW_H__

#define MXF_ZWO_EFW_DEBUG_COMMANDS		0x1

#define MXU_ZWO_EFW_SDK_VERSION_NAME_LENGTH	20

typedef struct {
	MX_RECORD *record;

	unsigned long zwo_efw_flags;

	unsigned long maximum_num_filter_wheels;
	unsigned long current_num_filter_wheels;
	MX_RECORD **filter_wheel_record_array;

	long *filter_wheel_id_array;

	char sdk_version_name[MXU_ZWO_EFW_SDK_VERSION_NAME_LENGTH+1];

} MX_ZWO_EFW;

#define MXI_ZWO_EFW_STANDARD_FIELDS \
  {-1, -1, "zwo_efw_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ZWO_EFW, zwo_efw_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "maximum_num_filter_wheels", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ZWO_EFW, maximum_num_filter_wheels), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "current_num_filter_wheels", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ZWO_EFW, current_num_filter_wheels), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "filter_wheel_record_array", MXFT_RECORD, \
	  				NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ZWO_EFW, filter_wheel_record_array), \
	{ sizeof(MX_RECORD *) }, NULL, (MXFF_READ_ONLY | MXFF_VARARGS) }, \
  \
  {-1, -1, "filter_wheel_id_array", MXFT_LONG, \
	  				NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ZWO_EFW, filter_wheel_id_array), \
	{ sizeof(long) }, NULL, (MXFF_READ_ONLY | MXFF_VARARGS) }, \
  \
  {-1, -1, "sdk_version_name", MXFT_STRING, \
	  		NULL, 1, {MXU_ZWO_EFW_SDK_VERSION_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ZWO_EFW, sdk_version_name), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }

MX_API mx_status_type mxi_zwo_efw_create_record_structures(
						MX_RECORD *record );

MX_API mx_status_type mxi_zwo_efw_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_zwo_efw_record_function_list;

extern long mxi_zwo_efw_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_zwo_efw_rfield_def_ptr;

#endif /* __I_ZWO_EFW_H__ */

