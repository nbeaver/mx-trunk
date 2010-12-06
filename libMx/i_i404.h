/*
 * Name:    i_i404.h
 *
 * Purpose: Header file for the Pyramid Technical Consultants I404
 *          digital electrometer.
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

#ifndef __I_I404_H__
#define __I_I404_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	long address;

	double version;

	long calibration_gain;
	long calibration_source;
	long configure_capacitor;
	long configure_resolution;
} MX_I404;

#define MXLV_I404_CALIBRATION_GAIN	16101
#define MXLV_I404_CALIBRATION_SOURCE	16102
#define MXLV_I404_CONFIGURE_CAPACITOR	16103
#define MXLV_I404_CONFIGURE_RESOLUTION	16104

#define MXI_I404_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_I404, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_I404, address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_I404_CALIBRATION_GAIN, -1, "calibration_gain", \
					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_I404, calibration_gain), \
	{0}, NULL, 0 }, \
  \
  {MXLV_I404_CALIBRATION_SOURCE, -1, "calibration_source", \
					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_I404, calibration_source), \
	{0}, NULL, 0 }, \
  \
  {MXLV_I404_CONFIGURE_CAPACITOR, -1, "configure_capacitor", \
					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_I404, configure_capacitor), \
	{0}, NULL, 0 }, \
  \
  {MXLV_I404_CONFIGURE_RESOLUTION, -1, "configure_resolution", \
					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_I404, configure_resolution), \
	{0}, NULL, 0 }

MX_API mx_status_type mxi_i404_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_i404_open( MX_RECORD *record );
MX_API mx_status_type mxi_i404_special_processing_setup( MX_RECORD *record );

MX_API mx_status_type mxi_i404_command( MX_I404 *i404,
					char *command,
					char *response,
					size_t max_response_length,
					unsigned long i404_flags );

extern MX_RECORD_FUNCTION_LIST mxi_i404_record_function_list;

extern long mxi_i404_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_i404_rfield_def_ptr;

#endif /* __I_I404_H__ */

