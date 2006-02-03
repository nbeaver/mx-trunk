/*
 * Name:   i_cxtilt02.h
 *
 * Purpose: Header for MX driver for Crossbow Technology's CXTILT02 series
 *          of digital inclinometers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_CXTILT02_H__
#define __I_CXTILT02_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	int resolution_level;

	short raw_pitch;
	short raw_roll;
} MX_CXTILT02;

#define MXI_CXTILT02_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CXTILT02, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "resolution_level", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CXTILT02, resolution_level), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \

MX_API mx_status_type mxi_cxtilt02_create_record_structures( MX_RECORD *record);
MX_API mx_status_type mxi_cxtilt02_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_cxtilt02_record_function_list;

extern mx_length_type mxi_cxtilt02_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_cxtilt02_rfield_def_ptr;

MX_API mx_status_type mxi_cxtilt02_command( MX_CXTILT02 *cxtilt02,
				char *command,
				char *response,
				size_t num_bytes_expected,
				double timeout_in_seconds,
				int debug_flag );

MX_API mx_status_type mxi_cxtilt02_read_angles( MX_CXTILT02 *cxtilt02 );

/* === Driver specific functions === */

#endif /* __I_CXTILT02_H__ */
