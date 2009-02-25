/*
 * Name:    d_aviex_pccd.c
 *
 * Purpose: MX driver for the Aviex PCCD series of CCD detectors.
 *
 * Author:  William Lavender
 *
 * Note:    Currently supported detectors include
 *              PCCD-170170   at SOLEIL SWING
 *              PCCD-4824     at ESRF (WAXS)
 *              PCCD-16080    at APS BioCAT
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_AVIEX_PCCD_DEBUG				FALSE

#define MXD_AVIEX_PCCD_DEBUG_LINEARITY_LOOKUP		FALSE

#define MXD_AVIEX_PCCD_DEBUG_DESCRAMBLING		FALSE

#define MXD_AVIEX_PCCD_DEBUG_ALLOCATION			FALSE

#define MXD_AVIEX_PCCD_DEBUG_ALLOCATION_DETAILS		FALSE

#define MXD_AVIEX_PCCD_DEBUG_SERIAL			TRUE

#define MXD_AVIEX_PCCD_DEBUG_MX_IMAGE_ALLOC		FALSE

#define MXD_AVIEX_PCCD_DEBUG_TIMING			FALSE

#define MXD_AVIEX_PCCD_DEBUG_FRAME_CORRECTION		FALSE

#define MXD_AVIEX_PCCD_DEBUG_SETUP_GEOMETRICAL_MASK	FALSE

#define MXD_AVIEX_PCCD_DEBUG_EXTENDED_STATUS		FALSE

#define MXD_AVIEX_PCCD_DEBUG_MEMORY_LEAK		FALSE

#define MXD_AVIEX_PCCD_DEBUG_CONTROL_REGISTER		FALSE

/* FIXME: Leave the geometrical mask kludge definition set to TRUE. */

#define MXD_AVIEX_PCCD_GEOMETRICAL_MASK_KLUDGE		TRUE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_unistd.h"
#include "mx_ascii.h"
#include "mx_clock.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_key.h"
#include "mx_cfn.h"
#include "mx_bit.h"
#include "mx_hrt.h"
#include "mx_memory.h"
#include "mx_image.h"
#include "mx_digital_output.h"
#include "mx_video_input.h"
#include "mx_camera_link.h"
#include "mx_area_detector.h"
#include "i_epix_xclib.h"
#include "d_epix_xclib.h"
#include "d_aviex_pccd.h"

#if MXD_AVIEX_PCCD_DEBUG_TIMING
#  include "mx_hrt_debug.h"
#endif

/*------------------------------------------------------------------------*/

#if MXD_AVIEX_PCCD_DEBUG_MEMORY_LEAK
#  define DISPLAY_MEMORY_USAGE( label ) \
	do {                                                                  \
	    MX_PROCESS_MEMINFO meminfo;                                       \
	    (void) mx_get_process_meminfo( MXF_PROCESS_ID_SELF, &meminfo );   \
	    MX_DEBUG(-2,("\n%s: total_bytes = %lu\n",                         \
			(label), (unsigned long) meminfo.total_bytes));       \
	} while(0)

#else
#  define DISPLAY_MEMORY_USAGE( label )
#endif

/*------------------------------------------------------------------------*/

#if MXD_AVIEX_PCCD_DEBUG_CONTROL_REGISTER
#  define DISPLAY_CONTROL_REGISTER( label ) \
	do {                                                                  \
		long dse_dh_control;                                          \
		(void) mx_area_detector_get_register( ad->record,             \
			"dh_control", &dse_dh_control );                      \
		MX_DEBUG(-2,("%s: dh_control = %#lx", label, dse_dh_control));\
	} while(0)
#else
#  define DISPLAY_CONTROL_REGISTER( label )
#endif

/*------------------------------------------------------------------------*/

/* Internal prototype for smvspatial. */

MX_API int
smvspatial( void *imarr,
	int imwid,
	int imhit,
	int cflags,
	char *splname,
	uint16_t *maskval );

/*------------------------------------------------------------------------*/


