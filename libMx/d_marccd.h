/*
 * Name:    d_marccd.h
 *
 * Purpose: MX driver header for MarCCD remote control mode.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MARCCD_H__
#define __D_MARCCD_H__

/* MarCCD currently executing command. */

#define MXF_MARCCD_CMD_UNKNOWN		(-1)
#define MXF_MARCCD_CMD_NONE		0
#define MXF_MARCCD_CMD_START		1
#define MXF_MARCCD_CMD_READOUT		2
#define MXF_MARCCD_CMD_DEZINGER		3
#define MXF_MARCCD_CMD_CORRECT		4
#define MXF_MARCCD_CMD_WRITEFILE	5
#define MXF_MARCCD_CMD_ABORT		6
#define MXF_MARCCD_CMD_HEADER		7
#define MXF_MARCCD_CMD_SHUTTER		8
#define MXF_MARCCD_CMD_GET_STATE	9
#define MXF_MARCCD_CMD_SET_STATE	10
#define MXF_MARCCD_CMD_GET_BIN		11
#define MXF_MARCCD_CMD_SET_BIN		12
#define MXF_MARCCD_CMD_GET_SIZE		13
#define MXF_MARCCD_CMD_PHI		14
#define MXF_MARCCD_CMD_DISTANCE		15
#define MXF_MARCCD_CMD_END_AUTOMATION	16

/* MarCCD current state. */

#define MXF_MARCCD_STATE_UNKNOWN	(-1)
#define MXF_MARCCD_STATE_IDLE		0
#define MXF_MARCCD_STATE_ACQUIRE	1
#define MXF_MARCCD_STATE_READOUT	2
#define MXF_MARCCD_STATE_CORRECT	3
#define MXF_MARCCD_STATE_WRITING	4
#define MXF_MARCCD_STATE_ABORTING	5
#define MXF_MARCCD_STATE_UNAVAILABLE	6
#define MXF_MARCCD_STATE_ERROR		7
#define MXF_MARCCD_STATE_BUSY		8

/* Flag values for the command routines below. */

#define MXF_MARCCD_FORCE_READ	0x2

typedef struct {
	MX_RECORD *record;

	/* If we were started directly by MarCCD, the following two file
	 * descriptors are used to talk to MarCCD.
	 */

	int fd_from_marccd;
	int fd_to_marccd;

	/* If we are talking to the Mar provided remote server, then the
	 * following socket is used to talk to it.
	 */

	MX_SOCKET *marccd_socket;

	char marccd_host[ MXU_HOSTNAME_LENGTH + 1 ];
	long marccd_port;

	/* The command that currently should be executing. */

	int current_command;

	/* The MarCCD state value from the last 'is_state' message. */

	int current_state;

	/* The MarCCD state value we expect to see in the next 'is_state'
	 * message.
	 */

	int next_state;

	/* Calculated time for the most recent 'start' command to finish. */

	MX_CLOCK_TICK finish_time;
	int use_finish_time;
} MX_MARCCD;

#define MXD_MARCCD_STANDARD_FIELDS \
  {-1, -1, "marccd_host", MXFT_STRING, NULL, 1, {MXU_HOSTNAME_LENGTH + 1}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MARCCD, marccd_host), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "marccd_port", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MARCCD, marccd_port), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_marccd_initialize_type( long record_type );
MX_API mx_status_type mxd_marccd_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_marccd_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_marccd_open( MX_RECORD *record );
MX_API mx_status_type mxd_marccd_close( MX_RECORD *record );

MX_API mx_status_type mxd_marccd_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_get_extended_status( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_correct_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_set_parameter( MX_AREA_DETECTOR *ad );

MX_API mx_status_type mxd_marccd_command( MX_AREA_DETECTOR *ad,
					MX_MARCCD *marccd,
					char *command,
					unsigned long flags );

MX_API mx_status_type mxd_marccd_check_for_responses( MX_AREA_DETECTOR *ad,
							MX_MARCCD *marccd,
							unsigned long flags );

extern MX_RECORD_FUNCTION_LIST mxd_marccd_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_marccd_ad_function_list;

extern long mxd_marccd_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_marccd_rfield_def_ptr;

#endif /* __D_MARCCD_H__ */

