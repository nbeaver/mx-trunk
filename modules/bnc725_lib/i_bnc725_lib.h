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
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_BNC725_LIB_H__
#define __I_BNC725_LIB_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	MX_RECORD *record;

} MX_BNC725_LIB;

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

