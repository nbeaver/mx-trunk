/*
 * Name:    ph_aps_topup.h
 *
 * Purpose: Header file for the scan APS topup time to inject handler.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __PH_APS_TOPUP_H__
#define __PH_APS_TOPUP_H__

typedef struct {
	MX_RECORD *topup_time_to_inject_record;
} MX_TOPUP_MEASUREMENT_PERMIT;

MX_API mx_status_type mxph_topup_create_handler(
			MX_MEASUREMENT_PERMIT **permit_handler,
			void *permit_driver, void *scan,
			char *description );

MX_API mx_status_type mxph_topup_destroy_handler(
			MX_MEASUREMENT_PERMIT *permit_handler );

MX_API mx_status_type mxph_topup_check_for_permission(
			MX_MEASUREMENT_PERMIT *permit_handler );

extern MX_MEASUREMENT_PERMIT_FUNCTION_LIST mxph_topup_function_list;

#endif /* __PH_APS_TOPUP_H__ */

