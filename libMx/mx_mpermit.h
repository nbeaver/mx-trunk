/*
 * Name:    mx_mpermit.h
 *
 * Purpose: Header file for measurement permission handlers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_MPERMIT_H__
#define __MX_MPERMIT_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MXU_MEASUREMENT_PERMIT_DRIVER_NAME_LENGTH	32

#define MXPH_SIMPLE		1
#define MXPH_TOPUP		2

typedef struct {
	void *scan;	/* Reference to the MX_SCAN this handler is for. */

	long type;
	char *mx_typename;

	int permit_status;

	void *type_struct;
	void *function_list;
} MX_MEASUREMENT_PERMIT;

typedef struct {
	mx_status_type ( *create_handler ) ( MX_MEASUREMENT_PERMIT **, void *,
						void *, char *description );
	mx_status_type ( *destroy_handler ) ( MX_MEASUREMENT_PERMIT * );
	mx_status_type ( *check_for_permission ) ( MX_MEASUREMENT_PERMIT * );
} MX_MEASUREMENT_PERMIT_FUNCTION_LIST;

typedef struct {
	long type;
	char name[ MXU_MEASUREMENT_PERMIT_DRIVER_NAME_LENGTH + 1 ];
	MX_MEASUREMENT_PERMIT_FUNCTION_LIST *function_list;
} MX_MEASUREMENT_PERMIT_DRIVER;

MX_API mx_status_type mx_measurement_permit_get_driver_by_name(
				MX_MEASUREMENT_PERMIT_DRIVER *driver_list,
				char *name,
				MX_MEASUREMENT_PERMIT_DRIVER **driver );

MX_API mx_status_type mx_measurement_permit_create_handler(
				MX_MEASUREMENT_PERMIT **permit_handler,
				void *scan,
				char *handler_description );

MX_API mx_status_type mx_measurement_permit_destroy_handler(
				MX_MEASUREMENT_PERMIT *permit_handler );

MX_API mx_status_type mx_measurement_permit_check_for_permission(
				MX_MEASUREMENT_PERMIT *permit_handler,
				int *permit_status );

MX_API mx_status_type mx_measurement_permit_wait_for_permission(
				MX_MEASUREMENT_PERMIT *permit_handler );

/* One global variable. */

extern MX_MEASUREMENT_PERMIT_DRIVER mx_measurement_permit_driver_list[];

#ifdef __cplusplus
}
#endif

#endif /* __MX_MPERMIT_H__ */

