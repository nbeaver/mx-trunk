/*
 * Name:    i_synaccess_netbooter.h
 *
 * Purpose: Header file for the Synaccess netBooter Remote Power
 *          Management system.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_SYNACCESS_NETBOOTER_H__
#define __I_SYNACCESS_NETBOOTER_H__

/*---*/

#define MXI_SYNACCESS_NETBOOTER_USERNAME_LENGTH		20
#define MXI_SYNACCESS_NETBOOTER_PASSWORD_LENGTH		80

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	char username[ MXI_SYNACCESS_NETBOOTER_USERNAME_LENGTH ];
	char password[ MXI_SYNACCESS_NETBOOTER_PASSWORD_LENGTH ];
} MX_SYNACCESS_NETBOOTER;

#define MXI_SYNACCESS_NETBOOTER_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SYNACCESS_NETBOOTER, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "username", MXFT_STRING, NULL, \
			1, {MXI_SYNACCESS_NETBOOTER_USERNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SYNACCESS_NETBOOTER, username), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_NO_ACCESS) }, \
  \
  {-1, -1, "password", MXFT_STRING, NULL, \
			1, {MXI_SYNACCESS_NETBOOTER_PASSWORD_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SYNACCESS_NETBOOTER, username), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_NO_ACCESS) }

MX_API mx_status_type mxi_synaccess_netbooter_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxi_synaccess_netbooter_open( MX_RECORD *record );

MX_API mx_status_type mxi_synaccess_netbooter_command(
				MX_SYNACCESS_NETBOOTER *synaccess_netbooter,
				char *command,
				char *response,
				size_t response_buffer_length,
				mx_bool_type debug_flag );

extern MX_RECORD_FUNCTION_LIST mxi_synaccess_netbooter_record_function_list;

extern long mxi_synaccess_netbooter_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_synaccess_netbooter_rfield_def_ptr;

#endif /* __I_SYNACCESS_NETBOOTER_H__ */

