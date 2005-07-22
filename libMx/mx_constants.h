/* 
 * Name:    mx_constants.h
 *
 * Purpose: Physical and mathematical constants used by MX programs.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2002, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_CONSTANTS_H__
#define __MX_CONSTANTS_H__

/* Mathematical constants are given rounded to 16 digits accuracy. */

#define MX_PI    		3.141592653589793

#define MX_RADIANS_PER_DEGREE	0.01745329251994330

/* For Irix, ULONG_MAX can be too large to fit into a 32 bit integer. */

#if defined( OS_IRIX ) && defined(__GNUC__)
#  define MX_ULONG_MAX		(unsigned long) LONG_MAX
#  define MX_LONG_MAX		(long) LONG_MAX
#else
#  define MX_ULONG_MAX		ULONG_MAX
#  define MX_LONG_MAX		LONG_MAX
#endif

/*
 * HC is derived from fundamental constants as stated at the NIST web site
 * on November 29, 2002:
 *
 *     http://physics.nist.gov/cuu/Constants/index.html
 *
 */

#define MX_HC      		12398.4186	/* in eV-angstroms */

#endif /* __MX_CONSTANTS_H__ */

