/* ----------------------------------------------------------------------------
@COPYRIGHT  :
              Copyright 1993,1994,1995 David MacDonald,
              McConnell Brain Imaging Centre,
              Montreal Neurological Institute, McGill University.
              Permission to use, copy, modify, and distribute this
              software and its documentation for any purpose and without
              fee is hereby granted, provided that the above copyright
              notice appear in all copies.  The author and McGill University
              make no representations about the suitability of this
              software for any purpose.  It is provided "as is" without
              express or implied warranty.
---------------------------------------------------------------------------- */

#ifndef lint
static char rcsid[] = "$Header: /private-cvsroot/minc/volume_io/Volumes/volume_cache.c,v 1.15 1995-10-26 18:30:03 david Exp $";
#endif

#include  <internal_volume_io.h>

#define   HASH_FUNCTION_CONSTANT          0.6180339887498948482
#define   HASH_TABLE_SIZE_FACTOR          3

#define   DEFAULT_BLOCK_SIZE              8
#define   DEFAULT_CACHE_THRESHOLD         80000000
#define   DEFAULT_MAX_BYTES_IN_CACHE      80000000

static  BOOLEAN  n_bytes_cache_threshold_set = FALSE;
static  int      n_bytes_cache_threshold = DEFAULT_CACHE_THRESHOLD;

static  BOOLEAN  default_cache_size_set = FALSE;
static  int      default_cache_size = DEFAULT_MAX_BYTES_IN_CACHE;


static  Cache_block_size_hints   block_size_hint = RANDOM_VOLUME_ACCESS;
static  BOOLEAN  default_block_sizes_set = FALSE;
static  int      default_block_sizes[MAX_DIMENSIONS] = {
                                                     DEFAULT_BLOCK_SIZE,
                                                     DEFAULT_BLOCK_SIZE,
                                                     DEFAULT_BLOCK_SIZE,
                                                     DEFAULT_BLOCK_SIZE,
                                                     DEFAULT_BLOCK_SIZE };

private  void  alloc_volume_cache(
    volume_cache_struct   *cache,
    Volume                volume );

