/***************************************************************************
 *  tests/mng/test_config.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/mng>
#include <stxxl/bits/verbose.h>

int main()
{
    // test disk_config parser:

    stxxl::disk_config cfg;

    cfg.parseline("disk=/var/tmp/stxxl.tmp,100GiB,syscall unlink direct=on");

    STXXL_CHECK( cfg.path == "/var/tmp/stxxl.tmp" );
    STXXL_CHECK( cfg.size == 100 * 1024 * 1024 * stxxl::uint64(1024) );
    STXXL_CHECK( cfg.fileio_string() == "syscall direct=on unlink_on_open" );

    // test disk_config parser:

    cfg.parseline("disk=/var/tmp/stxxl.tmp,100GiB,wincall queue=5 delete_on_exit nodirect");

    STXXL_CHECK( cfg.path == "/var/tmp/stxxl.tmp" );
    STXXL_CHECK( cfg.size == 100 * 1024 * 1024 * stxxl::uint64(1024) );
    STXXL_CHECK( cfg.fileio_string() == "wincall delete_on_exit direct=off queue=5" );
    STXXL_CHECK( cfg.queue == 5 );

    // bad configurations

    STXXL_CHECK_THROW(
        cfg.parseline("disk=/var/tmp/stxxl.tmp,100GiB,wincall_fileperblock unlink direct=on"),
        std::runtime_error
        );

    STXXL_CHECK_THROW(
        cfg.parseline("disk=/var/tmp/stxxl.tmp,0x,syscall"),
        std::runtime_error
        );

    return 0;
}
