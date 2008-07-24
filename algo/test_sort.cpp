/***************************************************************************
 *  algo/test_sort.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example algo/test_sort.cpp
//! This is an example of how to use \c stxxl::sort() algorithm

#include <stxxl/mng>
#include <stxxl/sort>
#include <stxxl/vector>


#define RECORD_SIZE 8

struct my_type
{
    typedef unsigned key_type;

    key_type _key;
    char _data[RECORD_SIZE - sizeof(key_type)];
    key_type key() const
    {
        return _key;
    }

    my_type() { }
    my_type(key_type __key) : _key(__key) { }

    static my_type min_value()
    {
        return my_type(std::numeric_limits<key_type>::min());
    }
    static my_type max_value()
    {
        return my_type(std::numeric_limits<key_type>::max());
    }

    ~my_type() { }
};

std::ostream & operator << (std::ostream & o, const my_type & obj)
{
    o << obj._key;
    return o;
}

bool operator < (const my_type & a, const my_type & b)
{
    return a.key() < b.key();
}

bool operator == (const my_type & a, const my_type & b)
{
    return a.key() == b.key();
}

bool operator != (const my_type & a, const my_type & b)
{
    return a.key() != b.key();
}

struct cmp : public std::less<my_type>
{
    my_type min_value() const
    {
        return my_type::min_value();
    }
    my_type max_value() const
    {
        return my_type::max_value();
    }
};


int main()
{
    unsigned memory_to_use = 128 * 1024 * 1024;
    typedef stxxl::vector<my_type> vector_type;
    const stxxl::int64 n_records =
        stxxl::int64(384) * stxxl::int64(1024 * 1024) / sizeof(my_type);
    vector_type v(n_records);

    stxxl::random_number32 rnd;
    STXXL_MSG("Filling vector..., input size =" << v.size());
    for (vector_type::size_type i = 0; i < v.size(); i++)
        v[i]._key = 1 + (rnd() % 0xfffffff);


    STXXL_MSG("Checking order...");
    STXXL_MSG(((stxxl::is_sorted(v.begin(), v.end())) ? "OK" : "WRONG"));

    STXXL_MSG("Sorting...");
    stxxl::sort(v.begin(), v.end(), cmp(), memory_to_use);

    STXXL_MSG("Checking order...");
    STXXL_MSG(((stxxl::is_sorted(v.begin(), v.end())) ? "OK" : "WRONG"));


    STXXL_MSG("Done, output size=" << v.size());

    return 0;
}
