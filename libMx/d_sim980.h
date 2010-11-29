/*
 * Name:    d_sim980.h
 *
 * Purpose: Header for the Stanford Research Systems SIM980 summing amplifier.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SIM980_H__
#define __D_SIM980_H__

#include "mx_analog_input.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *port_record;

	double version;
	unsigned long channel_number;
	long channel_control;
	unsigned long averaging_time;	/* in milliseconds */
} MX_SIM980;

MX_API mx_status_type mxd_sim980_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_sim980_open( MX_RECORD *record );
MX_API mx_status_type mxd_sim980_special_processing_setup( MX_RECORD *record );

MX_API mx_status_type mxd_sim980_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_sim980_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_sim980_analog_input_function_list;

extern long mxd_sim980_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_sim980_rfield_def_ptr;

#define MXLV_SIM980_CHANNEL_NUMBER	9801
#define MXLV_SIM980_CHANNEL_CONTROL	9802

#define MXD_SIM980_STANDARD_FIELDS \
  {-1, -1, "port_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIM980, port_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "version", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIM980, version), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_SIM980_CHANNEL_NUMBER, -1, "channel_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIM980, channel_number), \
	{0}, NULL, 0}, \
  \
  {MXLV_SIM980_CHANNEL_CONTROL, -1, "channel_control", \
					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIM980, channel_control), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "averaging_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIM980, averaging_time), \
	{0}, NULL, 0}

#endif /* __D_SIM980_H__ */

