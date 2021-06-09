/*
 * Name:    i_flowbus_format.c
 *
 * Purpose: Bronkhorst FLOW-BUS ASCII protocol string formatting.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "i_flowbus.h"

/*---*/

/* NOTE: We have to use the MX datatype here rather than the
 * FlowBus parameter type since FlowBus regrettably assigns
 * the same parameter type to 32-bit unsigned longs and to
 * 32-bit floats.  But we need to handle those two cases
 * differently here.
 */

MX_EXPORT mx_status_type
mxi_flowbus_format_string( char *external_buffer,
			size_t external_buffer_bytes,
			unsigned long field_number,
			unsigned long mx_datatype,
			void *value_ptr )
{
	static const char fname[] = "mxi_flowbus_format_string()";

	uint8_t uint8_value;
	uint16_t uint16_value;
	uint32_t uint32_value;
	float float_value;
	size_t i, needed_field_bytes, needed_buffer_bytes;
	size_t field_buffer_offset;
	char *field_buffer_ptr;

	char temp_buffer[500];

	if ( external_buffer == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The external_buffer pointer passed was NULL." );
	}
	if ( value_ptr == (void *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The value_ptr pointer passed was NULL." );
	}

	field_buffer_offset = (2 * (field_number - 1)) + 1;

	field_buffer_ptr = external_buffer + field_buffer_offset;

	switch( mx_datatype ) {
	case MXFT_UCHAR:
		needed_field_bytes = 2;
		break;
	case MXFT_USHORT:
		needed_field_bytes = 4;
		break;
	case MXFT_ULONG:
	case MXFT_FLOAT:
		needed_field_bytes = 8;
		break;
	case MXFT_STRING:
		needed_field_bytes = strlen( (char *) value_ptr );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported MX datatype %lu requested for Flowbus.",
			mx_datatype );
		break;
	}

	needed_buffer_bytes = field_buffer_offset + needed_field_bytes;

	if ( needed_buffer_bytes > external_buffer_bytes ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Writing out field %lu (datatype %lu) needs a total buffer "
		"size of %lu bytes, but only %lu bytes are available.",
			field_number, mx_datatype,
			(unsigned long) needed_buffer_bytes,
			(unsigned long) external_buffer_bytes );
	}
				
	if ( mx_datatype == MXFT_STRING ) {
		for ( i = 0; i < needed_field_bytes; i++ ) {
			field_buffer_ptr[i] = '*';
		}

		return MX_SUCCESSFUL_RESULT;
	}

	switch( mx_datatype ) {
	case MXFT_UCHAR:
		uint8_value = *((uint8_t *) value_ptr );

		snprintf( temp_buffer, sizeof(temp_buffer),
				"%02X", uint8_value );
		break;
	case MXFT_USHORT:
		uint16_value = *((uint16_t *) value_ptr );

		snprintf( temp_buffer, sizeof(temp_buffer),
				"%04X", uint16_value );
		break;
	case MXFT_ULONG:
		uint32_value = *((uint32_t *) value_ptr );

		snprintf( temp_buffer, sizeof(temp_buffer),
				"%08X", uint32_value );
		break;
	case MXFT_FLOAT:
		float_value = *((float *) value_ptr );

		/* FIXME, FIXME, FIXME: The cast to unsigned int is WRONG? */

		snprintf( temp_buffer, sizeof(temp_buffer),
				"%08X", (unsigned int) float_value );
		break;
	}

	for ( i = 0; i < needed_field_bytes; i++ ) {
		field_buffer_ptr[i] = temp_buffer[i];
	}

	return MX_SUCCESSFUL_RESULT;
}

