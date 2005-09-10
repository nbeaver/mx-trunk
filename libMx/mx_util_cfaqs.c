/*
 * Name:    mx_util_cfaqs.c
 *
 * Purpose: A couple of utility functions derived from the book
 *
 *              "C Programming FAQs" by Steve Summit
 *              Addison Wesley, 1996.
 *              ISBN: 0-201-84519-9
 *              LC:   QA76.73.C15S86 1996
 *
 * Author:  William Lavender and others described below.
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * NOTE: The source code for the "C Programming FAQs" book may be found
 *       as of Sept. 2005 in the FTP directory
 *
 *           ftp://ftp.aw.com/cseng/authors/summit/cfaq/
 *
 *       The only thing resembling a license that can be found in the
 *       cfaq.tar.Z file found there is in the README file which ends
 *       with the text:
 *
 *         These sources are Copyright 1995 by Steve Summit, but may be used
 *         or modified as desired.  (We'd appreciate an acknowledgement.)
 *         They have been tested with some care, but are not guaranteed for
 *         any particular purpose.  The author and publisher do not offer
 *         any warranties or representations, nor do they accept any
 *         liabilities with respect to these sources.
 */

#include <stdio.h>
#include "mx_util.h"

/* mx_difference() computes a relative difference function.
 *
 * Derived from pages 250-251 in "C Programming FAQs".  The book says that
 * the function described there was derived in turn from Knuth, Volume 2,
 * Section 4.2.2, pages 217-218.
 */

MX_EXPORT double
mx_difference( double value1, double value2 )
{
	double abs1, abs2, maxabs, result;

	if ( value1 >= 0.0 ) {
		abs1 = value1;
	} else {
		abs1 = - value1;
	}

	if ( value2 >= 0.0 ) {
		abs2 = value2;
	} else {
		abs2 = - value2;
	}

	if ( abs1 >= abs2 ) {
		maxabs = abs1;
	} else {
		maxabs = abs2;
	}
	
	if ( maxabs == 0.0 ) {
		result = 0.0;
	} else {
		result = ( value1 - value2 ) / maxabs;

		if ( result < 0.0 ) {
			result = - result;
		}
	}

	return result;
}

/* Another function from "C Programming FAQs" is the following simple
 * wildcard matcher from page 228 which is attributed to Arjan Kenter.
 *
 * mx_match() does what I need without requiring the inclusion
 * of complicated regular expression matching functions.
 *
 * As an example, the call mx_match( "a*b.c", "aplomb.c" ) would return 1.
 */

MX_EXPORT int
mx_match( char *pattern, char *string )
{
	switch( *pattern ) {
	case '\0':	return ! *string;

	case '*':	return mx_match( pattern+1, string ) ||
				( *string && mx_match( pattern, string+1 ) );

	case '?':	return *string && mx_match( pattern+1, string+1 );

	default:	return *pattern == *string &&
					mx_match( pattern+1, string+1 );
	}
}

