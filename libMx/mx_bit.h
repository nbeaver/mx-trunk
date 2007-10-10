/*
 * Name:    mx_bit.h
 *
 * Purpose: Bit manipulation utility functions
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000, 2003, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_BIT_H__
#define __MX_BIT_H__

#include "mx_stdint.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MX_DATAFMT_BIG_ENDIAN		0x1
#define MX_DATAFMT_LITTLE_ENDIAN	0x2

#define MX_DATAFMT_IEEE_FLOAT		0x100
#define MX_DATAFMT_VAX_D_FLOAT		0x200
#define MX_DATAFMT_VAX_G_FLOAT		0x400

/*---*/

MX_API unsigned long mx_native_byteorder( void );

MX_API unsigned long mx_native_float_format( void );

MX_API unsigned long mx_native_data_format( void );

MX_API unsigned long mx_native_program_model( void );

MX_API uint16_t mx_16bit_byteswap( uint16_t original_value );

MX_API uint32_t mx_32bit_byteswap( uint32_t original_value );

MX_API uint32_t mx_32bit_wordswap( uint32_t original_value );

/*---*/

MX_API mx_bool_type mx_is_power_of_two( unsigned long value );

#ifdef __cplusplus
}
#endif

#endif /* __MX_BIT_H__ */
