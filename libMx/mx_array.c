/*
 * Name:    mx_array.c
 *
 * Purpose: MX array handling functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_ARRAY_DEBUG_ALLOCATE		FALSE

#define MX_ARRAY_DEBUG_OVERLAY		FALSE

#define MX_ARRAY_DEBUG_64BIT		FALSE

#define MX_ARRAY_DEBUG_64BIT_COPY	FALSE

/* FIXME - Soon the following will be the default. */

#define MX_ARRAY_USE_ARRAY_HEADER	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_osdef.h"
#include "mx_stdint.h"
#include "mx_inttypes.h"
#include "mx_socket.h"
#include "mx_array.h"

#if HAVE_XDR
#  include "mx_xdr.h"
#endif

/* Verify that MX_MAXIMUM_ALIGNMENT is defined on this platform. */

unsigned long __mx_maximum_alignment = MX_MAXIMUM_ALIGNMENT;

/*---------------------------------------------------------------------------*/

/* WARNING: Strictly speaking, mx_read_void_pointer_from_memory_location()
 * and mx_write_void_pointer_to_memory_location() are not portable.  But
 * they work on all supported MX platforms and they _probably_ will work
 * on any platform that uses a flat memory address space.
 */

/* The purpose of the following function is to interpret the value
 * stored at a memory location pointed to by a void * as if it were
 * itself an address.
 */

MX_EXPORT void *
mx_read_void_pointer_from_memory_location( void *ptr )
{
	/* As far as I know, the only assumption I am making here is
	 * that the internal representation of a void * is the same
	 * as the internal representation of a void **.  This is
	 * reasonably safe (I think).
	 */

	union {
		void *ptr_to_void;
		void **ptr_to_ptr_to_void;
	} u;

	void **result;

	u.ptr_to_void = ptr;

	result = *( u.ptr_to_ptr_to_void );

#if 0
	MX_DEBUG( 9, ("readvp: read pointer %p from memory location %p",
				result, ptr));
#endif

	return result;
}

/* Similarly, the following function writes an address value "ptr"
 * to a memory location pointed to by the void pointer "memory_location".
 */

