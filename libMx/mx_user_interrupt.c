/*
 * Name:    mx_user_interrupt.c
 *
 * Purpose: Functions for handling user interrupts.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_key.h"

static int ( *mx_user_interrupt_function )( void )
			= mx_default_user_interrupt_function;

MX_EXPORT int
mx_user_requested_interrupt( void )
{
	const char fname[] = "mx_user_requested_interrupt()";

	int user_interrupt;

	if ( mx_user_interrupt_function != NULL ) {

		user_interrupt = ( *mx_user_interrupt_function )();

	} else {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
"The mx_user_interrupt_function pointer is NULL.  This shouldn't happen." );

		user_interrupt = MXF_USER_INT_ERROR;
	}
	return user_interrupt;
}

MX_EXPORT int
mx_default_user_interrupt_function( void )
{
	int c, user_interrupt;

	if ( mx_kbhit() ) {
		c = mx_getch();

		switch ( c ) {
		case 'p':
		case 'P':
			user_interrupt = MXF_USER_INT_PAUSE;
			break;
		default:
			user_interrupt = MXF_USER_INT_ABORT;
			break;
		}
	} else {
		user_interrupt = MXF_USER_INT_NONE;
	}

	return user_interrupt;
}

MX_EXPORT void
mx_set_user_interrupt_function( int (*user_interrupt_function)(void) )
{
	if ( user_interrupt_function == NULL ) {
		mx_user_interrupt_function
			= mx_default_user_interrupt_function;
	} else {
		mx_user_interrupt_function = user_interrupt_function;
	}
	return;
}

