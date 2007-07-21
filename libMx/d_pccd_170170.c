/*
 * Name:    d_pccd_170170.c
 *
 * Purpose: MX driver for the Aviex PCCD-170170 CCD detector.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PCCD_170170_DEBUG				TRUE

#define MXD_PCCD_170170_DEBUG_DESCRAMBLING		FALSE

#define MXD_PCCD_170170_DEBUG_ALLOCATION		FALSE

#define MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS	FALSE

#define MXD_PCCD_170170_DEBUG_SERIAL			TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_ascii.h"
#include "mx_clock.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_key.h"
#include "mx_bit.h"
#include "mx_hrt.h"
#include "mx_image.h"
#include "mx_digital_output.h"
#include "mx_video_input.h"
#include "mx_camera_link.h"
#include "mx_area_detector.h"
#include "d_pccd_170170.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_pccd_170170_record_function_list = {
	mxd_pccd_170170_initialize_type,
	mxd_pccd_170170_create_record_structures,
	mxd_pccd_170170_finish_record_initialization,
	mxd_pccd_170170_delete_record,
	NULL,
	NULL,
	NULL,
	mxd_pccd_170170_open,
	mxd_pccd_170170_close,
	NULL,
	mxd_pccd_170170_resynchronize,
	mxd_pccd_170170_special_processing_setup
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_pccd_170170_function_list = {
	mxd_pccd_170170_arm,
	mxd_pccd_170170_trigger,
	mxd_pccd_170170_stop,
	mxd_pccd_170170_abort,
	NULL,
	NULL,
	NULL,
	mxd_pccd_170170_get_extended_status,
	mxd_pccd_170170_readout_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_pccd_170170_get_parameter,
	mxd_pccd_170170_set_parameter,
	mx_area_detector_default_measure_correction
};

MX_RECORD_FIELD_DEFAULTS mxd_pccd_170170_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_PCCD_170170_STANDARD_FIELDS
};

long mxd_pccd_170170_num_record_fields
		= sizeof( mxd_pccd_170170_record_field_defaults )
			/ sizeof( mxd_pccd_170170_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pccd_170170_rfield_def_ptr
			= &mxd_pccd_170170_record_field_defaults[0];

/*---*/

#define INIT_REGISTER( i, v, r, t, n, x ) \
	do {                                                          \
		mx_status = mxd_pccd_170170_init_register(            \
			pccd_170170, (i), (v), (r), (t), (n), (x) );  \
	                                                              \
		if ( mx_status.code != MXE_SUCCESS )                  \
			return mx_status;                             \
	} while(0)

/*---*/

static mx_status_type mxd_pccd_170170_process_function( void *record_ptr,
							void *record_field_ptr,
							int operation );

