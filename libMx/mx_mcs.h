/*
 * Name:    mx_mcs.h
 *
 * Purpose: MX system header file for multichannel scalers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2002, 2004-2007, 2009-2010, 2014-2015, 2018-2020
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_MCS_H__
#define __MX_MCS_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MXU_MCS_EXTENDED_STATUS_STRING_LENGTH	40

typedef struct {
	MX_RECORD *record; /* Pointer to the MX_RECORD structure that points
			    * to this MCS.
			    */

	long maximum_num_scalers;
	long maximum_num_measurements;

	MX_RECORD **scaler_record_array;

	long **data_array;

	MX_RECORD *external_next_measurement_record;
	char external_next_measurement_name[ MXU_RECORD_NAME_LENGTH + 1 ];

	mx_bool_type external_next_measurement;
	unsigned long external_prescale;

	mx_bool_type manual_next_measurement;

	MX_RECORD *timer_record;
	char timer_name[ MXU_RECORD_NAME_LENGTH + 1 ];

	mx_bool_type arm;
	mx_bool_type trigger;
	mx_bool_type start;
	mx_bool_type stop;
	mx_bool_type clear;
	mx_bool_type busy;
	long last_measurement_number;
	long total_num_measurements;
	unsigned long status;
	char extended_status[ MXU_MCS_EXTENDED_STATUS_STRING_LENGTH + 1 ];

	unsigned long latched_status;

	/* The following fields are used to handle multichannel scalers
	 * that do not set the "busy" flag immediately after the start of
	 * an acquisition sequence.
	 */

	mx_bool_type busy_start_interval_enabled;
	double busy_start_interval;			/* in seconds */
	double last_start_time;				/* in seconds */

	MX_CLOCK_TICK busy_start_ticks;
	MX_CLOCK_TICK last_start_tick;

	/*---*/

	/* counting_mode is for preset time vs. preset count.
	 *
	 * trigger_mode is for internal trigger vs external trigger and others.
	 *
	 * autostart tell you whether the MCS is immediately started after
	 * it is armed.
	 */

	long counting_mode;
	long trigger_mode;
	long autostart;

	long parameter_type;

	double measurement_time;
	unsigned long measurement_counts;
	unsigned long current_num_scalers;
	unsigned long current_num_measurements;

	long measurement_number;

	long readout_preference;

	long scaler_index;
	long measurement_index;

	double dark_current;

	double *dark_current_array;	/* Expressed in counts per second. */

	long *scaler_data;
	long *measurement_data;
	long scaler_measurement;
	double *timer_data;

	mx_bool_type new_data_available;
	double clear_deadband;				/* in seconds */
	MX_CLOCK_TICK clear_deadband_ticks;
	MX_CLOCK_TICK next_clear_tick;
} MX_MCS;

/*---*/

/* Flag bits for the 'status' field. */

#define MXSF_MCS_IS_BUSY		0x1

/*---*/

#define MXF_MCS_PREFER_READ_SCALER	1
#define MXF_MCS_PREFER_READ_MEASUREMENT	2
#define MXF_MCS_PREFER_READ_ALL		3

#define MXLV_MCS_MAXIMUM_NUM_SCALERS		1001
#define MXLV_MCS_MAXIMUM_NUM_MEASUREMENTS	1002
#define MXLV_MCS_DATA_ARRAY			1003
#define MXLV_MCS_EXTERNAL_NEXT_MEASUREMENT	1004
#define MXLV_MCS_EXTERNAL_PRESCALE		1005
#define MXLV_MCS_MANUAL_NEXT_MEASUREMENT	1006
#define MXLV_MCS_TIMER_RECORD_NAME		1007
#define MXLV_MCS_ARM				1008
#define MXLV_MCS_TRIGGER			1009
#define MXLV_MCS_START				1010
#define MXLV_MCS_STOP				1011
#define MXLV_MCS_CLEAR				1012
#define MXLV_MCS_BUSY				1013
#define MXLV_MCS_LAST_MEASUREMENT_NUMBER	1014
#define MXLV_MCS_TOTAL_NUM_MEASUREMENTS		1015
#define MXLV_MCS_STATUS				1016
#define MXLV_MCS_EXTENDED_STATUS		1017
#define MXLV_MCS_LATCHED_STATUS			1018
#define MXLV_MCS_BUSY_START_INTERVAL_ENABLED	1019
#define MXLV_MCS_BUSY_START_INTERVAL		1020
#define MXLV_MCS_LAST_START_TIME		1021
#define MXLV_MCS_COUNTING_MODE			1022
#define MXLV_MCS_TRIGGER_MODE			1023
#define MXLV_MCS_AUTOSTART			1024
#define MXLV_MCS_MEASUREMENT_TIME		1025
#define MXLV_MCS_MEASUREMENT_COUNTS		1026
#define MXLV_MCS_CURRENT_NUM_SCALERS		1027
#define MXLV_MCS_CURRENT_NUM_MEASUREMENTS	1028
#define MXLV_MCS_MEASUREMENT_NUMBER		1029
#define MXLV_MCS_READOUT_PREFERENCE		1030
#define MXLV_MCS_SCALER_INDEX			1031
#define MXLV_MCS_MEASUREMENT_INDEX		1032
#define MXLV_MCS_DARK_CURRENT			1033
#define MXLV_MCS_DARK_CURRENT_ARRAY		1034
#define MXLV_MCS_SCALER_DATA			1035
#define MXLV_MCS_MEASUREMENT_DATA		1036
#define MXLV_MCS_SCALER_MEASUREMENT		1037
#define MXLV_MCS_TIMER_DATA			1038
#define MXLV_MCS_CLEAR_DEADBAND			1039

