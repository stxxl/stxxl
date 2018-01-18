/***************************************************************************
 *  include/stxxl/bits/containers/pq_helpers.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 1999 Peter Sanders <sanders@mpi-sb.mpg.de>
 *  Copyright (C) 2003, 2004, 2007 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007, 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2007, 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_CONTAINERS_PQ_HELPERS_HEADER
#define STXXL_CONTAINERS_PQ_HELPERS_HEADER

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

#include <foxxll/common/error_handling.hpp>
#include <foxxll/common/tmeta.hpp>
#include <foxxll/mng/block_alloc_strategy.hpp>
#include <foxxll/mng/block_manager.hpp>
#include <foxxll/mng/prefetch_pool.hpp>
#include <foxxll/mng/read_write_pool.hpp>
#include <foxxll/mng/typed_block.hpp>
#include <foxxll/mng/write_pool.hpp>

#include <stxxl/bits/algo/sort_base.h>
#include <stxxl/bits/common/is_sorted.h>
#include <stxxl/bits/deprecated.h>
#include <stxxl/bits/parallel.h>

#if STXXL_PARALLEL

#if defined(STXXL_PARALLEL_MODE) && ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) < 40400)
#undef STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL
#undef STXXL_PARALLEL_PQ_MULTIWAY_MERGE_EXTERNAL
#undef STXXL_PARALLEL_PQ_MULTIWAY_MERGE_DELETE_BUFFER
#define STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL 0
#define STXXL_PARALLEL_PQ_MULTIWAY_MERGE_EXTERNAL 0
#define STXXL_PARALLEL_PQ_MULTIWAY_MERGE_DELETE_BUFFER 0
#endif

// enable/disable parallel merging for certain cases, for performance tuning
#ifndef STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL
#define STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL 1
#endif
#ifndef STXXL_PARALLEL_PQ_MULTIWAY_MERGE_EXTERNAL
#define STXXL_PARALLEL_PQ_MULTIWAY_MERGE_EXTERNAL 1
#endif
#ifndef STXXL_PARALLEL_PQ_MULTIWAY_MERGE_DELETE_BUFFER
#define STXXL_PARALLEL_PQ_MULTIWAY_MERGE_DELETE_BUFFER 1
#endif

#endif //STXXL_PARALLEL

#if STXXL_PARALLEL && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_EXTERNAL
#define STXXL_PQ_EXTERNAL_LOSER_TREE 0 // no loser tree for the external sequences
#else
#define STXXL_PQ_EXTERNAL_LOSER_TREE 1
#endif

#if STXXL_PARALLEL && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL
#define STXXL_PQ_INTERNAL_LOSER_TREE 0 // no loser tree for the internal sequences
#else
#define STXXL_PQ_INTERNAL_LOSER_TREE 1
#endif

namespace stxxl {

//! \defgroup stlcontinternals internals
//! \ingroup stlcont
//! Supporting internal classes
//! \{

/*! \internal
 */
namespace priority_queue_local {

/*!
 * Similar to std::priority_queue, with the following differences:
 * - Maximum size is fixed at construction time, so an array can be used.
 * - Provides access to underlying heap, so (parallel) sorting in place is possible.
 * - Can be cleared "at once", without reallocation.
 */
template <typename ValueType, typename ContainerType = std::vector<ValueType>,
          typename CompareType = std::less<ValueType> >
class internal_priority_queue
{
public:
    using value_type = ValueType;
    using container_type = ContainerType;
    using compare_type = CompareType;
    using reference = typename container_type::reference;
    using const_reference = typename container_type::const_reference;
    using size_type = typename container_type::size_type;

protected:
    //  See queue::heap for notes on these names.
    container_type heap;
    CompareType comp;
    size_type current_size;

public:
    //! Default constructor creates no elements.
    explicit internal_priority_queue(size_type capacity, const CompareType& compare)
        : heap(capacity),
          comp(compare),
          current_size(0)
    { }

    //! Returns true if the %queue is empty.
    bool
    empty() const
    { return current_size == 0; }

    //! Returns the number of elements in the %queue.
    size_type
    size() const
    { return current_size; }

