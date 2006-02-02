/*
 * Name:    fh_simple.h
 *
 * Purpose: Header file for the scan simple fault handler.
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

#ifndef __FH_SIMPLE_H__
#define __FH_SIMPLE_H__

typedef struct {
	MX_RECORD *fault_record;
	MX_RECORD *reset_record;

	uint32_t no_fault_value;
	uint32_t reset_value;
} MX_SIMPLE_MEASUREMENT_FAULT;

MX_API mx_status_type mxfh_simple_create_handler(
			MX_MEASUREMENT_FAULT **fault_handler,
			void *fault_driver, void *scan,
			char *description );

MX_API mx_status_type mxfh_simple_destroy_handler(
			MX_MEASUREMENT_FAULT *fault_handler );

MX_API mx_status_type mxfh_simple_check_for_fault(
			MX_MEASUREMENT_FAULT *fault_handler );

MX_API mx_status_type mxfh_simple_reset(
			MX_MEASUREMENT_FAULT *fault_handler );

extern MX_MEASUREMENT_FAULT_FUNCTION_LIST mxfh_simple_function_list;

#endif /* __FH_SIMPLE_H__ */