static mx_status_type
mxd_pccd_170170_get_pointers( MX_AREA_DETECTOR *ad,
			MX_PCCD_170170 **pccd_170170,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pccd_170170_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pccd_170170 == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PCCD_170170 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pccd_170170 = (MX_PCCD_170170 *)
				ad->record->record_type_struct;

	if ( *pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_PCCD_170170 pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

#if MXD_PCCD_170170_DEBUG_ALLOCATION
static void
mxd_pccd_170170_display_ul_corners( uint16_t ***sector_array, int num_sectors )
{
	static const char fname[] = "mxd_pccd_170170_display_ul_corners()";

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

static mx_status_type
mxd_pccd_170170_alloc_sector_array( uint16_t ****sector_array_ptr,
					long sector_width,
					long sector_height,
					uint16_t *image_data )
{
	static const char fname[] = "mxd_pccd_170170_alloc_sector_array()";

	uint16_t ***sector_array;
	uint16_t **sector_array_row_ptr;
	long num_sector_rows, num_sector_columns, num_sectors;
	long n, sector_row, row, sector_column;
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

#if MXD_PCCD_170170_DEBUG_ALLOCATION
	MX_DEBUG(-2,("%s: sector_width = %ld, sector_height = %ld",
		fname, sector_width, sector_height));
#endif

	num_sector_rows = 4;
	num_sector_columns = 4;
	num_sectors = num_sector_rows * num_sector_columns;

	*sector_array_ptr = NULL;
	
	sector_array = malloc( num_sectors * sizeof(uint16_t **) );

	if ( sector_array == (uint16_t ***) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element "
		"sector array pointer.", num_sectors );
	}

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
	MX_DEBUG(-2,("%s: allocated sector_array = %p, length = %lu",
		fname, sector_array, num_sectors * sizeof(uint16_t **) ));
#endif

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
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

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
	MX_DEBUG(-2,("%s: allocated sector_array_row_ptr = %p, length = %lu",
		fname, sector_array_row_ptr,
		num_sectors * sector_height * sizeof(uint16_t *) ));
#endif

	row_byte_offset = sector_height * sizeof(sector_array_row_ptr);

	row_ptr_offset = row_byte_offset / sizeof(uint16_t *);

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
	MX_DEBUG(-2,
	("%s: row_byte_offset = %ld (%#lx), row_ptr_offset = %ld (%#lx)",
		fname, row_byte_offset, row_byte_offset,
		row_ptr_offset, row_ptr_offset));

	MX_DEBUG(-2,("%s: sector_array_row_pointer = %p",
			fname, sector_array_row_ptr ));
#endif

	for ( n = 0; n < num_sectors; n++ ) {

		sector_array[n] = sector_array_row_ptr + n * row_ptr_offset;

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
		MX_DEBUG(-2,("%s: sector_array[%ld] = %p",
			fname, n, sector_array[n] ));
#endif
	}

	/* The 'sizeof_full_row' is the number of bytes in a single horizontal
	 * line of the image.  The 'sizeof_row_of_sectors' is the number of
	 * bytes in a horizontal row of sectors.  In unbinned mode,
	 * sizeof_row_of_sectors = 1024 * sizeof_full_row.
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
	 *   sizeof_row_of_sectors =
	 *                1024 * sizeof_full_row = 1024 * 4 * sizeof_sector_row
	 */

	sizeof_sector_row = sector_width * sizeof(uint16_t);

	sizeof_full_row = num_sector_columns * sizeof_sector_row;

	sizeof_row_of_sectors = sizeof_full_row * sector_height;

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
	MX_DEBUG(-2,
("%s: image_data = %p, sizeof_full_row = %#lx, sizeof_row_of_sectors = %#lx",
		fname, image_data, sizeof_full_row, sizeof_row_of_sectors));
#endif

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
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

		    n = sector_column + 4 * sector_row;

		    byte_offset = sector_row * sizeof_row_of_sectors
				+ row * sizeof_full_row
				+ sector_column * sizeof_sector_row;

		    ptr_offset = byte_offset / sizeof(uint16_t);

		    sector_array[n][row] = image_data + ptr_offset;

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
		    if ( row == 0 ) {
			MX_DEBUG(-2,
	("*** n = %ld, sector_row = %ld, sector_column = %ld, addr = %#lx ***",
				n, sector_row, sector_column,
				(long) byte_offset + (long) image_data));

#if MXD_PCCD_170170_DEBUG_DESCRAMBLING
			mxd_pccd_170170_display_ul_corners( sector_array,
								num_sectors );
#endif
		    }
#endif

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
		    MX_DEBUG(-2,
	    ("offset = %#lx = %#lx * (%#lx) + %#lx * (%#lx) + %#lx * (%#lx)",
			byte_offset, sizeof_row_of_sectors, sector_row,
			sizeof_full_row, row,
			sizeof_sector_row, sector_column));
#endif

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
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

#if MXD_PCCD_170170_DEBUG_ALLOCATION
	mxd_pccd_170170_display_ul_corners( sector_array, num_sectors );

#if 0
	fprintf(stderr, "Type any key to continue..." );
	mx_getch();
	fprintf(stderr, "\n");
#endif
#endif

	return MX_SUCCESSFUL_RESULT;
}

static void
mxd_pccd_170170_free_sector_array( uint16_t ***sector_array )
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

static mx_status_type
mxd_pccd_170170_descramble_raw_data( uint16_t *raw_frame_data,
				uint16_t ***image_sector_array,
				long i_framesize,
				long j_framesize )
{
#if MXD_PCCD_170170_DEBUG_DESCRAMBLING
	static const char fname[] = "mxd_pccd_170170_descramble_raw_data()";
#endif

	long i, j;

#if MXD_PCCD_170170_DEBUG_DESCRAMBLING
	mxd_pccd_170170_display_ul_corners( image_sector_array, num_sectors );
#endif

	for ( i = 0; i < i_framesize; i++ ) {
	    for ( j = 0; j < j_framesize; j++ ) {

		image_sector_array[0][i][j] = raw_frame_data[14];

		image_sector_array[1][i][j_framesize-j-1] = raw_frame_data[15];

		image_sector_array[2][i][j] = raw_frame_data[10];

		image_sector_array[3][i][j_framesize-j-1] = raw_frame_data[11];

		image_sector_array[4][i_framesize-i-1][j] = raw_frame_data[13];

		image_sector_array[5][i_framesize-i-1][j_framesize-j-1]
							= raw_frame_data[12];

		image_sector_array[6][i_framesize-i-1][j] = raw_frame_data[9];

		image_sector_array[7][i_framesize-i-1][j_framesize-j-1]
							= raw_frame_data[8];

		image_sector_array[8][i][j] = raw_frame_data[0];

		image_sector_array[9][i][j_framesize-j-1] = raw_frame_data[1];

		image_sector_array[10][i][j] = raw_frame_data[4];

		image_sector_array[11][i][j_framesize-j-1] = raw_frame_data[5];

		image_sector_array[12][i_framesize-i-1][j] = raw_frame_data[3];

		image_sector_array[13][i_framesize-i-1][j_framesize-j-1]
							= raw_frame_data[2];

		image_sector_array[14][i_framesize-i-1][j] = raw_frame_data[7];

		image_sector_array[15][i_framesize-i-1][j_framesize-j-1]
							= raw_frame_data[6];

		raw_frame_data += 16;
	    }
	}

#if 0 && MXD_PCCD_170170_DEBUG_DESCRAMBLING
	{
		long k;

		for ( k = 0; k < 16; k++ ) {
			MX_DEBUG(-2,("%s: ul_corner[%ld] = %d",
				fname, k, image_sector_array[k][0][0] ));
		}
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pccd_170170_descramble_image( MX_AREA_DETECTOR *ad,
				MX_PCCD_170170 *pccd_170170,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *raw_frame )
{
	static const char fname[] = "mxd_pccd_170170_descramble_image()";

	MX_SEQUENCE_PARAMETERS *sp;
	long i, i_framesize, j_framesize;
	uint16_t *frame_data;
	mx_status_type mx_status;

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}
	if ( raw_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The raw_frame pointer passed was NULL." );
	}

	sp = &(ad->sequence_parameters);

	/* If this is a subimage or streak camera frame, we must
	 * first descramble to a temporary frame.
	 */

	if ( (sp->sequence_type == MXT_SQ_SUBIMAGE)
	  || (sp->sequence_type == MXT_SQ_STREAK_CAMERA) )
	{
		mx_status = mx_image_alloc( &(pccd_170170->temp_frame),
					image_frame->image_type,
					image_frame->framesize,
					image_frame->image_format,
					image_frame->byte_order,
					image_frame->bytes_per_pixel,
					image_frame->header_length,
					image_frame->image_length );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		frame_data = pccd_170170->temp_frame->image_data;
	} else {
		frame_data = image_frame->image_data;
	}

#if 1
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

	i_framesize = image_frame->framesize[1] / 4;
	j_framesize = image_frame->framesize[0] / 4;

	if ( pccd_170170->sector_array == NULL ) {

		mx_status = mxd_pccd_170170_alloc_sector_array(
				&(pccd_170170->sector_array),
				j_framesize, i_framesize,
				frame_data );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Copy and descramble the pixels from the raw frame to the image frame.
	 */

	mx_status = mxd_pccd_170170_descramble_raw_data(
				raw_frame->image_data,
				pccd_170170->sector_array,
				i_framesize, j_framesize );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if (sp->sequence_type == MXT_SQ_STREAK_CAMERA) {
		/* FIXME: Temporarily, we just copy the image data from
		 * the temporary frame to the image frame.
		 */

		memcpy( image_frame->image_data, frame_data,
				image_frame->allocated_image_length );
	} else
	if (sp->sequence_type == MXT_SQ_SUBIMAGE) {

		long num_lines_per_subimage, num_subimages, frame_width;
		long bytes_per_half_subimage, bytes_per_half_image;
		long top_offset, bottom_offset;
		char *temp_ptr, *image_ptr;
		char *top_src_ptr, *top_dest_ptr;
		char *bottom_src_ptr, *bottom_dest_ptr;

		num_lines_per_subimage = sp->parameter_array[0];
		num_subimages          = sp->parameter_array[1];
		frame_width            = image_frame->framesize[0];

		temp_ptr  = pccd_170170->temp_frame->image_data;
		image_ptr = image_frame->image_data;

		bytes_per_half_subimage =
			frame_width * num_lines_per_subimage
			* ( sizeof(uint16_t) / 2L );

		bytes_per_half_image = image_frame->framesize[0]
				* image_frame->framesize[1]
				* ( sizeof(uint16_t) / 2L );

		MX_DEBUG(-2,("%s: num_lines_per_subimage = %ld",
			fname, num_lines_per_subimage));
		MX_DEBUG(-2,("%s: num_subimages = %ld", fname, num_subimages));
		MX_DEBUG(-2,("%s: frame_width = %ld", fname, frame_width));
		MX_DEBUG(-2,("%s: bytes_per_half_subimage = %ld",
			fname, bytes_per_half_subimage));
		MX_DEBUG(-2,("%s: bytes_per_half_image = %ld",
			fname, bytes_per_half_image));

		for ( i = 0; i < num_subimages; i++ ) {
			top_offset = bytes_per_half_image
					+ i * bytes_per_half_subimage;

			top_src_ptr = temp_ptr + top_offset;

			top_dest_ptr = image_ptr
				+ (2L * i) * bytes_per_half_subimage;

			memcpy( top_dest_ptr, top_src_ptr,
					bytes_per_half_subimage );

			bottom_offset = 2 * bytes_per_half_image
				- (i+1) * bytes_per_half_subimage;

			bottom_src_ptr = temp_ptr + bottom_offset;

			bottom_dest_ptr = image_ptr
				+ ((2L * i) + 1L) * bytes_per_half_subimage;

			memcpy( bottom_dest_ptr, bottom_src_ptr,
					bytes_per_half_subimage );
		}
	}

#if 1
	MX_DEBUG(-2,("%s: Image descrambling complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pccd_170170_check_value( MX_PCCD_170170 *pccd_170170,
				unsigned long register_address,
				unsigned long register_value,
				MX_PCCD_170170_REGISTER **register_pointer )
{
	static const char fname[] = "mxd_pccd_170170_check_value()";

	MX_PCCD_170170_REGISTER *reg;

	if ( register_address < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal register address %lu.", register_address );
	} else
	if ( register_address >= pccd_170170->num_registers ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal register address %lu.", register_address );
	}

	reg = &(pccd_170170->register_array[register_address]);

	if ( reg->read_only ) {
		return mx_error( MXE_READ_ONLY, fname,
			"Register %lu is read only.", register_address );
	}

	if ( register_value < reg->minimum ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested value %lu for register %lu is less than the "
		"minimum allowed value of %lu.", register_value,
			register_address, reg->minimum );
	}

	if ( register_value > reg->maximum ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested value %lu for register %lu is greater than the "
		"maximum allowed value of %lu.", register_value,
			register_address, reg->maximum );
	}

	if ( register_pointer != (MX_PCCD_170170_REGISTER **) NULL ) {
		*register_pointer = reg;
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pccd_170170_init_register( MX_PCCD_170170 *pccd_170170,
				long register_index,
				unsigned long register_value,
				mx_bool_type read_only,
				mx_bool_type power_of_two,
				unsigned long minimum,
				unsigned long maximum )
{
	static const char fname[] = "mxd_pccd_170170_init_register()";

	MX_PCCD_170170_REGISTER *reg;
	long register_address;

	register_address = register_index - MXLV_PCCD_170170_DH_BASE;

	if ( register_address < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal register address %ld.", register_address );
	} else
	if ( register_address >= pccd_170170->num_registers ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal register address %ld.", register_address );
	}

	reg = &(pccd_170170->register_array[register_address]);

	reg->value        = register_value;
	reg->read_only    = read_only;
	reg->power_of_two = power_of_two;
	reg->minimum      = minimum;
	reg->maximum      = maximum;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pccd_170170_simulated_cl_command( MX_PCCD_170170 *pccd_170170,
					char *command,
					char *response,
					size_t max_response_length,
					int debug_flag )
{
	static const char fname[] = "mxd_pccd_170170_simulated_cl_command()";

	MX_PCCD_170170_REGISTER *reg;
	unsigned long register_address, register_value, combined_value;
	int num_items;
	mx_status_type mx_status;

	switch( command[0] ) {
	case 'R':
		num_items = sscanf( command, "R%lu", &register_address );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Illegal command '%s' sent to detector '%s'.",
				command, pccd_170170->record->name );
		}

		if ( register_address >= MX_PCCD_170170_NUM_REGISTERS ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Illegal register address %lu.",
				register_address );
		}

		reg = &(pccd_170170->register_array[register_address]);

		snprintf( response, max_response_length, "S%05lu", reg->value );
		break;
	case 'W':
		num_items = sscanf( command, "W%lu", &combined_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Illegal command '%s' sent to detector '%s'.",
				command, pccd_170170->record->name );
		}

		register_address = combined_value / 100000;
		register_value   = combined_value % 100000;

		mx_status = mxd_pccd_170170_check_value( pccd_170170,
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
mxd_pccd_170170_get_num_frames_in_sequence( MX_AREA_DETECTOR *ad,
					long *num_frames_in_sequence )
{
	MX_SEQUENCE_PARAMETERS *sp;
	long num_frames;

	sp = &(ad->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_CONTINUOUS:
	case MXT_SQ_STREAK_CAMERA:
	case MXT_SQ_SUBIMAGE:
		num_frames = 1;
		break;
	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_CIRCULAR_MULTIFRAME:
	case MXT_SQ_STROBE:
	case MXT_SQ_BULB:
	case MXT_SQ_GEOMETRICAL:
		num_frames = mx_round( sp->parameter_array[0] );
		break;
	default:
		num_frames = 0;
		break;
	}

	*num_frames_in_sequence = num_frames;

	return MX_SUCCESSFUL_RESULT;
}

/* PCCD-170170 detector readout time formula as of July 13, 2007. */

static mx_status_type
mxd_pccd_170170_compute_detector_readout_time( MX_AREA_DETECTOR *ad,
					MX_PCCD_170170 *pccd_170170,
					double *detector_readout_time )
{
#if MXD_PCCD_170170_DEBUG
	static const char fname[] =
		"mxd_pccd_170170_compute_detector_readout_time()";
#endif

	MX_SEQUENCE_PARAMETERS *sp;
	mx_bool_type high_speed;
	unsigned long control_register;
	unsigned long linenum, pixnum, linesrd, pixrd, lbin, pixbin;
	double t;
	mx_status_type mx_status;

	/* If we are using a detector head simulator,
	 * return a small fake value.
	 */

	if ( pccd_170170->pccd_170170_flags
				& MXF_PCCD_170170_USE_DETECTOR_HEAD_SIMULATOR )
	{
		*detector_readout_time = 0.01;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Otherwise, compute the real value. */

	sp = &(ad->sequence_parameters);

	/* Read out the necessary register values from the detector head. */

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_CONTROL,
				&control_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( control_register & 0x2 ) {
		high_speed = TRUE;
	} else {
		high_speed = FALSE;
	}

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_PHYSICAL_LINES_IN_QUADRANT,
				&linenum );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_PHYSICAL_PIXELS_IN_QUADRANT,
				&pixnum );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_LINES_READ_IN_QUADRANT,
				&linesrd );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_PIXELS_READ_IN_QUADRANT,
				&pixrd );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_LINE_BINNING,
				&lbin );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_PIXEL_BINNING,
				&pixbin );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/******************************************************************
	 *                    ... Here Be Dragons ...                     *
	 ******************************************************************/

	t = 0.0;

	switch( sp->sequence_type ) {
	case MXT_SQ_STREAK_CAMERA:
	    mx_warning(
	    "STREAK CAMERA DETECTOR READOUT TIME NOT YET IMPLEMENTED!" );

	    *detector_readout_time = 0.01;
	    break;

	case MXT_SQ_SUBIMAGE:
	    mx_warning(
	    "SUBIMAGE DETECTOR READOUT TIME NOT YET IMPLEMENTED!" );

	    *detector_readout_time = 0.01;
	    break;

	default:	/* Full Frame Mode */

	    if ( high_speed ) {		/* High Speed Mode */
		
		if ( (lbin == 1) && (pixbin == 1) ) {

		    t = (100.0e-6 + (360.0e-9 + 300.0e-9)
			+ 360.0e-9*(pixnum - 2))*(linenum);
		} else
		if ( (lbin > 1) && (pixbin == 1) ) {

		    t = (100.0e-6 + (360.0e-9 + 300.0e-9)
			+ 360.0e-9*(pixnum - 2))*(linenum - (linenum - 6))
			+ ((100.0e-6 + 60.0e-6) + (360.0e-9 + 300.0e-9)
			+ 360.0e-9*(pixnum - 2))*(((linenum - 6) - linesrd) / 2)
			+ ((100.0e-6 + 60.0e-6*(lbin - 1))
			+ (360.0e-9 + 300.0e-9)
			+ 360.0e-9*(pixnum - 2))*(linesrd / lbin);
		} else
		if ( (lbin == 1) && (pixbin < 16) ) {

		    t = (100.0e-6 + (360.0e-9 + 300.0e-9)
	+ (360.0e-9 + 300.0e-9*(pixbin - 1))*((pixnum - 2) / pixbin))*(linenum);

		} else
		if ( (lbin == 1) && (pixbin >= 16) ) {

		    t = (100.0e-6 + (360.0e-9 + 300.0e-9)
	    + (360.0e-9 + 300.0e-9*(7))*(((pixnum - 2) - pixrd) / 8)
	    + (360.0e-9 + 300.0e-9*(pixbin - 1))*(pixrd / pixbin))*(linenum);

		} else
		if ( (lbin > 1) && (pixbin < 16) ) {

		    t = (100.0e-6 + (360.0e-9 + 300.0e-9)
			+ (360.0e-9 + 300.0e-9*(pixbin - 1))
				*((pixnum - 2) / pixbin))
				*(linenum - (linenum - 6))
			+ ((100.0e-6 + 60.0e-6) + (360.0e-9 + 300.0e-9)
			+ (360.0e-9 + 300.0e-9*(pixbin - 1))
			*((pixnum - 2) / pixbin))*(((linenum - 6) - linesrd)/2)
			+ ((100.0e-6 + 60.0e-6*(lbin - 1))
			+ (360.0e-9 + 300.0e-9) + (360.0e-9
			+ 300.0e-9*(pixbin - 1))*((pixnum - 2) / pixbin))
				*(linesrd / lbin);

		} else
		if ( (lbin > 1) && (pixbin >= 16 ) ) {

		    t = (100.0e-6 + (360.0e-9 + 300.0e-9)
			+ (360.0e-9 + 300.0e-9*(7))*(((pixnum - 2) - pixrd) / 8)			+ (360.0e-9 + 300.0e-9*(pixbin - 1))*(pixrd / pixbin))
				* (linenum - (linenum - 6))
			+ ((100.0e-6 + 60.0e-6) + (360.0e-9 + 300.0e-9)
			+ (360.0e-9 + 300.0e-9*(7))*(((pixnum - 2) - pixrd) / 8)			+ (360.0e-9 + 300.0e-9*(pixbin - 1))*(pixrd / pixbin))
				*(((linenum - 6) - linesrd) / 2)
			+ ((100.0e-6 + 60.0e-6*(lbin - 1))
			+ (360.0e-9 + 300.0e-9)
			+ (360.0e-9 + 300.0e-9*(7))*(((pixnum - 2) - pixrd) / 8)
			+ (360.0e-9 + 300.0e-9*(pixbin - 1))*(pixrd / pixbin))
				*(linesrd / lbin);
		}
	    } else {			/* Low Noise Mode */

		if ( (lbin == 1) && (pixbin == 1) ) {

		    t = (100.0e-6 + (1.25e-6 + 300.0e-9)
			+ 1.25e-6 *(pixnum - 2))*(linenum);

		} else
		if ( (lbin > 1) && (pixbin == 1) ) {

		    t = (100.0e-6 + (1.25e-6 + 300.0e-9)
			+ 1.25e-6 *(pixnum - 2))*(linenum - (linenum - 6))
			+ ((100.0e-6 + 60.0e-6) + (1.25e-6 + 300.0e-9)
			+ 1.25e-6 *(pixnum - 2))*(((linenum - 6) - linesrd) / 2)
			+ ((100.0e-6 + 60.0e-6*(lbin - 1))
			+ (1.25e-6 + 300.0e-9) + 1.25e-6 *(pixnum - 2))
				*(linesrd / lbin);
		
		} else
		if ( (lbin == 1) && (pixbin < 16) ) {

		    t = (100.0e-6 + (1.25e-6 + 300.0e-9)
			+ (1.25e-6 + 300.0e-9*(pixbin - 1))
				*((pixnum - 2) / pixbin))*(linenum);

		} else
		if ( (lbin == 1) && (pixbin >= 16) ) {

		    t = (100.0e-6 + (1.25e-6 + 300.0e-9)
			+ (1.25e-6 + 300.0e-9*(7))*(((pixnum - 2) - pixrd) / 8)
			+ (1.25e-6 + 300.0e-9*(pixbin - 1))
				*(pixrd / pixbin))*(linenum);

		} else
		if ( (lbin > 1) && (pixbin < 16) ) {

		    t = (100.0e-6 + (1.25e-6 + 300.0e-9)
			+ (1.25e-6 + 300.0e-9*(pixbin - 1))
				*((pixnum - 2) / pixbin))
				*(linenum - (linenum - 6))
			+ ((100.0e-6 + 60.0e-6) + (1.25e-6 + 300.0e-9)
			+ (1.25e-6 + 300.0e-9*(pixbin - 1))
				*((pixnum - 2) / pixbin))
				*(((linenum - 6) - linesrd) / 2)
			+ ((100.0e-6 + 60.0e-6*(lbin - 1))
			+ (1.25e-6 + 300.0e-9)
			+ (1.25e-6 + 300.0e-9*(pixbin - 1))
				*((pixnum - 2) / pixbin))*(linesrd / lbin);

		} else
		if ( (lbin > 1) && (pixbin >= 16) ) {

		    t = (100.0e-6 + (1.25e-6 + 300.0e-9)
			+ (1.25e-6 + 300.0e-9*(7))*(((pixnum - 2) - pixrd) / 8)
			+ (1.25e-6 + 300.0e-9*(pixbin - 1))
				*(pixrd / pixbin))* (linenum - (linenum - 6))
			+ ((100.0e-6 + 60.0e-6) + (1.25e-6 + 300.0e-9)
			+ (1.25e-6 + 300.0e-9*(7))*(((pixnum - 2) - pixrd) / 8)
			+ (1.25e-6 + 300.0e-9*(pixbin - 1))*(pixrd / pixbin))
				*(((linenum - 6) - linesrd) / 2)
			+ ((100.0e-6 + 60.0e-6*(lbin - 1))
			+ (1.25e-6 + 300.0e-9) + (1.25e-6 + 300.0e-9*(7))
				*(((pixnum - 2) - pixrd) / 8)
			+ (1.25e-6 + 300.0e-9*(pixbin - 1))*(pixrd / pixbin))
				*(linesrd / lbin);
		}
	    }
		
	    *detector_readout_time = t;

	    break;
	}

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: detector_readout_time = %g",
			fname, *detector_readout_time));
