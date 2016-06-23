/*
 * Name:    mx_dictionary.c
 *
 * Purpose: Support for an MX record superclass that provides support
 *          for key/value pairs as used in dictionary, associative array,
 *          map, symbol table, etc. data structures.  The dictionary is
 *          referred to by its MX record name, while the keys are MX
 *          record fields in the record.
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

