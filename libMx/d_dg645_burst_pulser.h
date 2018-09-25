/*
 * Name:    d_dg645_burst_pulser.h
 *
 * Purpose: Header file for the Stanford Research Systems DG645 Digital
 *          Pulse Generator using Burst Mode.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2017-2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_DG645_BURST_PULSER_H__
#define __D_DG645_BURST_PULSER_H__

#include "mx_pulse_generator.h"

#define MXU_DG645_BURST_PULSER_OUTPUT_NAME_LENGTH	40

typedef struct {
	MX_RECORD *record;

	MX_RECORD *dg645_record;
	char output_name[MXU_DG645_BURST_PULSER_OUTPUT_NAME_LENGTH+1];
	unsigned long dg645_burst_pulser_flags;

	unsigned long output_number;

} MX_DG645_BURST_PULSER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_dg645_burst_pulser_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_dg645_burst_pulser_open( MX_RECORD *record );

MX_API mx_status_type mxd_dg645_burst_pulser_arm( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_dg645_burst_pulser_stop( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_dg645_burst_pulser_get_parameter(
					MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_dg645_burst_pulser_set_parameter(
					MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_dg645_burst_pulser_get_status(
					MX_PULSE_GENERATOR *pulser );

extern MX_RECORD_FUNCTION_LIST mxd_dg645_burst_pulser_record_function_list;
extern MX_PULSE_GENERATOR_FUNCTION_LIST mxd_dg645_burst_pulser_pulser_function_list;

extern long mxd_dg645_burst_pulser_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_dg645_burst_pulser_rfield_def_ptr;

#define MXD_DG645_BURST_PULSER_STANDARD_FIELDS \
  {-1, -1, "dg645_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645_BURST_PULSER, dg645_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "output_name", MXFT_STRING, \
			NULL, 1, {MXU_DG645_BURST_PULSER_OUTPUT_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645_BURST_PULSER, output_name), \
	{sizeof(char)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY) }, \
  \
  {-1, -1, "dg645_burst_pulser_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_DG645_BURST_PULSER, dg645_burst_pulser_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_DG645_BURST_PULSER_H__ */

