/***************************************************************************
 *  common/stxxl_info.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright © 2007 Andreas Beckmann <beckmann@mpi-inf.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/mng>

int main(int argc, char **)
{
    stxxl::config::get_instance();
    stxxl::block_manager::get_instance();
    stxxl::stats::get_instance();
    return argc == 1 ? 0 : -1;
}
