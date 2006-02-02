/*
 * Name:    mx_relay.h
 *
 * Purpose: MX header file for relay support.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_RELAY_H__
#define __MX_RELAY_H__

#include "mx_record.h"

#define MXF_RELAY_IS_CLOSED		1
#define MXF_RELAY_IS_OPEN		0
#define MXF_RELAY_ILLEGAL_STATUS	(-1)

#define MXF_CLOSE_RELAY		1
#define MXF_OPEN_RELAY		0

typedef struct {
	MX_RECORD *record; /* Pointer to the MX_RECORD structure that points
                            * to this relay.
                            */
	int32_t relay_command;
	int32_t relay_status;
} MX_RELAY;

#define MXLV_RLY_RELAY_COMMAND	1001
#define MXLV_RLY_RELAY_STATUS	1002

#define MX_RELAY_STANDARD_FIELDS \
  {MXLV_RLY_RELAY_COMMAND, -1, "relay_command", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RELAY, relay_command), \
	{0}, NULL, 0}, \
  \
  {MXLV_RLY_RELAY_STATUS, -1, "relay_status", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RELAY, relay_status), \
	{0}, NULL, MXFF_IN_SUMMARY}

typedef struct {
	mx_status_type ( *relay_command ) ( MX_RELAY *relay );
	mx_status_type ( *get_relay_status ) ( MX_RELAY *relay );
} MX_RELAY_FUNCTION_LIST;

MX_API mx_status_type mx_relay_command( MX_RECORD *relay_record,
						int32_t relay_command );
MX_API mx_status_type mx_get_relay_status( MX_RECORD *relay_record,
						int32_t *relay_status );

#endif /* __MX_RELAY_H__ */
