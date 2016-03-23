/*
 * Name:    d_network_pulser.h
 *
 * Purpose: Header for MX network pulse generator devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2004-2005, 2015-2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NETWORK_PULSER_H__
#define __D_NETWORK_PULSER_H__

typedef struct {
	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD busy_nf;
	MX_NETWORK_FIELD last_pulse_number_nf;
	MX_NETWORK_FIELD mode_nf;
	MX_NETWORK_FIELD num_pulses_nf;
	MX_NETWORK_FIELD pulse_delay_nf;
	MX_NETWORK_FIELD pulse_period_nf;
	MX_NETWORK_FIELD pulse_width_nf;
	MX_NETWORK_FIELD setup_nf;
	MX_NETWORK_FIELD start_nf;
	MX_NETWORK_FIELD stop_nf;
} MX_NETWORK_PULSER;

#define MXD_NETWORK_PULSER_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_PULSER, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
					NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_PULSER, remote_record_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_network_pulser_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_network_pulser_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_network_pulser_open( MX_RECORD *record );

MX_API mx_status_type mxd_network_pulser_busy(
					MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_network_pulser_start(
					MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_network_pulser_stop(
					MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_network_pulser_get_parameter(
					MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_network_pulser_set_parameter(
					MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_network_pulser_setup(
					MX_PULSE_GENERATOR *pulse_generator );

extern MX_RECORD_FUNCTION_LIST mxd_network_pulser_record_function_list;
extern MX_PULSE_GENERATOR_FUNCTION_LIST 
		mxd_network_pulser_pulse_generator_function_list;

extern long mxd_network_pulser_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_network_pulser_rfield_def_ptr;

#endif /* __D_NETWORK_PULSER_H__ */

