/*
 * Name:    d_synaccess_netbooter_relay.h
 *
 * Purpose: MX header file for using a Synaccess netBooter-controlled
 *          power outlet as an MX relay record.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SYNACCESS_NETBOOTER_RELAY_H__
#define __D_SYNACCESS_NETBOOTER_RELAY_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *synaccess_netbooter_record;
	unsigned long port_number;
} MX_SYNACCESS_NETBOOTER_RELAY;

#define MXD_SYNACCESS_NETBOOTER_RELAY_STANDARD_FIELDS \
  {-1, -1, "synaccess_netbooter_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
	  offsetof(MX_SYNACCESS_NETBOOTER_RELAY, synaccess_netbooter_record),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "port_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SYNACCESS_NETBOOTER_RELAY, port_number),\
	{0}, NULL, MXFF_IN_DESCRIPTION}

/* Define all of the device functions. */

MX_API mx_status_type mxd_sa_netbooter_relay_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_sa_netbooter_relay_open( MX_RECORD *record );

MX_API mx_status_type mxd_sa_netbooter_relay_relay_command( MX_RELAY *relay );

MX_API mx_status_type mxd_sa_netbooter_relay_get_relay_status( MX_RELAY *relay);

extern MX_RECORD_FUNCTION_LIST mxd_sa_netbooter_relay_record_function_list;
extern MX_RELAY_FUNCTION_LIST mxd_sa_netbooter_relay_relay_function_list;

extern long mxd_sa_netbooter_relay_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_sa_netbooter_relay_rfield_def_ptr;

#endif /* __D_SYNACCESS_NETBOOTER_RELAY_H__ */
