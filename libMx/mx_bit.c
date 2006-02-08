/*
 * Name:    mx_bit.c
 *
 * Purpose: Bit manipulation utility functions.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_bit.h"

MX_EXPORT unsigned long
mx_native_byteorder( void )
{
	union {
		uint16_t word_value;
		uint8_t byte_value[2];
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

MX_EXPORT unsigned long
mx_native_program_model( void )
{
	return MX_PROGRAM_MODEL;
}

MX_EXPORT uint16_t
mx_16bit_byteswap( uint16_t original_value )
{
	uint8_t high_byte, low_byte;
	uint16_t new_value;

	high_byte = (uint8_t) ( ( original_value & 0xff00 ) >> 8 );
	low_byte = (uint8_t) ( original_value & 0xff );

	new_value = high_byte;
	new_value |= ( low_byte << 8 );

	return new_value;
}

MX_EXPORT uint32_t
mx_32bit_byteswap( uint32_t original_value )
{
	uint8_t byte0, byte1, byte2, byte3;
	uint32_t new_value;

	byte3 = (uint8_t) ( ( original_value & 0xff000000 ) >> 24 );
	byte2 = (uint8_t) ( ( original_value & 0xff0000 ) >> 16 );
	byte1 = (uint8_t) ( ( original_value & 0xff00 ) >> 8 );
	byte0 = (uint8_t) ( original_value & 0xff );

	new_value = byte3;
	new_value |= ( byte2 << 8 );
	new_value |= ( byte1 << 16 );
	new_value |= ( byte0 << 24 );

	return new_value;
}

MX_EXPORT uint32_t
mx_32bit_wordswap( uint32_t original_value )
{
	uint16_t high_word, low_word;
	uint32_t new_value;

	high_word = (uint16_t) ( ( original_value & 0xffff0000 ) >> 16 );
	low_word = (uint16_t) ( original_value & 0xffff );

	new_value = high_word;
	new_value |= ( low_word << 16 );

	return new_value;
}

