/*
 * Name:    d_keithley2600_aoutput.h
 *
 * Purpose: Header file for analog outputs of Keithley 2600 SourceMeters.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_KEITHLEY2600_AOUTPUT_H__
#define __D_KEITHLEY2600_AOUTPUT_H__

#include "mx_analog_output.h"

typedef struct {
	MX_RECORD *controller_record;
	char channel_name;
	char signal_type[MXU_KEITHLEY2600_SIGNAL_TYPE_NAME_LENGTH+1];

	long channel_number;
	char lowercase_channel_name;
	char lowercase_signal_type;
} MX_KEITHLEY2600_AOUTPUT;

MX_API mx_status_type mxd_keithley2600_aoutput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_keithley2600_aoutput_open( MX_RECORD *record );
MX_API mx_status_type mxd_keithley2600_aoutput_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_keithley2600_aoutput_read(MX_ANALOG_OUTPUT *aoutput);
MX_API mx_status_type mxd_keithley2600_aoutput_write(MX_ANALOG_OUTPUT *aoutput);

extern MX_RECORD_FUNCTION_LIST mxd_keithley2600_aoutput_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
		mxd_keithley2600_aoutput_analog_output_function_list;

extern long mxd_keithley2600_aoutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_keithley2600_aoutput_rfield_def_ptr;

#define MXLV_KEITHLEY2600_AOUTPUT_SIGNAL_TYPE			82001

#define MXD_KEITHLEY2600_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "controller_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_KEITHLEY2600_AOUTPUT, controller_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel_name", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY2600_AOUTPUT,channel_name),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_KEITHLEY2600_AOUTPUT_SIGNAL_TYPE, -1, "signal_type", MXFT_STRING, \
		NULL, 1, {MXU_KEITHLEY2600_SIGNAL_TYPE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY2600_AOUTPUT,signal_type),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \


#endif /* __D_KEITHLEY2600_AOUTPUT_H__ */
