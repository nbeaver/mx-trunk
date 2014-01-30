/*
 * Name:    MX_WAVEFORM_INPUT.h
 *
 * Purpose: MX system header file for waveform input devices.
 *          waveform generators.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __mx_waveform_input_H__
#define __mx_waveform_input_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#include "mx_namefix.h"

/* Definitions for 'trigger_repeat'. */

#define MXF_WVI_ONE_SHOT	0
#define MXF_WVI_FOREVER		(-1)

/*---*/

typedef struct {
	MX_RECORD *record; /* Pointer to the MX_RECORD structure that points
			    * to this waveform input device.
			    */

	long maximum_num_channels;
	long maximum_num_steps;

	double **data_array;

	long current_num_channels;
	long current_num_steps;

	unsigned long channel_index;

	/* 'channel_data' does not have a data array of its own.  Instead,
	 * it points into the appropriate row of 'data_array'.
	 */

	double *channel_data;

	double scale;
	double offset;
	char units[MXU_UNITS_NAME_LENGTH+1];

	long parameter_type;

	double frequency;		/* in steps/second */
	long trigger_mode;
	long trigger_repeat;

	mx_bool_type arm;
	mx_bool_type trigger;
	mx_bool_type stop;
	mx_bool_type busy;

} MX_WAVEFORM_INPUT;

#define MXLV_WVI_CHANNEL_DATA	26501
#define MXLV_WVI_CHANNEL_INDEX	26502
#define MXLV_WVI_FREQUENCY	26503
#define MXLV_WVI_TRIGGER_MODE	26504
#define MXLV_WVI_TRIGGER_REPEAT	26505
#define MXLV_WVI_ARM		26506
#define MXLV_WVI_TRIGGER	26507
#define MXLV_WVI_STOP		26508
#define MXLV_WVI_BUSY		26509

#define MX_WAVEFORM_INPUT_STANDARD_FIELDS \
  {-1, -1, "maximum_num_channels", MXFT_LONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_WAVEFORM_INPUT, maximum_num_channels), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "maximum_num_steps", MXFT_LONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_INPUT, maximum_num_steps),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "data_array", MXFT_DOUBLE, \
			NULL, 2, {MXU_VARARGS_LENGTH, MXU_VARARGS_LENGTH}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_INPUT, data_array), \
	{sizeof(double), sizeof(double *)}, NULL, MXFF_VARARGS}, \
  \
  {-1, -1, "current_num_channels", MXFT_LONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_WAVEFORM_INPUT, current_num_channels), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "current_num_steps", MXFT_LONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_INPUT, current_num_steps),\
	{0}, NULL, 0}, \
  \
  {MXLV_WVI_CHANNEL_INDEX, -1, "channel_index", MXFT_ULONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_INPUT, channel_index),\
	{0}, NULL, 0}, \
  \
  {MXLV_WVI_CHANNEL_DATA, -1, "channel_data", MXFT_DOUBLE, NULL, 1, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_INPUT, channel_data),\
	{sizeof(double)}, NULL, MXFF_VARARGS}, \
  \
  {-1, -1, "scale", MXFT_DOUBLE, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_INPUT, scale), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "offset", MXFT_DOUBLE, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_INPUT, offset), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "units", MXFT_STRING, NULL, 1, {MXU_UNITS_NAME_LENGTH}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_INPUT, units), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_WVI_FREQUENCY, -1, "frequency", MXFT_DOUBLE, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_INPUT, frequency), \
	{0}, NULL, 0}, \
  \
  {MXLV_WVI_TRIGGER_MODE, -1, "trigger_mode", MXFT_LONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_INPUT, trigger_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_WVI_TRIGGER_REPEAT, -1, "trigger_repeat", MXFT_LONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_INPUT, trigger_repeat), \
	{0}, NULL, 0}, \
  \
  {MXLV_WVI_ARM, -1, "arm", MXFT_BOOL, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_INPUT, arm), \
	{0}, NULL, 0}, \
  \
  {MXLV_WVI_TRIGGER, -1, "trigger", MXFT_BOOL, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_INPUT, trigger), \
	{0}, NULL, 0}, \
  \
  {MXLV_WVI_STOP, -1, "stop", MXFT_BOOL, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_INPUT, stop), \
	{0}, NULL, 0}, \
  \
  {MXLV_WVI_BUSY, -1, "busy", MXFT_BOOL, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_INPUT, busy), \
	{0}, NULL, 0}

