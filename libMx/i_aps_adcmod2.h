/*
 * Name:    i_aps_adcmod2.h
 *
 * Purpose: Header for MX interface driver for the ADCMOD2 electrometer
 *          system designed by Steve Ross of the Advanced Photon Source.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_APS_ADCMOD2_H__
#define __I_APS_ADCMOD2_H__

#define MX_APS_ADCMOD2_MAX_AMPLIFIERS	4

#define MX_APS_ADCMOD2_MAX_INPUTS	16

typedef struct {
	MX_RECORD *record;

	MX_RECORD *vme_record;
	unsigned long crate_number;
	unsigned long base_address;
	double averaged_value_update_frequency;			/* in Hz */
	double raw_value_read_frequency;			/* in Hz */
	unsigned long num_raw_measurements_to_average;

	MX_CLOCK_TICK ticks_between_averaged_measurements;
	MX_CLOCK_TICK next_measurement_time;

	unsigned long microseconds_between_raw_measurements;
	int use_udelay;

	MX_RECORD *amplifier_array[MX_APS_ADCMOD2_MAX_AMPLIFIERS];
	mx_uint16_type input_value[MX_APS_ADCMOD2_MAX_INPUTS];
} MX_APS_ADCMOD2;

#define MXI_APS_ADCMOD2_STANDARD_FIELDS \
  {-1, -1, "vme_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_APS_ADCMOD2, vme_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "crate_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_APS_ADCMOD2, crate_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "base_address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_APS_ADCMOD2, base_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "averaged_value_update_frequency", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_ADCMOD2, averaged_value_update_frequency), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "raw_value_read_frequency", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_ADCMOD2, raw_value_read_frequency), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_raw_measurements_to_average", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_ADCMOD2, num_raw_measurements_to_average), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_aps_adcmod2_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_aps_adcmod2_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxi_aps_adcmod2_open( MX_RECORD *record );

/*---*/

MX_API mx_status_type mxi_aps_adcmod2_in16( MX_APS_ADCMOD2 *aps_adcmod2,
						unsigned long offset,
						mx_uint16_type *word_value );

MX_API mx_status_type mxi_aps_adcmod2_out16( MX_APS_ADCMOD2 *aps_adcmod2,
						unsigned long offset,
						mx_uint16_type word_value );

MX_API mx_status_type mxi_aps_adcmod2_command( MX_APS_ADCMOD2 *aps_adcmod2,
						int electrometer_number,
						mx_uint16_type command,
						mx_uint16_type value );

MX_API mx_status_type mxi_aps_adcmod2_latch_values(MX_APS_ADCMOD2 *aps_adcmod2);

MX_API mx_status_type mxi_aps_adcmod2_read_value( MX_APS_ADCMOD2 *aps_adcmod2,
						unsigned long ainput_number,
						mx_uint16_type *word_value );

extern MX_RECORD_FUNCTION_LIST mxi_aps_adcmod2_record_function_list;

extern long mxi_aps_adcmod2_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_aps_adcmod2_rfield_def_ptr;

#endif /* __I_APS_ADCMOD2_H__ */
