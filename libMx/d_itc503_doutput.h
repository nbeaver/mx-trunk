/*
 * Name:    d_itc503_doutput.h
 *
 * Purpose: Header file for setting control parameters in Oxford Instruments
 *          ITC503 and Cryojet temperature controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2008, 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ITC503_DOUTPUT_H__
#define __D_ITC503_DOUTPUT_H__

#include "mx_digital_output.h"

typedef struct {
	MX_RECORD *controller_record;
	char parameter_type;
} MX_ITC503_DOUTPUT;

/* The value of 'parameter_type' is the letter that starts the ITC503 command
 * that will be sent.  The currently supported values are:
 *
 * 'A' - Set auto/manual for heater and gas
 * 'C' - Set local/remote/lock status
 *
 */

/* Define all of the interface functions. */

MX_API mx_status_type mxd_itc503_doutput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_itc503_doutput_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_itc503_doutput_read( MX_DIGITAL_OUTPUT *ainput );
MX_API mx_status_type mxd_itc503_doutput_write( MX_DIGITAL_OUTPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_itc503_doutput_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_itc503_doutput_digital_output_function_list;

extern long mxd_itc503_doutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_itc503_doutput_rfield_def_ptr;

#define MXD_ITC503_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "controller_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ITC503_DOUTPUT, controller_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "parameter_type", MXFT_CHAR, NULL, 0, {0}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_ITC503_DOUTPUT, parameter_type), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_ITC503_DOUTPUT_H__ */

