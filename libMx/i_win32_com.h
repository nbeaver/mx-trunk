/*
 * Name:    i_win32_com.h
 *
 * Purpose: Header for MX driver for Microsoft Win32 COM ports.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_WIN32_COM_H__
#define __I_WIN32_COM_H__

#include <windows.h>

#include "mx_record.h"

/* Define all of the interface functions. */

MX_API mx_status_type mxi_win32com_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_win32com_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_win32com_open( MX_RECORD *record );
MX_API mx_status_type mxi_win32com_close( MX_RECORD *record );

MX_API mx_status_type mxi_win32com_getchar( MX_RS232 *rs232, char *c );
MX_API mx_status_type mxi_win32com_putchar( MX_RS232 *rs232, char c );
MX_API mx_status_type mxi_win32com_read( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
MX_API mx_status_type mxi_win32com_write( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_write,
					size_t *bytes_written );
MX_API mx_status_type mxi_win32com_getline( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
MX_API mx_status_type mxi_win32com_putline( MX_RS232 *rs232,
					char *buffer,
					size_t *bytes_written );
MX_API mx_status_type mxi_win32com_num_input_bytes_available( MX_RS232 *rs232 );
MX_API mx_status_type mxi_win32com_discard_unread_input( MX_RS232 *rs232 );
MX_API mx_status_type mxi_win32com_discard_unwritten_output( MX_RS232 *rs232 );
MX_API mx_status_type mxi_win32com_get_signal_state( MX_RS232 *rs232 );
MX_API mx_status_type mxi_win32com_set_signal_state( MX_RS232 *rs232 );

/* Define the data structures used by a Microsoft Win32 COM interface. */

typedef struct {
	HANDLE handle;
	char filename[MXU_FILENAME_LENGTH + 1];

	int signal_state_initialized;
} MX_WIN32COM;

extern MX_RECORD_FUNCTION_LIST mxi_win32com_record_function_list;
extern MX_RS232_FUNCTION_LIST mxi_win32com_rs232_function_list;

extern mx_length_type mxi_win32com_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_win32com_rfield_def_ptr;

#define MXI_WIN32COM_STANDARD_FIELDS \
  {-1, -1, "filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_WIN32COM, filename), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_WIN32_COM_H__ */