MX_EXPORT void
mx_write_void_pointer_to_memory_location( void *memory_location, void *ptr )
{
	union {
		void *ptr_to_void;
		void **ptr_to_ptr_to_void;
	} u;

	u.ptr_to_void = memory_location;

	*( u.ptr_to_ptr_to_void ) = ptr;

#if 0
	MX_DEBUG( 9, ("writevp: wrote pointer %p to memory location %p",
					ptr, memory_location));
#endif

	return;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_compute_array_size( long num_dimensions,
		long *dimension_array,
		size_t *data_element_size_array,
		size_t *array_size )
{
	static const char fname[] = "mx_compute_array_size()";

	long i;
	
	if ( array_size == (size_t *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The array_size pointer passed was NULL." );
	}
	if ( data_element_size_array == (size_t *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The data_element_size_array pointer passed was NULL." );
	}

	if ( num_dimensions < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The num_dimensions argument specified (%ld) is illegal.  "
		"The minimum number of allowed dimensions is 0.",
			num_dimensions );
	}

	/* Handle scalars first, since scalars have 0 dimensions. */

	if ( num_dimensions == 0 ) {
		(*array_size) = data_element_size_array[0];

		return MX_SUCCESSFUL_RESULT;
	}

	/* Finally, handle arrays of 1 dimension or more. */

	if ( dimension_array == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The dimension_array pointer passed was NULL." );
	}

	(*array_size) = data_element_size_array[0];

	for ( i = 0; i < num_dimensions; i++ ) {
		(*array_size) *= dimension_array[i];
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT size_t
mx_get_scalar_element_size( long mx_datatype,
			mx_bool_type longs_are_64bits )
{
	size_t element_size;

	switch( mx_datatype ) {
	case MXFT_STRING:     element_size = sizeof(char);                break;
	case MXFT_CHAR:       element_size = sizeof(char);                break;
	case MXFT_UCHAR:      element_size = sizeof(unsigned char);       break;
	case MXFT_SHORT:      element_size = sizeof(short);               break;
	case MXFT_USHORT:     element_size = sizeof(unsigned short);      break;
	case MXFT_BOOL:       element_size = sizeof(mx_bool_type);        break;
	case MXFT_INT64:      element_size = sizeof(int64_t);             break;
	case MXFT_UINT64:     element_size = sizeof(uint64_t);            break;
	case MXFT_FLOAT:      element_size = sizeof(float);               break;
	case MXFT_DOUBLE:     element_size = sizeof(double);              break;

	case MXFT_HEX:        element_size = sizeof(unsigned long);       break;

	case MXFT_LONG:       
		if ( longs_are_64bits ) {
		              element_size = 8;
		} else {
		              element_size = 4;
		}
		break;

	case MXFT_ULONG:       
		if ( longs_are_64bits ) {
		              element_size = 8;
		} else {
		              element_size = 4;
		}
		break;

	case MXFT_RECORD:     element_size = MXU_RECORD_NAME_LENGTH;    break;
	case MXFT_RECORDTYPE: element_size = MXU_DRIVER_NAME_LENGTH;    break;
	case MXFT_INTERFACE:  element_size = MXU_INTERFACE_NAME_LENGTH; break;
	case MXFT_RECORD_FIELD:
			      element_size = MXU_RECORD_FIELD_NAME_LENGTH;
									  break;
	default:              element_size = 0;                           break;
	}

#if 0 | MX_ARRAY_DEBUG_64BIT
	if ( longs_are_64bits ) {
		MX_DEBUG(-2,
		("mxp_scaler_element_size(): longs_are_64bits = %d, "
		"mx_datatype = %ld, element_size = %ld",
	    		longs_are_64bits, mx_datatype, (long) element_size ));
	}
#endif

	return element_size;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_compute_array_header_length( unsigned long *array_header_length,
				long num_dimensions,
				long *dimension_array,
				size_t *data_element_size_array )
{
	static const char fname[] = "mx_compute_array_header_length()";

	unsigned long raw_array_header_length, modulo_value, num_blocks;

	if ( array_header_length == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The array_header_length pointer passed was NULL." );
	}
	if ( num_dimensions < 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The array num_dimensions (%ld) is illegal.  "
		"The minimum num_dimensions is 1.",
			num_dimensions );
	}

	/* The raw array header length contains space for the
	 * magic, header length, and num_dimensions fields,
	 * as well as space for both the dimension_array
	 * and the data_element_size_array.
	 */

	raw_array_header_length = 3 + 2 * num_dimensions;

	/* This header length will be used to compute an offset
	 * to the nominal start of the top level array pointer.
	 * This nominal start pointer will be used _as_ the
	 * array pointer by most other routines, so it is best
	 * if it is maximally aligned.  We do this by rounding
	 * up to the next multiple of the maximal alignment size.
	 */

	modulo_value = raw_array_header_length % MX_MAXIMUM_ALIGNMENT;

	if ( modulo_value == 0 ) {
		*array_header_length = raw_array_header_length;
	} else {
		num_blocks = 
			1 + ( raw_array_header_length / MX_MAXIMUM_ALIGNMENT );

		*array_header_length = num_blocks * MX_MAXIMUM_ALIGNMENT;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_setup_array_header( void *array_pointer,
			unsigned long array_header_length,
			long num_dimensions,
			long *dimension_array,
			size_t *data_element_size_array )
{
	static const char fname[] = "mx_setup_array_header()";

	uint32_t *header, *header_ptr;
	long i;

	if ( array_pointer == (void *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The array pointer passed was NULL." );
	}
	if ( num_dimensions < 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The array num_dimensions (%ld) is illegal.  "
		"The minimum num_dimensions is 1.",
			num_dimensions );
	}
	if ( dimension_array == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The dimension_array pointer passed was NULL." );
	}
	if ( data_element_size_array == (size_t *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The data_element_size_array pointer passed was NULL." );
	}

	header = (uint32_t *) array_pointer;

	header[ MX_ARRAY_OFFSET_MAGIC ] = MX_ARRAY_HEADER_MAGIC;
	header[ MX_ARRAY_OFFSET_HEADER_LENGTH ] = array_header_length;
	header[ MX_ARRAY_OFFSET_NUM_DIMENSIONS ] = num_dimensions;

	header_ptr = header + MX_ARRAY_OFFSET_DIMENSION_ARRAY;

	for ( i = 0; i < num_dimensions; i++ ) {
		*header_ptr = dimension_array[i];

		header_ptr--;
	}

	for ( i = 0; i < num_dimensions; i++ ) {
		*header_ptr = data_element_size_array[i];

		header_ptr--;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

/* mx_array_add_overlay() adds a multidimensional veneer on top of the
 * 1-dimensional array passed as vector_pointer.
 */

MX_EXPORT mx_status_type
mx_array_add_overlay( void *vector_pointer,
			long num_dimensions,
			long *dimension_array,
			size_t *element_size_array,
			void **array_pointer )
{
	static const char fname[] = "mx_array_add_overlay()";

	void **array_of_level_pointers;
	unsigned long i, dim, dimx, pointer_size;
	unsigned long num_pointers_in_this_level;
	unsigned long num_pointers_in_previous_level;
	unsigned long upper_pointer_size, lower_data_size;
	unsigned long upper_step_size, lower_step_size;
	char *upper_pointer, *lower_pointer;

#if MX_ARRAY_USE_ARRAY_HEADER
	char *raw_top_level_array;
#endif

	if ( vector_pointer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The vector_pointer passed was NULL." );
	}
	if ( dimension_array == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The dimension_array pointer passed was NULL." );
	}
	if ( element_size_array == (size_t *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The element_size_array pointer passed was NULL." );
	}
	if ( array_pointer == (void **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The overlay_pointer passed was NULL." );
	}

	if ( num_dimensions < 2 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The requested number of dimensions (%ld) is not supported.",
			num_dimensions );
	}

#if MX_ARRAY_DEBUG_OVERLAY
	MX_DEBUG(-2,("%s: vector_pointer = %p, num_dimensions = %ld",
		fname, vector_pointer, num_dimensions));

	for ( dim = 0; dim < num_dimensions; dim++ ) {
		MX_DEBUG(-2,(
		"%s: dimension_array[%lu] = %lu, element_size_array[%lu] = %lu",
			fname, dim, dimension_array[dim],
			dim, (unsigned long) element_size_array[dim]));
	}
#endif

	/* For each dimension above dimension 1, we allocate a single array
	 * of pointers to the next level below.
	 */

#if ( MX_ARRAY_USE_ARRAY_HEADER == FALSE )

	array_of_level_pointers = calloc( num_dimensions, sizeof(void *) );

	if ( array_of_level_pointers == (void **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of level pointers.", num_dimensions - 1 );
	}
#else
	/* The top level array of pointers is special, since we prepend
	 * the array of pointer with an array header that tells us the
	 * dimensions and size of the array.
	 */

	{
		unsigned long array_header_length, top_level_array_bytes;
		mx_status_type mx_status;

		mx_status = mx_compute_array_header_length(
					&array_header_length,
					num_dimensions,
					dimension_array,
					element_size_array );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		top_level_array_bytes = array_header_length * sizeof(uint32_t)
					 + num_dimensions * sizeof(void *);;

		raw_top_level_array = malloc( top_level_array_bytes );

		if ( raw_top_level_array == (char *) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate the raw "
			"top level array for an array of %ld dimensions.",
				num_dimensions );
		}

		memset( raw_top_level_array, 0, top_level_array_bytes );

		/* Construct the top level array_of_level_pointers
		 * from the raw_top_level_array using an offset to
		 * skip over the array header.
		 */

		array_of_level_pointers = (void *)( raw_top_level_array
				+ array_header_length * sizeof(uint32_t) );

		mx_status = mx_setup_array_header( array_of_level_pointers,
						array_header_length,
						num_dimensions,
						dimension_array,
						element_size_array );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
#endif

#if MX_ARRAY_DEBUG_OVERLAY
	MX_DEBUG(-2,("%s: array_of_level_pointers = %p",
		fname, array_of_level_pointers));
#endif

	num_pointers_in_this_level = 1;

	for ( dim = num_dimensions; dim >= 2; dim-- ) {
		num_pointers_in_this_level
			*= dimension_array[num_dimensions - dim];

		pointer_size = element_size_array[dim-1];

#if MX_ARRAY_DEBUG_OVERLAY
		MX_DEBUG(-2,("%s: allocating %lu pointers of size %lu",
			fname, num_pointers_in_this_level, pointer_size ));
#endif
		array_of_level_pointers[dim-1] =
			calloc( num_pointers_in_this_level, pointer_size );

		if ( array_of_level_pointers[dim-1] == NULL ) {

			for ( dimx = num_dimensions; dimx > dim; dimx-- ) {
				free( array_of_level_pointers[dimx-1] );
			}
			free( array_of_level_pointers );

			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %lu element "
			"array of dimension %lu pointers.",
				num_pointers_in_this_level, dim );
		}
#if MX_ARRAY_DEBUG_OVERLAY
		MX_DEBUG(-2,
    ("%s: array_of_level_pointers[%lu] = %p, num_pointers_in_this_level = %lu",
			fname, dim-1, array_of_level_pointers[dim-1],
			num_pointers_in_this_level));
#endif
	}

	array_of_level_pointers[0] = vector_pointer;

#if MX_ARRAY_DEBUG_OVERLAY
	MX_DEBUG(-2,("%s: array_of_level_pointers[0] = %p",
			fname, array_of_level_pointers[0]));

	MX_DEBUG(-2,("%s: Now fill in the dimension pointers.", fname));
#endif

	/* Now fill in the pointers from the upper layer to the lower layer
	 * at each dimension level.
	 */

	num_pointers_in_previous_level = 1;

	for ( dim = num_dimensions; dim >= 2; dim-- ) {
		upper_pointer = array_of_level_pointers[dim-1];
		lower_pointer = array_of_level_pointers[dim-2];

		upper_pointer_size = element_size_array[dim-1];
		lower_data_size    = element_size_array[dim-2];

		upper_step_size = upper_pointer_size;

		lower_step_size = lower_data_size
				* dimension_array[num_dimensions - dim + 1];

		num_pointers_in_this_level = num_pointers_in_previous_level
				* dimension_array[num_dimensions - dim];

#if MX_ARRAY_DEBUG_OVERLAY
		MX_DEBUG(-2,("%s: upper_pointer = %p, lower_pointer = %p",
			fname, upper_pointer, lower_pointer));

		MX_DEBUG(-2,("%s: dimension_array[%lu] = %lu",
			fname, dim-1, dimension_array[dim-1]));

		MX_DEBUG(-2,("%s: upper_pointer_size = %lu",
			fname, upper_pointer_size));

		MX_DEBUG(-2,("%s: lower_data_size = %lu",
			fname, lower_data_size));

		MX_DEBUG(-2,("%s: upper_step_size = %lu, lower_step_size = %lu",
			fname, upper_step_size, lower_step_size));

		MX_DEBUG(-2,("%s: num_pointers_in_this_level = %lu",
			fname, num_pointers_in_this_level));
#endif

		for ( i = 0; i < num_pointers_in_this_level; i++ ) {
#if MX_ARRAY_DEBUG_OVERLAY
			MX_DEBUG(-2,("%s: dim = %lu, i = %lu, up = %p, lp = %p",
				fname, dim, i, upper_pointer, lower_pointer));
#endif
			mx_write_void_pointer_to_memory_location( upper_pointer,
				lower_pointer );

			upper_pointer += upper_step_size;
			lower_pointer += lower_step_size;
		}

		num_pointers_in_previous_level = num_pointers_in_this_level;
	}

	*array_pointer = array_of_level_pointers[num_dimensions - 1];

#if ( MX_ARRAY_USE_ARRAY_HEADER == FALSE )
	free( array_of_level_pointers );
#else
	free( raw_top_level_array );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

#if ( MX_ARRAY_USE_ARRAY_HEADER == FALSE )

MX_EXPORT mx_status_type
mx_array_free_overlay( void *array_pointer,
			long num_dimensions )
{
	static const char fname[] = "mx_array_free_overlay()";

	void *level_pointer, *next_level_pointer;
	long dim;

	if ( array_pointer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The array_pointer argument passed was NULL." );
	}

	/* Do _not_ free the 1-D vector at the bottom of the array!
	 * It may be in use by other code such as an MX_IMAGE_FRAME.
	 * This means that we must stop at dim == 2.
	 */

	level_pointer = array_pointer;

	for ( dim = num_dimensions; dim >= 2; dim-- ) {
		next_level_pointer =
		    mx_read_void_pointer_from_memory_location( level_pointer );

		free(level_pointer);

		level_pointer = next_level_pointer;
	}

	return MX_SUCCESSFUL_RESULT;
}

#else

MX_EXPORT mx_status_type
mx_array_free_overlay( void *array_pointer,
			long num_dimensions )
{
	static const char fname[] = "mx_array_free_overlay()";

	void *level_pointer, *next_level_pointer;
	uint32_t *header, *raw_array_pointer;
	long num_header_words, header_num_dimensions;
	long dim;

	if ( array_pointer == (void *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The array_pointer argument passed was NULL." );
	}

	/* See if the top level array pointer has an array header.
	 *
	 * This is a potentially hazardous operation that may result in
	 * a segmentation fault if it does _not_ have an array header.
	 */

	header = (uint32_t *) array_pointer;

	/* First look for the header magic number. */

	if ( header[MX_ARRAY_OFFSET_MAGIC] != MX_ARRAY_HEADER_MAGIC ) {
		mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The array_pointer %p is not an array that was created "
		"using mx_array_add_offset().  "
		"This is a fatal error, so we are intentionally crashing now.",
			array_pointer );

		mx_sleep(2);

		raise( SIGSEGV );
	}

	/* Get the number of 32-bit words in the header and the number
	 * of dimensions in the array.
	 */

	num_header_words      = header[MX_ARRAY_OFFSET_HEADER_LENGTH];
	header_num_dimensions = header[MX_ARRAY_OFFSET_NUM_DIMENSIONS];

	if ( header_num_dimensions < 2 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"This function is only meant for arrays with 2 or more "
		"dimensions.  The array pointer %p has %ld dimensions.",
			array_pointer, header_num_dimensions );
	}

	if ( header_num_dimensions != num_dimensions ) {
		mx_warning( "The num_dimensions argument (%ld) passed to "
		"this function for array %p does not have the same value as "
		"the header_num_dimensions value (%ld) read from the header.  "
		"We will proceed anyway using the value from the header.",
			num_dimensions, array_pointer,
			header_num_dimensions );

		num_dimensions = header_num_dimensions;
	}

	/* The top level array of pointers must be handled specially,
	 * since the array_pointer passed to us does _not_ point to
	 * the start of the array that was initially allocated using
	 * malloc().  The _real_ start of the array starts at the
	 * beginning of the header.
	 */

	raw_array_pointer = ((uint32_t *) array_pointer) - num_header_words;

	if ( num_dimensions == 2 ) {
		mx_free( raw_array_pointer );

		return MX_SUCCESSFUL_RESULT;
	}

	/* If we have 3 dimensions or more, then we must continue on. */

	level_pointer =
		mx_read_void_pointer_from_memory_location( array_pointer );

	mx_free( raw_array_pointer );

	/* Do _not_ free the 1-D vector at the bottom of the array!
	 * It may be in use by other code such as an MX_IMAGE_FRAME.
	 * This means that we must stop at dim == 2.
	 */

	level_pointer = array_pointer;

	for ( dim = num_dimensions-1; dim >= 2; dim-- ) {
		next_level_pointer =
		    mx_read_void_pointer_from_memory_location( level_pointer );

		free(level_pointer);

		level_pointer = next_level_pointer;
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_subarray_add_overlay( void *array_pointer,
			long num_dimensions,
			long *range_array,
			size_t *data_element_size_array,
			void **subarray_pointer )
{
	static const char fname[] = "mx_subarray_add_overlay()";

	char *char_array_pointer, *char_subarray_pointer;

	if ( array_pointer == (void *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The array pointer passed was NULL." );
	}
	if ( range_array == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The range_array pointer is NULL." );
	}
	if ( data_element_size_array == (size_t *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The data_element_size_array pointer is NULL." );
	}
	if ( subarray_pointer == (void **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The subarray pointer is NULL." );
	}

	/* FIXME: For now, we only support 1-dimensional arrays. */

	if ( num_dimensions != 1 ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"You requested a subarray of a %lu dimensional array.  "
		"At present, this function only supports "
		"1-dimensional arrays.", num_dimensions );
	}

	if ( *subarray_pointer == (void *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a subarray pointer." );
	}

	char_array_pointer = array_pointer;

	char_subarray_pointer = char_array_pointer
		+ ( range_array[0] * data_element_size_array[0] );

	*subarray_pointer = char_subarray_pointer;

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_subarray_free_overlay( void *subarray_pointer,
			long num_dimensions )
{
	mx_status_type mx_status;

	/* mx_subarray_add_overlay() is supposed to create a set of row
	 * pointers that resemble the kind of set created by the 
	 * mx_array_add_overlay() function.  If this has been done
	 * correctly, then we do not have to do anything special with
	 * subarray overlays and can just pass this request on to the
	 * mx_array_free_overlay() function.
	 */

	mx_status = mx_array_free_overlay( subarray_pointer, num_dimensions );

	return mx_status;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT void *
mx_array_get_vector( void *array_pointer,
			long num_dimensions )
{
	static const char fname[] = "mx_array_get_vector()";

	void *level_pointer;
	long dim;

	if ( array_pointer == NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"The array_pointer argument passed was NULL." );

		return NULL;
	}

	level_pointer = array_pointer;

	for ( dim = num_dimensions; dim >= 2; dim-- ) {
		level_pointer =
		    mx_read_void_pointer_from_memory_location( level_pointer );
	}

	return level_pointer;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT void *
mx_allocate_array( long num_dimensions,
		long *dimension_array,
		size_t *data_element_size_array )
{
	static const char fname[] = "mx_allocate_array()";

	void *vector, *array;
	unsigned long dim, num_elements, vector_size;
	mx_status_type mx_status;

	if ( dimension_array == (long *) NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"The dimension_array pointer passed was NULL." );
	}
	if ( data_element_size_array == (size_t *) NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"The data_element_size_array pointer passed was NULL." );
	}
	if ( num_dimensions < 1 ) {
		mx_error(MXE_ILLEGAL_ARGUMENT, fname,
			"The number of dimensions (%ld) requested was less "
			"than the minimum allowed value of 1.", num_dimensions);

		return NULL;
	}

#if MX_ARRAY_DEBUG_ALLOCATE
	{
		long i;

		MX_DEBUG(-2,("%s: num_dimensions = %ld", fname,num_dimensions));

		for ( i = 0; i < num_dimensions; i++ ) {
			MX_DEBUG(-2,("%s: dim[%ld] = %lu, size[%ld] = %lu",
			fname, i, (unsigned long) dimension_array[i],
			i, (unsigned long) data_element_size_array[i] ));
		}
	}
#endif

	/* Allocate the 1-dimensional vector that goes at the bottom
	 * of the array.
	 */

	num_elements = 1;

	for ( dim = 0; dim < num_dimensions; dim++ ) {
		num_elements *= dimension_array[dim];
	}

	vector_size = num_elements * data_element_size_array[0];

#if MX_ARRAY_DEBUG_ALLOCATE
	MX_DEBUG(-2,("%s: vector_size = %lu", fname, vector_size));
#endif

#if ( MX_ARRAY_USE_ARRAY_HEADER == FALSE )	/*---*/

	vector = malloc( vector_size );

#if MX_ARRAY_DEBUG_ALLOCATE
	MX_DEBUG(-2,("%s: vector = %p", fname, vector));
#endif

	if ( vector == NULL ) {
		return NULL;
	}

	/* If this is a 1-dimensional array, then just return the vector
	 * that we just allocated.
	 */

	if ( num_dimensions < 2 ) {
		return vector;
	}

#else /* MX_ARRAY_USE_ARRAY_HEADER */

	if ( num_dimensions == 1 ) {

		/* For 1-dimensional arrays, we must encode the information
		 * about the array size in an array header before the start
		 * of the vector.
		 */

		unsigned long array_header_length, raw_vector_size;
		char *raw_vector;

		mx_status = mx_compute_array_header_length(
					&array_header_length,
					num_dimensions,
					dimension_array,
					data_element_size_array );

		if ( mx_status.code != MXE_SUCCESS )
			return NULL;

		raw_vector_size = vector_size
				+ array_header_length * sizeof(uint32_t);

		raw_vector = malloc( raw_vector_size );

		if ( raw_vector == (char *) NULL ) {
			(void) mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate the raw "
			"vector for a 1-dimensional array of %lu bytes.",
				raw_vector_size );
				
			return NULL;
		}

		memset( raw_vector, 0, raw_vector_size );

		vector = (void *)( raw_vector
				+ array_header_length * sizeof(uint32_t) );

		mx_status = mx_setup_array_header( vector,
					array_header_length,
					num_dimensions,
					dimension_array,
					data_element_size_array );

		if ( mx_status.code != MXE_SUCCESS )
			return NULL;

		return vector;
	}

	/* If we get here, we are making a multidimensional array
	 * and mx_array_add_overlay() will handle making the header.
	 */

	vector = malloc( vector_size );

	if ( vector == NULL ) {
		return NULL;
	}
#endif /* MX_ARRAY_USE_ARRAY_HEADER */

	/* Multidimensional arrays use mx_array_add_overlay(). */

	mx_status = mx_array_add_overlay( vector,
					num_dimensions,
					dimension_array,
					data_element_size_array,
					&array );

	if ( mx_status.code != MXE_SUCCESS )
		return NULL;

	return array;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_free_array( void *array_pointer,
		long num_dimensions )
{
	static const char fname[] = "mx_free_array()";

	void *vector;
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

	if ( array_pointer == NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"The array_pointer value passed was NULL." );
	}
	if ( num_dimensions < 1 ) {
		return mx_error(MXE_ILLEGAL_ARGUMENT, fname,
			"The number of dimensions (%ld) requested was less "
			"than the minimum allowed value of 1.", num_dimensions);
	}

	/* We do not actually use the dimension_array or the
	 * data_element_size_array arguments which are present
	 * only for compatibility with mx_free_array_old().
	 */

	vector = mx_array_get_vector( array_pointer, num_dimensions );

	if ( vector == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The vector pointer for array pointer %p is NULL.",
			array_pointer );
	}

#if ( MX_ARRAY_USE_ARRAY_HEADER == FALSE )

	if ( num_dimensions >= 2 ) {
		mx_status = mx_array_free_overlay( array_pointer,
							num_dimensions );
	}

	mx_free( vector );

#else /* MX_ARRAY_USE_ARRAY_HEADER */

	if ( num_dimensions >= 2 ) {
		mx_status = mx_array_free_overlay( array_pointer,
							num_dimensions );

		mx_free( vector );

		return mx_status;
	}

	{
		uint32_t *header, *raw_vector;
		long num_header_words;

		/* For 1-dimensional arrays, we need to find the start of the
		 * array header.
		 */

		header = (uint32_t *) vector;

		/* Look for the header magic number. */

		if ( header[MX_ARRAY_OFFSET_MAGIC] != MX_ARRAY_HEADER_MAGIC ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The array_pointer %p is not an array that was created "
			"using mx_allocate_array().  This is a fatal error, "
			"so we are intentionally crashing now.",
				array_pointer );

			mx_sleep(2);

			raise( SIGSEGV );
		}

		num_header_words = header[MX_ARRAY_OFFSET_HEADER_LENGTH];

		raw_vector = ((uint32_t *) vector) - num_header_words;

		mx_free( vector );

		mx_status = MX_SUCCESSFUL_RESULT;
	}

#endif /* MX_ARRAY_USE_ARRAY_HEADER */

	return mx_status;
}

/*---------------------------------------------------------------------------*/

static mx_status_type
mxp_realloc_handle_array_level( long new_num_dimensions,
				void *array_pointer,
				void *new_array_pointer,
				long *old_dimension_array,
				long *new_dimension_array,
				size_t *old_element_size_array )
{
	static const char fname[] = "mxp_realloc_handle_array_level()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"This function has not yet been implemented." );
}

/*----*/

MX_EXPORT void *
mx_reallocate_array( void *array_pointer,
			long new_num_dimensions,
			long *new_dimension_array )
{
	static const char fname[] = "mx_reallocate_array()";

	void *new_array_pointer;
	uint32_t *header;
	unsigned long header_magic, header_length, computed_header_length;
	long i, j, num_dimensions;
	long *old_dimension_array;
	size_t *old_element_size_array;
	mx_status_type mx_status;

	if ( array_pointer == (void *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The array pointer passed is NULL." );

		return NULL;
	}
	if ( new_dimension_array == (long *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The dimension array pointer passed was NULL." );

		return NULL;
	}

	header = (unsigned long *) array_pointer;

	header_magic = header[MX_ARRAY_OFFSET_MAGIC];

	if ( header_magic != MX_ARRAY_HEADER_MAGIC ) {
		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The array pointer (%p) was not created by "
		"mx_allocate_array().", array_pointer );

		return NULL;
	}

	header_length = header[MX_ARRAY_OFFSET_HEADER_LENGTH];

	num_dimensions = header[MX_ARRAY_OFFSET_NUM_DIMENSIONS];

	if ( new_num_dimensions < 0 ) {
		new_num_dimensions = num_dimensions;
	}

	if ( num_dimensions != new_num_dimensions ) {
		(void) mx_error( MXE_UNSUPPORTED, fname,
		"This function does not support changing the number "
		"of dimensions of an array.  Only changing the size "
		"of each dimension is supported." );

		return NULL;
	}

	computed_header_length = 3 + 2 * num_dimensions;

	if ( computed_header_length != header_length ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The computed header length (%lu) of array %p does not "
		"match the header length (%lu) found in the array's header.",
			computed_header_length,
			array_pointer,
			header_length );

		return NULL;
	}

	old_dimension_array = calloc( header_length, sizeof(size_t) );

	if ( old_dimension_array == (long *) NULL ) {
		(void) mx_error( MXE_OUT_OF_MEMORY, fname,
		"The attempt to allocate a %lu element array "
		"of dimension sizes failed.",
			header_length );

		return NULL;
	}

	old_element_size_array = calloc( header_length, sizeof(size_t) );

	if ( old_element_size_array == (size_t *) NULL ) {
		(void) mx_error( MXE_OUT_OF_MEMORY, fname,
		"The attempt to allocate a %lu element array "
		"of data element sizes failed.",
			header_length );

		mx_free( old_dimension_array );
		return NULL;
	}

	for ( i = 0; i < num_dimensions; i++ ) {
		j = 4 + i;

		old_dimension_array[i] = header[ -j ];
	}

	for ( i = 0; i < num_dimensions; i++ ) {
		j = 4 + num_dimensions + i;

		old_element_size_array[i] = header[ -j ];
	}

	new_array_pointer = mx_allocate_array( new_num_dimensions,
						new_dimension_array,
						old_element_size_array );

	if ( new_array_pointer == (void *) NULL ) {
		mx_free( old_dimension_array );
		mx_free( old_element_size_array );
		return NULL;
	}

	mx_status = mxp_realloc_handle_array_level( new_num_dimensions,
						array_pointer,
						new_array_pointer,
						old_dimension_array,
						new_dimension_array,
						old_element_size_array );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_free( old_dimension_array );
		mx_free( old_element_size_array );
		mx_free_array( new_array_pointer, new_num_dimensions );

		return NULL;
	}

	return new_array_pointer;
}

/*---------------------------------------------------------------------------*/

static void
mx_copy_32bits_to_64bits( void *destination, void *source, size_t num_elements )
{
#if MX_ARRAY_DEBUG_64BIT_COPY
	static const char fname[] = "mx_copy_32bits_to_64bits()";

	size_t max_elements;
#endif
	uint64_t *uint64_ptr;
	uint32_t *uint32_ptr;

	uint64_t uint64_value;

	union {
		int32_t int32_value;
		uint32_t uint32_value;
	} u_32bit;

	size_t i;

#if MX_ARRAY_DEBUG_64BIT_COPY
	MX_DEBUG(-2,("%s: num_elements = %ld", fname, (long) num_elements ));
#endif

	uint64_ptr = destination;
	uint32_ptr = source;

#if MX_ARRAY_DEBUG_64BIT_COPY
	MX_DEBUG(-2,("%s: uint64_ptr = %p, uint32_ptr = %p",
		fname, uint64_ptr, uint32_ptr));

	if ( num_elements > 10 ) {
		max_elements = 10;
	} else {
		max_elements = num_elements;
	}

	for ( i = 0; i < max_elements; i++ ) {
		fprintf( stderr, "&uint32_ptr[%d] = %p, ",
			(int) i, &uint32_ptr[i] );

		fprintf( stderr, "uint32_ptr[%d] = %" PRIu32 "\n",
			(int) i, uint32_ptr[i] );
	}
#endif

#if 0
	for ( i = 0; i < num_elements; i++ ) {
		uint64_ptr[i] = (uint64_t) uint32_ptr[i];
	}
#else
	for ( i = 0; i < num_elements; i++ ) {
		u_32bit.uint32_value = uint32_ptr[i];

		uint64_value = (uint64_t) u_32bit.uint32_value;

		/* If uint32_value is negative, then sign extend the
		 * 64 bit value.
		 *
		 * Warning: This _ASSUMES_ we are using two's complement
		 * integer arithmetic.
		 */

		if ( u_32bit.int32_value < 0 ) {
			uint64_value |= UINT64_C( 0xffffffff00000000 );
		}

		uint64_ptr[i] = uint64_value;
	}
#endif

#if MX_ARRAY_DEBUG_64BIT_COPY
	for ( i = 0; i < max_elements; i++ ) {
		fprintf( stderr, "&uint64_ptr[%d] = %p, ",
			(int) i, &uint64_ptr[i] );

		fprintf( stderr, "uint64_ptr[%d] = %" PRIu64 "\n",
			(int) i, uint64_ptr[i] );
	}
#endif
	return;
}

static void
mx_copy_64bits_to_32bits( void *destination, void *source, size_t num_elements )
{
#if MX_ARRAY_DEBUG_64BIT_COPY
	static const char fname[] = "mx_copy_64bits_to_32bits()";

	size_t max_elements;
#endif
	uint64_t *uint64_ptr;
	uint32_t *uint32_ptr;
	size_t i;

#if MX_ARRAY_DEBUG_64BIT_COPY
	MX_DEBUG(-2,("%s: num_elements = %ld", fname, (long) num_elements ));
#endif

	uint32_ptr = destination;
	uint64_ptr = source;

#if MX_ARRAY_DEBUG_64BIT_COPY
	MX_DEBUG(-2,("%s: uint32_ptr = %p, uint64_ptr = %p",
		fname, uint32_ptr, uint64_ptr));

	if ( num_elements > 10 ) {
		max_elements = 10;
	} else {
		max_elements = num_elements;
	}

	for ( i = 0; i < max_elements; i++ ) {
		fprintf( stderr, "&uint64_ptr[%d] = %p, ",
			(int) i, &uint64_ptr[i] );

		fprintf( stderr, "uint64_ptr[%d] = %" PRIu64 "\n",
			(int) i, uint64_ptr[i] );
	}
#endif

	for ( i = 0; i < num_elements; i++ ) {
		uint32_ptr[i] = (uint32_t) uint64_ptr[i];
	}

#if MX_ARRAY_DEBUG_64BIT_COPY
	for ( i = 0; i < max_elements; i++ ) {
		fprintf( stderr, "&uint32_ptr[%d] = %p, ",
			(int) i, &uint32_ptr[i] );

		fprintf( stderr, "uint32_ptr[%d] = %" PRIu32 "\n",
			(int) i, uint32_ptr[i] );
	}
#endif
	return;
}

static size_t
mx_get_network_bytes_from_native_bytes( long mx_datatype,
					size_t native_bytes_to_copy,
					mx_bool_type use_64bit_network_longs,
					mx_bool_type native_longs_are_64bits )
{
	size_t network_bytes_to_copy;

#if MX_ARRAY_DEBUG_64BIT
	static const char fname[] = "mx_get_network_bytes_from_native_bytes()";

	MX_DEBUG(-2,("%s: mx_datatype = %ld, native_bytes_to_copy = %ld, "
		"use_64bit_network_longs = %d, native_longs_are_64bits = %d",
		fname, mx_datatype,
		(long) native_bytes_to_copy,
		(int) use_64bit_network_longs,
		(int) native_longs_are_64bits ));
#endif

	switch( mx_datatype ) {
	case MXFT_HEX:
	case MXFT_LONG:
	case MXFT_ULONG:
		if ( use_64bit_network_longs ) {
		    if ( native_longs_are_64bits ) {
			network_bytes_to_copy = native_bytes_to_copy;
		    } else {
			network_bytes_to_copy = 2*native_bytes_to_copy;
		    }
		} else {
		    if ( native_longs_are_64bits ) {
			network_bytes_to_copy = native_bytes_to_copy/2;
		    } else {
			network_bytes_to_copy = native_bytes_to_copy;
		    }
		}
		break;

	default:
		network_bytes_to_copy = native_bytes_to_copy;
		break;
	}

#if MX_ARRAY_DEBUG_64BIT
	MX_DEBUG(-2,("%s: network_bytes_to_copy = %ld",
		fname, (long) network_bytes_to_copy));
#endif

	return network_bytes_to_copy;
}

MX_EXPORT mx_status_type
mx_copy_array_to_buffer( void *array_pointer,
		mx_bool_type array_is_dynamically_allocated,
		long mx_datatype, long num_dimensions,
		long *dimension_array, size_t *data_element_size_array,
		void *destination_buffer, size_t destination_buffer_length,
		size_t *num_network_bytes_copied,
		mx_bool_type use_64bit_network_longs )
{
	static const char fname[] = "mx_copy_array_to_buffer()";

	MX_RECORD *mx_record;
	MX_DRIVER *mx_driver;
	MX_INTERFACE *mx_interface;
	MX_RECORD_FIELD *mx_field;
	char *array_row_pointer, *destination_pointer;
	size_t native_bytes_to_copy, network_bytes_to_copy;
	size_t array_size, subarray_size, buffer_left;
	size_t element_size;
	mx_bool_type native_longs_are_64bits;
	long i, n, mx_type;
	int num_subarray_elements;
	int structure_name_length;
	mx_bool_type use_structure_name;
	mx_status_type mx_status;

	if ( ( array_pointer == NULL )
	  || ( data_element_size_array == NULL )
	  || ( destination_buffer == NULL ) )
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "One or more of the pointers passed to this function were NULL." );
	}
	if ( ( num_dimensions > 0 ) && ( dimension_array == NULL ) ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"dimension_array is NULL even though num_dimensions (%ld) "
		"is greater than 0.", num_dimensions );
	}

	if ( num_network_bytes_copied != NULL ) {
		*num_network_bytes_copied = 0;
	}

	if ( num_dimensions < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The number of dimensions passed (%ld) is a negative number.",
			num_dimensions );
	}

#if MX_ARRAY_DEBUG_64BIT
	MX_DEBUG(-2,("%s: use_64bit_network_longs = %d",
		fname, (int) use_64bit_network_longs));
#endif

	/* For datatypes that correspond to structures, we report the name
	 * of the structure, for example record->name, rather than something
	 * like a pointer to the data structure
	 */

	switch( mx_datatype ) {
	case MXFT_RECORD:
	case MXFT_RECORDTYPE:
	case MXFT_INTERFACE:
	case MXFT_RECORD_FIELD:
		structure_name_length =
			mx_get_scalar_element_size( mx_datatype, FALSE );
		use_structure_name = TRUE;
		break;
	default:
		structure_name_length = 0;
		use_structure_name = FALSE;
		break;
	}

#if 0
	MX_DEBUG(-2,("%s: *** array_pointer = %p", fname, array_pointer));
	MX_DEBUG(-2,("%s: mx_datatype = %ld", fname, mx_datatype));
	MX_DEBUG(-2,("%s: num_dimensions = %ld", fname, num_dimensions));
	MX_DEBUG(-2,("%s: dimension_array = %p", fname, dimension_array));
	MX_DEBUG(-2,("%s: data_element_size_array = %p",
					fname, data_element_size_array));
	MX_DEBUG(-2,("%s: destination_buffer = %p", fname, destination_buffer));
#endif

	if ( MX_WORDSIZE >= 64 ) {
		native_longs_are_64bits = TRUE;
	} else {
		native_longs_are_64bits = FALSE;
	}

	if ( num_dimensions == 0 ) {

		/* Handling scalars takes a bit more effort. */

		native_bytes_to_copy = mx_get_scalar_element_size( mx_datatype,
						native_longs_are_64bits );

		network_bytes_to_copy = mx_get_network_bytes_from_native_bytes(
						mx_datatype,
						native_bytes_to_copy,
						use_64bit_network_longs,
						native_longs_are_64bits );

		if ( network_bytes_to_copy > destination_buffer_length ) {
			if ( num_network_bytes_copied != NULL ) {
				*num_network_bytes_copied =
			    network_bytes_to_copy - destination_buffer_length;
			}
			return mx_error(
				(MXE_WOULD_EXCEED_LIMIT | MXE_QUIET), fname,
				"The scaler of size %ld bytes is too large to "
				"fit into the destination buffer of %ld bytes.",
					(long) network_bytes_to_copy,
					(long) destination_buffer_length );
		}

#if 0
		MX_DEBUG(-2,("%s: num_dimensions = 0, mx_datatype = %ld, "
		"native_bytes_to_copy = %d, network_bytes_to_copy = %d",
			fname, mx_datatype,
			(int) native_bytes_to_copy,
			(int) network_bytes_to_copy));
#endif

		switch( mx_datatype ) {
		case MXFT_STRING:
			return mx_error( MXE_UNSUPPORTED, fname,
				"MX does not support 0-dimensional strings." );
		case MXFT_CHAR:
		case MXFT_UCHAR:
		case MXFT_SHORT:
		case MXFT_USHORT:
		case MXFT_BOOL:
		case MXFT_INT64:
		case MXFT_UINT64:
		case MXFT_FLOAT:
		case MXFT_DOUBLE:
			memcpy( destination_buffer,
					array_pointer, network_bytes_to_copy );
			break;

		case MXFT_HEX:
		case MXFT_LONG:
		case MXFT_ULONG:
			if ( native_longs_are_64bits ) {
			    if ( use_64bit_network_longs ) {
				memcpy( destination_buffer,
					array_pointer, network_bytes_to_copy );
			    } else {
				mx_copy_64bits_to_32bits( destination_buffer,
					array_pointer, 1 );
			    }
			} else {
			    if ( use_64bit_network_longs ) {

				mx_copy_32bits_to_64bits( destination_buffer,
					array_pointer, 1 );
			    } else {
				memcpy( destination_buffer,
					array_pointer, network_bytes_to_copy );
			    }
			}
			break;

			/* NOTE: 1-dimensional arrays of structures are
			 * reported as if they were 2-dimensional arrays
			 * of strings, where the strings are the 'names'
			 * of the structures, for example record->name.
			 */

		case MXFT_RECORD:
			mx_record = (MX_RECORD *) array_pointer;

			strlcpy( destination_buffer, mx_record->name,
					network_bytes_to_copy );
			break;
		case MXFT_RECORDTYPE:
			mx_type = *((unsigned long *) array_pointer);

#if 0
			MX_DEBUG(-2,("%s: mx_type = %lu",
				fname, mx_type));
#endif

			mx_driver = mx_get_driver_by_type( mx_type );

			if ( mx_driver == (MX_DRIVER *) NULL ) {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"MX driver type %ld is not found in the "
				"loaded list of MX drivers.", mx_type );
			}

#if 0
			MX_DEBUG(-2,("%s: mx_driver->name = '%s'",
				fname, mx_driver->name));
#endif

			strlcpy( destination_buffer, mx_driver->name,
					network_bytes_to_copy );
			break;
		case MXFT_INTERFACE:
			mx_interface = (MX_INTERFACE *) array_pointer;

			strlcpy( destination_buffer,
					mx_interface->address_name,
					network_bytes_to_copy );
			break;
		case MXFT_RECORD_FIELD:
			mx_field = (MX_RECORD_FIELD *) array_pointer;

			if ( mx_field->record == (MX_RECORD *) NULL ) {
				snprintf( destination_buffer,
					network_bytes_to_copy,
					"\?\?\?.%s", mx_field->name );
			} else {
				snprintf( destination_buffer,
					network_bytes_to_copy,
					"%s.%s", mx_field->record->name,
						mx_field->name );
			}

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Array copy for data type %ld is not supported.",
				mx_datatype );
		}

		if ( num_network_bytes_copied != NULL ) {
			*num_network_bytes_copied = network_bytes_to_copy;
		}

		return MX_SUCCESSFUL_RESULT;
	}

	if ( num_dimensions == 1 ) { 

		MX_RECORD **record_array = NULL;

		element_size = mx_get_scalar_element_size( mx_datatype,
						native_longs_are_64bits );

		native_bytes_to_copy = dimension_array[0] * element_size;

		network_bytes_to_copy = mx_get_network_bytes_from_native_bytes(
						mx_datatype,
						native_bytes_to_copy,
						use_64bit_network_longs,
						native_longs_are_64bits );

		if ( network_bytes_to_copy > destination_buffer_length ) {
			if ( num_network_bytes_copied != NULL ) {
				*num_network_bytes_copied =
			    network_bytes_to_copy - destination_buffer_length;
			}
			return mx_error(
				(MXE_WOULD_EXCEED_LIMIT | MXE_QUIET), fname,
				"The 1-dimensional array of size %ld bytes is "
				"too large to fit into the destination buffer "
				"of %ld bytes.",
					(long) network_bytes_to_copy,
					(long) destination_buffer_length );
		}

		switch( mx_datatype ) {
		case MXFT_STRING:
			strlcpy( (char *) destination_buffer,
					array_pointer, network_bytes_to_copy );
			break;

		case MXFT_CHAR:
		case MXFT_UCHAR:
		case MXFT_SHORT:
		case MXFT_USHORT:
		case MXFT_BOOL:
		case MXFT_INT64:
		case MXFT_UINT64:
		case MXFT_FLOAT:
		case MXFT_DOUBLE:
			memcpy( destination_buffer,
					array_pointer, network_bytes_to_copy );
			break;
		case MXFT_HEX:
		case MXFT_LONG:
		case MXFT_ULONG:
			if ( native_longs_are_64bits ) {
			    if ( use_64bit_network_longs ) {
				memcpy( destination_buffer,
					array_pointer, network_bytes_to_copy );
			    } else {
				mx_copy_64bits_to_32bits(
				    destination_buffer, array_pointer,
				    network_bytes_to_copy / sizeof(uint32_t) );
			    }
			} else {
			    if ( use_64bit_network_longs ) {

				mx_copy_32bits_to_64bits(
				    destination_buffer, array_pointer,
				    network_bytes_to_copy / sizeof(uint64_t) );
			    } else {
				memcpy( destination_buffer,
					array_pointer, network_bytes_to_copy );
			    }
			}
			break;
		case MXFT_RECORD:
			record_array = array_pointer;

			if ( network_bytes_to_copy > destination_buffer_length )
			{
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"Copying the record names from the %lu element "
				"array of MX_RECORD names starting with '%s' "
				"would take %lu network bytes.  "
				"However, the destination buffer has a "
				"maximum length of %lu.",
					dimension_array[0],
					record_array[0]->name,
				    (unsigned long) network_bytes_to_copy,
				    (unsigned long) destination_buffer_length );
			}

			memset( destination_buffer, 0, network_bytes_to_copy );

			for ( i = 0; i < dimension_array[0]; i++ ) {
				destination_pointer =
					(char *) destination_buffer
						+ i * element_size;

				strlcpy( destination_pointer,
					record_array[i]->name,
					element_size );

#if 0
				MX_DEBUG(-2,
				("%s: i = %ld, destination_pointer = %p",
					fname, i, destination_pointer));
				MX_DEBUG(-2,("%s: destination_pointer = '%s'",
					fname, destination_pointer));
#endif
			}

			break;
		case MXFT_RECORDTYPE:
		case MXFT_INTERFACE:
		case MXFT_RECORD_FIELD:
			mx_breakpoint();
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Array copy for data type %ld is not supported.",
				mx_datatype );
		}

		if ( num_network_bytes_copied != NULL ) {
			*num_network_bytes_copied = network_bytes_to_copy;
		}

		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, either 'num_dimensions > 1' or this array is 
	 * a structure array for which we report the name instead.
	 */

	num_subarray_elements = 1;

	for ( i = 1; i < num_dimensions; i++ ) {
		num_subarray_elements *= ( dimension_array[i] );
	}

	if ( use_structure_name ) {
		/* This is a structure array. */

		subarray_size = num_subarray_elements * structure_name_length;
	} else {
		/* num_dimensions > 1 */

		subarray_size = num_subarray_elements
					* data_element_size_array[0];
	}

	array_size = subarray_size * dimension_array[0];

#if 0
	switch( mx_datatype ) {
	default:
		MX_DEBUG(-2,(
			"%s: mx_datatype = %ld, num_subarray_elements = %d",
			fname, mx_datatype, num_subarray_elements));
		MX_DEBUG(-2,("%s: subarray_size = %ld, array_size = %ld",
			fname, (long) subarray_size, (long) array_size));
	}
#endif

	if ( array_size > destination_buffer_length ) {
		if ( num_network_bytes_copied != NULL ) {
			*num_network_bytes_copied =
				array_size - destination_buffer_length;
		}
		return mx_error( (MXE_WOULD_EXCEED_LIMIT | MXE_QUIET), fname,
			"The %ld-dimensional array of size %ld bytes is "
			"too large to fit into the destination buffer "
			"of %ld bytes.",
				num_dimensions,
				(long) array_size,
				(long) destination_buffer_length );
	}

	for ( n = 0; n < dimension_array[0]; n++ ) {

		array_row_pointer = (char *) array_pointer
			+ n * data_element_size_array[num_dimensions - 1];

		if ( array_is_dynamically_allocated ) {
			array_row_pointer = (char *)
				mx_read_void_pointer_from_memory_location(
					array_row_pointer );
		}

		destination_pointer = (char *) destination_buffer
			+ n * subarray_size;

		buffer_left = destination_buffer_length - n * subarray_size;

		mx_status = mx_copy_array_to_buffer( array_row_pointer,
				array_is_dynamically_allocated,
				mx_datatype, num_dimensions - 1,
				&dimension_array[1], data_element_size_array,
				destination_pointer, buffer_left,
				NULL, use_64bit_network_longs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( num_network_bytes_copied != NULL ) {
		*num_network_bytes_copied = array_size;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_copy_buffer_to_array( void *source_buffer, size_t source_buffer_length,
		void *array_pointer,
		mx_bool_type array_is_dynamically_allocated,
		long mx_datatype, long num_dimensions,
		long *dimension_array, size_t *data_element_size_array,
		size_t *num_native_bytes_copied,
		mx_bool_type use_64bit_network_longs )
{
	static const char fname[] = "mx_copy_buffer_to_array()";

	char *array_row_pointer, *source_pointer;
	size_t network_bytes_to_copy, native_bytes_to_copy;
	size_t array_size, subarray_size, buffer_left;
	mx_bool_type native_longs_are_64bits;
	long i, n, num_subarray_elements;
	mx_status_type mx_status;

	if ( ( array_pointer == NULL )
	  || ( data_element_size_array == NULL )
	  || ( source_buffer == NULL ) )
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "One or more of the pointers passed to this function were NULL." );
	}
	if ( ( num_dimensions > 0 ) && ( dimension_array == NULL ) ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"dimension_array is NULL even though num_dimensions (%ld) "
		"is greater than 0.", num_dimensions );
	}

#if MX_ARRAY_DEBUG_64BIT
	MX_DEBUG(-2,("%s: use_64bit_network_longs = %d",
		fname, (int) use_64bit_network_longs));
#endif

	/* Disallow writes to complex field types since correctly supporting
	 * writes to these datatypes would be very difficult, especially for
	 * MXFT_RECORDTYPE, and not worth the effort required to implement
	 * such a thing.
	 */

	switch( mx_datatype ) {
	case MXFT_RECORD:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Writing to an MXFT_RECORD field is not allowed." );
	case MXFT_RECORDTYPE:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Writing to an MXFT_RECORDTYPE field is not allowed." );
	case MXFT_INTERFACE:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Writing to an MXFT_INTERFACE field is not allowed." );
	case MXFT_RECORD_FIELD:
		return mx_error( MXE_UNSUPPORTED, fname,
		    "Writing to an MXFT_RECORD_FIELD field is not allowed." );
	}

	if ( num_native_bytes_copied != NULL ) {
		*num_native_bytes_copied = 0;
	}

	if ( num_dimensions < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The number of dimensions passed (%ld) is a negative number.",
			num_dimensions );
	}

#if 0
	MX_DEBUG(-2,("%s: *** array_pointer = %p", fname, array_pointer));
	MX_DEBUG(-2,("%s: mx_datatype = %ld", fname, mx_datatype));
	MX_DEBUG(-2,("%s: num_dimensions = %ld", fname, num_dimensions));
	MX_DEBUG(-2,("%s: dimension_array = %p", fname, dimension_array));
	MX_DEBUG(-2,("%s: data_element_size_array = %p",
					fname, data_element_size_array));
	MX_DEBUG(-2,("%s: source_buffer = %p", fname, source_buffer));
#endif

	if ( MX_WORDSIZE >= 64 ) {
		native_longs_are_64bits = TRUE;
	} else {
		native_longs_are_64bits = FALSE;
	}

	if ( num_dimensions == 0 ) {

		/* Handling scalars takes a bit more effort. */

		native_bytes_to_copy = mx_get_scalar_element_size( mx_datatype,
						native_longs_are_64bits );

		network_bytes_to_copy = mx_get_network_bytes_from_native_bytes(
						mx_datatype,
						native_bytes_to_copy,
						use_64bit_network_longs,
						native_longs_are_64bits );

		if ( network_bytes_to_copy > source_buffer_length ) {
			return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
			"The size (%ld bytes) of the scalar value for this "
			"array is larger than the source buffer length "
			"of %ld bytes.",
				(long) network_bytes_to_copy,
				(long) source_buffer_length );
		}

#if 0
		MX_DEBUG(-2,("%s: num_dimensions = 0, mx_datatype = %ld, "
		"native_bytes_to_copy = %d, network_bytes_to_copy = %d",
			fname, mx_datatype,
			native_bytes_to_copy, network_bytes_to_copy));
#endif

		switch( mx_datatype ) {
		case MXFT_STRING:
			return mx_error( MXE_UNSUPPORTED, fname,
				"MX does not support 0-dimensional strings." );
		case MXFT_CHAR:
		case MXFT_UCHAR:
		case MXFT_SHORT:
		case MXFT_USHORT:
		case MXFT_BOOL:
		case MXFT_INT64:
		case MXFT_UINT64:
		case MXFT_FLOAT:
		case MXFT_DOUBLE:
			memcpy( array_pointer,
					source_buffer, native_bytes_to_copy );
			break;

		case MXFT_HEX:
		case MXFT_LONG:
		case MXFT_ULONG:
			if ( native_longs_are_64bits ) {
			    if ( use_64bit_network_longs ) {

				memcpy( array_pointer,
					source_buffer, network_bytes_to_copy );
			    } else {
				mx_copy_32bits_to_64bits( array_pointer,
							source_buffer, 1 );
			    }
			} else {
			    if ( use_64bit_network_longs ) {

				mx_copy_64bits_to_32bits( array_pointer,
							source_buffer, 1 );
			    } else {
				memcpy( array_pointer,
					source_buffer, network_bytes_to_copy );
			    }
			}
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Array copy for data type %ld is not supported.",
				mx_datatype );
		}

		if ( num_native_bytes_copied != NULL ) {
			*num_native_bytes_copied = native_bytes_to_copy;
		}

		return MX_SUCCESSFUL_RESULT;
	}

	if ( num_dimensions == 1 ) {

		switch( mx_datatype ) {
		case MXFT_RECORD:
		case MXFT_RECORDTYPE:
		case MXFT_INTERFACE:
		case MXFT_RECORD_FIELD:
			dimension_array[0] = 1;
			break;
		}

		native_bytes_to_copy = dimension_array[0]
				* mx_get_scalar_element_size( mx_datatype,
						native_longs_are_64bits );

		network_bytes_to_copy = mx_get_network_bytes_from_native_bytes(
						mx_datatype,
						native_bytes_to_copy,
						use_64bit_network_longs,
						native_longs_are_64bits );

		/* Check for buffer overread. */

		if ( network_bytes_to_copy > source_buffer_length ) {
			network_bytes_to_copy = source_buffer_length;
		}
			
		switch( mx_datatype ) {
		case MXFT_STRING:
			strlcpy( (char *) array_pointer,
				source_buffer, network_bytes_to_copy );
			break;

		case MXFT_CHAR:
		case MXFT_UCHAR:
		case MXFT_SHORT:
		case MXFT_USHORT:
		case MXFT_BOOL:
		case MXFT_INT64:
		case MXFT_UINT64:
		case MXFT_FLOAT:
		case MXFT_DOUBLE:
		case MXFT_RECORD:
		case MXFT_RECORDTYPE:
		case MXFT_INTERFACE:
		case MXFT_RECORD_FIELD:
			memcpy( array_pointer,
				source_buffer, network_bytes_to_copy );
			break;

		case MXFT_HEX:
		case MXFT_LONG:
		case MXFT_ULONG:
			if ( native_longs_are_64bits ) {
			    if ( use_64bit_network_longs ) {

				memcpy( array_pointer,
					source_buffer, network_bytes_to_copy );
			    } else {
				mx_copy_32bits_to_64bits(
				    array_pointer, source_buffer,
				    network_bytes_to_copy / sizeof(uint32_t) );
			    }
			} else {
			    if ( use_64bit_network_longs ) {

				mx_copy_64bits_to_32bits(
				    array_pointer, source_buffer,
				    network_bytes_to_copy / sizeof(uint64_t) );
			    } else {
				memcpy( array_pointer,
					source_buffer, network_bytes_to_copy );
			    }
			}
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Array copy for data type %ld is not supported.",
				mx_datatype );
		}

		if ( num_native_bytes_copied != NULL ) {
			*num_native_bytes_copied = native_bytes_to_copy;
		}

		return MX_SUCCESSFUL_RESULT;
	}

	/* num_dimensions > 1 */

	num_subarray_elements = 1;

	for ( i = 1; i < num_dimensions; i++ ) {
		num_subarray_elements *= ( dimension_array[i] );
	}

	subarray_size = num_subarray_elements * data_element_size_array[0];

	array_size = subarray_size * dimension_array[0];

	if ( array_size > source_buffer_length ) {
		return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
			"The %ld-dimensional array of size %ld bytes is larger "
			"than the source buffer of %ld bytes.",
				num_dimensions,
				(long) array_size,
				(long) source_buffer_length );
	}

	for ( n = 0; n < dimension_array[0]; n++ ) {

		array_row_pointer = (char *) array_pointer
			+ n * data_element_size_array[num_dimensions - 1];

		if ( array_is_dynamically_allocated ) {
			array_row_pointer = (char *)
				mx_read_void_pointer_from_memory_location(
					array_row_pointer );
		}

		source_pointer = (char *) source_buffer
			+ n * subarray_size;

		buffer_left = source_buffer_length - n * subarray_size;

		mx_status = mx_copy_buffer_to_array(
				source_pointer, buffer_left,
				array_row_pointer,
				array_is_dynamically_allocated,
				mx_datatype, num_dimensions - 1,
				&dimension_array[1], data_element_size_array,
				NULL,
				use_64bit_network_longs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( num_native_bytes_copied != NULL ) {
		*num_native_bytes_copied = array_size;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT size_t
mx_xdr_get_scalar_element_size( long mx_datatype ) {

	size_t element_size, native_size;

	switch( mx_datatype ) {
	case MXFT_STRING:
		element_size = 1;
		break;
	case MXFT_CHAR:
	case MXFT_UCHAR:
	case MXFT_SHORT:
	case MXFT_USHORT:
	case MXFT_BOOL:
	case MXFT_LONG:
	case MXFT_ULONG:
	case MXFT_HEX:
	case MXFT_FLOAT:
		element_size = 4;
		break;
	case MXFT_INT64:
	case MXFT_UINT64:
	case MXFT_DOUBLE:
		element_size = 8;
		break;
	case MXFT_RECORD:
	case MXFT_RECORDTYPE:
	case MXFT_INTERFACE:
	case MXFT_RECORD_FIELD:
		native_size = mx_get_scalar_element_size( mx_datatype, FALSE );

		element_size = 4 * (native_size / 4);

		if ( ( native_size % 4 ) != 0 ) {
			element_size += 4;
		}
		break;
	default:
		element_size = 0;
		break;
	}

	return element_size;
}

#if HAVE_XDR

MX_EXPORT mx_status_type
mx_xdr_data_transfer( int direction, void *array_pointer,
		int array_is_dynamically_allocated,
		long mx_datatype, long num_dimensions,
		long *dimension_array, size_t *data_element_size_array,
		void *xdr_buffer, size_t xdr_buffer_length,
		size_t *num_bytes_copied )
{
	static const char fname[] = "mx_xdr_data_transfer()";

	XDR xdrs;
	MX_RECORD *mx_record;
	MX_DRIVER *mx_driver;
	MX_INTERFACE *mx_interface;
	MX_RECORD_FIELD *mx_field;
	char field_name[ MXU_RECORD_FIELD_NAME_LENGTH+1 ];
	char *array_row_pointer, *xdr_ptr, *ptr;
	size_t subarray_size_product;
	size_t first_dimension_size, last_dimension_size;
	size_t last_dimension_native_size_in_bytes;
	size_t last_dimension_xdr_size_in_bytes;
	size_t native_array_size, native_subarray_size;
	size_t xdr_data_size, xdr_array_size, xdr_subarray_size;
	size_t buffer_left, remainder;
	unsigned long xdr_buffer_address, address_remainder;
	long i, n;
	int xdr_status;
	u_int num_array_elements;
	int return_structure_name;
	enum xdr_op operation;
	mx_status_type mx_status;

	if ( ( array_pointer == NULL )
	  || ( data_element_size_array == NULL )
	  || ( xdr_buffer == NULL ) )
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "One or more of the pointers passed to this function were NULL." );
	}
	if ( ( num_dimensions > 0 ) && ( dimension_array == NULL ) ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"dimension_array is NULL even though num_dimensions (%ld) "
		"is greater than 0.", num_dimensions );
	}

	/* Verify that the xdr_buffer pointer is correctly aligned
	 * on a 4 byte address.
	 */

	xdr_buffer_address = (unsigned long) xdr_buffer;

	address_remainder = xdr_buffer_address % 4;

	if ( address_remainder != 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The 'xdr_buffer' pointer %p passed to this routine is not "
		"correctly aligned.  XDR buffer pointers must have an address "
		"that is a multiple of 4 bytes, since misaligned buffer "
		"addresses can cause segmentation faults on for some "
		"computer CPU types such as SPARC and MIPS.  "
		"For safety's sake, we do not allow misaligned buffer "
		"addresses to be used.",
			xdr_buffer );
	}

	if ( direction == MX_XDR_ENCODE ) {
		operation = XDR_ENCODE;
	} else {
		operation = XDR_DECODE;
	}

	if ( num_bytes_copied != NULL ) {
		*num_bytes_copied = 0;
	}

	if ( num_dimensions < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The number of dimensions passed (%ld) is a negative number.",
			num_dimensions );
	}

	/* For datatypes that correspond to structures, we report the name
	 * of the structure, for example record->name, rather than something
	 * like a pointer to the data structure
	 */

	switch( mx_datatype ) {
	case MXFT_RECORD:
		if ( direction == MX_XDR_DECODE ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Writing to an MXFT_RECORD field is not allowed." );
		}

		return_structure_name = TRUE;
		break;
	case MXFT_RECORDTYPE:
		if ( direction == MX_XDR_DECODE ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Writing to an MXFT_RECORDTYPE field is not allowed." );
		}

		return_structure_name = TRUE;
		break;
	case MXFT_INTERFACE:
		if ( direction == MX_XDR_DECODE ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Writing to an MXFT_INTERFACE field is not allowed." );
		}

		return_structure_name = TRUE;
		break;
	case MXFT_RECORD_FIELD:
		if ( direction == MX_XDR_DECODE ) {
			return mx_error( MXE_UNSUPPORTED, fname,
		    "Writing to an MXFT_RECORD_FIELD field is not allowed." );
		}

		return_structure_name = TRUE;
		break;
	default:
		return_structure_name = FALSE;
		break;
	}

