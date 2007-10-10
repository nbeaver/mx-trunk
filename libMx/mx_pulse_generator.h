/*
 * Name:    mx_pulse_generator.h
 *
 * Purpose: MX system header file for pulse generators and function generators.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2002, 2005-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_PULSE_GENERATOR_H__
#define __MX_PULSE_GENERATOR_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* Values for the mode field. */

#define MXF_PGN_PULSE		1
#define MXF_PGN_SQUARE_WAVE	2
#define MXF_PGN_TRIANGLE_WAVE	3
#define MXF_PGN_SAWTOOTH_WAVE	4

/* If num_pulses is set to MXF_PGN_FOREVER, then the pulse generator
 * will pulse until explicitly stopped.
 */

#define MXF_PGN_FOREVER		0

typedef struct {
	MX_RECORD *record;

	double pulse_period;
	double pulse_width;
	unsigned long num_pulses;

	double pulse_delay;

	long parameter_type;
	long mode;

	mx_bool_type busy;
	mx_bool_type start;
	mx_bool_type stop;
} MX_PULSE_GENERATOR;

#define MXLV_PGN_PULSE_PERIOD	16001
#define MXLV_PGN_PULSE_WIDTH	16002
#define MXLV_PGN_NUM_PULSES	16003
#define MXLV_PGN_PULSE_DELAY	16004
#define MXLV_PGN_MODE		16005
#define MXLV_PGN_BUSY		16006
#define MXLV_PGN_START		16007
#define MXLV_PGN_STOP		16008

#define MX_PULSE_GENERATOR_STANDARD_FIELDS \
  {MXLV_PGN_PULSE_PERIOD, -1, "pulse_period", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, pulse_period), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {MXLV_PGN_PULSE_WIDTH, -1, "pulse_width", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, pulse_width), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {MXLV_PGN_NUM_PULSES, -1, "num_pulses", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, num_pulses), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {MXLV_PGN_PULSE_DELAY, -1, "pulse_delay", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, pulse_delay), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {MXLV_PGN_MODE, -1, "mode", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, mode), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {MXLV_PGN_BUSY, -1, "busy", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, busy), \
	{0}, NULL, MXFF_IN_SUMMARY }, \
  \
  {MXLV_PGN_START, -1, "start", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, start), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {MXLV_PGN_STOP, -1, "stop", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, stop), \
	{0}, NULL, 0 }

typedef struct {
	mx_status_type ( *busy ) ( MX_PULSE_GENERATOR *pulse_generator );
	mx_status_type ( *start ) ( MX_PULSE_GENERATOR *pulse_generator );
	mx_status_type ( *stop ) ( MX_PULSE_GENERATOR *pulse_generator );
	mx_status_type ( *get_parameter ) (MX_PULSE_GENERATOR *pulse_generator);
	mx_status_type ( *set_parameter ) (MX_PULSE_GENERATOR *pulse_generator);
} MX_PULSE_GENERATOR_FUNCTION_LIST;

MX_API_PRIVATE mx_status_type mx_pulse_generator_get_pointers(
	MX_RECORD *pulse_generator_record,
	MX_PULSE_GENERATOR **pulse_generator,
	MX_PULSE_GENERATOR_FUNCTION_LIST **function_list_ptr,
	const char *calling_fname );

MX_API mx_status_type mx_pulse_generator_initialize( MX_RECORD *record );

MX_API mx_status_type mx_pulse_generator_is_busy( MX_RECORD *record,
							mx_bool_type *busy );

MX_API mx_status_type mx_pulse_generator_start( MX_RECORD *record );

MX_API mx_status_type mx_pulse_generator_stop( MX_RECORD *record );

MX_API mx_status_type mx_pulse_generator_get_mode( MX_RECORD *record,
							long *mode );

MX_API mx_status_type mx_pulse_generator_set_mode( MX_RECORD *record,
							long mode );

MX_API mx_status_type mx_pulse_generator_get_pulse_period( MX_RECORD *record, 
							double *pulse_period );

MX_API mx_status_type mx_pulse_generator_set_pulse_period( MX_RECORD *record, 
							double pulse_period );

MX_API mx_status_type mx_pulse_generator_get_pulse_width( MX_RECORD *record, 
							double *pulse_width );

MX_API mx_status_type mx_pulse_generator_set_pulse_width( MX_RECORD *record, 
							double pulse_width );

MX_API mx_status_type mx_pulse_generator_get_num_pulses( MX_RECORD *record, 
						unsigned long *num_pulses );

MX_API mx_status_type mx_pulse_generator_set_num_pulses( MX_RECORD *record, 
						unsigned long num_pulses );

MX_API mx_status_type mx_pulse_generator_get_pulse_delay( MX_RECORD *record, 
							double *pulse_delay );

MX_API mx_status_type mx_pulse_generator_set_pulse_delay( MX_RECORD *record, 
							double pulse_delay );

MX_API mx_status_type mx_pulse_generator_default_get_parameter_handler(
					MX_PULSE_GENERATOR *pulse_generator );

MX_API mx_status_type mx_pulse_generator_default_set_parameter_handler(
					MX_PULSE_GENERATOR *pulse_generator );

#ifdef __cplusplus
}
#endif

#endif /* __MX_PULSE_GENERATOR_H__ */
