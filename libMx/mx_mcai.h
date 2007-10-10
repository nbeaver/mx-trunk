/*
 * Name:    mx_mcai.h
 *
 * Purpose: MX header file for multichannel analog input devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_MCAI_H__
#define __MX_MCAI_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MXT_MCAI_PERFORM_TIME_NORMALIZATION	0x1

typedef struct {
	MX_RECORD *record;

	long maximum_num_channels;
	long current_num_channels;

	double *channel_array;

	double scale;
	double offset;
	char units[MXU_UNITS_NAME_LENGTH+1];
	unsigned long mcai_flags;
} MX_MCAI;

#define MXLV_MCAI_CHANNEL_ARRAY		10001
#define MXLV_MCAI_MAXIMUM_NUM_CHANNELS	10002
#define MXLV_MCAI_CURRENT_NUM_CHANNELS	10002

#define MX_MCAI_STANDARD_FIELDS \
  {MXLV_MCAI_MAXIMUM_NUM_CHANNELS, -1, "maximum_num_channels", \
	  		MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_MCAI, maximum_num_channels), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY),}, \
  \
  {MXLV_MCAI_CURRENT_NUM_CHANNELS, -1, "current_num_channels", \
	  		MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCAI, current_num_channels), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY),}, \
  \
  {MXLV_MCAI_CHANNEL_ARRAY, -1, "channel_array", \
	  		MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCAI, channel_array), \
	{sizeof(double)}, NULL, (MXFF_VARARGS | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "scale", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCAI, scale), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "offset", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCAI, offset), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "units", MXFT_STRING, NULL, 1, {MXU_UNITS_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCAI, units), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "mcai_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCAI, mcai_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

/*
 * The structure type MX_MCAI_FUNCTION_LIST contains
 * a list of pointers to functions that vary from driver to driver.
 */

typedef struct {
	mx_status_type ( *read ) ( MX_MCAI *mcai );
} MX_MCAI_FUNCTION_LIST;

MX_API mx_status_type mx_mcai_initialize_type(
			long record_type,
			long *num_record_fields,
			MX_RECORD_FIELD_DEFAULTS **record_field_defaults,
			long *maximum_num_channels_varargs_cookie );

MX_API mx_status_type mx_mcai_finish_record_initialization(
							MX_RECORD *mcai_record);

MX_API mx_status_type mx_mcai_read( MX_RECORD *mcai_record,
						unsigned long *num_channels,
						double **channel_array );

#ifdef __cplusplus
}
#endif

#endif /* __MX_MCAI_H__ */

