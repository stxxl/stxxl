/***************************************************************************
 *  include/stxxl/bits/utils/malloc.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MALLOC_H
#define STXXL_MALLOC_H

#include <ostream>
#ifdef __FreeBSD__
    #include <stdlib.h>
#else
    #include <malloc.h>
#endif
#include <cstdlib>

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/common/utils.h>


__STXXL_BEGIN_NAMESPACE


//! \brief Access to some useful malloc statistics

//! malloc is default C++ allocator
class malloc_stats
{
#if !defined(__APPLE__) && !defined(__FreeBSD__)

public:
    typedef int return_type;

    //! \brief Returns number of bytes allocated from system not including mmapped regions
    return_type from_system_nmmap() const
    {
        struct mallinfo info = mallinfo();
        return info.arena;
    }

    //! \brief Returns number of free chunks
    return_type free_chunks() const
    {
        struct mallinfo info = mallinfo();
        return info.ordblks;
    }

    //! \brief Number of bytes allocated and in use
    return_type used() const
    {
        struct mallinfo info = mallinfo();
        return info.uordblks;
    }

    //! \brief Number of bytes allocated but not in use
    return_type not_used() const
    {
        struct mallinfo info = mallinfo();
        return info.fordblks;
    }

    //! \brief Top-most, releasable (via malloc_trim) space (bytes)
    return_type releasable() const
    {
        struct mallinfo info = mallinfo();
        return info.keepcost;
    }

    //! \brief Maximum total allocated space (bytes) (always 0 ?)
    return_type max_allocated() const
    {
        struct mallinfo info = mallinfo();
        return info.usmblks;
    }

    //! \brief Number of fastbin blocks
    return_type fastbin_blocks() const
    {
        struct mallinfo info = mallinfo();
        return info.smblks;
    }

    //! \brief Space available in freed fastbin blocks (bytes)
    return_type fastbin_free() const
    {
        struct mallinfo info = mallinfo();
        return info.fsmblks;
    }

    //! \brief Returns number of bytes allocated from system using mmap
    return_type from_system_mmap() const
    {
        struct mallinfo info = mallinfo();
        return info.hblkhd;
    }

    //! \brief Number of chunks allocated via mmap()
    return_type mmap_chunks() const
    {
        struct mallinfo info = mallinfo();
        return info.hblks;
    }

    //! \brief Returns \b total number of bytes allocated from system including mmapped regions
    return_type from_system_total() const
    {
        return from_system_nmmap() + from_system_mmap();
    }
#endif
};

//! \brief Prints current malloc statistics in a convenient way
inline std::ostream & operator << (std::ostream & s, const malloc_stats & st)
{
#if !defined(__APPLE__) && !defined(__FreeBSD__)
    s << "MALLOC statistics" << std::endl;
    s << "=================================================================" << std::endl;
    s << "Space allocated from system not using mmap: " << st.from_system_nmmap() << " bytes" << std::endl;
    s << "       number of free chunks                       : " << st.free_chunks() << std::endl;
    s << "       space allocated and in use                  : " << st.used() << " bytes" << std::endl;
    s << "       space allocated but not in use              : " << st.not_used() << " bytes" << std::endl;
    s << "       top-most, releasable (via malloc_trim) space: " << st.releasable() << " bytes" << std::endl;
    s << "       maximum total allocated space (?)           : " << st.max_allocated() << " bytes" << std::endl;
    s << "   FASTBIN blocks " << std::endl;
    s << "       number of fastbin blocks: " << st.fastbin_blocks() << std::endl;
    s << "       space available in freed fastbin blocks: " << st.fastbin_free() << " bytes" << std::endl;
    s << "Space allocated from system using mmap: " << st.from_system_mmap() << " bytes" << std::endl;
    s << "       number of chunks allocated via mmap(): " << st.mmap_chunks() << std::endl;
    s << "Total space allocated from system (mmap and not mmap): " <<
    st.from_system_total() << " bytes" << std::endl;
    s << "=================================================================" << std::endl;
#else
    s << "MALLOC statistics are not supported on this platform";
    STXXL_UNUSED(st);
#endif
    return s;
}

class malloc_setup
{ };

__STXXL_END_NAMESPACE

#endif // !STXXL_MALLOC_H
