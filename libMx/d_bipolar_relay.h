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

	MX_RECORD *relay_1_record;
	MX_RECORD *relay_2_record;
	MX_RECORD *relay_3_record;
	MX_RECORD *relay_4_record;

	unsigned long first_active_ms;		/* in milliseconds */
	unsigned long between_time_ms;		/* in milliseconds */
	unsigned long second_active_ms;		/* in milliseconds */
	unsigned long settling_time_ms;		/* in milliseconds */
} MX_BIPOLAR_RELAY;

#define MXD_BIPOLAR_RELAY_STANDARD_FIELDS \
  {-1, -1, "relay_1_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIPOLAR_RELAY, relay_1_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "relay_2_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIPOLAR_RELAY, relay_2_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "relay_3_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIPOLAR_RELAY, relay_3_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "relay_4_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIPOLAR_RELAY, relay_4_record), \
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

MX_API mx_status_type mxd_bipolar_relay_3reate_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_bipolar_relay_open( MX_RECORD *record );

MX_API mx_status_type mxd_bipolar_relay_relay_command( MX_RELAY *relay );

MX_API mx_status_type mxd_bipolar_relay_get_relay_status( MX_RELAY *relay );

extern MX_RECORD_FUNCTION_LIST mxd_bipolar_relay_record_function_list;
extern MX_RELAY_FUNCTION_LIST mxd_bipolar_relay_relay_function_list;

extern long mxd_bipolar_relay_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_bipolar_relay_rfield_def_ptr;

#endif /* __D_BIPOLAR_RELAY_H__ */