#if 0
	MX_DEBUG(-2,("%s: *** direction = %d", fname, direction));
	MX_DEBUG(-2,("%s: *** array_pointer = %p", fname, array_pointer));
	MX_DEBUG(-2,("%s: mx_datatype = %ld", fname, mx_datatype));
	MX_DEBUG(-2,("%s: num_dimensions = %ld", fname, num_dimensions));
	MX_DEBUG(-2,("%s: dimension_array = %p", fname, dimension_array));
	MX_DEBUG(-2,("%s: data_element_size_array = %p",
					fname, data_element_size_array));
	MX_DEBUG(-2,("%s: xdr_buffer = %p", fname, xdr_buffer));
	MX_DEBUG(-2,("%s: xdr_buffer_length = %ld",
					fname, (long)xdr_buffer_length));
#endif

	if ( num_dimensions == 0 ) {

		/* Handling scalars takes a bit more effort. */

		xdr_data_size = mx_xdr_get_scalar_element_size( mx_datatype );

		if ( direction == MX_XDR_ENCODE ) {
			if ( xdr_data_size > xdr_buffer_length ) {

				if ( num_bytes_copied != NULL ) {
					*num_bytes_copied =
					    xdr_data_size - xdr_buffer_length;
				}

				return mx_error(
				(MXE_WOULD_EXCEED_LIMIT | MXE_QUIET), fname,
			"The XDR scalar of size %ld bytes is too large "
			"to fit into the destination buffer of %ld bytes.",
					(long) xdr_data_size,
					(long) xdr_buffer_length );
			}
		} else {
			if ( xdr_buffer_length > xdr_data_size ) {

				if ( num_bytes_copied != NULL ) {
					*num_bytes_copied =
					    xdr_buffer_length - xdr_data_size;
				}

				return mx_error(
				(MXE_WOULD_EXCEED_LIMIT | MXE_QUIET), fname,
			"The source buffer of %ld bytes is too big to fit "
			"into the XDR scalar of size %ld bytes.",
					(long) xdr_buffer_length,
					(long) xdr_data_size );
			}
		}

		xdrmem_create( &xdrs,
				xdr_buffer,
				(u_int) xdr_buffer_length,
				operation );

		switch( mx_datatype ) {
		case MXFT_STRING:
			xdr_destroy( &xdrs );

			return mx_error( MXE_UNSUPPORTED, fname,
				"MX does not support 0-dimensional strings." );
		case MXFT_CHAR:
			xdr_status = xdr_char( &xdrs, array_pointer );
			break;
		case MXFT_UCHAR:
			xdr_status = xdr_u_char( &xdrs, array_pointer );
			break;
		case MXFT_SHORT:
			xdr_status = xdr_short( &xdrs, array_pointer );
			break;
		case MXFT_USHORT:
			xdr_status = xdr_u_short( &xdrs, array_pointer );
			break;
		case MXFT_BOOL:
			xdr_status = xdr_int( &xdrs, array_pointer );
			break;
		case MXFT_LONG:
			xdr_status = xdr_long( &xdrs, array_pointer );
			break;
		case MXFT_HEX:
		case MXFT_ULONG:
			xdr_status = xdr_u_long( &xdrs, array_pointer );
			break;
		case MXFT_INT64:
			xdr_status = xdr_hyper( &xdrs, array_pointer );
			break;
		case MXFT_UINT64:
			xdr_status = xdr_u_hyper( &xdrs, array_pointer );
			break;
		case MXFT_FLOAT:
			xdr_status = xdr_float( &xdrs, array_pointer );
			break;
		case MXFT_DOUBLE:
			xdr_status = xdr_double( &xdrs, array_pointer );
			break;

		case MXFT_RECORD:
			mx_record = (MX_RECORD *) array_pointer;

			ptr = mx_record->name;

			xdr_status = xdr_string( &xdrs, &ptr,
		    (u_int) mx_get_scalar_element_size(mx_datatype, FALSE));

			break;
		case MXFT_RECORDTYPE:
			mx_driver = (MX_DRIVER *) array_pointer;

			ptr = mx_driver->name;

			xdr_status = xdr_string( &xdrs, &ptr,
		    (u_int) mx_get_scalar_element_size(mx_datatype, FALSE));

			break;
		case MXFT_INTERFACE:
			mx_interface = (MX_INTERFACE *) array_pointer;

			ptr = mx_interface->address_name;

			xdr_status = xdr_string( &xdrs, &ptr,
		    (u_int) mx_get_scalar_element_size(mx_datatype, FALSE));

			break;
		case MXFT_RECORD_FIELD:
			mx_field = (MX_RECORD_FIELD *) array_pointer;

			if ( mx_field->record == (MX_RECORD *) NULL ) {
				snprintf( field_name, sizeof(field_name),
					"\?\?\?.%s", mx_field->name );
			} else {
				snprintf( field_name, sizeof(field_name),
					"%s.%s", mx_field->record->name,
						mx_field->name );
			}

			xdr_status = xdr_string( &xdrs, &ptr,
		    (u_int) mx_get_scalar_element_size(mx_datatype, FALSE));

			break;

		default:
			xdr_destroy( &xdrs );

			return mx_error( MXE_UNSUPPORTED, fname,
			"Array copy for data type %ld is not supported.",
				mx_datatype );
		}

		xdr_destroy( &xdrs );

		if ( xdr_status == 1 ) {
			if ( num_bytes_copied != NULL ) {
				*num_bytes_copied = xdr_data_size;
			}

			return MX_SUCCESSFUL_RESULT;
		} else {
			if ( num_bytes_copied != NULL ) {
				*num_bytes_copied = 0;
			}

			return mx_error( MXE_FUNCTION_FAILED, fname,
				"XDR scalar data copy of datatype %ld failed.",
					mx_datatype );
		}
	}

