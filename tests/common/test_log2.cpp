/***************************************************************************
 *  utils/log2.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright © 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright © 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iostream>
#include <math.h>
#include <assert.h>
#include <stxxl/bits/config.h>
#include <stxxl/bits/common/tmeta.h>
#include <stxxl/bits/common/utils.h>
#include <stxxl/bits/verbose.h>

using stxxl::LOG2;
using stxxl::unsigned_type;

template <unsigned_type i>
void log_i()
{
    std::cout << i << "\t" << (i < 1000000 ? "\t" : "")
              << stxxl::LOG2_floor<i>::value << "\t"
              << stxxl::LOG2<i>::floor << "\t"
              << stxxl::LOG2<i>::ceil << std::endl;

#ifndef STXXL_MSVC
    std::cout << "\t\t"
              << log2l((long double)i) << "\t"
              << (unsigned_type)floorl(log2l(i)) << "\t"
              << (unsigned_type)ceill(log2l(i)) << "\n";
#endif

    if (i == 0)
    {
        STXXL_CHECK( stxxl::LOG2_floor<i>::value == 0 );
        STXXL_CHECK( stxxl::LOG2<i>::floor == 0 );
        STXXL_CHECK( stxxl::LOG2<i>::ceil == 0 );
    }
    else if (i <= ((stxxl::uint64)(1) << 59)) // does not work for higher powers
    {
        std::cout << "\t\t"
                  << stxxl::ilog2_floor(i) << "\t"
                  << stxxl::ilog2_floor(i) << "\t"
                  << stxxl::ilog2_ceil(i) << "\n";

        STXXL_CHECK( stxxl::LOG2_floor<i>::value == stxxl::ilog2_floor(i) );
        STXXL_CHECK( stxxl::LOG2<i>::floor == stxxl::ilog2_floor(i) );
        STXXL_CHECK( stxxl::LOG2<i>::ceil == stxxl::ilog2_ceil(i) );

#ifndef STXXL_MSVC
        STXXL_CHECK( stxxl::LOG2_floor<i>::value == (unsigned_type)floorl(log2l(i)) );
        STXXL_CHECK( stxxl::LOG2<i>::floor == (unsigned_type)floorl(log2l(i)) );
        STXXL_CHECK( stxxl::LOG2<i>::ceil == (unsigned_type)ceill(log2l(i)) );
#endif
    }

    std::cout << "\n";
}

template <unsigned_type i>
void log_ipm1()
{
    log_i<i - 1>();
    log_i<i>();
    log_i<i + 1>();
    std::cout << std::endl;
}

int main()
{
    std::cout << "i\t\tLOG2<i>\tLOG2<i>\tLOG2<i>" << std::endl;
    std::cout << "\t\t\tfloor\tceil" << std::endl;
    log_ipm1<1 << 0>();
    log_ipm1<1 << 1>();
    log_ipm1<1 << 2>();
    log_ipm1<1 << 3>();
    log_ipm1<1 << 4>();
    log_ipm1<1 << 5>();
    log_ipm1<1 << 6>();
    log_ipm1<1 << 7>();
    log_ipm1<1 << 8>();
    log_ipm1<1 << 9>();
    log_ipm1<1 << 10>();
    log_ipm1<1 << 11>();
    log_ipm1<1 << 12>();
    log_ipm1<1 << 16>();
    log_ipm1<1 << 24>();
    log_ipm1<1 << 30>();
    log_ipm1<1UL << 31>();
#if __WORDSIZE == 64
    log_ipm1<1UL << 32>();
    log_ipm1<1UL << 33>();
    log_ipm1<1UL << 48>();
    log_ipm1<1UL << 50>();
    log_ipm1<1UL << 55>();
    log_ipm1<1UL << 63>();
#endif
}
