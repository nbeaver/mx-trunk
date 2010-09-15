/* 
 * Name:    d_epics_mca.h
 *
 * Purpose: Header file for EPICS-controlled multichannel analyzers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002-2003, 2009-2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EPICS_MCA_H__
#define __D_EPICS_MCA_H__

typedef struct {
	char epics_detector_name[ MXU_EPICS_PVNAME_LENGTH+1 ];
	char epics_mca_name[ MXU_EPICS_PVNAME_LENGTH+1 ];
	char epics_dxp_name[ MXU_EPICS_PVNAME_LENGTH+1 ];

	MX_EPICS_PV acquiring_pv;
	MX_EPICS_PV erase_pv;
	MX_EPICS_PV erase_start_pv;
	MX_EPICS_PV preset_live_pv;
	MX_EPICS_PV preset_real_pv;
	MX_EPICS_PV stop_pv;
	MX_EPICS_PV start_pv;

	MX_EPICS_PV act_pv;
	MX_EPICS_PV eltm_pv;
	MX_EPICS_PV ertm_pv;
	MX_EPICS_PV nuse_pv;
	MX_EPICS_PV pct_pv;
	MX_EPICS_PV pcth_pv;
	MX_EPICS_PV pctl_pv;
	MX_EPICS_PV val_pv;

	long num_roi_pvs;
	MX_EPICS_PV *roi_low_pv_array;
	MX_EPICS_PV *roi_high_pv_array;
	MX_EPICS_PV *roi_integral_pv_array;
	MX_EPICS_PV *roi_background_pv_array;

	mx_bool_type is_dxp;
	MX_EPICS_PV icr_pv;
	MX_EPICS_PV ocr_pv;

	unsigned long epics_mca_flags;

	long num_associated_mcas;
	MX_RECORD **associated_mca_record_array;
} MX_EPICS_MCA;

/* Values for the 'epics_mca_flags' field. */

#define MXF_EPICS_MCA_MULTIELEMENT_DETECTOR		0x1
#define MXF_EPICS_MCA_NO_ERASE_ON_START			0x2
#define MXF_EPICS_MCA_DISABLE_READ_OPTIMIZATION		0x4
#define MXF_EPICS_MCA_WAIT_ON_START			0x8
#define MXF_EPICS_MCA_USE_VERBOSE_ICR_OCR_NAMES		0x10

/*---*/

#define MXD_EPICS_MCA_STANDARD_FIELDS \
  {-1, -1, "epics_detector_name", MXFT_STRING, \
		NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_MCA, epics_detector_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "epics_mca_name", MXFT_STRING, \
		NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_MCA, epics_mca_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "epics_dxp_name", MXFT_STRING, \
		NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_MCA, epics_dxp_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "epics_mca_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_MCA, epics_mca_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "num_associated_mcas", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_MCA, num_associated_mcas), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "associated_mca_record_array", \
		MXFT_RECORD, NULL, 1, {MXU_VARARGS_LENGTH},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_EPICS_MCA, associated_mca_record_array), \
	{sizeof(MX_RECORD *)}, NULL, MXFF_VARARGS }

MX_API mx_status_type mxd_epics_mca_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_epics_mca_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_mca_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_mca_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_epics_mca_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_mca_open( MX_RECORD *record );

MX_API mx_status_type mxd_epics_mca_start( MX_MCA *mca );
MX_API mx_status_type mxd_epics_mca_stop( MX_MCA *mca );
MX_API mx_status_type mxd_epics_mca_read( MX_MCA *mca );
MX_API mx_status_type mxd_epics_mca_clear( MX_MCA *mca );
MX_API mx_status_type mxd_epics_mca_busy( MX_MCA *mca );
MX_API mx_status_type mxd_epics_mca_get_parameter( MX_MCA *mca );
MX_API mx_status_type mxd_epics_mca_set_parameter( MX_MCA *mca );

extern MX_RECORD_FUNCTION_LIST mxd_epics_mca_record_function_list;
extern MX_MCA_FUNCTION_LIST mxd_epics_mca_mca_function_list;

extern long mxd_epics_mca_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epics_mca_rfield_def_ptr;

#endif /* __D_EPICS_MCA_H__ */

