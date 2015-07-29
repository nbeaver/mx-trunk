/*
 * Name:    m_none.h
 *
 * Purpose: Header file for dummy measurements during scans.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __M_NONE_H__
#define __M_NONE_H__

typedef struct {
	int placeholder;
} MX_MEASUREMENT_NONE;

MX_API mx_status_type mxm_none_configure( MX_MEASUREMENT *measurement );
MX_API mx_status_type mxm_none_deconfigure( MX_MEASUREMENT *measurement );
MX_API mx_status_type mxm_none_acquire_data( MX_MEASUREMENT *measurement );

extern MX_MEASUREMENT_FUNCTION_LIST mxm_none_function_list;

#endif /* __M_NONE_H__ */

