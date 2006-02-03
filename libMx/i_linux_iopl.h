/*
 * Name:    i_linux_iopl.h
 *
 * Purpose: Header for MX driver for a port I/O driver using the Linux 
 *          iopl() or ioperm() functions.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002-2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_LINUX_IOPL_H__
#define __I_LINUX_IOPL_H__

/* Define all of the interface functions. */

MX_API mx_status_type mxi_linux_iopl_create_record_structures(
					MX_RECORD *record );

MX_API mx_status_type mxi_linux_iopl_open( MX_RECORD *record );

MX_API mx_status_type mxi_linux_iopl_close( MX_RECORD *record );

MX_API uint8_t mxi_linux_iopl_inp8( MX_RECORD *record,
					unsigned long port_number );

MX_API uint16_t mxi_linux_iopl_inp16( MX_RECORD *record,
					unsigned long port_number );

MX_API void mxi_linux_iopl_outp8( MX_RECORD *record,
					unsigned long port_number,
					uint8_t byte_value );

MX_API void mxi_linux_iopl_outp16( MX_RECORD *record,
					unsigned long port_number,
					uint16_t word_value );

MX_API mx_status_type mxi_linux_iopl_request_region( MX_RECORD *record,
					unsigned long port_number,
					unsigned long length );

MX_API mx_status_type mxi_linux_iopl_release_region( MX_RECORD *record,
					unsigned long port_number,
					unsigned long length );

typedef struct {
	int dummy;
} MX_LINUX_IOPL;

extern MX_RECORD_FUNCTION_LIST mxi_linux_iopl_record_function_list;
extern MX_PORTIO_FUNCTION_LIST mxi_linux_iopl_portio_function_list;

extern mx_length_type mxi_linux_iopl_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_linux_iopl_rfield_def_ptr;

#define MXI_LINUX_IOPL_STANDARD_FIELDS

#endif /* __I_LINUX_IOPL_H__ */

