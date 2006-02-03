/*
 * Name:    i_driverlinx_portio.h
 *
 * Purpose: Header for MX driver for the DriverLINX port I/O driver for
 *          Windows NT/98/95 written by Scientific Software Tools, Inc.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2000, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_DRIVERLINX_PORTIO_H__
#define __I_DRIVERLINX_PORTIO_H__

/* Define all of the interface functions. */

MX_API mx_status_type mxi_driverlinx_portio_create_record_structures(
					MX_RECORD *record );

MX_API uint8_t mxi_driverlinx_portio_inp8( MX_RECORD *record,
					unsigned long port_number );

MX_API uint16_t mxi_driverlinx_portio_inp16( MX_RECORD *record,
					unsigned long port_number );

MX_API void mxi_driverlinx_portio_outp8( MX_RECORD *record,
					unsigned long port_number,
					uint8_t byte_value );

MX_API void mxi_driverlinx_portio_outp16( MX_RECORD *record,
					unsigned long port_number,
					uint16_t word_value );

MX_API mx_status_type mxi_driverlinx_portio_request_region( MX_RECORD *record,
					unsigned long port_number,
					unsigned long length );

MX_API mx_status_type mxi_driverlinx_portio_release_region( MX_RECORD *record,
					unsigned long port_number,
					unsigned long length );

typedef struct {
	int dummy;
} MX_DRIVERLINX_PORTIO;

extern MX_RECORD_FUNCTION_LIST mxi_driverlinx_portio_record_function_list;
extern MX_PORTIO_FUNCTION_LIST mxi_driverlinx_portio_portio_function_list;

extern mx_length_type mxi_driverlinx_portio_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_driverlinx_portio_rfield_def_ptr;

#endif /* __I_DRIVERLINX_PORTIO_H__ */

