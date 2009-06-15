/*
 * Name:    mx_multi.c
 *
 * Purpose: Functions for cross-platform network support.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>

#include "mxconfig.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_multi.h"

#if HAVE_EPICS
#  include "mx_epics.h"
#endif

MX_EXPORT void
mx_multi_set_debug_flag( MX_RECORD *record_list,
			mx_bool_type flag )
{
	static const char fname[] = "mx_multi_set_debug_flag()";

	MX_LIST_HEAD *list_head;

	/* First handle debug settings that do not require an MX record list. */
#if HAVE_EPICS
	mx_epics_set_debug_flag( flag );
#endif

	if ( record_list == (MX_RECORD *) NULL )
		return;

	/* Set the network debug flag for the MX database. */

	list_head = mx_get_record_list_head_struct( record_list );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for record list %p is NULL.",
			record_list );
		return;
	}

	if ( flag ) {
		list_head->network_debug = TRUE;
	} else {
		list_head->network_debug = FALSE;
	}

	return;
}

