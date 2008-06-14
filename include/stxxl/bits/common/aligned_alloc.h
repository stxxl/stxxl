/***************************************************************************
 *            aligned_alloc.h
 *
 *  Sat Aug 24 23:53:12 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#ifndef STXXL_ALIGNED_ALLOC
#define STXXL_ALIGNED_ALLOC

#include "stxxl/bits/common/utils.h"


__STXXL_BEGIN_NAMESPACE

template <size_t ALIGNMENT>
inline void * aligned_alloc (size_t size, size_t meta_info_size = 0)
{
    STXXL_VERBOSE1("stxxl::aligned_alloc<" << ALIGNMENT << ">(), size = " << size << ", meta info size = " << meta_info_size);
    char * buffer = new char[size + ALIGNMENT + sizeof(char *) + meta_info_size];
    char * reserve_buffer = buffer + sizeof(char *) + meta_info_size;
    char * result = reserve_buffer + ALIGNMENT -
                    (((unsigned long) reserve_buffer) % (ALIGNMENT)) - meta_info_size;
    STXXL_VERBOSE1("stxxl::aligned_alloc<" << ALIGNMENT << ">() address 0x" << std::hex << long(result)
                                           << std::dec << " lost " << unsigned(result - buffer) << " bytes");
    assert(int(result - buffer) >= int(sizeof(char *)));
    *(((char * *) result) - 1) = buffer;
    STXXL_VERBOSE1("stxxl::aligned_alloc<" << ALIGNMENT <<
                   ">(), allocated at " << std::hex << ((unsigned long)buffer) << " returning " << ((unsigned long)result)
                                           << std::dec);

    return result;
}

template <size_t ALIGNMENT> inline void
aligned_dealloc (void * ptr)
{
    STXXL_VERBOSE2("stxxl::aligned_dealloc(<" << ALIGNMENT << ">), ptr = 0x" << std::hex << (unsigned long)(ptr) << std::dec);
    delete[] * (((char * *) ptr) - 1);
}

__STXXL_END_NAMESPACE

#endif // !STXXL_ALIGNED_ALLOC
