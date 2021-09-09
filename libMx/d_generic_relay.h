/*
 * Name:    d_generic_relay.h
 *
 * Purpose: MX header file for generic relay support.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004, 2006, 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_GENERIC_RELAY_H__
#define __D_GENERIC_RELAY_H__

/* Flag values for the grelay_flags field. */

#define MXF_GRELAY_INVERT_INPUT				0x1
#define MXF_GRELAY_INVERT_OUTPUT			0x2
#define MXF_GRELAY_ONLY_CHECK_CLOSED			0x4
#define MXF_GRELAY_ONLY_CHECK_OPEN			0x8
#define MXF_GRELAY_IGNORE_STATUS			0x10

#define MXF_GRELAY_SUPPRESS_ILLEGAL_STATUS_MESSAGE	0x1000

typedef struct {
	MX_RECORD *record;

	unsigned long grelay_flags;

	unsigned long settling_time;	/* in milliseconds */

	MX_RECORD *input_record;   /* You read the relay status from here. */
	long input_bit_offset;
	long num_input_bits;
	unsigned long closed_input_value;
	unsigned long open_input_value;

	MX_RECORD *output_record;  /* You send relay commands to here. */
	long output_bit_offset;
	long num_output_bits;
	unsigned long close_output_value;
	unsigned long open_output_value;
} MX_GENERIC_RELAY;

#define MXD_GENERIC_RELAY_STANDARD_FIELDS \
  {-1, -1, "grelay_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GENERIC_RELAY, grelay_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "settling_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GENERIC_RELAY, settling_time), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "input_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GENERIC_RELAY, input_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "input_bit_offset", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GENERIC_RELAY, input_bit_offset), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "num_input_bits", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GENERIC_RELAY, num_input_bits), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "closed_input_value", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GENERIC_RELAY, closed_input_value),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "open_input_value", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GENERIC_RELAY, open_input_value),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "output_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GENERIC_RELAY, output_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "output_bit_offset", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GENERIC_RELAY, output_bit_offset), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "num_output_bits", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GENERIC_RELAY, num_output_bits), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "close_output_value", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GENERIC_RELAY, close_output_value),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "open_output_value", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GENERIC_RELAY, open_output_value),\
	{0}, NULL, MXFF_IN_DESCRIPTION}

/* Define all of the device functions. */

MX_API mx_status_type mxd_generic_relay_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_generic_relay_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_generic_relay_relay_command( MX_RELAY *relay );

MX_API mx_status_type mxd_generic_relay_get_relay_status( MX_RELAY *relay );

extern MX_RECORD_FUNCTION_LIST mxd_generic_relay_record_function_list;
extern MX_RELAY_FUNCTION_LIST mxd_generic_relay_relay_function_list;

extern long mxd_generic_relay_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_generic_relay_rfield_def_ptr;

#endif /* __D_GENERIC_RELAY_H__ */
