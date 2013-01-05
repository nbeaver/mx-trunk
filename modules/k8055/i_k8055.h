/*
 * Name:    i_k8055.h
 *
 * Purpose: Header for the Velleman K8055 USB Experimenter Board
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_K8055_H__
#define __I_K8055_H__

/*----*/

#if !defined(USE_LIBK8055)
#  define USE_LIBK8055	TRUE
#endif

#if USE_LIBK8055
#  include "usb.h"
#  include "k8055.h"
#else
#  include "k8055.h"
#endif

/*----*/

typedef struct {
	MX_RECORD *record;

	unsigned long card_address;
} MX_K8055;

#define MXI_K8055_STANDARD_FIELDS \
  {-1, -1, "card_address", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_K8055, card_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/*-----*/

MX_API mx_status_type mxi_k8055_create_record_structures( MX_RECORD *record );

MX_API mx_status_type mxi_k8055_open( MX_RECORD *record );

MX_API mx_status_type mxi_k8055_close( MX_RECORD *record );

/*-----*/

extern MX_RECORD_FUNCTION_LIST mxi_k8055_record_function_list;

extern long mxi_k8055_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_k8055_rfield_def_ptr;

#endif /* __I_K8055_H__ */

