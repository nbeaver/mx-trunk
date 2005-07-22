/*
 * Name:    d_pdi45_pulser.h
 *
 * Purpose: Header for MX pulse generator driver for a Prairie Digital
 *          Model 45 digital I/O channel configured for PWM output.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PDI45_PULSER_H__
#define __D_PDI45_PULSER_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pdi45_record;
	int line_number;
} MX_PDI45_PULSER;

#define MXD_PDI45_PULSER_STANDARD_FIELDS \
  {-1, -1, "pdi45_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI45_PULSER, pdi45_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "line_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI45_PULSER, line_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_pdi45_pulser_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_pdi45_pulser_open( MX_RECORD *record );

MX_API mx_status_type mxd_pdi45_pulser_is_busy(
					MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_pdi45_pulser_start(
					MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_pdi45_pulser_stop(
					MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_pdi45_pulser_get_parameter(
					MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_pdi45_pulser_set_parameter(
					MX_PULSE_GENERATOR *pulse_generator );

extern MX_RECORD_FUNCTION_LIST mxd_pdi45_pulser_record_function_list;
extern MX_PULSE_GENERATOR_FUNCTION_LIST 
		mxd_pdi45_pulser_pulse_generator_function_list;

extern long mxd_pdi45_pulser_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pdi45_pulser_rfield_def_ptr;

#endif /* __D_PDI45_PULSER_H__ */
