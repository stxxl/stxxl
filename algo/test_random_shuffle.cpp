/***************************************************************************
 *  algo/test_random_shuffle.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2007 Manuel Krings
 *  Copyright (C) 2007 Markus Westphal
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example algo/test_random_shuffle.cpp
//! Test \c stxxl::random_shuffle()

#include <stxxl/bits/algo/random_shuffle.h>


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


int main()
{
    typedef stxxl::vector<int> ext_vec_type;
    ext_vec_type STXXLVector(1024 * 1024 * 256 / sizeof(int));

    STXXL_MSG("Filling vector with increasing values...");
    stxxl::generate(STXXLVector.begin(), STXXLVector.end(),
                    counter<stxxl::uint64>(), 4);

    stxxl::uint64 i;

    STXXL_MSG("Begin: ");
    for (i = 0; i < 10; i++)
        STXXL_MSG(STXXLVector[i]);

    STXXL_MSG("End: ");
    for (i = STXXLVector.size() - 10; i < STXXLVector.size(); i++)
        STXXL_MSG(STXXLVector[i]);

    STXXL_MSG("Permute randomly...");
    stxxl::random_shuffle(STXXLVector.begin(), STXXLVector.end(), 1024 * 1024 * 128);


    STXXL_MSG("Begin: ");
    for (i = 0; i < 10; i++)
        STXXL_MSG(STXXLVector[i]);

    STXXL_MSG("End: ");
    for (i = STXXLVector.size() - 10; i < STXXLVector.size(); i++)
        STXXL_MSG(STXXLVector[i]);
}
