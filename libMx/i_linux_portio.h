/*
 * Name:    i_linux_portio.h
 *
 * Purpose: Header for MX driver for the Linux 'portio' device driver.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_LINUX_PORTIO_H__
#define __I_LINUX_PORTIO_H__

/* Define all of the interface functions. */

MX_API mx_status_type mxi_linux_portio_create_record_structures(
					MX_RECORD *record );

MX_API mx_status_type mxi_linux_portio_open( MX_RECORD *record );

MX_API mx_status_type mxi_linux_portio_close( MX_RECORD *record );

MX_API uint8_t mxi_linux_portio_inp8( MX_RECORD *record,
					unsigned long port_number );

MX_API uint16_t mxi_linux_portio_inp16( MX_RECORD *record,
					unsigned long port_number );

MX_API void mxi_linux_portio_outp8( MX_RECORD *record,
					unsigned long port_number,
					uint8_t byte_value );

MX_API void mxi_linux_portio_outp16( MX_RECORD *record,
					unsigned long port_number,
					uint16_t word_value );

MX_API mx_status_type mxi_linux_portio_request_region( MX_RECORD *record,
					unsigned long port_number,
					unsigned long length );

MX_API mx_status_type mxi_linux_portio_release_region( MX_RECORD *record,
					unsigned long port_number,
					unsigned long length );

typedef struct {
	char filename[MXU_FILENAME_LENGTH + 1];
	int file_handle;
} MX_LINUX_PORTIO;

extern MX_RECORD_FUNCTION_LIST mxi_linux_portio_record_function_list;
extern MX_PORTIO_FUNCTION_LIST mxi_linux_portio_portio_function_list;

extern long mxi_linux_portio_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_linux_portio_rfield_def_ptr;

#define MXI_LINUX_PORTIO_STANDARD_FIELDS \
  {-1, -1, "filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINUX_PORTIO, filename), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_LINUX_PORTIO_H__ */

