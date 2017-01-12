/*
 * Name:    d_wti_nps_relay.h
 *
 * Purpose: MX header file for using a Western Telematics Inc. Network
 *          Power Switch power outlet as an MX relay record.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_WTI_NPS_RELAY_H__
#define __D_WTI_NPS_RELAY_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *wti_nps_record;
	unsigned long plug_number;
} MX_WTI_NPS_RELAY;

#define MXD_WTI_NPS_RELAY_STANDARD_FIELDS \
  {-1, -1, "wti_nps_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
	  offsetof(MX_WTI_NPS_RELAY, wti_nps_record),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "plug_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_WTI_NPS_RELAY, plug_number),\
	{0}, NULL, MXFF_IN_DESCRIPTION}

/* Define all of the device functions. */

MX_API mx_status_type mxd_wti_nps_relay_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_wti_nps_relay_open( MX_RECORD *record );

MX_API mx_status_type mxd_wti_nps_relay_relay_command( MX_RELAY *relay );

MX_API mx_status_type mxd_wti_nps_relay_get_relay_status( MX_RELAY *relay);

extern MX_RECORD_FUNCTION_LIST mxd_wti_nps_relay_record_function_list;
extern MX_RELAY_FUNCTION_LIST mxd_wti_nps_relay_relay_function_list;

extern long mxd_wti_nps_relay_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_wti_nps_relay_rfield_def_ptr;

#endif /* __D_WTI_NPS_RELAY_H__ */
