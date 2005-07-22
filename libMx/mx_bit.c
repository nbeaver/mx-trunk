/*
 * Name:    mx_bit.c
 *
 * Purpose: Bit manipulation utility functions.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_types.h"
#include "mx_bit.h"

MX_EXPORT unsigned long
mx_native_byteorder( void )
{
	union {
		mx_uint16_type word_value;
		mx_uint8_type byte_value[2];
	} u;

	u.word_value = 0x1234;

	if ( u.byte_value[0] == 0x34 ) {
		return MX_DATAFMT_LITTLE_ENDIAN;
	} else {
		return MX_DATAFMT_BIG_ENDIAN;
	}
}

/* mx_native_float_format: A cop-out for now. */

#if defined( OS_VMS )

MX_EXPORT unsigned long
mx_native_float_format( void )
{
	return MX_DATAFMT_VAX_FLOAT;
}

#else /* everything else */

MX_EXPORT unsigned long
mx_native_float_format( void )
{
	return MX_DATAFMT_IEEE_FLOAT;
}

#endif

MX_EXPORT unsigned long
mx_native_data_format( void )
{
	unsigned long format;

	format = mx_native_byteorder();

	format |= mx_native_float_format();

	return format;
}

MX_EXPORT mx_uint16_type
mx_16bit_byteswap( mx_uint16_type original_value )
{
	mx_uint8_type high_byte, low_byte;
	mx_uint16_type new_value;

	high_byte = (mx_uint8_type) ( ( original_value & 0xff00 ) >> 8 );
	low_byte = (mx_uint8_type) ( original_value & 0xff );

	new_value = high_byte;
	new_value |= ( low_byte << 8 );

	return new_value;
}

MX_EXPORT mx_uint32_type
mx_32bit_byteswap( mx_uint32_type original_value )
{
	mx_uint8_type byte0, byte1, byte2, byte3;
	mx_uint32_type new_value;

	byte3 = (mx_uint8_type) ( ( original_value & 0xff000000 ) >> 24 );
	byte2 = (mx_uint8_type) ( ( original_value & 0xff0000 ) >> 16 );
	byte1 = (mx_uint8_type) ( ( original_value & 0xff00 ) >> 8 );
	byte0 = (mx_uint8_type) ( original_value & 0xff );

	new_value = byte3;
	new_value |= ( byte2 << 8 );
	new_value |= ( byte1 << 16 );
	new_value |= ( byte0 << 24 );

	return new_value;
}

MX_EXPORT mx_uint32_type
mx_32bit_wordswap( mx_uint32_type original_value )
{
	mx_uint16_type high_word, low_word;
	mx_uint32_type new_value;

	high_word = (mx_uint16_type) ( ( original_value & 0xffff0000) >> 16 );
	low_word = (mx_uint16_type) ( original_value & 0xffff );

	new_value = high_word;
	new_value |= ( low_word << 16 );

	return new_value;
}

