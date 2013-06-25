/*
 * Name:    i_avt_vimba.h
 *
 * Purpose: Header file for the Vimba API used by cameras from 
 *          Allied Vision Technologies.
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

#ifndef __I_AVT_VIMBA_H__
#define __I_AVT_VIMBA_H__

#include <VimbaC.h>

typedef struct {
	MX_RECORD *record;

} MX_AVT_VIMBA;

#define MXI_AVT_VIMBA_STANDARD_FIELDS

MX_API mx_status_type mxi_avt_vimba_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxi_avt_vimba_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_avt_vimba_record_function_list;

extern long mxi_avt_vimba_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_avt_vimba_rfield_def_ptr;

#endif /* __I_AVT_VIMBA_H__ */

