/*
 * Name:    i_newport_xps.h
 *
 * Purpose: Header file for Newport XPS motor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_NEWPORT_XPS_H__
#define __I_NEWPORT_XPS_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
} MX_NEWPORT_XPS;

#define MXI_NEWPORT_XPS_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_newport_xps_create_record_structures( MX_RECORD *record );

MX_API mx_status_type mxi_newport_xps_open( MX_RECORD *record );

MX_API mx_status_type mxi_newport_xps_command(
					MX_NEWPORT_XPS *newport_xps,
					char *command,
					char *response,
					size_t max_response_length,
					unsigned long newport_xps_flags );

extern MX_RECORD_FUNCTION_LIST mxi_newport_xps_record_function_list;

extern long mxi_newport_xps_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_newport_xps_rfield_def_ptr;

#endif /* __I_NEWPORT_XPS_H__ */

