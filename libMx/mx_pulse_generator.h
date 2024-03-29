/*
 * Name:    mx_pulse_generator.h
 *
 * Purpose: MX system header file for pulse generators and function generators.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2002, 2005-2007, 2015-2016, 2018, 2022
 *    Illinois Institute of Technology
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

#define MXU_PGN_NUM_SETUP_PARAMETERS	6

/* Labels for the setup array. */

#define MXSUP_PGN_PULSE_PERIOD	0
#define MXSUP_PGN_PULSE_WIDTH	1
#define MXSUP_PGN_NUM_PULSES	2
#define MXSUP_PGN_PULSE_DELAY	3
#define MXSUP_PGN_FUNCTION_MODE	4
#define MXSUP_PGN_TRIGGER_MODE	5

/* Values for the function mode field. */

#define MXF_PGN_PULSE		1
#define MXF_PGN_SQUARE_WAVE	2
#define MXF_PGN_TRIANGLE_WAVE	3
#define MXF_PGN_SAWTOOTH_WAVE	4
#define MXF_PGN_SINE_WAVE	5

#define MXF_PGN_EXT_DURATION    1001

/* If num_pulses is set to MXF_PGN_FOREVER, then the pulse generator
 * will pulse until explicitly stopped.
 */

#define MXF_PGN_FOREVER		0

/* Status bit definitions for the status field. */

#define MXSF_PGN_IS_BUSY	0x1

#define MXSF_PGN_ARMED		0x1000000
#define MXSF_PGN_TRIGGERED	0x2000000

#define MXSF_PGN_ERROR		0x80000000

typedef struct {
	MX_RECORD *record;

	double pulse_period;
	double pulse_width;
	long num_pulses;
	double pulse_delay;
	long function_mode;
	long trigger_mode;

	double setup[MXU_PGN_NUM_SETUP_PARAMETERS];

	long parameter_type;

	
	mx_bool_type arm;
	mx_bool_type trigger;
	mx_bool_type start;
	mx_bool_type stop;
	mx_bool_type abort;

	mx_bool_type busy;
	long last_pulse_number;
	unsigned long total_num_pulses;
	unsigned long status;

	/* The following fields are used to handle pulse generators that
	 * do not set the 'busy' flag immediately after arming themselves.
	 */

	mx_bool_type busy_start_interval_enabled;
	double busy_start_interval;	/* In seconds */
	double last_start_time;		/* In seconds */

	MX_CLOCK_TICK busy_start_ticks;
	MX_CLOCK_TICK last_start_tick;

} MX_PULSE_GENERATOR;

#define MXLV_PGN_PULSE_PERIOD		16000
#define MXLV_PGN_PULSE_WIDTH		16001
#define MXLV_PGN_NUM_PULSES		16002
#define MXLV_PGN_PULSE_DELAY		16003
#define MXLV_PGN_FUNCTION_MODE		16004
#define MXLV_PGN_TRIGGER_MODE		16005
#define MXLV_PGN_SETUP			16006
#define MXLV_PGN_ARM			16007
#define MXLV_PGN_TRIGGER		16008
#define MXLV_PGN_START			16009
#define MXLV_PGN_STOP			16010
#define MXLV_PGN_ABORT			16011
#define MXLV_PGN_BUSY			16012
#define MXLV_PGN_LAST_PULSE_NUMBER	16013
#define MXLV_PGN_TOTAL_NUM_PULSES	16014
#define MXLV_PGN_STATUS			16015

#define MX_PULSE_GENERATOR_STANDARD_FIELDS \
  {MXLV_PGN_PULSE_PERIOD, -1, "pulse_period", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, pulse_period), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {MXLV_PGN_PULSE_WIDTH, -1, "pulse_width", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, pulse_width), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {MXLV_PGN_NUM_PULSES, -1, "num_pulses", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, num_pulses), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {MXLV_PGN_PULSE_DELAY, -1, "pulse_delay", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, pulse_delay), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {MXLV_PGN_FUNCTION_MODE, -1, "function_mode", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, function_mode), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {MXLV_PGN_TRIGGER_MODE, -1, "trigger_mode", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, trigger_mode), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {MXLV_PGN_SETUP, -1, "setup", MXFT_DOUBLE, \
				NULL, 1, {MXU_PGN_NUM_SETUP_PARAMETERS}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, setup), \
	{0}, NULL, 0 }, \
  \
  {MXLV_PGN_ARM, -1, "arm", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, arm), \
	{0}, NULL, 0 }, \
  \
  {MXLV_PGN_TRIGGER, -1, "trigger", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, trigger), \
	{0}, NULL, 0 }, \
  \
  {MXLV_PGN_START, -1, "start", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, start), \
	{0}, NULL, 0 }, \
  \
  {MXLV_PGN_STOP, -1, "stop", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, stop), \
	{0}, NULL, 0 }, \
  \
  {MXLV_PGN_ABORT, -1, "abort", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, abort), \
	{0}, NULL, 0 }, \
  \
  {MXLV_PGN_BUSY, -1, "busy", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, busy), \
	{0}, NULL, (MXFF_READ_ONLY | MXFF_POLL) }, \
  \
  {MXLV_PGN_LAST_PULSE_NUMBER, -1, "last_pulse_number", \
					MXFT_LONG, NULL, 0, {0},\
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, last_pulse_number), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_PGN_TOTAL_NUM_PULSES, -1, "total_num_pulses", \
					MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, total_num_pulses), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_PGN_STATUS, -1, "status", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PULSE_GENERATOR, status), \
	{0}, NULL, (MXFF_IN_SUMMARY | MXFF_READ_ONLY | MXFF_POLL)}

