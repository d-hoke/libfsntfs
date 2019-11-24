/*
 * File system functions
 *
 * Copyright (C) 2010-2019, Joachim Metz <joachim.metz@gmail.com>
 *
 * Refer to AUTHORS for acknowledgements.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <byte_stream.h>
#include <memory.h>
#include <types.h>

#include "libfsntfs_cluster_block.h"
#include "libfsntfs_cluster_block_vector.h"
#include "libfsntfs_definitions.h"
#include "libfsntfs_file_system.h"
#include "libfsntfs_libcerror.h"
#include "libfsntfs_libcnotify.h"
#include "libfsntfs_libcthreads.h"
#include "libfsntfs_libuna.h"
#include "libfsntfs_mft.h"
#include "libfsntfs_mft_entry.h"
#include "libfsntfs_name.h"
#include "libfsntfs_security_descriptor_index.h"
#include "libfsntfs_security_descriptor_values.h"

/* Creates a file system
 * Make sure the value file_system is referencing, is set to NULL
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_file_system_initialize(
     libfsntfs_file_system_t **file_system,
     libcerror_error_t **error )
{
	static char *function = "libfsntfs_file_system_initialize";

	if( file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file system.",
		 function );

		return( -1 );
	}
	if( *file_system != NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid file system value already set.",
		 function );

		return( -1 );
	}
	*file_system = memory_allocate_structure(
	                libfsntfs_file_system_t );

	if( *file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_INSUFFICIENT,
		 "%s: unable to create file system.",
		 function );

		goto on_error;
	}
	if( memory_set(
	     *file_system,
	     0,
	     sizeof( libfsntfs_file_system_t ) ) == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_SET_FAILED,
		 "%s: unable to clear file system.",
		 function );

		memory_free(
		 *file_system );

		*file_system = NULL;

		return( -1 );
	}
#if defined( HAVE_LIBFSNTFS_MULTI_THREAD_SUPPORT )
	if( libcthreads_read_write_lock_initialize(
	     &( ( *file_system )->read_write_lock ),
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to initialize read/write lock.",
		 function );

		goto on_error;
	}
#endif
	return( 1 );

on_error:
	if( *file_system != NULL )
	{
		memory_free(
		 *file_system );

		*file_system = NULL;
	}
	return( -1 );
}

/* Frees a file system
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_file_system_free(
     libfsntfs_file_system_t **file_system,
     libcerror_error_t **error )
{
	static char *function = "libfsntfs_file_system_free";
	int result            = 1;

	if( file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file system.",
		 function );

		return( -1 );
	}
	if( *file_system != NULL )
	{
#if defined( HAVE_LIBFSNTFS_MULTI_THREAD_SUPPORT )
		if( libcthreads_read_write_lock_free(
		     &( ( *file_system )->read_write_lock ),
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
			 "%s: unable to free read/write lock.",
			 function );

			result = -1;
		}
#endif
		if( ( *file_system )->security_descriptor_index != NULL )
		{
			if( libfsntfs_security_descriptor_index_free(
			     &( ( *file_system )->security_descriptor_index ),
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
				 "%s: unable to free security descriptor index.",
				 function );

				result = -1;
			}
		}
		if( ( *file_system )->mft != NULL )
		{
			if( libfsntfs_mft_free(
			     &( ( *file_system )->mft ),
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
				 "%s: unable to free MFT.",
				 function );

				result = -1;
			}
		}
		memory_free(
		 *file_system );

		*file_system = NULL;
	}
	return( result );
}

/* Reads the MFT
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_file_system_read_mft(
     libfsntfs_file_system_t *file_system,
     libfsntfs_io_handle_t *io_handle,
     libbfio_handle_t *file_io_handle,
     off64_t mft_offset,
     size64_t mft_size,
     uint8_t flags,
     libcerror_error_t **error )
{
	libfsntfs_mft_entry_t *mft_entry = NULL;
	static char *function            = "libfsntfs_file_system_read_mft";
	uint64_t number_of_mft_entries   = 0;

	if( file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file system.",
		 function );

		return( -1 );
	}
	if( file_system->mft != NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid file system - MFT value already set.",
		 function );

		return( -1 );
	}
	if( io_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid IO handle.",
		 function );

		return( -1 );
	}
	if( mft_offset < 0 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid MFT offset value out of bounds.",
		 function );

		goto on_error;
	}
	if( mft_size > (size64_t) INT64_MAX )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_EXCEEDS_MAXIMUM,
		 "%s: invalid MFT size value exceeds maximum.",
		 function );

		return( -1 );
	}
	/* Since MFT entry 0 can contain an attribute list
	 * we define MFT entry vector before knowning all the data runs
	 */
	if( libfsntfs_mft_initialize(
	     &( file_system->mft ),
	     io_handle,
	     mft_offset,
	     mft_size,
	     (size64_t) io_handle->mft_entry_size,
	     flags,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create MFT.",
		 function );

		goto on_error;
	}
	if( libfsntfs_mft_entry_initialize(
	     &mft_entry,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create MFT entry.",
		 function );

		goto on_error;
	}
	if( libfsntfs_mft_read_mft_entry(
	     file_system->mft,
	     io_handle,
	     file_io_handle,
	     mft_offset,
	     0,
	     mft_entry,
	     flags,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read MFT entry: 0.",
		 function );

		goto on_error;
	}
	if( ( flags & LIBFSNTFS_FILE_ENTRY_FLAGS_MFT_ONLY ) == 0 )
	{
		if( libfsntfs_mft_set_data_runs(
		     file_system->mft,
		     mft_entry,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
			 "%s: unable to set MFT data runs.",
			 function );

			goto on_error;
		}
	}
	else
	{
		if( mft_entry->data_attribute == NULL )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
			 "%s: invalid MFT entry: 0 - missing data attribute.",
			 function );

			goto on_error;
		}
		number_of_mft_entries = (uint64_t) ( mft_size / io_handle->mft_entry_size );

		if( number_of_mft_entries > (uint64_t) INT_MAX )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: invalid number of MFT entries value out of bounds.",
			 function );

			goto on_error;
		}
		file_system->mft->number_of_mft_entries = number_of_mft_entries;
	}
	if( libfsntfs_mft_entry_free(
	     &mft_entry,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
		 "%s: unable to free MFT entry.",
		 function );

		goto on_error;
	}
	return( 1 );

