/*
 * Name:    d_file_vinput.c
 *
 * Purpose: MX driver for an emulated video input device that reads
 *          image frames from a directory of existing image files.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2007-2010, 2013, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_FILE_VINPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_dirent.h"
#include "mx_array.h"
#include "mx_bit.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "d_file_vinput.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_file_vinput_record_function_list = {
	NULL,
	mxd_file_vinput_create_record_structures,
	mxd_file_vinput_finish_record_initialization,
	NULL,
	NULL,
	mxd_file_vinput_open,
	mxd_file_vinput_close
};

MX_VIDEO_INPUT_FUNCTION_LIST mxd_file_vinput_video_input_function_list = {
	mxd_file_vinput_arm,
	mxd_file_vinput_trigger,
	mxd_file_vinput_stop,
	mxd_file_vinput_abort,
	mxd_file_vinput_asynchronous_capture,
	NULL,
	NULL,
	NULL,
	mxd_file_vinput_get_extended_status,
	mxd_file_vinput_get_frame,
	mxd_file_vinput_get_parameter,
	mxd_file_vinput_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_file_vinput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VIDEO_INPUT_STANDARD_FIELDS,
	MXD_FILE_VINPUT_STANDARD_FIELDS
};

long mxd_file_vinput_num_record_fields
		= sizeof( mxd_file_vinput_record_field_defaults )
			/ sizeof( mxd_file_vinput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_file_vinput_rfield_def_ptr
			= &mxd_file_vinput_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_file_vinput_get_pointers( MX_VIDEO_INPUT *vinput,
			MX_FILE_VINPUT **file_vinput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_file_vinput_get_pointers()";

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (file_vinput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_FILE_VINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*file_vinput = (MX_FILE_VINPUT *)
				vinput->record->record_type_struct;

	if ( *file_vinput == (MX_FILE_VINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_FILE_VINPUT pointer for record '%s' passed by '%s' is NULL.",
			vinput->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_file_vinput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_file_vinput_create_record_structures()";

	MX_VIDEO_INPUT *vinput;
	MX_FILE_VINPUT *file_vinput = NULL;

	vinput = (MX_VIDEO_INPUT *) malloc( sizeof(MX_VIDEO_INPUT) );

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VIDEO_INPUT structure." );
	}

	file_vinput = (MX_FILE_VINPUT *)
				malloc( sizeof(MX_FILE_VINPUT) );

	if ( file_vinput == (MX_FILE_VINPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_FILE_VINPUT structure." );
	}

	record->record_class_struct = vinput;
	record->record_type_struct = file_vinput;
	record->class_specific_function_list = 
			&mxd_file_vinput_video_input_function_list;

	memset( &(vinput->sequence_parameters),
			0, sizeof(vinput->sequence_parameters) );

	vinput->record = record;
	file_vinput->record = record;

	vinput->trigger_mode = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_file_vinput_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_file_vinput_finish_record_initialization()";

	MX_VIDEO_INPUT *vinput;
	MX_FILE_VINPUT *file_vinput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_file_vinput_get_pointers( vinput, &file_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_FILE_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	mx_status = mx_video_input_finish_record_initialization( record );

	return mx_status;
}

#define MXD_FILE_VINPUT_BLOCK_SIZE	100

static int
mxp_strcmp( const void *p1, const void *p2 )
{
	union { const void *p1;
		char **c1;
	} u1;

	union { const void *p2;
		char **c2;
	} u2;

	char *ptr1, *ptr2;
	int result;

	/* Wild and crazy stuff to 'unconst' the const pointers. */

	u1.p1 = p1;
	u2.p2 = p2;

	ptr1 = *(u1.c1);
	ptr2 = *(u2.c2);

	/* Now we can do the actual comparison. */

	result = strcmp( ptr1, ptr2 );

#if 0 && MXD_FILE_VINPUT_DEBUG
	MX_DEBUG(-2,("mxp_strcmp(): ptr1 = '%s', ptr2 = '%s', result = %d",
			ptr1, ptr2, result));
#endif

	return result;
}

