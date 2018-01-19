/***************************************************************************
 *  tests/containers/test_queue.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2005 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define STXXL_DEFAULT_BLOCK_SIZE(T) 4096

// stxxl::queue contains deprecated funtions
#define STXXL_NO_DEPRECATED 1

#include <queue>
#include <random>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <stxxl/queue>
#include <stxxl/random>

using my_type = unsigned;

template <class q1type, class q2type>
void check(const q1type& q1, const q2type& q2)
{
    die_unless(q1.empty() == q2.empty());
    die_unless(q1.size() == q2.size());
    if (!q1.empty())
    {
        if (q1.front() != q2.front() || q1.back() != q2.back())
            LOG1 << q1.size() << ": (" << q1.front() << ", " << q1.back() << ") (" << q2.front() << ", " << q2.back() << ")" << (q1.front() == q2.front() ? "" : " FRONT");
        die_unless(q1.front() == q2.front());
        die_unless(q1.back() == q2.back());
    }
}

// forced instantiation
template class stxxl::queue<my_type>;

int main()
{
    std::mt19937 randgen;
    std::uniform_int_distribution<my_type> distr;
    std::uniform_int_distribution<my_type> distr02(0, 2);
    std::uniform_int_distribution<my_type> distr03(0, 3);

    unsigned cnt;
    LOG1 << "Elements in a block: " << stxxl::queue<my_type>::block_type::size;

    // FIXME: can this be raised to recommended (3, 2) without breaking "Testing special case 4" or any other tests?
    stxxl::queue<my_type> xqueue(2, 2, -1);
    std::queue<my_type> squeue;
    check(xqueue, squeue);

    LOG1 << "Testing special case 4";
    cnt = stxxl::queue<my_type>::block_type::size;
    while (cnt--)
    {
        my_type val = distr(randgen);
        xqueue.push(val);
        squeue.push(val);
        check(xqueue, squeue);
    }
    cnt = stxxl::queue<my_type>::block_type::size;
    while (cnt--)
    {
        xqueue.pop();
        squeue.pop();
        check(xqueue, squeue);
    }
    LOG1 << "Testing other cases ";
    cnt = 20 * stxxl::queue<my_type>::block_type::size;
    while (cnt--)
    {
        if ((cnt % 1000) == 0)
            LOG1 << "Operations left: " << cnt << " queue size " << squeue.size();

        int rndtmp = distr02(randgen);
        if (rndtmp > 0 || squeue.empty())
        {
            my_type val = distr(randgen);
            xqueue.push(val);
            squeue.push(val);
            check(xqueue, squeue);
        }
        else
        {
            xqueue.pop();
            squeue.pop();
            check(xqueue, squeue);
        }
    }
    while (!squeue.empty())
    {
        if ((cnt++ % 1000) == 0)
            LOG1 << "Operations: " << cnt << " queue size " << squeue.size();

        int rndtmp = distr03(randgen);
        if (rndtmp >= 3)
        {
            my_type val = distr(randgen);
            xqueue.push(val);
            squeue.push(val);
            check(xqueue, squeue);
        }
        else
        {
            xqueue.pop();
            squeue.pop();
            check(xqueue, squeue);
        }
    }
    cnt = 10 * stxxl::queue<my_type>::block_type::size;
    while (cnt--)
    {
        if ((cnt % 1000) == 0)
            LOG1 << "Operations left: " << cnt << " queue size " << squeue.size();

        int rndtmp = distr02(randgen);
        if (rndtmp > 0 || squeue.empty())
        {
            my_type val = distr(randgen);
            xqueue.push(val);
            squeue.push(val);
            check(xqueue, squeue);
        }
        else
        {
            xqueue.pop();
            squeue.pop();
            check(xqueue, squeue);
        }
    }
    using block_type = stxxl::queue<my_type>::block_type;
    foxxll::read_write_pool<block_type> pool(5, 5);
    stxxl::queue<my_type> xqueue1(pool, -1);
    std::queue<my_type> squeue1;

    cnt = 10 * stxxl::queue<my_type>::block_type::size;
    while (cnt--)
    {
        if ((cnt % 1000) == 0)
            LOG1 << "Operations left: " << cnt << " queue size " << squeue1.size();

        int rndtmp = distr02(randgen);
        if (rndtmp > 0 || squeue1.empty())
        {
            my_type val = distr(randgen);
            xqueue1.push(val);
            squeue1.push(val);
            check(xqueue1, squeue1);
        }
        else
        {
            xqueue1.pop();
            squeue1.pop();
            check(xqueue1, squeue1);
        }
    }

    {
        // test proper destruction of a single-block queue
        stxxl::queue<int> q;
        q.push(42);
    }

    return 0;
}
