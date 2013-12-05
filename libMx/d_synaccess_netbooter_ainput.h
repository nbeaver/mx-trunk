/*
 * Name:    d_synaccess_netbooter_ainput.h
 *
 * Purpose: Header for the temperature or current inputs of the
 *          Synaccess netBooter Remote Power Management system.
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

#ifndef __D_SYNACCESS_NETBOOTER_AINPUT_H__
#define __D_SYNACCESS_NETBOOTER_AINPUT_H__

#define MXU_SYNACCESS_NETBOOTER_AINPUT_TYPE_LENGTH	20

#define MXT_SYNACCESS_NETBOOTER_AINPUT_CURRENT		1
#define MXT_SYNACCESS_NETBOOTER_AINPUT_TEMPERATURE	2

typedef struct {
	MX_RECORD *record;

	MX_RECORD *synaccess_netbooter_record;
	char input_type_name[ MXU_SYNACCESS_NETBOOTER_AINPUT_TYPE_LENGTH+1 ];
	unsigned long port_number;

	int input_type;
} MX_SYNACCESS_NETBOOTER_AINPUT;

MX_API mx_status_type mxd_sa_netbooter_ainput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_sa_netbooter_ainput_open( MX_RECORD *record );

MX_API mx_status_type mxd_sa_netbooter_ainput_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_sa_netbooter_ainput_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_sa_netbooter_ainput_analog_input_function_list;

extern long mxd_sa_netbooter_ainput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_sa_netbooter_ainput_rfield_def_ptr;

#define MXD_SYNACCESS_NETBOOTER_AINPUT_STANDARD_FIELDS \
  {-1, -1, "synaccess_netbooter_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
	  offsetof(MX_SYNACCESS_NETBOOTER_AINPUT, synaccess_netbooter_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "input_type_name", MXFT_STRING, NULL, \
		1, {MXU_SYNACCESS_NETBOOTER_AINPUT_TYPE_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SYNACCESS_NETBOOTER_AINPUT, input_type_name), \
	{sizeof(char)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY) }, \
  \
  {-1, -1, "port_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SYNACCESS_NETBOOTER_AINPUT, port_number), \
	{0}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY) }

#endif /* __D_SYNACCESS_NETBOOTER_AINPUT_H__ */