#define MX_MCS_STANDARD_FIELDS \
  {MXLV_MCS_MAXIMUM_NUM_SCALERS, -1, "maximum_num_scalers",\
					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, maximum_num_scalers), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_MCS_MAXIMUM_NUM_MEASUREMENTS, -1, "maximum_num_measurements", \
					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, maximum_num_measurements), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_MCS_DATA_ARRAY, -1, "data_array", \
		MXFT_ULONG, NULL, 2, {MXU_VARARGS_LENGTH, MXU_VARARGS_LENGTH},\
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, data_array), \
	{sizeof(unsigned long), sizeof(unsigned long *)}, NULL, MXFF_VARARGS},\
  \
  {-1, -1, "external_next_measurement_name", \
	  	MXFT_STRING, NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, external_next_measurement_name),\
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_MCS_EXTERNAL_NEXT_MEASUREMENT, -1, "external_next_measurement", \
					  	MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, external_next_measurement), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_MCS_EXTERNAL_NEXT_MEASUREMENT, -1, "external_channel_advance", \
					  	MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, external_next_measurement), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_EXTERNAL_PRESCALE, -1, "external_prescale", \
	  					MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, external_prescale), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_MCS_MANUAL_NEXT_MEASUREMENT, -1, "manual_next_measurement", \
					  	MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, manual_next_measurement), \
	{0}, NULL, 0 }, \
  \
  {MXLV_MCS_TIMER_RECORD_NAME, -1, "timer_name", \
			MXFT_STRING, NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, timer_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_MCS_ARM, -1, "arm", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, arm), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_TRIGGER, -1, "trigger", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, trigger), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_START, -1, "start", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, start), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_STOP, -1, "stop", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, stop), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_CLEAR, -1, "clear", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, clear), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_BUSY, -1, "busy", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, busy), \
	{0}, NULL, MXFF_POLL}, \
  \
  {MXLV_MCS_LAST_MEASUREMENT_NUMBER, -1, "last_measurement_number", \
		MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, last_measurement_number), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_TOTAL_NUM_MEASUREMENTS, -1, "total_num_measurements", \
		MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, total_num_measurements), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_STATUS, -1, "status", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, status), \
	{0}, NULL, MXFF_POLL}, \
  \
  {MXLV_MCS_EXTENDED_STATUS, -1, "extended_status", MXFT_STRING, \
			NULL, 1, {MXU_MCS_EXTENDED_STATUS_STRING_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, extended_status), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_MCS_LATCHED_STATUS, -1, "latched_status", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, latched_status), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_BUSY_START_INTERVAL_ENABLED, -1, "busy_start_interval_enabled", \
	  				MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, busy_start_interval_enabled), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_BUSY_START_INTERVAL, -1, "busy_start_interval", \
	  				MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, busy_start_interval), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_LAST_START_TIME, -1, "last_start_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, last_start_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_COUNTING_MODE, -1, "counting_mode", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, counting_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_TRIGGER_MODE, -1, "trigger_mode", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, trigger_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_AUTOSTART, -1, "autostart", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, autostart), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "parameter_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, parameter_type), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_MEASUREMENT_TIME, -1, "measurement_time", MXFT_DOUBLE, NULL,0,{0},\
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, measurement_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_MEASUREMENT_COUNTS, -1, "measurement_counts", \
					MXFT_ULONG, NULL,0,{0},\
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, measurement_counts), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_CURRENT_NUM_SCALERS, -1, "current_num_scalers", \
						MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, current_num_scalers), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_CURRENT_NUM_MEASUREMENTS, -1, "current_num_measurements", \
						MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, current_num_measurements), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_MEASUREMENT_NUMBER, -1, "measurement_number", \
						MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, measurement_number), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_MCS_READOUT_PREFERENCE, -1, "readout_preference", \
						MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, readout_preference), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_SCALER_INDEX, -1, "scaler_index", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, scaler_index), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_MEASUREMENT_INDEX, -1, "measurement_index", \
					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, measurement_index), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_DARK_CURRENT, -1, "dark_current", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, dark_current), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_DARK_CURRENT_ARRAY, -1, "dark_current_array", MXFT_DOUBLE, \
					NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, dark_current_array), \
	{sizeof(double)}, NULL, MXFF_VARARGS}, \
  \
  {MXLV_MCS_SCALER_DATA, -1, "scaler_data", \
			MXFT_LONG, NULL, 1, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, scaler_data), \
	{sizeof(long)}, NULL, MXFF_VARARGS}, \
  \
  {MXLV_MCS_MEASUREMENT_DATA, -1, "measurement_data", \
			MXFT_LONG, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, measurement_data), \
	{sizeof(long)}, NULL, MXFF_VARARGS}, \
  \
  {MXLV_MCS_SCALER_MEASUREMENT, -1, "scaler_measurement", \
			MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, scaler_measurement), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCS_TIMER_DATA, -1, "timer_data", \
		MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, timer_data), \
	{sizeof(double)}, NULL, MXFF_VARARGS}, \
  \
  {MXLV_MCS_CLEAR_DEADBAND, -1, "clear_deadband", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCS, clear_deadband), \
	{0}, NULL, 0}