MX_EXPORT mx_status_type
mxd_file_vinput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_file_vinput_open()";

	MX_VIDEO_INPUT *vinput;
	MX_FILE_VINPUT *file_vinput = NULL;
	long i, pixels_per_frame;
	DIR *dir;
	struct dirent *dirent_ptr;
	long dimension_array[2];
	size_t sizeof_array[2];
	char *name_ptr;
	int saved_errno;
	mx_status_type mx_status;

	file_vinput = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_file_vinput_get_pointers( vinput, &file_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_FILE_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	/* Initialize a bunch of driver parameters. */

	vinput->parameter_type = -1;
	vinput->frame_number   = -100;
	vinput->get_frame      = -100;
	vinput->frame          = NULL;
	vinput->frame_buffer   = NULL;
	vinput->byte_order     = (long) mx_native_byteorder();
	vinput->trigger_mode   = MXT_IMAGE_NO_TRIGGER;

	vinput->total_num_frames = 0;

	file_vinput->old_total_num_frames = 0;

	switch( vinput->image_format ) {
	case MXT_IMAGE_FORMAT_RGB:
		vinput->bytes_per_pixel = 3;
		vinput->bits_per_pixel  = 24;
		break;
	case MXT_IMAGE_FORMAT_GREY8:
		vinput->bytes_per_pixel = 1;
		vinput->bits_per_pixel  = 8;
		break;
	case MXT_IMAGE_FORMAT_GREY16:
		vinput->bytes_per_pixel = 2;
		vinput->bits_per_pixel  = 16;
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image format %ld '%s' is not a supported image format "
		"for emulated video input '%s'.",
			vinput->image_format, vinput->image_format_name,
			record->name );
	}

	pixels_per_frame = vinput->framesize[0] * vinput->framesize[1];

	vinput->bytes_per_frame = pixels_per_frame
					* mx_round( vinput->bytes_per_pixel );

#if MXD_FILE_VINPUT_DEBUG
	MX_DEBUG(-2,
	("%s: vinput->framesize[0] = %ld, vinput->framesize[1] = %ld",
		fname, vinput->framesize[0], vinput->framesize[1] ));

	MX_DEBUG(-2,("%s: vinput->image_format_name = '%s'",
		fname, vinput->image_format_name));

	MX_DEBUG(-2,("%s: vinput->image_format = %ld",
		fname, vinput->image_format));

	MX_DEBUG(-2,("%s: vinput->byte_order = %ld",
		fname, vinput->byte_order));

	MX_DEBUG(-2,("%s: vinput->trigger_mode = %ld",
		fname, vinput->trigger_mode));

	MX_DEBUG(-2,("%s: vinput->bits_per_pixel = %ld",
		fname, vinput->bits_per_pixel));

	MX_DEBUG(-2,("%s: vinput->bytes_per_pixel = %g",
		fname, vinput->bytes_per_pixel));

	MX_DEBUG(-2,("%s: vinput->bytes_per_frame = %ld",
		fname, vinput->bytes_per_frame));
#endif

	if ( mx_strcasecmp( "pnm", file_vinput->file_format_name ) == 0 ) {
		file_vinput->file_format = MXT_IMAGE_FILE_PNM;
	} else
	if ( mx_strcasecmp( "marccd", file_vinput->file_format_name ) == 0 ) {
		file_vinput->file_format = MXT_IMAGE_FILE_MARCCD;
	} else
	if ( mx_strcasecmp( "smv", file_vinput->file_format_name ) == 0 ) {
		file_vinput->file_format = MXT_IMAGE_FILE_SMV;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The image file format '%s' requested for video input '%s' is "
		"not supported.", file_vinput->file_format_name, record->name );
	}

	/* Find out how many files are in the specified directory. */

	file_vinput->num_files = 0;
	file_vinput->filename_array = NULL;
	file_vinput->current_filenum = 0;

	dir = opendir( file_vinput->directory_name );

	if ( dir == (DIR *) NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot access the directory '%s'.  "
		"Errno = %d, error message = '%s'.",
			file_vinput->directory_name,
			saved_errno, strerror(saved_errno) );
	}

	while(1) {
		errno = 0;

		dirent_ptr = readdir(dir);

		if ( dirent_ptr != NULL ) {
			name_ptr = dirent_ptr->d_name;

			if ( name_ptr[0] == '.' ) {
				/* Skip files whose names start with '.' */
			} else {
				file_vinput->num_files++;
			}
		} else {
			if ( errno == 0 ) {
				break;		/* Exit the while() loop. */
			} else {
				saved_errno = errno;

				return mx_error( MXE_FILE_IO_ERROR, fname,
				"An error occurred while counting the number "
				"of files in the directory '%s'.",
					file_vinput->directory_name );
			}
		}
	}

	rewinddir(dir);

#if MXD_FILE_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: Total number of files in '%s' = %ld",
		fname, file_vinput->directory_name, file_vinput->num_files));
