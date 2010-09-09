/*
 * Name:    i_dos_portio.h
 *
 * Purpose: Header for MX driver for DOS style port I/O.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_DOS_PORTIO_H__
#define __I_DOS_PORTIO_H__

/* Define all of the interface functions. */

MX_API mx_status_type mxi_dos_portio_initialize_type( long type );

MX_API mx_status_type mxi_dos_portio_create_record_structures(
					MX_RECORD *record );

MX_API mx_status_type mxi_dos_portio_finish_record_initialization(
					MX_RECORD *record );

MX_API mx_status_type mxi_dos_portio_delete_record( MX_RECORD *record );

MX_API mx_status_type mxi_dos_portio_open( MX_RECORD *record );

MX_API mx_status_type mxi_dos_portio_close( MX_RECORD *record );

MX_API uint8_t mxi_dos_portio_inp8( MX_RECORD *record,
					unsigned long port_number );

MX_API uint16_t mxi_dos_portio_inp16( MX_RECORD *record,
					unsigned long port_number );

MX_API void mxi_dos_portio_outp8( MX_RECORD *record,
					unsigned long port_number,
					uint8_t byte_value );

MX_API void mxi_dos_portio_outp16( MX_RECORD *record,
					unsigned long port_number,
					uint16_t word_value );

MX_API mx_status_type mxi_dos_portio_request_region( MX_RECORD *record,
					unsigned long port_number,
					unsigned long length );

MX_API mx_status_type mxi_dos_portio_release_region( MX_RECORD *record,
					unsigned long port_number,
					unsigned long length );

extern MX_RECORD_FUNCTION_LIST mxi_dos_portio_record_function_list;
extern MX_PORTIO_FUNCTION_LIST mxi_dos_portio_portio_function_list;

extern long mxi_dos_portio_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_dos_portio_rfield_def_ptr;

#endif /* __I_DOS_PORTIO_H__ */

