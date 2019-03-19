/* 
 * Name:    d_amptek_dp5_mca.h
 *
 * Purpose: Header file for Amptek MCAs that use the DP5 protocol.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2017-2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_AMPTEK_DP5_MCA_H__
#define __D_AMPTEK_DP5_MCA_H__

#define MXU_AMPTEK_DP5_MAX_SCAS	8
#define MXU_AMPTEK_DP5_MAX_BINS	8192

#define MXU_AMPTEK_DP5_NUM_STATUS_BYTES		64

/* Bit values for the 'amptek_dp5_mca_flags' field. */

#define MXF_AMPTEK_DP5_MCA_DEBUG_STATUS_PACKET	0x1000

typedef struct {
	MX_RECORD *record;

	MX_RECORD *amptek_dp5_record;
	unsigned long amptek_dp5_mca_flags;

	char status_data[MXU_AMPTEK_DP5_NUM_STATUS_BYTES];

	unsigned long fast_counts;
	unsigned long slow_counts;
	unsigned long general_purpose_counter;
	double accumulation_time;

	double high_voltage;
	double temperature;

	char raw_mca_spectrum[ 3 * MXU_AMPTEK_DP5_MAX_BINS ];
} MX_AMPTEK_DP5_MCA;

#define MXLV_AMPTEK_DP5_STATUS_DATA		87001
#define MXLV_AMPTEK_DP5_FAST_COUNTS		87002
#define MXLV_AMPTEK_DP5_SLOW_COUNTS		87003
#define MXLV_AMPTEK_DP5_GENERAL_PURPOSE_COUNTER	87004
#define MXLV_AMPTEK_DP5_ACCUMULATION_TIME	87005
#define MXLV_AMPTEK_DP5_HIGH_VOLTAGE		87006
#define MXLV_AMPTEK_DP5_TEMPERATURE		87007

#define MXD_AMPTEK_DP5_STANDARD_FIELDS \
  {-1, -1, "amptek_dp5_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_AMPTEK_DP5_MCA, amptek_dp5_record ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "amptek_dp5_mca_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_AMPTEK_DP5_MCA, amptek_dp5_mca_flags ), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {MXLV_AMPTEK_DP5_STATUS_DATA, -1, "status_data", MXFT_CHAR, NULL, \
				1, {MXU_AMPTEK_DP5_NUM_STATUS_BYTES}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_AMPTEK_DP5_MCA, status_data ), \
	{sizeof(unsigned char)}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_AMPTEK_DP5_FAST_COUNTS, -1, "fast_counts", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_AMPTEK_DP5_MCA, fast_counts ), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_AMPTEK_DP5_SLOW_COUNTS, -1, "slow_counts", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_AMPTEK_DP5_MCA, slow_counts ), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_AMPTEK_DP5_GENERAL_PURPOSE_COUNTER, -1, "general_purpose_counter", \
						MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_AMPTEK_DP5_MCA, general_purpose_counter ), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_AMPTEK_DP5_ACCUMULATION_TIME, -1, "accumulation_time", \
						MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_AMPTEK_DP5_MCA, accumulation_time ), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_AMPTEK_DP5_HIGH_VOLTAGE, -1, "high_voltage", MXFT_DOUBLE, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof( MX_AMPTEK_DP5_MCA, high_voltage ), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_AMPTEK_DP5_TEMPERATURE, -1, "temperature", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_AMPTEK_DP5_MCA, temperature ), \
	{0}, NULL, MXFF_READ_ONLY }

MX_API mx_status_type mxd_amptek_dp5_mca_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_amptek_dp5_mca_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_amptek_dp5_mca_open( MX_RECORD *record );
MX_API mx_status_type mxd_amptek_dp5_mca_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_amptek_dp5_mca_trigger( MX_MCA *mca );
MX_API mx_status_type mxd_amptek_dp5_mca_stop( MX_MCA *mca );
MX_API mx_status_type mxd_amptek_dp5_mca_read( MX_MCA *mca );
MX_API mx_status_type mxd_amptek_dp5_mca_clear( MX_MCA *mca );
MX_API mx_status_type mxd_amptek_dp5_mca_get_status( MX_MCA *mca );
MX_API mx_status_type mxd_amptek_dp5_mca_get_parameter( MX_MCA *mca );
MX_API mx_status_type mxd_amptek_dp5_mca_set_parameter( MX_MCA *mca );

extern MX_RECORD_FUNCTION_LIST mxd_amptek_dp5_mca_record_function_list;
extern MX_MCA_FUNCTION_LIST mxd_amptek_dp5_mca_mca_function_list;

extern long mxd_amptek_dp5_mca_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_amptek_dp5_mca_rfield_def_ptr;

#endif /* __D_AMPTEK_DP5_MCA_H__ */
