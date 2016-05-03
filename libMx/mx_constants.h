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

/*
 * HC is derived from fundamental constants as stated at the NIST web site
 * on November 29, 2002:
 *
 *     http://physics.nist.gov/cuu/Constants/index.html
 *
 */

#define MX_HC      			12398.4186    /* in eV-angstroms */

/* Used to compute XAFS electron wavenumbers from X-ray energy. */

#define MX_HBAR_SQUARED_OVER_2M_ELECTRON   3.8095     /* in eV-(angstroms**2) */

#endif /* __MX_CONSTANTS_H__ */

