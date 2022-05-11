/*
 * Name:    mx_bit.h
 *
 * Purpose: Bit manipulation utility functions
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000, 2003, 2006-2007, 2019, 2022 Illinois Institute of Technology
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

#define MX_DATAFMT_FLOAT_IEEE_BIG	0x1000
#define MX_DATAFMT_FLOAT_IEEE_LITTLE	0x2000
#define MX_DATAFMT_FLOAT_VAX_D		0x4000
#define MX_DATAFMT_FLOAT_VAX_G		0x8000

#define MX_DATAFMT_FLOAT_DEPRECATED	0xF00	/* Not used after MX 2.1.9 */

/*---*/

MX_API unsigned long mx_native_byteorder( void );

MX_API unsigned long mx_native_float_format( void );

MX_API unsigned long mx_native_data_format( void );

MX_API unsigned long mx_native_program_model( void );

MX_API uint16_t mx_uint16_byteswap( uint16_t original_value );

MX_API uint32_t mx_uint32_byteswap( uint32_t original_value );

MX_API uint64_t mx_uint64_byteswap( uint64_t original_value );

/*---*/

MX_API mx_bool_type mx_is_power_of_two( unsigned long value );

#ifdef __cplusplus
}
#endif

#endif /* __MX_BIT_H__ */
