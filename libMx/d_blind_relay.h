/*
 * Name:    d_blind_relay.h
 *
 * Purpose: MX header file for blind relay support.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_BLIND_RELAY_H__
#define __D_BLIND_RELAY_H__

typedef struct {
	unsigned long settling_time;	/* in milliseconds */

	MX_RECORD *output_record;
	long output_bit_offset;
	long num_output_bits;
	unsigned long close_output_value;
	unsigned long open_output_value;
} MX_BLIND_RELAY;

#define MXD_BLIND_RELAY_STANDARD_FIELDS \
  {-1, -1, "settling_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BLIND_RELAY, settling_time), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "output_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BLIND_RELAY, output_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "output_bit_offset", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BLIND_RELAY, output_bit_offset), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "num_output_bits", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BLIND_RELAY, num_output_bits), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "close_output_value", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BLIND_RELAY, close_output_value),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "open_output_value", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BLIND_RELAY, open_output_value),\
	{0}, NULL, MXFF_IN_DESCRIPTION}

/* Define all of the device functions. */

MX_API mx_status_type mxd_blind_relay_initialize_type( long type );

MX_API mx_status_type mxd_blind_relay_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_blind_relay_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_blind_relay_delete_record( MX_RECORD *record );

MX_API mx_status_type mxd_blind_relay_print_structure( FILE *file,
							MX_RECORD *record );

MX_API mx_status_type mxd_blind_relay_read_parms_from_hardware(
							MX_RECORD *record );

MX_API mx_status_type mxd_blind_relay_write_parms_to_hardware(
							MX_RECORD *record );

MX_API mx_status_type mxd_blind_relay_open( MX_RECORD *record );

MX_API mx_status_type mxd_blind_relay_close( MX_RECORD *record );

MX_API mx_status_type mxd_blind_relay_relay_command( MX_RELAY *relay );

MX_API mx_status_type mxd_blind_relay_get_relay_status( MX_RELAY *relay );

extern MX_RECORD_FUNCTION_LIST mxd_blind_relay_record_function_list;
extern MX_RELAY_FUNCTION_LIST mxd_blind_relay_relay_function_list;

extern long mxd_blind_relay_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_blind_relay_rfield_def_ptr;

#endif /* __D_BLIND_RELAY_H__ */
