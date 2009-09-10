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

#ifndef STXXL_USE_MEMALIGN
#define STXXL_USE_MEMALIGN 0
#endif

#if STXXL_USE_MEMALIGN
#include <cstdlib>
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
inline void * aligned_alloc(size_t size, size_t meta_info_size)
{
    if (meta_info_size == 0)
        return aligned_alloc<ALIGNMENT>(size);

#if STXXL_USE_MEMALIGN
    if (meta_info_size != 0)
        throw "aligned_alloc: meta_info_size != 0";
    void * result;
    if (posix_memalign(&result, ALIGNMENT, size) != 0)
        throw std::bad_alloc();
#else
    STXXL_VERBOSE2("stxxl::aligned_alloc<" << ALIGNMENT << ">(), size = " << size << ", meta info size = " << meta_info_size);
    size_t alloc_size = ALIGNMENT + sizeof(char *) + meta_info_size + size;
    char * buffer = (char *)std::malloc(alloc_size);
    if (buffer == NULL)
        throw std::bad_alloc();
    #ifdef STXXL_ALIGNED_CALLOC
    memset(buffer, 0, alloc_size);
    #endif
    char * reserve_buffer = buffer + sizeof(char *) + meta_info_size;
    char * result = reserve_buffer + ALIGNMENT -
                    (((unsigned long)reserve_buffer) % (ALIGNMENT)) - meta_info_size;
    STXXL_VERBOSE2("stxxl::aligned_alloc<" << ALIGNMENT << ">() address " << (void *)result << " lost " << (result - buffer) << " bytes");
    assert(int(result - buffer) >= int(sizeof(char *)));

    // free unused memory behind the data area
    // so access behind the requested size can be recognized
    size_t realloc_size = (result - buffer) + meta_info_size + size;
    char * realloced = (char *)std::realloc(buffer, realloc_size);
    if (buffer != realloced)
        throw std::bad_alloc();
    assert(result + size <= buffer + realloc_size);

    *(((char **)result) - 1) = buffer;
    STXXL_VERBOSE2("stxxl::aligned_alloc<" << ALIGNMENT << ">(), allocated at " << (void *)buffer << " returning " << (void *)result);
    STXXL_VERBOSE_ALIGNED_ALLOC("stxxl::aligned_alloc<" << ALIGNMENT
            << ">(size = " << size << ", meta info size = " << meta_info_size
            << ") => buffer = " << (void *)buffer << ", ptr = " << (void *)result);
#endif

    return result;
}

template <size_t ALIGNMENT>
inline void * aligned_alloc(size_t size)
{
#if STXXL_USE_MEMALIGN
    void * result;
    if (posix_memalign(&result, ALIGNMENT, size) != 0)
        throw std::bad_alloc();
#else
    STXXL_VERBOSE2("stxxl::aligned_alloc<" << ALIGNMENT << ">(), size = " << size);
    size_t alloc_size = ALIGNMENT + sizeof(char *) + size;
    char * buffer = (char *)std::malloc(alloc_size);
    if (buffer == NULL)
        throw std::bad_alloc();
    #ifdef STXXL_ALIGNED_CALLOC
    memset(buffer, 0, alloc_size);
    #endif
    char * reserve_buffer = buffer + sizeof(char *);
    char * result = reserve_buffer + ALIGNMENT -
                    (((unsigned long)reserve_buffer) % (ALIGNMENT));
    STXXL_VERBOSE2("stxxl::aligned_alloc<" << ALIGNMENT << ">() address " << (void *)result << " lost " << (result - buffer) << " bytes");
    assert(int(result - buffer) >= int(sizeof(char *)));

    // free unused memory behind the data area
    // so access behind the requested size can be recognized
    size_t realloc_size = (result - buffer) + size;
    char * realloced = (char *)std::realloc(buffer, realloc_size);
    if (buffer != realloced)
        throw std::bad_alloc();
    assert(result + size <= buffer + realloc_size);

    *(((char **)result) - 1) = buffer;
    STXXL_VERBOSE2("stxxl::aligned_alloc<" << ALIGNMENT << ">(), allocated at " << (void *)buffer << " returning " << (void *)result);
    STXXL_VERBOSE_ALIGNED_ALLOC("stxxl::aligned_alloc<" << ALIGNMENT
            << ">(size = " << size
            << ") => buffer = " << (void *)buffer << ", ptr = " << (void *)result);
#endif

    return result;
}

template <size_t ALIGNMENT>
inline void
aligned_dealloc(void * ptr)
{
#if STXXL_USE_MEMALIGN
    std::free(ptr);
#else
    char * buffer = * (((char **)ptr) - 1);
    STXXL_VERBOSE_ALIGNED_ALLOC("stxxl::aligned_dealloc<" << ALIGNMENT << ">(), ptr = " << ptr << ", buffer = " << (void *)buffer);
    std::free(buffer);
#endif
}

__STXXL_END_NAMESPACE

#endif // !STXXL_ALIGNED_ALLOC
// vim: et:ts=4:sw=4
