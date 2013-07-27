/*
 * Name:    i_bnc725_lib.h
 *
 * Purpose: MX driver header for the vendor-provided Win32 C++ library
 *          for the BNC725 digital delay generator.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_BNC725_LIB_H__
#define __I_BNC725_LIB_H__

#ifdef __cplusplus

/* BNC-provided headers. */

#include "BNC725.h"
#include "Channel.h"
#include "DlyDrt.h"
#include "Clock.h"
#include "Null.h"

/*----*/

#define MXU_BNC725_PORT_NAME_LENGTH	8

typedef struct {
	MX_RECORD *record;
	char port_name[MXU_BNC725_PORT_NAME_LENGTH+1];
	unsigned long port_speed;

	CLC880 *port;

} MX_BNC725_LIB;

#define MXI_BNC725_LIB_STANDARD_FIELDS \
  {-1, -1, "port_name", MXFT_STRING, NULL, 1, {MXU_BNC725_PORT_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BNC725_LIB, port_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_speed", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BNC725_LIB, port_speed), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif  /* __cplusplus */

/*--------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type mxi_bnc725_lib_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_bnc725_lib_open( MX_RECORD *record );
MX_API mx_status_type mxi_bnc725_lib_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_bnc725_lib_record_function_list;

extern long mxi_bnc725_lib_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_bnc725_lib_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __I_BNC725_LIB_H__ */

