/***************************************************************************
 *  mng/diskallocator.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008,2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/mng/diskallocator.h>


__STXXL_BEGIN_NAMESPACE

void DiskAllocator::dump() const
{
    int64 total = 0;
    sortseq::const_iterator cur = free_space.begin();
    STXXL_ERRMSG("Free regions dump:");
    for ( ; cur != free_space.end(); ++cur)
    {
        STXXL_ERRMSG("Free chunk: begin: " << (cur->first) << " size: " << (cur->second));
        total += cur->second;
    }
    STXXL_ERRMSG("Total bytes: " << total);
}

void DiskAllocator::deallocation_error(
        stxxl::int64 block_pos, stxxl::int64 block_size,
        const sortseq::iterator & pred, const sortseq::iterator & succ) const
{
    STXXL_ERRMSG("Error deallocating block at " << block_pos << " size " << block_size);
    STXXL_ERRMSG(((pred == succ) ? "pred==succ" : "pred!=succ"));
    STXXL_ERRMSG(((pred == free_space.begin()) ? "pred==free_space.begin()" : "pred!=free_space.begin()"));
    STXXL_ERRMSG(((pred == free_space.end()) ? "pred==free_space.end()" : "pred!=free_space.end()"));
    STXXL_ERRMSG(((succ == free_space.begin()) ? "succ==free_space.begin()" : "succ!=free_space.begin()"));
    STXXL_ERRMSG(((succ == free_space.end()) ? "succ==free_space.end()" : "succ!=free_space.end()"));
    dump();
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