/* Note that the 'scaler_data' field is special, since it has the MXFF_VARARGS
 * flag set, but the length of the first dimension is 0.  This means that
 * MX will dereference 'field->data_pointer' through the function
 * mx_read_void_pointer_from_memory_location(), but no memory will actually
 * be allocated for the field.
 */

typedef struct {
	mx_status_type ( *arm ) ( MX_MCS *mcs );
	mx_status_type ( *trigger ) ( MX_MCS *mcs );
	mx_status_type ( *stop ) ( MX_MCS *mcs );
	mx_status_type ( *clear ) ( MX_MCS *mcs );
	mx_status_type ( *busy ) ( MX_MCS *mcs );
	mx_status_type ( *get_status ) ( MX_MCS *mcs );
	mx_status_type ( *read_all ) ( MX_MCS *mcs );
	mx_status_type ( *read_scaler ) ( MX_MCS *mcs );
	mx_status_type ( *read_measurement ) ( MX_MCS *mcs );
	mx_status_type ( *read_scaler_measurement ) ( MX_MCS *mcs );
	mx_status_type ( *read_timer ) ( MX_MCS * );
	mx_status_type ( *get_parameter ) ( MX_MCS *mcs );
	mx_status_type ( *set_parameter ) ( MX_MCS *mcs );
	mx_status_type ( *get_last_measurement_number ) ( MX_MCS *mcs );
	mx_status_type ( *get_total_num_measurements ) ( MX_MCS *mcs );
	mx_status_type ( *get_extended_status ) ( MX_MCS *mcs );
} MX_MCS_FUNCTION_LIST;

MX_API mx_status_type mx_mcs_get_pointers( MX_RECORD *mcs_record,
	MX_MCS **mcs, MX_MCS_FUNCTION_LIST **function_list_ptr,
	const char *calling_fname );

MX_API mx_status_type mx_mcs_initialize_driver( MX_DRIVER *driver,
				long *maximum_num_scalers_varargs_cookie,
				long *maximum_num_measurements_varargs_cookie );

MX_API mx_status_type mx_mcs_finish_record_initialization(
					MX_RECORD *mcs_record );

MX_API mx_status_type mx_mcs_arm( MX_RECORD *mcs_record );

MX_API mx_status_type mx_mcs_trigger( MX_RECORD *mcs_record );

MX_API mx_status_type mx_mcs_start( MX_RECORD *mcs_record );

MX_API mx_status_type mx_mcs_stop( MX_RECORD *mcs_record );

MX_API mx_status_type mx_mcs_clear( MX_RECORD *mcs_record );

MX_API mx_status_type mx_mcs_is_busy( MX_RECORD *mcs_record,
					mx_bool_type *busy );

MX_API mx_status_type mx_mcs_get_last_measurement_number( MX_RECORD *mcs_record,
					long *last_measurement_number );

MX_API mx_status_type mx_mcs_get_total_num_measurements( MX_RECORD *mcs_record,
						long *total_num_measurements );

MX_API mx_status_type mx_mcs_get_status( MX_RECORD *mcs_record,
					unsigned long *mcs_status );

MX_API mx_status_type mx_mcs_get_extended_status( MX_RECORD *mcs_record,
					long *last_measurement_number,
					long *total_num_measurements,
					unsigned long *mcs_status );

MX_API mx_status_type mx_mcs_set_busy_start_interval( MX_RECORD *mcs_record,
					double busy_start_interval_in_seconds );

