/***************************************************************************
 *  include/stxxl/bits/containers/hash_map/tuning.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2007 Markus Westphal <marwes@users.sourceforge.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_CONTAINERS_HASH_MAP_TUNING_HEADER
#define STXXL_CONTAINERS_HASH_MAP_TUNING_HEADER
#define _STXXL_TUNING_H_

#include <foxxll/mng.hpp>
#include <foxxll/singleton.hpp>

namespace stxxl {
namespace hash_map {

//! Tuning parameters for external memory hash map.
class tuning : public foxxll::singleton<tuning>
{
    friend class foxxll::singleton<tuning>;

public:
    //! see buffered_reader
    size_t prefetch_page_size;
    //! see buffered_reader
    size_t prefetch_pages;
    //! see block_cache and hash_map
    size_t blockcache_size;

private:
    /*! set reasonable default values for tuning params */
    tuning()
        : prefetch_page_size(foxxll::config::get_instance()->disks_number() * 2),
          prefetch_pages(2),
          blockcache_size(foxxll::config::get_instance()->disks_number() * 12)
    { }
};

} // namespace hash_map
} // namespace stxxl

#endif // !STXXL_CONTAINERS_HASH_MAP_TUNING_HEADER