#endif
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_pccd_170170_initialize_type( long record_type )
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
mxd_pccd_170170_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_pccd_170170_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_PCCD_170170 *pccd_170170;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	pccd_170170 = (MX_PCCD_170170 *)
				malloc( sizeof(MX_PCCD_170170) );

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_PCCD_170170 structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = pccd_170170;
	record->class_specific_function_list = 
			&mxd_pccd_170170_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	pccd_170170->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_finish_record_initialization( MX_RECORD *record )
{
	return mx_area_detector_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxd_pccd_170170_delete_record( MX_RECORD *record )
{
	static const char fname[] = "mxd_pccd_170170_delete_record()";

	MX_AREA_DETECTOR *ad;
	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pccd_170170 != NULL ) {
		if ( pccd_170170->raw_frame != NULL ) {
			mx_free( pccd_170170->raw_frame );
		}
		mx_free( pccd_170170 );
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
mxd_pccd_170170_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pccd_170170_open()";

	MX_AREA_DETECTOR *ad;
	MX_PCCD_170170 *pccd_170170;
	MX_RECORD *video_input_record;
	MX_VIDEO_INPUT *vinput;
	long vinput_framesize[2];
	unsigned long flags;
	unsigned long controller_fpga_version, comm_fpga_version;
	unsigned long control_register_value;
	size_t array_size;
	long master_clock;
	mx_bool_type camera_is_master;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	flags = pccd_170170->pccd_170170_flags;

	if ( flags & MXF_PCCD_170170_SUPPRESS_DESCRAMBLING ) {
		mx_warning( "Area detector '%s' will not descramble "
			"images from the camera head.",
				record->name );
	}
	if ( flags & MXF_PCCD_170170_USE_DETECTOR_HEAD_SIMULATOR ) {
		mx_warning( "Area detector '%s' will use "
	"an Aviex detector head simulator instead of a real detector head.",
				record->name );
	}

	video_input_record = pccd_170170->video_input_record;

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

	/* Make sure the internal trigger output is low. */

	mx_status = mx_digital_output_write(
				pccd_170170->internal_trigger_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize data structures used to specify attributes
	 * of each detector head register.
	 */

	array_size = MX_PCCD_170170_NUM_REGISTERS
			* sizeof(MX_PCCD_170170_REGISTER);

	/* Allocate memory for the simulated register array. */

	pccd_170170->num_registers = MX_PCCD_170170_NUM_REGISTERS;

	pccd_170170->register_array = malloc( array_size );

	if ( pccd_170170->register_array == NULL ) {
		pccd_170170->num_registers = 0;

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %d element array "
		"of simulated detector head registers for detector '%s.'",
			MX_PCCD_170170_NUM_REGISTERS,
			pccd_170170->record->name );
	}

	memset( pccd_170170->register_array, 0, array_size );

	/* Initialize register attributes. */

	INIT_REGISTER( MXLV_PCCD_170170_DH_CONTROL,
					0x184, FALSE, FALSE, 0,  0xffff );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OVERSCANNED_PIXELS_PER_LINE,
					4,     FALSE, FALSE, 1,  2048 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_PHYSICAL_LINES_IN_QUADRANT,
					1046,  FALSE, FALSE, 1,  8192 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_PHYSICAL_PIXELS_IN_QUADRANT,
					1050,  FALSE, FALSE, 1,  8192 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_LINES_READ_IN_QUADRANT,
					1024,  FALSE, FALSE, 1,  8192 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_PIXELS_READ_IN_QUADRANT,
					1024,  FALSE, FALSE, 1,  8192 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_SHUTTER_DELAY_TIME,
					0,     FALSE, FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_EXPOSURE_TIME,
					0,     FALSE, FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_READOUT_DELAY_TIME,
					0,     FALSE, FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_FRAMES_PER_SEQUENCE,
					1,     FALSE, FALSE, 1,  128 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_GAP_TIME,
					1,     FALSE, FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_EXPOSURE_MULTIPLIER,
					0,     FALSE, FALSE, 0,  255 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_GAP_MULTIPLIER,
					0,     FALSE, FALSE, 0,  255 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_CONTROLLER_FPGA_VERSION,
					17010, TRUE,  FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_LINE_BINNING,
					1,     FALSE, TRUE,  1,  128 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_PIXEL_BINNING,
					1,     FALSE, TRUE,  1,  128 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_SUBFRAME_SIZE,
					1024,  FALSE, TRUE,  16, 1024 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_SUBIMAGES_PER_READ,
					1,     FALSE, FALSE, 1,  128 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_STREAK_MODE_LINES,
					1,     FALSE, FALSE, 1,  65535 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_COMM_FPGA_VERSION,
					17010, TRUE,  FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_A1,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_A2,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_A3,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_A4,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_B1,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_B2,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_B3,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_B4,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_C1,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_C2,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_C3,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_C4,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_D1,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_D2,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_D3,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_D4,
					0,     FALSE, FALSE, 0,  65534 );

	/* Check to find out the firmware versions that are being used by
	 * the detector head.
	 */

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_CONTROLLER_FPGA_VERSION,
				&controller_fpga_version );

	/* The first read after the detector head is powered on may fail.
	 * If so, we try one more time.
	 */

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_info( "%s: Retrying read of register %d for detector '%s'.",
			fname,
    (MXLV_PCCD_170170_DH_CONTROLLER_FPGA_VERSION - MXLV_PCCD_170170_DH_BASE),
			record->name );

		mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_CONTROLLER_FPGA_VERSION,
				&controller_fpga_version );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_COMM_FPGA_VERSION,
				&comm_fpga_version );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: controller FPGA version = %lu",
			fname, controller_fpga_version ));
	MX_DEBUG(-2,("%s: communications FPGA version = %lu",
			fname, comm_fpga_version ));
