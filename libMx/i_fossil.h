/*
 * Name:    i_fossil.h
 *
 * Purpose: Header for MX driver for MSDOS FOSSIL RS-232 devices.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_FOSSIL_H__
#define __I_FOSSIL_H__

#include "mx_record.h"

#define MXF_FOSSIL_SEND_ERROR		0x8000
#define MXF_FOSSIL_TIMED_OUT_ERROR	0x8000
#define MXF_FOSSIL_RECEIVE_ERROR	0x8e00
#define MXF_FOSSIL_STATUS_ERROR		0x8e00

#define MXF_FOSSIL_STATUS		0x00ff
#define MXF_FOSSIL_DATA			0x00ff

/* Define all of the interface functions. */

MX_API mx_status_type mxi_fossil_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_fossil_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_fossil_open( MX_RECORD *record );
MX_API mx_status_type mxi_fossil_close( MX_RECORD *record );

MX_API mx_status_type mxi_fossil_getchar( MX_RS232 *rs232, char *c );
MX_API mx_status_type mxi_fossil_putchar( MX_RS232 *rs232, char c );
MX_API mx_status_type mxi_fossil_num_input_bytes_available( MX_RS232 *rs232 );
MX_API mx_status_type mxi_fossil_discard_unread_input( MX_RS232 *rs232 );
MX_API mx_status_type mxi_fossil_discard_unwritten_output( MX_RS232 *rs232 );

/* Define the data structures used by a MSDOS FOSSIL interface. */

typedef struct {
	int port_number;
	int32_t max_read_retries;
	char filename[MXU_FILENAME_LENGTH + 1];
} MX_FOSSIL;

extern MX_RECORD_FUNCTION_LIST mxi_fossil_record_function_list;
extern MX_RS232_FUNCTION_LIST mxi_fossil_rs232_function_list;

extern mx_length_type mxi_fossil_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_fossil_rfield_def_ptr;

#define MXI_FOSSIL_STANDARD_FIELDS \
  {-1, -1, "filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FOSSIL, filename), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "max_read_retries", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FOSSIL, max_read_retries), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_FOSSIL_H__ */

