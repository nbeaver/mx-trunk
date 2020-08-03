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

#define MXF_ZWO_EFW_DEBUG_COMMANDS	0x1

typedef struct {
	MX_RECORD *record;

	unsigned long zwo_efw_flags;

	unsigned long maximum_num_motors;
	unsigned long current_num_motors;
	MX_RECORD **motor_record_array;

#if defined(EFW_FILTER_H)
#endif

} MX_ZWO_EFW;

#define MXI_ZWO_EFW_STANDARD_FIELDS \
  {-1, -1, "zwo_efw_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ZWO_EFW, zwo_efw_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "maximum_num_motors", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ZWO_EFW, maximum_num_motors), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "current_num_motors", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ZWO_EFW, current_num_motors), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "motor_record_array", MXFT_RECORD, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ZWO_EFW, motor_record_array), \
	{ sizeof(MX_RECORD *) }, NULL, (MXFF_READ_ONLY | MXFF_VARARGS) }

MX_API mx_status_type mxi_zwo_efw_create_record_structures(
						MX_RECORD *record );

MX_API mx_status_type mxi_zwo_efw_finish_record_initialization(
						MX_RECORD *record );

MX_API mx_status_type mxi_zwo_efw_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_zwo_efw_record_function_list;

extern long mxi_zwo_efw_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_zwo_efw_rfield_def_ptr;

#endif /* __I_ZWO_EFW_H__ */

