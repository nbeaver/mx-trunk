/*
 * Name:    mx_portio.c
 *
 * Purpose: I/O port access functions.
 *          
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"

#include "mx_portio.h"

/* Error checking for mx_portio_inp8, mx_portio_inp16, mx_portio_outp8,
 * and mx_portio_outp16 is dispensed with in the name of speed.
 */

MX_EXPORT uint8_t
mx_portio_inp8( MX_RECORD *record, unsigned long port_number )
{
	MX_PORTIO_FUNCTION_LIST *flist;
	uint8_t ( *fptr ) ( MX_RECORD *, unsigned long );
	uint8_t byte_value;

	flist = (MX_PORTIO_FUNCTION_LIST *)
			record->class_specific_function_list;

	fptr = flist->inp8;

	byte_value = (*fptr)( record, port_number );

	return byte_value;
}

MX_EXPORT uint16_t
mx_portio_inp16( MX_RECORD *record, unsigned long port_number )
{
	MX_PORTIO_FUNCTION_LIST *flist;
	uint16_t ( *fptr ) ( MX_RECORD *, unsigned long );
	uint16_t word_value;

	flist = (MX_PORTIO_FUNCTION_LIST *)
			record->class_specific_function_list;

	fptr = flist->inp16;

	word_value = (*fptr)( record, port_number );

	return word_value;
}

MX_EXPORT void
mx_portio_outp8( MX_RECORD *record,
		unsigned long port_number,
		uint8_t byte_value )
{
	MX_PORTIO_FUNCTION_LIST *flist;
	void ( *fptr ) ( MX_RECORD *, unsigned long, uint8_t );

	flist = (MX_PORTIO_FUNCTION_LIST *)
			record->class_specific_function_list;

	fptr = flist->outp8;

	(*fptr)( record, port_number, byte_value );

	return;
}

MX_EXPORT void
mx_portio_outp16( MX_RECORD *record,
		unsigned long port_number,
		uint16_t word_value )
{
	MX_PORTIO_FUNCTION_LIST *flist;
	void ( *fptr ) ( MX_RECORD *, unsigned long, uint16_t );

	flist = (MX_PORTIO_FUNCTION_LIST *)
			record->class_specific_function_list;

	fptr = flist->outp16;

	(*fptr)( record, port_number, word_value );

	return;
}

MX_EXPORT mx_status_type
mx_portio_request_region( MX_RECORD *record,
		unsigned long port_number,
		unsigned long length )
{
	const char fname[] = "mx_portio_request_region()";

	MX_PORTIO_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_RECORD *, unsigned long, unsigned long );
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	flist = (MX_PORTIO_FUNCTION_LIST *)
			record->class_specific_function_list;

	if ( flist == (MX_PORTIO_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PORTIO_FUNCTION_LIST for record '%s' is NULL.",
			record->name );
	}

	fptr = flist->request_region;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"request_region function pointer for record '%s' is NULL.",
			record->name );
	}

	status = (*fptr)( record, port_number, length );

	return status;
}

MX_EXPORT mx_status_type
mx_portio_release_region( MX_RECORD *record,
		unsigned long port_number,
		unsigned long length )
{
	const char fname[] = "mx_portio_release_region()";

	MX_PORTIO_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_RECORD *, unsigned long, unsigned long );
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	flist = (MX_PORTIO_FUNCTION_LIST *)
			record->class_specific_function_list;

	if ( flist == (MX_PORTIO_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PORTIO_FUNCTION_LIST for record '%s' is NULL.",
			record->name );
	}

	fptr = flist->release_region;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"release_region function pointer for record '%s' is NULL.",
			record->name );
	}

	status = (*fptr)( record, port_number, length );

	return status;
}

