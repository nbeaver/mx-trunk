/*
 * Name:     mx_measurement.h
 *
 * Purpose:  Header file to define functions to perform a scan measurement.
 *
 * Author:   William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2002, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_MEASUREMENT_H__
#define __MX_MEASUREMENT_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* List of measurement types. */

#define MXM_NONE		0
#define MXM_PRESET_TIME		1
#define MXM_PRESET_COUNT	2
#define MXM_PRESET_PULSE_PERIOD	3

#define MXU_MEASUREMENT_TYPE_NAME_LENGTH	20

#define MXU_MEASUREMENT_ARGUMENTS_LENGTH	80

/* Scaler and timer mode flags */

#define MXCM_UNKNOWN_MODE	0
#define MXCM_PRESET_MODE	1
#define MXCM_COUNTER_MODE	2

typedef struct {
	/* Reference to the MX_SCAN that performs this measurement. */
	void *scan;

	long type;
	char *mx_typename;
	char *measurement_arguments;
	void *measurement_type_struct;
	void *measurement_function_list;
} MX_MEASUREMENT;

typedef struct {
	mx_status_type ( *configure ) ( MX_MEASUREMENT *measurement );
	mx_status_type ( *deconfigure ) ( MX_MEASUREMENT *measurement );
	mx_status_type ( *prescan_processing) ( MX_MEASUREMENT *measurement );
	mx_status_type ( *postscan_processing) ( MX_MEASUREMENT *measurement );
	mx_status_type ( *preslice_processing) ( MX_MEASUREMENT *measurement );
	mx_status_type ( *postslice_processing) ( MX_MEASUREMENT *measurement );
	mx_status_type ( *acquire_data ) ( MX_MEASUREMENT *measurement );
} MX_MEASUREMENT_FUNCTION_LIST;

typedef struct {
	long type;
	char name[MXU_MEASUREMENT_TYPE_NAME_LENGTH + 1];
	MX_MEASUREMENT_FUNCTION_LIST *measurement_function_list;
} MX_MEASUREMENT_TYPE_ENTRY;

MX_API mx_status_type mx_get_measurement_type_by_name(
	MX_MEASUREMENT_TYPE_ENTRY *measurement_type_list,
	char *name,
	MX_MEASUREMENT_TYPE_ENTRY **measurement_type_entry );

MX_API mx_status_type mx_get_measurement_type_by_value(
	MX_MEASUREMENT_TYPE_ENTRY *measurement_type_list,
	long measurement_type,
	MX_MEASUREMENT_TYPE_ENTRY **measurement_type_entry );

MX_API mx_status_type mx_configure_measurement_type(
					MX_MEASUREMENT *measurement );

MX_API mx_status_type mx_deconfigure_measurement_type(
					MX_MEASUREMENT *measurement );

MX_API mx_status_type mx_prescan_measurement_processing(
					MX_MEASUREMENT *measurement );

MX_API mx_status_type mx_postscan_measurement_processing(
					MX_MEASUREMENT *measurement );

MX_API mx_status_type mx_preslice_measurement_processing(
					MX_MEASUREMENT *measurement );

MX_API mx_status_type mx_postslice_measurement_processing(
					MX_MEASUREMENT *measurement );

MX_API mx_status_type mx_acquire_data( MX_MEASUREMENT *measurement );

MX_API mx_status_type mx_readout_data( MX_MEASUREMENT *measurement );

MX_API mx_status_type mx_get_measurement_time( MX_MEASUREMENT *measurement,
						double *measurement_time );

MX_API mx_status_type mx_get_measurement_counts( MX_MEASUREMENT *measurement,
						long *measurement_counts );

/* One global variable. */

extern MX_MEASUREMENT_TYPE_ENTRY mx_measurement_type_list[];

#ifdef __cplusplus
}
#endif

#endif /* __MX_MEASUREMENT_H__ */

