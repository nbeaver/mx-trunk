/*
 * Name:    mx_relay.h
 *
 * Purpose: MX header file for relay support.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2002, 2006-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_RELAY_H__
#define __MX_RELAY_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MXF_RELAY_IS_CLOSED		1
#define MXF_RELAY_IS_OPEN		0
#define MXF_RELAY_ILLEGAL_STATUS	(-1)

#define MXF_CLOSE_RELAY		1
#define MXF_OPEN_RELAY		0

typedef struct {
	MX_RECORD *record; /* Pointer to the MX_RECORD structure that points
                            * to this relay.
                            */
	long relay_command;
	long relay_status;

	long pulse_on_value;
	long pulse_off_value;
	double pulse_duration;		/* in seconds */
} MX_RELAY;

#define MXLV_RLY_RELAY_COMMAND	1001
#define MXLV_RLY_RELAY_STATUS	1002
#define MXLV_RLY_PULSE_ON	1003
#define MXLV_RLY_PULSE_OFF	1004
#define MXLV_RLY_PULSE_DURATION	1005

#define MX_RELAY_STANDARD_FIELDS \
  {MXLV_RLY_RELAY_COMMAND, -1, "relay_command", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RELAY, relay_command), \
	{0}, NULL, 0}, \
  \
  {MXLV_RLY_RELAY_STATUS, -1, "relay_status", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RELAY, relay_status), \
	{0}, NULL, (MXFF_IN_SUMMARY | MXFF_POLL)}, \
  \
  {MXLV_RLY_PULSE_ON, -1, "pulse_on_value", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RELAY, pulse_on_value), \
	{0}, NULL, 0}, \
  \
  {MXLV_RLY_PULSE_OFF, -1, "pulse_off_value", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RELAY, pulse_off_value), \
	{0}, NULL, 0}, \
  \
  {MXLV_RLY_PULSE_DURATION, -1, "pulse_duration", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RELAY, pulse_duration), \
	{0}, NULL, 0}

typedef struct {
	mx_status_type ( *relay_command ) ( MX_RELAY *relay );
	mx_status_type ( *get_relay_status ) ( MX_RELAY *relay );
	mx_status_type ( *pulse ) ( MX_RELAY *relay );
} MX_RELAY_FUNCTION_LIST;

MX_API mx_status_type mx_relay_command( MX_RECORD *relay_record,
						long relay_command );
MX_API mx_status_type mx_get_relay_status( MX_RECORD *relay_record,
						long *relay_status );

MX_API mx_status_type mx_relay_pulse( MX_RECORD *relay_record,
					long pulse_on_value,
					long pulse_off_value,
					double pulse_duration );

MX_API mx_status_type mx_relay_pulse_wait( MX_RECORD *relay_record,
					long pulse_on_value,
					long pulse_off_value,
					double pulse_duration,
					mx_bool_type busy_wait );

#ifdef __cplusplus
}
#endif

#endif /* __MX_RELAY_H__ */
