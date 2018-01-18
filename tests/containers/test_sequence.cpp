/***************************************************************************
 *  tests/containers/test_sequence.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2012-2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define STXXL_DEFAULT_BLOCK_SIZE(T) 4096

#include <iostream>
#include <iterator>

#include <tlx/die.hpp>

#include <stxxl/random>
#include <stxxl/sequence>

int main(int argc, char* argv[])
{
    uint64_t ops = (argc >= 2)
                   ? foxxll::atouint64(argv[1]) : 16 * STXXL_DEFAULT_BLOCK_SIZE(int);

    stxxl::random_number32 random;
    stxxl::sequence<int> XXLDeque;
    std::deque<int> STDDeque;

    for (uint64_t i = 0; i < ops; ++i)
    {
        unsigned curOP = random() % 6;
        unsigned value = random();
        switch (curOP)
        {
        case 0: // make insertion a bit more likely
        case 1:
            XXLDeque.push_front(value);
            STDDeque.push_front(value);
            break;
        case 2: // make insertion a bit more likely
        case 3:
            XXLDeque.push_back(value);
            STDDeque.push_back(value);
            break;
        case 4:
            if (!XXLDeque.empty())
            {
                XXLDeque.pop_front();
                STDDeque.pop_front();
            }
            break;
        case 5:
            if (!XXLDeque.empty())
            {
                XXLDeque.pop_back();
                STDDeque.pop_back();
            }
            break;
        }

        die_unless(XXLDeque.empty() == STDDeque.empty());
        die_unless(XXLDeque.size() == STDDeque.size());

        if (XXLDeque.size() > 0)
        {
            die_unless(XXLDeque.back() == STDDeque.back());
            die_unless(XXLDeque.front() == STDDeque.front());
        }

        if (!(i % 1000))
        {
            std::cout << "Complete check of sequence/deque (size " << XXLDeque.size() << ")\n";
            stxxl::sequence<int>::stream stream = XXLDeque.get_stream();
            std::deque<int>::const_iterator b = STDDeque.begin();

            while (!stream.empty())
            {
                die_unless(b != STDDeque.end());
                die_unless(*stream == *b);
                ++stream;
                ++b;
            }

            die_unless(b == STDDeque.end());
        }

        if (!(i % 1000))
        {
            std::cout << "Complete check of reverse sequence/deque (size " << XXLDeque.size() << ")\n";
            stxxl::sequence<int>::reverse_stream stream = XXLDeque.get_reverse_stream();
            std::deque<int>::reverse_iterator b = STDDeque.rbegin();

            while (!stream.empty())
            {
                die_unless(b != STDDeque.rend());
                die_unless(*stream == *b);
                ++stream;
                ++b;
            }

            die_unless(b == STDDeque.rend());
        }
    }

    return 0;
}
