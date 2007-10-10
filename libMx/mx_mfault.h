/*
 * Name:    mx_mfault.h
 *
 * Purpose: Header file for measurement fault handlers.
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

#ifndef __MX_MFAULT_H__
#define __MX_MFAULT_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MXU_MEASUREMENT_FAULT_DRIVER_NAME_LENGTH	32

#define MXMF_NONE					0
#define MXMF_PREPARE_FOR_FIRST_MEASUREMENT_ATTEMPT	1

#define MXFH_SIMPLE		1
#define MXFH_AUTOSCALE		2

typedef struct {
	void *scan;	/* Reference to the MX_SCAN this handler is for. */

	long type;
	char *mx_typename;

	int fault_status;
	int reset_flags;

	void *type_struct;
	void *function_list;
} MX_MEASUREMENT_FAULT;

typedef struct {
	mx_status_type ( *create_handler ) ( MX_MEASUREMENT_FAULT **, void *,
						void *, char *description );
	mx_status_type ( *destroy_handler ) ( MX_MEASUREMENT_FAULT * );
	mx_status_type ( *check_for_fault ) ( MX_MEASUREMENT_FAULT * );
	mx_status_type ( *reset ) ( MX_MEASUREMENT_FAULT * );
} MX_MEASUREMENT_FAULT_FUNCTION_LIST;

typedef struct {
	long type;
	char name[ MXU_MEASUREMENT_FAULT_DRIVER_NAME_LENGTH + 1 ];
	MX_MEASUREMENT_FAULT_FUNCTION_LIST *function_list;
} MX_MEASUREMENT_FAULT_DRIVER;

MX_API mx_status_type mx_measurement_fault_get_driver_by_name(
				MX_MEASUREMENT_FAULT_DRIVER *driver_list,
				char *name,
				MX_MEASUREMENT_FAULT_DRIVER **driver );

MX_API mx_status_type mx_measurement_fault_create_handler(
				MX_MEASUREMENT_FAULT **fault_handler,
				void *scan,
				char *handler_description );

MX_API mx_status_type mx_measurement_fault_destroy_handler(
				MX_MEASUREMENT_FAULT *fault_handler );

MX_API mx_status_type mx_measurement_fault_check_for_fault(
				MX_MEASUREMENT_FAULT *fault_handler,
				int *fault_status );

MX_API mx_status_type mx_measurement_fault_reset(
				MX_MEASUREMENT_FAULT *fault_handler,
				int reset_flags );

/* One global variable. */

extern MX_MEASUREMENT_FAULT_DRIVER mx_measurement_fault_driver_list[];

#ifdef __cplusplus
}
#endif

#endif /* __MX_MFAULT_H__ */

