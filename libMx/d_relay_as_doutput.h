/*
 * Name:    d_relay_as_doutput.h
 *
 * Purpose: Header file for treating an MX relay as a digital output.
 *          The driver commands the relay to open or close depending on
 *          the value of the digital output record (0/1).
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_RELAY_AS_DOUTPUT_H__
#define __D_RELAY_AS_DOUTPUT_H__

#include "mx_digital_output.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *relay_record;
	mx_bool_type use_inverted_logic;
} MX_RELAY_AS_DOUTPUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_relay_as_doutput_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_relay_as_doutput_read( MX_DIGITAL_OUTPUT *ainput );
MX_API mx_status_type mxd_relay_as_doutput_write( MX_DIGITAL_OUTPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_relay_as_doutput_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_relay_as_doutput_digital_output_function_list;

extern long mxd_relay_as_doutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_relay_as_doutput_rfield_def_ptr;

#define MXD_RELAY_AS_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "relay_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RELAY_AS_DOUTPUT, relay_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "use_inverted_logic", MXFT_BOOL, NULL, 0, {0}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_RELAY_AS_DOUTPUT, use_inverted_logic), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_RELAY_AS_DOUTPUT_H__ */