#define XDR_DO_ARRAY(xdr_filter) \
		xdr_status = xdr_array( &xdrs, &ptr, \
					&num_array_elements, \
					num_array_elements, \
					(u_int) data_element_size_array[0], \
					(xdrproc_t) (xdr_filter) )

	/* NOTE: 1-dimensional arrays of structures are reported as if they
	 * were 2-dimensional arrays of strings, where the strings are the
	 * 'names' of the structures, for example record->name.  Thus, the
	 * ( num_dimensions == 1 ) case below should never be invoked for
	 * complex structures.
	 */

	if ( ( num_dimensions == 1 ) && ( return_structure_name == FALSE ) ) {

		native_array_size = dimension_array[0]
					* data_element_size_array[0];

		xdr_array_size = dimension_array[0] 
				* mx_xdr_get_scalar_element_size(mx_datatype);

		num_array_elements = (u_int) (dimension_array[0]);

		xdr_array_size += 4;   /* Add space for the length field. */

		/* Round up xdr_array_size to the next multiple of 4. */

		remainder = xdr_array_size % 4;

		if ( remainder != 0 ) {
			xdr_array_size = 4 * ( 1 + xdr_array_size / 4 );
		}

		if ( direction == MX_XDR_ENCODE ) {
			if ( xdr_array_size > xdr_buffer_length ) {
#if 0
				mx_warning(
			"The 1-dimensional XDR array of size %ld bytes was "
			"truncated to fit into the buffer length of %ld bytes.",
					(long) xdr_array_size,
					(long) xdr_buffer_length );

				xdr_array_size = xdr_buffer_length;
#else
				if ( num_bytes_copied != NULL ) {
					*num_bytes_copied =
					    xdr_array_size - xdr_buffer_length;
				}
				return mx_error(
				(MXE_WOULD_EXCEED_LIMIT | MXE_QUIET), fname,
				"The 1-dimensional array of size %ld bytes is "
				"too large to fit into the destination buffer "
				"of %ld bytes.",
					(long) xdr_array_size,
					(long) xdr_buffer_length );
#endif
			}
		} else
		if ( direction == MX_XDR_DECODE ) {

			/* In MX, it is permitted to ask for fewer bytes
			 * than are actually sent by the server. However,
			 * the native XDR routines get very upset if the
			 * local buffer is not big enough to hold _all_
			 * of the bytes sent by the server.
			 *
			 * At present, MX deals with this by testing for
			 * this situation and overwriting XDR's array
			 * length count if it is longer than the array
			 * length of the local array.  This might be
			 * thought of as a kludge, but it is necessary
			 * to get compatibility with the behavior of
			 * MX when using RAW data format.
			 */

			uint32_t net_long_value;

			net_long_value = *( (uint32_t *) xdr_buffer );

			xdr_array_size = mx_ntohl( net_long_value );

#if 0
			MX_DEBUG( 2,
			("%s: dimension_array[0] = %lu, xdr_array_size = %lu",
				fname, dimension_array[0],
				(unsigned long) xdr_array_size));
#endif

			if ( xdr_array_size > dimension_array[0] ) {

				/* Overwrite the XDR array length. */

				net_long_value = mx_htonl( dimension_array[0] );

				*( (uint32_t *) xdr_buffer) = net_long_value;
			}
		}

		/* Create the XDR stream object. */

		xdrmem_create( &xdrs,
				xdr_buffer,
				(u_int) xdr_buffer_length,
				operation );

		ptr = (char *) array_pointer;

		if ( mx_datatype == MXFT_STRING ) {
			xdr_status = xdr_string( &xdrs, &ptr,
						(u_int) native_array_size );
		} else {
			switch( mx_datatype ) {
			case MXFT_CHAR:   XDR_DO_ARRAY(xdr_char);    break;
			case MXFT_UCHAR:  XDR_DO_ARRAY(xdr_u_char);  break;
			case MXFT_SHORT:  XDR_DO_ARRAY(xdr_short);   break;
			case MXFT_USHORT: XDR_DO_ARRAY(xdr_u_short); break;
			case MXFT_BOOL:   XDR_DO_ARRAY(xdr_int);     break;
			case MXFT_LONG:   XDR_DO_ARRAY(xdr_long);    break;
			case MXFT_ULONG:  XDR_DO_ARRAY(xdr_u_long);  break;
			case MXFT_HEX:    XDR_DO_ARRAY(xdr_u_long);  break;
			case MXFT_INT64:  XDR_DO_ARRAY(xdr_hyper);   break;
			case MXFT_UINT64: XDR_DO_ARRAY(xdr_u_hyper); break;
			case MXFT_FLOAT:  XDR_DO_ARRAY(xdr_float);   break;
			case MXFT_DOUBLE: XDR_DO_ARRAY(xdr_double);  break;
			default:
				xdr_destroy( &xdrs );

				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Illegal MX datatype %ld for %ld-dimensional XDR data transfer.",
					mx_datatype, num_dimensions );
			}

		}

		xdr_destroy( &xdrs );

		if ( xdr_status == 1 ) {
			if ( num_bytes_copied != NULL ) {
				*num_bytes_copied = xdr_array_size;
			}

			return MX_SUCCESSFUL_RESULT;
		} else {
			if ( num_bytes_copied != NULL ) {
				*num_bytes_copied = 0;
			}

			return mx_error( MXE_FUNCTION_FAILED, fname,
			"XDR 1-dimensional data copy of datatype %ld failed.",
					mx_datatype );
		}
	}

	/* num_dimensions > 1 */

	/* If we get here, either 'num_dimensions > 1' or this array is
	 * a structure array for which we report the name instead.
	 */

	/* Compute some shared values used to calculate both the
	 * native and XDR sizes of the array and subarrays.
	 *
	 * Note that we stop one step short of the last dimension.
	 * This is due to the fact that for the XDR size calculation
	 * we must adjust the size of the last dimension to take into
	 * account the 4 bytes necessary for the length field and the
	 * necessity to round the length up to an even multiple of
	 * 4 bytes.
	 */

	subarray_size_product = 1;

	for ( i = 1; i < num_dimensions - 1; i++ ) {
		subarray_size_product *= ( dimension_array[i] );
	}

	first_dimension_size = dimension_array[0];
	last_dimension_size  = dimension_array[num_dimensions - 1];

	/* Compute the native and XDR sizes of the array and subarrays. */

	if ( return_structure_name ) {
		/* This is a structure array. */

		last_dimension_native_size_in_bytes = 
			mx_get_scalar_element_size( mx_datatype, FALSE );

		last_dimension_xdr_size_in_bytes =
				mx_xdr_get_scalar_element_size( mx_datatype );
	} else {
		/* num_dimensions > 1 */

		last_dimension_native_size_in_bytes = last_dimension_size
						* data_element_size_array[0];

		last_dimension_xdr_size_in_bytes = last_dimension_size
			* mx_xdr_get_scalar_element_size( mx_datatype );
	
	}

	native_subarray_size = subarray_size_product
				* last_dimension_native_size_in_bytes;

	native_array_size = native_subarray_size * first_dimension_size;

	/* Add 4 for the length field. */

	last_dimension_xdr_size_in_bytes += 4;

	/* Round up the size to the next multiple of 4. */

	remainder = last_dimension_xdr_size_in_bytes % 4;

	if ( remainder != 0 ) {
		last_dimension_xdr_size_in_bytes =
			4 * ( 1 + last_dimension_xdr_size_in_bytes / 4 );
	}

	xdr_subarray_size = subarray_size_product
				* last_dimension_xdr_size_in_bytes;

	xdr_array_size = xdr_subarray_size * first_dimension_size;

