/*
 * Name:     n_umx.h
 *
 * Purpose:  Header file for client interface to a UMX-based microcontroller.
 *
 * Author:   William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2019-2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __N_UMX_H__
#define __N_UMX_H__

#include "mx_socket.h"

/* ===== umx_flag values ===== */

#define MXF_UMX_SERVER_DEBUG			0x1
#define MXF_UMX_SERVER_SKIP_STARTUP_MESSAGE	0x2

/* ===== UMX server record structure ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	unsigned long umx_flags;
} MX_UMX_SERVER;

/* Define all of the client interface functions. */

MX_API mx_status_type mxn_umx_server_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxn_umx_server_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxn_umx_server_record_function_list;

extern long mxn_umx_server_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxn_umx_server_rfield_def_ptr;

#define MXN_UMX_SERVER_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_UMX_SERVER, rs232_record), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "umx_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_UMX_SERVER, umx_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __N_UMX_H__ */

