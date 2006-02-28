/*
 * Name:    fh_autoscale.h
 *
 * Purpose: Header file for the scan autoscaling handler.
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

#ifndef __FH_AUTOSCALE_H__
#define __FH_AUTOSCALE_H__

typedef struct {
	MX_RECORD *autoscale_scaler_record;
	MX_RECORD *autoscale_record;

	long change_request;
} MX_AUTOSCALE_MEASUREMENT_FAULT;

MX_API mx_status_type mxfh_autoscale_create_handler(
			MX_MEASUREMENT_FAULT **fault_handler,
			void *fault_driver, void *scan,
			char *description );

MX_API mx_status_type mxfh_autoscale_destroy_handler(
			MX_MEASUREMENT_FAULT *fault_handler );

MX_API mx_status_type mxfh_autoscale_check_for_fault(
			MX_MEASUREMENT_FAULT *fault_handler );

MX_API mx_status_type mxfh_autoscale_reset(
			MX_MEASUREMENT_FAULT *fault_handler );

extern MX_MEASUREMENT_FAULT_FUNCTION_LIST mxfh_autoscale_function_list;

#endif /* __FH_AUTOSCALE_H__ */