#endif

	if ( file_vinput->num_files <= 0 ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"There are no files in the directory '%s' "
		"specified by video input record '%s'.",
			file_vinput->directory_name, record->name );
	}

	/* Read in the names of all of the files in the specified directory. */

#if 1
	dimension_array[0] = file_vinput->num_files;
	dimension_array[1] = MXU_FILENAME_LENGTH+1;

	sizeof_array[0] = sizeof(char);
	sizeof_array[1] = sizeof(char *);

	file_vinput->filename_array = mx_allocate_array( 2,
						dimension_array, sizeof_array );

	if ( file_vinput->filename_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Unable to allocate a %ld element filename array "
			"for video input '%s'.",
				file_vinput->num_files, record->name );
	}
#else
	{
		char *ptr, *row_ptr;

		dimension_array[0] = 0;
		sizeof_array[0] = 0;

		ptr = malloc( file_vinput->num_files
				* (MXU_FILENAME_LENGTH+1) * sizeof(char) );

		if ( ptr == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Cannot allocate memory for a %ld element array "
			"of filenames for video input '%s'.",
				file_vinput->num_files, record->name );
		}

		file_vinput->filename_array =
			malloc( file_vinput->num_files * sizeof(char *) );

		if ( ptr == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Cannot allocate memory for a %ld element array "
			"of filename pointers for video input '%s'.",
				file_vinput->num_files, record->name );
		}

		for ( i = 0; i < file_vinput->num_files; i++ ) {
			row_ptr = ptr + i * (MXU_FILENAME_LENGTH+1);

			file_vinput->filename_array[i] = row_ptr;
		}
	}
#endif

	dir = opendir( file_vinput->directory_name );

	if ( dir == (DIR *) NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot access the directory '%s' "
		"even though we did access it just a moment ago.  "
		"Errno = %d, error message = '%s'.",
			file_vinput->directory_name,
			saved_errno, strerror(saved_errno) );
	}

	for ( i = 0; i < file_vinput->num_files; ) {

		errno = 0;

		dirent_ptr = readdir(dir);

		if ( dirent_ptr == NULL ) {
			if ( errno == 0 ) {
				return mx_error(
				MXE_UNEXPECTED_END_OF_DATA, fname,
				"Directory '%s' used by video input '%s' "
				"now has fewer files (%ld) than it did "
				"just a moment ago (%ld).",
					file_vinput->directory_name,
					vinput->record->name,
					i, file_vinput->num_files );
			} else {
				saved_errno = errno;

				return mx_error( MXE_FILE_IO_ERROR, fname,
				"An error occurred while getting the name "
				"of file %ld in the directory '%s'.",
					i, file_vinput->directory_name );
			}
		}

		name_ptr = dirent_ptr->d_name;

		if ( name_ptr[0] == '.' ) {
			/* Skip files whose names start with '.' */
		} else {
			strlcpy( file_vinput->filename_array[i],
				name_ptr, MXU_FILENAME_LENGTH );

			i++;	/* Move to the next array element. */
		}

#if 0 && MXD_FILE_VINPUT_DEBUG
		MX_DEBUG(-2,
		("%s: Unsorted filename_array[%ld] = '%s'",
			fname, i, file_vinput->filename_array[i]));
#endif
	}

	/* Sort the filenames into alphanumeric order. */

	qsort( file_vinput->filename_array, file_vinput->num_files,
			sizeof(char *), mxp_strcmp );

#if 0 && MXD_FILE_VINPUT_DEBUG
	/* Print out the sorted list of filenames. */

	for ( i = 0; i < file_vinput->num_files; i++ ) {
		MX_DEBUG(-2,("%s: Sorted filename_array[%ld] = '%s'",
			fname, i, file_vinput->filename_array[i]));
	}
#endif

