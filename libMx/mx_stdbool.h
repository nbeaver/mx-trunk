/*
 * Name:     mx_stdbool.h
 *
 * Purpose:  This header file declares the 'bool' type as well as the
 *           'false' and 'true' values specified by the <stdbool.h>
 *           header file of the C99 standard.
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

#ifndef __MX_STDBOOL_H__
#define __MX_STDBOOL_H__

/* The non-standard build targets are listed first. */

/*=======================================================================*/

#if 0

typedef int    bool;

#define true   1
#define false  0

#define __bool_true_false_are_defined  1

/*=======================================================================*/
#else
   /* Most build targets should be able to use a vendor provided <stdbool.h>. */

#  include <stdbool.h>

#endif

/* The following are MX-specific versions of the boolean type values. */

#ifndef TRUE
#define TRUE	true
#define FALSE	false
#endif

#endif /* __MX_STDBOOL_H__ */

