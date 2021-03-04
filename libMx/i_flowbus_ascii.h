/*
 * Name:    i_flowbus_ascii.h
 *
 * Purpose: Header file for Bronkhorst FLOW-BUS ASCII protocol interface.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_FLOWBUS_ASCII_H__
#define __I_FLOWBUS_ASCII_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
} MX_FLOWBUS_ASCII;

#define MXI_FLOWBUS_ASCII_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS_ASCII, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_flowbus_ascii_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxi_flowbus_ascii_open( MX_RECORD *record );

MX_API mx_status_type mxi_flowbus_ascii_command(MX_FLOWBUS_ASCII *flowbus_ascii,
						char *command,
						char *response,
						size_t max_response_length );

extern MX_RECORD_FUNCTION_LIST mxi_flowbus_ascii_record_function_list;

extern long mxi_flowbus_ascii_num_record_fields;

extern MX_RECORD_FIELD_DEFAULTS *mxi_flowbus_ascii_rfield_def_ptr;

#endif /* __I_FLOWBUS_ASCII_H__ */

