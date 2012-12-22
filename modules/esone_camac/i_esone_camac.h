/*
 * Name:    i_esone_camac.h
 *
 * Purpose: Header for MX driver for CAMAC access via an ESONE CAMAC library.
 *
 *          At present, there is no support for block mode CAMAC operations
 *          in this driver.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2004, 2006, 2008, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_ESONE_CAMAC_H__
#define __I_ESONE_CAMAC_H__

#include "mx_record.h"

/* Define all of the interface functions. */

MX_API mx_status_type mxi_esone_camac_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxi_esone_camac_get_lam_status( MX_CAMAC *crate,
							long *lam_status);

MX_API mx_status_type mxi_esone_camac_controller_command( MX_CAMAC *crate,
							long command );

MX_API mx_status_type mxi_esone_camac( MX_CAMAC *crate,
		long slot, long subaddress, long function_code,
		int32_t *data, int *Q, int *X );

/* Define the data structures used by the ESONE CAMAC interface code. */

typedef struct {
	MX_RECORD *record;
} MX_ESONE_CAMAC;

extern MX_RECORD_FUNCTION_LIST mxi_esone_camac_record_function_list;
extern MX_CAMAC_FUNCTION_LIST mxi_esone_camac_camac_function_list;

extern long mxi_esone_camac_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_esone_camac_rfield_def_ptr;

#define MXI_ESONE_CAMAC_STANDARD_FIELDS

#endif /* __I_ESONE_CAMAC_H__ */

