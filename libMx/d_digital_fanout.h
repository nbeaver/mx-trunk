/*
 * Name:    d_digital_fanout.h
 *
 * Purpose: Header file for forwarding the digital output value written
 *          to this record to a number of other record fields.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_DIGITAL_FANOUT_H__
#define __D_DIGITAL_FANOUT_H__

#include "mx_digital_output.h"

typedef struct {
	MX_RECORD *record;

	long num_fields;
	char **field_name_array;

	MX_RECORD_FIELD **field_array;
	void **field_value_array;
} MX_DIGITAL_FANOUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_digital_fanout_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_digital_fanout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_digital_fanout_open( MX_RECORD *record );

MX_API mx_status_type mxd_digital_fanout_write( MX_DIGITAL_OUTPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_digital_fanout_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_digital_fanout_digital_output_function_list;

extern long mxd_digital_fanout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_digital_fanout_rfield_def_ptr;

#define MXD_DIGITAL_FANOUT_STANDARD_FIELDS \
  {-1, -1, "num_fields", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DIGITAL_FANOUT, num_fields), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY)}, \
  \
  {-1, -1, "field_name_array", MXFT_STRING, NULL, \
		2, {MXU_VARARGS_LENGTH,(MXU_RECORD_FIELD_NAME_LENGTH+1)}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DIGITAL_FANOUT, field_name_array), \
	{sizeof(char), sizeof(char *)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_READ_ONLY | MXFF_VARARGS)}

#endif /* __D_DIGITAL_FANOUT_H__ */

