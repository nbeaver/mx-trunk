/*
 * Name:    mx_sca.h
 *
 * Purpose: MX system header file for single channel analyzers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_SCA_H__
#define __MX_SCA_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	MX_RECORD *record;

	long parameter_type;

	double lower_level;
	double upper_level;

	double gain;
	double time_constant;

	long sca_mode;
} MX_SCA;

/* SCA modes */

#define MXF_SCA_UNKNOWN_MODE	0
#define MXF_SCA_WINDOW_MODE	1
#define MXF_SCA_INTEGRAL_MODE	2
#define MXF_SCA_ASYMMETRIC_MODE	3
#define MXF_SCA_SYMMETRIC_MODE	4


#define MXLV_SCA_LOWER_LEVEL		1001
#define MXLV_SCA_UPPER_LEVEL		1002
#define MXLV_SCA_GAIN			1003
#define MXLV_SCA_TIME_CONSTANT		1004
#define MXLV_SCA_MODE			1005

#define MX_SCA_STANDARD_FIELDS \
  {MXLV_SCA_LOWER_LEVEL, -1, "lower_level", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SCA, lower_level), \
	{0}, NULL, MXFF_IN_SUMMARY}, \
  \
  {MXLV_SCA_UPPER_LEVEL, -1, "upper_level", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SCA, upper_level), \
	{0}, NULL, MXFF_IN_SUMMARY}, \
  \
  {MXLV_SCA_GAIN, -1, "gain", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SCA, gain), \
	{0}, NULL, MXFF_IN_SUMMARY}, \
  \
  {MXLV_SCA_TIME_CONSTANT, -1, "time_constant", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SCA, time_constant), \
	{0}, NULL, MXFF_IN_SUMMARY}, \
  \
  {MXLV_SCA_MODE, -1, "sca_mode", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SCA, sca_mode), \
	{0}, NULL, MXFF_IN_SUMMARY}

typedef struct {
	mx_status_type ( *get_parameter ) ( MX_SCA *sca );
	mx_status_type ( *set_parameter ) ( MX_SCA *sca );
} MX_SCA_FUNCTION_LIST;

MX_API_PRIVATE mx_status_type mx_sca_get_pointers( MX_RECORD *sca_record,
	MX_SCA **sca, MX_SCA_FUNCTION_LIST **function_list_ptr,
	const char *calling_fname );

MX_API mx_status_type mx_sca_get_parameter( MX_RECORD *sca_record,
						long parameter_type );

MX_API mx_status_type mx_sca_set_parameter( MX_RECORD *sca_record,
						long parameter_type );

MX_API mx_status_type mx_sca_get_lower_level( MX_RECORD *sca_record,
						double *lower_level );

MX_API mx_status_type mx_sca_set_lower_level( MX_RECORD *sca_record,
						double lower_level );

MX_API mx_status_type mx_sca_get_upper_level( MX_RECORD *sca_record,
						double *upper_level );

MX_API mx_status_type mx_sca_set_upper_level( MX_RECORD *sca_record,
						double upper_level );

MX_API mx_status_type mx_sca_get_gain( MX_RECORD *sca_record, double *gain );

MX_API mx_status_type mx_sca_set_gain( MX_RECORD *sca_record, double gain );

MX_API mx_status_type mx_sca_get_time_constant( MX_RECORD *sca_record,
						double *time_constant );

MX_API mx_status_type mx_sca_set_time_constant( MX_RECORD *sca_record,
						double time_constant );

MX_API mx_status_type mx_sca_get_mode( MX_RECORD *sca_record, long *mode );

MX_API mx_status_type mx_sca_set_mode( MX_RECORD *sca_record, long mode );

MX_API mx_status_type mx_sca_default_get_parameter_handler( MX_SCA *sca );

MX_API mx_status_type mx_sca_default_set_parameter_handler( MX_SCA *sca );

#ifdef __cplusplus
}
#endif

#endif /* __MX_SCA_H__ */

