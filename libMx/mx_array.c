/*
 * Name:    mx_array.c
 *
 * Purpose: MX array handling functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_ARRAY_DEBUG_OVERLAY	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_osdef.h"
#include "mx_stdint.h"
#include "mx_socket.h"
#include "mx_array.h"

#if HAVE_XDR
#  include "mx_xdr.h"
#endif

/*
 * Re: definition of ADD_STRING_BYTE
 *
 * You should _not_ add an extra byte to string fields since it makes all
 * of the length calculations more difficult.  In particular, it makes
 * calculation of the string length in Python more awkward.  If you want
 * an extra byte, make the original string buffer longer.
 */

#define ADD_STRING_BYTE    FALSE

MX_EXPORT void *
mx_allocate_array( long num_dimensions,
		long *dimension_array,
		size_t *data_element_size_array )
{
	static const char fname[] = "mx_allocate_array()";

	void *array_pointer;
	char *array_element_pointer;
	void *subarray_pointer;
	long array_size;
	long i, j;
	long current_dimension_element_size;
	mx_status_type mx_status;

	if ( num_dimensions <= 0 ) {
		mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal number of dimensions = %ld", num_dimensions );

		return NULL;
	}

	if ( dimension_array == (long *) NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"dimension_array pointer passed is NULL." );

		return NULL;
	}

	if ( data_element_size_array == (size_t *) NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"data_element_size_array pointer passed is NULL." );

		return NULL;
	}

	MX_DEBUG( 8,("%s: num_dimensions = %ld", fname, num_dimensions));

	for ( i = 0; i < num_dimensions; i++ ) {
		MX_DEBUG( 8,(
			"%s:   dimension[%ld] = %ld, element size[%ld] = %ld",
			fname, i, dimension_array[i],
			i, (long) data_element_size_array[i]));
	}

	current_dimension_element_size
			= (long) (data_element_size_array[num_dimensions - 1]);

	array_size = dimension_array[0] * current_dimension_element_size;

	MX_DEBUG( 8,("%s: array_size = %ld", fname, array_size));

	if ( array_size <= 0 ) {
		array_pointer = NULL;
	} else {
#if 1
		array_pointer = (char **) malloc( array_size );
#else
		array_pointer = (char **) calloc( array_size, 1 );
#endif
	}

	MX_DEBUG( 8,("%s: array_pointer = %p", fname, array_pointer));

	if ( array_pointer == NULL ) {
		(void) mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory trying to allocate %ld elements of size %ld.",
			dimension_array[0],
			(long) current_dimension_element_size );
		return NULL;
	}

	if ( num_dimensions > 1 ) {
		for ( i = 0; i < dimension_array[0]; i++ ) {
			array_element_pointer = (char *)array_pointer
					+ i * current_dimension_element_size;

			subarray_pointer = mx_allocate_array(
				num_dimensions - 1,
				&dimension_array[1],
				data_element_size_array );

			MX_DEBUG( 8,
		("%s: array_element_pointer = %p, subarray_pointer = %p",
			fname, array_element_pointer, subarray_pointer));

			/* If the allocation of this part of the array
			 * failed, attempt to free all the parts of
			 * the array that we have already allocated.
			 */

			if ( subarray_pointer == NULL ) {
				for ( j = 0; j < i; j++ ) {
					array_element_pointer
						= (char *)array_pointer
					+ j * current_dimension_element_size;

					subarray_pointer =
				mx_read_void_pointer_from_memory_location(
						array_element_pointer );

					mx_status = mx_free_array(
						subarray_pointer,
						num_dimensions - 1,
						&dimension_array[1],
						data_element_size_array );

					/* If we can't even free memory,
					 * then there's not much else we
					 * can do but give up.
					 */

					if ( mx_status.code != MXE_SUCCESS )
						return NULL;
				}

				return NULL;
			}
			mx_write_void_pointer_to_memory_location(
				array_element_pointer, subarray_pointer );
		}
	}

	return array_pointer;
}