#if MXD_FILE_VINPUT_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_file_vinput_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_file_vinput_close()";

	MX_VIDEO_INPUT *vinput;
	MX_FILE_VINPUT *file_vinput = NULL;
	long dimension_array[2];
	size_t sizeof_array[2];
	mx_status_type mx_status;

	file_vinput = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_file_vinput_get_pointers( vinput, &file_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_FILE_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif
	dimension_array[0] = file_vinput->num_files;
	dimension_array[1] = MXU_FILENAME_LENGTH;

	sizeof_array[0] = sizeof(char);
	sizeof_array[1] = sizeof(char *);

	mx_status = mx_free_array( file_vinput->filename_array );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_file_vinput_arm( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_file_vinput_arm()";

	MX_FILE_VINPUT *file_vinput = NULL;
	MX_SEQUENCE_PARAMETERS *seq;
	mx_status_type mx_status;

	mx_status = mxd_file_vinput_get_pointers( vinput, &file_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_FILE_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif
	file_vinput->old_total_num_frames = vinput->total_num_frames;

	seq = &(vinput->sequence_parameters);

	switch( seq->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_CONTINUOUS:
		file_vinput->seconds_per_frame = seq->parameter_array[0];
		file_vinput->num_frames_in_sequence = 1;
		break;
	case MXT_SQ_MULTIFRAME:
		file_vinput->seconds_per_frame = seq->parameter_array[2];
		file_vinput->num_frames_in_sequence =
					mx_round( seq->parameter_array[0] );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported sequence type %lu requested for video input '%s'.",
			seq->sequence_type, vinput->record->name );
		break;
	}

	switch( vinput->trigger_mode ) {
	case MXT_IMAGE_INTERNAL_TRIGGER:
		file_vinput->sequence_in_progress = FALSE;
		break;
	case MXT_IMAGE_EXTERNAL_TRIGGER:
		file_vinput->sequence_in_progress = TRUE;
		file_vinput->start_tick = mx_current_clock_tick();
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_file_vinput_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_file_vinput_trigger()";

	MX_FILE_VINPUT *file_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_file_vinput_get_pointers( vinput, &file_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_FILE_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif
	file_vinput->sequence_in_progress = TRUE;
	file_vinput->start_tick = mx_current_clock_tick();

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_file_vinput_stop( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_file_vinput_stop()";

	MX_FILE_VINPUT *file_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_file_vinput_get_pointers( vinput, &file_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_FILE_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif
	file_vinput->sequence_in_progress = FALSE;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_file_vinput_abort( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_file_vinput_abort()";

	MX_FILE_VINPUT *file_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_file_vinput_get_pointers( vinput, &file_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_FILE_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif
	file_vinput->sequence_in_progress = FALSE;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_file_vinput_asynchronous_capture( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_file_vinput_asynchronous_capture()";

	MX_FILE_VINPUT *file_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_file_vinput_get_pointers( vinput, &file_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_FILE_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif
	file_vinput->sequence_in_progress = TRUE;
	file_vinput->start_tick = mx_current_clock_tick();

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_file_vinput_get_extended_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_file_vinput_get_extended_status()";

	MX_FILE_VINPUT *file_vinput = NULL;
	MX_CLOCK_TICK clock_ticks_since_start;
	double seconds_since_start, frames_since_start_dbl;
	long frames_since_start;
	mx_status_type mx_status;

	mx_status = mxd_file_vinput_get_pointers( vinput,
						&file_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( file_vinput->sequence_in_progress == FALSE ) {
		vinput->status = 0;
	} else {
		clock_ticks_since_start = mx_subtract_clock_ticks(
			mx_current_clock_tick(), file_vinput->start_tick );

		seconds_since_start = mx_convert_clock_ticks_to_seconds(
						clock_ticks_since_start );

		frames_since_start_dbl = mx_divide_safely( seconds_since_start,
						file_vinput->seconds_per_frame);

		frames_since_start = mx_round( frames_since_start_dbl );

		switch( vinput->sequence_parameters.sequence_type ) {
		case MXT_SQ_ONE_SHOT:
			if ( frames_since_start < 1 ) {
				vinput->status = MXSF_VIN_IS_BUSY;
				vinput->last_frame_number = -1;
				vinput->total_num_frames =
					file_vinput->old_total_num_frames;
			} else {
				vinput->status = 0;
				vinput->last_frame_number = 0;
				vinput->total_num_frames =
					file_vinput->old_total_num_frames + 1;

				file_vinput->sequence_in_progress = FALSE;
			}
			break;

		case MXT_SQ_CONTINUOUS:
			vinput->status = MXSF_VIN_IS_BUSY;
			vinput->last_frame_number = 0;
			vinput->total_num_frames =
		    file_vinput->old_total_num_frames + frames_since_start;

		    	break;

		case MXT_SQ_MULTIFRAME:
			if ( frames_since_start <
				file_vinput->num_frames_in_sequence )
			{
				vinput->status = MXSF_VIN_IS_BUSY;
			} else {
				vinput->status = 0;
				file_vinput->sequence_in_progress = FALSE;
			}

			vinput->last_frame_number = frames_since_start - 1;

			vinput->total_num_frames =
					file_vinput->old_total_num_frames
						+ frames_since_start;
			break;
		}
	}

	if ( vinput->status & MXSF_VIN_IS_BUSY ) {
		vinput->busy = TRUE;
	} else {
		vinput->busy = FALSE;
	}

#if MXD_FILE_VINPUT_DEBUG
	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, total_num_frames = %ld, status = %#lx",
		fname, vinput->last_frame_number, vinput->total_num_frames,
		vinput->status));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_file_vinput_get_frame( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_file_vinput_get_frame()";

	MX_FILE_VINPUT *file_vinput = NULL;
	MX_IMAGE_FRAME *frame;
	long i;
	char datafile_name[MXU_FILENAME_LENGTH+1];
	mx_status_type mx_status;

	char path_format[] = "%s/%s";

	file_vinput = NULL;

	mx_status = mxd_file_vinput_get_pointers( vinput,
						&file_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	frame = vinput->frame;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

#if MXD_FILE_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	if ( file_vinput->num_files <= 0 ) {
		return mx_error( MXE_NOT_AVAILABLE, fname,
		"There are no files in the directory '%s' specified "
		"by video input record '%s'.",
			file_vinput->directory_name,
			vinput->record->name );
	}

	if ( file_vinput->current_filenum >= file_vinput->num_files ) {
#if 1
		file_vinput->current_filenum = 0;
#else
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Video input record '%s' read image frame %ld "
		"from directory '%s' which has only %ld files in it.",
			vinput->record->name,
			file_vinput->current_filenum,
			file_vinput->directory_name,
			file_vinput->num_files );
#endif
	}

	i = file_vinput->current_filenum;

	snprintf( datafile_name, MXU_FILENAME_LENGTH, path_format, 
			file_vinput->directory_name,
			file_vinput->filename_array[i] );

#if MXD_FILE_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: i = %ld, datafile_name = '%s'",
		fname, i, datafile_name));
#endif

	mx_status = mx_image_read_file( &(vinput->frame),
					file_vinput->file_format,
					datafile_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	file_vinput->current_filenum++;

#if MXD_FILE_VINPUT_DEBUG
	MX_DEBUG(-2,
	("%s: successfully read a %lu byte image frame from video input '%s'.",
		fname, (unsigned long) frame->image_length,
		vinput->record->name ));
#endif

#if 0 && MXD_FILE_VINPUT_DEBUG
	{
		int n;
		unsigned char c;
		unsigned char *frame_buffer;

		frame_buffer = frame->image_data;

		MX_DEBUG(-2,("%s: frame = %p, frame_buffer = %p",
			fname, frame, frame_buffer));

		for ( n = 0; n < 10; n++ ) {
			c = frame_buffer[n];

			MX_DEBUG(-2,("%s: frame_buffer[%d] = %u", fname, n, c));
		}
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_file_vinput_get_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_file_vinput_get_parameter()";

	MX_FILE_VINPUT *file_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_file_vinput_get_pointers( vinput,
						&file_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_FILE_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:

		mx_status = mx_image_get_image_format_name_from_type(
				vinput->image_format, vinput->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
#if MXD_FILE_VINPUT_DEBUG
		MX_DEBUG(-2,("%s: video format = %ld, format name = '%s'",
		    fname, vinput->image_format, vinput->image_format_name));
#endif
		break;

	case MXLV_VIN_BYTE_ORDER:
		break;

	case MXLV_VIN_TRIGGER_MODE:
		break;

	case MXLV_VIN_BYTES_PER_FRAME:
		break;

	case MXLV_VIN_BYTES_PER_PIXEL:
		break;

	case MXLV_VIN_BUSY:
		break;

	case MXLV_VIN_STATUS:
		break;

	case MXLV_VIN_SEQUENCE_TYPE:

		break;

	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:
		break;
	default:
		mx_status =
			mx_video_input_default_get_parameter_handler( vinput );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_file_vinput_set_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_file_vinput_set_parameter()";

	MX_FILE_VINPUT *file_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_file_vinput_get_pointers( vinput,
						&file_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_FILE_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the image format is not supported for "
			"video input '%s'.", vinput->record->name );

	case MXLV_VIN_BYTE_ORDER:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the byte order for video input '%s' "
			"is not supported.", vinput->record->name );

	case MXLV_VIN_TRIGGER_MODE:
		break;

	case MXLV_VIN_BYTES_PER_FRAME:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Directly changing the number of bytes per frame "
			"for video input '%s' is not supported.",
				vinput->record->name );

	case MXLV_VIN_SEQUENCE_TYPE:
		break;

	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:
		break;

	default:
		mx_status =
			mx_video_input_default_set_parameter_handler( vinput );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

