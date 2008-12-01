/*
 * Name:    d_marccd_server_socket.h
 *
 * Purpose: MX driver header for controlling a MarCCD area detector with
 *          the Mar-USA provided 'marccd_server_socket' program.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2007-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MARCCD_SERVER_SOCKET_H__
#define __D_MARCCD_SERVER_SOCKET_H__

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

typedef struct {
	MX_RECORD *record;

	/* The following socket is used to communicate with the 
	 * 'marccd_server_socket' program.
	 */

	MX_SOCKET *marccd_socket;

	char marccd_host[ MXU_HOSTNAME_LENGTH + 1 ];
	long marccd_port;
	char remote_filename_prefix[ MXU_FILENAME_LENGTH + 1 ]; 
	char local_filename_prefix[ MXU_FILENAME_LENGTH + 1 ];

	/* Calculated time for the most recent 'start' command to finish. */

	MX_CLOCK_TICK finish_time;

} MX_MARCCD_SERVER_SOCKET;

#define MXD_MARCCD_SERVER_SOCKET_STANDARD_FIELDS \
  {-1, -1, "marccd_host", MXFT_STRING, NULL, 1, {MXU_HOSTNAME_LENGTH + 1}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MARCCD_SERVER_SOCKET, marccd_host), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "marccd_port", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MARCCD_SERVER_SOCKET, marccd_port), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "remote_filename_prefix", \
  		MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_MARCCD_SERVER_SOCKET, remote_filename_prefix), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "local_filename_prefix", \
  		MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_MARCCD_SERVER_SOCKET, local_filename_prefix), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_marccd_server_socket_initialize_type(
							long record_type );
MX_API mx_status_type mxd_marccd_server_socket_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_marccd_server_socket_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_marccd_server_socket_open( MX_RECORD *record );
MX_API mx_status_type mxd_marccd_server_socket_close( MX_RECORD *record );
MX_API mx_status_type mxd_marccd_server_socket_resynchronize(
							MX_RECORD *record );

MX_API mx_status_type mxd_marccd_server_socket_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_server_socket_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_server_socket_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_server_socket_get_extended_status(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_server_socket_readout_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_server_socket_correct_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_server_socket_transfer_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_server_socket_get_parameter(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_server_socket_set_parameter(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_server_socket_measure_correction(
							MX_AREA_DETECTOR *ad );

MX_API mx_status_type mxd_marccd_server_socket_command(
					MX_MARCCD_SERVER_SOCKET *mss,
					char *command,
					char *response,
					size_t response_buffer_length,
					unsigned long flags );

extern MX_RECORD_FUNCTION_LIST mxd_marccd_server_socket_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_marccd_server_socket_ad_function_list;

extern long mxd_marccd_server_socket_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_marccd_server_socket_rfield_def_ptr;

#endif /* __D_MARCCD_SERVER_SOCKET_H__ */

