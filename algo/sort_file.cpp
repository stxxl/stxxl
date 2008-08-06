/***************************************************************************
 *  algo/sort_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example algo/sort_file.cpp
//! This example imports a file into an \c stxxl::vector without copying its
//! content and then sorts it using stxxl::sort.

#include <stxxl/mng>
#include <stxxl/ksort>
#include <stxxl/sort>
#include <stxxl/vector>


struct my_type
{
    typedef unsigned key_type;

    key_type _key;
    char _data[128 - sizeof(key_type)];
    key_type key() const
    {
        return _key;
    }

    my_type() { }
    my_type(key_type __key) : _key(__key) { }

    static my_type min_value()
    {
        return my_type((std::numeric_limits<key_type>::min)());
    }
    static my_type max_value()
    {
        return my_type((std::numeric_limits<key_type>::max)());
    }
};


bool operator < (const my_type & a, const my_type & b)
{
    return a.key() < b.key();
}

struct Cmp
{
    bool operator () (const my_type & a, const my_type & b) const
    {
        return a < b;
    }
    static my_type min_value()
    {
        return my_type::min_value();
    }
    static my_type max_value()
    {
        return my_type::max_value();
    }
};

std::ostream & operator << (std::ostream & o, const my_type & obj)
{
    o << obj._key;
    return o;
}

int main()
{
    stxxl::syscall_file f("./in", stxxl::file::DIRECT | stxxl::file::RDWR);

    unsigned memory_to_use = 50 * 1024 * 1024;
    typedef stxxl::vector<my_type> vector_type;
    {
        vector_type v(&f);

        /*
                STXXL_MSG("Printing...");
                for(stxxl::int64 i=0; i < v.size(); i++)
                        STXXL_MSG(v[i].key());
         */

        STXXL_MSG("Checking order...");
        STXXL_MSG(((stxxl::is_sorted(v.begin(), v.end())) ? "OK" : "WRONG"));

        STXXL_MSG("Sorting...");
        stxxl::ksort(v.begin(), v.end(), memory_to_use);
        //stxxl::sort(v.begin(),v.end()-101,Cmp(),memory_to_use);

        STXXL_MSG("Checking order...");
        STXXL_MSG(((stxxl::is_sorted(v.begin(), v.end())) ? "OK" : "WRONG"));
    }
    STXXL_MSG("OK");

    return 0;
}
