/*
 * Name:   i_dante.h
 *
 * Purpose: Interface driver header for the XGLab DANTE MCA.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_DANTE_H__
#define __I_DANTE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* The following flags are used by the 'dante_flags' field. */

#define MXF_DANTE_FOO			0x1

/* Define the data structures used by the Dante driver. */

typedef struct {
	MX_RECORD *record;

	unsigned long num_mcas;
	MX_RECORD **mca_record_array;

} MX_DANTE;

#define MXI_DANTE_STANDARD_FIELDS 

MX_API mx_status_type mxi_dante_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxi_dante_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_dante_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_dante_open( MX_RECORD *record );
MX_API mx_status_type mxi_dante_special_processing_setup(
							MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_dante_record_function_list;

extern long mxi_dante_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_dante_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

/* === Driver specific functions === */

#endif /* __I_DANTE_H__ */