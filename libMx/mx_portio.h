/*
 * Name:    mx_portio.h
 *
 * Purpose: I/O port access functions.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_PORTIO_H__
#define __MX_PORTIO_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint8_t        ( *inp8 ) ( MX_RECORD *record,
						unsigned long port_number );
	uint16_t       ( *inp16 ) ( MX_RECORD *record,
						unsigned long port_number );
	void           ( *outp8 ) ( MX_RECORD *record,
						unsigned long port_number,
						uint8_t byte_value );
	void           ( *outp16 ) ( MX_RECORD *record,
						unsigned long port_number,
						uint16_t word_value );
	mx_status_type ( *request_region ) ( MX_RECORD *record,
						unsigned long port_number,
						unsigned long length );
	mx_status_type ( *release_region ) ( MX_RECORD *record,
						unsigned long port_number,
						unsigned long length );
} MX_PORTIO_FUNCTION_LIST;

MX_API uint8_t        mx_portio_inp8( MX_RECORD *record,
				unsigned long port_number );

MX_API uint16_t       mx_portio_inp16( MX_RECORD *record,
				unsigned long port_number );

MX_API void           mx_portio_outp8( MX_RECORD *record,
				unsigned long port_number,
				uint8_t value );

MX_API void           mx_portio_outp16( MX_RECORD *record,
				unsigned long port_number,
				uint16_t value );

MX_API mx_status_type mx_portio_request_region( MX_RECORD *record,
				unsigned long port_number,
				unsigned long length );

MX_API mx_status_type mx_portio_release_region( MX_RECORD *record,
				unsigned long port_number,
				unsigned long length );

#ifdef __cplusplus
}
#endif

#endif /* __MX_PORTIO_H__ */
