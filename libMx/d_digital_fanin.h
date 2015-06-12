/*
 * Name:    d_digital_fanin.h
 *
 * Purpose: Header file for reading values from other MX record fields and
 *          performing a logical operation on the values read (and, or, xor).
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_DIGITAL_FANIN_H__
#define __D_DIGITAL_FANIN_H__

#define MXU_DIGITAL_FANIN_OPERATION_NAME_LENGTH 	5

#include "mx_digital_input.h"

#define MXT_DFIN_AND		1
#define MXT_DFIN_OR		2
#define MXT_DFIN_XOR		3

typedef struct {
	MX_RECORD *record;

	unsigned long operation_type;
	char operation_name[ MXU_DIGITAL_FANIN_OPERATION_NAME_LENGTH + 1 ];

	long num_fields;
	char **field_name_array;

	MX_RECORD_FIELD **field_array;
	void **field_value_array;
} MX_DIGITAL_FANIN;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_digital_fanin_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_digital_fanin_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_digital_fanin_open( MX_RECORD *record );

MX_API mx_status_type mxd_digital_fanin_read( MX_DIGITAL_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_digital_fanin_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
			mxd_digital_fanin_digital_input_function_list;

extern long mxd_digital_fanin_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_digital_fanin_rfield_def_ptr;

#define MXD_DIGITAL_FANIN_STANDARD_FIELDS \
  {-1, -1, "operation_name", MXFT_STRING, NULL, \
			1, {MXU_DIGITAL_FANIN_OPERATION_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DIGITAL_FANIN, operation_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY)}, \
  {-1, -1, "num_fields", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DIGITAL_FANIN, num_fields), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY)}, \
  \
  {-1, -1, "field_name_array", MXFT_STRING, NULL, \
		2, {MXU_VARARGS_LENGTH,(MXU_RECORD_FIELD_NAME_LENGTH+1)}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DIGITAL_FANIN, field_name_array), \
	{sizeof(char), sizeof(char *)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_READ_ONLY | MXFF_VARARGS)}

#endif /* __D_DIGITAL_FANIN_H__ */

