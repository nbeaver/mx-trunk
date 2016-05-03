/* 
 * Name:    mx_constants.h
 *
 * Purpose: Physical and mathematical constants used by MX programs.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2002, 2005, 2007, 2016 Illinois Institute of Technology
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

/*----------------------------------------------------------------------------*/

/* Physical constants frequently used in MX. */

#define MX_HC                            12398.4197   /* in eV-Angstroms */

#define MX_HBAR_SQUARED_OVER_2M_ELECTRON  3.8099821   /* in eV-(Angstroms**2) */

/* The above values were computed using the following values from the NIST
 * Standard Reference Database 121 as of May 3, 2016.  These values were
 * acquired using the web interface at http://physics.nist.gov/cuu/Constants/.
 * 
 *   c             = 299 792 458                   meters/second
 *   electron mass = 9.109 383 56(11) * 10^(-31)   kilograms
 *   electron volt = 1.602 176 6208(98) * 10^(-19) Joules
 *   h             = 6.626 070 040(81) * 10^(-34)  Joule-seconds
 *   hbar          = 1.054 571 800(13) * 10^(-34)  Joule-seconds
 */

/*----------------------------------------------------------------------------*/
#if 0
/* Old values of the constants used before May 3, 2016. */
#define MX_HC                            12398.4186
#define MX_HBAR_SQUARED_OVER_2M_ELECTRON   3.8095
#endif

#endif /* __MX_CONSTANTS_H__ */

