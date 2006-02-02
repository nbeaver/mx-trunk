/*
 * Name:    ph_simple.h
 *
 * Purpose: Header file for the scan simple permission handler.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __PH_SIMPLE_H__
#define __PH_SIMPLE_H__

typedef struct {
	MX_RECORD *permit_record;

	uint32_t permit_value;
} MX_SIMPLE_MEASUREMENT_PERMIT;

MX_API mx_status_type mxph_simple_create_handler(
			MX_MEASUREMENT_PERMIT **permit_handler,
			void *permit_driver, void *scan,
			char *description );

MX_API mx_status_type mxph_simple_destroy_handler(
			MX_MEASUREMENT_PERMIT *permit_handler );

MX_API mx_status_type mxph_simple_check_for_permission(
			MX_MEASUREMENT_PERMIT *permit_handler );

extern MX_MEASUREMENT_PERMIT_FUNCTION_LIST mxph_simple_function_list;

#endif /* __PH_SIMPLE_H__ */