MX_EXPORT mx_status_type
mx_free_array( void *array_pointer,
		long num_dimensions,
		long *dimension_array,
		size_t *data_element_size_array )
{
	static const char fname[] = "mx_free_array()";

	char *array_element_pointer;
	void *subarray_pointer;
	long i;
	size_t current_dimension_element_size;
	mx_status_type mx_status;

	if ( array_pointer == NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"array_pointer passed is NULL." );
	}

	if ( num_dimensions <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal number of dimensions = %ld", num_dimensions );
	}

	if ( dimension_array == (long *) NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"dimension_array pointer passed is NULL." );
	}

	if ( data_element_size_array == (size_t *) NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"data_element_size_array pointer passed is NULL." );
	}

	MX_DEBUG( 8,("%s: num_dimensions = %ld", fname, num_dimensions));

	MX_DEBUG( 8,("%s: array_pointer = %p", fname, array_pointer));

	for ( i = 0; i < num_dimensions; i++ ) {
		MX_DEBUG( 8,(
			"%s:   dimension[%ld] = %ld, element size[%ld] = %ld",
			fname, i, dimension_array[i],
			i, (long) data_element_size_array[i]));
	}

	current_dimension_element_size
				= data_element_size_array[num_dimensions - 1];

	if ( num_dimensions == 1 ) {
		if ( array_pointer == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"array_pointer at address %p is NULL.",
				&array_pointer );
		}

		/* Finally, let's free the memory pointed to by the pointer. */

		MX_DEBUG( 8,( "%s: About to free pointer array_pointer = %p",
			fname, array_pointer ));

		free( array_pointer );

		/* Set the pointer to NULL, so that we will know that it
		 * contains an invalid pointer value.
		 */

		array_pointer = NULL;

		/* free() is defined to have a void return value in ANSI C,
		 * so there really isn't any way to confirm that things
		 * worked correctly.
		 */
	} else {
		for ( i = 0; i < dimension_array[0]; i++ ) {
			array_element_pointer = (char *)array_pointer
					+ i * current_dimension_element_size;

			if ( array_element_pointer == NULL ) {
				return mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname, 
				"array_element_pointer at address %p is NULL.",
					&array_element_pointer );
			}

			subarray_pointer
				= mx_read_void_pointer_from_memory_location(
						array_element_pointer );

			if ( subarray_pointer == NULL ) {
				return mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname, 
				"subarray_pointer at address %p is NULL.",
					&subarray_pointer );
			}

			MX_DEBUG( 8,
		("%s: array_element_pointer = %p, subarray_pointer = %p",
			fname, array_element_pointer, subarray_pointer ));

			mx_status = mx_free_array( subarray_pointer,
				num_dimensions - 1, &dimension_array[1],
				data_element_size_array );

			/*If freeing the subarray didn't work, then give up.*/

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

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

	MX_DEBUG( 9, ("readvp: read pointer %p from memory location %p",
				result, ptr));

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

	MX_DEBUG( 9, ("writevp: wrote pointer %p to memory location %p",
					ptr, memory_location));

	return;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_get_array_size( long num_dimensions,
		long *dimension_array,
		size_t *data_element_size_array,
		size_t *array_size )
{
	static const char fname[] = "mx_get_array_size()";

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
			mx_bool_type truncate_64bit_longs )
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
		if ( truncate_64bit_longs ) {
		              element_size = 4;
		} else {
		              element_size = sizeof(long);
		}
		break;

	case MXFT_ULONG:       
		if ( truncate_64bit_longs ) {
		              element_size = 4;
		} else {
		              element_size = sizeof(unsigned long);
		}
		break;

	case MXFT_RECORD:     element_size = MXU_RECORD_NAME_LENGTH;      break;
	case MXFT_RECORDTYPE: element_size = MXU_DRIVER_NAME_LENGTH;      break;
	case MXFT_INTERFACE:  element_size = MXU_INTERFACE_NAME_LENGTH;   break;
	default:              element_size = 0;                           break;
	}

#if 0
	if ( truncate_64bit_longs ) {
		MX_DEBUG(-2,
	    ("mxp_scaler_element_size(): mx_datatype = %ld, element_size = %ld",
	    		mx_datatype, (long) element_size ));
	}