MX_RECORD_FUNCTION_LIST mxd_aviex_pccd_record_function_list = {
	mxd_aviex_pccd_initialize_type,
	mxd_aviex_pccd_create_record_structures,
	mx_area_detector_finish_record_initialization,
	mxd_aviex_pccd_delete_record,
	NULL,
	NULL,
	NULL,
	mxd_aviex_pccd_open,
	mxd_aviex_pccd_close,
	NULL,
	mxd_aviex_pccd_resynchronize,
	mxd_aviex_pccd_special_processing_setup
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_aviex_pccd_ad_function_list = {
	mxd_aviex_pccd_arm,
	mxd_aviex_pccd_trigger,
	mxd_aviex_pccd_stop,
	mxd_aviex_pccd_abort,
	NULL,
	NULL,
	NULL,
	mxd_aviex_pccd_get_extended_status,
	mxd_aviex_pccd_readout_frame,
	mxd_aviex_pccd_correct_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_aviex_pccd_get_parameter,
	mxd_aviex_pccd_set_parameter,
	mxd_aviex_pccd_measure_correction,
	mxd_aviex_pccd_geometrical_correction
};

/*---*/

static mx_status_type mxd_aviex_pccd_process_function( void *record_ptr,
							void *record_field_ptr,
							int operation );

static mx_status_type
mxd_aviex_pccd_get_pointers( MX_AREA_DETECTOR *ad,
			MX_AVIEX_PCCD **aviex_pccd,
			const char *calling_fname )
{
	static const char fname[] = "mxd_aviex_pccd_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( aviex_pccd == (MX_AVIEX_PCCD **) NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AVIEX_PCCD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*aviex_pccd = (MX_AVIEX_PCCD *) ad->record->record_type_struct;

	if ( *aviex_pccd == (MX_AVIEX_PCCD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_AVIEX_PCCD pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

#if MXD_AVIEX_PCCD_DEBUG_ALLOCATION
static void
mxd_aviex_pccd_display_ul_corners( uint16_t ***sector_array, int num_sectors )
{
	static const char fname[] = "mxd_aviex_pccd_display_ul_corners()";

	long k;
	char *ptr;

	MX_DEBUG(-2,("****** %s BEGIN *****", fname));

	for ( k = 0; k < num_sectors; k++ ) {
		if ( sector_array[k] == NULL ) {
			MX_DEBUG(-2,("****** %s END alt *****", fname));
			return;
		}

		ptr = (char *) &(sector_array[k][0][0]);

		MX_DEBUG(-2,("ul_corner[%ld] = %p", k, ptr));
	}

	MX_DEBUG(-2,("****** %s END *****", fname));
}
#endif

MX_EXPORT mx_status_type
mxd_aviex_pccd_alloc_sector_array( uint16_t ****sector_array_ptr,
					long sector_width,
					long sector_height,
					long num_sector_rows,
					long num_sector_columns,
					uint16_t *image_data )
{
	static const char fname[] = "mxd_aviex_pccd_alloc_sector_array()";

	uint16_t ***sector_array;
	uint16_t **sector_array_row_ptr;
	long n, sector_row, row, sector_column, num_sectors;
	long sizeof_row_of_sectors, sizeof_full_row, sizeof_sector_row;
	long row_byte_offset, row_ptr_offset;
	long byte_offset, ptr_offset;

	if ( sector_array_ptr == (uint16_t ****) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The sector_array_ptr argument passed was NULL." );
	}
	if ( image_data == (uint16_t *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The image_data pointer passed was NULL." );
	}

#if MXD_AVIEX_PCCD_DEBUG_ALLOCATION
	MX_DEBUG(-2,("%s: sector_width = %ld, sector_height = %ld",
		fname, sector_width, sector_height));
#endif

	num_sectors = num_sector_rows * num_sector_columns;

	*sector_array_ptr = NULL;
	
	sector_array = malloc( num_sectors * sizeof(uint16_t **) );

	if ( sector_array == (uint16_t ***) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element "
		"sector array pointer.", num_sectors );
	}

#if MXD_AVIEX_PCCD_DEBUG_ALLOCATION_DETAILS
	MX_DEBUG(-2,("%s: allocated sector_array = %p, length = %lu",
		fname, sector_array, num_sectors * sizeof(uint16_t **) ));
#endif

#if MXD_AVIEX_PCCD_DEBUG_ALLOCATION_DETAILS
	memset( sector_array, 0, num_sectors * sizeof(uint16_t **) );
#endif

	sector_array_row_ptr = 
		malloc( num_sectors * sector_height * sizeof(uint16_t *) );

	if ( sector_array_row_ptr == (uint16_t **) NULL ) {
		free( sector_array );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of row pointers.", num_sectors * sector_height );
	}

#if MXD_AVIEX_PCCD_DEBUG_ALLOCATION_DETAILS
	MX_DEBUG(-2,("%s: allocated sector_array_row_ptr = %p, length = %lu",
		fname, sector_array_row_ptr,
		num_sectors * sector_height * sizeof(uint16_t *) ));
#endif

	row_byte_offset = sector_height * sizeof(sector_array_row_ptr);

	row_ptr_offset = row_byte_offset / sizeof(uint16_t *);

#if MXD_AVIEX_PCCD_DEBUG_ALLOCATION_DETAILS
	MX_DEBUG(-2,
	("%s: row_byte_offset = %ld (%#lx), row_ptr_offset = %ld (%#lx)",
		fname, row_byte_offset, row_byte_offset,
		row_ptr_offset, row_ptr_offset));

	MX_DEBUG(-2,("%s: sector_array_row_pointer = %p",
			fname, sector_array_row_ptr ));
#endif

	for ( n = 0; n < num_sectors; n++ ) {

		sector_array[n] = sector_array_row_ptr + n * row_ptr_offset;

#if MXD_AVIEX_PCCD_DEBUG_ALLOCATION_DETAILS
		MX_DEBUG(-2,("%s: sector_array[%ld] = %p",
			fname, n, sector_array[n] ));
#endif
	}

	/* The 'sizeof_full_row' is the number of bytes in a single horizontal
	 * line of the image.  The 'sizeof_row_of_sectors' is the number of
	 * bytes in a horizontal row of sectors.  In unbinned mode,
	 * sizeof_row_of_sectors = sector_height * sizeof_full_row.
	 */

	/* sizeof_sector_row     = number of bytes in a single horizontal
	 *                         row of a sector.
	 *
	 * sizeof_full_row       = number of bytes in a single horizontal
	 *                         row of the full image.
	 *
	 * sizeof_row_of_sectors = number of bytes in all the rows of a
	 *                         single row of sectors.
	 *
	 * For an unbinned image:
	 *
	 *   sizeof_row_of_sectors = sector_height * sizeof_full_row
	 *        = sector_height * num_sector_columns * sizeof_sector_row
	 */

	sizeof_sector_row = sector_width * sizeof(uint16_t);

	sizeof_full_row = num_sector_columns * sizeof_sector_row;

	sizeof_row_of_sectors = sizeof_full_row * sector_height;

#if MXD_AVIEX_PCCD_DEBUG_ALLOCATION_DETAILS
	MX_DEBUG(-2,
("%s: image_data = %p, sizeof_full_row = %#lx, sizeof_row_of_sectors = %#lx",
		fname, image_data, sizeof_full_row, sizeof_row_of_sectors));
#endif

#if MXD_AVIEX_PCCD_DEBUG_ALLOCATION_DETAILS
	MX_DEBUG(-2,("%s:\n"
		"    byte_offset = ( %#lx ) * sector_row\n"
		"                  + ( %#lx ) * row\n"
		"                  + ( %#lx ) * sector_column\n",
	fname, sizeof_row_of_sectors, sizeof_full_row, sizeof_sector_row));

	MX_DEBUG(-2,("%s:\n"
		"    byte_offset = ( %lu ) * sector_row\n"
		"                  + ( %lu ) * row\n"
		"                  + ( %lu ) * sector_column\n",
	fname, sizeof_row_of_sectors, sizeof_full_row, sizeof_sector_row));

	MX_DEBUG(-2,("%s: image_data = %p", fname, image_data));
#endif

	for ( sector_row = 0; sector_row < num_sector_rows; sector_row++ ) {
	    for ( row = 0; row < sector_height; row++ ) {
		for ( sector_column = 0; sector_column < num_sector_columns;
							sector_column++ )
		{

		    n = sector_column + num_sector_columns * sector_row;

		    byte_offset = sector_row * sizeof_row_of_sectors
				+ row * sizeof_full_row
				+ sector_column * sizeof_sector_row;

		    ptr_offset = byte_offset / sizeof(uint16_t);

		    sector_array[n][row] = image_data + ptr_offset;

#if MXD_AVIEX_PCCD_DEBUG_ALLOCATION_DETAILS
		    if ( row == 0 ) {
			MX_DEBUG(-2,
	("*** n = %ld, sector_row = %ld, sector_column = %ld, addr = %#lx ***",
				n, sector_row, sector_column,
				(long) byte_offset + (long) image_data));

#if MXD_AVIEX_PCCD_DEBUG_DESCRAMBLING
			mxd_aviex_pccd_display_ul_corners( sector_array,
								num_sectors );
#endif
		    }
#endif

#if MXD_AVIEX_PCCD_DEBUG_ALLOCATION_DETAILS
		    MX_DEBUG(-2,
	    ("offset = %#lx = %#lx * (%#lx) + %#lx * (%#lx) + %#lx * (%#lx)",
			byte_offset, sizeof_row_of_sectors, sector_row,
			sizeof_full_row, row,
			sizeof_sector_row, sector_column));
#endif

#if MXD_AVIEX_PCCD_DEBUG_ALLOCATION_DETAILS
		    MX_DEBUG(-2,
	    ("offset = %ld = %ld * (%ld) + %ld * (%ld) + %ld * (%ld)",
			byte_offset, sizeof_row_of_sectors, sector_row,
			sizeof_full_row, row,
			sizeof_sector_row, sector_column));
#endif
		}
	    }
	}

	*sector_array_ptr = sector_array;

#if MXD_AVIEX_PCCD_DEBUG_ALLOCATION
	mxd_aviex_pccd_display_ul_corners( sector_array, num_sectors );

#if 0
	fprintf(stderr, "Type any key to continue..." );
	mx_getch();
	fprintf(stderr, "\n");
#endif
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT void
mxd_aviex_pccd_free_sector_array( uint16_t ***sector_array )
{
	uint16_t **sector_array_row_ptr;

	if ( sector_array == NULL ) {
		return;
	}

	sector_array_row_ptr = *sector_array;

	if ( sector_array_row_ptr != NULL ) {
		free( sector_array_row_ptr );
	}

	free( sector_array );

	return;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_aviex_pccd_load_linearity_lookup_table( MX_AVIEX_PCCD *aviex_pccd )
{
	static const char fname[] =
		"mxd_aviex_pccd_load_linearity_lookup_table()";

	FILE *file;
	int saved_errno;
	unsigned long table_size, bytes_read;

#if MXD_AVIEX_PCCD_DEBUG_LINEARITY_LOOKUP
	MX_DEBUG(-2,("%s invoked for detector '%s'.",
		fname, aviex_pccd->record->name ));
#endif

	/* The linearity lookup file is a binary file in 'native' byte order
	 * that contains 65536 16-bit integers.  No byte swapping is required.
	 */

	file = fopen( aviex_pccd->linearity_lookup_table_filename, "rb" );

	if ( file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The attempt to open linearity lookup table file '%s' failed.  "
		"Errno = %d, error message = '%s'",
			aviex_pccd->linearity_lookup_table_filename,
			saved_errno, mx_strerror( saved_errno, NULL, 0 ) );
	}

	/* Allocate memory for the linearity lookup table. */

	aviex_pccd->num_linearity_lookup_table_entries =
					65536 * aviex_pccd->num_ccd_taps;

#if MXD_AVIEX_PCCD_DEBUG_LINEARITY_LOOKUP
	MX_DEBUG(-2,
	("%s: num_ccd_taps = %lu, num_linearity_lookup_table_entries = %lu",
		fname, aviex_pccd->num_ccd_taps,
		aviex_pccd->num_linearity_lookup_table_entries));
#endif

	table_size = sizeof(uint16_t) *
			aviex_pccd->num_linearity_lookup_table_entries;

	aviex_pccd->linearity_lookup_table = malloc( table_size );

	if ( aviex_pccd->linearity_lookup_table == NULL ) {
		fclose( file );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu byte "
		"linearity lookup table.", table_size );
	}

	/* Read the contents of the linearity lookup table file into the 
	 * local data structure.
	 */

	bytes_read = fread( aviex_pccd->linearity_lookup_table,
				1, table_size, file );

	fclose( file );

	if ( bytes_read < table_size ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Only %lu bytes were read from the linearity lookup table "
		"file '%s'.  The file was supposed to be %lu bytes long.",
			bytes_read, aviex_pccd->linearity_lookup_table_filename,
			table_size );
	}
	
	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_aviex_pccd_descramble_image( MX_AREA_DETECTOR *ad,
				MX_AVIEX_PCCD *aviex_pccd,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *raw_frame )
{
	static const char fname[] = "mxd_aviex_pccd_descramble_image()";

	MX_SEQUENCE_PARAMETERS *sp;
	long i, i_framesize, j_framesize;
	long row_framesize, column_framesize;
	long num_sector_rows, num_sector_columns;
	uint16_t *frame_data;
	unsigned long pccd_flags;
	mx_bool_type use_linearity_lookup_table;
	mx_status_type mx_status;

#if MXD_AVIEX_PCCD_DEBUG_TIMING
	MX_HRT_TIMING memcpy_measurement;
	MX_HRT_TIMING total_measurement;

	/* Shut up useless GCC 4 initialized variable warning. */

	memset( &memcpy_measurement, 0, sizeof(MX_HRT_TIMING) );

	MX_HRT_START(total_measurement);
#endif
	/* Shut up useless GCC 4 initialized variable warnings. */

	i_framesize = j_framesize = 0;

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}
	if ( raw_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The raw_frame pointer passed was NULL." );
	}

	/*---*/

	pccd_flags = aviex_pccd->aviex_pccd_flags;

	if ( (pccd_flags & MXF_AVIEX_PCCD_USE_BIAS_LOOKUP_TABLE)
	  && (aviex_pccd->linearity_lookup_table != NULL) )
	{
		use_linearity_lookup_table = TRUE;
	} else {
		use_linearity_lookup_table = FALSE;
	}

	/*---*/

	sp = &(ad->sequence_parameters);

	switch( ad->record->mx_type ) {
	case MXT_AD_PCCD_170170:
		num_sector_rows = 4;
		num_sector_columns = 4;
		break;
	case MXT_AD_PCCD_4824:
		num_sector_rows = 2;
		num_sector_columns = 2;
		break;
	case MXT_AD_PCCD_16080:
		num_sector_rows = 2;
		num_sector_columns = 4;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal MX driver type '%s' passed for record '%s'.",
			mx_get_driver_name( ad->record ),
			ad->record->name );
		break;
	}

	/* We handle streak camera descrambling in a special function
	 * just for it.
	 */

	if ( sp->sequence_type == MXT_SQ_STREAK_CAMERA ) {
		switch( ad->record->mx_type ) {
		case MXT_AD_PCCD_170170:
			mx_status =
			    mxd_aviex_pccd_170170_streak_camera_descramble(
				ad, aviex_pccd, image_frame, raw_frame );
			break;
		case MXT_AD_PCCD_4824:
			mx_status =
			    mxd_aviex_pccd_4824_streak_camera_descramble(
				ad, aviex_pccd, image_frame, raw_frame );
			break;
		case MXT_AD_PCCD_16080:
			mx_status =
			    mxd_aviex_pccd_16080_streak_camera_descramble(
				ad, aviex_pccd, image_frame, raw_frame );
			break;
		}

#if MXD_AVIEX_PCCD_DEBUG_DESCRAMBLING
		MX_DEBUG(-2,
		("%s: Streak camera descrambling complete.", fname));
#endif
		return mx_status;
	}

	/* If this is a subimage frame, we initially
	 * descramble to a temporary frame.
	 */

	if (sp->sequence_type == MXT_SQ_SUBIMAGE) {

#if MXD_AVIEX_PCCD_DEBUG_MX_IMAGE_ALLOC
		MX_DEBUG(-2,
	("%s: Invoking mx_image_alloc() for aviex_pccd->temp_frame = %p",
			fname, aviex_pccd->temp_frame ));
#endif

		mx_status = mx_image_alloc( &(aviex_pccd->temp_frame),
					MXIF_ROW_FRAMESIZE(image_frame),
					MXIF_COLUMN_FRAMESIZE(image_frame),
					MXIF_IMAGE_FORMAT(image_frame),
					MXIF_BYTE_ORDER(image_frame),
					MXIF_BYTES_PER_PIXEL(image_frame),
					image_frame->header_length,
					image_frame->image_length );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		frame_data = aviex_pccd->temp_frame->image_data;
	} else {
		frame_data = image_frame->image_data;
	}

#if MXD_AVIEX_PCCD_DEBUG_DESCRAMBLING
	{
		long num_pixels;

		num_pixels = image_frame->allocated_image_length / 2;

		for ( i = 0; i < num_pixels; i++ ) {
			frame_data[i] = 0x3fff;
		}
	}
#endif

	/* For each frame, we overlay the frame with 16 sets of sector array
	 * pointers so that we can treat each of the 16 sectors as independent
	 * two dimensional arrays.
	 */

	/* The sector_array pointer will be NULL, if any of the following
	 * has happened.
	 *
	 * 1. This is the first frame taken by the program.
	 *
	 * 2. The framesize or detector mode has changed
	 *      (full frame, subimage, streak camera).
	 */

	/* Initially descramble the full image. */

	column_framesize = MXIF_COLUMN_FRAMESIZE(image_frame);
	row_framesize = MXIF_ROW_FRAMESIZE(image_frame);

	switch( ad->record->mx_type ) {
	case MXT_AD_PCCD_170170:
		i_framesize = column_framesize / 4;
		j_framesize = row_framesize / 4;
		break;
	case MXT_AD_PCCD_4824:
		i_framesize = column_framesize / 2;
		j_framesize = row_framesize / 2;
		break;
	case MXT_AD_PCCD_16080:
		i_framesize = column_framesize / 2;
		j_framesize = row_framesize / 4;
		break;
	}

	if ( aviex_pccd->sector_array == NULL ) {

		mx_status = mxd_aviex_pccd_alloc_sector_array(
				&(aviex_pccd->sector_array),
				j_framesize, i_framesize,
				num_sector_rows, num_sector_columns,
				frame_data );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Copy and descramble the pixels from the raw frame to the image frame.
	 */
	switch( ad->record->mx_type ) {
	case MXT_AD_PCCD_170170:
		if ( use_linearity_lookup_table ) {
			mx_status =
			    mxd_aviex_pccd_170170_linearity_descramble(
					raw_frame->image_data,
					aviex_pccd->sector_array,
					i_framesize, j_framesize,
					aviex_pccd->linearity_lookup_table );
		} else {
			mx_status = mxd_aviex_pccd_170170_descramble(
					raw_frame->image_data,
					aviex_pccd->sector_array,
					i_framesize, j_framesize );
		}
		break;
	case MXT_AD_PCCD_4824:
		if ( use_linearity_lookup_table ) {
			mx_status =
			    mxd_aviex_pccd_4824_linearity_descramble(
					raw_frame->image_data,
					aviex_pccd->sector_array,
					i_framesize, j_framesize,
					aviex_pccd->linearity_lookup_table );
		} else {
			mx_status = mxd_aviex_pccd_4824_descramble(
					raw_frame->image_data,
					aviex_pccd->sector_array,
					i_framesize, j_framesize );
		}
		break;
	case MXT_AD_PCCD_16080:
		mx_status = mxd_aviex_pccd_16080_descramble(
					raw_frame->image_data,
					aviex_pccd->sector_array,
					i_framesize, j_framesize );
		break;
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if (sp->sequence_type == MXT_SQ_SUBIMAGE) {

		long num_lines_per_subimage, num_subimages, frame_width;
		long bytes_per_half_subimage, bytes_per_half_image;
		long top_offset, bottom_offset, use_top_half;
		char *temp_ptr, *image_ptr;
		char *top_src_ptr, *top_dest_ptr;
		char *bottom_src_ptr, *bottom_dest_ptr;

		num_lines_per_subimage = sp->parameter_array[0];
		num_subimages          = sp->parameter_array[1];
		frame_width            = MXIF_ROW_FRAMESIZE(image_frame);

		temp_ptr  = aviex_pccd->temp_frame->image_data;
		image_ptr = image_frame->image_data;

		bytes_per_half_subimage =
			frame_width * num_lines_per_subimage
			* ( sizeof(uint16_t) / 2L );

		bytes_per_half_image = MXIF_ROW_FRAMESIZE(image_frame)
				* MXIF_COLUMN_FRAMESIZE(image_frame)
				* ( sizeof(uint16_t) / 2L );

#if MXD_AVIEX_PCCD_DEBUG_DESCRAMBLING
		MX_DEBUG(-2,("%s: num_lines_per_subimage = %ld",
			fname, num_lines_per_subimage));
		MX_DEBUG(-2,("%s: num_subimages = %ld", fname, num_subimages));
		MX_DEBUG(-2,("%s: frame_width = %ld", fname, frame_width));
		MX_DEBUG(-2,("%s: bytes_per_half_subimage = %ld",
			fname, bytes_per_half_subimage));
		MX_DEBUG(-2,("%s: bytes_per_half_image = %ld",
			fname, bytes_per_half_image));
#endif

		/* Check the flag that tells subimage mode to use the top
		 * half of the detector rather than the bottom half.
		 */

		if ( aviex_pccd->use_top_half_of_detector ) {
			use_top_half = 1L;
		} else {
			use_top_half = 0L;
		}

#if MXD_AVIEX_PCCD_DEBUG_TIMING
		MX_HRT_START( memcpy_measurement );
#endif

		for ( i = 0; i < num_subimages; i++ ) {
			top_offset =
				(1L - use_top_half) * bytes_per_half_image
					+ i * bytes_per_half_subimage;

			top_src_ptr = temp_ptr + top_offset;

			top_dest_ptr = image_ptr
				+ (2L * i) * bytes_per_half_subimage;

			memcpy( top_dest_ptr, top_src_ptr,
					bytes_per_half_subimage );

			bottom_offset =
				(2L - use_top_half) * bytes_per_half_image
					- (i+1L) * bytes_per_half_subimage;

			bottom_src_ptr = temp_ptr + bottom_offset;

			bottom_dest_ptr = image_ptr
				+ ((2L * i) + 1L) * bytes_per_half_subimage;

			memcpy( bottom_dest_ptr, bottom_src_ptr,
					bytes_per_half_subimage );
		}

#if MXD_AVIEX_PCCD_DEBUG_TIMING
		MX_HRT_END( memcpy_measurement );
#endif

#if 1
		/* Patch the column framesize and the image length so that
		 * it matches the total size of the subimage frame.
		 */

		MXIF_COLUMN_FRAMESIZE(image_frame) =
					num_subimages * num_lines_per_subimage;

		image_frame->image_length =
			num_subimages * bytes_per_half_subimage * 2L;
#endif
	}

#if MXD_AVIEX_PCCD_DEBUG_TIMING
	MX_HRT_END( total_measurement );

	MX_HRT_RESULTS( memcpy_measurement, fname, "for subimage memcpy." );
	MX_HRT_RESULTS( total_measurement, fname, "for total descramble." );
#endif

#if MXD_AVIEX_PCCD_DEBUG_DESCRAMBLING
	MX_DEBUG(-2,("%s: Image descrambling complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_aviex_pccd_check_value( MX_AVIEX_PCCD *aviex_pccd,
				unsigned long register_address,
				unsigned long register_value,
				MX_AVIEX_PCCD_REGISTER **register_pointer )
{
	static const char fname[] = "mxd_aviex_pccd_check_value()";

	MX_AVIEX_PCCD_REGISTER *reg;

	if ( register_address >= aviex_pccd->num_registers ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal register address %lu.", register_address );
	}

	reg = &(aviex_pccd->register_array[register_address]);

	if ( reg->read_only ) {
		return mx_error( MXE_READ_ONLY, fname,
			"Register %lu is read only.", register_address );
	}

	if ( reg->power_of_two ) {
		if ( mx_is_power_of_two( register_value ) == FALSE ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested value %lu for register '%s' (%lu) "
			"is not valid since the value is not a power of two.",
				register_value, reg->name, register_address );
		}
	}

	if ( register_value < reg->minimum ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested value %lu for register '%s' (%lu) "
		"is less than the minimum allowed value of %lu.",
			register_value, reg->name,
			register_address, reg->minimum );
	}

	if ( register_value > reg->maximum ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested value %lu for register '%s' (%lu) "
		"is greater than the maximum allowed value of %lu.",
			register_value, reg->name,
			register_address, reg->maximum );
	}

	if ( register_pointer != (MX_AVIEX_PCCD_REGISTER **) NULL ) {
		*register_pointer = reg;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_init_register( MX_AVIEX_PCCD *aviex_pccd,
				long register_index,
				mx_bool_type use_low_byte_name,
				int register_size,		/* in bytes */
				unsigned long register_value,
				mx_bool_type read_only,
				mx_bool_type write_only,
				mx_bool_type power_of_two,
				unsigned long minimum,
				unsigned long maximum )
{
	static const char fname[] = "mxd_aviex_pccd_init_register()";

	MX_AVIEX_PCCD_REGISTER *reg;
	long register_address;
	MX_RECORD_FIELD *register_field;
	mx_status_type mx_status;

	register_address = register_index - MXLV_AVIEX_PCCD_DH_BASE;

	if ( register_address < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal register address %ld.", register_address );
	} else
	if ( register_address >= aviex_pccd->num_registers ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal register address %ld.", register_address );
	}

	reg = &(aviex_pccd->register_array[register_address]);

	reg->size          = register_size;
	reg->value         = register_value;
	reg->read_only     = read_only;
	reg->write_only    = write_only;
	reg->power_of_two  = power_of_two;
	reg->minimum       = minimum;
	reg->maximum       = maximum;
	reg->name          = NULL;

	if ( use_low_byte_name ) {
		register_index--;
	}

	mx_status = mx_get_field_by_label_value( aviex_pccd->record,
						register_index,
						&register_field );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	reg->name = register_field->name;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_aviex_pccd_simulated_cl_command( MX_AVIEX_PCCD *aviex_pccd,
					char *command,
					char *response,
					size_t max_response_length,
					int debug_flag )
{
	static const char fname[] = "mxd_aviex_pccd_simulated_cl_command()";

	MX_AVIEX_PCCD_REGISTER *reg;
	unsigned long register_address, register_value, combined_value;
	int num_items;
	mx_status_type mx_status;

	switch( command[0] ) {
	case 'A':
		num_items = sscanf( command, "A%lu", &register_address );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Illegal command '%s' sent to detector '%s'.",
				command, aviex_pccd->record->name );
		}

		if ( register_address >= aviex_pccd->num_registers ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Illegal register address %lu.",
				register_address );
		}

		reg = &(aviex_pccd->register_array[register_address]);

		switch( aviex_pccd->record->mx_type ) {
		case MXT_AD_PCCD_16080:
			snprintf( response, max_response_length,
				"B%05lu", reg->value );
			break;
		case MXT_AD_PCCD_170170:
		case MXT_AD_PCCD_4824:
			strlcpy( response, "E", max_response_length );
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported Aviex record type %lu",
				aviex_pccd->record->mx_type );
			break;
		}
		break;
	case 'R':
		num_items = sscanf( command, "R%lu", &register_address );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Illegal command '%s' sent to detector '%s'.",
				command, aviex_pccd->record->name );
		}

		if ( register_address >= aviex_pccd->num_registers ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Illegal register address %lu.",
				register_address );
		}

		reg = &(aviex_pccd->register_array[register_address]);

		switch( aviex_pccd->record->mx_type ) {
		case MXT_AD_PCCD_16080:
			snprintf( response, max_response_length,
				"S%03lu", reg->value );
			break;
		case MXT_AD_PCCD_170170:
		case MXT_AD_PCCD_4824:
			snprintf( response, max_response_length,
				"S%05lu", reg->value );
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported Aviex record type %lu",
				aviex_pccd->record->mx_type );
			break;
		}
		break;
	case 'W':
		num_items = sscanf( command, "W%lu", &combined_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Illegal command '%s' sent to detector '%s'.",
				command, aviex_pccd->record->name );
		}

		switch( aviex_pccd->record->mx_type ) {
		case MXT_AD_PCCD_16080:
			register_address = combined_value / 1000;
			register_value   = combined_value % 1000;
			break;
		case MXT_AD_PCCD_170170:
		case MXT_AD_PCCD_4824:
			register_address = combined_value / 100000;
			register_value   = combined_value % 100000;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported Aviex record type %lu",
				aviex_pccd->record->mx_type );
			break;
		}

		mx_status = mxd_aviex_pccd_check_value( aviex_pccd,
							register_address,
							register_value,
							&reg );

		if ( mx_status.code != MXE_SUCCESS ) {
			strlcpy( response, "E", max_response_length );
		} else {
			strlcpy( response, "X", max_response_length );

			reg->value = register_value;
		}
		break;
	default:
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxp_aviex_pccd_epix_save_start_timespec( MX_AVIEX_PCCD *aviex_pccd )
{
	static const char fname[] = "mxp_aviex_pccd_epix_save_start_timespec()";

	MX_EPIX_XCLIB *xclib;
	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	struct timespec absolute_timespec, relative_timespec;

	epix_xclib_vinput = aviex_pccd->video_input_record->record_type_struct;

	if ( epix_xclib_vinput == (MX_EPIX_XCLIB_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPIX_XCLIB_VIDEO_INPUT pointer "
		"for record '%s' is NULL.",
			aviex_pccd->video_input_record->name );
	}
	if ( epix_xclib_vinput->xclib_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The xclib_record pointer for record '%s' is NULL.",
			epix_xclib_vinput->record->name );
	}

	xclib = epix_xclib_vinput->xclib_record->record_type_struct;

	if ( xclib == (MX_EPIX_XCLIB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPIX_XCLIB pointer for record '%s' is NULL.",
			epix_xclib_vinput->xclib_record->name );
	}

	relative_timespec = mx_high_resolution_time();

	absolute_timespec = mx_add_high_resolution_times(
					xclib->system_boot_timespec,
					relative_timespec );

	xclib->sequence_start_timespec = absolute_timespec;

