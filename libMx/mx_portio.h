/*
 * Name:    mx_portio.h
 *
 * Purpose: I/O port access functions.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_PORTIO_H__
#define __MX_PORTIO_H__

typedef struct {
	mx_uint8_type  ( *inp8 ) ( MX_RECORD *record,
						unsigned long port_number );
	mx_uint16_type ( *inp16 ) ( MX_RECORD *record,
						unsigned long port_number );
	void           ( *outp8 ) ( MX_RECORD *record,
						unsigned long port_number,
						mx_uint8_type byte_value );
	void           ( *outp16 ) ( MX_RECORD *record,
						unsigned long port_number,
						mx_uint16_type word_value );
	mx_status_type ( *request_region ) ( MX_RECORD *record,
						unsigned long port_number,
						unsigned long length );
	mx_status_type ( *release_region ) ( MX_RECORD *record,
						unsigned long port_number,
						unsigned long length );
} MX_PORTIO_FUNCTION_LIST;

MX_API mx_uint8_type  mx_portio_inp8( MX_RECORD *record,
				unsigned long port_number );

MX_API mx_uint16_type mx_portio_inp16( MX_RECORD *record,
				unsigned long port_number );

MX_API void           mx_portio_outp8( MX_RECORD *record,
				unsigned long port_number,
				mx_uint8_type value );

MX_API void           mx_portio_outp16( MX_RECORD *record,
				unsigned long port_number,
				mx_uint16_type value );

MX_API mx_status_type mx_portio_request_region( MX_RECORD *record,
				unsigned long port_number,
				unsigned long length );

MX_API mx_status_type mx_portio_release_region( MX_RECORD *record,
				unsigned long port_number,
				unsigned long length );

#endif /* __MX_PORTIO_H__ */
