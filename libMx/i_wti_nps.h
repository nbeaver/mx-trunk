/*
 * Name:    i_wti_nps.h
 *
 * Purpose: Header file for the Western Telematics Inc. Network Power Switch.
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

#ifndef __I_WTI_NPS_H__
#define __I_WTI_NPS_H__

/*---*/

#define MXI_WTI_NPS_PASSWORD_LENGTH		80

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	char password[ MXI_WTI_NPS_PASSWORD_LENGTH ];
	double version_number;
} MX_WTI_NPS;

#define MXI_WTI_NPS_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_WTI_NPS, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "password", MXFT_STRING, NULL, \
			1, {MXI_WTI_NPS_PASSWORD_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_WTI_NPS, password), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_NO_ACCESS) }, \
  \
  {-1, -1, "version_number", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_WTI_NPS, version_number), \
	{0}, NULL, MXFF_READ_ONLY }

MX_API mx_status_type mxi_wti_nps_create_record_structures( MX_RECORD *record );

MX_API mx_status_type mxi_wti_nps_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_wti_nps_record_function_list;

extern long mxi_wti_nps_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_wti_nps_rfield_def_ptr;

#endif /* __I_WTI_NPS_H__ */

