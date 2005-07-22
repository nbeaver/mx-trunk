/*
 * Name:    i_vxworks_rs232.h
 *
 * Purpose: Header for MX driver for VxWorks RS-232 devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_VXWORKS_RS232_H__
#define __I_VXWORKS_RS232_H__

#include "mx_record.h"

/* Define all of the interface functions. */

MX_API mx_status_type mxi_vxworks_rs232_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_vxworks_rs232_finish_record_initialization( MX_RECORD *record );
MX_API mx_status_type mxi_vxworks_rs232_open( MX_RECORD *record );
MX_API mx_status_type mxi_vxworks_rs232_close( MX_RECORD *record );

MX_API mx_status_type mxi_vxworks_rs232_getchar( MX_RS232 *rs232, char *c );
MX_API mx_status_type mxi_vxworks_rs232_putchar( MX_RS232 *rs232, char c );
MX_API mx_status_type mxi_vxworks_rs232_read( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
MX_API mx_status_type mxi_vxworks_rs232_write( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_write,
					size_t *bytes_written );
MX_API mx_status_type mxi_vxworks_rs232_getline( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
MX_API mx_status_type mxi_vxworks_rs232_putline( MX_RS232 *rs232,
					char *buffer,
					size_t *bytes_written );
MX_API mx_status_type mxi_vxworks_rs232_num_input_bytes_available(
							MX_RS232 *rs232 );
MX_API mx_status_type mxi_vxworks_rs232_discard_unread_input( MX_RS232 *rs232 );
MX_API mx_status_type mxi_vxworks_rs232_discard_unwritten_output(
							MX_RS232 *rs232 );
MX_API mx_status_type mxi_vxworks_rs232_get_signal_state( MX_RS232 *rs232 );
MX_API mx_status_type mxi_vxworks_rs232_set_signal_state( MX_RS232 *rs232 );

/* Define the data structures used by a VxWorks RS-232 interface. */

typedef struct {
	int file_handle;
	char filename[MXU_FILENAME_LENGTH + 1];
} MX_VXWORKS_RS232;

extern MX_RECORD_FUNCTION_LIST mxi_vxworks_rs232_record_function_list;
extern MX_RS232_FUNCTION_LIST mxi_vxworks_rs232_rs232_function_list;

extern long mxi_vxworks_rs232_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_vxworks_rs232_rfield_def_ptr;

#define MXI_VXWORKS_RS232_STANDARD_FIELDS \
  {-1, -1, "filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VXWORKS_RS232, filename), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_VXWORKS_RS232_H__ */

