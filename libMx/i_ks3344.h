/*
 * Name:    i_ks3344.h
 *
 * Purpose: Header for MX driver for CAMAC-based KS3344 RS-232 interfaces.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_KS3344_H__
#define __I_KS3344_H__

#include "mx_record.h"

/* Define all of the interface functions. */

MX_API mx_status_type mxi_ks3344_initialize_type( long type );
MX_API mx_status_type mxi_ks3344_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_ks3344_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_ks3344_delete_record( MX_RECORD *record );
MX_API mx_status_type mxi_ks3344_read_parms_from_hardware( MX_RECORD *record );
MX_API mx_status_type mxi_ks3344_write_parms_to_hardware( MX_RECORD *record );
MX_API mx_status_type mxi_ks3344_open( MX_RECORD *record );
MX_API mx_status_type mxi_ks3344_close( MX_RECORD *record );
MX_API mx_status_type mxi_ks3344_getchar( MX_RS232 *rs232, char *c );
MX_API mx_status_type mxi_ks3344_putchar( MX_RS232 *rs232, char c );
MX_API mx_status_type mxi_ks3344_read( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
MX_API mx_status_type mxi_ks3344_write( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_write,
					size_t *bytes_written );
MX_API mx_status_type mxi_ks3344_getline( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
MX_API mx_status_type mxi_ks3344_putline( MX_RS232 *rs232,
					char *buffer,
					size_t *bytes_written );
MX_API mx_status_type mxi_ks3344_num_input_bytes_available( MX_RS232 *rs232 );
MX_API mx_status_type mxi_ks3344_discard_unread_input( MX_RS232 *rs232 );
MX_API mx_status_type mxi_ks3344_discard_unwritten_output( MX_RS232 *rs232 );

/* Define the data structures used by a KS3344 RS-232 interface. */

typedef struct {
	MX_RECORD *camac_record;
	long slot;
	long subaddress;
	long max_read_retries;
} MX_KS3344;

extern MX_RECORD_FUNCTION_LIST mxi_ks3344_record_function_list;
extern MX_RS232_FUNCTION_LIST mxi_ks3344_rs232_function_list;

extern long mxi_ks3344_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_ks3344_rfield_def_ptr;

#define MXI_KS3344_STANDARD_FIELDS \
  {-1, -1, "camac_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KS3344, camac_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "slot", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KS3344, slot), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "subaddress", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KS3344, subaddress), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "max_read_retries", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KS3344, max_read_retries), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_KS3344_H__ */

