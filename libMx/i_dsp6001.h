/*
 * Name:    i_dsp6001.h
 *
 * Purpose: Header for MX driver for the DSP Technology Model 6001/6002
 *          CAMAC crate controller for PC compatible machines.
 #
 *          At present, there is no support for block mode CAMAC operations
 *          in this driver.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_DSP6001_H__
#define __I_DSP6001_H__

#include "mx_record.h"

/* Define all of the interface functions. */

MX_API mx_status_type mxi_dsp6001_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_dsp6001_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxi_dsp6001_get_lam_status( MX_CAMAC *crate, int *lam_n);
MX_API mx_status_type mxi_dsp6001_controller_command( MX_CAMAC *crate,
								int command );
MX_API mx_status_type mxi_dsp6001_camac( MX_CAMAC *crate,
		int slot, int subaddress, int function_code,
		int32_t *data, int *Q, int *X );

/* Define the data structures used by the DSP 6001/6002 interface code. */

typedef struct {
	MX_RECORD *portio_record;
	unsigned long base_address;  /* Base address of the interface card. */
} MX_DSP6001;

extern MX_RECORD_FUNCTION_LIST mxi_dsp6001_record_function_list;
extern MX_CAMAC_FUNCTION_LIST mxi_dsp6001_camac_function_list;

extern long mxi_dsp6001_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_dsp6001_rfield_def_ptr;

#define MXI_DSP6001_STANDARD_FIELDS \
  {-1, -1, "portio_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DSP6001, portio_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "base_address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DSP6001, base_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_DSP6001_H__ */