#if MXD_AVIEX_PCCD_DEBUG
	MX_DEBUG(-2,("%s:\n"
		"sequence_start_timespec = (%ld,%ld),\n"
		"system_boot_timespec    = (%ld,%ld),\n"
		"relative_timespec       = (%ld,%ld)",
			fname, (long) xclib->sequence_start_timespec.tv_sec,
			xclib->sequence_start_timespec.tv_nsec,
			(long) xclib->system_boot_timespec.tv_sec,
			xclib->system_boot_timespec.tv_nsec,
			(long) relative_timespec.tv_sec,
			relative_timespec.tv_nsec));
#endif
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_initialize_type( long record_type )
{
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	long num_record_fields;
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_type( record_type,
					&num_record_fields,
					&record_field_defaults,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_aviex_pccd_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_AVIEX_PCCD *aviex_pccd;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	aviex_pccd = (MX_AVIEX_PCCD *) malloc( sizeof(MX_AVIEX_PCCD) );

	if ( aviex_pccd == (MX_AVIEX_PCCD *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AVIEX_PCCD structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = aviex_pccd;
	record->class_specific_function_list = &mxd_aviex_pccd_ad_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	aviex_pccd->record = record;

	aviex_pccd->first_dh_command = TRUE;

	aviex_pccd->horiz_descramble_factor = -1;
	aviex_pccd->vert_descramble_factor = -1;

	/* Set the default file format. */

	ad->frame_file_format = MXT_IMAGE_FILE_SMV;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_delete_record( MX_RECORD *record )
{
	static const char fname[] = "mxd_aviex_pccd_delete_record()";

	MX_AREA_DETECTOR *ad;
	MX_AVIEX_PCCD *aviex_pccd;
	mx_status_type mx_status;

	aviex_pccd = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_aviex_pccd_get_pointers( ad, &aviex_pccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( aviex_pccd != NULL ) {
		if ( aviex_pccd->raw_frame != NULL ) {
			mx_free( aviex_pccd->raw_frame );
		}
		mx_free( aviex_pccd );
	}

	if ( ad != NULL ) {
		mx_free( ad );
	}

	if ( record != NULL ) {
		mx_free( record );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_aviex_pccd_open()";

	MX_AREA_DETECTOR *ad;
	MX_AVIEX_PCCD *aviex_pccd;
	MX_RECORD *video_input_record;
	MX_VIDEO_INPUT *vinput;
	long vinput_framesize[2];
	long ad_binsize[2];
	unsigned long ad_flags, pccd_flags;
	long x_framesize, y_framesize;
	long master_clock;
	struct timespec hrt;
	mx_bool_type camera_is_master;
	mx_status_type mx_status;

	aviex_pccd = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_aviex_pccd_get_pointers( ad, &aviex_pccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVIEX_PCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	/* Set the datafile format to the frame file format. */

	/* FIXME: Are both of these necessary? */

	ad->datafile_format = ad->frame_file_format;

	ad_flags = ad->area_detector_flags;

	pccd_flags = aviex_pccd->aviex_pccd_flags;

#if MXD_AVIEX_PCCD_DEBUG
	MX_DEBUG(-2,("%s: area_detector_flags = %#lx", fname, ad_flags));
	MX_DEBUG(-2,("%s: aviex_pccd_flags = %#lx", fname, pccd_flags));
#endif

	if ( pccd_flags & MXF_AVIEX_PCCD_SUPPRESS_DESCRAMBLING ) {
		mx_warning( "Area detector '%s' will not descramble "
			"images from the camera head.",
				record->name );
	}
	if ( pccd_flags & MXF_AVIEX_PCCD_USE_DETECTOR_HEAD_SIMULATOR ) {
		mx_warning( "Area detector '%s' will use "
	"an Aviex detector head simulator instead of a real detector head.",
				record->name );
	}

	video_input_record = aviex_pccd->video_input_record;

	if ( video_input_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The video_input_record pointer for area detector '%s' is NULL.",
			record->name );
	}

	vinput = video_input_record->record_class_struct;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VIDEO_INPUT pointer for video input record '%s' "
		"used by area detector '%s' is NULL.",
			video_input_record->name, record->name );
	}

	/* Automatic loading of image frames does not make any sense for
	 * this detector.
	 */

	if ( ad_flags & MXF_AD_LOAD_FRAME_AFTER_ACQUISITION ) {
	    ad->area_detector_flags &= (~MXF_AD_LOAD_FRAME_AFTER_ACQUISITION);

	    (void) mx_error( MXE_UNSUPPORTED, fname,
		"Automatic loading of image frames from disk files "
		"after an acquisition sequence "
		"is not supported for area detector '%s', since that "
		"would result in the loss of all data from the detector.  "
		"The load frame flag has been turned off.",
			record->name );
	}

	/* See if the user has requested the saving of image frames by MX. */

	if ( ad_flags & MXF_AD_SAVE_FRAME_AFTER_ACQUISITION ) {

		MX_DEBUG(-2,("%s: Enabling automatic saving of image frames "
		"for area detector '%s'.", fname, record->name ));

		mx_status =
		    mx_area_detector_setup_datafile_management( record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Initialize the geometrical mask frames. */

	aviex_pccd->geometrical_mask_frame = NULL;
	aviex_pccd->rebinned_geometrical_mask_frame = NULL;

	/* Set a limit on the range of values for
	 * the flood field scaling factor.
	 */

	ad->flood_field_scale_max = 10.0;
	ad->flood_field_scale_min = 0.1;

	/* Set the step size in seconds per step
	 * for the exposure time and the gap time.
	 */

	switch( ad->record->mx_type ) {
	case MXT_AD_PCCD_170170:
	case MXT_AD_PCCD_4824:
		aviex_pccd->exposure_and_gap_step_size = 0.001;
		break;
	case MXT_AD_PCCD_16080:
		aviex_pccd->exposure_and_gap_step_size = 0.01;
		break;
	}

	/* Make sure the internal trigger output is low. */

	mx_status = mx_digital_output_write(
				aviex_pccd->internal_trigger_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off the buffer overrun flag. */

	aviex_pccd->buffer_overrun = FALSE;

	/* Turn off the flag that tells the subimage and streak camera modes
	 * to use the top half of the detector rather than the bottom half.
	 */

	aviex_pccd->use_top_half_of_detector = FALSE;

	/*-------------------------------------------------------------------*/

	/* Some of the detector initialization steps are specific to the
	 * kind of detector in use.  Therefore, we call type-specific
	 * initialization functions here.
	 */

	switch( record->mx_type ) {
	case MXT_AD_PCCD_170170:
		mx_status = mxd_aviex_pccd_170170_initialize_detector(
					    record, ad, aviex_pccd, vinput );
		break;
	case MXT_AD_PCCD_4824:
		mx_status = mxd_aviex_pccd_4824_initialize_detector(
					    record, ad, aviex_pccd, vinput );
		break;
	case MXT_AD_PCCD_16080:
		mx_status = mxd_aviex_pccd_16080_initialize_detector(
					    record, ad, aviex_pccd, vinput );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported Aviex detector type %lu", record->mx_type );
		break;
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*-------------------------------------------------------------------*/

	/* If requested, load the linearity lookup table. */

	if ( strlen(aviex_pccd->linearity_lookup_table_filename) > 0 ) {

		mx_status =
		    mxd_aviex_pccd_load_linearity_lookup_table( aviex_pccd );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else {
		aviex_pccd->linearity_lookup_table = NULL;
	}

	/*-------------------------------------------------------------------*/

	/* Initialize the bytes per pixel from the video driver. */

	mx_status = mx_area_detector_get_bytes_per_pixel( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the default framesize and binning. */

	ad->framesize[0] = ad->maximum_framesize[0];
	ad->framesize[1] = ad->maximum_framesize[1];

	mx_status = mx_area_detector_set_binsize( record, 1, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Generate a warning if the video board was not able to get
	 * enough memory for a even just a single frame.
	 */

	mx_status = mx_area_detector_get_framesize( record,
						&x_framesize, &y_framesize );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVIEX_PCCD_DEBUG
	MX_DEBUG(-2,("%s: x_framesize = %ld, y_framesize = %ld",
		fname, x_framesize, y_framesize));
#endif

	if ( ( x_framesize < ad->maximum_framesize[0] )
	  || ( y_framesize < ad->maximum_framesize[1] ) )
	{
		mx_warning( "Area detector '%s' was not able to acquire "
		"enough memory for even a single full frame.  "
		"Desired framesize = (%ld,%ld).  "
		"Actual framesize = (%ld,%ld).  "
		"This can happen if you have run a large number of "
		"application programs before starting MX.  "
		"Generally you can fix this by starting MX "
		"as soon after the computer boots as is possible.",
			record->name,
			ad->maximum_framesize[0],
			ad->maximum_framesize[1],
			x_framesize, y_framesize );
	}

	/* Set the pixel clock frequency in the video card. */

	mx_status = mx_video_input_set_pixel_clock_frequency(
		video_input_record, aviex_pccd->pixel_clock_frequency );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure the video input's external trigger to trigger on
	 * the rising edge of the trigger pulse.
	 */

	mx_status = mx_video_input_set_external_trigger_polarity(
				video_input_record, MXF_VIN_TRIGGER_RISING );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure the video input to use negative Camera Link pulses. */

	mx_status = mx_video_input_set_camera_trigger_polarity(
				video_input_record, MXF_VIN_TRIGGER_LOW );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the video input's initial trigger mode (internal/external/etc) */

	mx_status = mx_video_input_set_trigger_mode( video_input_record,
				aviex_pccd->initial_trigger_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Specify whether or not the camera head or the video board is
	 * in charge of generating the trigger pulse for sequences.
	 */

	camera_is_master =
	   (aviex_pccd->aviex_pccd_flags & MXF_AVIEX_PCCD_CAMERA_IS_MASTER);

	if ( camera_is_master ) {
		master_clock = MXF_VIN_MASTER_CAMERA;
	} else {
		master_clock = MXF_VIN_MASTER_VIDEO_BOARD;
	}

	mx_status = mx_video_input_set_master_clock( video_input_record,
							master_clock );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize area detector parameters. */

	ad->byte_order = mx_native_byteorder();
	ad->header_length = 0;

	mx_status = mx_area_detector_get_image_format( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_bytes_per_frame( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_bits_per_pixel( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize some parameters used to switch between streak camera
	 * mode and other modes.
	 */

	mx_status = mx_video_input_get_framesize( video_input_record,
						&vinput_framesize[0],
						&vinput_framesize[1] );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	aviex_pccd->vinput_normal_framesize[0] = vinput_framesize[0];
	aviex_pccd->vinput_normal_framesize[1] = vinput_framesize[1];

	mx_status = mx_area_detector_get_binsize( record,
						&ad_binsize[0], &ad_binsize[1]);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	aviex_pccd->normal_binsize[0] = ad_binsize[0];
	aviex_pccd->normal_binsize[1] = ad_binsize[1];

	/* Allocate space for a raw frame buffer.  This buffer will be used to
	 * read in the raw pixels from the imaging board before descrambling.
	 */

#if MXD_AVIEX_PCCD_DEBUG_MX_IMAGE_ALLOC
	MX_DEBUG(-2,
	("%s: Invoking mx_image_alloc() for aviex_pccd->raw_frame = %p",
		fname, aviex_pccd->raw_frame ));
#endif

	mx_status = mx_image_alloc( &(aviex_pccd->raw_frame),
					vinput_framesize[0],
					vinput_framesize[1],
					ad->image_format,
					ad->byte_order,
					ad->bytes_per_pixel,
					ad->header_length,
					ad->bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MXIF_ROW_BINSIZE(aviex_pccd->raw_frame) = 1;
	MXIF_COLUMN_BINSIZE(aviex_pccd->raw_frame) = 1;
	MXIF_BITS_PER_PIXEL(aviex_pccd->raw_frame) = ad->bits_per_pixel;

	aviex_pccd->old_framesize[0] = -1;
	aviex_pccd->old_framesize[1] = -1;

	aviex_pccd->sector_array = NULL;

	/* Load the image correction files. */

	mx_status = mx_area_detector_load_correction_files( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the detector to one-shot mode with an exposure time
	 * of 1 second.
	 */

	mx_status = mx_area_detector_set_one_shot_mode( record, 1.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pccd_flags & MXF_AVIEX_PCCD_TEST_DEZINGER ) {

		/* If we are doing a dezinger test, provide a seed value for
		 * the random number generator.  The numbers that we generate
		 * here do not need to be very random, so we just use the
		 * computer's high resolution clock to generate the seed.
		 */
#if 1
		hrt.tv_sec = 1;
#else
		hrt = mx_high_resolution_time();
#endif
		srand( hrt.tv_sec );

		ad->dezinger_threshold = 2.0;
	}

#if MXD_AVIEX_PCCD_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_aviex_pccd_resynchronize()";

	MX_AREA_DETECTOR *ad;
	MX_AVIEX_PCCD *aviex_pccd;
	mx_status_type mx_status;

	aviex_pccd = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_aviex_pccd_get_pointers( ad, &aviex_pccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVIEX_PCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	/* Send an 'Initialize CCD Controller' command to the detector head
	 * by toggling the CC4 line.
	 */

#if 0
	mx_status = mx_camera_link_pulse_cc_line(
					aviex_pccd->camera_link_record, 4,
					-1, 1000 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	/* Restart the PIXCI library. */

	mx_status = mx_resynchronize_record( aviex_pccd->video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_aviex_pccd_arm()";

	MX_AVIEX_PCCD *aviex_pccd;
	MX_VIDEO_INPUT *vinput;
	MX_SEQUENCE_PARAMETERS *sp;
	MX_CLOCK_TICK num_ticks_to_wait, finish_tick;
	double timeout_in_seconds;
	int comparison;
	long num_frames_in_sequence, master_clock;
	unsigned long status_flags;
	mx_bool_type camera_is_master, external_trigger, circular;
	mx_status_type mx_status;

	aviex_pccd = NULL;

	mx_status = mxd_aviex_pccd_get_pointers( ad, &aviex_pccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( aviex_pccd->video_input_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The video_input_record pointer for area detector '%s' is NULL.",
			ad->record->name );
	}

	DISPLAY_CONTROL_REGISTER(fname);

	vinput = aviex_pccd->video_input_record->record_class_struct;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VIDEO_INPUT pointer for video input '%s' "
		"used by area detector '%s' is NULL.",
			aviex_pccd->video_input_record->name,
			ad->record->name );
	}

#if MXD_AVIEX_PCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif
	sp = &(ad->sequence_parameters);

	camera_is_master =
	   (aviex_pccd->aviex_pccd_flags & MXF_AVIEX_PCCD_CAMERA_IS_MASTER);

	external_trigger = (vinput->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER);

#if MXD_AVIEX_PCCD_DEBUG
	MX_DEBUG(-2,("%s: camera_is_master = %d, external_trigger = %d",
		fname, (int) camera_is_master, (int) external_trigger));
#endif

	switch ( sp->sequence_type ) {
	case MXT_SQ_CONTINUOUS:
		/* For continuous sequences, we must use video board
		 * CC1 pulses to trigger the taking of frames, so that
		 * the total number of frames is not limited.
		 */

		camera_is_master = FALSE;
		break;

	case MXT_SQ_STROBE:
		/* For strobe sequences, the external trigger goes into
		 * the video card, which triggers the taking of frames
		 * via CC1 pulses.
		 */

		camera_is_master = FALSE;
		break;
	}

	/* Stop any currently running imaging sequence. */

	if ( ad->record->mx_type != MXT_AD_PCCD_16080 ) {
		mx_status = mx_video_input_abort(
					aviex_pccd->video_input_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Wait for up to 5 seconds for the video input to stop. */

	timeout_in_seconds = 5.0;

	num_ticks_to_wait =
		mx_convert_seconds_to_clock_ticks( timeout_in_seconds );

	finish_tick = mx_add_clock_ticks( mx_current_clock_tick(),
						num_ticks_to_wait );

	while(1) {
		mx_status = mx_area_detector_get_status( ad->record,
							&status_flags );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (status_flags & MXSF_AD_ACQUISITION_IN_PROGRESS) == 0 )
		{
			break;		/* Exit the while() loop. */
		}

		comparison = mx_compare_clock_ticks( mx_current_clock_tick(),
							finish_tick );

		if ( comparison >= 0 ) {
			(void) mx_error( MXE_TIMED_OUT, fname,
			"Area detector '%s' timed out after waiting %g seconds "
			"for video input '%s' to stop acquiring frames.",
				ad->record->name,
				timeout_in_seconds, vinput->record->name );

			break;		/* Exit the while(1) loop. */
		}

		mx_usleep(1000);
	}

	/* Select the master clock. */

	if ( camera_is_master ) {
		master_clock = MXF_VIN_MASTER_CAMERA;
	} else {
		master_clock = MXF_VIN_MASTER_VIDEO_BOARD;
	}

	mx_status = mx_video_input_set_master_clock( vinput->record,
							master_clock );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off the buffer overrun flag. */

	aviex_pccd->buffer_overrun = FALSE;

	/* Configure the detector head to use either an internal or
	 * an external trigger.
	 */

	switch( ad->record->mx_type ) {
	case MXT_AD_PCCD_170170:
		mx_status = mxd_aviex_pccd_170170_set_external_trigger_mode(
						aviex_pccd, external_trigger );
		break;
	case MXT_AD_PCCD_4824:
		mx_status = mxd_aviex_pccd_4824_set_external_trigger_mode(
						aviex_pccd, external_trigger );
		break;
	case MXT_AD_PCCD_16080:
		mx_status = mxd_aviex_pccd_16080_set_external_trigger_mode(
						aviex_pccd, external_trigger );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Attempted to set external trigger mode for unsupported "
		"record type %lu of record '%s'.",
			ad->record->mx_type, ad->record->name );
		break;
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Prepare the video input for the next trigger. */

	if ( camera_is_master && external_trigger ) {

		mx_status = mx_sequence_get_num_frames(
						&(ad->sequence_parameters),
						&num_frames_in_sequence );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( sp->sequence_type ) {
		case MXT_SQ_CONTINUOUS:
		case MXT_SQ_CIRCULAR_MULTIFRAME:
			circular = TRUE;
			break;
		default:
			circular = FALSE;
			break;
		}

		mx_status = mx_video_input_asynchronous_capture(
					aviex_pccd->video_input_record,
					num_frames_in_sequence, circular );
	} else {
		mx_status = mx_video_input_arm(
					aviex_pccd->video_input_record );
	}

	/* If we are using an EPIX PIXCI imaging board, record the time
	 * that the sequence started in the 'epix_xclib' record.
	 */

	if ( aviex_pccd->video_input_record->mx_type == MXT_VIN_EPIX_XCLIB ) {
	    mx_status = mxp_aviex_pccd_epix_save_start_timespec(aviex_pccd);
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_aviex_pccd_trigger()";

	MX_AVIEX_PCCD *aviex_pccd;
	MX_VIDEO_INPUT *vinput;
	MX_SEQUENCE_PARAMETERS *sp;
	long num_frames_in_sequence;
	int i, num_triggers;
	mx_bool_type camera_is_master, internal_trigger, circular;
	mx_status_type mx_status;

	aviex_pccd = NULL;

	mx_status = mxd_aviex_pccd_get_pointers( ad, &aviex_pccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( aviex_pccd->video_input_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The video_input_record pointer for area detector '%s' is NULL.",
			ad->record->name );
	}

	vinput = aviex_pccd->video_input_record->record_class_struct;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VIDEO_INPUT pointer for video input '%s' "
		"used by area detector '%s' is NULL.",
			aviex_pccd->video_input_record->name,
			ad->record->name );
	}

	DISPLAY_CONTROL_REGISTER(fname);

#if MXD_AVIEX_PCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sp = &(ad->sequence_parameters);

	/* Make sure we are configured for a supported sequence type. */

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_CONTINUOUS:
	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_CIRCULAR_MULTIFRAME:
	case MXT_SQ_STROBE:
	case MXT_SQ_BULB:
	case MXT_SQ_GEOMETRICAL:
	case MXT_SQ_SUBIMAGE:
	case MXT_SQ_STREAK_CAMERA:
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Image sequence type %ld is not supported by "
			"area detector '%s'.",
			sp->sequence_type, ad->record->name );
	}

	camera_is_master =
	   (aviex_pccd->aviex_pccd_flags & MXF_AVIEX_PCCD_CAMERA_IS_MASTER);

	internal_trigger = (vinput->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER);

#if MXD_AVIEX_PCCD_DEBUG
	MX_DEBUG(-2,("%s: camera_is_master = %d, internal_trigger = %d",
		fname, (int) camera_is_master, (int) internal_trigger));
#endif

	switch ( sp->sequence_type ) {
	case MXT_SQ_CONTINUOUS:
		/* For continuous sequences, we must use video board
		 * CC1 pulses to trigger the taking of frames, so that
		 * the total number of frames is not limited.
		 */

		camera_is_master = FALSE;
		break;

	case MXT_SQ_STROBE:
		/* For strobe sequences, the external trigger goes into
		 * the video card, which triggers the taking of frames
		 * via CC1 pulses.
		 */

		camera_is_master = FALSE;
		break;
	}

	if ( camera_is_master && internal_trigger ) {

		mx_status = mx_sequence_get_num_frames(
						&(ad->sequence_parameters),
						&num_frames_in_sequence );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( sp->sequence_type ) {
		case MXT_SQ_CONTINUOUS:
		case MXT_SQ_CIRCULAR_MULTIFRAME:
			circular = TRUE;
			break;
		default:
			circular = FALSE;
			break;
		}

		mx_status = mx_video_input_asynchronous_capture(
					aviex_pccd->video_input_record,
					num_frames_in_sequence, circular );
	} else {
		/* Send the trigger request to the video input board. */

		mx_status = mx_video_input_trigger(
					aviex_pccd->video_input_record );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If we are configured for internal triggering and the camera is
	 * expected to be in charge of frame timing, then we must send
	 * a start pulse to the camera.
	 */

	if ( camera_is_master && internal_trigger ) {

#if 0
		if ( sp->sequence_type == MXT_SQ_SUBIMAGE ) {
			num_triggers = 2;
		} else {
			num_triggers = 1;
		}
#else
		num_triggers = 1;
#endif

		for ( i = 0; i < num_triggers; i++ ) {

			/* Set the output high. */

#if MXD_AVIEX_PCCD_DEBUG
			MX_DEBUG(-2,
				("%s: Sending trigger to camera head.", fname));
#endif
			switch( ad->record->mx_type ) {
			case MXT_AD_PCCD_170170:
			case MXT_AD_PCCD_4824:
				/* Send a 0.1 second pulse. */

				mx_status = mx_digital_output_pulse(
					aviex_pccd->internal_trigger_record,
					1, 0, 0.1 );
				break;
			case MXT_AD_PCCD_16080:
#if 1
				/* Send a 0.001 second pulse. */

				mx_status = mx_camera_link_pulse_cc_line(
					aviex_pccd->camera_link_record, 1,
					-1, 1000 );
				break;
#endif
			default:
				return mx_error( MXE_UNSUPPORTED, fname,
				"Triggering detector type %lu, record '%s', "
				"is not supported.",
					ad->record->mx_type,
					ad->record->name );
				break;
			}

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( num_triggers > 1 ) {
				mx_msleep(100);		/* Wait 0.1 seconds. */
			}
		}
	}

#if MXD_AVIEX_PCCD_DEBUG
	MX_DEBUG(-2,("%s: Started taking a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_aviex_pccd_stop()";

	MX_AVIEX_PCCD *aviex_pccd;
	MX_CLOCK_TICK timeout_in_ticks, current_tick, finish_tick;
	double timeout;
	int comparison;
	mx_bool_type busy, timed_out;
	mx_status_type mx_status;

	aviex_pccd = NULL;

	mx_status = mxd_aviex_pccd_get_pointers( ad, &aviex_pccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVIEX_PCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	/* Send an 'Abort Exposure' command to the detector head by
	 * toggling the CC2 line.  (but not for the PCCD-16080)
	 */

	if ( ad->record->mx_type != MXT_AD_PCCD_16080 ) {
		mx_status = mx_camera_link_pulse_cc_line(
					aviex_pccd->camera_link_record, 2,
					-1, 1000 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Tell the imaging board to stop acquiring frames after the end
	 * of the current frame.  If we are using an EPIX video board via
	 * the XCLIB library, then we force the driver to use the abort
	 * function instead, since the EPIX driver waits until the end
	 * of the current sequence before acknowledging the stop command.
	 */

	if ( ad->record->mx_type != MXT_AD_PCCD_16080 ) {
		if ( aviex_pccd->video_input_record->mx_type
				== MXT_VIN_EPIX_XCLIB )
		{
			mx_status = mx_video_input_abort(
				aviex_pccd->video_input_record );
		} else {
			mx_status = mx_video_input_stop(
				aviex_pccd->video_input_record );
		}
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the imaging board to stop.  If this takes more than
	 * 'timeout' seconds, then break out with a timeout error.
	 */

	timeout = 1.0;		/* in seconds */

	timeout_in_ticks = mx_convert_seconds_to_clock_ticks( timeout );

	current_tick = mx_current_clock_tick();

	finish_tick = mx_add_clock_ticks( current_tick, timeout_in_ticks );

	timed_out = FALSE;

	for (;;) {
		mx_status = mx_video_input_is_busy(
				aviex_pccd->video_input_record, &busy );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( busy == FALSE ) {
			break;			/* Exit the for(;;) loop. */
		}

		current_tick = mx_current_clock_tick();

		comparison = mx_compare_clock_ticks(current_tick, finish_tick);

		if ( comparison >= 0 ) {
			timed_out = TRUE;
			break;			/* Exit the for(;;) loop. */
		}
	}

	if ( timed_out ) {
		return mx_error( MXE_TIMED_OUT, fname,
		"Timed out after waiting %g seconds for video input '%s' "
		"to stop acquiring the current frame.",
			timeout, aviex_pccd->video_input_record->name );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_abort( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_aviex_pccd_abort()";

	MX_AVIEX_PCCD *aviex_pccd;
	mx_status_type mx_status;

	aviex_pccd = NULL;

	mx_status = mxd_aviex_pccd_get_pointers( ad, &aviex_pccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVIEX_PCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	/* Send a 'Reset CCD Controller' command to the detector head by
	 * toggling the CC3 line.  Not all versions of the firmware 
	 * actually pay attention to CC3.  (but not for the PCCD-16080)
	 */

	if ( ad->record->mx_type != MXT_AD_PCCD_16080 ) {
		mx_status = mx_camera_link_pulse_cc_line(
					aviex_pccd->camera_link_record, 3,
					-1, 1000 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Tell the imaging board to immediately stop acquiring frames.
		 */

		mx_status = mx_video_input_abort(
					aviex_pccd->video_input_record );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_aviex_pccd_get_extended_status()";

	MX_AVIEX_PCCD *aviex_pccd;
	long last_frame_number;
	long total_num_frames;
	unsigned long status_flags;
	mx_status_type mx_status;

	aviex_pccd = NULL;

	mx_status = mxd_aviex_pccd_get_pointers( ad, &aviex_pccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	DISPLAY_CONTROL_REGISTER(fname);

	/* Ask the video board for its current status. */

	mx_status = mx_video_input_get_extended_status(
						aviex_pccd->video_input_record,
						&last_frame_number,
						&total_num_frames,
						&status_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVIEX_PCCD_DEBUG_EXTENDED_STATUS
	MX_DEBUG(-2,
("%s: last_frame_number = %ld, total_num_frames = %ld, status_flags = %#lx",
		fname, last_frame_number, total_num_frames, status_flags));
#endif

	ad->last_frame_number = last_frame_number;

	ad->total_num_frames = total_num_frames;

	ad->status = 0;

	if ( status_flags & MXSF_VIN_IS_BUSY ) {
		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	}

	if ( aviex_pccd->buffer_overrun ) {
		ad->status |= MXSF_AD_BUFFER_OVERRUN;
	}

	return MX_SUCCESSFUL_RESULT;
}

static void
mxd_aviex_pccd_save_raw_frame( uint16_t *image_data,
				unsigned long image_length )
{
	static const char fname[] = "mxd_aviex_pccd_save_raw_frame()";

	FILE *file;
	int saved_errno, filename_type;
	unsigned long bytes_written;
	char raw_frame_filename[MXU_FILENAME_LENGTH+1];

	if ( getenv("MXDIR") == NULL ) {
		filename_type = MX_CFN_CWD;
	} else {
		filename_type = MX_CFN_STATE;
	}

	mx_construct_control_system_filename( filename_type,
						"aviex_raw_frame.bin",
						raw_frame_filename,
						sizeof(raw_frame_filename) );

	MX_DEBUG(-2,("%s: Writing raw frame to '%s'",
		fname, raw_frame_filename ));

	file = fopen( raw_frame_filename, "w" );

	if ( file == NULL ) {
		saved_errno = errno;

		mx_error( MXE_NOT_FOUND, fname,
		"Cannot open raw frame file '%s'.  "
		"Errno = %d, error message = '%s'",
			raw_frame_filename,
			saved_errno, strerror(saved_errno) );

		return;
	}

	bytes_written = fwrite( image_data, 1, image_length, file );

	fclose( file );

	if ( bytes_written < image_length ) {
		mx_error( MXE_FILE_IO_ERROR, fname,
		"Raw frame file '%s' was supposed to be %lu bytes long, "
		"but only %lu bytes were written.",
			raw_frame_filename, image_length, bytes_written );
	}

	return;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_aviex_pccd_readout_frame()";

	MX_AVIEX_PCCD *aviex_pccd;
	unsigned long flags;
	long frame_number, maximum_num_frames, total_num_frames;
	long frame_difference, num_times_looped;
	long number_of_frame_that_overwrote_the_frame_we_want;
	long row_framesize, column_framesize;
	size_t bytes_to_copy, raw_frame_length, image_length;
	struct timespec exposure_timespec;
	double exposure_time;
	mx_status_type mx_status;

	aviex_pccd = NULL;

	mx_status = mxd_aviex_pccd_get_pointers( ad, &aviex_pccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVIEX_PCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	/* Compute the frame number modulo the maximum_number of frames.
	 * The modulo part is for the sake of MXT_SQ_CIRCULAR_MULTIFRAME
	 * and MXT_SQ_CONTINUOUS sequences.
	 */

	ad->parameter_type = MXLV_AD_MAXIMUM_FRAME_NUMBER;

	mx_status = mxd_aviex_pccd_get_parameter( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	maximum_num_frames = ad->maximum_frame_number + 1;

	frame_number = ad->readout_frame % maximum_num_frames;

#if MXD_AVIEX_PCCD_DEBUG
	MX_DEBUG(-2,("%s: Reading raw frame %ld", fname, frame_number));
	MX_DEBUG(-2,("%s: ad->readout_frame = %ld, maximum_num_frames = %ld",
		fname, ad->readout_frame, maximum_num_frames));
#endif
	/* Has this frame already been overwritten? */

	mx_status = mx_video_input_get_total_num_frames(
						aviex_pccd->video_input_record,
						&total_num_frames );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	frame_difference = total_num_frames - ad->readout_frame;

#if MXD_AVIEX_PCCD_DEBUG
	MX_DEBUG(-2,
("%s: frame_difference = %ld, total_num_frames = %ld, ad->readout_frame = %ld",
		fname, frame_difference, total_num_frames, ad->readout_frame));
#endif
	/* Only check for frame overwriting if ad->readout_frame is larger
	 * than the total number of frame buffers.  Only in this case can
	 * we be sure that the possibility of buffer overrun exists.
	 */

	if ( ( ad->readout_frame >= maximum_num_frames )
	  && ( frame_difference > 0 ) )
	{
		num_times_looped = frame_difference / maximum_num_frames;

#if MXD_AVIEX_PCCD_DEBUG
		MX_DEBUG(-2,("%s: num_times_looped = %ld",
			fname, num_times_looped));
#endif
		if ( num_times_looped > 0 ) {
			aviex_pccd->buffer_overrun = TRUE;

			number_of_frame_that_overwrote_the_frame_we_want
				= ad->readout_frame
					+ num_times_looped * maximum_num_frames;

#if 0
			mx_warning(
		    "%s: Frame %ld has already been overwritten by frame %ld.",
			fname, ad->readout_frame, 
			number_of_frame_that_overwrote_the_frame_we_want );
#endif
		}
	}

#if MXD_AVIEX_PCCD_DEBUG
	{
		long video_x_width, video_y_width;

		mx_status = mx_video_input_get_framesize(
				aviex_pccd->video_input_record,
				&video_x_width, &video_y_width );

		MX_DEBUG(-2,("%s: video_x_width = %ld, video_y_width = %ld",
			fname, video_x_width, video_y_width));

		MX_DEBUG(-2,
		("%s: ad->framesize[0] = %ld, ad->framesize[1] = %ld",
			fname, ad->framesize[0], ad->framesize[1]));

		MX_DEBUG(-2,
		("%s: raw_frame x width = %ld, raw_frame y width = %ld",
			fname, (long)MXIF_ROW_FRAMESIZE(aviex_pccd->raw_frame),
			(long)MXIF_COLUMN_FRAMESIZE(aviex_pccd->raw_frame) ));
	}
#endif

	/* Read in the raw image frame. */

	mx_status = mx_video_input_get_frame(
		aviex_pccd->video_input_record,
		frame_number, &(aviex_pccd->raw_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the binsize in the header. */

	MXIF_ROW_BINSIZE(aviex_pccd->raw_frame)    = ad->binsize[0];
	MXIF_COLUMN_BINSIZE(aviex_pccd->raw_frame) = ad->binsize[1];

	/* Compute and add the exposure time to the image frame header. */

	mx_status = mx_sequence_get_exposure_time( &(ad->sequence_parameters),
					ad->readout_frame, &exposure_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	exposure_timespec =
		mx_convert_seconds_to_high_resolution_time( exposure_time );

	MXIF_EXPOSURE_TIME_SEC(aviex_pccd->raw_frame)
						= exposure_timespec.tv_sec;

	MXIF_EXPOSURE_TIME_NSEC(aviex_pccd->raw_frame)
						= exposure_timespec.tv_nsec;

	/* Make sure that the image frame is the correct size. */

	row_framesize = MXIF_ROW_FRAMESIZE(aviex_pccd->raw_frame)
				/ aviex_pccd->horiz_descramble_factor;

	column_framesize = MXIF_COLUMN_FRAMESIZE(aviex_pccd->raw_frame)
				* aviex_pccd->vert_descramble_factor;

#if MXD_AVIEX_PCCD_DEBUG_MX_IMAGE_ALLOC
	MX_DEBUG(-2,
	("%s: Invoking mx_image_alloc() for ad->image_frame = %p",
		fname, ad->image_frame));
#endif

	mx_status = mx_image_alloc( &(ad->image_frame),
				row_framesize,
				column_framesize,
				MXIF_IMAGE_FORMAT(aviex_pccd->raw_frame),
				MXIF_BYTE_ORDER(aviex_pccd->raw_frame),
				MXIF_BYTES_PER_PIXEL(aviex_pccd->raw_frame),
				aviex_pccd->raw_frame->header_length,
				aviex_pccd->raw_frame->image_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Copy the image header. */

	mx_status = mx_image_copy_header( aviex_pccd->raw_frame,
					ad->image_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Copying the header overwrites all of the values,
	 * so we patch in the correct descrambled dimensions.
	 */

	MXIF_ROW_FRAMESIZE(ad->image_frame) = row_framesize;

	MXIF_COLUMN_FRAMESIZE(ad->image_frame) = column_framesize;

	flags = aviex_pccd->aviex_pccd_flags;

	/* If requested, save a copy of the undescrambled data. */

	if ( flags & MXF_AVIEX_PCCD_SAVE_RAW_FRAME ) {
		mxd_aviex_pccd_save_raw_frame(
			aviex_pccd->raw_frame->image_data,
			aviex_pccd->raw_frame->image_length );
	}

	/* If required, we now descramble the image. */

	if ( flags & MXF_AVIEX_PCCD_SUPPRESS_DESCRAMBLING ) {

		/* Just copy directly from the raw frame to the image frame. */

		raw_frame_length = aviex_pccd->raw_frame->image_length;

		image_length = ad->image_frame->image_length;

		if ( raw_frame_length < image_length ) {
			bytes_to_copy = raw_frame_length;
		} else {
			bytes_to_copy = image_length;
		}

#if MXD_AVIEX_PCCD_DEBUG
		MX_DEBUG(-2,
	    ("%s: Copying camera simulator image.  Image length = %lu bytes.",
			fname, (unsigned long) bytes_to_copy ));
#endif
		memcpy( ad->image_frame->image_data,
			aviex_pccd->raw_frame->image_data,
			bytes_to_copy );
	} else {
		/* Descramble the image. */

		switch( ad->record->mx_type ) {
		case MXT_AD_PCCD_170170:
		case MXT_AD_PCCD_4824:
		case MXT_AD_PCCD_16080:
			mx_status = mxd_aviex_pccd_descramble_image( ad,
							aviex_pccd,
							ad->image_frame,
							aviex_pccd->raw_frame);
			break;
		default:
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The AVIEX PCCD driver requested for record '%s' "
			"cannot be used by records of type '%s'.",
				ad->record->name,
				mx_get_driver_name( ad->record ) );
			break;
		}
	}

	/* Dezingering test code. */

	if ( flags & MXF_AVIEX_PCCD_TEST_DEZINGER ) {

		/* If we are testing the dezingering logic, we potentially
		 * introduce fake zingers here.
		 */

		uint16_t *image_data;
		int i, num_random_values, random_location, random_value;

		image_data = ad->image_frame->image_data;

		num_random_values = rand() % 4;

		for ( i = 0; i < num_random_values; i++ ) {
			random_location = rand()
			   % (ad->image_frame->image_length / sizeof(uint16_t));

			random_value = rand() % 65536;

			image_data[random_location] = random_value;

			MX_DEBUG(-2,
			("%s: Replaced pixel %d with the zinger value %d.",
				fname, random_location, random_value));
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_aviex_pccd_correct_frame()";

	MX_AVIEX_PCCD *aviex_pccd;
	MX_SEQUENCE_PARAMETERS *sp;
	MX_IMAGE_FRAME *mask_frame;
	MX_IMAGE_FRAME *bias_frame;
	MX_IMAGE_FRAME *flood_field_frame;
	unsigned long flags;
	uint16_t *mask_data_ptr, *bias_data_ptr, *flood_field_data_ptr;
	uint16_t *image_data_array;
	uint16_t *image_stripe_ptr, *image_row_ptr, *image_pixel_ptr;
	uint16_t *mask_pixel_ptr, *bias_pixel_ptr, *flood_field_pixel_ptr;
	double flood_field_scale_factor;
	long big_image_pixel;
	long image_row_framesize, image_column_framesize;
	long image_num_stripes, image_pixels_per_stripe;
	long corr_row_framesize, corr_column_framesize;
	long corr_centerline, corr_num_rows_per_stripe;
	long corr_first_row, corr_start_offset;
	long i, j, k;
	mx_status_type mx_status;

	aviex_pccd = NULL;

	mx_status = mxd_aviex_pccd_get_pointers( ad, &aviex_pccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sp = &(ad->sequence_parameters);

	flags = ad->correction_flags;

#if MXD_AVIEX_PCCD_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
	MX_DEBUG(-2,("%s: ad->correction_flags = %#lx", fname, flags));
#endif

	if ( flags == 0 ) {
		/* No corrections have been requested. */

		return MX_SUCCESSFUL_RESULT;
	}

	if ( ad->image_frame == NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"No image frames have been taken or loaded by detector '%s' "
		"since this program was started.", ad->record->name );
	}

	image_data_array = ad->image_frame->image_data;

	image_row_framesize    = MXIF_ROW_FRAMESIZE(ad->image_frame);
	image_column_framesize = MXIF_COLUMN_FRAMESIZE(ad->image_frame);

#if MXD_AVIEX_PCCD_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,
	("%s: image_row_framesize = %ld, image_column_framesize = %ld",
		fname, image_row_framesize, image_column_framesize));
#endif

	/*-----*/

	if ( (sp->sequence_type != MXT_SQ_SUBIMAGE)
	  && (sp->sequence_type != MXT_SQ_STREAK_CAMERA) )
	{
		/**********************
		 * NORMAL FULL FRAMES *
		 **********************/

		/* For normal full frame images, first apply the
		 * default correction function.
		 */

		mx_status = mx_area_detector_default_correct_frame( ad );

		return mx_status;
	}

	/*-----*/

	/************************************
	 * STREAK CAMERA OR SUBIMAGE FRAMES *
	 ************************************/

	/*
	 * Find the pixel offset of the first row in the correction frames
	 * that will be used to correct the data.
	 */

	/* We get the dimensions of the correction frames from the mask frame.*/

	mx_status = mx_area_detector_get_correction_frame( ad, ad->image_frame,
							MXFT_AD_MASK_FRAME,
							"mask",
							&mask_frame );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	corr_row_framesize    = MXIF_ROW_FRAMESIZE(mask_frame);
	corr_column_framesize = MXIF_COLUMN_FRAMESIZE(mask_frame);

	if ( image_column_framesize != corr_column_framesize ) {
		return mx_error( MXE_CONFIGURATION_CONFLICT, fname,
		"The number of columns (%ld) in the image frame is not "
		"same as the number of columns (%ld) in the mask frame "
		"for area detector '%s'.",
			image_column_framesize, corr_column_framesize,
			ad->record->name );
	}

#if MXD_AVIEX_PCCD_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,
	("%s: corr_row_framesize = %ld, corr_column_framesize = %ld",
		fname, corr_row_framesize, corr_column_framesize));
#endif

	/* Find the centerline of the correction frame data. */

	if ( aviex_pccd->use_top_half_of_detector ) {
		corr_centerline = mx_round(0.25 * (double) corr_row_framesize);
	} else {
		corr_centerline = mx_round(0.75 * (double) corr_row_framesize);
	}

	if ( sp->sequence_type == MXT_SQ_STREAK_CAMERA ) {
		corr_num_rows_per_stripe = 2L;
	} else {
		corr_num_rows_per_stripe = mx_round( sp->parameter_array[0] );
	}

	image_num_stripes = image_column_framesize / corr_num_rows_per_stripe;

	image_pixels_per_stripe =
			image_row_framesize * corr_num_rows_per_stripe;

#if MXD_AVIEX_PCCD_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,
	("%s: corr_centerline = %ld, corr_num_rows_per_stripe = %ld",
		fname, corr_centerline, corr_num_rows_per_stripe));
	MX_DEBUG(-2,
	("%s: image_num_stripes = %ld, image_pixels_per_stripe = %ld",
		fname, image_num_stripes, image_pixels_per_stripe));
#endif
	/* Now find the first row pixel offset. */

	corr_first_row = corr_centerline - ( corr_num_rows_per_stripe / 2L );

	corr_start_offset = corr_first_row * corr_column_framesize;

#if MXD_AVIEX_PCCD_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("%s: corr_first_row = %ld, corr_start_offset = %ld",
		fname, corr_first_row, corr_start_offset));
#endif

	/*---*/

	/* Get pointers to the parts of the correction frames that we
	 * will use for the subimage or streak camera frame.
	 */

	if ( (flags & MXFT_AD_MASK_FRAME) == 0 ) {
		mask_data_ptr = NULL;
	} else {
#if 0
		/* We already found the mask frame earlier in the routine. */

		mx_status = mx_area_detector_get_correction_frame(
							ad, ad->image_frame,
							MXFT_AD_MASK_FRAME,
							"mask",
							&mask_frame );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
#endif

		mask_data_ptr = mask_frame->image_data;
		mask_data_ptr += corr_start_offset;
	}

	if ( (flags & MXFT_AD_BIAS_FRAME) == 0 ) {
		bias_data_ptr = NULL;
	} else {
		mx_status = mx_area_detector_get_correction_frame(
							ad, ad->image_frame,
							MXFT_AD_BIAS_FRAME,
							"bias",
							&bias_frame );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		bias_data_ptr = bias_frame->image_data;
		bias_data_ptr += corr_start_offset;
	}

	/* We do not do dark current correction for subimage
	 * or streak camera frames.
	 */

	if ( (flags & MXFT_AD_FLOOD_FIELD_FRAME) == 0 ) {
		flood_field_data_ptr = NULL;
	} else {
		mx_status = mx_area_detector_get_correction_frame(
						ad, ad->image_frame,
						MXFT_AD_FLOOD_FIELD_FRAME,
						"flood_field",
						&flood_field_frame );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		flood_field_data_ptr = flood_field_frame->image_data;
		flood_field_data_ptr += corr_start_offset;
	}

	/* Now walk through the stripes in the image data
	 * and do the corrections.
	 */

	for ( i = 0; i < image_num_stripes; i++ ) {
	    image_stripe_ptr = image_data_array + i * image_pixels_per_stripe;

	    for ( j = 0; j < corr_num_rows_per_stripe; j++ ) {
		image_row_ptr = image_stripe_ptr + j * image_column_framesize;

		for ( k = 0; k < image_column_framesize; k++ ) {
		    image_pixel_ptr = image_row_ptr + k;

		    /* See if the pixel is masked off. */

		    if ( mask_data_ptr != NULL ) {
			mask_pixel_ptr = mask_data_ptr
						+ j * corr_column_framesize + k;

			if ( *mask_pixel_ptr == 0 ) {

			    /* If the pixel is masked off, skip this pixel
			     * and return to the top of the k loop for
			     * the next pixel.
			     */

			    *image_pixel_ptr = 0;
			    continue;
			}					
		    }

		    /* Add the detector bias. */

		    if ( bias_data_ptr != NULL ) {
			bias_pixel_ptr = bias_data_ptr
						+ j * corr_column_framesize + k;

			*image_pixel_ptr += *bias_pixel_ptr;
		    }

		    /* Do the flood field correction. */

		    if ( flood_field_data_ptr != NULL ) {
			flood_field_pixel_ptr = flood_field_data_ptr
						+ j * corr_column_framesize + k;

			flood_field_scale_factor =
			    mx_divide_safely( ad->flood_field_average_intensity,
					(double) *flood_field_pixel_ptr );

			big_image_pixel = mx_round(
			  flood_field_scale_factor * (double) *image_pixel_ptr);

			if ( big_image_pixel > 65535 ) {
			    *image_pixel_ptr = 65535;
			} else {
			    *image_pixel_ptr = big_image_pixel;
			}
		    }
		}
	    }
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_aviex_pccd_find_register( MX_AREA_DETECTOR *ad,
					MX_AVIEX_PCCD *aviex_pccd,
					long *parameter_type,
					void **value_ptr,
					MX_RECORD_FIELD **field )
{
	static const char fname[] = "mxd_aviex_pccd_find_register()";

	MX_RECORD_FIELD *register_value_field;
	mx_status_type mx_status;

	if ( *parameter_type < 0 ) {
		/* In this case, we got here by way of the 'register_value'
		 * field.  We must find the label value of the actual field
		 * so that we can tell whether or not it is a pseudo register
		 * that depends on the control register.
		 */

		mx_status = mx_find_record_field( ad->record,
					ad->register_name, field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		*parameter_type = (*field)->label_value;

		if ( *parameter_type < MXLV_AVIEX_PCCD_DH_BASE ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"'%s' is not the name of a register for area detector '%s'.",
				ad->register_name, ad->record->name );
		}

		/* In this case, we read from and write to the 'register_value'
		 * field, instead of the field we just found above.
		 */
		
		mx_status = mx_find_record_field( ad->record,
				"register_value", &register_value_field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		*value_ptr = mx_get_field_value_pointer( register_value_field );
	} else
	if ( *parameter_type >= MXLV_AVIEX_PCCD_DH_BASE ) {
		/* If we were passed the field label value directly, then the
		 * 'register_value' field was not involved in getting here.
		 *
		 * In this case, we read from and write to the corresponding
		 * field directly.
		 */

		mx_status = mx_get_field_by_label_value( ad->record,
						*parameter_type, field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		*value_ptr = mx_get_field_value_pointer( *field );
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized parameter type %ld for Aviex detector '%s'.",
					*parameter_type, ad->record->name );
	}

	if ( (*field)->datatype != MXFT_ULONG ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	    "Record field '%s' for record '%s' is not an unsigned long field.",
			(*field)->name, ad->record->name );
	}

	return mx_status;
}

static mx_status_type
mxd_aviex_pccd_get_register_value( MX_AREA_DETECTOR *ad,
					MX_AVIEX_PCCD *aviex_pccd,
					long parameter_type )
{
	static const char fname[] = "mxd_aviex_pccd_get_register_value()";

	MX_RECORD_FIELD *field;
	unsigned long pseudo_reg_value;
	void *value_ptr;
	unsigned long *ulong_value_ptr;
	mx_status_type mx_status;

	/* Note: mxd_aviex_pccd_find_register() will modify the value of
	 * 'parameter_type' if the original value of 'parameter_type' is
	 * less than zero.
	 */

	mx_status = mxd_aviex_pccd_find_register( ad, aviex_pccd,
							&parameter_type,
							&value_ptr,
							&field );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ulong_value_ptr = value_ptr;

	/* For real registers, getting the value is simple. */

	if ( parameter_type < MXLV_AVIEX_PCCD_DH_PSEUDO_BASE ) {

		switch( aviex_pccd->record->mx_type ) {
		case MXT_AD_PCCD_170170:
		case MXT_AD_PCCD_4824:
			mx_status = mxd_aviex_pccd_read_register(
					aviex_pccd, parameter_type, value_ptr );
			break;
		case MXT_AD_PCCD_16080:
			mx_status = mxd_aviex_pccd_16080_read_register(
					aviex_pccd, parameter_type, value_ptr );
			break;
		}

		return mx_status;
	}

	/* For pseudo registers, we must extract the value from
	 * the control register.
	 */

	switch( aviex_pccd->record->mx_type ) {
	case MXT_AD_PCCD_170170:
	case MXT_AD_PCCD_4824:
		mx_status = mxd_aviex_pccd_170170_get_pseudo_register(
							aviex_pccd,
							parameter_type,
							&pseudo_reg_value );
		break;
	case MXT_AD_PCCD_16080:
		mx_status = mxd_aviex_pccd_16080_get_pseudo_register(
							aviex_pccd,
							parameter_type,
							&pseudo_reg_value );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported record type %lu for record '%s'.",
			aviex_pccd->record->mx_type,
			aviex_pccd->record->name );
		break;
	}

	if ( mx_status.code == MXE_SUCCESS ) {
		*ulong_value_ptr = pseudo_reg_value;
	}

	return mx_status;
}

static mx_status_type
mxd_aviex_pccd_set_register_value( MX_AREA_DETECTOR *ad,
					MX_AVIEX_PCCD *aviex_pccd,
					long parameter_type )
{
	static const char fname[] = "mxd_aviex_pccd_set_register_value()";

	MX_RECORD_FIELD *field;
	unsigned long register_value;
	void *value_ptr;
	mx_status_type mx_status;

	/* Note: mxd_aviex_pccd_find_register() will modify the value of
	 * 'parameter_type' if the original value of 'parameter_type' is
	 * less than zero.
	 */

	mx_status = mxd_aviex_pccd_find_register( ad, aviex_pccd,
							&parameter_type,
							&value_ptr,
							&field );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	register_value = *((unsigned long *) value_ptr);

	/* We refuse to write to registers that are marked as read only. */

	if ( field->flags & MXFF_READ_ONLY ) {
		return mx_error( MXE_READ_ONLY, fname,
		"Aviex register '%s' for area detector '%s' is read only.",
			field->name, ad->record->name );
	}

	/* For real registers, setting the value is simple. */

	if ( parameter_type < MXLV_AVIEX_PCCD_DH_PSEUDO_BASE ) {

		switch( aviex_pccd->record->mx_type ) {
		case MXT_AD_PCCD_170170:
		case MXT_AD_PCCD_4824:
			mx_status = mxd_aviex_pccd_write_register(
				aviex_pccd, parameter_type, register_value );
			break;
		case MXT_AD_PCCD_16080:
			mx_status = mxd_aviex_pccd_16080_write_register(
				aviex_pccd, parameter_type, register_value );
			break;
		}

		return mx_status;
	}

	/* For pseudo registers, we must change specific bits in
	 * the control register.
	 */

	switch( aviex_pccd->record->mx_type ) {
	case MXT_AD_PCCD_170170:
	case MXT_AD_PCCD_4824:
		mx_status = mxd_aviex_pccd_170170_set_pseudo_register(
							aviex_pccd,
							parameter_type,
							register_value );
		break;
	case MXT_AD_PCCD_16080:
		mx_status = mxd_aviex_pccd_16080_set_pseudo_register(
							aviex_pccd,
							parameter_type,
							register_value );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported record type %lu for record '%s'.",
			aviex_pccd->record->mx_type,
			aviex_pccd->record->name );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_aviex_pccd_get_parameter()";

	MX_AVIEX_PCCD *aviex_pccd;
	MX_RECORD *video_input_record;
	long vinput_horiz_framesize, vinput_vert_framesize;
	long shutter_disabled;
	mx_status_type mx_status;

	aviex_pccd = NULL;

	mx_status = mxd_aviex_pccd_get_pointers( ad, &aviex_pccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVIEX_PCCD_DEBUG
	{
		char name_buffer[MXU_FIELD_NAME_LENGTH+1];

		MX_DEBUG(-2,("%s: record '%s', parameter '%s'",
			fname, ad->record->name,
			mx_get_parameter_name_from_type(
				ad->record, ad->parameter_type,
				name_buffer, sizeof(name_buffer)) ));
	}
#endif
	video_input_record = aviex_pccd->video_input_record;

	switch( ad->parameter_type ) {
	case MXLV_AD_MAXIMUM_FRAME_NUMBER:
		mx_status = mx_video_input_get_maximum_frame_number(
			video_input_record, &(ad->maximum_frame_number) );
		break;

	case MXLV_AD_FRAMESIZE:
		mx_status = mx_video_input_get_framesize( video_input_record,
			     &vinput_horiz_framesize, &vinput_vert_framesize );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* See the comments about the Aviex camera in the function
		 * mxd_aviex_pccd_open() for the explanation of where the
		 * horizontal and vertical descramble factors come from.
		 */

		ad->framesize[0] = 
		  vinput_horiz_framesize / aviex_pccd->horiz_descramble_factor;

		ad->framesize[1] =
		  vinput_vert_framesize * aviex_pccd->vert_descramble_factor;

		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		mx_status = mx_video_input_get_image_format( video_input_record,
						&(ad->image_format) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_format_name_from_type(
				ad->image_format, ad->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
		break;

	case MXLV_AD_BYTES_PER_FRAME:
		mx_status = mx_video_input_get_bytes_per_frame(
				video_input_record, &(ad->bytes_per_frame) );
		break;

	case MXLV_AD_BYTES_PER_PIXEL:
		mx_status = mx_video_input_get_bytes_per_pixel(
				video_input_record, &(ad->bytes_per_pixel) );
		break;

	case MXLV_AD_SEQUENCE_START_DELAY:
	case MXLV_AD_TOTAL_ACQUISITION_TIME:
	case MXLV_AD_DETECTOR_READOUT_TIME:
	case MXLV_AD_TOTAL_SEQUENCE_TIME:
		switch( ad->record->mx_type ) {
		case MXT_AD_PCCD_170170:
			mx_status =
				mxd_aviex_pccd_170170_compute_sequence_times(
					ad, aviex_pccd );
			break;
		case MXT_AD_PCCD_4824:
		case MXT_AD_PCCD_16080:
			ad->sequence_start_delay = 0.0;
			ad->total_acquisition_time = 0.0;
			ad->detector_readout_time = 0.0;
			ad->total_sequence_time = 0.0;
			break;
		default:
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The AVIEX PCCD driver requested for record '%s' "
			"cannot be used by records of type '%s'.",
				ad->record->name,
				mx_get_driver_name( ad->record ) );
			break;
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXD_AVIEX_PCCD_DEBUG
		MX_DEBUG(-2,("%s: ad->total_sequence_time = %g",
			fname, ad->total_sequence_time));
#endif
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		break;

	case MXLV_AD_TRIGGER_MODE:
		mx_status = mx_video_input_get_trigger_mode(
				video_input_record, &(ad->trigger_mode) );
		break;

	case MXLV_AD_REGISTER_VALUE:
		mx_status = mxd_aviex_pccd_get_register_value( ad,
							aviex_pccd, -1 );
		break;

	case MXLV_AD_SHUTTER_ENABLE:
		switch( ad->record->mx_type ) {
		case MXT_AD_PCCD_170170:
			mx_status = mx_area_detector_get_register( ad->record,
							"dh_shutter_disable",
							&shutter_disabled );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( shutter_disabled ) {
				ad->shutter_enable = FALSE;
			} else {
				ad->shutter_enable = TRUE;
			}
			break;
		default:
			ad->shutter_enable = FALSE;
			break;
		}
		break;

	default:
		if ( ad->parameter_type >= MXLV_AVIEX_PCCD_DH_BASE ) {
			mx_status = mxd_aviex_pccd_get_register_value( ad,
					    aviex_pccd, ad->parameter_type );
		} else {
			mx_status =
			    mx_area_detector_default_get_parameter_handler(ad);
		}
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_aviex_pccd_set_parameter()";

	MX_AVIEX_PCCD *aviex_pccd;
	MX_SEQUENCE_PARAMETERS *sp;
	unsigned long flags;
	long vinput_horiz_framesize, vinput_vert_framesize;
	long horiz_binsize, vert_binsize;
	long saved_vert_binsize, saved_vert_framesize;
	long num_frames, shutter_disable;
	unsigned long new_delay_time;
	double bytes_per_frame;
	mx_status_type mx_status;

	static long allowed_binsize[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

	static int num_allowed_binsizes = sizeof( allowed_binsize )
						/ sizeof( allowed_binsize[0] );

	aviex_pccd = NULL;

	mx_status = mxd_aviex_pccd_get_pointers( ad, &aviex_pccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVIEX_PCCD_DEBUG
	{
		char name_buffer[MXU_FIELD_NAME_LENGTH+1];

		MX_DEBUG(-2,("%s: record '%s', parameter '%s'",
			fname, ad->record->name,
			mx_get_parameter_name_from_type(
				ad->record, ad->parameter_type,
				name_buffer, sizeof(name_buffer)) ));
	}
#endif
	flags = aviex_pccd->aviex_pccd_flags;

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:

		sp = &(ad->sequence_parameters);

		/* Invalidate any existing image sector array. */

		mxd_aviex_pccd_free_sector_array(
				aviex_pccd->sector_array );

		aviex_pccd->sector_array = NULL;

		/* Find a compatible framesize and binsize. */

		if ( sp->sequence_type == MXT_SQ_STREAK_CAMERA ) {
			/* Handle streak camera mode. */

			saved_vert_binsize   = ad->binsize[1];
			saved_vert_framesize = ad->framesize[1];

			/* Restrict the allowed binsize for Y
			 * to either 1 or 2.
			 */

			if ( saved_vert_binsize <= 1 ) {
				saved_vert_binsize = 1;
			} else
			if ( saved_vert_binsize >= 2 ) {
				saved_vert_binsize = 2;
			}

#if MXD_AVIEX_PCCD_DEBUG
			MX_DEBUG(-2,
		("%s: saved_vert_binsize = %ld, saved_vert_framesize = %ld",
			    fname, saved_vert_binsize, saved_vert_framesize));
#endif

			/* Allow the utility function to change the
			 * X binning and framesize.
			 */

			mx_status = mx_area_detector_compute_new_binning( ad,
							ad->parameter_type,
							num_allowed_binsizes,
							allowed_binsize );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Restore the saved Y binsize and framesize. */

			ad->binsize[1] = saved_vert_binsize;
			ad->framesize[1] = saved_vert_framesize;
		} else {
			/* Handle non-streak camera modes. */

			mx_status = mx_area_detector_compute_new_binning( ad,
							ad->parameter_type,
							num_allowed_binsizes,
							allowed_binsize );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		/* Tell the detector head to change its binsize. */

		switch( ad->record->mx_type ) {
		case MXT_AD_PCCD_170170:
			mx_status = mxd_aviex_pccd_170170_set_binsize( ad,
								aviex_pccd );
			break;
		case MXT_AD_PCCD_4824:
			mx_status = mxd_aviex_pccd_4824_set_binsize( ad,
								aviex_pccd );
			break;
		case MXT_AD_PCCD_16080:
			mx_status = mxd_aviex_pccd_16080_set_binsize( ad,
								aviex_pccd );
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported record type %lu for record '%s'.",
				ad->record->mx_type, ad->record->name );
			break;
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Tell the video input to change its framesize. */

		/* See the comments about the Aviex camera in the function
		 * mxd_aviex_pccd_open() for the explanation of where the
		 * horizontal and vertical descramble factors come from.
		 */

		vinput_horiz_framesize =
			ad->framesize[0] * aviex_pccd->horiz_descramble_factor;

		vinput_vert_framesize =
			ad->framesize[1] / aviex_pccd->vert_descramble_factor;

		horiz_binsize = ad->binsize[0];
		vert_binsize  = ad->binsize[1];

		mx_status = mx_video_input_set_framesize(
					aviex_pccd->video_input_record,
					vinput_horiz_framesize,
					vinput_vert_framesize );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* If the resolution change succeeded, save the framesize for
		 * later use in switching to and from streak camera mode.
		 */

		aviex_pccd->vinput_normal_framesize[0] =
						vinput_horiz_framesize;

		aviex_pccd->vinput_normal_framesize[1] =
						vinput_vert_framesize;

		aviex_pccd->normal_binsize[0] = horiz_binsize;

		aviex_pccd->normal_binsize[1] = vert_binsize;

		/* Update the number of bytes per frame for this resolution. */

		bytes_per_frame =
		    ad->bytes_per_pixel * ad->framesize[0] * ad->framesize[1];

		ad->bytes_per_frame = mx_round( bytes_per_frame );
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:

		/* Do not send any changes to the hardware when these two
		 * values are changed since the contents of the sequence
		 * parameter array may not match the change.  For example,
		 * if we are changing from one-shot mode to multiframe mode,
		 * parameter_array[0] is the exposure time in one-shot mode,
		 * but parameter_array[0] is the number of frames in
		 * multiframe mode.  Thus, the only quasi-safe time for
		 * sending changes to the hardware is when the contents
		 * of the sequence parameter array has been changed.
		 */
		break;

	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 

		/* Invalidate any existing image sector array. */

		mxd_aviex_pccd_free_sector_array( aviex_pccd->sector_array );

		aviex_pccd->sector_array = NULL;

		/* Update the maximum frame number. */

		mx_status = mx_video_input_get_maximum_frame_number(
					aviex_pccd->video_input_record,
					&(ad->maximum_frame_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Get the number of frames requested by the sequence. */

		sp = &(ad->sequence_parameters);

		mx_status = mx_sequence_get_num_frames( sp, &num_frames );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( sp->sequence_type ) {
		case MXT_SQ_CONTINUOUS:
			/* Continuous scans are not limited by the maximum
			 * number of frames that the video card can handle,
			 * since they wrap back to the first frame when they
			 * reach the last frame.
			 */
			break;
		case MXT_SQ_CIRCULAR_MULTIFRAME:
			if ( num_frames >
				MXF_AVIEX_PCCD_MAXIMUM_DETECTOR_HEAD_FRAMES )
			{
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"The circular multiframe sequence requested "
				"for area detector '%s' would have more "
				"frames (%ld) than the maximum number of "
				"frames available (%d).",
				ad->record->name, num_frames,
				MXF_AVIEX_PCCD_MAXIMUM_DETECTOR_HEAD_FRAMES );
			}
			break;
		default:
			if ( num_frames > (ad->maximum_frame_number + 1) ) {
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"The sequence requested for area detector '%s' "
				"would have more frames (%ld) than the maximum "
				"number of frames available (%ld) from video "
				"input '%s'.",
					ad->record->name, num_frames,
					ad->maximum_frame_number + 1,
					aviex_pccd->video_input_record->name );
			}
			break;
		}

		/* Configure the registers in the Aviex detector head
		 * for this sequence.
		 */

		switch( ad->record->mx_type ) {
		case MXT_AD_PCCD_170170:
			mx_status =
			    mxd_aviex_pccd_170170_configure_for_sequence(
							ad, aviex_pccd );
			break;
		case MXT_AD_PCCD_4824:
			mx_status =
			    mxd_aviex_pccd_4824_configure_for_sequence(
							ad, aviex_pccd );
			break;
		case MXT_AD_PCCD_16080:
			mx_status =
			    mxd_aviex_pccd_16080_configure_for_sequence(
							ad, aviex_pccd );
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported record type %lu for record '%s'.",
				ad->record->mx_type, ad->record->name );
			break;
		}

		/* Reprogram the imaging board. */

		mx_status = mx_video_input_set_sequence_parameters(
					aviex_pccd->video_input_record, sp );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the trigger mode for the imaging board. */

		switch( sp->sequence_type ) {
		case MXT_SQ_ONE_SHOT:
		case MXT_SQ_CONTINUOUS:
			mx_status = mx_video_input_set_trigger_mode( 
						aviex_pccd->video_input_record,
						MXT_IMAGE_INTERNAL_TRIGGER );
			break;

		case MXT_SQ_STROBE:
		case MXT_SQ_BULB:
			mx_status = mx_video_input_set_trigger_mode( 
						aviex_pccd->video_input_record,
						MXT_IMAGE_EXTERNAL_TRIGGER );
			break;

		case MXT_SQ_MULTIFRAME:
		case MXT_SQ_CIRCULAR_MULTIFRAME:
		case MXT_SQ_GEOMETRICAL:
		case MXT_SQ_STREAK_CAMERA:
		case MXT_SQ_SUBIMAGE:
			/* For these cases, we leave the trigger mode alone,
			 * since we may want to use either internal triggering
			 * or external triggering.
			 */
			break;

		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal sequence type %ld requested "
			"for detector '%s'.",
				sp->sequence_type, ad->record->name );
		}

		/* Insert a delay to give the detector head FPGAs time to
		 * finish their processing.
		 */

		mx_msleep(1);

		break; 

	case MXLV_AD_SEQUENCE_START_DELAY:
		new_delay_time = mx_round( 1.0e5 * ad->sequence_start_delay );

		switch( ad->record->mx_type ) {
		case MXT_AD_PCCD_170170:
			mx_status =
			    mxd_aviex_pccd_170170_set_sequence_start_delay(
						aviex_pccd, new_delay_time );
			break;
		case MXT_AD_PCCD_4824:
			mx_status =
			    mxd_aviex_pccd_4824_set_sequence_start_delay(
						aviex_pccd, new_delay_time );
			break;
		case MXT_AD_PCCD_16080:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Not yet implemented." );
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Setting the sequence start delay is not supported "
			"for record type %lu, record '%s'.",
				ad->record->mx_type, ad->record->name );
			break;
		}

		return mx_status;
		break;

	case MXLV_AD_TRIGGER_MODE:
		mx_status = mx_video_input_set_trigger_mode(
				aviex_pccd->video_input_record,
				ad->trigger_mode );
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Changing parameter '%s' for area detector '%s' is not supported.",
			mx_get_field_label_string( ad->record,
				ad->parameter_type ), ad->record->name );
		break;

	case MXLV_AD_REGISTER_VALUE:
		mx_status = mxd_aviex_pccd_set_register_value( ad,
							aviex_pccd, -1 );
		break;

	case MXLV_AD_SHUTTER_ENABLE:
		switch( ad->record->mx_type ) {
		case MXT_AD_PCCD_170170:
			if ( ad->shutter_enable ) {
				shutter_disable = 0;
			} else {
				shutter_disable = 1;
			}

			mx_status = mx_area_detector_set_register( ad->record,
							"dh_shutter_disable",
							shutter_disable );
			break;
		default:
			ad->shutter_enable = FALSE;
			break;
		}
		break;

	default:
		if ( ad->parameter_type >= MXLV_AVIEX_PCCD_DH_BASE ) {
			mx_status = mxd_aviex_pccd_set_register_value( ad,
					    aviex_pccd, ad->parameter_type );
		} else {
			mx_status =
			    mx_area_detector_default_set_parameter_handler(ad);
		}
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_measure_correction( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_aviex_pccd_measure_correction()";

	MX_AVIEX_PCCD *aviex_pccd;
	unsigned long flags;
	mx_status_type mx_status;

	mx_status = mxd_aviex_pccd_get_pointers( ad, &aviex_pccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	flags = aviex_pccd->aviex_pccd_flags;

	if ( (flags & MXF_AVIEX_PCCD_SKIP_FIRST_CORRECTION_FRAME) == 0 ) {

		/* If we are not supposed to skip the first frame, then
		 * just invoke the default correction measurement function.
		 */

		mx_status = mx_area_detector_default_measure_correction( ad );

		return mx_status;
	}

	/* We have been asked to skip the first frame of a correction
	 * measurement sequence.  This means that we must use a multiframe
	 * sequence rather than a series of one-shot exposures.
	 */

	mx_status = mx_area_detector_set_multiframe_mode( ad->record,
					ad->num_correction_measurements + 1,
					ad->correction_measurement_time,
					ad->correction_measurement_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the sequence. */

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Skipping the first frame has not yet been implemented." );
}

#if MXD_AVIEX_PCCD_GEOMETRICAL_MASK_KLUDGE

static mx_status_type
mxp_aviex_pccd_geometrical_mask_kludge( MX_AREA_DETECTOR *ad,
					MX_AVIEX_PCCD *aviex_pccd )
{
	static const char fname[] = "mxp_aviex_pccd_geometrical_mask_kludge()";

	MX_IMAGE_FRAME *mask_frame;
	long mask_row_binsize, mask_column_binsize;
	long mask_row_framesize, mask_column_framesize;
	long unbinned_row_framesize, unbinned_column_framesize;

	mask_frame = aviex_pccd->geometrical_mask_frame;

	if ( mask_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The geometrical_mask_frame pointer for area detector '%s' "
		"is NULL at a time when it should not be NULL.",
			ad->record->name );
	}

	mask_row_binsize    = MXIF_ROW_BINSIZE(mask_frame);
	mask_column_binsize = MXIF_COLUMN_BINSIZE(mask_frame);

	/* If either the mask row binsize or the mask column binsize
	 * is not 1, then we assume that the bin sizes for the mask
	 * frame have been set correctly and return.
	 */

	if ( ( mask_row_binsize != 1 )
	  || ( mask_column_binsize != 1 ) )
	{
		return MX_SUCCESSFUL_RESULT;
	}

#if MXD_AVIEX_PCCD_DEBUG_SETUP_GEOMETRICAL_MASK
	MX_DEBUG(-2,("%s: computing mask binsize for area detector '%s'.",
		fname, ad->record->name ));
#endif

	/* If we get here, then we need to attempt to figure out what the
	 * real mask binsize is.
	 */

	/* FIXME: We _assume_ that an unbinned image is square and has a
	 * width that is identical to the maximum row width of the image
	 * frame, namely, ad->maximum_framesize[0].
	 */

	unbinned_row_framesize = ad->maximum_framesize[0];
	unbinned_column_framesize = unbinned_row_framesize;

	mask_row_framesize    = MXIF_ROW_FRAMESIZE(mask_frame);
	mask_column_framesize = MXIF_COLUMN_FRAMESIZE(mask_frame);

#if MXD_AVIEX_PCCD_DEBUG_SETUP_GEOMETRICAL_MASK
	MX_DEBUG(-2,
	("%s: unbinned_row_framesize = %lu, unbinned_column_framesize = %lu",
		fname, unbinned_row_framesize, unbinned_column_framesize));
	MX_DEBUG(-2,
	("%s: mask_row_framesize = %lu, mask_column_framesize = %lu",
		fname, mask_row_framesize, mask_column_framesize));
#endif

	if ( ( mask_row_framesize > unbinned_row_framesize )
	  || ( mask_column_framesize > unbinned_column_framesize ) )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"One or more of the dimensions (%ld,%ld) of the "
		"mask frame '%s' is larger than the maximum image "
		"size (%ld,%ld) for area detector '%s'.",
			mask_row_framesize, mask_column_framesize,
			aviex_pccd->geometrical_mask_filename,
			unbinned_row_framesize, unbinned_column_framesize,
			ad->record->name );
	}

	if ( ( (unbinned_row_framesize % mask_row_framesize) != 0 )
	  || ( (unbinned_column_framesize % mask_column_framesize) != 0 ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"One or more of the dimensions (%ld,%ld) of the "
		"mask frame '%s' is not an integer fraction of "
		"the maximum image size (%ld,%ld) for area detector '%s'.",
			mask_row_framesize, mask_column_framesize,
			aviex_pccd->geometrical_mask_filename,
			unbinned_row_framesize, unbinned_column_framesize,
			ad->record->name );
	}

	mask_row_binsize = unbinned_row_framesize / mask_row_framesize;
	mask_column_binsize = unbinned_column_framesize / mask_column_framesize;

#if MXD_AVIEX_PCCD_DEBUG_SETUP_GEOMETRICAL_MASK
	MX_DEBUG(-2,
	("%s: mask_row_binsize = %lu, mask_column_binsize = %lu",
		fname, mask_row_binsize, mask_column_binsize));
#endif

	MXIF_ROW_BINSIZE(mask_frame)    = mask_row_binsize;
	MXIF_COLUMN_BINSIZE(mask_frame) = mask_column_binsize;

	return MX_SUCCESSFUL_RESULT;
}

#endif

static mx_status_type
mxd_aviex_pccd_setup_geometrical_mask_frame( MX_AREA_DETECTOR *ad,
				MX_AVIEX_PCCD *aviex_pccd,
				MX_IMAGE_FRAME *image_frame,
				uint16_t **geometrical_mask_frame_buffer )
{
#if MXD_AVIEX_PCCD_DEBUG_SETUP_GEOMETRICAL_MASK
	static const char fname[] =
			"mxd_aviex_pccd_setup_geometrical_mask_frame()";
#endif

	MX_IMAGE_FRAME *mask_frame;
	void *image_data_pointer;
	long image_row_binsize, image_column_binsize;
	long mask_row_binsize, mask_column_binsize;
	long rebinned_row_binsize, rebinned_column_binsize;
	mx_bool_type need_rebinned_mask_frame, new_mask_frame_loaded;
	mx_bool_type create_rebinned_mask_frame;
	mx_status_type mx_status;

	image_row_binsize    = MXIF_ROW_BINSIZE(image_frame);
	image_column_binsize = MXIF_COLUMN_BINSIZE(image_frame);

#if MXD_AVIEX_PCCD_DEBUG_SETUP_GEOMETRICAL_MASK
	MX_DEBUG(-2,("%s: image_row_binsize = %ld, image_column_binsize = %ld",
		fname, image_row_binsize, image_column_binsize));

	MX_DEBUG(-2,("%s: ad->binsize[0] = %ld, ad->binsize[1] = %ld",
		fname, ad->binsize[0], ad->binsize[1]));
#endif

	/* Make sure that the unbinned geometrical mask frame
	 * has been loaded.
	 */

	if ( aviex_pccd->geometrical_mask_frame == (MX_IMAGE_FRAME *) NULL ) {
		mx_image_free( aviex_pccd->rebinned_geometrical_mask_frame );

		aviex_pccd->rebinned_geometrical_mask_frame = NULL;

		mx_status = mx_image_read_smv_file(
				&(aviex_pccd->geometrical_mask_frame),
				aviex_pccd->geometrical_mask_filename );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXD_AVIEX_PCCD_GEOMETRICAL_MASK_KLUDGE
		mx_status = mxp_aviex_pccd_geometrical_mask_kludge( ad,
								aviex_pccd );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
#endif

		new_mask_frame_loaded = TRUE;
	} else {
		new_mask_frame_loaded = FALSE;
	}

	mask_row_binsize =
		MXIF_ROW_BINSIZE(aviex_pccd->geometrical_mask_frame);

	mask_column_binsize =
		MXIF_COLUMN_BINSIZE(aviex_pccd->geometrical_mask_frame);

#if MXD_AVIEX_PCCD_DEBUG_SETUP_GEOMETRICAL_MASK
	MX_DEBUG(-2,("%s: mask_row_binsize = %ld, mask_column_binsize = %ld",
		fname, mask_row_binsize, mask_column_binsize));
#endif

	/* Do we need to use a rebinned geometrical mask frame? */

	if ( mask_row_binsize != image_row_binsize ) {
		need_rebinned_mask_frame = TRUE;
	} else
	if ( mask_column_binsize != image_column_binsize ) {
		need_rebinned_mask_frame = TRUE;
	} else {
		need_rebinned_mask_frame = FALSE;
	}

#if MXD_AVIEX_PCCD_DEBUG_SETUP_GEOMETRICAL_MASK
	MX_DEBUG(-2,
	("%s: new_mask_frame_loaded = %d, need_rebinned_mask_frame = %d",
		fname, (int) new_mask_frame_loaded,
		(int) need_rebinned_mask_frame ));
#endif

	/* If it is needed, see if the correct rebinned mask image
	 * is already present.
	 */

	if ( need_rebinned_mask_frame == FALSE ) {

		/* If we do not need a rebinned mask frame, then discard any 
		 * such frame that currently exists.
		 */

		mx_image_free( aviex_pccd->rebinned_geometrical_mask_frame );

		aviex_pccd->rebinned_geometrical_mask_frame = NULL;
	} else {
		/* We _do_ need a rebinned mask frame.
		 * Do we need to create a new one now?
		 */

		create_rebinned_mask_frame = FALSE;

		if ( new_mask_frame_loaded ) {
			create_rebinned_mask_frame = TRUE;
		} else
		if ( aviex_pccd->rebinned_geometrical_mask_frame == NULL ) {
			create_rebinned_mask_frame = TRUE;
		} else {
			/* See if the existing rebinned mask frame already has
			 * the dimensions that we need.  If so, then we just
			 * use the already existing mask frame.
			 */

			rebinned_row_binsize =
	    MXIF_ROW_BINSIZE(aviex_pccd->rebinned_geometrical_mask_frame);
		
			rebinned_column_binsize =
	    MXIF_COLUMN_BINSIZE(aviex_pccd->rebinned_geometrical_mask_frame);

#if MXD_AVIEX_PCCD_DEBUG_SETUP_GEOMETRICAL_MASK
			MX_DEBUG(-2,
	    ("%s: rebinned_row_binsize = %ld, rebinned_column_binsize = %ld",
			fname, rebinned_row_binsize, rebinned_column_binsize));
#endif
	    		if ( (rebinned_row_binsize != image_row_binsize)
			  || (rebinned_column_binsize != image_column_binsize) )
			{
				create_rebinned_mask_frame = TRUE;
			}
		}

		/* If necessary, create a new rebinned mask frame. */

		if ( create_rebinned_mask_frame ) {
			unsigned long row_framesize, column_framesize;

			row_framesize = MXIF_ROW_FRAMESIZE(image_frame);
			column_framesize = MXIF_COLUMN_FRAMESIZE(image_frame);

			mx_status = mx_image_rebin(
			    &(aviex_pccd->rebinned_geometrical_mask_frame),
			    	aviex_pccd->geometrical_mask_frame,
				row_framesize, column_framesize );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	/* Return the frame buffer pointer for the geometrical mask frame
	 * that we are using.
	 */

	if ( need_rebinned_mask_frame == FALSE ) {
		mask_frame = aviex_pccd->geometrical_mask_frame;
	} else {
		mask_frame = aviex_pccd->rebinned_geometrical_mask_frame;
	}

#if MXD_AVIEX_PCCD_DEBUG_SETUP_GEOMETRICAL_MASK
	MX_DEBUG(-2,("%s: aviex_pccd->geometrical_mask_frame = %p",
		fname, aviex_pccd->geometrical_mask_frame));

	MX_DEBUG(-2,("%s: aviex_pccd->rebinned_geometrical_mask_frame = %p",
		fname, aviex_pccd->rebinned_geometrical_mask_frame));

	MX_DEBUG(-2,("%s: mask_frame = %p", fname, mask_frame));
#endif

	mx_status = mx_image_get_image_data_pointer( mask_frame, NULL,
							&image_data_pointer );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*geometrical_mask_frame_buffer = image_data_pointer;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_geometrical_correction( MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame )
{
	static const char fname[] = "mxd_aviex_pccd_geometrical_correction()";

	MX_AVIEX_PCCD *aviex_pccd;
	int spatial_status, os_status, saved_errno;
	int row_framesize, column_framesize;
	uint16_t *geometrical_mask_frame_buffer;
	mx_status_type mx_status;

	/* Suppress stupid GCC 4 initialization warnings. */

	aviex_pccd = NULL;
	geometrical_mask_frame_buffer = NULL;

	mx_status = mxd_aviex_pccd_get_pointers( ad, &aviex_pccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {

		if ( ad->image_frame != (MX_IMAGE_FRAME *) NULL ) {
			image_frame = ad->image_frame;
		} else {
			return mx_error( MXE_NOT_READY, fname,
			"Area detector '%s' has not yet taken an image frame.",
				ad->record->name );
		}
	}

	if ( image_frame->image_data == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	  "The image_frame->image_data pointer for area detector '%s' is NULL.",
			ad->record->name );
	}
	if ( aviex_pccd->geometrical_spline_filename == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The geometrical_spline_filename pointer for "
		"area detector '%s' is NULL.", ad->record->name );
	}
	if ( strlen( aviex_pccd->geometrical_spline_filename ) == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"No geometrical correction spline filename was provided for "
		"area detector '%s'.", ad->record->name );
	}
	if ( aviex_pccd->geometrical_mask_filename == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The geometrical_mask_filename pointer for "
		"area detector '%s' is NULL.", ad->record->name );
	}
	if ( strlen( aviex_pccd->geometrical_mask_filename ) == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"No geometrical correction mask filename was provided for "
		"area detector '%s'.", ad->record->name );
	}

	os_status = access( aviex_pccd->geometrical_spline_filename, R_OK );

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot read geometrical correction spline file '%s'.  "
		"errno = %d, error message = '%s'.",
			aviex_pccd->geometrical_spline_filename,
			saved_errno, strerror(saved_errno) );
	}

	os_status = access( aviex_pccd->geometrical_mask_filename, R_OK );

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot read geometrical correction mask file '%s'.  "
		"errno = %d, error message = '%s'.",
			aviex_pccd->geometrical_mask_filename,
			saved_errno, strerror(saved_errno) );
	}

	/* Make sure that the geometrical mask frame is binned to the
	 * correct size.
	 */

	DISPLAY_MEMORY_USAGE( "BEFORE setup of geometrical mask" );

	mx_status = mxd_aviex_pccd_setup_geometrical_mask_frame(
			ad, aviex_pccd, image_frame,
			&geometrical_mask_frame_buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	row_framesize = MXIF_ROW_FRAMESIZE(image_frame);
	column_framesize = MXIF_COLUMN_FRAMESIZE(image_frame);

#if MXD_AVIEX_PCCD_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("BEFORE smvspatial(), image_frame histogram = "));
	mx_image_statistics( image_frame );
#endif

#if 0
	DISPLAY_MEMORY_USAGE( "BEFORE kludge" );

	/* Kludge: free internal smvspatial() memory. */

	smvspatial( NULL, -1, -1, 0, NULL, NULL );
#endif

	DISPLAY_MEMORY_USAGE( "BEFORE smvspatial()" );

#if defined(OS_LINUX) || defined(OS_MACOSX) || defined(_MSC_VER)
	/* Perform the geometrical correction. */

	spatial_status = smvspatial( image_frame->image_data, 
				row_framesize, column_framesize, 0,
				aviex_pccd->geometrical_spline_filename,
				geometrical_mask_frame_buffer );
#else
	mx_warning("XGEN geometrical correction is currently only available "
		"on Linux, Windows, and MacOS X.");

	spatial_status = 0;
#endif

#if 0
	fflush(stdout);		/* Thank you MSI! */
#endif

	DISPLAY_MEMORY_USAGE( "AFTER smvspatial()" );

	switch( spatial_status ) {
	case 0:		/* Spatial correction succeeded. */
		mx_status = MX_SUCCESSFUL_RESULT;
		break;
	case ENOENT:
		mx_status = mx_error( MXE_FILE_IO_ERROR, fname,
			"smvspatial() was unable to open one of the correction "
			"files '%s' or '%s' for area detector '%s'.",
				aviex_pccd->geometrical_spline_filename,
				aviex_pccd->geometrical_mask_filename,
				ad->record->name );
		break;
	case EIO:
		mx_status = mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred in smvspatial() while reading one "
			"of the correction files '%s' or '%s' for "
			"area detector '%s'.",
				aviex_pccd->geometrical_spline_filename,
				aviex_pccd->geometrical_mask_filename,
				ad->record->name );
		break;
	case ENOMEM:
		mx_status = mx_error( MXE_OUT_OF_MEMORY, fname,
			"smvspatial() was unable to allocate memory for "
			"spatial correction of the current image for "
			"area detector '%s'.",
				ad->record->name );
		break;
	case EINVAL:
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"An illegal value was seen by smvspatial() during "
			"spatial correction of the current image for "
			"area detector '%s'.",
				ad->record->name );
		break;
	default:
		mx_status = mx_error( MXE_UNKNOWN_ERROR, fname,
			"Unexpected error code %d was returned by "
			"smvspatial() for area detector '%s'.",
				spatial_status, ad->record->name );
		break;
	}

#if MXD_AVIEX_PCCD_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("AFTER smvspatial(), image_frame histogram = "));
	mx_image_statistics( image_frame );
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_camera_link_command( MX_AVIEX_PCCD *aviex_pccd,
					char *command,
					char *response,
					size_t max_response_length,
					int debug_flag )
{
	static const char fname[] = "mxd_aviex_pccd_camera_link_command()";

	MX_RECORD *camera_link_record;
	size_t command_length, response_length;
	size_t i, bytes_available, bytes_to_read, total_bytes_read, buffer_left;
	long total_bytes_expected;
	char *ptr;
	unsigned long use_dh_simulator;
	double timeout_in_seconds;
	MX_CLOCK_TICK timeout_in_clock_ticks;
	MX_CLOCK_TICK start_tick, timeout_tick, current_tick;
	int comparison;
	mx_status_type mx_status;

#if MXD_AVIEX_PCCD_DEBUG_SERIAL
	debug_flag |= 1;
#endif

	if ( aviex_pccd == (MX_AVIEX_PCCD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AVIEX_PCCD pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer passed for record '%s' was NULL.",
			aviex_pccd->record->name );
	}
	if ( response == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The response pointer passed for record '%s' was NULL.",
			aviex_pccd->record->name );
	}
	if ( max_response_length < 1 ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The requested response buffer length %lu for record '%s' "
		"is too short to hold a minimum length response.",
			(unsigned long) max_response_length,
			aviex_pccd->record->name );
	}

	camera_link_record = aviex_pccd->camera_link_record;

	if ( camera_link_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The camera_link_record pointer for record '%s' is NULL.",
			aviex_pccd->record->name );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sent '%s' to '%s'",
			fname, command, camera_link_record->name ));
	}

	use_dh_simulator = aviex_pccd->aviex_pccd_flags
				& MXF_AVIEX_PCCD_USE_DETECTOR_HEAD_SIMULATOR;

	if ( use_dh_simulator ) {

		/* Talk to a simulated detector head. */

		mx_status = mxd_aviex_pccd_simulated_cl_command( 
						aviex_pccd, command,
						response, max_response_length,
						debug_flag );
	} else {
		/* Talk to the real detector head. */

		/* Throw away any unread input.  It is likely to be garbage
		 * from electromagnetic interference.
		 */

		mx_status = mx_camera_link_flush_port( camera_link_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Send the command. */

		command_length = strlen(command);

		mx_status = mx_camera_link_serial_write( camera_link_record,
						command, &command_length, -1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Arrange for a timeout if it takes too long to get a
		 * response from the detector head.
		 */

		timeout_in_seconds = 1.0;

		timeout_in_clock_ticks = 
			mx_convert_seconds_to_clock_ticks( timeout_in_seconds );

		start_tick = mx_current_clock_tick();

		timeout_tick = mx_add_clock_ticks( start_tick,
						timeout_in_clock_ticks );

		/* For some detectors, we must count the number
		 * of characters received rather than look for
		 * a carriage return character.
		 */

		switch( aviex_pccd->record->mx_type ) {
		case MXT_AD_PCCD_16080:
			switch( command[0] ) {
			case 'A':
				total_bytes_expected = 5;
				break;
			case 'R':
				total_bytes_expected = 4;
				break;
			case 'W':
				total_bytes_expected = 1;
				break;
			default:
				total_bytes_expected = 0;
				break;
			}
			break;
		default:
			total_bytes_expected = -1;
			break;
		}
		
		/* Leave room in the response buffer to null terminate
		 * the response.
		 */

		response_length = max_response_length - 1;

		/* Read in the response. */

		total_bytes_read = 0;

		for (;;) {
			mx_msleep(1);	/* Wait 1 millisecond. */

			mx_status = mx_camera_link_get_num_bytes_avail(
							camera_link_record,
							&bytes_available );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			ptr = response + total_bytes_read;

			buffer_left = response_length - total_bytes_read;

			if ( bytes_available < 1 ) {
#if 0 && MXD_AVIEX_PCCD_DEBUG_SERIAL
				MX_DEBUG(-2,
				("%s: No bytes available from '%s'.",
					fname, camera_link_record->name));
#endif
			}

#if 0
			bytes_to_read = 1;
#else
			if ( bytes_available > 0 ) {
				bytes_to_read = 1;
			} else {
				bytes_to_read = 0;
			}
#endif

			if ( bytes_to_read > buffer_left ) {
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"The number of bytes that we want to "
				"read (%ld) is larger than the space "
				"left (%ld) in the "
				"response buffer for Camera Link record '%s'.",
					(long) bytes_to_read,
					(long) buffer_left,
					camera_link_record->name );
			}

			/* The Camera Link specification implies that you
			 * should never attempt to read more characters than
			 * were said to be available by a previous call to
			 * mx_camera_link_get_num_bytes_available().
			 *
			 * If you do attempt to read more bytes than are
			 * available, the spec says that no bytes should
			 * be read and you should get back an error code.
			 */

			mx_status = mx_camera_link_serial_read(
				camera_link_record, ptr, &bytes_to_read, -1);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			total_bytes_read += bytes_to_read;

			if ( total_bytes_expected >= 0 ) {
				if ( total_bytes_read >= total_bytes_expected )
				{
					/* We have received all of the
					 * characters we expected, so
					 * we break out of for(;;) loop.
					 */
					break;
				}
			} else {
				/* Were there any carriage returns in the bytes
				 * that we just received?  If so, we have
				 * reached the end of the response and can
				 * now break out of this loop.
				 */

				for ( i = 0; i < bytes_to_read; i++ ) {
					if ( ptr[i] == MX_CR ) {
						ptr[i] = '\0';

						/* Exit the for(i) loop. */
						break;
					}
				}

				if ( i < bytes_to_read ) {
					/* We saw a carriage return, so break
					 * out of the for(;;) loop as well.
					 */
					break;
				}
			}

			current_tick = mx_current_clock_tick();

			comparison = mx_compare_clock_ticks( current_tick,
								timeout_tick );

			if ( comparison >= 0 ) {
				return mx_error( MXE_TIMED_OUT, fname,
				"Timed out after waiting %g seconds for "
				"a response to the '%s' command sent to '%s'.",
					timeout_in_seconds, command,
					camera_link_record->name );
			}
		}

		response[total_bytes_read] = '\0';
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
			fname, response, camera_link_record->name ));
	}

	if ( response[0] == 'E' ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error response received for command '%s' sent to "
		"Camera Link interface '%s'.",
			command, camera_link_record->name );
	}
	
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_read_register( MX_AVIEX_PCCD *aviex_pccd,
				unsigned long register_address,
				unsigned long *register_value )
{
	static const char fname[] = "mxd_aviex_pccd_read_register()";

	MX_AVIEX_PCCD_REGISTER *reg;
	char command[20], response[20];
	int num_items;
	mx_status_type mx_status;

	if ( aviex_pccd == (MX_AVIEX_PCCD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AVIEX_PCCD pointer passed was NULL." );
	}
	if ( register_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The register_value pointer passed for record '%s' was NULL.",
			aviex_pccd->record->name );
	}

	if ( register_address >= MXLV_AVIEX_PCCD_DH_BASE ) {
		register_address -= MXLV_AVIEX_PCCD_DH_BASE;
	}

	reg = &(aviex_pccd->register_array[register_address]);

	/* If the register is write only, then return a value with all bits
	 * set to 1.
	 */

	if ( reg->write_only ) {
		*register_value = ~0;

		return MX_SUCCESSFUL_RESULT;
	}

	switch( aviex_pccd->record->mx_type ) {
	case MXT_AD_PCCD_170170:
		snprintf(command, sizeof(command), "R%03lu", register_address);
		break;
	case MXT_AD_PCCD_4824:
	case MXT_AD_PCCD_16080:
		snprintf(command, sizeof(command), "R%02lu", register_address);
		break;
	}

	mx_status = mxd_aviex_pccd_camera_link_command( aviex_pccd,
					command, response, sizeof(response),
					MXD_AVIEX_PCCD_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "S%lu", register_value );

	if ( num_items != 1 ) {
		/* The first read from the detector head may fail if the
		 * detector head has just been powered on.  If so, then
		 * we just retry the command without generating an
		 * error message.
		 */

		if ( aviex_pccd->first_dh_command ) {
			aviex_pccd->first_dh_command = FALSE;

			mx_status = mxd_aviex_pccd_read_register(
						aviex_pccd,
						register_address,
						register_value );

			return mx_status;
		} else {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Could not find the register value in the response "
			"'%s' by detector '%s' to the command '%s'.",
				response, aviex_pccd->record->name, command );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_write_register( MX_AVIEX_PCCD *aviex_pccd,
				unsigned long register_address,
				unsigned long register_value )
{
	static const char fname[] = "mxd_aviex_pccd_write_register()";

	MX_AVIEX_PCCD_REGISTER *reg;
	char command[20], response[20];
	unsigned long value_read, flags;
	mx_status_type mx_status;

	if ( aviex_pccd == (MX_AVIEX_PCCD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AVIEX_PCCD pointer passed was NULL." );
	}

	flags = aviex_pccd->aviex_pccd_flags;

	if ( register_address >= MXLV_AVIEX_PCCD_DH_BASE ) {
		register_address -= MXLV_AVIEX_PCCD_DH_BASE;
	}

	reg = &(aviex_pccd->register_array[register_address]);

	mx_status = mxd_aviex_pccd_check_value( aviex_pccd,
					register_address, register_value, NULL);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( aviex_pccd->record->mx_type ) {
	case MXT_AD_PCCD_170170:
		snprintf( command, sizeof(command),
			"W%03lu%05lu", register_address, register_value );
		break;
	case MXT_AD_PCCD_4824:
		snprintf( command, sizeof(command),
			"W%02lu%05lu", register_address, register_value );
		break;
	case MXT_AD_PCCD_16080:
		snprintf( command, sizeof(command),
			"W%02lu%03lu", register_address, register_value );
		break;
	}

	/* Write the new value. */

	mx_status = mxd_aviex_pccd_camera_link_command( aviex_pccd,
					command, response, sizeof(response),
					MXD_AVIEX_PCCD_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( reg->write_only == FALSE ) {

#if 1
		if ( aviex_pccd->record->mx_type == MXT_AD_PCCD_4824 ) {
			mx_msleep(100);
		}
#endif

		/* Read the value back to verify that the value
		 * was set correctly.
		 */

		mx_status = mxd_aviex_pccd_read_register( aviex_pccd,
						register_address, &value_read );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Verify that the value read back matches the value sent. */

		if ( value_read != register_value ) {
			return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
			"The attempt to set '%s' register %lu to %lu "
			"appears to have failed since the value read back "
			"from that register is now %lu.",
				aviex_pccd->record->name, register_address,
				register_value, value_read );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

/* mxp_aviex_pccd_geometrical_mask_changed() is invoked if someone has
 * written to the 'geometrical_mask_filename' field.  All the function
 * does is discard any existing geometrical mask data structures.
 */

static mx_status_type
mxp_aviex_pccd_geometrical_mask_changed( MX_RECORD *record )
{
	static const char fname[] =
			"mxp_aviex_pccd_geometrical_mask_changed()";

	MX_AVIEX_PCCD *aviex_pccd;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	aviex_pccd = record->record_type_struct;

	if ( aviex_pccd == (MX_AVIEX_PCCD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AVIEX_PCCD pointer for record '%s' is NULL.",
			record->name );
	}

#if MXD_AVIEX_PCCD_DEBUG_SETUP_GEOMETRICAL_MASK
	MX_DEBUG(-2,("%s invoked for area detector '%s', filename = '%s'",
		fname, record->name, aviex_pccd->geometrical_mask_filename));
#endif

	mx_image_free( aviex_pccd->geometrical_mask_frame );

	aviex_pccd->geometrical_mask_frame = NULL;

	mx_image_free( aviex_pccd->rebinned_geometrical_mask_frame );

	aviex_pccd->rebinned_geometrical_mask_frame = NULL;

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_special_processing_setup( MX_RECORD *record )
{
#if MXD_AVIEX_PCCD_DEBUG
	static const char fname[] = "mxd_aviex_pccd_special_processing_setup()";
#endif

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

#if MXD_AVIEX_PCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		if ( record_field->label_value >= MXLV_AVIEX_PCCD_DH_BASE ) {
			record_field->process_function
					= mxd_aviex_pccd_process_function;
		} else {
			switch( record_field->label_value ) {
			case MXLV_AVIEX_PCCD_GEOMETRICAL_MASK_FILENAME:
				record_field->process_function
					= mxd_aviex_pccd_process_function;
				break;
			default:
				break;
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_aviex_pccd_process_function( void *record_ptr,
				void *record_field_ptr,
				int operation )
{
	static const char fname[] = "mxd_aviex_pccd_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_AREA_DETECTOR *ad;
	unsigned long *register_value_ptr;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	record_field = (MX_RECORD_FIELD *) record_field_ptr;

	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AREA_DETECTOR pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		if ( record_field->label_value >= MXLV_AVIEX_PCCD_DH_BASE ) {
			mx_status = mx_area_detector_get_register(
					record, record_field->name, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			register_value_ptr =
				mx_get_field_value_pointer( record_field );

			*register_value_ptr = ad->register_value;
		}
		break;

	case MX_PROCESS_PUT:
		if ( record_field->label_value >= MXLV_AVIEX_PCCD_DH_BASE ) {

			register_value_ptr =
				mx_get_field_value_pointer( record_field );

			mx_status = mx_area_detector_set_register(
					record, record_field->name,
					*register_value_ptr );
		} else {
			switch( record_field->label_value ) {
			case MXLV_AVIEX_PCCD_GEOMETRICAL_MASK_FILENAME:
				mx_status =
			    mxp_aviex_pccd_geometrical_mask_changed( record );

				break;
			default:
				break;
			}
		}
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unknown operation code = %d for record '%s'.",
			operation, record->name );
	}

	return mx_status;
}

