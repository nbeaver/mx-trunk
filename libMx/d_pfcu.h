/*
 * Name:    d_pfcu.h
 *
 * Purpose: MX header file for using PF4 and PF2C2 filters and shutters
 *          controlled by a XIA PFCU controller.
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

#ifndef __D_PFCU_H__
#define __D_PFCU_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pfcu_record;
	int module_number;
	int filter_number;
} MX_PFCU_RELAY;

#define MXD_PFCU_FILTER_STANDARD_FIELDS \
  {-1, -1, "pfcu_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PFCU_RELAY, pfcu_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "module_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PFCU_RELAY, module_number),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "filter_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PFCU_RELAY, filter_number),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_PFCU_SHUTTER_STANDARD_FIELDS \
  {-1, -1, "pfcu_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PFCU_RELAY, pfcu_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "module_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PFCU_RELAY, module_number),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the device functions. */

MX_API mx_status_type mxd_pfcu_create_record_structures( MX_RECORD *record );

MX_API mx_status_type mxd_pfcu_relay_command( MX_RELAY *relay );

MX_API mx_status_type mxd_pfcu_get_relay_status( MX_RELAY *relay );

extern MX_RECORD_FUNCTION_LIST mxd_pfcu_record_function_list;
extern MX_RELAY_FUNCTION_LIST mxd_pfcu_relay_function_list;

extern long mxd_pfcu_filter_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pfcu_filter_rfield_def_ptr;

extern long mxd_pfcu_shutter_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pfcu_shutter_rfield_def_ptr;

#endif /* __D_PFCU_H__ */
