/*
 * Name:    mx_version.c
 *
 * Purpose: Functions to display the version of MX being used.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>

#include "mx_util.h"
#include "mx_version.h"

#define MX_DATE "November 23, 2009"

static char buffer[60];

MX_EXPORT int
mx_get_major_version( void )
{
	return MX_MAJOR_VERSION;
}

MX_EXPORT int
mx_get_minor_version( void )
{
	return MX_MINOR_VERSION;
}

MX_EXPORT int
mx_get_update_version( void )
{
	return MX_UPDATE_VERSION;
}

MX_EXPORT char *
mx_get_version_date( void )
{
	strlcpy( buffer, MX_DATE, sizeof(buffer) );

	return &buffer[0];
}

MX_EXPORT char *
mx_get_version_string( void )
{
	snprintf( buffer, sizeof(buffer), "%d.%d.%d (%s)",
	    MX_MAJOR_VERSION, MX_MINOR_VERSION, MX_UPDATE_VERSION, MX_DATE );

	return &buffer[0];
}

