/***************************************************************************
 *  local/stats_test.cpp
 *
 *  This is an example file included in the local/ directory of STXXL. All .cpp
 *  files in local/ are automatically compiled and linked with STXXL by CMake.
 *  You can use this method for simple prototype applications.
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2016 Alex Schickedanz <alex@ae.cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iostream>

#include <stxxl/sequence>
#include <foxxll/io/iostats.hpp>

int main()
{
    foxxll::stats* Stats = foxxll::stats::get_instance();

    foxxll::stats_data stats_begin(*Stats);

    stxxl::sequence<uint64_t> seq(60, 60);

    for(uint64_t i = 0; i < (1llu << 26); ++i)
        seq.push_back(i);

    for(auto stream = seq.get_stream(); !stream.empty(); ++stream){}

    std::cout << "--------------------------------------------" << std::endl;
    std::cout << (foxxll::stats_data(*Stats) - stats_begin);
}
