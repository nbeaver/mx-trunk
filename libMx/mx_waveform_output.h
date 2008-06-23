/*
 * Name:    mx_waveform_output.h
 *
 * Purpose: MX system header file for waveform output devices such as
 *          waveform generators.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_WAVEFORM_OUTPUT_H__
#define __MX_WAVEFORM_OUTPUT_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	MX_RECORD *record; /* Pointer to the MX_RECORD structure that points
			    * to this waveform output device.
			    */

	long maximum_num_channels;
	long maximum_num_points;

	double **data_array;

	double *timer_data;

	long current_num_channels;
	long current_num_points;

	unsigned long channel_index;

	/* 'channel_data' does not have a data array of its own.  Instead,
	 * it points into the appropriate row of 'data_array'.
	 */

	double *channel_data;

	double scale;
	double offset;
	char units[MXU_UNITS_NAME_LENGTH+1];

	mx_bool_type busy;

	long parameter_type;

} MX_WAVEFORM_OUTPUT;

#define MXLV_WVO_DATA_ARRAY	23001
#define MXLV_WVO_TIMER_DATA	23002
#define MXLV_WVO_BUSY		23003

#define MX_WAVEFORM_OUTPUT_STANDARD_FIELDS \
  {-1, -1, "maximum_num_channels", MXFT_LONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_WAVEFORM_OUTPUT, maximum_num_channels), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "maximum_num_points", MXFT_LONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_OUTPUT, maximum_num_points),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_WVO_DATA_ARRAY, -1, "data_array", MXFT_DOUBLE, \
			NULL, 2, {MXU_VARARGS_LENGTH, MXU_VARARGS_LENGTH}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_OUTPUT, data_array), \
	{sizeof(double), sizeof(double *)}, NULL, MXFF_VARARGS}, \
  \
  {MXLV_WVO_TIMER_DATA, -1, "timer_data", MXFT_DOUBLE, \
  			NULL, 1, {MXU_VARARGS_LENGTH}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_OUTPUT, timer_data),\
	{sizeof(double)}, NULL, MXFF_VARARGS}, \
  \
  {-1, -1, "current_num_channels", MXFT_LONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_WAVEFORM_OUTPUT, current_num_channels), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "current_num_points", MXFT_LONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_OUTPUT, current_num_points),\
	{0}, NULL, 0}, \
  \
  {-1, -1, "channel_index", MXFT_ULONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_OUTPUT, channel_index),\
	{0}, NULL, 0}, \
  \
  {-1, -1, "channel_data", MXFT_DOUBLE, NULL, 1, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_OUTPUT, channel_data),\
	{sizeof(double)}, NULL, 0}, \
  \
  {-1, -1, "scale", MXFT_DOUBLE, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_OUTPUT, scale), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "offset", MXFT_DOUBLE, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_OUTPUT, offset), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "units", MXFT_STRING, NULL, 1, {MXU_UNITS_NAME_LENGTH}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_OUTPUT, units), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "busy", MXFT_BOOL, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_WAVEFORM_OUTPUT, busy), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

/* Note that the 'channel_data' field is special, since it has the MXFF_VARARGS
 * flag set, but the length of the first dimension is 0.  This means that
 * MX will dereference 'field->data_pointer' through the function
 * mx_read_void_pointer_from_memory_location(), but not memory will actually
 * be allocated for the field.
 */

typedef struct {
	mx_status_type ( *arm ) ( MX_WAVEFORM_OUTPUT *wvout );
	mx_status_type ( *trigger ) ( MX_WAVEFORM_OUTPUT *wvout );
	mx_status_type ( *stop ) ( MX_WAVEFORM_OUTPUT *wvout );
	mx_status_type ( *busy ) ( MX_WAVEFORM_OUTPUT *wvout );
	mx_status_type ( *read_all ) ( MX_WAVEFORM_OUTPUT *wvout );
	mx_status_type ( *write_all ) ( MX_WAVEFORM_OUTPUT *wvout );
	mx_status_type ( *read_channel ) ( MX_WAVEFORM_OUTPUT *wvout );
	mx_status_type ( *write_channel ) ( MX_WAVEFORM_OUTPUT *wvout );
	mx_status_type ( *get_parameter ) ( MX_WAVEFORM_OUTPUT *wvout );
	mx_status_type ( *set_parameter ) ( MX_WAVEFORM_OUTPUT *wvout );
} MX_WAVEFORM_OUTPUT_FUNCTION_LIST;

MX_API mx_status_type mx_waveform_output_get_pointers(
		MX_RECORD *wvout_record,
		MX_WAVEFORM_OUTPUT **wvout,
		MX_WAVEFORM_OUTPUT_FUNCTION_LIST **function_list_ptr,
		const char *calling_fname );

MX_API mx_status_type mx_waveform_output_initialize_type(
			long record_type,
			long *num_record_fields,
			MX_RECORD_FIELD_DEFAULTS **record_field_defaults,
			long *maximum_num_channels_varargs_cookie,
			long *maximum_num_points_varargs_cookie );

MX_API mx_status_type mx_waveform_output_finish_record_initialization(
					MX_RECORD *wvout_record );

MX_API mx_status_type mx_waveform_output_arm( MX_RECORD *wvout_record );

MX_API mx_status_type mx_waveform_output_trigger( MX_RECORD *wvout_record );

MX_API mx_status_type mx_waveform_output_start( MX_RECORD *wvout_record );

MX_API mx_status_type mx_waveform_output_stop( MX_RECORD *wvout_record );

MX_API mx_status_type mx_waveform_output_is_busy( MX_RECORD *wvout_record,
							mx_bool_type *busy );

MX_API mx_status_type mx_waveform_output_read_all( MX_RECORD *wvout_record,
						unsigned long *num_channels,
						unsigned long *num_points,
						double ***wvout_data );

MX_API mx_status_type mx_waveform_output_write_all( MX_RECORD *wvout_record,
						unsigned long num_channels,
						unsigned long num_points,
						double **wvout_data );

MX_API mx_status_type mx_waveform_output_read_channel( MX_RECORD *wvout_record,
						unsigned long channel_index,
						unsigned long *num_points,
						double **channel_data );

MX_API mx_status_type mx_waveform_output_write_channel( MX_RECORD *wvout_record,
						unsigned long channel_index,
						unsigned long num_points,
						double *channel_data );

MX_API mx_status_type mx_waveform_output_get_timer_data(
						MX_RECORD *wvout_record,
						unsigned long num_points,
						double *timer_data );

MX_API mx_status_type mx_waveform_output_set_timer_data(
						MX_RECORD *wvout_record,
						unsigned long num_points,
						double *timer_data );

MX_API mx_status_type mx_waveform_output_get_num_points(
						MX_RECORD *wvout_record,
						unsigned long *num_points );

MX_API mx_status_type mx_waveform_output_set_num_points(
						MX_RECORD *wvout_record,
						unsigned long num_points );

/*----*/

MX_API mx_status_type mx_waveform_output_default_get_parameter_handler(
						MX_WAVEFORM_OUTPUT *wvout );

MX_API mx_status_type mx_waveform_output_default_set_parameter_handler(
						MX_WAVEFORM_OUTPUT *wvout );

#ifdef __cplusplus
}
#endif

#endif /* __MX_WAVEFORM_OUTPUT_H__ */

