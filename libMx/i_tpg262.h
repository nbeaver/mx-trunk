/*
 * Name:    i_tpg262.h
 *
 * Purpose: Header file for the MX interface driver for Pfeiffer TPG 261
 *          and TPG 262 vacuum gauge controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_TPG262_H__
#define __I_TPG262_H__

/* Values for the 'tpg262_flags' field. */

/* Values for the 'debug_flag' argument to mxi_tpg262_command(). */

#define MXF_TPG262_ERR_COMMAND_IN_PROGRESS	0x8000000

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	unsigned long tpg262_flags;
} MX_TPG262;

#define MXI_TPG262_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TPG262, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "tpg262_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TPG262, tpg262_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \

MX_API mx_status_type mxi_tpg262_create_record_structures( MX_RECORD *record );

MX_API mx_status_type mxi_tpg262_open( MX_RECORD *record );

MX_API mx_status_type mxi_tpg262_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_tpg262_command( MX_TPG262 *tpg262,
					char *command,
					char *response,
					size_t response_buffer_length,
					int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxi_tpg262_record_function_list;

extern long mxi_tpg262_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_tpg262_rfield_def_ptr;

#endif /* __I_TPG262_H__ */