#if 0
	MX_DEBUG(-2,("%s: last_dimension_size = %ld",
			fname, (long) last_dimension_size));

	MX_DEBUG(-2,("%s: last_dimension_native_size_in_bytes = %ld",
			fname, (long) last_dimension_native_size_in_bytes));

	MX_DEBUG(-2,("%s: last_dimension_xdr_size_in_bytes = %ld",
			fname, (long) last_dimension_xdr_size_in_bytes));

	MX_DEBUG(-2,("%s: xdr_subarray_size = %ld, xdr_array_size = %ld",
			fname, (long) xdr_subarray_size, (long)xdr_array_size));
#endif

	if ( direction == MX_XDR_ENCODE ) {
		if ( xdr_array_size > xdr_buffer_length ) {
#if 0
			mx_warning(
			"The %ld-dimensional XDR array of size %ld bytes was "
			"truncated to %ld bytes.",
				num_dimensions,
				(long) xdr_array_size,
				(long) xdr_buffer_length );

			xdr_array_size = xdr_buffer_length;
#else
			if ( num_bytes_copied != NULL ) {
				*num_bytes_copied =
					xdr_array_size - xdr_buffer_length;
			}
			return mx_error(
				(MXE_WOULD_EXCEED_LIMIT | MXE_QUIET), fname,
				"The %ld-dimensional array of size %ld bytes "
				"is too large to fit into the destination "
				"buffer of %ld bytes.",
					num_dimensions,
					(long) xdr_array_size,
					(long) xdr_buffer_length );
#endif
		}
	}

	xdrmem_create( &xdrs,
			xdr_buffer,
			(u_int) xdr_buffer_length,
			operation );

	for ( n = 0; n < dimension_array[0]; n++ ) {

		array_row_pointer = (char *) array_pointer
			    + n * data_element_size_array[num_dimensions - 1];

		if ( array_is_dynamically_allocated ) {
			array_row_pointer = (char *)
				mx_read_void_pointer_from_memory_location(
					array_row_pointer );
		}

		xdr_ptr = (char *) xdr_buffer + n * xdr_subarray_size;

		buffer_left = xdr_buffer_length - n * xdr_subarray_size;

		mx_status = mx_xdr_data_transfer( direction,
				array_row_pointer,
				array_is_dynamically_allocated,
				mx_datatype, num_dimensions - 1,
				&dimension_array[1], data_element_size_array,
				xdr_ptr, buffer_left,
				NULL );

		if ( mx_status.code != MXE_SUCCESS ) {
			xdr_destroy( &xdrs );

			return mx_status;
		}
	}

	if ( num_bytes_copied != NULL ) {
		*num_bytes_copied = xdr_array_size;
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_XDR */

/* mx_convert_and_copy_array() is designed to copy from one MX-compatible
 * array to another MX-compatible array and convert from one datatype to
 * another datatype along the way.
 *
 * FIXME - Very little of this function has been implemented as of yet.
 *         The only supported cases at the moment are 0-dimensional to
 *         1-dimensional and 1-dimensional to 0-dimensional.  Only ints
 *         and doubles are currently supported.
 *
 *         At the moment, only enough of this function is implemented as
 *         is needed to support the 'record_field_motor' driver.
 */

MX_EXPORT mx_status_type
mx_convert_and_copy_array(
	void *source_array_pointer,
	long source_datatype,
	long source_num_dimensions,
	long *source_dimension_array,
	size_t *source_data_element_size_array,
	void *destination_array_pointer,
	long destination_datatype,
	long destination_num_dimensions,
	long *destination_dimension_array,
	size_t *destination_data_element_size_array )
{
	static const char fname[] = "mx_convert_and_copy_array()";

	char char_source_value;
	unsigned char uchar_source_value;
	short short_source_value;
	unsigned short ushort_source_value;
	mx_bool_type bool_source_value;
	long long_source_value;
	unsigned long ulong_source_value;
	float float_source_value;
	double double_source_value;
	char *char_dest_ptr;
	unsigned char *uchar_dest_ptr;
	short *short_dest_ptr;
	unsigned short *ushort_dest_ptr;
	mx_bool_type *bool_dest_ptr;
	long *long_dest_ptr;
	unsigned long *ulong_dest_ptr;
	float *float_dest_ptr;
	double *double_dest_ptr;

#if 0
	MX_DEBUG(-2,("%s: source_array_pointer = %p, "
		"source_datatype = %ld, source_num_dimensions = %ld",
			fname, source_array_pointer,
			source_datatype, source_num_dimensions));

	MX_DEBUG(-2,("%s: destination_array_pointer = %p, "
		"destination_datatype = %ld, destination_num_dimensions = %ld",
			fname, destination_array_pointer,
			destination_datatype, destination_num_dimensions));
#endif

	if ( ( source_num_dimensions == 0 )
	  && ( destination_num_dimensions == 0 ) )
	{
		/* This case is supported. */
	} else
	if ( ( source_num_dimensions == 0 )
	  && ( destination_num_dimensions == 1 ) )
	{
		/* This case is supported. */
	} else
	if ( ( source_num_dimensions == 1 )
	  && ( destination_num_dimensions == 0 ) )
	{
		/* This case is supported. */
	} else {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Copying from a %ld-dimensional array to a %ld-dimensional "
		"array is not yet implemented.",
			source_num_dimensions, destination_num_dimensions );
	}

	switch( source_datatype ) {
	case MXFT_CHAR:
		char_source_value = *((char *) source_array_pointer);

		switch( destination_datatype ) {
		case MXFT_CHAR:
			char_dest_ptr = (char *) destination_array_pointer;

			*char_dest_ptr = char_source_value;
			break;

		case MXFT_UCHAR:
			uchar_dest_ptr = (unsigned char *)
						destination_array_pointer;

			*uchar_dest_ptr = char_source_value;
			break;

		case MXFT_SHORT:
			short_dest_ptr = (short *) destination_array_pointer;

			*short_dest_ptr = char_source_value;
			break;

		case MXFT_USHORT:
			ushort_dest_ptr = (unsigned short *)
						destination_array_pointer;

			*ushort_dest_ptr = char_source_value;
			break;

		case MXFT_BOOL:
			bool_dest_ptr = (mx_bool_type *)
						destination_array_pointer;

			*bool_dest_ptr = char_source_value;
			break;

		case MXFT_LONG:
			long_dest_ptr = (long *) destination_array_pointer;

			*long_dest_ptr = char_source_value;
			break;

		case MXFT_ULONG:
		case MXFT_HEX:
			ulong_dest_ptr = (unsigned long *)
						destination_array_pointer;

			*ulong_dest_ptr = char_source_value;
			break;

		case MXFT_FLOAT:
			float_dest_ptr = (float *) destination_array_pointer;

			*float_dest_ptr = (float) char_source_value;
			break;

		case MXFT_DOUBLE:
			double_dest_ptr = (double *) destination_array_pointer;

			*double_dest_ptr = (double) char_source_value;
			break;

		default:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		    "MX datatype %ld is not yet implemented for this function.",
				destination_datatype );
			break;
		}
		break;

	case MXFT_UCHAR:
		uchar_source_value = *((unsigned char *) source_array_pointer);

		switch( destination_datatype ) {
		case MXFT_CHAR:
			char_dest_ptr = (char *) destination_array_pointer;

			*char_dest_ptr = uchar_source_value;
			break;

		case MXFT_UCHAR:
			uchar_dest_ptr = (unsigned char *)
						destination_array_pointer;

			*uchar_dest_ptr = uchar_source_value;
			break;

		case MXFT_SHORT:
			short_dest_ptr = (short *) destination_array_pointer;

			*short_dest_ptr = uchar_source_value;
			break;

		case MXFT_USHORT:
			ushort_dest_ptr = (unsigned short *)
						destination_array_pointer;

			*ushort_dest_ptr = uchar_source_value;
			break;

		case MXFT_BOOL:
			bool_dest_ptr = (mx_bool_type *)
						destination_array_pointer;

			*bool_dest_ptr = uchar_source_value;
			break;

		case MXFT_LONG:
			long_dest_ptr = (long *) destination_array_pointer;

			*long_dest_ptr = uchar_source_value;
			break;

		case MXFT_ULONG:
		case MXFT_HEX:
			ulong_dest_ptr = (unsigned long *)
						destination_array_pointer;

			*ulong_dest_ptr = uchar_source_value;
			break;

		case MXFT_FLOAT:
			float_dest_ptr = (float *) destination_array_pointer;

			*float_dest_ptr = (float) uchar_source_value;
			break;

		case MXFT_DOUBLE:
			double_dest_ptr = (double *) destination_array_pointer;

			*double_dest_ptr = (double) uchar_source_value;
			break;

		default:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		    "MX datatype %ld is not yet implemented for this function.",
				destination_datatype );
			break;
		}
		break;

	case MXFT_SHORT:
		short_source_value = *((short *) source_array_pointer);

		switch( destination_datatype ) {
		case MXFT_CHAR:
			char_dest_ptr = (char *) destination_array_pointer;

			*char_dest_ptr = short_source_value;
			break;

		case MXFT_UCHAR:
			uchar_dest_ptr = (unsigned char *)
						destination_array_pointer;

			*uchar_dest_ptr = short_source_value;
			break;

		case MXFT_SHORT:
			short_dest_ptr = (short *) destination_array_pointer;

			*short_dest_ptr = short_source_value;
			break;

		case MXFT_USHORT:
			ushort_dest_ptr = (unsigned short *)
						destination_array_pointer;

			*ushort_dest_ptr = short_source_value;
			break;

		case MXFT_BOOL:
			bool_dest_ptr = (mx_bool_type *)
						destination_array_pointer;

			*bool_dest_ptr = short_source_value;
			break;

		case MXFT_LONG:
			long_dest_ptr = (long *) destination_array_pointer;

			*long_dest_ptr = short_source_value;
			break;

		case MXFT_ULONG:
		case MXFT_HEX:
			ulong_dest_ptr = (unsigned long *)
						destination_array_pointer;

			*ulong_dest_ptr = short_source_value;
			break;

		case MXFT_FLOAT:
			float_dest_ptr = (float *) destination_array_pointer;

			*float_dest_ptr = (float) short_source_value;
			break;

		case MXFT_DOUBLE:
			double_dest_ptr = (double *) destination_array_pointer;

			*double_dest_ptr = (double) short_source_value;
			break;

		default:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		    "MX datatype %ld is not yet implemented for this function.",
				destination_datatype );
			break;
		}
		break;

	case MXFT_USHORT:
		ushort_source_value = *((unsigned short *)source_array_pointer);

		switch( destination_datatype ) {
		case MXFT_CHAR:
			char_dest_ptr = (char *) destination_array_pointer;

			*char_dest_ptr = ushort_source_value;
			break;

		case MXFT_UCHAR:
			uchar_dest_ptr = (unsigned char *)
						destination_array_pointer;

			*uchar_dest_ptr = ushort_source_value;
			break;

		case MXFT_SHORT:
			short_dest_ptr = (short *) destination_array_pointer;

			*short_dest_ptr = ushort_source_value;
			break;

		case MXFT_USHORT:
			ushort_dest_ptr = (unsigned short *)
						destination_array_pointer;

			*ushort_dest_ptr = ushort_source_value;
			break;

		case MXFT_BOOL:
			bool_dest_ptr = (mx_bool_type *)
						destination_array_pointer;

			*bool_dest_ptr = ushort_source_value;
			break;

		case MXFT_LONG:
			long_dest_ptr = (long *) destination_array_pointer;

			*long_dest_ptr = ushort_source_value;
			break;

		case MXFT_ULONG:
		case MXFT_HEX:
			ulong_dest_ptr = (unsigned long *)
						destination_array_pointer;

			*ulong_dest_ptr = ushort_source_value;
			break;

		case MXFT_FLOAT:
			float_dest_ptr = (float *) destination_array_pointer;

			*float_dest_ptr = (float) ushort_source_value;
			break;

		case MXFT_DOUBLE:
			double_dest_ptr = (double *) destination_array_pointer;

			*double_dest_ptr = (double) ushort_source_value;
			break;

		default:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		    "MX datatype %ld is not yet implemented for this function.",
				destination_datatype );
			break;
		}
		break;

	case MXFT_BOOL:
		bool_source_value = *((mx_bool_type *) source_array_pointer);

		switch( destination_datatype ) {
		case MXFT_CHAR:
			char_dest_ptr = (char *) destination_array_pointer;

			*char_dest_ptr = bool_source_value;
			break;

		case MXFT_UCHAR:
			uchar_dest_ptr = (unsigned char *)
						destination_array_pointer;

			*uchar_dest_ptr = bool_source_value;
			break;

		case MXFT_SHORT:
			short_dest_ptr = (short *) destination_array_pointer;

			*short_dest_ptr = bool_source_value;
			break;

		case MXFT_USHORT:
			ushort_dest_ptr = (unsigned short *)
						destination_array_pointer;

			*ushort_dest_ptr = bool_source_value;
			break;

		case MXFT_BOOL:
			bool_dest_ptr = (mx_bool_type *)
						destination_array_pointer;

			*bool_dest_ptr = bool_source_value;
			break;

		case MXFT_LONG:
			long_dest_ptr = (long *) destination_array_pointer;

			*long_dest_ptr = bool_source_value;
			break;

		case MXFT_ULONG:
		case MXFT_HEX:
			ulong_dest_ptr = (unsigned long *)
						destination_array_pointer;

			*ulong_dest_ptr = bool_source_value;
			break;

		case MXFT_FLOAT:
			float_dest_ptr = (float *) destination_array_pointer;

			*float_dest_ptr = (float) bool_source_value;
			break;

		case MXFT_DOUBLE:
			double_dest_ptr = (double *) destination_array_pointer;

			*double_dest_ptr = (double) bool_source_value;
			break;

		default:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		    "MX datatype %ld is not yet implemented for this function.",
				destination_datatype );
			break;
		}
		break;

	case MXFT_LONG:
		long_source_value = *((long *) source_array_pointer);

		switch( destination_datatype ) {
		case MXFT_CHAR:
			char_dest_ptr = (char *) destination_array_pointer;

			*char_dest_ptr = (char) long_source_value;
			break;

		case MXFT_UCHAR:
			uchar_dest_ptr = (unsigned char *)
						destination_array_pointer;

			*uchar_dest_ptr = (unsigned char) long_source_value;
			break;

		case MXFT_SHORT:
			short_dest_ptr = (short *) destination_array_pointer;

			*short_dest_ptr = (short) long_source_value;
			break;

		case MXFT_USHORT:
			ushort_dest_ptr = (unsigned short *)
						destination_array_pointer;

			*ushort_dest_ptr = (unsigned short) long_source_value;
			break;

		case MXFT_BOOL:
			bool_dest_ptr = (mx_bool_type *)
						destination_array_pointer;

			*bool_dest_ptr = long_source_value;
			break;

		case MXFT_LONG:
			long_dest_ptr = (long *) destination_array_pointer;

			*long_dest_ptr = long_source_value;
			break;

		case MXFT_ULONG:
		case MXFT_HEX:
			ulong_dest_ptr = (unsigned long *)
						destination_array_pointer;

			*ulong_dest_ptr = long_source_value;
			break;

		case MXFT_FLOAT:
			float_dest_ptr = (float *) destination_array_pointer;

			*float_dest_ptr = (float) long_source_value;
			break;

		case MXFT_DOUBLE:
			double_dest_ptr = (double *) destination_array_pointer;

			*double_dest_ptr = (double) long_source_value;
			break;

		default:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		    "MX datatype %ld is not yet implemented for this function.",
				destination_datatype );
			break;
		}
		break;

	case MXFT_ULONG:
	case MXFT_HEX:
		ulong_source_value = *((unsigned long *) source_array_pointer);

		switch( destination_datatype ) {
		case MXFT_CHAR:
			char_dest_ptr = (char *) destination_array_pointer;

			*char_dest_ptr = (char) ulong_source_value;
			break;

		case MXFT_UCHAR:
			uchar_dest_ptr = (unsigned char *)
						destination_array_pointer;

			*uchar_dest_ptr = (unsigned char) ulong_source_value;
			break;

		case MXFT_SHORT:
			short_dest_ptr = (short *) destination_array_pointer;

			*short_dest_ptr = (short) ulong_source_value;
			break;

		case MXFT_USHORT:
			ushort_dest_ptr = (unsigned short *)
						destination_array_pointer;

			*ushort_dest_ptr = (unsigned short) ulong_source_value;
			break;

		case MXFT_BOOL:
			bool_dest_ptr = (mx_bool_type *)
						destination_array_pointer;

			*bool_dest_ptr = (mx_bool_type) ulong_source_value;
			break;

		case MXFT_LONG:
			long_dest_ptr = (long *) destination_array_pointer;

			*long_dest_ptr = (long) ulong_source_value;
			break;

		case MXFT_ULONG:
		case MXFT_HEX:
			ulong_dest_ptr = (unsigned long *)
						destination_array_pointer;

			*ulong_dest_ptr = ulong_source_value;
			break;

		case MXFT_FLOAT:
			float_dest_ptr = (float *) destination_array_pointer;

			*float_dest_ptr = (float) ulong_source_value;
			break;

		case MXFT_DOUBLE:
			double_dest_ptr = (double *) destination_array_pointer;

			*double_dest_ptr = (double) ulong_source_value;
			break;

		default:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		    "MX datatype %ld is not yet implemented for this function.",
				destination_datatype );
			break;
		}
		break;

	case MXFT_FLOAT:
		float_source_value = *((float *) source_array_pointer);

		switch( destination_datatype ) {
		case MXFT_CHAR:
			char_dest_ptr = (char *) destination_array_pointer;

			*char_dest_ptr = (char) mx_round( float_source_value );
			break;

		case MXFT_UCHAR:
			uchar_dest_ptr = (unsigned char *)
						destination_array_pointer;

			*uchar_dest_ptr = (unsigned char)
						mx_round( float_source_value );
			break;

		case MXFT_SHORT:
			short_dest_ptr = (short *) destination_array_pointer;

			*short_dest_ptr = (short)
						mx_round( float_source_value );
			break;

		case MXFT_USHORT:
			ushort_dest_ptr = (unsigned short *)
						destination_array_pointer;

			*ushort_dest_ptr = (unsigned short)
						mx_round( float_source_value );
			break;

		case MXFT_BOOL:
			bool_dest_ptr = (mx_bool_type *)
						destination_array_pointer;

			*bool_dest_ptr = mx_round( float_source_value );
			break;

		case MXFT_LONG:
			long_dest_ptr = (long *) destination_array_pointer;

			*long_dest_ptr = mx_round( float_source_value );
			break;

		case MXFT_ULONG:
		case MXFT_HEX:
			ulong_dest_ptr = (unsigned long *)
						destination_array_pointer;

			*ulong_dest_ptr = mx_round( float_source_value );
			break;

		case MXFT_FLOAT:
			float_dest_ptr = (float *) destination_array_pointer;

			*float_dest_ptr = float_source_value;
			break;

		case MXFT_DOUBLE:
			double_dest_ptr = (double *) destination_array_pointer;

			*double_dest_ptr = float_source_value;
			break;

		default:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		    "MX datatype %ld is not yet implemented for this function.",
				destination_datatype );
			break;
		}
		break;

	case MXFT_DOUBLE:
		double_source_value = *((double *) source_array_pointer);

		switch( destination_datatype ) {
		case MXFT_CHAR:
			char_dest_ptr = (char *) destination_array_pointer;

			*char_dest_ptr = (char) mx_round( double_source_value );
			break;

		case MXFT_UCHAR:
			uchar_dest_ptr = (unsigned char *)
						destination_array_pointer;

			*uchar_dest_ptr = (unsigned char)
						mx_round( double_source_value );
			break;

		case MXFT_SHORT:
			short_dest_ptr = (short *) destination_array_pointer;

			*short_dest_ptr = (short)
						mx_round( double_source_value );
			break;

		case MXFT_USHORT:
			ushort_dest_ptr = (unsigned short *)
						destination_array_pointer;

			*ushort_dest_ptr = (unsigned short)
						mx_round( double_source_value );
			break;

		case MXFT_BOOL:
			bool_dest_ptr = (mx_bool_type *)
						destination_array_pointer;

			*bool_dest_ptr = mx_round( double_source_value );
			break;

		case MXFT_LONG:
			long_dest_ptr = (long *) destination_array_pointer;

			*long_dest_ptr = mx_round( double_source_value );
			break;

		case MXFT_ULONG:
		case MXFT_HEX:
			ulong_dest_ptr = (unsigned long *)
						destination_array_pointer;

			*ulong_dest_ptr = mx_round( double_source_value );
			break;

		case MXFT_FLOAT:
			float_dest_ptr = (float *) destination_array_pointer;

			*float_dest_ptr = double_source_value;
			break;

		case MXFT_DOUBLE:
			double_dest_ptr = (double *) destination_array_pointer;

			*double_dest_ptr = double_source_value;
			break;

		default:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		    "MX datatype %ld is not yet implemented for this function.",
				destination_datatype );
			break;
		}
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		    "MX datatype %ld is not yet implemented for this function.",
			source_datatype );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

