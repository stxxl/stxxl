/***************************************************************************
 *  tests/algo/test_scan.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define STXXL_DEFAULT_BLOCK_SIZE(T) 4096

//! \example algo/test_scan.cpp
//! This is an example of how to use \c stxxl::for_each() and \c stxxl::find() algorithms

#include <algorithm>
#include <iostream>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <foxxll/common/die_with_message.hpp>

#include <stxxl/scan>
#include <stxxl/vector>

using foxxll::timestamp;

template <typename type>
struct counter
{
    type value;
    explicit counter(type v = type(0)) : value(v) { }
    type operator () ()
    {
        type old_val = value;
        value++;
        return old_val;
    }
};

template <typename type>
struct square
{
    void operator () (type& arg)
    {
        arg = arg * arg;
    }
};

template <typename type>
struct fill_value
{
    type val;
    explicit fill_value(const type& v_) : val(v_) { }

    type operator () ()
    {
        return val;
    }
};

int main()
{
    stxxl::vector<int64_t>::size_type i;
    stxxl::vector<int64_t> v(128 * STXXL_DEFAULT_BLOCK_SIZE(int64_t));
    double b, e;

    foxxll::stats_data stats_begin(*foxxll::stats::get_instance());

    LOG1 << "write " << (v.end() - v.begin()) << " elements ...";

    stxxl::generate(v.begin(), v.end(), counter<int64_t>(), 4);

    LOG1 << "for_each_m ...";
    b = timestamp();
    stxxl::for_each_m(v.begin(), v.end(), square<int64_t>(), 4);
    e = timestamp();
    LOG1 << "for_each_m time: " << (e - b);

    LOG1 << "check";
    for (i = 0; i < v.size(); ++i)
    {
        die_with_message_unless(v[i] == int64_t(i * i), "Error at position " << i);
    }

    LOG1 << "Pos of value    1023: " << (stxxl::find(v.begin(), v.end(), 1023, 4) - v.begin());
    LOG1 << "Pos of value 1048576: " << (stxxl::find(v.begin(), v.end(), 1024 * 1024, 4) - v.begin());
    LOG1 << "Pos of value    1024: " << (stxxl::find(v.begin(), v.end(), 32 * 32, 4) - v.begin());

    LOG1 << "generate ...";
    b = timestamp();

    stxxl::generate(v.begin() + 1, v.end() - 1, fill_value<int64_t>(555), 4);
    e = timestamp();
    LOG1 << "generate: " << (e - b);

    LOG1 << "check";
    die_with_message_unless(v[0] == 0, "Error at position " << 0);

    die_with_message_unless(v[v.size() - 1] == int64_t((v.size() - 1) * (v.size() - 1)),
                            "Error at position " << v.size() - 1);

    for (i = 1; i < v.size() - 1; ++i)
    {
        die_with_message_unless(v[i] == 555, "Error at position " << i);
    }

    std::cout << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;

    return 0;
}
