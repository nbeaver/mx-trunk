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

#define MXD_PCCD_170170_DEBUG			TRUE

#define MXD_PCCD_170170_DEBUG_DESCRAMBLING	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_image.h"
#include "mx_digital_input.h"
#include "mx_video_input.h"
#include "mx_camera_link.h"
#include "mx_area_detector.h"
#include "d_pccd_170170.h"

/*---*/

#if HAVE_CAMERA_LINK

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
};

MX_RECORD_FIELD_DEFAULTS mxd_pccd_170170_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
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
		mx_status = mxp_pccd_170170_init_register(            \
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

static mx_status_type
mxd_pccd_170170_free_sector_array( uint16_t ****sector_array_ptr )
{
	static const char fname[] = "mxd_pccd_170170_free_sector_array()";

	uint16_t ***sector_array;
	uint16_t **sector_array_row_ptr;

	if ( sector_array_ptr == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The sector_array_ptr argument passed was NULL." );
	}

	sector_array = *sector_array_ptr;

	if ( sector_array == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	sector_array_row_ptr = *sector_array;

	if ( sector_array_row_ptr != NULL ) {
		free( sector_array_row_ptr );
	}

	free( sector_array );

	*sector_array_ptr = NULL;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pccd_170170_alloc_sector_array( uint16_t ****sector_array_ptr,
					long sector_height,
					long sector_width,
					uint16_t *image_data )
{
	static const char fname[] = "mxd_pccd_170170_alloc_sector_array()";

	uint16_t ***sector_array;
	uint16_t **sector_array_row_ptr;
	long num_sector_rows, num_sector_columns, num_sectors;
	long n, sector_row, row, sector_column;
	long row_of_sectors_size, row_size, sector_row_size;
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

	sector_array_row_ptr = 
		malloc( num_sectors * sector_height * sizeof(uint16_t *) );

	if ( sector_array_row_ptr == (uint16_t **) NULL ) {
		free( sector_array );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of row pointers.", num_sectors * sector_height );
	}

	row_byte_offset = sector_height * sizeof(sector_array_row_ptr);

	row_ptr_offset = row_byte_offset / sizeof(uint16_t *);

#if 0
	MX_DEBUG(-2,
	("%s: row_byte_offset = %ld (%#lx), row_ptr_offset = %ld (%#lx)",
		fname, row_byte_offset, row_byte_offset,
		row_ptr_offset, row_ptr_offset));

	MX_DEBUG(-2,("%s: sector_array_row_pointer = %p",
			fname, sector_array_row_ptr ));
#endif

	for ( n = 0; n < num_sectors; n++ ) {

		sector_array[n] = sector_array_row_ptr + n * row_ptr_offset;

#if 0
		MX_DEBUG(-2,("%s: sector_array[%ld] = %p",
			fname, n, sector_array[n] ));
#endif
	}

	/* The 'row_size' is the number of bytes in a single horizontal
	 * line of the image.  The 'row_of_sectors_size' is the number of
	 * bytes in a horizontal row of sectors.  In unbinned mode,
	 * row_of_sectors_size = 1024 * row_size.
	 */

	/* sector_row_size     = number of bytes in a single horizontal
	 *                       row of a sector.
	 *
	 * row_size            = number of bytes in a single horizontal
	 *                       row of the full image.
	 *
	 * row_of_sectors_size = number of bytes in all the rows of a
	 *                       single row of sectors.
	 *
	 * For an unbinned image:
	 *
	 *   row_of_sectors_size = 1024 * row_size = 1024 * 4 * sector_row_size
	 */

	sector_row_size = sector_width * sizeof(uint16_t);

	row_size = num_sector_columns * sector_row_size;

	row_of_sectors_size = row_size * sector_height;

#if 0
	MX_DEBUG(-2,
	("%s: image_data = %p, row_size = %#lx, row_of_sectors_size = %#lx",
		fname, image_data, row_size, row_of_sectors_size));
#endif

	for ( sector_row = 0; sector_row < num_sector_rows; sector_row++ ) {
	    for ( row = 0; row < sector_width; row++ ) {
		for ( sector_column = 0; sector_column < num_sector_columns;
							sector_column++ )
		{
		    n = sector_column + 4 * sector_row;

		    byte_offset = sector_row * row_of_sectors_size
				+ row * row_size
				+ sector_column * sector_row_size;

		    ptr_offset = byte_offset / sizeof(uint16_t);

		    sector_array[n][row] = image_data + ptr_offset;

#if 0
		    MX_DEBUG(-2,
    ("byte_offset = %#lx, ptr_offset = %#lx, array[%ld][%ld] = %#lx",
			byte_offset, ptr_offset, n, row,
			(long) sector_array[n][row] ));
#endif
		}
	    }
	}

	*sector_array_ptr = sector_array;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pccd_170170_descramble_image( MX_PCCD_170170 *pccd_170170,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *raw_frame )
{
	static const char fname[] = "mxd_pccd_170170_descramble_image()";

	long bytes_to_copy, raw_length, image_length;
	uint16_t *raw_frame_data;
	uint16_t ***image_sector_array;
	long i, j, i_framesize, j_framesize;
	mx_status_type mx_status;

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}
	if ( raw_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The raw_frame pointer passed was NULL." );
	}

	image_length = image_frame->image_length;

	raw_length = raw_frame->image_length;

	if ( raw_length <= image_length ) {
		bytes_to_copy = raw_length;
	} else {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The raw frame of length %ld bytes is too long to fit "
		"into the image frame of length %ld bytes.",
			raw_length, image_length );
	}

	MX_DEBUG(-2,
	("%s: image_length = %ld, raw_length = %ld, bytes_to_copy = %ld",
		fname, image_length, raw_length, bytes_to_copy));

#if 0
	if ( (raw_frame->framesize[0] != image_frame->framesize[0])
	  || (raw_frame->framesize[1] != image_frame->framesize[1]) )
	{
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The raw frame (%ld,%ld) and the image frame (%ld,%ld) have "
		"different dimensions.  At present, I do not know what the "
		"correct mapping is for this case.",
			raw_frame->framesize[0], raw_frame->framesize[1],
			image_frame->framesize[0], image_frame->framesize[1] );
	}
#endif

	/* For each frame, we overlay the frame with 16 sets of sector array
	 * pointers so that we can treat each of the 16 sectors as independent
	 * two dimensional arrays.
	 */

	i_framesize = image_frame->framesize[0] / 4;
	j_framesize = image_frame->framesize[1] / 4;

	/* If the framesize has changed since the last time we descrambled
	 * a frame, we must create new sector arrays to point into the raw
	 * frame data and the image frame data.
	 *
	 * If the framesize is the same, we just leave the existing sector
	 * arrays as they are.
	 */

	if ( ( pccd_170170->old_framesize[0] != image_frame->framesize[0] )
	  || ( pccd_170170->old_framesize[1] != image_frame->framesize[1] ) )
	{
		mx_status = mxd_pccd_170170_free_sector_array(
					&(pccd_170170->image_sector_array) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_pccd_170170_alloc_sector_array(
					&(pccd_170170->image_sector_array),
					i_framesize, j_framesize,
					image_frame->image_data );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	raw_frame_data = raw_frame->image_data;

	MX_DEBUG(-2,("%s: raw_frame_data = %p, image_frame->image_data = %p",
		fname, raw_frame_data, image_frame->image_data));

	/* Copy and descramble the pixels from the raw frame
	 * to the image frame.
	 */

	image_sector_array = pccd_170170->image_sector_array;

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

#if MXD_PCCD_170170_DEBUG_DESCRAMBLING
	{
		long k;

		for ( k = 0; k < 16; k++ ) {
			MX_DEBUG(-2,("%s: upper_left_corner[%ld] = %d",
				fname, k, image_sector_array[k][0][0] ));
		}
	}
#endif

	MX_DEBUG(-2,("%s: Image descrambling complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxp_pccd_170170_check_value( MX_PCCD_170170 *pccd_170170,
				unsigned long register_address,
				unsigned long register_value,
				MX_PCCD_170170_REGISTER **register_pointer )
{
	static const char fname[] = "mxp_pccd_170170_check_value()";

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
mxp_pccd_170170_init_register( MX_PCCD_170170 *pccd_170170,
				long register_index,
				unsigned long register_value,
				mx_bool_type read_only,
				mx_bool_type power_of_two,
				unsigned long minimum,
				unsigned long maximum )
{
	static const char fname[] = "mxp_pccd_170170_init_register()";

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

		mx_status = mxp_pccd_170170_check_value( pccd_170170,
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
	size_t array_size;
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

	if ( flags & MXF_PCCD_170170_USE_CAMERA_SIMULATOR ) {
		mx_warning( "Area detector '%s' will use "
			"an Aviex camera simulator instead of a real camera.",
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

	/* The PCCD-170170 camera generates 16 bit per pixel images. */

	vinput->bits_per_pixel = 16;
	ad->bits_per_pixel = vinput->bits_per_pixel;

	/* Set the default file format. */

	ad->frame_file_format = MXT_IMAGE_FILE_SMV;

	ad->binsize[0] = 1;
	ad->binsize[1] = 1;

	ad->sequence_parameters.sequence_type = MXT_SQ_ONE_SHOT;
	ad->sequence_parameters.num_parameters = 1;
	ad->sequence_parameters.parameter_array[0] = 1.0;

	/* Get the video input's current framesize, which will be used
	 * to compute the area detector's maximum framesize.
	 */

	mx_status = mx_video_input_get_framesize( video_input_record,
					&vinput_framesize[0],
					&vinput_framesize[1] );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* At maximum resolution, the Aviex camera is configured such that
	 * each line contains 1024 groups of pixels, with 16 pixels per group.
	 * There are a total of 1024 lines.  This means that at maximum
	 * resolution, the video input is configured for a resolution
	 * of 16384 by 1024.  However, we want this to appear to the user
	 * to have a resolution of 4096 by 4096.  Thus we must rescale the
	 * resolution as reported by the video card by multiplying and
	 * dividing by appropriate factors of 4.
	 */

	ad->maximum_framesize[0] =
		vinput_framesize[0] / MXF_PCCD_170170_HORIZ_SCALE;

	ad->maximum_framesize[1] =
		vinput_framesize[1] * MXF_PCCD_170170_VERT_SCALE;

	/* Copy the maximum framesize to the current framesize. */

	ad->framesize[0] = ad->maximum_framesize[0];
	ad->framesize[1] = ad->maximum_framesize[1];

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

	/* Load the image correction files. */

	mx_status = mx_area_detector_load_correction_files( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize area detector parameters. */

	ad->pixel_order = MXT_IMAGE_PIXEL_ORDER_STANDARD;
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
	MX_DEBUG(-2,("%s: pixel_order = %ld", fname, ad->pixel_order ));
	MX_DEBUG(-2,("%s: bytes_per_pixel = %g", fname, ad->bytes_per_pixel));
	MX_DEBUG(-2,("%s: header_length = %ld", fname, ad->header_length));
	MX_DEBUG(-2,("%s: bytes_per_frame = %ld", fname, ad->bytes_per_frame));
#endif

	mx_status = mx_image_alloc( &(pccd_170170->raw_frame),
					MXT_IMAGE_LOCAL_1D_ARRAY,
					vinput_framesize,
					ad->image_format,
					ad->pixel_order,
					ad->bytes_per_pixel,
					ad->header_length,
					ad->bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pccd_170170->old_framesize[0] = -1;
	pccd_170170->old_framesize[1] = -1;

	pccd_170170->image_sector_array = NULL;

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

	INIT_REGISTER( MXLV_PCCD_170170_DH_PHYSICAL_PIXELS_PER_LINE,
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

	INIT_REGISTER( MXLV_PCCD_170170_DH_SUBIMAGE_LINES,
					1024,  FALSE, TRUE,  16, 1024 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_SUBIMAGES_PER_READ,
					2,     FALSE, FALSE, 2,  128 );

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

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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

	mx_status = mx_camera_link_pulse_cc_line(
					pccd_170170->camera_link_record, 4,
					-1, 1000 );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_arm()";

	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif
	mx_status = mx_video_input_arm( pccd_170170->video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_trigger()";

	MX_PCCD_170170 *pccd_170170;
	MX_SEQUENCE_PARAMETERS *sp;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sp = &(ad->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_CONTINUOUS:
	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_CIRCULAR_MULTIFRAME:
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Image sequence type %ld is not supported by "
			"area detector '%s'.",
			sp->sequence_type, ad->record->name );
	}

	mx_status = mx_video_input_trigger( pccd_170170->video_input_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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

	if ( mx_status.code != MXE_SUCCESS );
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

	if ( mx_status.code != MXE_SUCCESS );
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
	unsigned long status_flags, spare_status;
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

	MX_DEBUG(-137,
("%s: last_frame_number = %ld, total_num_frames = %ld, status_flags = %#lx",
		fname, last_frame_number, total_num_frames, status_flags));

	ad->last_frame_number = last_frame_number;

	ad->total_num_frames = total_num_frames;

	ad->status = 0;

	if ( status_flags & MXSF_VIN_IS_BUSY ) {
		ad->status |= MXSF_AD_IS_BUSY;
	}

	/* See if the Camera Link Spare line says that
	 * the detector head is idle.
	 */

	mx_status = mx_digital_input_read( pccd_170170->spare_line_record,
						&spare_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( spare_status != 0 ) {
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

#if 0
	mx_status = mx_video_input_get_frame(
		pccd_170170->video_input_record,
		ad->readout_frame, &(ad->image_frame) );
#else
	/* Read in the raw image frame. */

	mx_status = mx_video_input_get_frame(
		pccd_170170->video_input_record,
		ad->readout_frame, &(pccd_170170->raw_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If required, we now descramble the image. */

	flags = pccd_170170->pccd_170170_flags;

	if ( flags & MXF_PCCD_170170_USE_CAMERA_SIMULATOR ) {
		/* If we are using the camera simulator board, the data
		 * stream coming in is not scrambled, so we just copy
		 * directly from the raw frame to the image frame.
		 */

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

		mx_status = mxd_pccd_170170_descramble_image( pccd_170170,
							ad->image_frame,
							pccd_170170->raw_frame);
	}
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_get_parameter()";

	MX_PCCD_170170 *pccd_170170;
	MX_RECORD *video_input_record;
	long vinput_horiz_framesize, vinput_vert_framesize;
	MX_SEQUENCE_PARAMETERS seq;
	long i, num_frames;
	double exposure_time, frame_time, gap_time;
	double exposure_multiplier, gap_multiplier;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif
	video_input_record = pccd_170170->video_input_record;

	switch( ad->parameter_type ) {
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
		/* FIXME, FIXME, FIXME!!!
		 * 
		 * This is just a placeholder for the real value!
		 */

		ad->detector_readout_time = 0.01;
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
	long vinput_horiz_framesize, vinput_vert_framesize;
	long sequence_type, num_frames, exposure_steps, gap_steps;
	long exposure_multiplier_steps, gap_multiplier_steps;
	double exposure_time, frame_time, gap_time;
	double exposure_multiplier, gap_multiplier;
	mx_status_type mx_status;

	static long allowed_binsize[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

	static int num_allowed_binsizes = sizeof( allowed_binsize )
						/ sizeof( allowed_binsize[0] );

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:
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
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 

		/* Reprogram the detector head. */

		sequence_type = ad->sequence_parameters.sequence_type;

		switch( sequence_type ) {
		case MXT_SQ_GEOMETRICAL:
			exposure_multiplier =
				ad->sequence_parameters.parameter_array[3];

			exposure_multiplier_steps = mx_round(
						exposure_multiplier / 0.078 );

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_EXPOSURE_MULTIPLIER,
					exposure_multiplier_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			gap_multiplier =
				ad->sequence_parameters.parameter_array[3];

			gap_multiplier_steps = mx_round(gap_multiplier / 0.078);

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_GAP_MULTIPLIER,
					gap_multiplier_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Fall through to the next case. */
		case MXT_SQ_ONE_SHOT:
		case MXT_SQ_MULTIFRAME:
			if ( sequence_type == MXT_SQ_ONE_SHOT ) {
				num_frames = 1;
				exposure_time =
				    ad->sequence_parameters.parameter_array[0];
			} else {
				num_frames = mx_round(
				    ad->sequence_parameters.parameter_array[0]);
				exposure_time =
				    ad->sequence_parameters.parameter_array[1];
				frame_time =
				    ad->sequence_parameters.parameter_array[2];
			}

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_FRAMES_PER_SEQUENCE,
					num_frames );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			exposure_steps = mx_round( exposure_time / 0.01 );

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_EXPOSURE_TIME,
					exposure_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( num_frames > 1 ) {
				gap_time = frame_time - exposure_time;

				if ( gap_time < 0.0 ) {
					return mx_error(
						MXE_ILLEGAL_ARGUMENT, fname,
				"The requested time per frame of %g seconds "
				"is less than the requested exposure time "
				"of %g seconds for detector '%s'.",
						frame_time, exposure_time,
						ad->record->name );
				}

				gap_steps = mx_round( gap_time / 0.0001 );

				if ( gap_steps > 65535 ) {
					return mx_error(
						MXE_ILLEGAL_ARGUMENT, fname,
				"The difference between the requested frame "
				"time of %g seconds and the requested exposure "
				"time of %g seconds is greater than the "
				"maximum difference of %g seconds for "
				"detector '%s'.", frame_time, exposure_time,
						gap_time, ad->record->name );
				}

				mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_GAP_TIME,
					gap_steps );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
			break;
		case MXT_SQ_CONTINUOUS:
		case MXT_SQ_CIRCULAR_MULTIFRAME:
		case MXT_SQ_STROBE:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Detector sequence type %lu is not supported "
			"for detector '%s'.",
				sequence_type, ad->record->name );
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal sequence type %ld requested "
			"for detector '%s'.",
				sequence_type, ad->record->name );
		}

		/* Reprogram the imaging board. */

		mx_status = mx_video_input_set_sequence_parameters(
					pccd_170170->video_input_record,
					&(ad->sequence_parameters) );
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
	unsigned long use_dh_simulator;
	mx_status_type mx_status;

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

		command_length = strlen(command);

		mx_status = mx_camera_link_serial_write( camera_link_record,
						command, &command_length, -1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Leave room in the response buffer to null terminate
		 * the response.
		 */

		response_length = max_response_length - 1;

		mx_status = mx_camera_link_serial_read( camera_link_record,
						response, &response_length, -1);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		response[response_length] = '\0';
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
	unsigned long value_read;
	mx_status_type mx_status;

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PCCD_170170 pointer passed was NULL." );
	}

	if ( register_address >= MXLV_PCCD_170170_DH_BASE ) {
		register_address -= MXLV_PCCD_170170_DH_BASE;
	}

	mx_status = mxp_pccd_170170_check_value( pccd_170170,
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
	static const char fname[] =
		"mxd_pccd_170170_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));

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

#endif /* HAVE_CAMERA_LINK */

