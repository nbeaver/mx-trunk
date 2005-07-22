/*
 * Name:    mx_bit.h
 *
 * Purpose: Bit manipulation utility functions
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000, 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_BIT_H__
#define __MX_BIT_H__

#define MX_DATAFMT_BIG_ENDIAN			0x1
#define MX_DATAFMT_LITTLE_ENDIAN		0x2

#define MX_DATAFMT_IEEE_FLOAT			0x100
#define MX_DATAFMT_VAX_FLOAT			0x200

MX_API unsigned long mx_native_byteorder( void );

MX_API unsigned long mx_native_float_format( void );

MX_API unsigned long mx_native_data_format( void );

MX_API mx_uint16_type mx_16bit_byteswap( mx_uint16_type original_value );

MX_API mx_uint32_type mx_32bit_byteswap( mx_uint32_type original_value );

MX_API mx_uint32_type mx_32bit_wordswap( mx_uint32_type original_value );

#endif /* __MX_BIT_H__ */
