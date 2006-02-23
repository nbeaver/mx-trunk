/*
 * Name:    d_aps_adcmod2_ainput.h
 *
 * Purpose: Header file for reading the signal from the ADCMOD2 electrometer
 *          system designed by Steve Ross of the Advanced Photon Source.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_APS_ADCMOD2_AINPUT_H__
#define __D_APS_ADCMOD2_AINPUT_H__

#include "mx_analog_input.h"

typedef struct {
	MX_RECORD *aps_adcmod2_record;

	long ainput_number;
} MX_APS_ADCMOD2_AINPUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_aps_adcmod2_ainput_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_aps_adcmod2_ainput_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_aps_adcmod2_ainput_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
			mxd_aps_adcmod2_ainput_analog_input_function_list;

extern long mxd_aps_adcmod2_ainput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_aps_adcmod2_ainput_rfield_def_ptr;

#define MXD_APS_ADCMOD2_AINPUT_STANDARD_FIELDS \
  {-1, -1, "aps_adcmod2_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_ADCMOD2_AINPUT, aps_adcmod2_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "ainput_number", MXFT_LONG, NULL, 0, {0}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_APS_ADCMOD2_AINPUT, ainput_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_APS_ADCMOD2_AINPUT_H__ */

