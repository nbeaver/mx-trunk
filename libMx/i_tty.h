/*
 * Name:    i_tty.h
 *
 * Purpose: Header for MX driver for Unix TTY RS-232 devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2006, 2010, 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_TTY_H__
#define __I_TTY_H__

#include "mx_record.h"

/* Define all of the interface functions. */

MX_API mx_status_type mxi_tty_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_tty_finish_record_initialization( MX_RECORD *record );
MX_API mx_status_type mxi_tty_open( MX_RECORD *record );
MX_API mx_status_type mxi_tty_close( MX_RECORD *record );
MX_API mx_status_type mxi_tty_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_tty_getchar( MX_RS232 *rs232, char *c );
MX_API mx_status_type mxi_tty_putchar( MX_RS232 *rs232, char c );
MX_API mx_status_type mxi_tty_read( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
MX_API mx_status_type mxi_tty_write( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_write,
					size_t *bytes_written );
MX_API mx_status_type mxi_tty_getline( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
MX_API mx_status_type mxi_tty_putline( MX_RS232 *rs232,
					char *buffer,
					size_t *bytes_written );

MX_API mx_status_type mxi_tty_fionread_num_input_bytes_available(
							MX_RS232 *rs232 );
MX_API mx_status_type mxi_tty_select_num_input_bytes_available(
							MX_RS232 *rs232 );

#if HAVE_FIONREAD_FOR_TTY_PORTS
#define mxi_tty_num_input_bytes_available \
				mxi_tty_fionread_num_input_bytes_available
#else
#define mxi_tty_num_input_bytes_available \
				mxi_tty_select_num_input_bytes_available
#endif

MX_API mx_status_type mxi_tty_discard_unread_input( MX_RS232 *rs232 );
MX_API mx_status_type mxi_tty_discard_unwritten_output( MX_RS232 *rs232 );
MX_API mx_status_type mxi_tty_get_signal_state( MX_RS232 *rs232 );
MX_API mx_status_type mxi_tty_set_signal_state( MX_RS232 *rs232 );

MX_API mx_status_type mxi_tty_posix_termios_get_configuration(
						MX_RS232 *rs232 );
MX_API mx_status_type mxi_tty_posix_termios_set_configuration(
						MX_RS232 *rs232 );

MX_API mx_status_type mxi_tty_send_break( MX_RS232 *rs232 );

MX_API mx_status_type mxi_tty_get_echo( MX_RS232 *rs232 );
MX_API mx_status_type mxi_tty_set_echo( MX_RS232 *rs232 );

/* Define the data structures used by a Unix TTY interface. */

typedef struct {
	int file_handle;
	char filename[MXU_FILENAME_LENGTH + 1];
} MX_TTY;

extern MX_RECORD_FUNCTION_LIST mxi_tty_record_function_list;
extern MX_RS232_FUNCTION_LIST mxi_tty_rs232_function_list;

extern long mxi_tty_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_tty_rfield_def_ptr;

#define MXI_TTY_STANDARD_FIELDS \
  {-1, -1, "filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TTY, filename), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_TTY_H__ */