MX_API mx_status_type mx_mcs_check_busy_start_interval( MX_RECORD *mcs_record,
					mx_bool_type *busy_start_set );

/*---*/

MX_API mx_status_type mx_mcs_get_current_num_measurements(
					MX_RECORD *mcs_record,
					long *current_num_measurements );

MX_API mx_status_type mx_mcs_read_all( MX_RECORD *mcs_record,
					unsigned long *num_scalers,
					unsigned long *num_measurements,
					long ***mcs_data );

MX_API mx_status_type mx_mcs_read_scaler( MX_RECORD *mcs_record,
					unsigned long scaler_index,
					unsigned long *num_measurements,
					long **scaler_data );

MX_API mx_status_type mx_mcs_read_measurement( MX_RECORD *mcs_record,
					unsigned long measurement_index,
					unsigned long *num_scalers,
					long **measurement_data );

MX_API mx_status_type mx_mcs_read_scaler_measurement( MX_RECORD *mcs_record,
					unsigned long scaler_index,
					unsigned long measurement_index,
					long *scaler_measurement );

MX_API mx_status_type mx_mcs_read_timer( MX_RECORD *mcs_record,
					unsigned long *num_measurements,
					double **timer_data );

MX_API mx_status_type mx_mcs_get_counting_mode( MX_RECORD *mcs_record,
							long *counting_mode );

MX_API mx_status_type mx_mcs_set_counting_mode( MX_RECORD *mcs_record,
							long counting_mode );

MX_API mx_status_type mx_mcs_get_trigger_mode( MX_RECORD *mcs_record,
							long *trigger_mode );

MX_API mx_status_type mx_mcs_set_trigger_mode( MX_RECORD *mcs_record,
							long trigger_mode );

MX_API mx_status_type mx_mcs_get_autostart( MX_RECORD *mcs_record,
							long *autostart );

MX_API mx_status_type mx_mcs_set_autostart( MX_RECORD *mcs_record,
							long autostart );

MX_API mx_status_type mx_mcs_get_external_next_measurement(
					MX_RECORD *mcs_record,
					mx_bool_type *external_next_measurement);

MX_API mx_status_type mx_mcs_set_external_next_measurement(
					MX_RECORD *mcs_record,
					mx_bool_type external_next_measurement );

MX_API mx_status_type mx_mcs_get_external_prescale( MX_RECORD *mcs_record,
					unsigned long *external_prescale );

MX_API mx_status_type mx_mcs_set_external_prescale( MX_RECORD *mcs_record,
					unsigned long external_prescale );

MX_API mx_status_type mx_mcs_manual_next_measurement( MX_RECORD *mcs_record );

MX_API mx_status_type mx_mcs_get_measurement_time( MX_RECORD *mcs_record,
					double *measurement_time );
MX_API mx_status_type mx_mcs_set_measurement_time( MX_RECORD *mcs_record,
					double measurement_time );

MX_API mx_status_type mx_mcs_get_measurement_counts( MX_RECORD *mcs_record,
					unsigned long *measurement_counts );
MX_API mx_status_type mx_mcs_set_measurement_counts( MX_RECORD *mcs_record,
					unsigned long measurement_counts );

MX_API mx_status_type mx_mcs_get_num_measurements( MX_RECORD *mcs_record,
					unsigned long *num_measurements );
MX_API mx_status_type mx_mcs_set_num_measurements( MX_RECORD *mcs_record,
					unsigned long num_measurements );

MX_API mx_status_type mx_mcs_get_measurement_number( MX_RECORD *mcs_record,
					long *measurement_number );

MX_API mx_status_type mx_mcs_get_dark_current_array( MX_RECORD *mcs_record,
					long num_scalers,
					double *dark_current_array );

MX_API mx_status_type mx_mcs_set_dark_current_array( MX_RECORD *mcs_record,
					unsigned long num_scalers,
					double *dark_current_array );

MX_API mx_status_type mx_mcs_get_dark_current( MX_RECORD *mcs_record,
					unsigned long scaler_index,
					double *dark_current );

MX_API mx_status_type mx_mcs_set_dark_current( MX_RECORD *mcs_record,
					unsigned long scaler_index,
					double dark_current );

MX_API mx_status_type mx_mcs_get_parameter( MX_RECORD *mcs_record,
						long parameter_type );

MX_API mx_status_type mx_mcs_set_parameter( MX_RECORD *mcs_record,
						long parameter_type );

MX_API mx_status_type mx_mcs_default_get_parameter_handler( MX_MCS *mcs );

MX_API mx_status_type mx_mcs_default_set_parameter_handler( MX_MCS *mcs );

#ifdef __cplusplus
}
#endif

#endif /* __MX_MCS_H__ */
