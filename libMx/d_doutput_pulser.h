/*
 * Name:    d_doutput_pulser.h
 *
 * Purpose: Header file for using an MX digital output driver as a
 *          pulse generator.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011, 2013, 2018-2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_DOUTPUT_PULSER_H__
#define __D_DOUTPUT_PULSER_H__

#include "mx_pulse_generator.h"

#define MXF_DOUTPUT_PULSER_ALLOW_TIME_SKEW	0x1

typedef struct {
	MX_RECORD *record;

	MX_RECORD *digital_output_record;
	unsigned long digital_output_pulser_flags;

	mx_bool_type use_callback;
	MX_CALLBACK_MESSAGE *callback_message;

	mx_bool_type doutput_is_high;
	struct timespec next_transition_timespec;
	long num_pulses_left;
	mx_bool_type count_forever;
} MX_DOUTPUT_PULSER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_doutput_pulser_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_doutput_pulser_open( MX_RECORD *record );

MX_API mx_status_type mxd_doutput_pulser_is_busy( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_doutput_pulser_arm( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_doutput_pulser_trigger( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_doutput_pulser_stop( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_doutput_pulser_get_parameter(
					MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_doutput_pulser_set_parameter(
					MX_PULSE_GENERATOR *pulser );

extern MX_RECORD_FUNCTION_LIST mxd_doutput_pulser_record_function_list;
extern MX_PULSE_GENERATOR_FUNCTION_LIST mxd_doutput_pulser_pulser_function_list;

extern long mxd_doutput_pulser_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_doutput_pulser_rfield_def_ptr;

#define MXD_DOUTPUT_PULSER_STANDARD_FIELDS \
  {-1, -1, "digital_output_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_DOUTPUT_PULSER, digital_output_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "digital_output_pulser_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_DOUTPUT_PULSER, digital_output_pulser_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_DOUTPUT_PULSER_H__ */

