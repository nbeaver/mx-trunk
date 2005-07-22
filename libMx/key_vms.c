/*
 * Name:    key_vms.c
 *
 * Purpose: Emulation of DOS keyboard handling functions under VMS.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_key.h"

#ifdef OS_VMS

#include <errno.h>
#include <ssdef.h>
#include <iodef.h>
#include <descrip.h>
#include <starlet.h>
#include <ttdef.h>
#include <tt2def.h>

static int tt_channel = -1;

MX_EXPORT int
mx_vms_getch( void )
{
	static const char fname[] = "mx_vms_getch()";

	char c;
	short iosb[4];
	int vms_status;
	$DESCRIPTOR(tt_descriptor,"TT:");

	if ( tt_channel == -1 ) {
		vms_status = sys$assign(
				&tt_descriptor,
				&tt_channel,
				0,
				0,
				0 );

		if ( vms_status != SS$_NORMAL ) {
			(void) mx_error( MXE_FUNCTION_FAILED, fname,
    "An attempt to setup single character input from the terminal failed.  "
    "VMS error number %d, error message = '%s'",
			vms_status, strerror( EVMSERR, vms_status ) );

			return 0;
		}
	}

	vms_status = sys$qiow(
			0,
			tt_channel,
			IO$_READVBLK,
			iosb,
			0,
			0,
			&c,
			1,
			0,
			0,
			0,
			0 );

	if ( vms_status != SS$_NORMAL ) {
		(void) mx_error( MXE_FUNCTION_FAILED, fname,
		"An attempt to read a character from the terminal failed.  "
		"VMS error number %d, error message = '%s'",
			vms_status, strerror( EVMSERR, vms_status ) );

		return 0;
	}

	return (int) c;
}

MX_EXPORT int
mx_vms_kbhit( void )
{
	static const char fname[] = "mx_vms_kbhit()";

	struct {
		unsigned short typeahead_count;
		char first_character;
		char reserved[5];
	} typeahead_struct;

	short iosb[4];
	int vms_status;
	$DESCRIPTOR(tt_descriptor,"TT:");

	if ( tt_channel == -1 ) {
		vms_status = sys$assign(
				&tt_descriptor,
				&tt_channel,
				0,
				0,
				0 );

		if ( vms_status != SS$_NORMAL ) {
			(void) mx_error( MXE_FUNCTION_FAILED, fname,
    "An attempt to setup single character input from the terminal failed.  "
    "VMS error number %d, error message = '%s'",
			vms_status, strerror( EVMSERR, vms_status ) );

			return 0;
		}
	}

	vms_status = sys$qiow(
			0,
			tt_channel,
			IO$_SENSEMODE | IO$M_TYPEAHDCNT,
			iosb,
			0,
			0,
			&typeahead_struct,
			1,
			0,
			0,
			0,
			0 );

	if ( vms_status != SS$_NORMAL ) {
		(void) mx_error( MXE_FUNCTION_FAILED, fname,
"An attempt to see if characters were available from the terminal failed.  "
"VMS error number %d, error message = '%s'",
			vms_status, strerror( EVMSERR, vms_status ) );

		return 0;
	}

	if ( typeahead_struct.typeahead_count > 0 ) {
		return TRUE;
	} else {
		return FALSE;
	}
}

#endif /* OS_VMS */
