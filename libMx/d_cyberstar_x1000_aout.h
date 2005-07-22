/*
 * Name:    d_cyberstar_x1000_aout.h
 *
 * Purpose: Header file for an MX analog output driver to control the
 *          Oxford Danfysik Cyberstar X1000 high voltage and delay.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_CYBERSTAR_X1000_AOUT_H__
#define __D_CYBERSTAR_X1000_AOUT_H__

#include "mx_analog_output.h"

/* ===== Cyberstar X1000 analog output data structures ===== */

#define MXT_CYBERSTAR_X1000_HIGH_VOLTAGE	1
#define MXT_CYBERSTAR_X1000_DELAY		2

typedef struct {
	MX_RECORD *record;

	MX_RECORD *cyberstar_x1000_record;
	int output_type;
} MX_CYBERSTAR_X1000_AOUTPUT;

#define MXD_CYBERSTAR_X1000_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "cyberstar_x1000_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CYBERSTAR_X1000_AOUTPUT, \
						cyberstar_x1000_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "output_type", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CYBERSTAR_X1000_AOUTPUT, \
						output_type), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_cyberstar_x1000_aout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_cyberstar_x1000_aout_read(MX_ANALOG_OUTPUT *doutput);
MX_API mx_status_type mxd_cyberstar_x1000_aout_write(MX_ANALOG_OUTPUT *doutput);

extern MX_RECORD_FUNCTION_LIST mxd_cyberstar_x1000_aout_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
			mxd_cyberstar_x1000_aout_analog_output_function_list;

extern long mxd_cyberstar_x1000_aout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_cyberstar_x1000_aout_rfield_def_ptr;

#endif /* __D_CYBERSTAR_X1000_AOUT_H__ */

