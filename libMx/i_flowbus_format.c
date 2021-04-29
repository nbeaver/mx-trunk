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

MX_EXPORT char *
mxi_flowbus_format_string( char *external_buffer,
			size_t external_buffer_size,
			unsigned long mx_datatype,
			void *value_ptr )
{
	static const char fname[] = "mxi_flowbus_format_string()";

	uint8_t uint8_value;
	uint16_t uint16_value;
	uint32_t uint32_value;
	float float_value;
	size_t i;

	char temp_buffer[500];

	if ( external_buffer == (char *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
			"The external_buffer pointer passed was NULL." );
		return NULL;
	}
	if ( value_ptr == (void *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
			"The value_ptr pointer passed was NULL." );
		return NULL;
	}

	memset( temp_buffer, 0, sizeof(temp_buffer) );

	switch( mx_datatype ) {
	case MXFT_UCHAR:
		uint8_value = *((uint8_t *) value_ptr );

		snprintf( temp_buffer, sizeof(temp_buffer),
				"%02x", uint8_value );
		break;
	case MXFT_USHORT:
		uint16_value = *((uint16_t *) value_ptr );

		snprintf( temp_buffer, sizeof(temp_buffer),
				"%04x", uint16_value );
		break;
	case MXFT_ULONG:
		uint32_value = *((uint32_t *) value_ptr );

		snprintf( temp_buffer, sizeof(temp_buffer),
				"%08x", uint32_value );
		break;
	case MXFT_FLOAT:
		float_value = *((float *) value_ptr );

		/* FIXME, FIXME, FIXME: The cast to unsigned int is WRONG. */

		snprintf( temp_buffer, sizeof(temp_buffer),
				"%08x", (unsigned int) float_value );
		break;
	}

	/* Copy the formatted string to the external buffer.
	 *
	 * Note that we do _NOT_ want to copy a NUL byte onto the end
	 * of the copied string.  This allows us to overwrite fields
	 * in the middle of the string buffer without destroying the
	 * first byte of the next field.  So this is like doing strncpy()
	 * without actually invoking the poisoned function strncpy().
	 *
	 * The Invisible Pink Unicorn told me to do this.  He said that it
	 * was suggested to him by the Flying Spaghetti Monster.
	 */

	for ( i = 0; i < external_buffer_size; i++ ) {
		external_buffer[i] = temp_buffer[i];
	}

	/* See ma?  No NUL termination! */

	return external_buffer;
}

