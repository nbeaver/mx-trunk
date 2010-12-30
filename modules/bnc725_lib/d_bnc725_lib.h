/*
 * Name:    d_bnc725_lib.h
 *
 * Purpose: MX pulse generator driver for one channel of
 *          the BNC725 digital delay generator.
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

#ifndef __D_BNC725_LIB_H__
#define __D_BNC725_LIB_H__

#ifdef __cplusplus

typedef struct {
	MX_RECORD *record;

	MX_RECORD *bnc725_lib_record;
	char channel_name;
} MX_BNC725_LIB_CHANNEL;

#define MXD_BNC725_LIB_STANDARD_FIELDS \
  {-1, -1, "bnc725_lib_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BNC725_LIB_CHANNEL, bnc725_lib_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel_name", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BNC725_LIB_CHANNEL, channel_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __cplusplus */

/*--------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type mxd_bnc725_lib_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_bnc725_lib_open( MX_RECORD *record );

MX_API mx_status_type mxd_bnc725_lib_busy(
					MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_bnc725_lib_start(
					MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_bnc725_lib_stop(
					MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_bnc725_lib_get_parameter(
					MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_bnc725_lib_set_parameter(
					MX_PULSE_GENERATOR *pulse_generator );

extern MX_RECORD_FUNCTION_LIST mxd_bnc725_lib_record_function_list;
extern MX_PULSE_GENERATOR_FUNCTION_LIST 
		mxd_bnc725_lib_pulse_generator_function_list;

extern long mxd_bnc725_lib_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_bnc725_lib_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __D_BNC725_LIB_H__ */