#endif

	/* The PCCD-170170 camera generates 16 bit per pixel images. */

	vinput->bits_per_pixel = 16;
	ad->bits_per_pixel = vinput->bits_per_pixel;

	/* Set the default file format. */

	ad->frame_file_format = MXT_IMAGE_FILE_SMV;

	/* In unbinned mode, the Aviex camera is configured such that
	 * each line contains 1024 groups of pixels, with 16 pixels
	 * per group.  This means that for maximum resolution in
	 * full frame mode, the video input will be configured for a
	 * resolution of 16384 by 1024.  However, we want this to appear
	 * to the user to have a resolution of 4096 by 4096.  Thus we
         * must rescale the resolution as reported by the video card by
	 * multiplying and dividing by appropriate factors of 4.
	 */

	ad->maximum_framesize[0] = 4096;
	ad->maximum_framesize[1] = 4096;

	/* Set the default framesize and binning. */

	ad->framesize[0] = 4096;
	ad->framesize[1] = 4096;

	mx_status = mx_area_detector_set_binsize( record, 1, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the control register so that we can change it. */

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
					MXLV_PCCD_170170_DH_CONTROL,
					&control_register_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: OLD control_register_value = %#lx",
		fname, control_register_value));

	/* Put the detector head in full frame mode. */

	control_register_value &= (~0x60);

	/* Turn on an initial runt Frame Valid pulse.  This is used to
	 * work around a misfeature of the PIXCI E4 board.  The E4 board
	 * always ignores the first frame sent by the camera after 
	 * starting a new sequence.  EPIX says that this is to protect
	 * against a race condition in their system.  Aviex's solution
	 * is to send an extra runt Frame Valid pulse at the start of
	 * a sequence, so that the frame thrown away by EPIX is a frame
	 * that we do not want anyway.
	 */

	control_register_value |= 0x200;

	/* If requested, turn on the test mode pattern. */

	if ( flags & MXF_PCCD_170170_USE_TEST_PATTERN ) {
		control_register_value |= 0x1;

		mx_warning( "Area detector '%s' will return a test image "
			"instead of taking a real image.",
				record->name );
	} else {
		control_register_value &= (~0x1);
	}

	/* Write out the new control register value. */

	MX_DEBUG(-2,("%s: NEW control_register_value = %#lx",
		fname, control_register_value));

	mx_status = mxd_pccd_170170_write_register( pccd_170170,
					MXLV_PCCD_170170_DH_CONTROL,
					control_register_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The pixel clock frequency is 60 MHz. */

	mx_status = mx_video_input_set_pixel_clock_frequency(
				video_input_record, 6.0e7 );

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
				pccd_170170->initial_trigger_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Specify whether or not the camera head or the video board is
	 * in charge of generating the trigger pulse for sequences.
	 */

	camera_is_master =
	   (pccd_170170->pccd_170170_flags & MXF_PCCD_170170_CAMERA_IS_MASTER);

	if ( camera_is_master ) {
		master_clock = MXF_VIN_MASTER_CAMERA;
	} else {
		master_clock = MXF_VIN_MASTER_VIDEO_BOARD;
	}

	mx_status = mx_video_input_set_master_clock( video_input_record,
							master_clock );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Load the image correction files. */

	mx_status = mx_area_detector_load_correction_files( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize area detector parameters. */

	ad->byte_order = mx_native_byteorder();
	ad->header_length = 0;

	mx_status = mx_area_detector_get_image_format( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_bytes_per_pixel( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_bytes_per_frame( record, NULL );

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

	pccd_170170->vinput_normal_framesize[0] = vinput_framesize[0];
	pccd_170170->vinput_normal_framesize[1] = vinput_framesize[1];

	/* Allocate space for a raw frame buffer.  This buffer will be used to
	 * read in the raw pixels from the imaging board before descrambling.
	 */

#if 0
	MX_DEBUG(-2,("%s: Before final call to mx_image_alloc()", fname));

	MX_DEBUG(-2,("%s: &(pccd_170170->raw_frame) = %p",
			fname, &(pccd_170170->raw_frame) ));

	MX_DEBUG(-2,("%s: vinput_framesize = (%lu,%lu)",
			fname, vinput_framesize[0], vinput_framesize[1]));

	MX_DEBUG(-2,("%s: image_format = %ld", fname, ad->image_format ));
	MX_DEBUG(-2,("%s: byte_order = %ld", fname, ad->byte_order ));
	MX_DEBUG(-2,("%s: bytes_per_pixel = %g", fname, ad->bytes_per_pixel));
	MX_DEBUG(-2,("%s: header_length = %ld", fname, ad->header_length));
	MX_DEBUG(-2,("%s: bytes_per_frame = %ld", fname, ad->bytes_per_frame));
#endif

	mx_status = mx_image_alloc( &(pccd_170170->raw_frame),
					MXT_IMAGE_LOCAL_1D_ARRAY,
					vinput_framesize,
					ad->image_format,
					ad->byte_order,
					ad->bytes_per_pixel,
					ad->header_length,
					ad->bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pccd_170170->old_framesize[0] = -1;
	pccd_170170->old_framesize[1] = -1;

	pccd_170170->sector_array = NULL;

	/* Initialize the detector to one-shot mode with an exposure time
	 * of 1 second.
	 */

	mx_status = mx_area_detector_set_one_shot_mode( record, 1.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_pccd_170170_resynchronize()";

	MX_AREA_DETECTOR *ad;
	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	/* Send an 'Initialize CCD Controller' command to the detector head
	 * by toggling the CC4 line.
	 */

#if 0
	mx_status = mx_camera_link_pulse_cc_line(
					pccd_170170->camera_link_record, 4,
					-1, 1000 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	/* Restart the PIXCI library. */

	mx_status = mx_resynchronize_record( pccd_170170->video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_arm()";

	MX_PCCD_170170 *pccd_170170;
	MX_VIDEO_INPUT *vinput;
	MX_CLOCK_TICK num_ticks_to_wait, finish_tick;
	double timeout_in_seconds;
	int comparison;
	long num_frames_in_sequence;
	mx_bool_type camera_is_master, external_trigger, busy;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pccd_170170->video_input_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The video_input_record pointer for area detector '%s' is NULL.",
			ad->record->name );
	}

	vinput = pccd_170170->video_input_record->record_class_struct;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VIDEO_INPUT pointer for video input '%s' "
		"used by area detector '%s' is NULL.",
			pccd_170170->video_input_record->name,
			ad->record->name );
	}

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif
	camera_is_master =
	   (pccd_170170->pccd_170170_flags & MXF_PCCD_170170_CAMERA_IS_MASTER);

	external_trigger = (vinput->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER);

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: camera_is_master = %d, external_trigger = %d",
		fname, (int) camera_is_master, (int) external_trigger));
#endif

	mx_status = mx_video_input_abort( pccd_170170->video_input_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for up to 5 seconds for the video input to stop. */

	timeout_in_seconds = 5.0;

	num_ticks_to_wait =
		mx_convert_seconds_to_clock_ticks( timeout_in_seconds );

	finish_tick = mx_add_clock_ticks( mx_current_clock_tick(),
						num_ticks_to_wait );

	while(1) {
		mx_status = mx_area_detector_is_busy( ad->record, &busy );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( busy == FALSE ) {
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

	/* Prepare the video input for the next trigger. */

	if ( camera_is_master && external_trigger ) {
		mx_status = mxd_pccd_170170_get_num_frames_in_sequence(
						ad, &num_frames_in_sequence );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_video_input_asynchronous_capture(
					pccd_170170->video_input_record,
					num_frames_in_sequence, FALSE );
	} else {
		mx_status = mx_video_input_arm(
					pccd_170170->video_input_record );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_trigger()";

	MX_PCCD_170170 *pccd_170170;
	MX_VIDEO_INPUT *vinput;
	MX_SEQUENCE_PARAMETERS *sp;
	long num_frames_in_sequence;
	int i, num_triggers;
	mx_bool_type camera_is_master, internal_trigger;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pccd_170170->video_input_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The video_input_record pointer for area detector '%s' is NULL.",
			ad->record->name );
	}

	vinput = pccd_170170->video_input_record->record_class_struct;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VIDEO_INPUT pointer for video input '%s' "
		"used by area detector '%s' is NULL.",
			pccd_170170->video_input_record->name,
			ad->record->name );
	}

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sp = &(ad->sequence_parameters);

	/* Make sure we are configured for a supported sequence type. */

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_STROBE:
	case MXT_SQ_GEOMETRICAL:
	case MXT_SQ_SUBIMAGE:
	case MXT_SQ_STREAK_CAMERA:
		break;

	case MXT_SQ_CONTINUOUS:
	case MXT_SQ_CIRCULAR_MULTIFRAME:
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Image sequence type %ld is not supported by "
			"area detector '%s'.",
			sp->sequence_type, ad->record->name );
	}

	camera_is_master =
	   (pccd_170170->pccd_170170_flags & MXF_PCCD_170170_CAMERA_IS_MASTER);

	internal_trigger = (vinput->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER);

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: camera_is_master = %d, internal_trigger = %d",
		fname, (int) camera_is_master, (int) internal_trigger));
