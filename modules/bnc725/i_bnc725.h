/*
 * Name:    i_bnc725.h
 *
 * Purpose: MX driver header for the vendor-provided Win32 C++ library
 *          for the BNC725 digital delay generator.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010-2011, 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_BNC725_H__
#define __I_BNC725_H__

#ifdef __cplusplus

/* BNC-provided headers. */

#include "BNC725.h"
#include "Channel.h"
#include "DlyDrt.h"
#include "Clock.h"
#include "Null.h"

/* Flag bits for the 'status' variable. */

#define MXF_BNC725_ENABLED		0x1

/*----*/

#define MXU_BNC725_NUM_CHANNELS		8

/*----*/

#define MXU_BNC725_PORT_NAME_LENGTH	8

#define MXU_BNC725_LOGIC_LENGTH		200

typedef struct {
	MX_RECORD *record;
	char port_name[MXU_BNC725_PORT_NAME_LENGTH+1];
	unsigned long port_speed;
	char global_logic[MXU_BNC725_LOGIC_LENGTH+1];

	mx_bool_type start;
	mx_bool_type stop;
	unsigned long status;

	CLC880 *port;
	mx_bool_type enabled;

	MX_RECORD *channel_record_array[MXU_BNC725_NUM_CHANNELS];
} MX_BNC725;

#define MXLV_BNC725_START	92501
#define MXLV_BNC725_STOP	92502
#define MXLV_BNC725_STATUS	92503

#define MXI_BNC725_STANDARD_FIELDS \
  {-1, -1, "port_name", MXFT_STRING, NULL, 1, {MXU_BNC725_PORT_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BNC725, port_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_speed", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BNC725, port_speed), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "global_logic", MXFT_STRING, NULL, 1, {MXU_BNC725_PORT_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_BNC725, global_logic), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "start", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BNC725, start), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "stop", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BNC725, stop), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "status", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BNC725, status), \
	{0}, NULL, 0}

/*--------------------------------------------*/

MX_API mx_status_type mxi_bnc725_start( MX_BNC725 *bnc725 );

MX_API mx_status_type mxi_bnc725_stop( MX_BNC725 *bnc725 );

MX_API mx_status_type mxi_bnc725_get_status( MX_BNC725 *bnc725 );

MX_API_PRIVATE mx_status_type mxip_bnc725_start_logic( MX_BNC725 *bnc725 );

#endif  /* __cplusplus */

/*--------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type mxi_bnc725_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_bnc725_open( MX_RECORD *record );
MX_API mx_status_type mxi_bnc725_close( MX_RECORD *record );
MX_API mx_status_type mxi_bnc725_special_processing_setup( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_bnc725_record_function_list;

extern long mxi_bnc725_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_bnc725_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __I_BNC725_H__ */

