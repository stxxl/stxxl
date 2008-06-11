#include "stxxl/vector"
#include "stxxl/scan"

#include <iostream>
#include <algorithm>

//! \example algo/test_scan.cpp
//! This is an example of how to use \c stxxl::for_each() and \c stxxl::find() algorithms

using stxxl::int64;
using stxxl::stxxl_timestamp;


template <typename type>
struct counter
{
    type value;
    counter(type v = type(0)) : value(v) { }
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
    void operator () (type & arg)
    {
        arg = arg * arg;
    }
};

template <typename type>
struct fill_value
{
    type val;
    fill_value(const type &v_) : val(v_) { }

    type operator () ()
    {
        return val;
    }
};

int main()
{
    stxxl::vector<int64>::size_type i;
    stxxl::vector<int64> v(64 * int64(1024 * 1024));
    double b, e;

    STXXL_MSG("write " << (v.end() - v.begin()) << " elements ...");

    stxxl::generate(v.begin(), v.end(), counter<int64>(), 4);


    STXXL_MSG("for_each_m ...");
    b = stxxl_timestamp();
    stxxl::for_each_m(v.begin(), v.end(), square<int64>(), 4);
    e = stxxl_timestamp();
    STXXL_MSG("for_each_m time: " << (e - b));


    STXXL_MSG("check");
    for (i = 0; i < v.size(); ++i)
    {
        if (v[i] != int64(i * i) )
            STXXL_MSG("Error at position " << i);
    }

    STXXL_MSG("Pos of value    1023: " << (stxxl::find(v.begin(), v.end(), 1023, 4) - v.begin()));
    STXXL_MSG("Pos of value 1048576: " << (stxxl::find(v.begin(), v.end(), 1024 * 1024, 4) - v.begin()));
    STXXL_MSG("Pos of value    1024: " << (stxxl::find(v.begin(), v.end(), 32 * 32, 4) - v.begin()));

    STXXL_MSG("generate ...");
    b = stxxl_timestamp();

    stxxl::generate(v.begin() + 1, v.end() - 1, fill_value<int64>(555), 4);
    e = stxxl_timestamp();
    STXXL_MSG("generate: " << (e - b));


    STXXL_MSG("check");
    if (v[0] != 0)
        STXXL_MSG("Error at position " << i);
    if (v[v.size() - 1] != int64((v.size() - 1) * (v.size() - 1)) )
        STXXL_MSG("Error at position " << i);

    for (i = 1; i < v.size() - 1; ++i)
    {
        if (v[i] != 555 )
            STXXL_MSG("Error at position " << i);
    }

    return 0;
}