#endif

	if ( camera_is_master && internal_trigger ) {
		mx_status = mxd_pccd_170170_get_num_frames_in_sequence(
						ad, &num_frames_in_sequence );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_video_input_asynchronous_capture(
					pccd_170170->video_input_record,
					num_frames_in_sequence, FALSE );
	} else {
		/* Send the trigger request to the video input board. */

		mx_status = mx_video_input_trigger(
					pccd_170170->video_input_record );
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

			mx_status = mx_digital_output_write(
				pccd_170170->internal_trigger_record, 1 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_msleep(100);		/* Wait 0.1 seconds. */

			/* Set the output low. */

			mx_status = mx_digital_output_write(
				pccd_170170->internal_trigger_record, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( num_triggers > 1 ) {
				mx_msleep(100);		/* Wait 0.1 seconds. */
			}
		}
	}

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: Started taking a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_stop()";

	MX_PCCD_170170 *pccd_170170;
	MX_CLOCK_TICK timeout_in_ticks, current_tick, finish_tick;
	double timeout;
	int comparison;
	mx_bool_type busy, timed_out;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	/* Send an 'Abort Exposure' command to the detector head by
	 * toggling the CC2 line.
	 */

	mx_status = mx_camera_link_pulse_cc_line(
					pccd_170170->camera_link_record, 2,
					-1, 1000 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell the imaging board to stop acquiring frames after the end
	 * of the current frame.
	 */

	mx_status = mx_video_input_stop( pccd_170170->video_input_record );

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
				pccd_170170->video_input_record, &busy );

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
			timeout, pccd_170170->video_input_record->name );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

