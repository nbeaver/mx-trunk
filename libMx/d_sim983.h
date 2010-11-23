/*
 * Name:    d_sim983.h
 *
 * Purpose: Header for the Stanford Research Systems SIM983 scaling amplifier.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SIM983_H__
#define __D_SIM983_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *port_record;
	double version;
} MX_SIM983;

#define MXD_SIM983_STANDARD_FIELDS \
  {-1, -1, "port_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIM983, port_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_sim983_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_sim983_open( MX_RECORD *record );

MX_API mx_status_type mxd_sim983_get_gain( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_sim983_set_gain( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_sim983_get_offset( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_sim983_set_offset( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_sim983_get_time_constant( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_sim983_set_time_constant( MX_AMPLIFIER *amplifier );

extern MX_RECORD_FUNCTION_LIST mxd_sim983_record_function_list;

extern MX_AMPLIFIER_FUNCTION_LIST mxd_sim983_amplifier_function_list;

extern long mxd_sim983_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_sim983_rfield_def_ptr;

#endif /* __D_SIM983_H__ */