on_error:
	if( mft_entry != NULL )
	{
		libfsntfs_mft_entry_free(
		 &mft_entry,
		 NULL );
	}
	return( -1 );
}

/* Reads the bitmap file entry
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_file_system_read_bitmap(
     libfsntfs_file_system_t *file_system,
     libfsntfs_io_handle_t *io_handle,
     libbfio_handle_t *file_io_handle,
     libcerror_error_t **error )
{
	libfcache_cache_t *cluster_block_cache   = NULL;
	libfdata_vector_t *cluster_block_vector  = NULL;
	libfsntfs_cluster_block_t *cluster_block = NULL;
	libfsntfs_mft_entry_t *mft_entry         = NULL;
	static char *function                    = "libfsntfs_file_system_read_bitmap";
	off64_t bitmap_offset                    = 0;
	off64_t start_offset                     = 0;
	size_t cluster_block_data_offset         = 0;
	uint32_t value_32bit                     = 0;
	uint8_t bit_index                        = 0;
	int cluster_block_index                  = 0;
	int number_of_cluster_blocks             = 0;

	if( file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file system.",
		 function );

		return( -1 );
	}
	if( io_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid IO handle.",
		 function );

		return( -1 );
	}
	if( libfsntfs_mft_get_mft_entry_by_index(
	     file_system->mft,
	     file_io_handle,
	     LIBFSNTFS_MFT_ENTRY_INDEX_BITMAP,
	     &mft_entry,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve MFT entry: %d.",
		 function,
		 LIBFSNTFS_MFT_ENTRY_INDEX_BITMAP );

		goto on_error;
	}
	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: missing MFT entry: %d.",
		 function,
		 LIBFSNTFS_MFT_ENTRY_INDEX_BITMAP );

		goto on_error;
	}
	if( mft_entry->data_attribute == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: invalid MFT entry: %d - missing data attribute.",
		 function,
		 LIBFSNTFS_MFT_ENTRY_INDEX_BITMAP );

		goto on_error;
	}
	if( libfsntfs_cluster_block_vector_initialize(
	     &cluster_block_vector,
	     io_handle,
	     mft_entry->data_attribute,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create cluster block vector.",
		 function );

		goto on_error;
	}
	if( libfcache_cache_initialize(
	     &cluster_block_cache,
	     1,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create cluster block cache.",
		 function );

		goto on_error;
	}
	if( libfdata_vector_get_number_of_elements(
	     cluster_block_vector,
	     &number_of_cluster_blocks,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve number of cluster blocks.",
		 function );

		goto on_error;
	}
	for( cluster_block_index = 0;
	     cluster_block_index < number_of_cluster_blocks;
	     cluster_block_index++ )
	{
		if( libfdata_vector_get_element_value_by_index(
		     cluster_block_vector,
		     (intptr_t *) file_io_handle,
		     (libfdata_cache_t *) cluster_block_cache,
		     cluster_block_index,
		     (intptr_t **) &cluster_block,
		     0,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve cluster block: %d from vector.",
			 function,
			 cluster_block_index );

			goto on_error;
		}
		if( cluster_block == NULL )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
			 "%s: missing cluster block: %d.",
			 function,
			 cluster_block_index );

			goto on_error;
		}
		if( cluster_block->data == NULL )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
			 "%s: invalid cluster block: %d - missing data.",
			 function,
			 cluster_block_index );

			goto on_error;
		}
		if( ( ( cluster_block->data_size % 4 ) != 0 )
		 || ( cluster_block->data_size > (size_t) SSIZE_MAX ) )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: cluster block: %d data size value out of bounds.",
			 function,
			 cluster_block_index );

			goto on_error;
		}
#if defined( HAVE_DEBUG_OUTPUT )
		if( libcnotify_verbose != 0 )
		{
			libcnotify_printf(
			 "%s: bitmap segment: %d data:\n",
			 function,
			 cluster_block_index );
			libcnotify_print_data(
			 cluster_block->data,
			 cluster_block->data_size,
			 LIBCNOTIFY_PRINT_DATA_FLAG_GROUP_DATA );
		}
#endif
		cluster_block_data_offset = 0;
		start_offset              = -1;

		while( cluster_block_data_offset < cluster_block->data_size )
		{
			byte_stream_copy_to_uint32_little_endian(
			 &( cluster_block->data[ cluster_block_data_offset ] ),
			 value_32bit );

			for( bit_index = 0;
			     bit_index < 32;
			     bit_index++ )
			{
				if( ( value_32bit & 0x00000001UL ) == 0 )
				{
					if( start_offset >= 0 )
					{
#if defined( HAVE_DEBUG_OUTPUT )
						if( libcnotify_verbose != 0 )
						{
							libcnotify_printf(
							 "%s: offset range\t\t\t: 0x%08" PRIx64 " - 0x%08" PRIx64 " (0x%08" PRIx64 ")\n",
							 function,
							 start_offset,
							 bitmap_offset,
							 bitmap_offset - start_offset );
						}
#endif
/*
						if( libfsntfs_offset_list_append_offset(
						     offset_list,
						     start_offset,
						     bitmap_offset - start_offset,
						     1,
						     error ) != 1 )
						{
							libcerror_error_set(
							 error,
							 LIBCERROR_ERROR_DOMAIN_RUNTIME,
							 LIBCERROR_RUNTIME_ERROR_APPEND_FAILED,
							 "%s: unable to append offset range to offset list.",
							 function );

							goto on_error;
						}
*/
						start_offset = -1;
					}
				}
				else
				{
					if( start_offset < 0 )
					{
						start_offset = bitmap_offset;
					}
				}
				bitmap_offset += io_handle->cluster_block_size;

				value_32bit >>= 1;
			}
			cluster_block_data_offset += 4;
		}
		if( start_offset >= 0 )
		{
#if defined( HAVE_DEBUG_OUTPUT )
			if( libcnotify_verbose != 0 )
			{
				libcnotify_printf(
				 "%s: offset range\t\t\t: 0x%08" PRIx64 " - 0x%08" PRIx64 " (0x%08" PRIx64 ")\n",
				 function,
				 start_offset,
				 bitmap_offset,
				 bitmap_offset - start_offset );
			}
#endif
/* TODO
			if( libfsntfs_offset_list_append_offset(
			     offset_list,
			     start_offset,
			     bitmap_offset - start_offset,
			     1,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_APPEND_FAILED,
				 "%s: unable to append offset range to offset list.",
				 function );

				goto on_error;
			}
*/
		}
