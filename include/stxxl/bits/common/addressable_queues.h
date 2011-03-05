/***************************************************************************
 *  include/stxxl/bits/common/addressable_queues.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010-2011 Raoul Steffen <R-Steffen@gmx.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_ADDRESSABLE_QUEUES_HEADER
#define STXXL_ADDRESSABLE_QUEUES_HEADER

#include <set>
#include <list>
#include <map>

#include <stxxl/bits/namespace.h>

__STXXL_BEGIN_NAMESPACE

//! \brief An internal fifo queue that allows removing elements addressed with (a copy of) themself.
//! \tparam KeyType Type of contained elements.
template <typename KeyType>
class addressable_fifo_queue
{
    typedef std::list<KeyType> container_t;
    typedef typename container_t::iterator container_iter_t;
    typedef std::map<KeyType, container_iter_t> meta_t;
    typedef typename meta_t::iterator meta_iter_t;

    container_t vals;
    meta_t meta;

public:
    //! \brief Type of handle to an entry. For use with insert and remove.
    typedef meta_iter_t handle;

    //! \brief Create an empty queue.
    addressable_fifo_queue() {}
    ~addressable_fifo_queue() {}

    //! \brief Check if queue is empty.
    //! \return If queue is empty.
    bool empty() const
    { return vals.empty(); }

    //! \brief Insert new element. If the element is already in, it is moved to the back.
    //! \param e Element to insert.
    //! \return pair<handle, bool> Iterator to element; if element was newly inserted.
    std::pair<handle, bool> insert(const KeyType & e)
    {
        container_iter_t ei = vals.insert(vals.end(), e);
        std::pair<handle, bool> r = meta.insert(std::make_pair(e, ei));
        if (! r.second)
        {
            // element was already in
            vals.erase(r.first->second);
            r.first->second = ei;
        }
        return r;
    }

    //! \brief Erase element from the queue.
    //! \param e Element to remove.
    //! \return If element was in.
    bool erase(const KeyType & e)
    {
        handle mi = meta.find(e);
        if (mi == meta.end())
            return false;
        vals.erase(mi->second);
        meta.erase(mi);
        return true;
    }

    //! \brief Erase element from the queue.
    //! \param i Iterator to element to remove.
    void erase(handle i)
    {
        vals.erase(i->second);
        meta.erase(i);
    }

    //! \brief Access top element in the queue.
    //! \return Const reference to top element.
    const KeyType & top() const
    { return vals.front(); }

    //! \brief Remove top element from the queue.
    //! \return Top element.
    KeyType pop()
    {
        assert(! empty());
        const KeyType e = top();
        meta.erase(e);
        vals.pop_front();
        return e;
    }
};

//! \brief An internal priority queue that allows removing elements addressed with (a copy of) themself.
//! \tparam KeyType Type of contained elements.
//! \tparam PriorityType Type of Priority.
template < typename KeyType, typename PriorityType, class Cmp = std::less<PriorityType> >
class addressable_priority_queue
{
    struct cmp // like < for pair, but uses Cmp for < on first
    {
      bool operator()(const std::pair<PriorityType, KeyType> & left,
              const std::pair<PriorityType, KeyType> & right) const
      {
          Cmp c;
          return c(left.first, right.first) ||
                  ((! c(right.first, left.first)) && left.second < right.second);
      }
    };

    typedef std::set<std::pair<PriorityType, KeyType>, cmp> container_t;
    typedef typename container_t::iterator container_iter_t;
    typedef std::map<KeyType, container_iter_t> meta_t;
    typedef typename meta_t::iterator meta_iter_t;

    container_t vals;
    meta_t meta;

public:
    //! \brief Type of handle to an entry. For use with insert and remove.
    typedef meta_iter_t handle;

    //! \brief Create an empty queue.
    addressable_priority_queue() {}
    ~addressable_priority_queue() {}

    //! \brief Check if queue is empty.
    //! \return If queue is empty.
    bool empty() const
    { return vals.empty(); }

    //! \brief Insert new element. If the element is already in, it's priority is updated.
    //! \param e Element to insert.
    //! \param o Priority of element.
    //! \return pair<handle, bool> Iterator to element; if element was newly inserted.
    std::pair<handle, bool> insert(const KeyType & e, const PriorityType o)
    {
        std::pair<container_iter_t ,bool> s = vals.insert(std::make_pair(o, e));
        std::pair<handle, bool> r = meta.insert(std::make_pair(e, s.first));
        if (! r.second && s.second)
        {
            // was already in with different priority
            vals.erase(r.first->second);
            r.first->second = s.first;
        }
        return r;
    }

    //! \brief Erase element from the queue.
    //! \param e Element to remove.
    //! \return If element was in.
    bool erase(const KeyType & e)
    {
        handle mi = meta.find(e);
        if (mi == meta.end())
            return false;
        vals.erase(mi->second);
        meta.erase(mi);
        return true;
    }

    //! \brief Erase element from the queue.
    //! \param i Iterator to element to remove.
    void erase(handle i)
    {
        vals.erase(i->second);
        meta.erase(i);
    }

    //! \brief Access top (= min) element in the queue.
    //! \return Const reference to top element.
    const KeyType & top() const
    { return vals.begin()->second; }

    //! \brief Remove top (= min) element from the queue.
    //! \return Top element.
    KeyType pop()
    {
        assert(! empty());
        const KeyType e = top();
        meta.erase(e);
        vals.erase(vals.begin());
        return e;
    }
};

__STXXL_END_NAMESPACE

#endif /* STXXL_ADDRESSABLE_QUEUES_HEADER */
