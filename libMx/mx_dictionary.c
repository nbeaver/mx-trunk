/*
 * Name:    mx_dictionary.c
 *
 * Purpose: MX dictionaries are a key-value database where the keys are
 *          found in an array of string names.  The values are pointed
 *          to by an array of void pointers, where the actual values are
 *          MX arrays which are each allocated by the mx_allocate_array()
 *          functions.
 *
 * Note:    The current implementation of MX dictionaries has not been
 *          optimized for speed.  It anticipates that most key-value
 *          pairs will be initialized at program startup time.
 *
 * Warning: The values (including 1-dimensional values) _must_ have been
 *          allocated by mx_allocate_array().  If you put an array that
 *          you allocated directly using malloc() or calloc(), then at
 *          best you will get an error message.  At worst, it can crash
 *          your program, since MX dictionaries make use of the metadata
 *          created by mx_allocate_array() that describes the datatype and
 *          dimensions of the array.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_dictionary.h"

