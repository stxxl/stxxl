/***************************************************************************
 *  include/stxxl/bits/containers/pq_helpers.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
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

#ifndef STXXL_PQ_HELPERS_HEADER
#define STXXL_PQ_HELPERS_HEADER

#include <queue>

__STXXL_BEGIN_NAMESPACE

//! \addtogroup stlcontinternals
//!
//! \{

/*! \internal
 */
namespace priority_queue_local
{
    /**
     * @brief Similar to std::priority_queue, with the following differences:
     * - Maximum size is fixed at construction time, so an array can be used.
     * - Provides access to underlying heap, so (parallel) sorting in place is possible.
     * - Can be cleared "at once", without reallocation.
     */
    template <typename _Tp, typename _Sequence = std::vector<_Tp>,
              typename _Compare = std::less<typename _Sequence::value_type> >
    class internal_priority_queue
    {
        // concept requirements
        typedef typename _Sequence::value_type _Sequence_value_type;

    public:
        typedef typename _Sequence::value_type value_type;
        typedef typename _Sequence::reference reference;
        typedef typename _Sequence::const_reference const_reference;
        typedef typename _Sequence::size_type size_type;
        typedef          _Sequence container_type;

    protected:
        //  See queue::heap for notes on these names.
        _Sequence heap;
        _Compare comp;
        size_type current_size;

    public:
        /**
         *  @brief  Default constructor creates no elements.
         */
        explicit
        internal_priority_queue(size_type capacity)
            : heap(capacity), current_size(0)
        { }

        /**
         *  Returns true if the %queue is empty.
         */
        bool
        empty() const
        { return current_size == 0; }

        /**  Returns the number of elements in the %queue.  */
        size_type
        size() const
        { return current_size; }

        /**
         *  Returns a read-only (constant) reference to the data at the first
         *  element of the %queue.
         */
        const_reference
        top() const
        {
            return heap.front();
        }

        /**
         *  @brief  Add data to the %queue.
         *  @param  __x  Data to be added.
         *
         *  This is a typical %queue operation.
         *  The time complexity of the operation depends on the underlying
         *  sequence.
         */
        void
        push(const value_type & __x)
        {
            heap[current_size] = __x;
            ++current_size;
            std::push_heap(heap.begin(), heap.begin() + current_size, comp);
        }

        /**
         *  @brief  Removes first element.
         *
         *  This is a typical %queue operation.  It shrinks the %queue
         *  by one.  The time complexity of the operation depends on the
         *  underlying sequence.
         *
         *  Note that no data is returned, and if the first element's
         *  data is needed, it should be retrieved before pop() is
         *  called.
         */
        void
        pop()
        {
            std::pop_heap(heap.begin(), heap.begin() + current_size, comp);
            --current_size;
        }

        /**
         * @brief Sort all contained elements, write result to @c target.
         */
        void sort_to(value_type * target)
        {
            check_sort_settings();
            potentially_parallel::
            sort(heap.begin(), heap.begin() + current_size, comp);
            std::reverse_copy(heap.begin(), heap.begin() + current_size, target);
        }

        /**
         * @brief Remove all contained elements.
         */
        void clear()
        {
            current_size = 0;
        }
    };

    /**
 * @brief Inverts the order of a comparison functor by swapping its arguments.
 */
    template <class Predicate, typename first_argument_type, typename second_argument_type>
    class invert_order
    {
    protected:
        Predicate pred;

    public:
        explicit
        invert_order(const Predicate & _pred) : pred(_pred) { }

        bool operator () (const first_argument_type & x, const second_argument_type & y) const
        {
            return pred(y, x);
        }
    };


/**
 * \brief Similar to std::stack, with the following differences:
     * - Maximum size is fixed at compilation time, so an array can be used.
     * - Can be cleared "at once", without reallocation.
     */
    template <typename Tp_, unsigned_type max_size_>
    class internal_bounded_stack
    {
        typedef Tp_ value_type;
        typedef unsigned_type size_type;
        enum { max_size = max_size_ };

        size_type size_;
        value_type array[max_size];

    public:
        internal_bounded_stack() : size_(0) { }

        void push(const value_type & x)
        {
            assert(size_ < max_size);
            array[size_++] = x;
        }

        const value_type & top() const
        {
            assert(size_ > 0);
            return array[size_ - 1];
        }

        void pop()
        {
            assert(size_ > 0);
            --size_;
        }

        void clear()
        {
            size_ = 0;
        }

        size_type size() const
        {
            return size_;
        }

        bool empty() const
        {
            return size_ == 0;
        }
    };
} //priority_queue_local

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_PQ_HELPERS_HEADER
// vim: et:ts=4:sw=4