typedef struct {
	mx_status_type ( *busy ) ( MX_PULSE_GENERATOR *pulse_generator );
	mx_status_type ( *arm ) ( MX_PULSE_GENERATOR *pulse_generator );
	mx_status_type ( *trigger ) ( MX_PULSE_GENERATOR *pulse_generator );
	mx_status_type ( *stop ) ( MX_PULSE_GENERATOR *pulse_generator );
	mx_status_type ( *abort ) ( MX_PULSE_GENERATOR *pulse_generator );
	mx_status_type ( *get_parameter ) (MX_PULSE_GENERATOR *pulse_generator);
	mx_status_type ( *set_parameter ) (MX_PULSE_GENERATOR *pulse_generator);
	mx_status_type ( *setup ) (MX_PULSE_GENERATOR *pulse_generator);
	mx_status_type ( *get_status ) (MX_PULSE_GENERATOR *pulse_generator);
} MX_PULSE_GENERATOR_FUNCTION_LIST;

MX_API_PRIVATE mx_status_type mx_pulse_generator_get_pointers(
	MX_RECORD *pulse_generator_record,
	MX_PULSE_GENERATOR **pulse_generator,
	MX_PULSE_GENERATOR_FUNCTION_LIST **function_list_ptr,
	const char *calling_fname );

MX_API mx_status_type mx_pulse_generator_initialize( MX_RECORD *record );

MX_API mx_status_type mx_pulse_generator_is_busy( MX_RECORD *record,
							mx_bool_type *busy );

MX_API mx_status_type mx_pulse_generator_arm( MX_RECORD *record );

MX_API mx_status_type mx_pulse_generator_trigger( MX_RECORD *record );

MX_API mx_status_type mx_pulse_generator_start( MX_RECORD *record );

MX_API mx_status_type mx_pulse_generator_stop( MX_RECORD *record );

MX_API mx_status_type mx_pulse_generator_abort( MX_RECORD *record );

MX_API mx_status_type mx_pulse_generator_setup( MX_RECORD *record,
						double pulse_period,
						double pulse_width,
						long num_pulses,
						double pulse_delay,
						long function_mode,
						long trigger_mode );

MX_API mx_status_type mx_pulse_generator_update_settings_from_setup(
						MX_PULSE_GENERATOR *pulser );

MX_API mx_status_type mx_pulse_generator_get_function_mode( MX_RECORD *record,
							long *function_mode );

MX_API mx_status_type mx_pulse_generator_set_function_mode( MX_RECORD *record,
							long function_mode );

MX_API mx_status_type mx_pulse_generator_get_trigger_mode( MX_RECORD *record,
							long *trigger_mode );

MX_API mx_status_type mx_pulse_generator_set_trigger_mode( MX_RECORD *record,
							long trigger_mode );

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

MX_API mx_status_type mx_pulse_generator_get_last_pulse_number(
						MX_RECORD *record,
						long *last_pulse_number );

MX_API mx_status_type mx_pulse_generator_get_total_num_pulses(
					MX_RECORD *record,
					unsigned long *total_num_pulses );

MX_API mx_status_type mx_pulse_generator_get_status( MX_RECORD *record,
						unsigned long *pulser_status );

MX_API mx_status_type mx_pulse_generator_default_get_parameter_handler(
					MX_PULSE_GENERATOR *pulse_generator );

MX_API mx_status_type mx_pulse_generator_default_set_parameter_handler(
					MX_PULSE_GENERATOR *pulse_generator );

#ifdef __cplusplus
}
#endif

#endif /* __MX_PULSE_GENERATOR_H__ */