/* ----------------------------- MNI Header -----------------------------------
@NAME       : set_n_bytes_cache_threshold
@INPUT      : threshold
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Sets the threshold number of bytes which decides if a volume
              is small enough to be held entirely in memory, or whether it
              should be cached.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  set_n_bytes_cache_threshold(
    int  threshold )
{
    n_bytes_cache_threshold = threshold;
    n_bytes_cache_threshold_set = TRUE;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : get_n_bytes_cache_threshold
@INPUT      : 
@OUTPUT     : 
@RETURNS    : number of bytes
@DESCRIPTION: Returns the number of bytes defining the cache threshold.  If it
              hasn't been set, returns the program initialized value, or the
              value set by the environment variable.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  int  get_n_bytes_cache_threshold()
{
    int   n_bytes;

    if( !n_bytes_cache_threshold_set )
    {
        if( getenv( "VOLUME_CACHE_THRESHOLD" ) != NULL &&
            sscanf( getenv( "VOLUME_CACHE_THRESHOLD" ), "%d", &n_bytes ) == 1 )
        {
            n_bytes_cache_threshold = n_bytes;
        }
        n_bytes_cache_threshold_set = TRUE;
    }

    return( n_bytes_cache_threshold );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : set_default_max_bytes_in_cache
@INPUT      : max_bytes 
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Sets the default value for the maximum amount of memory
              in a single volume's cache.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Oct. 19, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  set_default_max_bytes_in_cache(
    int   max_bytes )
{
    default_cache_size_set = TRUE;
    default_cache_size = max_bytes;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : get_default_max_bytes_in_cache
@INPUT      : 
@OUTPUT     : 
@RETURNS    : number of bytes
@DESCRIPTION: Returns the maximum number of bytes allowed for a single
              volume's cache.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  int  get_default_max_bytes_in_cache()
{
    int   n_bytes;

    if( !default_cache_size_set )
    {
        if( getenv( "VOLUME_CACHE_SIZE" ) != NULL &&
            sscanf( getenv( "VOLUME_CACHE_SIZE" ), "%d", &n_bytes ) == 1 )
        {
            default_cache_size = n_bytes;
        }

        default_cache_size_set = TRUE;
    }

    return( default_cache_size );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : set_default_cache_block_sizes
@INPUT      : block_sizes
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Sets the default values for the volume cache block sizes.
              A non-positive value will result in a block size equal to the
              number of voxels in that dimension of the volume.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Oct. 19, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  set_default_cache_block_sizes(
    int                      block_sizes[] )
{
    int   dim;

    for_less( dim, 0, MAX_DIMENSIONS )
        default_block_sizes[dim] = block_sizes[dim];

    default_block_sizes_set = TRUE;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : set_cache_block_sizes_hint
@INPUT      : hint
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Sets the hint for deciding on block sizes.  This turns off
              the default_block_sizes_set flag, thereby overriding any
              previous calls to set_default_cache_block_sizes().
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Oct. 25, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  set_cache_block_sizes_hint(
    Cache_block_size_hints  hint )
{
    block_size_hint = hint;
    default_block_sizes_set = FALSE;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : get_default_cache_block_sizes
@INPUT      : 
@OUTPUT     : block_sizes[]
@RETURNS    : 
@DESCRIPTION: Passes back the size (in voxels) of each dimension of a cache
              block.  If it hasn't been set, returns the program initialized
              value, or the value set by the environment variable.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  void  get_default_cache_block_sizes(
    int    n_dims,
    int    volume_sizes[],
    int    block_sizes[] )
{
    int   dim, block_size;

    if( !default_block_sizes_set && block_size_hint == SLICE_ACCESS )
    {
        for_less( dim, 0, n_dims - 2 )
            block_sizes[dim] = 1;

        /*--- set the last two dimensions to be entire size of dimension */

        for_less( dim, MAX( 0, n_dims - 2), n_dims )
            block_sizes[dim] = -1;
    }
    else if( !default_block_sizes_set &&
             block_size_hint == RANDOM_VOLUME_ACCESS )
    {
        if( getenv( "VOLUME_CACHE_BLOCK_SIZE" ) == NULL ||
            sscanf( getenv( "VOLUME_CACHE_BLOCK_SIZE" ), "%d", &block_size )
                    != 1 || block_size < 1 )
        {
            block_size = DEFAULT_BLOCK_SIZE;
        }

        for_less( dim, 0, MAX_DIMENSIONS )
            block_sizes[dim] = block_size;
    }
    else
    {
        for_less( dim, 0, MAX_DIMENSIONS )
            block_sizes[dim] = default_block_sizes[dim];
    }

    /*--- now change any non-positive values to the correct volume size */

    for_less( dim, 0, MAX_DIMENSIONS )
    {
        if( block_sizes[dim] <= 0 )
            block_sizes[dim] = volume_sizes[dim];
    }
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : initialize_volume_cache
@INPUT      : cache
              volume
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Initializes the cache for a volume.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  initialize_volume_cache(
    volume_cache_struct   *cache,
    Volume                volume )
{
    int    dim, n_dims, sizes[MAX_DIMENSIONS];

    n_dims = get_volume_n_dimensions( volume );
    cache->n_dimensions = n_dims;
    cache->dim_names_set = FALSE;
    cache->writing_to_temp_file = FALSE;

    for_less( dim, 0, MAX_DIMENSIONS )
    {
        cache->file_offset[dim] = 0;
        cache->dimension_names[dim] = NULL;
    }

    cache->minc_file = NULL;
    cache->input_filename = NULL;
    cache->output_filename = NULL;
    cache->output_file_is_open = FALSE;
    cache->must_read_blocks_before_use = FALSE;

    get_volume_sizes( volume, sizes );

    get_default_cache_block_sizes( n_dims, sizes, cache->block_sizes );
    cache->max_cache_bytes = get_default_max_bytes_in_cache();

    alloc_volume_cache( cache, volume );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : alloc_volume_cache
@INPUT      : cache
              volume
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Allocates the volume cache.  Uses the current value of the
              volumes max cache size and block sizes to decide how much to
              allocate.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  void  alloc_volume_cache(
    volume_cache_struct   *cache,
    Volume                volume )
{
    int    dim, n_dims, sizes[MAX_DIMENSIONS], block, block_size;
    int    x, block_stride, remainder, block_index;

    get_volume_sizes( volume, sizes );
    n_dims = get_volume_n_dimensions( volume );

    /*--- count number of blocks needed per dimension */

    block_size = 1;
    block_stride = 1;

    for_down( dim, n_dims - 1, 0 )
    {
        cache->blocks_per_dim[dim] = (sizes[dim] - 1) / cache->block_sizes[dim]
                                     + 1;

        ALLOC( cache->lookup[dim], sizes[dim] );
        for_less( x, 0, sizes[dim] )
        {
            remainder = x % cache->block_sizes[dim];
            block_index = x / cache->block_sizes[dim];
            cache->lookup[dim][x].block_index_offset =
                                       block_index * block_stride;
            cache->lookup[dim][x].block_offset = remainder * block_size;
        }

        block_size *= cache->block_sizes[dim];
        block_stride *= cache->blocks_per_dim[dim];
    }

    cache->total_block_size = block_size;
    cache->max_blocks = cache->max_cache_bytes / block_size /
                        get_type_size(get_volume_data_type(volume));

    if( cache->max_blocks < 1 )
        cache->max_blocks = 1;

    /*--- create and initialize an empty hash table */

    cache->hash_table_size = cache->max_blocks * HASH_TABLE_SIZE_FACTOR;

    ALLOC( cache->hash_table, cache->hash_table_size );

    for_less( block, 0, cache->hash_table_size )
        cache->hash_table[block] = NULL;

    /*--- set up the initial pointers */

    cache->previous_block_index = -1;
    cache->head = NULL;
    cache->tail = NULL;
    cache->n_blocks = 0;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : get_block_start
@INPUT      : cache
              block_index
@OUTPUT     : block_start[]
@RETURNS    : 
@DESCRIPTION: Computes the starting voxel indices for a block.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  void  get_block_start(
    volume_cache_struct  *cache,
    int                  block_index,
    int                  block_start[] )
{
    int    dim, block_i;

    for_down( dim, cache->n_dimensions-1, 0 )
    {
        block_i = block_index % cache->blocks_per_dim[dim];
        block_start[dim] = block_i * cache->block_sizes[dim];
        block_index /= cache->blocks_per_dim[dim];
    }
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : write_cache_block
@INPUT      : cache
              volume
              block
              block_start
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Writes out a cache block to the appropriate position in the
              corresponding file.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  void  write_cache_block(
    volume_cache_struct  *cache,
    Volume               volume,
    cache_block_struct   *block,
    int                  block_start[] )
{
    Minc_file        minc_file;
    int              dim, ind, n_dims;
    int              file_start[MAX_DIMENSIONS];
    int              file_count[MAX_DIMENSIONS];
    int              volume_sizes[MAX_DIMENSIONS];
    void             *array_data_ptr;

    minc_file = (Minc_file) cache->minc_file;

    get_volume_sizes( volume, volume_sizes );

    for_less( dim, 0, minc_file->n_file_dimensions )
    {
        ind = minc_file->to_volume_index[dim];
        if( ind >= 0 )
        {
            file_start[dim] = cache->file_offset[dim] + block_start[ind];
            file_count[dim] = MIN( volume_sizes[ind] - file_start[dim],
                                   cache->block_sizes[ind] );
        }
        else
        {
            file_start[dim] = cache->file_offset[dim];
            file_count[dim] = 0;
        }
    }

    GET_MULTIDIM_PTR( array_data_ptr, block->array, 0, 0, 0, 0, 0 );
    n_dims = cache->n_dimensions;

    (void) output_minc_hyperslab( (Minc_file) cache->minc_file,
                                  get_multidim_data_type(&block->array),
                                  n_dims, cache->block_sizes, array_data_ptr,
                                  minc_file->to_volume_index,
                                  file_start, file_count );

    cache->must_read_blocks_before_use = TRUE;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : flush_cache_blocks
@INPUT      : cache
              volume
              deleting_volume_flag  - TRUE if deleting the volume
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Deletes all cache blocks, writing out all blocks, if the volume
              has been modified.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  void  flush_cache_blocks(
    volume_cache_struct   *cache,
    Volume                volume,
    BOOLEAN               deleting_volume_flag )
{
    int                 block, block_index;
    int                 block_start[MAX_DIMENSIONS];
    cache_block_struct  *this, *next;

    /*--- step through linked list, freeing blocks */

    this = cache->head;
    while( this != NULL )
    {
        if( this->modified_flag &&
            (!cache->writing_to_temp_file|| !deleting_volume_flag) )
        {
            block_index = this->block_index;
            get_block_start( cache, block_index, block_start );
            write_cache_block( cache, volume, this, block_start );
        }

        next = this->next_used;
        delete_multidim_array( &this->array );
        FREE( this );
        this = next;
    }

    cache->n_blocks = 0;

    for_less( block, 0, cache->hash_table_size )
        cache->hash_table[block] = NULL;

    cache->previous_block_index = -1;
    cache->head = NULL;
    cache->tail = NULL;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : delete_volume_cache
@INPUT      : cache
              volume
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Deletes the volume cache.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  delete_volume_cache(
    volume_cache_struct   *cache,
    Volume                volume )
{
    int   dim, n_dims;

    flush_cache_blocks( cache, volume, TRUE );

    FREE( cache->hash_table );

    n_dims = cache->n_dimensions;
    for_less( dim, 0, n_dims )
    {
        delete_string( cache->dimension_names[dim] );
        FREE( cache->lookup[dim] );
    }

    delete_string( cache->input_filename );
    delete_string( cache->output_filename );

    /*--- close the file that cache was reading from or writing to */

    if( cache->minc_file != NULL )
    {
        if( cache->output_file_is_open )
        {
            (void) close_minc_output( (Minc_file) cache->minc_file );
        }
        else
            (void) close_minc_input( (Minc_file) cache->minc_file );
    }
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : set_volume_cache_block_sizes
@INPUT      : volume
              block_sizes
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Changes the sizes of the cache blocks for the volume,
              if it is a cached volume.  This flushes the cache blocks,
              since they have changed.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Oct. 24, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  set_volume_cache_block_sizes(
    Volume    volume,
    int       block_sizes[] )
{
    volume_cache_struct   *cache;
    int                   d, dim;
    BOOLEAN               changed;

    if( !volume->is_cached_volume )
        return;

    cache = &volume->cache;

    changed = FALSE;

    for_less( d, 0, get_volume_n_dimensions(volume) )
    {
        if( block_sizes[d] >= 1 )
        {
            if( cache->block_sizes[d] != block_sizes[d] )
                changed = TRUE;
        }
        else
        {
            print_error(
                  "Invalid block sizes in set_volume_cache_block_sizes()\n" );
            return;
        }
    }

    /*--- if the block sizes have not changed, do nothing */

    if( !changed )
        return;

    flush_cache_blocks( cache, volume, FALSE );

    FREE( cache->hash_table );

    for_less( dim, 0, get_volume_n_dimensions( volume ) )
    {
        FREE( cache->lookup[dim] );
    }

    for_less( d, 0, get_volume_n_dimensions(volume) )
    {
        if( block_sizes[d] >= 1 )
            cache->block_sizes[d] = block_sizes[d];
        else
            print_error(
                  "Invalid block sizes in set_volume_cache_block_sizes()\n" );
    }

    alloc_volume_cache( cache, volume );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : set_volume_cache_size
@INPUT      : volume
              max_memory_bytes
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Changes the maximum amount of memory in the cache for this
              volume, if it is a cached volume.  This flushes the cache,
              in order to reallocate the hash table to a new size.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Oct. 24, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  set_volume_cache_size(
    Volume    volume,
    int       max_memory_bytes )
{
    int                   dim;
    volume_cache_struct   *cache;

    if( !volume->is_cached_volume )
        return;

    cache = &volume->cache;

    flush_cache_blocks( cache, volume, FALSE );

    FREE( cache->hash_table );

    for_less( dim, 0, get_volume_n_dimensions( volume ) )
    {
        FREE( cache->lookup[dim] );
    }

    cache->max_cache_bytes = max_memory_bytes;

    alloc_volume_cache( cache, volume );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : set_cache_volume_output_dimension_names
@INPUT      : volume
              dimension_names
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Sets the output dimension names for any file created as a
              result of using volume caching.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  set_cache_volume_output_dimension_names(
    Volume      volume,
    STRING      dimension_names[] )
{
    int   dim;

    for_less( dim, 0, get_volume_n_dimensions(volume) )
    {
        volume->cache.dimension_names[dim] =
                                  create_string( dimension_names[dim] );
    }

    volume->cache.dim_names_set = TRUE;
}
    
/* ----------------------------- MNI Header -----------------------------------
@NAME       : set_cache_volume_output_filename
@INPUT      : volume
              filename
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Sets the output filename for any file created as a result of
              using volume caching.  If a cached volume is modified, a
              temporary file is created to store the volume.  If this function
              is called prior to this, a permanent file with the given name
              is created instead.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  set_cache_volume_output_filename(
    Volume      volume,
    STRING      filename )
{
    volume->cache.output_filename = create_string( filename );
}
    
/* ----------------------------- MNI Header -----------------------------------
@NAME       : open_cache_volume_input_file
@INPUT      : cache
              volume
              filename
              options
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Opens the volume file for reading into the cache as needed.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  open_cache_volume_input_file(
    volume_cache_struct   *cache,
    Volume                volume,
    STRING                filename,
    minc_input_options    *options )
{
    cache->input_filename = create_string( filename );

    cache->minc_file = initialize_minc_input( filename, volume, options );

    cache->must_read_blocks_before_use = TRUE;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : open_cache_volume_output_file
@INPUT      : cache
              volume
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Opens a volume file for reading and writing cache blocks.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  void  open_cache_volume_output_file(
    volume_cache_struct   *cache,
    Volume                volume )
{
    int        dim, n_dims;
    int        out_sizes[MAX_DIMENSIONS], vol_sizes[MAX_DIMENSIONS];
    int        i, j, n_found;
    Real       voxel_min, voxel_max;
    Real       min_value, max_value;
    nc_type    nc_data_type;
    Minc_file  out_minc_file;
    BOOLEAN    done[MAX_DIMENSIONS], signed_flag;
    char       tmp_name[L_tmpnam+1];
    STRING     *vol_dim_names;
    STRING     out_dim_names[MAX_DIMENSIONS], output_filename;
    minc_output_options  options;

    /*--- check if the output filename has been set */

    if( string_length( cache->output_filename ) == 0 )
    {
        cache->writing_to_temp_file = TRUE;
        (void) tmpnam( tmp_name );
        output_filename = concat_strings( tmp_name, "." );
        concat_to_string( &output_filename, MNC_ENDING );
    }
    else
    {
        cache->writing_to_temp_file = FALSE;
        output_filename = create_string( cache->output_filename );
    }

    n_dims = get_volume_n_dimensions( volume );
    vol_dim_names = get_volume_dimension_names( volume );
    get_volume_sizes( volume, vol_sizes );

    if( cache->dim_names_set )
    {
        for_less( dim, 0, n_dims )
            out_dim_names[dim] = create_string( cache->dimension_names[dim] );
    }
    else
    {
        for_less( dim, 0, n_dims )
            out_dim_names[dim] = create_string( vol_dim_names[dim] );
    }

    /*--- using the dimension names, create output sizes */

    n_found = 0;
    for_less( i, 0, n_dims )
        done[i] = FALSE;

    for_less( i, 0, n_dims )
    {
        for_less( j, 0, n_dims )
        {
            if( !done[j] && equal_strings( vol_dim_names[i], out_dim_names[j] ))
            {
                out_sizes[j] = vol_sizes[i];
                ++n_found;
                done[j] = TRUE;
            }
        }
    }

    delete_dimension_names( volume, vol_dim_names );

    if( n_found != n_dims )
    {
        handle_internal_error(
                    "Open_cache: Volume dimension name do not match.\n" );
    }

    nc_data_type = get_volume_nc_data_type( volume, &signed_flag );
    get_volume_voxel_range( volume, &voxel_min, &voxel_max );
    get_volume_real_range( volume, &min_value, &max_value );

    set_default_minc_output_options( &options );
    set_minc_output_real_range( &options, min_value, max_value );

    /*--- open the file for writing */

    out_minc_file = initialize_minc_output( output_filename,
                                        n_dims, out_dim_names, out_sizes,
                                        nc_data_type, signed_flag,
                                        voxel_min, voxel_max,
                                        get_voxel_to_world_transform(volume),
                                        volume, &options );

    out_minc_file->converting_to_colour = FALSE;

    /*--- make temp file disappear when the volume is deleted */

    if( string_length( cache->output_filename ) == 0 )
        remove_file( output_filename );

    /*--- if the volume was previously reading a file, copy the volume to
          the output and close the input file */

    if( cache->minc_file != NULL )
    {
        (void) output_minc_volume( out_minc_file );

        (void) close_minc_input( (Minc_file) cache->minc_file );

        cache->must_read_blocks_before_use = TRUE;
    }
    else
        check_minc_output_variables( out_minc_file );

    cache->minc_file = out_minc_file;

    delete_string( output_filename );
    for_less( dim, 0, n_dims )
        delete_string( out_dim_names[dim] );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : set_cache_volume_file_offset
@INPUT      : cache
              volume
              file_offset
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Sets the offset in the file for writing volumes.  Used when
              writing several cached volumes to a file.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  set_cache_volume_file_offset(
    volume_cache_struct   *cache,
    Volume                volume,
    long                  file_offset[] )
{
    BOOLEAN  changed;
    int      dim;

    changed = FALSE;

    for_less( dim, 0, MAX_DIMENSIONS )
    {
        if( cache->file_offset[dim] != (int) file_offset[dim] )
            changed = TRUE;

        cache->file_offset[dim] = (int) file_offset[dim];
    }

    if( changed )
        flush_cache_blocks( cache, volume, FALSE );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : read_cache_block
@INPUT      : cache
              volume
              block
              block_start
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Reads one cache block.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  void  read_cache_block(
    volume_cache_struct  *cache,
    Volume               volume,
    cache_block_struct   *block,
    int                  block_start[] )
{
    Minc_file        minc_file;
    int              dim, ind, n_dims;
    int              sizes[MAX_DIMENSIONS];
    int              file_start[MAX_DIMENSIONS];
    int              file_count[MAX_DIMENSIONS];
    void             *array_data_ptr;

    minc_file = (Minc_file) cache->minc_file;

    get_volume_sizes( volume, sizes );

    for_less( dim, 0, minc_file->n_file_dimensions )
    {
        ind = minc_file->to_volume_index[dim];
        if( ind >= 0 )
        {
            file_start[dim] = cache->file_offset[dim] + block_start[ind];
            file_count[dim] = MIN( sizes[ind] - file_start[dim],
                                   cache->block_sizes[ind] );
        }
        else
        {
            file_start[dim] = cache->file_offset[dim];
            file_count[dim] = 0;
        }
    }

    n_dims = cache->n_dimensions;
    GET_MULTIDIM_PTR( array_data_ptr, block->array, 0, 0, 0, 0, 0 );

    (void) input_minc_hyperslab( (Minc_file) cache->minc_file,
                                 get_multidim_data_type(&block->array),
                                 n_dims, cache->block_sizes, array_data_ptr,
                                 minc_file->to_volume_index,
                                 file_start, file_count );

    block->modified_flag = FALSE;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : appropriate_a_cache_block
@INPUT      : cache
              volume
@OUTPUT     : block
@RETURNS    : 
@DESCRIPTION: Finds an available cache block, either by allocating one, or
              stealing the least recently used one.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  cache_block_struct  *appropriate_a_cache_block(
    volume_cache_struct  *cache,
    Volume               volume )
{
    int                 block_start[MAX_DIMENSIONS];
    cache_block_struct  *block;

    /*--- if can allocate more blocks, do so */

    if( cache->n_blocks < cache->max_blocks )
    {
        ALLOC( block, 1 );

        create_multidim_array( &block->array, 1, &cache->total_block_size,
                               get_volume_data_type(volume) );

        ++cache->n_blocks;
    }
    else  /*--- otherwise, steal the least-recently used block */
    {
        block = cache->tail;

        if( block->modified_flag )
        {
            get_block_start( cache, block->block_index, block_start );
            write_cache_block( cache, volume, block, block_start );
        }

        /*--- remove from used list */

        if( block->prev_used == NULL )
            cache->head = block->next_used;
        else
            block->prev_used->next_used = block->next_used;

        if( block->next_used == NULL )
            cache->tail = block->prev_used;
        else
            block->next_used->prev_used = block->prev_used;

        /*--- remove from hash table */

        *block->prev_hash = block->next_hash;
        if( block->next_hash != NULL )
            block->next_hash->prev_hash = block->prev_hash;
    }

    return( block );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : hash_block_index
@INPUT      : key
              table_size
@OUTPUT     : 
@RETURNS    : hash address
@DESCRIPTION: Hashes a block index key into a table index, using 
              multiplicative hashing.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  int  hash_block_index(
    int  key,
    int  table_size )
{
    int    index;
    Real   v;

    v = (Real) key * HASH_FUNCTION_CONSTANT;
    
    index = (int) (( v - (Real) ((int) v)) * (Real) table_size);

    return( index );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : get_cache_block_for_voxel
@INPUT      : volume
              x
              y
              z
              t
              v
@OUTPUT     : offset
@RETURNS    : pointer to cache block
@DESCRIPTION: Finds the cache block corresponding to a given voxel, and
              modifies the voxel indices to be block indices.  This function
              gets called for every set or get voxel value, so it must be
              efficient.  On return, offset contains the integer offset
              of the voxel within the cache block.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  cache_block_struct  *get_cache_block_for_voxel(
    Volume   volume,
    int      x,
    int      y,
    int      z,
    int      t,
    int      v,
    int      *offset )
{
    cache_block_struct   *block;
    cache_lookup_struct  *lookup0, *lookup1, *lookup2, *lookup3, *lookup4;
    int                  block_index;
    int                  block_start[MAX_DIMENSIONS];
    int                  n_dims, hash_index;
    volume_cache_struct  *cache;

    cache = &volume->cache;
    n_dims = cache->n_dimensions;

    switch( n_dims )
    {
    case 1:
        lookup0 = &cache->lookup[0][x];
        block_index = lookup0->block_index_offset;
        *offset = lookup0->block_offset;
        break;

    case 2:
        lookup0 = &cache->lookup[0][x];
        lookup1 = &cache->lookup[1][y];
        block_index = lookup0->block_index_offset +
                      lookup1->block_index_offset;
        *offset = lookup0->block_offset +
                  lookup1->block_offset;
        break;

    case 3:
        lookup0 = &cache->lookup[0][x];
        lookup1 = &cache->lookup[1][y];
        lookup2 = &cache->lookup[2][z];
        block_index = lookup0->block_index_offset +
                      lookup1->block_index_offset +
                      lookup2->block_index_offset;
        *offset = lookup0->block_offset +
                  lookup1->block_offset +
                  lookup2->block_offset;
        break;

    case 4:
        lookup0 = &cache->lookup[0][x];
        lookup1 = &cache->lookup[1][y];
        lookup2 = &cache->lookup[2][z];
        lookup3 = &cache->lookup[3][t];
        block_index = lookup0->block_index_offset +
                      lookup1->block_index_offset +
                      lookup2->block_index_offset +
                      lookup3->block_index_offset;
        *offset = lookup0->block_offset +
                  lookup1->block_offset +
                  lookup2->block_offset +
                  lookup3->block_offset;
        break;

    case 5:
        lookup0 = &cache->lookup[0][x];
        lookup1 = &cache->lookup[1][y];
        lookup2 = &cache->lookup[2][z];
        lookup3 = &cache->lookup[3][t];
        lookup4 = &cache->lookup[4][v];
        block_index = lookup0->block_index_offset +
                      lookup1->block_index_offset +
                      lookup2->block_index_offset +
                      lookup3->block_index_offset +
                      lookup4->block_index_offset;
        *offset = lookup0->block_offset +
                  lookup1->block_offset +
                  lookup2->block_offset +
                  lookup3->block_offset +
                  lookup4->block_offset;
        break;
    }

    /*--- if this is the same as the last access, just return the last
          block accessed */

    if( block_index == cache->previous_block_index )
        return( cache->previous_block );

    /*--- search the hash table for the block index */

    hash_index = hash_block_index( block_index, cache->hash_table_size );

    block = cache->hash_table[hash_index];

    while( block != NULL && block->block_index != block_index )
    {
        block = block->next_hash;
    }

    /*--- check if it was found in the hash table */

    if( block == NULL )
    {
        /*--- find a block to use */

        block = appropriate_a_cache_block( cache, volume );
        block->block_index = block_index;

        /*--- check if the block must be initialized from a file */

        if( cache->must_read_blocks_before_use )
        {
            get_block_start( cache, block_index, block_start );
            read_cache_block( cache, volume, block, block_start );
        }

        /*--- insert the block in cache hash table */

        block->next_hash = cache->hash_table[hash_index];
        if( block->next_hash != NULL )
            block->next_hash->prev_hash = &block->next_hash;
        block->prev_hash = &cache->hash_table[hash_index];
        *block->prev_hash = block;

        /*--- insert the block at the head of the used list */

        block->prev_used = NULL;
        block->next_used = cache->head;

        if( cache->head == NULL )
            cache->tail = block;
        else
            cache->head->prev_used = block;

        cache->head = block;
    }
    else   /*--- block was found in hash table */
    {
        /*--- move block to head of used list */

        if( block != cache->head )
        {
            block->prev_used->next_used = block->next_used;
            if( block->next_used != NULL )
                block->next_used->prev_used = block->prev_used;
            else
                cache->tail = block->prev_used;

            cache->head->prev_used = block;
            block->prev_used = NULL;
            block->next_used = cache->head;
            cache->head = block;
        }

        /*--- move block to beginning of hash chain, so if next access to
              this block, we will save some time */

        if( cache->hash_table[hash_index] != block )
        {
            /*--- remove it from where it is */

            *block->prev_hash = block->next_hash;
            if( block->next_hash != NULL )
                block->next_hash->prev_hash = block->prev_hash;

            /*--- place it at the front of the list */
                
            block->next_hash = cache->hash_table[hash_index];
            if( block->next_hash != NULL )
                block->next_hash->prev_hash = &block->next_hash;
            block->prev_hash = &cache->hash_table[hash_index];
            *block->prev_hash = block;
        }
    }

    /*--- record so if next access is to same block, we save some time */

    cache->previous_block = block;
    cache->previous_block_index = block_index;

    return( cache->previous_block );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : get_cached_volume_voxel
@INPUT      : volume
              x
              y
              z
              t
              v
@OUTPUT     : 
@RETURNS    : voxel value
@DESCRIPTION: Finds the voxel value for the given voxel in a cached volume.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  Real  get_cached_volume_voxel(
    Volume   volume,
    int      x,
    int      y,
    int      z,
    int      t,
    int      v )
{
    int                  offset;
    Real                 value;
    cache_block_struct   *block;

    if( volume->cache.minc_file == NULL )
        return( get_volume_voxel_min( volume ) );

    block = get_cache_block_for_voxel( volume, x, y, z, t, v, &offset );

    GET_MULTIDIM_1D( value, block->array, offset );

    return( value );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : set_cached_volume_voxel
@INPUT      : volume
              x
              y
              z
              t
              v
              value
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Sets the voxel value for the given voxel in a cached volume.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  set_cached_volume_voxel(
    Volume   volume,
    int      x,
    int      y,
    int      z,
    int      t,
    int      v,
    Real     value )
{
    int                  offset;
    cache_block_struct   *block;

    if( !volume->cache.output_file_is_open )
    {
        open_cache_volume_output_file( &volume->cache, volume );
        volume->cache.output_file_is_open = TRUE;
    }

    block = get_cache_block_for_voxel( volume, x, y, z, t, v, &offset );

    block->modified_flag = TRUE;

    SET_MULTIDIM_1D( block->array, offset, value );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : cached_volume_has_been_modified
@INPUT      : cache
@OUTPUT     : 
@RETURNS    : TRUE if the volume has been modified since creation
@DESCRIPTION: Determines if the volume has been modified.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Sep. 1, 1995    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  BOOLEAN  cached_volume_has_been_modified(
    volume_cache_struct  *cache )
{
    return( cache->minc_file != NULL );
}