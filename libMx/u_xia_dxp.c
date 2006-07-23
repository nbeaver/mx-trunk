/*
 * Name:    u_xia_dxp.c
 *
 * Purpose: MX utility functions used by the 'xia_handel' and 'xia_xerxes'
 *          drivers for the X-Ray Instrumentation Associates DXP series
 *          of multichannel analyzers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_XIA_XERXES

#include <xerxes_structures.h>
#include <xerxes_errors.h>
#include <xerxes.h>

#include "mx_util.h"
#include "u_xia_dxp.h"

MX_EXPORT mx_status_type
mxu_xia_dxp_replace_output_functions( void )
{
	static const char fname[] = "mxu_xia_dxp_replace_output_functions()";

	MX_DEBUG( 2,("%s is not yet implemented.\n", fname));

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_XIA_XERXES */
