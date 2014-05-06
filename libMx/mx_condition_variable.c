/*
 * Name:    mx_condition_variable.c
 *
 * Purpose: MX condition variable functions.
 *
 *          These functions are modeled on Posix condition variables.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_osdef.h"
#include "mx_condition_variable.h"

/************************ Microsoft Win32 ***********************/

#if defined(OS_WIN32)

#elif defined(OS_UNIX)

#else

#error MX condition variables are not yet implemented on this platform.

#endif
