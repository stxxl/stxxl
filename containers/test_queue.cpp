/***************************************************************************
 *            testqueue.cpp
 *
 *  Thu Mar 24 11:21:29 2005
 *  Copyright  2005 Roman Dementiev
 *  Email dementiev@ira.uka.de
 ****************************************************************************/

#include "queue.h"
#include <queue>

typedef unsigned my_type;

template <class q1type, class q2type>
void check(const q1type & q1, const q2type & q2)
{
    assert(q1.empty() == q2.empty());
    assert(q1.size() == q2.size() );
    if (!q1.empty())
    {
        assert(q1.front() == q2.front() );
        assert(q1.back() == q2.back() );
    }
}

int main()
{
    unsigned cnt;
    STXXL_MSG("Elements in a block: " << stxxl::queue<my_type>::block_type::size)

    stxxl::queue<my_type>  xqueue(2, 2, 2 );
    std::queue<my_type>  squeue;
    check(xqueue, squeue);

    STXXL_MSG("Testing special case 4")
    cnt = stxxl::queue < my_type > ::block_type::size;
    stxxl::random_number32 rnd;
    while (cnt--)
    {
        my_type val = rnd();
        xqueue.push(val);
        squeue.push(val);
        check(xqueue, squeue);
    }
    cnt = stxxl::queue < my_type > ::block_type::size;
    while (cnt--)
    {
        xqueue.pop();
        squeue.pop();
        check(xqueue, squeue);
    }
    STXXL_MSG("Testing other cases ")
    cnt = 100 * stxxl::queue<my_type>::block_type::size;
    while (cnt--)
    {
        if ((cnt % 1000000) == 0)
            STXXL_MSG("Operations left: " << cnt << " queue size " << squeue.size());

        int rndtmp = rnd() % 3;
        if (rndtmp > 0 || squeue.empty())
        {
            my_type val = rnd();
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
        if ((cnt++ % 1000000) == 0)
            STXXL_MSG("Operations: " << cnt << " queue size " << squeue.size());

        int rndtmp = rnd() % 4;
        if (rndtmp >= 3 )
        {
            my_type val = rnd();
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
        if ((cnt % 1000000) == 0)
            STXXL_MSG("Operations left: " << cnt << " queue size " << squeue.size());

        int rndtmp = rnd() % 3;
        if (rndtmp > 0 || squeue.empty())
        {
            my_type val = rnd();
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
    typedef stxxl::queue<my_type>::block_type block_type;
    stxxl::write_pool<block_type> w_pool(5);
    stxxl::prefetch_pool<block_type> p_pool(5);
    stxxl::queue<my_type>  xqueue1(w_pool, p_pool, 5);
    std::queue<my_type>  squeue1;

    cnt = 10 * stxxl::queue<my_type>::block_type::size;
    while (cnt--)
    {
        if ((cnt % 1000000) == 0)
            STXXL_MSG("Operations left: " << cnt << " queue size " << squeue1.size());

        int rndtmp = rnd() % 3;
        if (rndtmp > 0 || squeue1.empty())
        {
            my_type val = rnd();
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
}