#if MX_ARRAY_DEBUG_OVERLAY

/* TEST CODE - TEST CODE - TEST CODE - TEST CODE - TEST CODE */

/* The following function is used for testing array overlays
 * and should not be used in normal code.
 */

#include "mx_image.h"

MX_API mx_status_type mx_array_debug_overlay( MX_IMAGE_FRAME * );

MX_EXPORT mx_status_type
mx_array_debug_overlay( MX_IMAGE_FRAME *frame )
{
	static const char fname[] = "mx_array_debug_overlay()";

	uint16_t **array_pointer;
	uint16_t *image_data;
	long dimension_array[2];
	size_t element_size_array[2];
	unsigned long frame_height, frame_width;
	int i, j, n;
	unsigned long value;
	mx_status_type mx_status;

	frame_height = MXIF_COLUMN_FRAMESIZE(frame);
	frame_width  = MXIF_ROW_FRAMESIZE(frame);

	MX_DEBUG(-2,("%s: frame_width = %lu, frame_height = %lu",
		fname, frame_width, frame_height));

	dimension_array[0] = frame_height;
	dimension_array[1] = frame_width;

	element_size_array[0] = sizeof(uint16_t);
	element_size_array[1] = sizeof(uint16_t *);

	mx_status = mx_array_add_overlay( frame->image_data,
					2, dimension_array,
					element_size_array,
					(void **) &array_pointer );

	if ( mx_status.code == MXE_SUCCESS ) {
		for ( i = 0; i < frame_height; i++ ) {
			for ( j = 0; j < frame_width; j++ ) {
				value = 1000 * i + j;
				value %= 10000;

				array_pointer[i][j] = value;

				MX_DEBUG(-2,
				("%s: array_pointer[%d][%d] = %d",
					fname, i, j,
					array_pointer[i][j]));
			}
		}

		image_data = frame->image_data;

		for ( i = 0; i < frame_height; i++ ) {
		    for ( j = 0; j < frame_width; j++ ) {
			n = i * frame_width + j;

			value = image_data[n];

			MX_DEBUG(-2,("%s: image_data[%d] = %ld",
				fname, n, value ));

			if ( value != array_pointer[i][j] ) {
			    MX_DEBUG(-2,
		("%s: value = %lu does not match array[%d][%d] = %d",
			    fname, value, i, j, array_pointer[i][j]));
			}
		    }
		}

	}

	return mx_status;
}

#endif /* MX_ARRAY_DEBUG_OVERLAY */

