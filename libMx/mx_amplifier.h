/*
 * Name:    mx_amplifier.h
 *
 * Purpose: MX header file for amplifier support.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2002, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_AMPLIFIER_H__
#define __MX_AMPLIFIER_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MX_AMPLIFIER_NUM_GAIN_RANGE_PARAMS	2

typedef struct {
	MX_RECORD *record; /* Pointer to the MX_RECORD structure that points
			    * to this amplifier.
			    */
	double gain;
	double offset;
	double time_constant;

	long parameter_type;

	double gain_range[2];
} MX_AMPLIFIER;

#define MXLV_AMP_GAIN		8001
#define MXLV_AMP_OFFSET		8002
#define MXLV_AMP_TIME_CONSTANT	8003
#define MXLV_AMP_PARAMETER_TYPE 8004
#define MXLV_AMP_GAIN_RANGE     8005

#define MX_AMPLIFIER_STANDARD_FIELDS \
  {MXLV_AMP_GAIN, -1, "gain", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AMPLIFIER, gain), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_AMP_OFFSET, -1, "offset", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AMPLIFIER, offset), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_AMP_TIME_CONSTANT, -1, "time_constant", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AMPLIFIER, time_constant), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "parameter_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AMPLIFIER, parameter_type), \
	{0}, NULL, 0}, \
  \
  {MXLV_AMP_GAIN_RANGE, -1, "gain_range", MXFT_DOUBLE, NULL, \
				1, {MX_AMPLIFIER_NUM_GAIN_RANGE_PARAMS}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AMPLIFIER, gain_range), \
	{sizeof(double)}, NULL, 0}

typedef struct {
	mx_status_type ( *get_gain ) ( MX_AMPLIFIER *amplifier );
	mx_status_type ( *set_gain ) ( MX_AMPLIFIER *amplifier );
	mx_status_type ( *get_offset ) ( MX_AMPLIFIER *amplifier );
	mx_status_type ( *set_offset ) ( MX_AMPLIFIER *amplifier );
	mx_status_type ( *get_time_constant ) ( MX_AMPLIFIER *amplifier );
	mx_status_type ( *set_time_constant ) ( MX_AMPLIFIER *amplifier );
	mx_status_type ( *get_parameter ) ( MX_AMPLIFIER *amplifier );
	mx_status_type ( *set_parameter ) ( MX_AMPLIFIER *amplifier );
} MX_AMPLIFIER_FUNCTION_LIST;

MX_API mx_status_type mx_amplifier_get_gain( MX_RECORD *amplifier_record,
							double *gain );

MX_API mx_status_type mx_amplifier_set_gain( MX_RECORD *amplifier_record,
							double gain );

MX_API mx_status_type mx_amplifier_get_offset( MX_RECORD *amplifier_record,
							double *offset );

MX_API mx_status_type mx_amplifier_set_offset( MX_RECORD *amplifier_record,
							double offset );

MX_API mx_status_type mx_amplifier_get_time_constant(
			MX_RECORD *amplifier_record, double *time_constant );

MX_API mx_status_type mx_amplifier_set_time_constant(
			MX_RECORD *amplifier_record, double time_constant );

MX_API mx_status_type mx_amplifier_get_parameter( MX_RECORD *amplifier_record,
							long parameter_type );

MX_API mx_status_type mx_amplifier_set_parameter( MX_RECORD *amplifier_record,
							long parameter_type );

MX_API mx_status_type mx_amplifier_default_get_parameter_handler(
						MX_AMPLIFIER *amplifier );

MX_API mx_status_type mx_amplifier_default_set_parameter_handler(
						MX_AMPLIFIER *amplifier );

MX_API mx_status_type mx_amplifier_get_gain_range( MX_RECORD *amplifier_record,
						double *gain_range_array );

#ifdef __cplusplus
}
#endif

#endif /* __MX_AMPLIFIER_H__ */
