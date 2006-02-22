/*
 * Name:    d_icplus.h
 *
 * Purpose: Header for the MX amplifier driver for the Oxford Danfysik
 *          IC PLUS intensity monitor and QBPM position monitor.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2002-2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ICPLUS_H__
#define __D_ICPLUS_H__

#define MXF_QBPM_USE_NEW_GAINS		0x1

#define MXU_ICPLUS_VALUE_NAME_LENGTH	10
#define MXU_ICPLUS_COMMAND_LENGTH	80

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	int address;
	int default_averaging;
	unsigned long qbpm_flags;

	int range;
	int discard_echoed_command_line;
} MX_ICPLUS;

#define MXD_ICPLUS_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ICPLUS, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address", MXFT_INT, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_ICPLUS, address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "range", MXFT_INT, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_ICPLUS, range), \
	{0}, NULL, 0}

#define MXD_QBPM_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ICPLUS, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address", MXFT_INT, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_ICPLUS, address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "default_averaging", MXFT_INT, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_ICPLUS, default_averaging), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "qbpm_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ICPLUS, qbpm_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "range", MXFT_INT, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_ICPLUS, range), \
	{0}, NULL, 0}

MX_API mx_status_type mxd_icplus_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_icplus_open( MX_RECORD *record );
MX_API mx_status_type mxd_icplus_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_icplus_get_gain( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_icplus_set_gain( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_icplus_get_offset( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_icplus_set_offset( MX_AMPLIFIER *amplifier );

MX_API mx_status_type mxd_icplus_command( MX_ICPLUS *icplus, char *command,
				char *response, size_t response_buffer_length,
				int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxd_icplus_record_function_list;

extern MX_AMPLIFIER_FUNCTION_LIST mxd_icplus_amplifier_function_list;

extern long mxd_icplus_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_icplus_rfield_def_ptr;

extern long mxd_qbpm_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_qbpm_rfield_def_ptr;

#endif /* __D_ICPLUS_H__ */
