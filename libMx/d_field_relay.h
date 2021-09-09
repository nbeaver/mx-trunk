/*
 * Name:    d_field_relay.h
 *
 * Purpose: MX header file for field relay support.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_FIELD_RELAY_H__
#define __D_FIELD_RELAY_H__

/* Flag values for the frelay_flags field. */

#define MXF_FRELAY_INVERT_INPUT				0x1
#define MXF_FRELAY_INVERT_OUTPUT			0x2
#define MXF_FRELAY_ONLY_CHECK_CLOSED			0x4
#define MXF_FRELAY_ONLY_CHECK_OPEN			0x8
#define MXF_FRELAY_IGNORE_STATUS			0x10

#define MXF_FRELAY_SUPPRESS_ILLEGAL_STATUS_MESSAGE	0x1000

typedef struct {
	MX_RECORD *record;

	unsigned long frelay_flags;

	unsigned long settling_time;	/* in milliseconds */

	/* You read the relay status from here. */

	char input_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	MX_RECORD_FIELD *input_field;
	long input_bit_offset;
	long num_input_bits;
	unsigned long closed_input_value;
	unsigned long open_input_value;

	/* You send relay commands to here. */

	char output_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	MX_RECORD_FIELD *output_field;
	long output_bit_offset;
	long num_output_bits;
	unsigned long close_output_value;
	unsigned long open_output_value;
} MX_FIELD_RELAY;

#define MXD_FIELD_RELAY_STANDARD_FIELDS \
  {-1, -1, "frelay_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FIELD_RELAY, frelay_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "settling_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FIELD_RELAY, settling_time), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "input_name", MXFT_STRING, NULL, 1, {MXU_RECORD_FIELD_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_FIELD_RELAY, input_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "input_bit_offset", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FIELD_RELAY, input_bit_offset), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "num_input_bits", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FIELD_RELAY, num_input_bits), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "closed_input_value", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FIELD_RELAY, closed_input_value),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "open_input_value", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FIELD_RELAY, open_input_value),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "output_name", MXFT_STRING, NULL, 1, {MXU_RECORD_FIELD_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_FIELD_RELAY, output_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "output_bit_offset", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FIELD_RELAY, output_bit_offset), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "num_output_bits", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FIELD_RELAY, num_output_bits), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "close_output_value", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FIELD_RELAY, close_output_value),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "open_output_value", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FIELD_RELAY, open_output_value),\
	{0}, NULL, MXFF_IN_DESCRIPTION}

/* Define all of the device functions. */

MX_API mx_status_type mxd_field_relay_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_field_relay_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_field_relay_relay_command( MX_RELAY *relay );

MX_API mx_status_type mxd_field_relay_get_relay_status( MX_RELAY *relay );

extern MX_RECORD_FUNCTION_LIST mxd_field_relay_record_function_list;
extern MX_RELAY_FUNCTION_LIST mxd_field_relay_relay_function_list;

extern long mxd_field_relay_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_field_relay_rfield_def_ptr;

#endif /* __D_FIELD_RELAY_H__ */
