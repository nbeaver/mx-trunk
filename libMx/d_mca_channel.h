/*
 * Name:    d_mca_channel.h
 *
 * Purpose: Header file for MX scaler driver for individual MCA channels.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 2000-2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MCA_CHANNEL_H__
#define __D_MCA_CHANNEL_H__

typedef struct {
	MX_RECORD *mca_record;
	uint32_t channel_number;
} MX_MCA_CHANNEL;

MX_API mx_status_type mxd_mca_channel_initialize_type( long type );
MX_API mx_status_type mxd_mca_channel_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_mca_channel_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_mca_channel_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_mca_channel_read_parms_from_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_mca_channel_write_parms_to_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_mca_channel_open( MX_RECORD *record );
MX_API mx_status_type mxd_mca_channel_close( MX_RECORD *record );

MX_API mx_status_type mxd_mca_channel_clear( MX_SCALER *scaler );
MX_API mx_status_type mxd_mca_channel_overflow_set( MX_SCALER *scaler );
MX_API mx_status_type mxd_mca_channel_read( MX_SCALER *scaler );
MX_API mx_status_type mxd_mca_channel_is_busy( MX_SCALER *scaler );
MX_API mx_status_type mxd_mca_channel_start( MX_SCALER *scaler );
MX_API mx_status_type mxd_mca_channel_stop( MX_SCALER *scaler );
MX_API mx_status_type mxd_mca_channel_get_parameter( MX_SCALER *scaler );
MX_API mx_status_type mxd_mca_channel_set_parameter( MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_mca_channel_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_mca_channel_scaler_function_list;

extern mx_length_type mxd_mca_channel_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mca_channel_rfield_def_ptr;

#define MXD_MCA_CHANNEL_STANDARD_FIELDS \
  {-1, -1, "mca_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCA_CHANNEL, mca_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "channel_number", MXFT_UINT32, NULL, 0, {0}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_MCA_CHANNEL, channel_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_MCA_CHANNEL_H__ */