#endif

	return element_size;
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

	array_of_level_pointers = calloc( num_dimensions, sizeof(void *) );

	if ( array_of_level_pointers == (void **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of level pointers.", num_dimensions - 1 );
	}

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

	free( array_of_level_pointers );

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

static void
mx_copy_32bits_to_64bits( void *destination, void *source, size_t num_elements )
{
	uint64_t *uint64_ptr;
	uint32_t *uint32_ptr;
	size_t i;

#if 0
	MX_DEBUG(-2,("mx_copy_32bits_to_64bits: num_elements = %ld",
		(long) num_elements ));
#endif

	uint64_ptr = destination;
	uint32_ptr = source;

	for ( i = 0; i < num_elements; i++ ) {
		uint64_ptr[i] = (uint64_t) uint32_ptr[i];
	}

	return;
}

static void
mx_copy_64bits_to_32bits( void *destination, void *source, size_t num_elements )
{
	uint64_t *uint64_ptr;
	uint32_t *uint32_ptr;
	size_t i;

#if 0
	MX_DEBUG(-2,("mx_copy_64bits_to_32bits: num_elements = %ld",
		(long) num_elements ));
#endif

	uint32_ptr = destination;
	uint64_ptr = source;

	for ( i = 0; i < num_elements; i++ ) {
		uint32_ptr[i] = (uint32_t) uint64_ptr[i];
	}

	return;
}

MX_EXPORT mx_status_type
mx_copy_array_to_buffer( void *array_pointer,
		mx_bool_type array_is_dynamically_allocated,
		long mx_datatype, long num_dimensions,
		long *dimension_array, size_t *data_element_size_array,
		void *destination_buffer, size_t destination_buffer_length,
		size_t *num_bytes_copied,
		mx_bool_type truncate_64bit_longs )
{
	static const char fname[] = "mx_copy_array_to_buffer()";

	MX_RECORD *mx_record;
	MX_DRIVER *mx_driver;
	MX_INTERFACE *mx_interface;
	char *array_row_pointer, *destination_pointer;
	size_t bytes_to_copy, array_size, subarray_size, buffer_left;
	long i, n;
	int num_subarray_elements;
	int return_structure_name, structure_name_length;
	mx_status_type mx_status;

	if (( array_pointer == NULL ) || ( dimension_array == NULL )
	 || (data_element_size_array == NULL) || (destination_buffer == NULL))
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "One or more of the pointers passed to this function were NULL." );
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
		structure_name_length = MXU_RECORD_NAME_LENGTH + 1;
		return_structure_name = TRUE;
		break;
	case MXFT_RECORDTYPE:
		structure_name_length = MXU_DRIVER_NAME_LENGTH + 1;
		return_structure_name = TRUE;
		break;
	case MXFT_INTERFACE:
		structure_name_length = MXU_INTERFACE_NAME_LENGTH + 1;
		return_structure_name = TRUE;
		break;
	default:
		structure_name_length = 0;
		return_structure_name = FALSE;
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

	if ( num_dimensions == 0 ) {

		/* Handling scalars takes a bit more effort. */

		bytes_to_copy = mx_get_scalar_element_size( mx_datatype,
							truncate_64bit_longs );

		if ( bytes_to_copy > destination_buffer_length ) {
			if ( num_bytes_copied != NULL ) {
				*num_bytes_copied =
				    bytes_to_copy - destination_buffer_length;
			}
			return mx_error(
				(MXE_WOULD_EXCEED_LIMIT | MXE_QUIET), fname,
				"The scaler of size %ld bytes is too large to "
				"fit into the destination buffer of %ld bytes.",
					(long) bytes_to_copy,
					(long) destination_buffer_length );
		}

#if 0
		MX_DEBUG(-2,
	("%s: num_dimensions = 0, mx_datatype = %ld, bytes_to_copy = %d",
			fname, mx_datatype, bytes_to_copy));
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
					array_pointer, bytes_to_copy );
			break;

		case MXFT_HEX:
		case MXFT_LONG:
		case MXFT_ULONG:
			if ( truncate_64bit_longs ) {
				mx_copy_64bits_to_32bits( destination_buffer,
					array_pointer, 1 );
			} else {
				memcpy( destination_buffer,
					array_pointer, bytes_to_copy );
			}
			break;

		case MXFT_RECORD:
			mx_record = (MX_RECORD *) array_pointer;

			destination_pointer = (char *) destination_buffer;

			strlcpy( destination_pointer, mx_record->name,
					MXU_RECORD_NAME_LENGTH );
			break;
		case MXFT_RECORDTYPE:
			mx_driver = (MX_DRIVER *) array_pointer;

			destination_pointer = (char *) destination_buffer;

			strlcpy( destination_pointer, mx_driver->name,
					MXU_DRIVER_NAME_LENGTH );
			break;
		case MXFT_INTERFACE:
			mx_interface = (MX_INTERFACE *) array_pointer;

			destination_pointer = (char *) destination_buffer;

			strlcpy( destination_pointer,
					mx_interface->address_name,
					MXU_INTERFACE_ADDRESS_NAME_LENGTH );
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Array copy for data type %ld is not supported.",
				mx_datatype );
		}

		if ( num_bytes_copied != NULL ) {
			*num_bytes_copied = bytes_to_copy;
		}

		return MX_SUCCESSFUL_RESULT;
	}

	/* NOTE: 1-dimensional arrays of structures are reported as if they
	 * were 2-dimensional arrays of strings, where the strings are the
	 * 'names' of the structures, for example record->name.  Thus, the
	 * ( num_dimensions == 1 ) case below should never be invoked for
	 * complex structures.
	 */

	if ( ( num_dimensions == 1 ) && ( return_structure_name == FALSE ) ) {

		bytes_to_copy = dimension_array[0] * data_element_size_array[0];

#if ADD_STRING_BYTE	/* Adding a byte is a bad idea. */

		if ( mx_datatype == MXFT_STRING ) {
			bytes_to_copy++;
		}
#endif

		if ( bytes_to_copy > destination_buffer_length ) {
			if ( num_bytes_copied != NULL ) {
				*num_bytes_copied =
				    bytes_to_copy - destination_buffer_length;
			}
			return mx_error(
				(MXE_WOULD_EXCEED_LIMIT | MXE_QUIET), fname,
				"The 1-dimensional array of size %ld bytes is "
				"too large to fit into the destination buffer "
				"of %ld bytes.",
					(long) bytes_to_copy,
					(long) destination_buffer_length );
		}

		switch( mx_datatype ) {
		case MXFT_STRING:
			strlcpy( (char *) destination_buffer,
					array_pointer, bytes_to_copy );
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
					array_pointer, bytes_to_copy );
			break;
		case MXFT_HEX:
		case MXFT_LONG:
		case MXFT_ULONG:
			if ( truncate_64bit_longs ) {
				mx_copy_64bits_to_32bits( destination_buffer,
					array_pointer, dimension_array[0] );
			} else {
				memcpy( destination_buffer,
					array_pointer, bytes_to_copy );
			}
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Array copy for data type %ld is not supported.",
				mx_datatype );
		}

		if ( num_bytes_copied != NULL ) {
			*num_bytes_copied = bytes_to_copy;
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

	if ( return_structure_name ) {
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
		if ( num_bytes_copied != NULL ) {
			*num_bytes_copied =
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
				NULL, truncate_64bit_longs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( num_bytes_copied != NULL ) {
		*num_bytes_copied = array_size;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_copy_buffer_to_array( void *source_buffer, size_t source_buffer_length,
		void *array_pointer,
		mx_bool_type array_is_dynamically_allocated,
		long mx_datatype, long num_dimensions,
		long *dimension_array, size_t *data_element_size_array,
		size_t *num_bytes_copied,
		mx_bool_type truncate_64bit_longs )
{
	static const char fname[] = "mx_copy_buffer_to_array()";

	char *array_row_pointer, *source_pointer;
	size_t bytes_to_copy, array_size, subarray_size, buffer_left;
	long i, n;
	int num_subarray_elements;
	mx_status_type mx_status;

	if (( array_pointer == NULL ) || ( dimension_array == NULL )
	 || ( data_element_size_array == NULL ) || ( source_buffer == NULL ))
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "One or more of the pointers passed to this function were NULL." );
	}

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
	}

	if ( num_bytes_copied != NULL ) {
		*num_bytes_copied = 0;
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

	if ( num_dimensions == 0 ) {

		/* Handling scalars takes a bit more effort. */

		bytes_to_copy = mx_get_scalar_element_size( mx_datatype,
						truncate_64bit_longs );

		if ( bytes_to_copy > source_buffer_length ) {
			return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
			"The size (%ld bytes) of the scalar value for this "
			"array is larger than the source buffer length "
			"of %ld bytes.",
				(long) bytes_to_copy,
				(long) source_buffer_length );
		}

#if 0
		MX_DEBUG(-2,
	("%s: num_dimensions = 0, mx_datatype = %ld, bytes_to_copy = %d",
			fname, mx_datatype, bytes_to_copy));
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
			memcpy( array_pointer, source_buffer, bytes_to_copy );
			break;

		case MXFT_HEX:
		case MXFT_LONG:
		case MXFT_ULONG:
			if ( truncate_64bit_longs ) {
				mx_copy_32bits_to_64bits( array_pointer,
					source_buffer, 1 );
			} else {
				memcpy( array_pointer,
					source_buffer, bytes_to_copy );
			}
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Array copy for data type %ld is not supported.",
				mx_datatype );
		}

		if ( num_bytes_copied != NULL ) {
			*num_bytes_copied = bytes_to_copy;
		}

		return MX_SUCCESSFUL_RESULT;
	}

	if ( num_dimensions == 1 ) {

		array_size = dimension_array[0] * data_element_size_array[0];

		bytes_to_copy = source_buffer_length;

#if ADD_STRING_BYTE	/* Adding a byte is a bad idea. */

		if ( mx_datatype == MXFT_STRING ) {
			bytes_to_copy++;
		}
#endif

		if ( bytes_to_copy > array_size ) {
			bytes_to_copy = array_size;
		}

		switch( mx_datatype ) {
		case MXFT_STRING:
			strlcpy( (char *) array_pointer,
					source_buffer, bytes_to_copy );
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
			memcpy( array_pointer, source_buffer, bytes_to_copy );
			break;

		case MXFT_HEX:
		case MXFT_LONG:
		case MXFT_ULONG:
			if ( truncate_64bit_longs ) {
				mx_copy_32bits_to_64bits( array_pointer,
					source_buffer, dimension_array[0] );
			} else {
				memcpy( array_pointer,
					source_buffer, bytes_to_copy );
			}
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Array copy for data type %ld is not supported.",
				mx_datatype );
		}

		if ( num_bytes_copied != NULL ) {
			*num_bytes_copied = bytes_to_copy;
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
				truncate_64bit_longs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( num_bytes_copied != NULL ) {
		*num_bytes_copied = array_size;
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

	if (( array_pointer == NULL ) || ( dimension_array == NULL )
	 || (data_element_size_array == NULL) || (xdr_buffer == NULL))
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "One or more of the pointers passed to this function were NULL." );
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

#if ADD_STRING_BYTE	/* Adding a byte is a bad idea. */

		if ( mx_datatype == MXFT_STRING ) {
			/* Add one byte for the string terminator. */

			native_array_size++;

			num_array_elements = (u_int) (dimension_array[0] + 1);
		} else {
			num_array_elements = (u_int) (dimension_array[0]);
		}

#else /* not ADD_STRING_BYTE */

		num_array_elements = (u_int) (dimension_array[0]);

#endif /* not ADD_STRING_BYTE */

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
		}

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

			*char_dest_ptr = long_source_value;
			break;

		case MXFT_UCHAR:
			uchar_dest_ptr = (unsigned char *)
						destination_array_pointer;

			*uchar_dest_ptr = long_source_value;
			break;

		case MXFT_SHORT:
			short_dest_ptr = (short *) destination_array_pointer;

			*short_dest_ptr = long_source_value;
			break;

		case MXFT_USHORT:
			ushort_dest_ptr = (unsigned short *)
						destination_array_pointer;

			*ushort_dest_ptr = long_source_value;
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

			*char_dest_ptr = ulong_source_value;
			break;

		case MXFT_UCHAR:
			uchar_dest_ptr = (unsigned char *)
						destination_array_pointer;

			*uchar_dest_ptr = ulong_source_value;
			break;

		case MXFT_SHORT:
			short_dest_ptr = (short *) destination_array_pointer;

			*short_dest_ptr = ulong_source_value;
			break;

		case MXFT_USHORT:
			ushort_dest_ptr = (unsigned short *)
						destination_array_pointer;

			*ushort_dest_ptr = ulong_source_value;
			break;

		case MXFT_BOOL:
			bool_dest_ptr = (mx_bool_type *)
						destination_array_pointer;

			*bool_dest_ptr = ulong_source_value;
			break;

		case MXFT_LONG:
			long_dest_ptr = (long *) destination_array_pointer;

			*long_dest_ptr = ulong_source_value;
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

			*char_dest_ptr = mx_round( float_source_value );
			break;

		case MXFT_UCHAR:
			uchar_dest_ptr = (unsigned char *)
						destination_array_pointer;

			*uchar_dest_ptr = mx_round( float_source_value );
			break;

		case MXFT_SHORT:
			short_dest_ptr = (short *) destination_array_pointer;

			*short_dest_ptr = mx_round( float_source_value );
			break;

		case MXFT_USHORT:
			ushort_dest_ptr = (unsigned short *)
						destination_array_pointer;

			*ushort_dest_ptr = mx_round( float_source_value );
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

			*char_dest_ptr = mx_round( double_source_value );
			break;

		case MXFT_UCHAR:
			uchar_dest_ptr = (unsigned char *)
						destination_array_pointer;

			*uchar_dest_ptr = mx_round( double_source_value );
			break;

		case MXFT_SHORT:
			short_dest_ptr = (short *) destination_array_pointer;

			*short_dest_ptr = mx_round( double_source_value );
			break;

		case MXFT_USHORT:
			ushort_dest_ptr = (unsigned short *)
						destination_array_pointer;

			*ushort_dest_ptr = mx_round( double_source_value );
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

