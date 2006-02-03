/*
 * Name:    d_net_amplifier.h
 *
 * Purpose: Header file for MX amplifier driver for amplifiers controlled
 *          via an MX network server.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NET_AMPLIFIER_H__
#define __D_NET_AMPLIFIER_H__

#include "mx_amplifier.h"
#include "mx_net.h"

typedef struct {
	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD gain_nf;
	MX_NETWORK_FIELD gain_range_nf;
	MX_NETWORK_FIELD offset_nf;
	MX_NETWORK_FIELD resynchronize_nf;
	MX_NETWORK_FIELD time_constant_nf;
} MX_NETWORK_AMPLIFIER;

MX_API mx_status_type mxd_network_amplifier_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_amplifier_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_amplifier_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_network_amplifier_get_gain(
						MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_network_amplifier_set_gain(
						MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_network_amplifier_get_offset(
						MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_network_amplifier_set_offset(
						MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_network_amplifier_get_time_constant(
						MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_network_amplifier_set_time_constant(
						MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_network_amplifier_get_parameter(
						MX_AMPLIFIER *amplifier );

MX_API mx_status_type mxd_network_amplifier_get_pointers(
				MX_AMPLIFIER *amplifier,
				MX_NETWORK_AMPLIFIER **network_amplifier,
				const char *calling_fname );

extern MX_RECORD_FUNCTION_LIST mxd_network_amplifier_record_function_list;
extern MX_AMPLIFIER_FUNCTION_LIST mxd_network_amplifier_amplifier_function_list;

extern mx_length_type mxd_network_amplifier_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_network_amplifier_rfield_def_ptr;

#define MXD_NETWORK_AMPLIFIER_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_AMPLIFIER, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
	NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_AMPLIFIER, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_NET_AMPLIFIER_H__ */
