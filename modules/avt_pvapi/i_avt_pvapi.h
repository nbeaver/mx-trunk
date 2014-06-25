/*
 * Name:    i_avt_pvapi.h
 *
 * Purpose: Header file for the PvAPI API used by cameras from 
 *          Allied Vision Technologies.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013-2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_AVT_PVAPI_H__
#define __I_AVT_PVAPI_H__

/* Make definitions needed by PvApi.h */

#if defined(__x86_64__)
#  define _x64
#elif defined(__i386__)
#  define _x86
#endif

#include <PvApi.h>
#include <PvRegIo.h>

typedef struct {
	MX_RECORD *record;

	unsigned long avt_pvapi_flags;
	unsigned long pvapi_version;
} MX_AVT_PVAPI;

#define MXI_AVT_PVAPI_STANDARD_FIELDS \
  {-1, -1, "avt_pvapi_flags", MXFT_HEX, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_AVT_PVAPI, avt_pvapi_flags), \
        {0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "pvapi_version", MXFT_ULONG, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_AVT_PVAPI, pvapi_version), \
        {0}, NULL, 0 }

MX_API mx_status_type mxi_avt_pvapi_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxi_avt_pvapi_open( MX_RECORD *record );
MX_API mx_status_type mxi_avt_pvapi_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_avt_pvapi_record_function_list;

extern long mxi_avt_pvapi_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_avt_pvapi_rfield_def_ptr;

#endif /* __I_AVT_PVAPI_H__ */

