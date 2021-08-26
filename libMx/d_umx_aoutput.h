/*
 * Name:    d_umx_aoutput.h
 *
 * Purpose: Header file for UMX analog output devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_UMX_AOUTPUT_H__
#define __D_UMX_AOUTPUT_H__

#include "mx_analog_output.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *umx_record;
	char aoutput_name[MXU_RECORD_NAME_LENGTH+1];
	double frequency;
} MX_UMX_AOUTPUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_umx_aoutput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_umx_aoutput_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_umx_aoutput_write( MX_ANALOG_OUTPUT *aoutput );

extern MX_RECORD_FUNCTION_LIST mxd_umx_aoutput_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
			mxd_umx_aoutput_analog_output_function_list;

extern long mxd_umx_aoutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_umx_aoutput_rfield_def_ptr;

#define MXLV_UMX_AOUTPUT_FREQUENCY			103001

#define MXD_UMX_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "umx_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT,  offsetof(MX_UMX_AOUTPUT, umx_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "aoutput_name", MXFT_STRING, NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_UMX_AOUTPUT, aoutput_name),\
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_UMX_AOUTPUT_FREQUENCY, -1, "frequency", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_UMX_AOUTPUT, frequency), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_UMX_AOUTPUT_H__ */

