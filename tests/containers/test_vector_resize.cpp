/***************************************************************************
 *  tests/containers/test_vector_resize.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2015 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define STXXL_DEFAULT_BLOCK_SIZE(T) 4096

#include <tlx/logger.hpp>

#include <foxxll/io.hpp>

#include <stxxl/vector>

using vector_type = stxxl::vector<int, 4, stxxl::lru_pager<4> >;

int main()
{
    foxxll::config* config = foxxll::config::get_instance();

    foxxll::disk_config disk1("/tmp/stxxl-$$.tmp", 16 * STXXL_DEFAULT_BLOCK_SIZE(int),
                              "syscall autogrow=no");
    disk1.unlink_on_open = true;
    disk1.direct = foxxll::disk_config::DIRECT_OFF;
    config->add_disk(disk1);

    vector_type* vector = new vector_type();

    try {
        while (true)
            vector->push_back(0);
    }
    catch (std::exception& e) {
        LOG1 << "Caught exception: " << e.what();
        delete vector; // here it will crash in block_manager::delete_block(bid)
    }

    LOG1 << "Delete done, all is well.";

    return 0;
}
