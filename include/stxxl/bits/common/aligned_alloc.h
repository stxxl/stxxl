/***************************************************************************
 *  include/stxxl/bits/common/aligned_alloc.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_ALIGNED_ALLOC
#define STXXL_ALIGNED_ALLOC

#include <stxxl/bits/common/utils.h>


#ifndef STXXL_VERBOSE_ALIGNED_ALLOC
#define STXXL_VERBOSE_ALIGNED_ALLOC STXXL_VERBOSE1
#endif

__STXXL_BEGIN_NAMESPACE

//                       meta_info
//                          aligned begin of data
//                       v  v
//  ----+--------------#+---*-------------------------
//      ^              ^^
//      buffer          result
//                     pointer to buffer
template <size_t ALIGNMENT>
inline void * aligned_alloc(size_t size, size_t meta_info_size = 0)
{
    STXXL_VERBOSE2("stxxl::aligned_alloc<" << ALIGNMENT << ">(), size = " << size << ", meta info size = " << meta_info_size);
    char * buffer = new char[size + ALIGNMENT + sizeof(char *) + meta_info_size];
    #ifdef STXXL_ALIGNED_CALLOC
    memset(buffer, 0, size + ALIGNMENT + sizeof(char *) + meta_info_size);
    #endif
    char * reserve_buffer = buffer + sizeof(char *) + meta_info_size;
    char * result = reserve_buffer + ALIGNMENT -
                    (((unsigned long)reserve_buffer) % (ALIGNMENT)) - meta_info_size;
    STXXL_VERBOSE2("stxxl::aligned_alloc<" << ALIGNMENT << ">() address " << (void *)result << " lost " << (result - buffer) << " bytes");
    assert(int(result - buffer) >= int(sizeof(char *)));
    *(((char **)result) - 1) = buffer;
    STXXL_VERBOSE2("stxxl::aligned_alloc<" << ALIGNMENT << ">(), allocated at " << (void *)buffer << " returning " << (void *)result);
    STXXL_VERBOSE_ALIGNED_ALLOC("stxxl::aligned_alloc<" << ALIGNMENT
            << ">(size = " << size << ", meta info size = " << meta_info_size
            << ") => buffer = " << (void *)buffer << ", ptr = " << (void *)result);

    return result;
}

template <size_t ALIGNMENT>
inline void
aligned_dealloc(void * ptr)
{
    char * buffer = * (((char **)ptr) - 1);
    STXXL_VERBOSE_ALIGNED_ALLOC("stxxl::aligned_dealloc<" << ALIGNMENT << ">(), ptr = " << ptr << ", buffer = " << (void *)buffer);
    delete[] buffer;
}

__STXXL_END_NAMESPACE

#endif // !STXXL_ALIGNED_ALLOC
// vim: et:ts=4:sw=4
