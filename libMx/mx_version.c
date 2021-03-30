/*
 * Name:    mx_version.c
 *
 * Purpose: Functions to display the version of MX being used.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>

#if defined(__GNUC__)
#  define __USE_XOPEN		/* For strptime() */
#endif

#include "mx_util.h"
#include "mx_time.h"
#include "mx_version.h"

#define MX_DATE "March 30, 2021"

#include "mx_private_revision.h"

#if !defined(MX_REVISION_STRING)
#  define MX_REVISION_STRING ""
#endif

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
mx_get_version_full_string( void )
{
	snprintf( buffer, sizeof(buffer), "%d.%d.%d (%s)",
	    MX_MAJOR_VERSION, MX_MINOR_VERSION, MX_UPDATE_VERSION, MX_DATE );

	return &buffer[0];
}

MX_EXPORT char *
mx_get_version_date_string( void )
{
	strlcpy( buffer, MX_DATE, sizeof(buffer) );

	return &buffer[0];
}

MX_EXPORT struct tm
mx_get_version_date_tm( void )
{
	struct tm tm;
	char *ptr;

	memset( &tm, 0, sizeof(tm) );

	ptr = strptime( MX_DATE, "%B %d, %Y", &tm );

	if ( ptr == NULL ) {
		memset( &tm, 0, sizeof(tm) );
		return tm;
	}

#if 0
	MX_DEBUG(-2,("mx_get_version_date_tm():"));
	MX_DEBUG(-2,("  tm_sec   = %d", tm.tm_sec));
	MX_DEBUG(-2,("  tm_min   = %d", tm.tm_min));
	MX_DEBUG(-2,("  tm_hour  = %d", tm.tm_hour));
	MX_DEBUG(-2,("  tm_mday  = %d", tm.tm_mday));
	MX_DEBUG(-2,("  tm_mon   = %d", tm.tm_mon));
	MX_DEBUG(-2,("  tm_year  = %d", tm.tm_year));
	MX_DEBUG(-2,("  tm_wday  = %d", tm.tm_wday));
	MX_DEBUG(-2,("  tm_yday  = %d", tm.tm_yday));
	MX_DEBUG(-2,("  tm_isdst = %d", tm.tm_isdst));
#endif

	return tm;
}

MX_EXPORT uint64_t
mx_get_version_date_time( void )
{
	struct tm tm;
	time_t result;

	tm = mx_get_version_date_tm();

	result = mktime( &tm );

#if 0
	MX_DEBUG(-2,("mx_get_version_date_time(): time = %lu", result));
#endif

	return result;
}

MX_EXPORT int
mx_get_revision_number( void )
{
	int num_items, revision_number;

	num_items = sscanf( MX_REVISION_STRING, "SVN %d", &revision_number );

	if ( num_items == 0 ) {
		revision_number = 0;
	}

	return revision_number;
}

MX_EXPORT char *
mx_get_revision_string( void )
{
	static char revision[] = MX_REVISION_STRING;

	return revision;
}

#if defined(OS_VMS)

MX_EXPORT const char *
mx_get_branch_label( void )
{
	static const char label[] = "none";

	return label;
}

#else

MX_EXPORT const char *
mx_get_branch_label( void )
{
	static const char label[] = MX_BRANCH_LABEL;

	return label;
}

#endif

