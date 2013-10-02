/***************************************************************************
 *  examples/containers/pqueue1.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) ?
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/priority_queue>

struct Cmp
{
  bool operator () (const int & a, const int & b) const
  { return a>b; }
  
  int min_value() const
  { return (std::numeric_limits<int>::max)(); }
};

// use 64 GiB on main memory and 1 billion items at most
typedef stxxl::PRIORITY_QUEUE_GENERATOR<int, Cmp, 64*1024*1024, 1024*1024>::result pq_type;  
typedef pq_type::block_type block_type;

int main() {

  // use 10 block read and write pools for enable overlapping of I/O and computation
  stxxl::prefetch_pool<block_type> p_pool(10);
  stxxl::write_pool<block_type>    w_pool(10);
  
  pq_type Q(p_pool,w_pool);
  Q.push(1);
  Q.push(4);
  Q.push(2);
  Q.push(8);
  Q.push(5);
  Q.push(7);

  assert(Q.size() == 6);
  
  assert(Q.top() == 8);
  Q.pop();

  assert(Q.top() == 7);
  Q.pop();

  assert(Q.top() == 5);
  Q.pop();

  assert(Q.top() == 4);
  Q.pop();

  assert(Q.top() == 2);
  Q.pop();

  assert(Q.top() == 1);
  Q.pop();

  assert(Q.empty());
  
  return 0;
}