    /*!
     * Returns a read-only (constant) reference to the data at the first
     * element of the %queue.
     */
    const_reference
    top() const
    {
        return heap.front();
    }

    /*!
     * Add data to the %queue.
     * \param  x  Data to be added.
     *
     * This is a typical %queue operation.
     * The time complexity of the operation depends on the underlying
     * container.
     */
    void
    push(const value_type& x)
    {
        heap[current_size] = x;
        ++current_size;
        std::push_heap(heap.begin(), heap.begin() + current_size, comp);
    }

    /*!
     * Removes first element.
     *
     * This is a typical %queue operation.  It shrinks the %queue
     * by one.  The time complexity of the operation depends on the
     * underlying container.
     *
     * Note that no data is returned, and if the first element's
     * data is needed, it should be retrieved before pop() is
     * called.
     */
    void
    pop()
    {
        std::pop_heap(heap.begin(), heap.begin() + current_size, comp);
        --current_size;
    }

    //! Sort all contained elements, write result to \c target.
    void sort_to(value_type* target)
    {
        check_sort_settings();
        potentially_parallel::
        sort(heap.begin(), heap.begin() + current_size, comp);
        std::reverse_copy(heap.begin(), heap.begin() + current_size, target);
    }

    //! Remove all contained elements.
    void clear()
    {
        current_size = 0;
    }
};

//! Inverts the order of a comparison functor by swapping its arguments.
template <class Predicate, typename FirstType, typename SecondType>
class invert_order
{
protected:
    Predicate pred;

public:
    explicit invert_order(const Predicate& _pred) : pred(_pred) { }

    bool operator () (const FirstType& x, const SecondType& y) const
    {
        return pred(y, x);
    }
};

/*!
 * Similar to std::stack, with the following differences:
 * - Maximum size is fixed at compilation time, so an array can be used.
 * - Can be cleared "at once", without reallocation.
 */
template <typename ValueType, size_t MaxSize>
class internal_bounded_stack
{
    using value_type = ValueType;
    using size_type = size_t;
    enum { max_size = MaxSize };

    size_type m_size;
    value_type m_array[max_size];

public:
    internal_bounded_stack() : m_size(0) { }

    void push(const value_type& x)
    {
        assert(m_size < max_size);
        m_array[m_size++] = x;
    }

    const value_type & top() const
    {
        assert(m_size > 0);
        return m_array[m_size - 1];
    }

    void pop()
    {
        assert(m_size > 0);
        --m_size;
    }

    void clear()
    {
        m_size = 0;
    }

    size_type size() const
    {
        return m_size;
    }

    bool empty() const
    {
        return m_size == 0;
    }
};

template <typename Iterator>
class short_sequence : public std::pair<Iterator, Iterator>
{
    using pair = std::pair<Iterator, Iterator>;

public:
    using iterator = Iterator;
    using const_iterator = const iterator;
    using value_type = typename std::iterator_traits<iterator>::value_type;
    using size_type = typename std::iterator_traits<iterator>::difference_type;
    using reference = value_type &;
    using const_reference = const value_type &;
    using origin_type = size_t;

private:
    origin_type m_origin;

public:
    short_sequence(Iterator first, Iterator last, origin_type origin)
        : pair(first, last), m_origin(origin)
    { }

    iterator begin()
    {
        return this->first;
    }

    const_iterator begin() const
    {
        return this->first;
    }

    const_iterator cbegin() const
    {
        return begin();
    }

    iterator end()
    {
        return this->second;
    }

    const_iterator end() const
    {
        return this->second;
    }

    const_iterator cend() const
    {
        return end();
    }

    reference front()
    {
        return *begin();
    }

    const_reference front() const
    {
        return *begin();
    }

    reference back()
    {
        return *(end() - 1);
    }

    const_reference back() const
    {
        return *(end() - 1);
    }

    size_type size() const
    {
        return end() - begin();
    }

    bool empty() const
    {
        return size() == 0;
    }

    origin_type origin() const
    {
        return m_origin;
    }
};

} // namespace priority_queue_local

//! \}

} // namespace stxxl

#endif // !STXXL_CONTAINERS_PQ_HELPERS_HEADER
// vim: et:ts=4:sw=4
