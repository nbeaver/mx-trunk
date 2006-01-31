/*
 * Name:     mx_stdint.h
 *
 * Purpose:  This header file declares printf() and scanf() macros
 *           for processing the data types defined by the C99 standard
 *           in <inttypes.h>.  It also includes "mx_stdint.h".
 *
 * Author:   William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_INTTYPES_H__
#define __MX_INTTYPES_H__

#include "mx_stdint.h"

/* The non-standard build targets are listed first. */

/*=======================================================================*/

#if 0

/*=======================================================================*/
#else
   /* Most build targets should be able to use a vendor provided <inttypes.h>.*/

#  include <inttypes.h>

#endif

#endif /* __MX_INTTYPES_H__ */