#if defined( HAVE_DEBUG_OUTPUT )
		if( libcnotify_verbose != 0 )
		{
			libcnotify_printf(
			 "\n" );
		}
#endif
	}
	if( libfdata_vector_free(
	     &cluster_block_vector,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
		 "%s: unable to free cluster block vector.",
		 function );

		goto on_error;
	}
	if( libfcache_cache_free(
	     &cluster_block_cache,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
		 "%s: unable to free cluster block cache.",
		 function );

		goto on_error;
	}
	return( 1 );

on_error:
	if( cluster_block_cache != NULL )
	{
		libfcache_cache_free(
		 &cluster_block_cache,
		 NULL );
	}
	if( cluster_block_vector != NULL )
	{
		libfdata_vector_free(
		 &cluster_block_vector,
		 NULL );
	}
	return( -1 );
}

/* Reads the security descriptors
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_file_system_read_security_descriptors(
     libfsntfs_file_system_t *file_system,
     libfsntfs_io_handle_t *io_handle,
     libbfio_handle_t *file_io_handle,
     libcerror_error_t **error )
{
	libfsntfs_file_name_values_t *file_name_values = NULL;
	libfsntfs_mft_attribute_t *data_attribute      = NULL;
	libfsntfs_mft_attribute_t *mft_attribute       = NULL;
	libfsntfs_mft_entry_t *mft_entry               = NULL;
	static char *function                          = "libfsntfs_file_system_read_security_descriptors";
	int result                                     = 0;

	if( file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file system.",
		 function );

		return( -1 );
	}
	if( file_system->security_descriptor_index != NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid file system - security descriptor index value already set.",
		 function );

		return( -1 );
	}
	if( libfsntfs_mft_get_mft_entry_by_index(
	     file_system->mft,
	     file_io_handle,
	     LIBFSNTFS_MFT_ENTRY_INDEX_SECURE,
	     &mft_entry,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve MFT entry: %d.",
		 function,
		 LIBFSNTFS_MFT_ENTRY_INDEX_SECURE );

		goto on_error;
	}
	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: missing MFT entry: %d.",
		 function,
		 LIBFSNTFS_MFT_ENTRY_INDEX_SECURE );

		goto on_error;
	}
	if( libfsntfs_mft_entry_get_attribute_by_index(
	     mft_entry,
	     mft_entry->file_name_attribute_index,
	     &mft_attribute,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve attribute: %d from MFT entry: %d.",
		 function,
		 mft_entry->file_name_attribute_index,
		 LIBFSNTFS_MFT_ENTRY_INDEX_SECURE );

		goto on_error;
	}
	if( libfsntfs_file_name_values_initialize(
	     &file_name_values,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create file name values.",
		 function );

		goto on_error;
	}
	if( libfsntfs_file_name_values_read_from_mft_attribute(
	     file_name_values,
	     mft_attribute,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read file name values from attribute: %d from MFT entry: %d.",
		 function,
		 mft_entry->file_name_attribute_index,
		 LIBFSNTFS_MFT_ENTRY_INDEX_SECURE );

		goto on_error;
	}
	result = libfsntfs_name_compare_with_utf8_string(
	          file_name_values->name,
	          file_name_values->name_size,
	          (uint8_t *) "$Secure",
	          7,
	          1,
	          error );

	if( result == -1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GENERIC,
		 "%s: unable to compare UTF-8 string with data attribute name.",
		 function );

		goto on_error;
	}
	else if( result == LIBUNA_COMPARE_EQUAL )
	{
		if( libfsntfs_mft_entry_get_alternate_data_attribute_by_utf8_name(
		     mft_entry,
		     (uint8_t *) "$SDS",
		     4,
		     &data_attribute,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve $SDS data attribute.",
			 function );

			goto on_error;
		}
		if( libfsntfs_security_descriptor_index_initialize(
		     &( file_system->security_descriptor_index ),
		     io_handle,
		     file_io_handle,
		     data_attribute,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
			 "%s: unable to create security descriptor index.",
			 function );

			goto on_error;
		}
		if( libfsntfs_security_descriptor_index_read_sii_index(
		     file_system->security_descriptor_index,
		     io_handle,
		     file_io_handle,
		     mft_entry,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_READ_FAILED,
			 "%s: unable to read security descriptor identifier ($SII) index.",
			 function );

			goto on_error;
		}
	}
	if( libfsntfs_file_name_values_free(
	     &file_name_values,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
		 "%s: unable to free file name values.",
		 function );

		goto on_error;
	}
	return( 1 );

on_error:
	if( file_system->security_descriptor_index != NULL )
	{
		libfsntfs_security_descriptor_index_free(
		 &( file_system->security_descriptor_index ),
		 NULL );
	}
	if( file_name_values != NULL )
	{
		libfsntfs_file_name_values_free(
		 &file_name_values,
		 NULL );
	}
	return( -1 );
}

/* Retrieves the number of MFT entries
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_file_system_get_number_of_mft_entries(
     libfsntfs_file_system_t *file_system,
     uint64_t *number_of_mft_entries,
     libcerror_error_t **error )
{
	static char *function = "libfsntfs_file_system_get_number_of_mft_entries";

	if( file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file system.",
		 function );

		return( -1 );
	}
	if( libfsntfs_mft_get_number_of_entries(
	     file_system->mft,
	     number_of_mft_entries,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve number of MFT entries.",
		 function );

		return( -1 );
	}
	return( 1 );
}

/* Retrieves the MFT entry for a specific index
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_file_system_get_mft_entry_by_index(
     libfsntfs_file_system_t *file_system,
     libbfio_handle_t *file_io_handle,
     uint64_t mft_entry_index,
     libfsntfs_mft_entry_t **mft_entry,
     libcerror_error_t **error )
{
	static char *function = "libfsntfs_file_system_get_mft_entry_by_index";

	if( file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file system.",
		 function );

		return( -1 );
	}
	if( libfsntfs_mft_get_mft_entry_by_index(
	     file_system->mft,
	     file_io_handle,
	     mft_entry_index,
	     mft_entry,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve MFT entry: %" PRIi64 ".",
		 function,
		 mft_entry_index );

		return( -1 );
	}
	return( 1 );
}

/* Retrieves the MFT entry for a specific index
 * This function creates new MFT entry
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_file_system_get_mft_entry_by_index_no_cache(
     libfsntfs_file_system_t *file_system,
     libbfio_handle_t *file_io_handle,
     uint64_t mft_entry_index,
     libfsntfs_mft_entry_t **mft_entry,
     libcerror_error_t **error )
{
	static char *function = "libfsntfs_file_system_get_mft_entry_by_index_no_cache";

	if( file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file system.",
		 function );

		return( -1 );
	}
	if( libfsntfs_mft_get_mft_entry_by_index_no_cache(
	     file_system->mft,
	     file_io_handle,
	     mft_entry_index,
	     mft_entry,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve MFT entry: %" PRIi64 ".",
		 function,
		 mft_entry_index );

		return( -1 );
	}
	return( 1 );
}

/* Retrieves the security descriptor for a specific identifier
 * This function creates new security descriptor values
 * Returns 1 if successful, 0 if not available or -1 on error
 */
int libfsntfs_file_system_get_security_descriptor_values_by_identifier(
     libfsntfs_file_system_t *file_system,
     libbfio_handle_t *file_io_handle,
     uint32_t security_descriptor_identifier,
     libfsntfs_security_descriptor_values_t **security_descriptor_values,
     libcerror_error_t **error )
{
	static char *function = "libfsntfs_file_system_get_security_descriptor_values_by_identifier";
	int result            = 0;

	if( file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file system.",
		 function );

		return( -1 );
	}
	if( file_system->security_descriptor_index != NULL )
	{
		result = libfsntfs_security_descriptor_index_get_entry_by_identifier(
		          file_system->security_descriptor_index,
		          file_io_handle,
		          security_descriptor_identifier,
		          security_descriptor_values,
		          error );

		if( result == -1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve security descriptor from index for identifier: %" PRIu32 ".",
			 function,
			 security_descriptor_identifier );

			return( -1 );
		}
	}
	return( result );
}

