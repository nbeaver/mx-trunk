/*
 * Name:    d_bipolar_relay.h
 *
 * Purpose: MX header file for bipolar relay support.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2023 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_BIPOLAR_RELAY_H__
#define __D_BIPOLAR_RELAY_H__

/* Note that this driver can use either analog output records or digital
 * output records.  For digital outputs, the active output is 1 if
 * 'active_volts' >= 0.1, while the idle output is 0.
 */

#define MXF_BIPOLAR_DIGITAL_ACTIVE_THRESHOLD	0.1	/* in fake 'volts' */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *relay_a_record;
	MX_RECORD *relay_b_record;
	MX_RECORD *relay_c_record;
	MX_RECORD *relay_d_record;

	unsigned long first_active_ms;		/* in milliseconds */
	unsigned long between_time_ms;		/* in milliseconds */
	unsigned long second_active_ms;		/* in milliseconds */
	unsigned long settling_time_ms;		/* in milliseconds */
} MX_BIPOLAR_RELAY;

#define MXD_BIPOLAR_RELAY_STANDARD_FIELDS \
  {-1, -1, "relay_a_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIPOLAR_RELAY, relay_a_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "relay_b_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIPOLAR_RELAY, relay_b_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "relay_c_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIPOLAR_RELAY, relay_c_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "relay_d_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIPOLAR_RELAY, relay_d_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "first_active_ms", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIPOLAR_RELAY, first_active_ms), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "between_time_ms", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIPOLAR_RELAY, between_time_ms), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "second_active_ms", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIPOLAR_RELAY, second_active_ms), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "settling_time_ms", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIPOLAR_RELAY, settling_time_ms), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

/* Define all of the device functions. */

MX_API mx_status_type mxd_bipolar_relay_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_bipolar_relay_open( MX_RECORD *record );

MX_API mx_status_type mxd_bipolar_relay_relay_command( MX_RELAY *relay );

MX_API mx_status_type mxd_bipolar_relay_get_relay_status( MX_RELAY *relay );

extern MX_RECORD_FUNCTION_LIST mxd_bipolar_relay_record_function_list;
extern MX_RELAY_FUNCTION_LIST mxd_bipolar_relay_relay_function_list;

extern long mxd_bipolar_relay_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_bipolar_relay_rfield_def_ptr;

#endif /* __D_BIPOLAR_RELAY_H__ */
