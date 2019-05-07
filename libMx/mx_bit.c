/*
 * Name:    mx_bit.c
 *
 * Purpose: Bit manipulation utility functions.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003, 2006-2007, 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#if defined(__GLIBC__)
#include <endian.h>
#endif

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

/*-------- mx_native_float_format. --------*/

#if defined( OS_VMS )

MX_EXPORT unsigned long
mx_native_float_format( void )
{
#if defined( __ia64 )
	return MX_DATAFMT_FLOAT_IEEE_LITTLE;	/* FIXME: Check this! */
#elif defined( __alpha )
	return MX_DATAFMT_FLOAT_VAX_G;
#elif defined( __vax )
	return MX_DATAFMT_FLOAT_VAX_D;
#else
#error Unrecognized VMS platform for mx_native_float_format().
#endif
}

/*-----------------------------------------*/

#elif defined( __FLOAT_WORD_ORDER__ )
    /* __FLOAT_WORD_ORDER__ appears in GNU CPP 5.5 and above. */

#  if ( __FLOAT_WORD_ORDER__ == __ORDER_LITTLE_ENDIAN__ )

MX_EXPORT unsigned long
mx_native_float_format( void )
{
	return MX_DATAFMT_FLOAT_IEEE_LITTLE;
}

#  elif ( __FLOAT_WORD_ORDER__ == __ORDER_BIG_ENDIAN__ )

MX_EXPORT unsigned long
mx_native_float_format( void )
{
	return MX_DATAFMT_FLOAT_IEEE_BIG;
}

#  else
#    error __FLOAT_WORD_ORDER__ is defined, but with an unrecognized value.
#  endif

/*-----------------------------------------*/

#elif 1
  /* https://stackoverflow.com/questions/35763790/endianness-for-floating-point
   *
   * This is for IEEE-754 floating point.  It relies on the knowledge that
   * a negative power of 2 (like -1.0) has a non-zero most significant byte
   * and a zero least significant byte.
   *
   * This method assumes that anything that is _not_ little-endian, must be
   * big-endian, which we know is not true.  For example, some ARM platforms
   * have mixed-endian floating point that looks like this: 45670123.
   */

MX_EXPORT unsigned long
mx_native_float_format( void )
{
	float float_number = 1.0;

	uint8_t *byte_pointer = (uint8_t *) &float_number;

	if ( byte_pointer[0] == 0 ) {
		return MX_DATAFMT_FLOAT_IEEE_LITTLE;
	} else {
		return MX_DATAFMT_FLOAT_IEEE_BIG;
	}
}

/*-----------------------------------------*/

#else
#error mx_native_float_format() not yet implemented for this platform.
#endif

/*--------*/

MX_EXPORT unsigned long
mx_native_data_format( void )
{
	unsigned long format;

	format = mx_native_byteorder();

	format |= mx_native_float_format();

	return format;
}

#if defined(OS_HPUX)
	/* HP/UX generates warnings for each of the comparisons of sizeof()
	 * with a number below such as 'sizeof(int) == 4'.  Therefore, we
	 * put in special case code just for HP/UX.
	 */

MX_EXPORT unsigned long
mx_native_program_model( void )
{
#if defined(__LP64__)
	return MX_PROGRAM_MODEL_LP64;
#else
	return MX_PROGRAM_MODEL_ILP32;
#endif
}

#else
	/* For everone else. */

MX_EXPORT unsigned long
mx_native_program_model( void )
{
	unsigned long program_model;

	if ( sizeof(int) == 2 ) {
		if (( sizeof(long) == 4 ) && ( sizeof(void *) == 4 )) {
			/* Example: Win16 (ick!) */

			program_model = MX_PROGRAM_MODEL_LP32;
		} else {
			program_model = MX_PROGRAM_MODEL_UNKNOWN;
		}
	} else
	if ( sizeof(int) == 4 ) {
		if ( sizeof(long) == 4 ) {
			if ( sizeof(void *) == 4 ) {
				/* Example: Most 32-bit Linux/Unix and Win32 */

				program_model = MX_PROGRAM_MODEL_ILP32;
			} else
			if ( sizeof(void *) == 8 ) {
#if defined(OS_WIN32)
				/* Example: Win64 */

				program_model = MX_PROGRAM_MODEL_LLP64;
#else
				program_model = MX_PROGRAM_MODEL_UNKNOWN;
#endif
			} else {
				program_model = MX_PROGRAM_MODEL_UNKNOWN;
			}
		} else
		if ( sizeof(long) == 8 ) {
			if ( sizeof(void *) == 8 ) {
				/* Example: Most 64-bit Linux/Unix */

				program_model = MX_PROGRAM_MODEL_LP64;
			} else {
				program_model = MX_PROGRAM_MODEL_UNKNOWN;
			}
		} else {
			program_model = MX_PROGRAM_MODEL_UNKNOWN;
		}
	} else
	if ( sizeof(int) == 8 ) {
		if (( sizeof(long) == 8 ) && ( sizeof(void *) == 8 )) {
			/* Example: 64-bit Crays */

			program_model = MX_PROGRAM_MODEL_ILP64;
		} else {
			program_model = MX_PROGRAM_MODEL_UNKNOWN;
		}
	} else {
		program_model = MX_PROGRAM_MODEL_UNKNOWN;
	}

	return program_model;
}

#endif

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

/*------------------------------------------------------------------------*/

MX_EXPORT mx_bool_type
mx_is_power_of_two( unsigned long value )
{
	/* Zero is not a power of 2. */

	if ( value == 0 ) {
		return FALSE;
	}

	/* Any value that is a power of 2 has only one bit set in its
	 * binary representation.  If we subtract 1 from that value,
	 * the resulting number will not have any bits in common with
	 * the original value.  This only works for twos complement
	 * binary representations.
	 */

	if ( (value & (value - 1)) == 0 ) {
		return TRUE;
	} else {
		return FALSE;
	}
}