MX_EXPORT mx_status_type
mxd_pccd_170170_abort( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_abort()";

	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	/* Send a 'Reset CCD Controller' command to the detector head by
	 * toggling the CC3 line.
	 */

	mx_status = mx_camera_link_pulse_cc_line(
					pccd_170170->camera_link_record, 3,
					-1, 1000 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell the imaging board to immediately stop acquiring frames. */

	mx_status = mx_video_input_abort( pccd_170170->video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_get_extended_status()";

	MX_PCCD_170170 *pccd_170170;
	long last_frame_number;
	long total_num_frames;
	unsigned long status_flags;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	/* Ask the video board for its current status. */

	mx_status = mx_video_input_get_extended_status(
						pccd_170170->video_input_record,
						&last_frame_number,
						&total_num_frames,
						&status_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,
("%s: last_frame_number = %ld, total_num_frames = %ld, status_flags = %#lx",
		fname, last_frame_number, total_num_frames, status_flags));
#endif

	ad->last_frame_number = last_frame_number;

	ad->total_num_frames = total_num_frames;

	ad->status = 0;

	if ( status_flags & MXSF_VIN_IS_BUSY ) {
		ad->status |= MXSF_AD_IS_BUSY;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_readout_frame()";

	MX_PCCD_170170 *pccd_170170;
	unsigned long flags;
	size_t bytes_to_copy, raw_frame_length, image_length;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	/* Read in the raw image frame. */

	mx_status = mx_video_input_get_frame(
		pccd_170170->video_input_record,
		ad->readout_frame, &(pccd_170170->raw_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If required, we now descramble the image. */

	flags = pccd_170170->pccd_170170_flags;

	if ( flags & MXF_PCCD_170170_SUPPRESS_DESCRAMBLING ) {

		/* Just copy directly from the raw frame to the image frame. */

		raw_frame_length = pccd_170170->raw_frame->image_length;

		image_length = ad->image_frame->image_length;

		if ( raw_frame_length < image_length ) {
			bytes_to_copy = raw_frame_length;
		} else {
			bytes_to_copy = image_length;
		}

#if MXD_PCCD_170170_DEBUG
		MX_DEBUG(-2,
	    ("%s: Copying camera simulator image.  Image length = %lu bytes.",
			fname, (unsigned long) bytes_to_copy ));
#endif
		memcpy( ad->image_frame->image_data,
			pccd_170170->raw_frame->image_data,
			bytes_to_copy );
	} else {
		/* Descramble the image. */

		mx_status = mxd_pccd_170170_descramble_image( ad,
							pccd_170170,
							ad->image_frame,
							pccd_170170->raw_frame);
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_get_parameter()";

	MX_PCCD_170170 *pccd_170170;
	MX_RECORD *video_input_record;
	MX_RECORD_FIELD *field;
	long vinput_horiz_framesize, vinput_vert_framesize;
	MX_SEQUENCE_PARAMETERS seq;
	long i, num_frames;
	double exposure_time, frame_time, gap_time;
	double exposure_multiplier, gap_multiplier;
	char name_buffer[MXU_FIELD_NAME_LENGTH+1];
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter '%s'",
		fname, ad->record->name,
		mx_get_parameter_name_from_type(
			ad->record, ad->parameter_type,
			name_buffer, sizeof(name_buffer)) ));
#endif
	video_input_record = pccd_170170->video_input_record;

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
		 * mxd_pccd_170170_open() for the explanation of where the
		 * factors of 4 come from.
		 */

		ad->framesize[0] = 
			vinput_horiz_framesize / MXF_PCCD_170170_HORIZ_SCALE;

		ad->framesize[1] =
			vinput_vert_framesize * MXF_PCCD_170170_VERT_SCALE;
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

	case MXLV_AD_DETECTOR_READOUT_TIME:
		mx_status = mxd_pccd_170170_compute_detector_readout_time(
				ad, pccd_170170, &(ad->detector_readout_time) );
		break;

	case MXLV_AD_TOTAL_SEQUENCE_TIME:
		mx_status = mx_area_detector_get_sequence_parameters(
							ad->record, &seq );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_area_detector_get_detector_readout_time(
							ad->record, NULL );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch ( seq.sequence_type ) {
		case MXT_SQ_ONE_SHOT:
		case MXT_SQ_CONTINUOUS:
		case MXT_SQ_MULTIFRAME:
		case MXT_SQ_CIRCULAR_MULTIFRAME:
		case MXT_SQ_STROBE:
		case MXT_SQ_BULB:
			/* For these cases, use the default calculation. */

			mx_status =
			   mx_area_detector_default_get_parameter_handler( ad );
			break;

		case MXT_SQ_GEOMETRICAL:
			num_frames          = seq.parameter_array[0];
			exposure_time       = seq.parameter_array[1];
			frame_time          = seq.parameter_array[2];
			exposure_multiplier = seq.parameter_array[3];
			gap_multiplier      = seq.parameter_array[4];

			gap_time = frame_time - exposure_time
						- ad->detector_readout_time;

			ad->total_sequence_time = 0.0;

			for ( i = 0; i < num_frames; i++ ) {

				ad->total_sequence_time
						+= ad->detector_readout_time;

				ad->total_sequence_time += exposure_time;
				ad->total_sequence_time += gap_time;

				exposure_time *= exposure_multiplier;
				gap_time *= gap_multiplier;
			}
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Area detector '%s' is configured for unsupported "
			"sequence type %ld.",
				ad->record->name,
				seq.sequence_type );
		}
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		break;

	case MXLV_AD_TRIGGER_MODE:
		mx_status = mx_video_input_get_trigger_mode(
				video_input_record, &(ad->trigger_mode) );
		break;

	case MXLV_PCCD_170170_DH_CONTROL:
	case MXLV_PCCD_170170_DH_OVERSCANNED_PIXELS_PER_LINE:
	case MXLV_PCCD_170170_DH_PHYSICAL_LINES_IN_QUADRANT:
	case MXLV_PCCD_170170_DH_PHYSICAL_PIXELS_IN_QUADRANT:
	case MXLV_PCCD_170170_DH_LINES_READ_IN_QUADRANT:
	case MXLV_PCCD_170170_DH_PIXELS_READ_IN_QUADRANT:
	case MXLV_PCCD_170170_DH_SHUTTER_DELAY_TIME:
	case MXLV_PCCD_170170_DH_EXPOSURE_TIME:
	case MXLV_PCCD_170170_DH_READOUT_DELAY_TIME:
	case MXLV_PCCD_170170_DH_FRAMES_PER_SEQUENCE:
	case MXLV_PCCD_170170_DH_GAP_TIME:
	case MXLV_PCCD_170170_DH_EXPOSURE_MULTIPLIER:
	case MXLV_PCCD_170170_DH_GAP_MULTIPLIER:
	case MXLV_PCCD_170170_DH_CONTROLLER_FPGA_VERSION:
	case MXLV_PCCD_170170_DH_LINE_BINNING:
	case MXLV_PCCD_170170_DH_PIXEL_BINNING:
	case MXLV_PCCD_170170_DH_SUBFRAME_SIZE:
	case MXLV_PCCD_170170_DH_SUBIMAGES_PER_READ:
	case MXLV_PCCD_170170_DH_STREAK_MODE_LINES:
	case MXLV_PCCD_170170_DH_COMM_FPGA_VERSION:
	case MXLV_PCCD_170170_DH_OFFSET_A1:
	case MXLV_PCCD_170170_DH_OFFSET_A2:
	case MXLV_PCCD_170170_DH_OFFSET_A3:
	case MXLV_PCCD_170170_DH_OFFSET_A4:
	case MXLV_PCCD_170170_DH_OFFSET_B1:
	case MXLV_PCCD_170170_DH_OFFSET_B2:
	case MXLV_PCCD_170170_DH_OFFSET_B3:
	case MXLV_PCCD_170170_DH_OFFSET_B4:
	case MXLV_PCCD_170170_DH_OFFSET_C1:
	case MXLV_PCCD_170170_DH_OFFSET_C2:
	case MXLV_PCCD_170170_DH_OFFSET_C3:
	case MXLV_PCCD_170170_DH_OFFSET_C4:
	case MXLV_PCCD_170170_DH_OFFSET_D1:
	case MXLV_PCCD_170170_DH_OFFSET_D2:
	case MXLV_PCCD_170170_DH_OFFSET_D3:
	case MXLV_PCCD_170170_DH_OFFSET_D4:
		mx_status = mx_get_field_by_label_value( ad->record,
							ad->parameter_type,
							&field );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_pccd_170170_read_register( pccd_170170,
					ad->parameter_type,
					mx_get_field_value_pointer( field ) );
		break;

	default:
		mx_status = mx_area_detector_default_get_parameter_handler(ad);
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_set_parameter()";

	MX_PCCD_170170 *pccd_170170;
	MX_RECORD_FIELD *field;
	MX_SEQUENCE_PARAMETERS *sp;
	unsigned long *register_value_ptr;
	unsigned long old_control_register_value, new_control_register_value;
	unsigned long old_detector_readout_mode;
	unsigned long flags;
	long vinput_horiz_framesize, vinput_vert_framesize;
	long num_streak_mode_lines;
	long num_frames, exposure_steps, gap_steps;
	long exposure_multiplier_steps, gap_multiplier_steps;
	double exposure_time, frame_time, gap_time;
	double exposure_multiplier, gap_multiplier;
	double subimage_time, detector_readout_time;
	char name_buffer[MXU_FIELD_NAME_LENGTH+1];
	mx_status_type mx_status;

	static long allowed_binsize[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

	static int num_allowed_binsizes = sizeof( allowed_binsize )
						/ sizeof( allowed_binsize[0] );

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter '%s'",
		fname, ad->record->name,
		mx_get_parameter_name_from_type(
			ad->record, ad->parameter_type,
			name_buffer, sizeof(name_buffer)) ));
#endif
	flags = pccd_170170->pccd_170170_flags;

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:

		/* Invalidate any existing image sector array. */

		mxd_pccd_170170_free_sector_array(
				pccd_170170->sector_array );

		pccd_170170->sector_array = NULL;

		/* Find a compatible framesize and binsize. */

		mx_status = mx_area_detector_compute_new_binning( ad,
							ad->parameter_type,
							num_allowed_binsizes,
							allowed_binsize );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Tell the detector head to change its binsize. */

		mx_status = mxd_pccd_170170_write_register( pccd_170170,
					MXLV_PCCD_170170_DH_PIXEL_BINNING,
					ad->binsize[0] );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_pccd_170170_write_register( pccd_170170,
					MXLV_PCCD_170170_DH_LINE_BINNING,
					ad->binsize[1] );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Tell the video input to change its framesize. */

		/* See the comments about the Aviex camera in the function
		 * mxd_pccd_170170_open() for the explanation of where the
		 * factors of 4 come from.
		 */

		vinput_horiz_framesize =
			ad->framesize[0] * MXF_PCCD_170170_HORIZ_SCALE;

		vinput_vert_framesize =
			ad->framesize[1] / MXF_PCCD_170170_VERT_SCALE;

		mx_status = mx_video_input_set_framesize(
					pccd_170170->video_input_record,
					vinput_horiz_framesize,
					vinput_vert_framesize );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if 1 /* FIXME - I'm not sure if we want to do the following or not.    */
      /*         The alternative would be to _only_ save the resolution */
      /*         when we switch to streak camera mode.                  */

		/* If the resolution change succeeded, save the framesize for
		 * later use in switching to and from streak camera mode.
		 */

		pccd_170170->vinput_normal_framesize[0] =
						vinput_horiz_framesize;

		pccd_170170->vinput_normal_framesize[1] =
						vinput_vert_framesize;
#endif
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

		mxd_pccd_170170_free_sector_array(
				pccd_170170->sector_array );

		pccd_170170->sector_array = NULL;

		/* Get the detector readout time. */

		mx_status = mx_area_detector_get_detector_readout_time(
							ad->record, NULL );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Get the old detector readout mode, since that will be
		 * used by the following code.
		 */

		mx_status = mxd_pccd_170170_read_register( pccd_170170,
					MXLV_PCCD_170170_DH_CONTROL,
					&old_control_register_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		old_detector_readout_mode = 
				(old_control_register_value >> 5) & 0x3;
#if 1
		MX_DEBUG(-2,("%s: old_control_register_value = %#lx",
			fname, old_control_register_value));
		MX_DEBUG(-2,("%s: old_detector_readout_mode = %#lx",
			fname, old_detector_readout_mode));
#endif
		/* Reprogram the detector head for the requested sequence. */

		sp = &(ad->sequence_parameters);

		switch( sp->sequence_type ) {
		case MXT_SQ_GEOMETRICAL:
			exposure_multiplier = sp->parameter_array[3];

			exposure_multiplier_steps = 
			   mx_round_down( (exposure_multiplier - 1.0) * 256.0 );

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_EXPOSURE_MULTIPLIER,
					exposure_multiplier_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			gap_multiplier = sp->parameter_array[4];

			gap_multiplier_steps = 
				mx_round_down( (gap_multiplier - 1.0) * 256.0 );

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_GAP_MULTIPLIER,
					gap_multiplier_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* No break since we fall through to the next case. */
		case MXT_SQ_ONE_SHOT:
		case MXT_SQ_MULTIFRAME:
		case MXT_SQ_STROBE:
			if ( old_detector_readout_mode != 0 ) {
				/* We must switch to full frame mode. */

				new_control_register_value =
					old_control_register_value & (~0x60);
#if 1
				MX_DEBUG(-2,
			("%s: (full frame) new_control_register_value = %#lx",
					fname, new_control_register_value));
#endif
				mx_status = mxd_pccd_170170_write_register(
						pccd_170170,
						MXLV_PCCD_170170_DH_CONTROL,
						new_control_register_value );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				/* If we were previously in streak camera mode,
				 * we must also restore the normal imaging
				 * board framesize.
				 */

				if ( old_detector_readout_mode == 0x3 ) {
				    mx_status = mx_video_input_set_framesize(
					pccd_170170->video_input_record,
					pccd_170170->vinput_normal_framesize[0],
				       pccd_170170->vinput_normal_framesize[1]);

				    if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
				}
			}

			/* Configure the detector head for the requested 
			 * number of frames, exposure time, and gap time.
			 */

			if ( sp->sequence_type == MXT_SQ_ONE_SHOT ) {
				num_frames = 1;
				exposure_time = sp->parameter_array[0];
				frame_time = exposure_time;

			} else if ( sp->sequence_type == MXT_SQ_MULTIFRAME ) {
				num_frames = mx_round( sp->parameter_array[0] );
				exposure_time = sp->parameter_array[1];
				frame_time = sp->parameter_array[2];

			} else if ( sp->sequence_type == MXT_SQ_STROBE ) {
				num_frames = mx_round( sp->parameter_array[0] );
				exposure_time = sp->parameter_array[1];

				frame_time = exposure_time
						+ ad->detector_readout_time;

			} else if ( sp->sequence_type == MXT_SQ_GEOMETRICAL ) {
				num_frames = mx_round( sp->parameter_array[0] );
				exposure_time = sp->parameter_array[1];
				frame_time = sp->parameter_array[2];

			} else {
				return mx_error( MXE_FUNCTION_FAILED, fname,
				"Inconsistent control structures for "
				"sequence type.  Sequence type = %lu",
					sp->sequence_type );
			}

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_FRAMES_PER_SEQUENCE,
					num_frames );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( flags & MXF_PCCD_170170_EXPOSURE_TIME_KLUDGE) {
			    exposure_steps =
					mx_round_down( exposure_time / 0.01 );
			} else {
			    exposure_steps =
					mx_round_down( exposure_time / 0.001 );
			}

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_EXPOSURE_TIME,
					exposure_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( num_frames > 1 ) {
				gap_time = frame_time - exposure_time
						- ad->detector_readout_time;

				if ( gap_time < 0.0 ) {
					return mx_error(
						MXE_ILLEGAL_ARGUMENT, fname,
				"The requested time per frame (%g seconds) "
				"is less than the sum of the requested "
				"exposure time (%g seconds) and the detector "
				"readout time (%g seconds) for detector '%s'.",
						frame_time, exposure_time,
						ad->detector_readout_time,
						ad->record->name );
				}

				gap_steps =
					mx_round_down( gap_time / 0.0001 );

				if ( gap_steps > 65535 ) {
					return mx_error(
						MXE_ILLEGAL_ARGUMENT, fname,
				"The computed gap time (%g seconds) is "
				"greater than the maximum allowed gap time "
				"(%g seconds) for detector '%s'.",
						gap_time,
						0.0001 * (double) 65535,
						ad->record->name );
				}

				mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_GAP_TIME,
					gap_steps );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
			break;
		case MXT_SQ_STREAK_CAMERA:
			/* Get the current framesize. */

			mx_status = mx_video_input_get_framesize(
					pccd_170170->video_input_record,
					&vinput_horiz_framesize,
					&vinput_vert_framesize );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( old_detector_readout_mode != 0x3 ) {
				/* Before switching to streak camera mode,
				 * save the video board's current framesize
				 * for later restoration.
				 */

				pccd_170170->vinput_normal_framesize[0]
					= vinput_horiz_framesize;

				pccd_170170->vinput_normal_framesize[1]
					= vinput_vert_framesize;

				/* Now switch to streak camera mode. */

				new_control_register_value =
					old_control_register_value | 0x60;
#if 1
				MX_DEBUG(-2,
		       ("%s: (streak camera) new_control_register_value = %#lx",
					fname, new_control_register_value));
#endif
				mx_status = mxd_pccd_170170_write_register(
						pccd_170170,
						MXLV_PCCD_170170_DH_CONTROL,
						new_control_register_value );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}

			/* Set the number of frames in sequence to 1. */

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_FRAMES_PER_SEQUENCE,
					1 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Set the number of streak camera mode lines.*/

			num_streak_mode_lines
				= mx_round_down( sp->parameter_array[0] );

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_STREAK_MODE_LINES,
					num_streak_mode_lines );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Set the exposure time per line. */

			exposure_time = sp->parameter_array[1];

			if ( flags & MXF_PCCD_170170_EXPOSURE_TIME_KLUDGE) {
			    exposure_steps =
					mx_round_down( exposure_time / 0.01 );
			} else {
			    exposure_steps =
					mx_round_down( exposure_time / 0.001 );
			}

			mx_status = mxd_pccd_170170_write_register(
				pccd_170170,
				MXLV_PCCD_170170_DH_EXPOSURE_TIME,
				exposure_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Set the streak-mode framesize. */

			mx_status = mx_video_input_set_framesize(
				pccd_170170->video_input_record,
				vinput_horiz_framesize,
				num_streak_mode_lines );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
			break;
		case MXT_SQ_SUBIMAGE:
			if ( old_detector_readout_mode != 0x2 ) {
				/* Switch to sub-image mode. */

				new_control_register_value =
					old_control_register_value & (~0x60);

				new_control_register_value |= 0x20;
#if 1
				MX_DEBUG(-2,
			("%s: (sub-image) new_control_register_value = %#lx",
					fname, new_control_register_value));
#endif
				mx_status = mxd_pccd_170170_write_register(
						pccd_170170,
						MXLV_PCCD_170170_DH_CONTROL,
						new_control_register_value );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				/* If we were previously in streak camera mode,
				 * we must also restore the normal imaging
				 * board framesize.
				 */

				if ( old_detector_readout_mode == 0x3 ) {
				    mx_status = mx_video_input_set_framesize(
					pccd_170170->video_input_record,
					pccd_170170->vinput_normal_framesize[0],
				       pccd_170170->vinput_normal_framesize[1]);

				    if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
				}
			}

			/* Set the number of frames in sequence to 1. */

			mx_status = mxd_pccd_170170_write_register( pccd_170170,
					MXLV_PCCD_170170_DH_FRAMES_PER_SEQUENCE,
					1 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Set the number of lines per subimage.
			 * The number of subimage lines is twice
			 * the value of the "subframe size".
			 */

			mx_status = mxd_pccd_170170_write_register( pccd_170170,
					MXLV_PCCD_170170_DH_SUBFRAME_SIZE,
				    mx_round_down(sp->parameter_array[0]/2.0) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Set the number of subimages per frame. */

			mx_status = mxd_pccd_170170_write_register( pccd_170170,
					MXLV_PCCD_170170_DH_SUBIMAGES_PER_READ,
					mx_round_down(sp->parameter_array[1]) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Set the exposure time. */

			exposure_time = sp->parameter_array[2];

			if ( flags & MXF_PCCD_170170_EXPOSURE_TIME_KLUDGE) {
			    exposure_steps =
					mx_round_down( exposure_time / 0.01 );
			} else {
			    exposure_steps =
					mx_round_down( exposure_time / 0.001 );
			}

			mx_status = mxd_pccd_170170_write_register(
				pccd_170170,
				MXLV_PCCD_170170_DH_EXPOSURE_TIME,
				exposure_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Compute the gap time from the subimage time. */

			subimage_time = sp->parameter_array[3];

			mx_status =
				mxd_pccd_170170_compute_detector_readout_time(
				    ad, pccd_170170, &detector_readout_time );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			gap_time = subimage_time - exposure_time
					- detector_readout_time;

			if ( gap_time < 0.0 ) {
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"For area detector '%s', The requested "
				"exposure time (%g sec) and the requested "
				"subimage time (%g sec) together with the "
				"calculated detector readout time (%g sec) "
				"result in a gap time (%g sec) that is "
				"negative.  This is not supported.",
					ad->record->name,
					exposure_time, subimage_time,
					detector_readout_time, gap_time );
			}

			/* Set the gap time. */

			if ( flags & MXF_PCCD_170170_EXPOSURE_TIME_KLUDGE) {
				gap_steps = mx_round_down( gap_time / 0.01 );
			} else {
				gap_steps = mx_round_down( gap_time / 0.001 );
			}

			mx_status = mxd_pccd_170170_write_register(
				pccd_170170,
				MXLV_PCCD_170170_DH_GAP_TIME,
				gap_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
					
			exposure_multiplier = sp->parameter_array[4];

			exposure_multiplier_steps = 
			   mx_round_down( (exposure_multiplier - 1.0) * 256.0 );

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_EXPOSURE_MULTIPLIER,
					exposure_multiplier_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			gap_multiplier = sp->parameter_array[5];

			gap_multiplier_steps = 
				mx_round_down( (gap_multiplier - 1.0) * 256.0 );

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_GAP_MULTIPLIER,
					gap_multiplier_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
			break;
		case MXT_SQ_CONTINUOUS:
		case MXT_SQ_CIRCULAR_MULTIFRAME:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Detector sequence type %lu is not supported "
			"for detector '%s'.",
				sp->sequence_type, ad->record->name );
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal sequence type %ld requested "
			"for detector '%s'.",
				sp->sequence_type, ad->record->name );
		}

		/* Reprogram the imaging board. */

		mx_status = mx_video_input_set_sequence_parameters(
					pccd_170170->video_input_record, sp );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the trigger mode for the imaging board. */

		switch( sp->sequence_type ) {
		case MXT_SQ_ONE_SHOT:
			mx_status = mx_video_input_set_trigger_mode( 
						pccd_170170->video_input_record,
						MXT_IMAGE_INTERNAL_TRIGGER );
			break;

		case MXT_SQ_STROBE:
			mx_status = mx_video_input_set_trigger_mode( 
						pccd_170170->video_input_record,
						MXT_IMAGE_EXTERNAL_TRIGGER );
			break;

		case MXT_SQ_MULTIFRAME:
		case MXT_SQ_GEOMETRICAL:
		case MXT_SQ_STREAK_CAMERA:
		case MXT_SQ_SUBIMAGE:
			/* For these cases, we leave the trigger mode alone,
			 * since we may want to use either internal triggering
			 * or external triggering.
			 */
			break;

		case MXT_SQ_CONTINUOUS:
		case MXT_SQ_CIRCULAR_MULTIFRAME:
		case MXT_SQ_BULB:
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

	case MXLV_AD_TRIGGER_MODE:
		mx_status = mx_video_input_set_trigger_mode(
				pccd_170170->video_input_record,
				ad->trigger_mode );
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Changing parameter '%s' for area detector '%s' is not supported.",
			mx_get_field_label_string( ad->record,
				ad->parameter_type ), ad->record->name );
		break;

	case MXLV_PCCD_170170_DH_CONTROL:
	case MXLV_PCCD_170170_DH_OVERSCANNED_PIXELS_PER_LINE:
	case MXLV_PCCD_170170_DH_PHYSICAL_LINES_IN_QUADRANT:
	case MXLV_PCCD_170170_DH_PHYSICAL_PIXELS_IN_QUADRANT:
	case MXLV_PCCD_170170_DH_LINES_READ_IN_QUADRANT:
	case MXLV_PCCD_170170_DH_PIXELS_READ_IN_QUADRANT:
	case MXLV_PCCD_170170_DH_SHUTTER_DELAY_TIME:
	case MXLV_PCCD_170170_DH_EXPOSURE_TIME:
	case MXLV_PCCD_170170_DH_READOUT_DELAY_TIME:
	case MXLV_PCCD_170170_DH_FRAMES_PER_SEQUENCE:
	case MXLV_PCCD_170170_DH_GAP_TIME:
	case MXLV_PCCD_170170_DH_EXPOSURE_MULTIPLIER:
	case MXLV_PCCD_170170_DH_GAP_MULTIPLIER:
	case MXLV_PCCD_170170_DH_CONTROLLER_FPGA_VERSION:
	case MXLV_PCCD_170170_DH_LINE_BINNING:
	case MXLV_PCCD_170170_DH_PIXEL_BINNING:
	case MXLV_PCCD_170170_DH_SUBFRAME_SIZE:
	case MXLV_PCCD_170170_DH_SUBIMAGES_PER_READ:
	case MXLV_PCCD_170170_DH_STREAK_MODE_LINES:
	case MXLV_PCCD_170170_DH_COMM_FPGA_VERSION:
	case MXLV_PCCD_170170_DH_OFFSET_A1:
	case MXLV_PCCD_170170_DH_OFFSET_A2:
	case MXLV_PCCD_170170_DH_OFFSET_A3:
	case MXLV_PCCD_170170_DH_OFFSET_A4:
	case MXLV_PCCD_170170_DH_OFFSET_B1:
	case MXLV_PCCD_170170_DH_OFFSET_B2:
	case MXLV_PCCD_170170_DH_OFFSET_B3:
	case MXLV_PCCD_170170_DH_OFFSET_B4:
	case MXLV_PCCD_170170_DH_OFFSET_C1:
	case MXLV_PCCD_170170_DH_OFFSET_C2:
	case MXLV_PCCD_170170_DH_OFFSET_C3:
	case MXLV_PCCD_170170_DH_OFFSET_C4:
	case MXLV_PCCD_170170_DH_OFFSET_D1:
	case MXLV_PCCD_170170_DH_OFFSET_D2:
	case MXLV_PCCD_170170_DH_OFFSET_D3:
	case MXLV_PCCD_170170_DH_OFFSET_D4:
		mx_status = mx_get_field_by_label_value( ad->record,
							ad->parameter_type,
							&field );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		register_value_ptr = mx_get_field_value_pointer( field );

		if ( register_value_ptr == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The register value pointer for field '%s.%s' is NULL.",
				ad->record->name, field->name );
		}

		mx_status = mxd_pccd_170170_write_register( pccd_170170,
					ad->parameter_type,
					*register_value_ptr );
		break;

	default:
		mx_status = mx_area_detector_default_set_parameter_handler(ad);
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_camera_link_command( MX_PCCD_170170 *pccd_170170,
					char *command,
					char *response,
					size_t max_response_length,
					int debug_flag )
{
	static const char fname[] = "mxd_pccd_170170_camera_link_command()";

	MX_RECORD *camera_link_record;
	size_t command_length, response_length;
	size_t i, bytes_available, bytes_to_read, total_bytes_read, buffer_left;
	char *ptr;
	unsigned long use_dh_simulator;
	double timeout_in_seconds;
	MX_CLOCK_TICK timeout_in_clock_ticks;
	MX_CLOCK_TICK start_tick, timeout_tick, current_tick;
	int comparison;
	mx_status_type mx_status;

#if MXD_PCCD_170170_DEBUG_SERIAL
	debug_flag |= 1;
#endif

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PCCD_170170 pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer passed for record '%s' was NULL.",
			pccd_170170->record->name );
	}
	if ( response == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The response pointer passed for record '%s' was NULL.",
			pccd_170170->record->name );
	}
	if ( max_response_length < 1 ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The requested response buffer length %lu for record '%s' "
		"is too short to hold a minimum length response.",
			(unsigned long) max_response_length,
			pccd_170170->record->name );
	}

	camera_link_record = pccd_170170->camera_link_record;

	if ( camera_link_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The camera_link_record pointer for record '%s' is NULL.",
			pccd_170170->record->name );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sent '%s' to '%s'",
			fname, command, camera_link_record->name ));
	}

	use_dh_simulator = pccd_170170->pccd_170170_flags
				& MXF_PCCD_170170_USE_DETECTOR_HEAD_SIMULATOR;

	if ( use_dh_simulator ) {

		/* Talk to a simulated detector head. */

		mx_status = mxd_pccd_170170_simulated_cl_command( 
						pccd_170170, command,
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

		/* Leave room in the response buffer to null terminate
		 * the response.
		 */

		response_length = max_response_length - 1;

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
#if 0 && MXD_PCCD_170170_DEBUG_SERIAL
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

			/* Were there any carriage returns in the bytes
			 * that we just received?  If so, we have reached
			 * the end of the response and can now break out of
			 * this loop.
			 */

			for ( i = 0; i < bytes_to_read; i++ ) {
				if ( ptr[i] == MX_CR ) {
					ptr[i] = '\0';
					break;	/* Exit the for(i) loop. */
				}
			}

			if ( i < bytes_to_read ) {
				/* We saw a carriage return, so break out
				 * of the for(;;) loop as well.
				 */
				break;
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
mxd_pccd_170170_read_register( MX_PCCD_170170 *pccd_170170,
				unsigned long register_address,
				unsigned long *register_value )
{
	static const char fname[] = "mxd_pccd_170170_read_register()";

	char command[20], response[20];
	int num_items;
	mx_status_type mx_status;

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PCCD_170170 pointer passed was NULL." );
	}
	if ( register_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The register_value pointer passed for record '%s' was NULL.",
			pccd_170170->record->name );
	}

	if ( register_address >= MXLV_PCCD_170170_DH_BASE ) {
		register_address -= MXLV_PCCD_170170_DH_BASE;
	}

	snprintf( command, sizeof(command), "R%03lu", register_address );

	mx_status = mxd_pccd_170170_camera_link_command( pccd_170170,
					command, response, sizeof(response),
					MXD_PCCD_170170_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "S%lu", register_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Could not find the register value in the response '%s' "
		"by detector '%s' to the command '%s'.",
			response, pccd_170170->record->name, command );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_write_register( MX_PCCD_170170 *pccd_170170,
				unsigned long register_address,
				unsigned long register_value )
{
	static const char fname[] = "mxd_pccd_170170_write_register()";

	char command[20], response[20];
	unsigned long value_read, flags;
	mx_status_type mx_status;

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PCCD_170170 pointer passed was NULL." );
	}

	flags = pccd_170170->pccd_170170_flags;

	if ( (register_address == MXLV_PCCD_170170_DH_FRAMES_PER_SEQUENCE)
	  && (flags & MXF_PCCD_170170_NUM_FRAMES_KLUDGE) )
	{
		mx_info(
		"***********************************************************\n"
		"* FIXME! Using PCCD-170170 num frames kludge!             *\n"
		"* This must be removed from the software before shipping! *\n"
		"***********************************************************");

		register_value++;
	}

	if ( register_address >= MXLV_PCCD_170170_DH_BASE ) {
		register_address -= MXLV_PCCD_170170_DH_BASE;
	}

	mx_status = mxd_pccd_170170_check_value( pccd_170170,
					register_address, register_value, NULL);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"W%03lu%05lu", register_address, register_value );

	/* Write the new value. */

	mx_status = mxd_pccd_170170_camera_link_command( pccd_170170,
					command, response, sizeof(response),
					MXD_PCCD_170170_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the value back to verify that the value was set correctly. */

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
						register_address, &value_read );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that the value read back matches the value sent. */

	if ( value_read != register_value ) {
		return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
		"The attempt to set '%s' register %lu to %lu "
		"appears to have failed since the value read back "
		"from that register is now %lu.",
			pccd_170170->record->name, register_address,
			register_value, value_read );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_pccd_170170_special_processing_setup( MX_RECORD *record )
{
#if MXD_PCCD_170170_DEBUG
	static const char fname[] =
		"mxd_pccd_170170_special_processing_setup()";
#endif

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		if ( record_field->label_value > MXLV_PCCD_170170_DH_BASE ) {
			record_field->process_function
					= mxd_pccd_170170_process_function;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pccd_170170_process_function( void *record_ptr,
				void *record_field_ptr,
				int operation )
{
	static const char fname[] = "mxd_pccd_170170_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
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

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		if ( record_field->label_value > MXLV_PCCD_170170_DH_BASE ) {
			mx_status = mx_area_detector_get_long_parameter(
					record, record_field->name, NULL );
		}
		break;

	case MX_PROCESS_PUT:
		if ( record_field->label_value > MXLV_PCCD_170170_DH_BASE ) {
			mx_status = mx_area_detector_set_long_parameter(
					record, record_field->name, NULL );
		}
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unknown operation code = %d for record '%s'.",
			operation, record->name );
	}

	return mx_status;
}

