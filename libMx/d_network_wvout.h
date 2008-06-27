/*
 * Name:    d_network_wvout.h
 *
 * Purpose: Header for MX waveform output driver to control 
 *          MX network timers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NETWORK_WVOUT_H__
#define __D_NETWORK_WVOUT_H__

/*----*/

typedef struct {
	MX_RECORD *record;

	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD arm_nf;
	MX_NETWORK_FIELD busy_nf;
	MX_NETWORK_FIELD channel_data_nf;
	MX_NETWORK_FIELD channel_index_nf;
	MX_NETWORK_FIELD current_num_steps_nf;
	MX_NETWORK_FIELD frequency_nf;
	MX_NETWORK_FIELD stop_nf;
	MX_NETWORK_FIELD trigger_nf;
	MX_NETWORK_FIELD trigger_mode_nf;
} MX_NETWORK_WVOUT;

#define MXD_NETWORK_WVOUT_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_NETWORK_WVOUT, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, NULL, \
					1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_WVOUT, remote_record_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}

MX_API mx_status_type mxd_network_wvout_initialize_type( long type );
MX_API mx_status_type mxd_network_wvout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_wvout_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_wvout_open( MX_RECORD *record );

MX_API mx_status_type mxd_network_wvout_arm( MX_WAVEFORM_OUTPUT *wvout );
MX_API mx_status_type mxd_network_wvout_trigger( MX_WAVEFORM_OUTPUT *wvout );
MX_API mx_status_type mxd_network_wvout_stop( MX_WAVEFORM_OUTPUT *wvout );
MX_API mx_status_type mxd_network_wvout_busy( MX_WAVEFORM_OUTPUT *wvout );
MX_API mx_status_type mxd_network_wvout_read_channel(
						MX_WAVEFORM_OUTPUT *wvout );
MX_API mx_status_type mxd_network_wvout_write_channel(
						MX_WAVEFORM_OUTPUT *wvout );
MX_API mx_status_type mxd_network_wvout_get_parameter(
						MX_WAVEFORM_OUTPUT *wvout );
MX_API mx_status_type mxd_network_wvout_set_parameter(
						MX_WAVEFORM_OUTPUT *wvout );

extern MX_RECORD_FUNCTION_LIST mxd_network_wvout_record_function_list;
extern MX_WAVEFORM_OUTPUT_FUNCTION_LIST
			mxd_network_wvout_wvout_function_list;

extern long mxd_network_wvout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_network_wvout_rfield_def_ptr;

#endif /* __D_NETWORK_WVOUT_H__ */