/* Note that the 'channel_data' field is special, since it has the MXFF_VARARGS
 * flag set, but the length of the first dimension is 0.  This means that
 * MX will dereference 'field->data_pointer' through the function
 * mx_read_void_pointer_from_memory_location(), but not memory will actually
 * be allocated for the field.
 */

typedef struct {
	mx_status_type ( *arm ) ( MX_WAVEFORM_INPUT *wvin );
	mx_status_type ( *trigger ) ( MX_WAVEFORM_INPUT *wvin );
	mx_status_type ( *stop ) ( MX_WAVEFORM_INPUT *wvin );
	mx_status_type ( *busy ) ( MX_WAVEFORM_INPUT *wvin );
	mx_status_type ( *read_all ) ( MX_WAVEFORM_INPUT *wvin );
	mx_status_type ( *read_channel ) ( MX_WAVEFORM_INPUT *wvin );
	mx_status_type ( *get_parameter ) ( MX_WAVEFORM_INPUT *wvin );
	mx_status_type ( *set_parameter ) ( MX_WAVEFORM_INPUT *wvin );
} MX_WAVEFORM_INPUT_FUNCTION_LIST;

MX_API mx_status_type mx_waveform_input_get_pointers(
		MX_RECORD *wvin_record,
		MX_WAVEFORM_INPUT **wvin,
		MX_WAVEFORM_INPUT_FUNCTION_LIST **function_list_ptr,
		const char *calling_fname );

MX_API mx_status_type mx_waveform_input_initialize_driver( MX_DRIVER *driver,
				long *maximum_num_channels_varargs_cookie,
				long *maximum_num_steps_varargs_cookie );

MX_API mx_status_type mx_waveform_input_finish_record_initialization(
					MX_RECORD *wvin_record );

MX_API mx_status_type mx_waveform_input_arm( MX_RECORD *wvin_record );

MX_API mx_status_type mx_waveform_input_trigger( MX_RECORD *wvin_record );

MX_API mx_status_type mx_waveform_input_start( MX_RECORD *wvin_record );

MX_API mx_status_type mx_waveform_input_stop( MX_RECORD *wvin_record );

MX_API mx_status_type mx_waveform_input_is_busy( MX_RECORD *wvin_record,
							mx_bool_type *busy );

MX_API mx_status_type mx_waveform_input_read_all( MX_RECORD *wvin_record,
						unsigned long *num_channels,
						unsigned long *num_steps,
						double ***wvin_data );

MX_API mx_status_type mx_waveform_input_write_all( MX_RECORD *wvin_record,
						unsigned long num_channels,
						unsigned long num_steps,
						double **wvin_data );

MX_API mx_status_type mx_waveform_input_read_channel( MX_RECORD *wvin_record,
						unsigned long channel_index,
						unsigned long *num_steps,
						double **channel_data );

MX_API mx_status_type mx_waveform_input_write_channel( MX_RECORD *wvin_record,
						unsigned long channel_index,
						unsigned long num_steps,
						double *channel_data );

MX_API mx_status_type mx_waveform_input_get_frequency( MX_RECORD *wvin_record,
						double *frequency );

MX_API mx_status_type mx_waveform_input_set_frequency( MX_RECORD *wvin_record,
						double frequency );

MX_API mx_status_type mx_waveform_input_get_trigger_mode(
						MX_RECORD *wvin_record,
						long *trigger_mode );

MX_API mx_status_type mx_waveform_input_set_trigger_mode(
						MX_RECORD *wvin_record,
						long trigger_mode );

MX_API mx_status_type mx_waveform_input_get_trigger_repeat(
						MX_RECORD *wvin_record,
						long *trigger_repeat );

MX_API mx_status_type mx_waveform_input_set_trigger_repeat(
						MX_RECORD *wvin_record,
						long trigger_repeat );

/*----*/

MX_API mx_status_type mx_waveform_input_default_get_parameter_handler(
						MX_WAVEFORM_INPUT *wvin );

MX_API mx_status_type mx_waveform_input_default_set_parameter_handler(
						MX_WAVEFORM_INPUT *wvin );

#ifdef __cplusplus
}
#endif

#endif /* __mx_waveform_input_H__ */

