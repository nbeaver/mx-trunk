/*
 * Name:    d_bnc725.h
 *
 * Purpose: MX pulse generator driver for one channel of
 *          the BNC725 digital delay generator.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010-2011, 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_BNC725_H__
#define __D_BNC725_H__

#ifdef __cplusplus

typedef struct {
	MX_RECORD *record;

	MX_RECORD *bnc725_record;
	char channel_name;
	char channel_logic[MXU_BNC725_LOGIC_LENGTH+1];

	CChannel *channel;
} MX_BNC725_CHANNEL;

#define MXD_BNC725_STANDARD_FIELDS \
  {-1, -1, "bnc725_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BNC725_CHANNEL, bnc725_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel_name", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BNC725_CHANNEL, channel_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel_logic", MXFT_STRING, NULL, 1, {MXU_BNC725_LOGIC_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BNC725_CHANNEL, channel_logic), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/*--------------------------------------------*/

MX_API_PRIVATE mx_status_type mxdp_bnc725_setup_channel(
				MX_PULSE_GENERATOR *pulser,
				MX_BNC725_CHANNEL *bnc723_channel,
				MX_BNC725 *bnc725 );

#endif /* __cplusplus */

/*--------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type mxd_bnc725_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_bnc725_open( MX_RECORD *record );

MX_API mx_status_type mxd_bnc725_busy( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_bnc725_start( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_bnc725_stop( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_bnc725_get_parameter( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_bnc725_set_parameter( MX_PULSE_GENERATOR *pulser );

extern MX_RECORD_FUNCTION_LIST mxd_bnc725_record_function_list;
extern MX_PULSE_GENERATOR_FUNCTION_LIST 
		mxd_bnc725_pulse_generator_function_list;

extern long mxd_bnc725_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_bnc725_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __D_BNC725_H__ */

