/***************************************************************************
 *  include/stxxl/bits/common/aligned_alloc.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_ALIGNED_ALLOC
#define STXXL_ALIGNED_ALLOC

#include <cstdlib>
#include <stxxl/bits/common/utils.h>


#ifndef STXXL_VERBOSE_ALIGNED_ALLOC
#define STXXL_VERBOSE_ALIGNED_ALLOC STXXL_VERBOSE1
#endif

__STXXL_BEGIN_NAMESPACE

template <typename must_be_int>
struct aligned_alloc_settings {
    static bool may_use_realloc;
};

template <typename must_be_int>
bool aligned_alloc_settings<must_be_int>::may_use_realloc = true;

// meta_info_size > 0 is needed for array allocations that have overhead
//
//                      meta_info
//                          aligned begin of data   unallocated behind data
//                      v   v                       v
//  ----===============#MMMM========================------
//      ^              ^^                           ^
//      buffer          result                      result+m_i_size+size
//                     pointer to buffer
// (---) unallocated, (===) allocated memory

template <size_t ALIGNMENT>
inline void * aligned_alloc(size_t size, size_t meta_info_size = 0)
{
    STXXL_VERBOSE2("stxxl::aligned_alloc<" << ALIGNMENT << ">(), size = " << size << ", meta info size = " << meta_info_size);
#if !defined(STXXL_WASTE_MORE_MEMORY_FOR_IMPROVED_ACCESS_AFTER_ALLOCATED_MEMORY_CHECKS)
    // malloc()/realloc() variant that frees the unused amount of memory
    // after the data area of size 'size'. realloc() from valgrind does not
    // preserve the old memory area when shrinking, so out-of-bounds
    // accesses can't be detected easily.
    // Overhead: about ALIGNMENT bytes.
    size_t alloc_size = ALIGNMENT + sizeof(char *) + meta_info_size + size;
    char * buffer = (char *)std::malloc(alloc_size);
#else
    // More space consuming and memory fragmenting variant using
    // posix_memalign() instead of malloc()/realloc(). Ensures that the end
    // of the data area (of size 'size') will match the end of the allocated
    // block, so no corrections are neccessary and
    // access-behind-allocated-memory problems can be easily detected by
    // valgrind. Usually produces an extra memory fragment of about
    // ALIGNMENT bytes.
    // Overhead: about 2 * ALIGNMENT bytes.
    size_t alloc_size = ALIGNMENT * STXXL_DIVRU(sizeof(char *) + meta_info_size, ALIGNMENT) + size;
    char * buffer;
    if (posix_memalign((void **)&buffer, ALIGNMENT, alloc_size) != 0)
        throw std::bad_alloc();
#endif
    if (buffer == NULL)
        throw std::bad_alloc();
    #ifdef STXXL_ALIGNED_CALLOC
    memset(buffer, 0, alloc_size);
    #endif
    char * reserve_buffer = buffer + sizeof(char *) + meta_info_size;
    char * result = reserve_buffer + ALIGNMENT -
                    (((unsigned long)reserve_buffer) % (ALIGNMENT)) - meta_info_size;
    STXXL_VERBOSE2("stxxl::aligned_alloc<" << ALIGNMENT << ">() address " << (void *)result << " lost " << (result - buffer) << " bytes");
    assert(long(result - buffer) >= long(sizeof(char *)));

    // free unused memory behind the data area
    // so access behind the requested size can be recognized
    size_t realloc_size = (result - buffer) + meta_info_size + size;
    if (realloc_size < alloc_size && aligned_alloc_settings<int>::may_use_realloc) {
        char * realloced = (char *)std::realloc(buffer, realloc_size);
        if (buffer != realloced) {
            // hmm, realloc does move the memory block around while shrinking,
            // might run under valgrind, so disable realloc and retry
            STXXL_ERRMSG("stxxl::aligned_alloc: disabling realloc()");
            std::free(realloced);
            aligned_alloc_settings<int>::may_use_realloc = false;
            return aligned_alloc<ALIGNMENT>(size, meta_info_size);
        }
        assert(result + size <= buffer + realloc_size);
    }

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
    if (!ptr)
        return;
    char * buffer = * (((char **)ptr) - 1);
    STXXL_VERBOSE_ALIGNED_ALLOC("stxxl::aligned_dealloc<" << ALIGNMENT << ">(), ptr = " << ptr << ", buffer = " << (void *)buffer);
    std::free(buffer);
}

__STXXL_END_NAMESPACE

#endif // !STXXL_ALIGNED_ALLOC
// vim: et:ts=4:sw=4
