/***************************************************************************
 *  include/stxxl/bits/containers/unordered_map.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008 Markus Westphal <marwes@users.sourceforge.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_CONTAINERS_UNORDERED_MAP_HEADER
#define STXXL_CONTAINERS_UNORDERED_MAP_HEADER

#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/containers/hash_map/hash_map.h>

STXXL_BEGIN_NAMESPACE

namespace hash_map {

template <
    class KeyType,
    class DataType,
    class HashType,
    class CompareType,
    unsigned SubBlockSize,
    unsigned BlockSize,
    class Alloc
    >
class hash_map;

} // namespace hash_map

//! \addtogroup stlcont
//! \{

/*!
 * An external memory implementation of the STL unordered_map container, which
 * is based on an external memory hash map.
 */
template <
    class KeyType,
    class MappedType,
    class HashType,
    class CompareType,
    //! subblock raw size; 8k default
    unsigned SubBlockSize = 8*1024,
    //! block size as number of subblocks; default 256*8KB = 2MB
    unsigned BlockSize = 256,
    //! allocator for internal-memory buffer
    class Alloc = std::allocator<std::pair<const KeyType, MappedType> >
    >
class unordered_map : private noncopyable
{
    typedef hash_map::hash_map<KeyType, MappedType, HashType, CompareType,
                               SubBlockSize, BlockSize, Alloc> impl_type;

    impl_type impl;

public:
    typedef typename impl_type::key_type key_type;
    typedef typename impl_type::mapped_type mapped_type;
    typedef typename impl_type::value_type value_type;

    typedef typename impl_type::reference reference;
    typedef typename impl_type::const_reference const_reference;
    typedef typename impl_type::pointer pointer;
    typedef typename impl_type::const_pointer const_pointer;

    typedef typename impl_type::external_size_type size_type;
    typedef typename impl_type::difference_type difference_type;

    typedef typename impl_type::external_size_type external_size_type;
    typedef typename impl_type::internal_size_type internal_size_type;

    typedef typename impl_type::iterator iterator;
    typedef typename impl_type::const_iterator const_iterator;

    typedef typename impl_type::hasher hasher;
    typedef typename impl_type::key_compare_type key_compare;

    typedef Alloc allocator_type;

    // construct

    //! A constructor
    //! \param n initial number of buckets
    //! \param hf hash-function
    //! \param cmp comparator-object
    //! \param buffer_size size of internal-memory buffer in bytes
    //! \param a allocation-strategory for internal-memory buffer
    unordered_map(internal_size_type n = 10000,
                  const hasher& hf = hasher(),
                  const key_compare& cmp = key_compare(),
                  internal_size_type buffer_size = 100*1024*1024,
                  const Alloc& a = Alloc())
        : impl(n, hf, cmp, buffer_size, a)
    { }

    //! Constructs a unordered-map from a given input range
    //! \param f beginning of the range
    //! \param l end of the range
    //! \param mem internal memory that may be used for bulk-construction (not to be confused with the buffer-memory)
    //! \param hf hash-function
    //! \param cmp comparator-object
    //! \param buffer_size size of internal-memory buffer in bytes
    //! \param a allocation-strategory for internal-memory buffer
    template <class InputIterator>
    unordered_map(InputIterator f,
                  InputIterator l,
                  internal_size_type mem = 256*1024*1024,
                  const hasher& hf = hasher(),
                  const key_compare& cmp = key_compare(),
                  internal_size_type buffer_size = 100*1024*1024,
                  const Alloc& a = Alloc())
        : impl(f, l, mem, hf, cmp, buffer_size, a)
    { }

    void swap(unordered_map& obj)
    {
        std::swap(impl, obj.impl);
    }

    // size and capacity
    external_size_type size() const
    {
        return impl.size();
    }
    external_size_type max_size() const
    {
        return impl.max_size();
    }
    bool empty() const
    {
        return impl.empty();
    }

    // iterators
    iterator begin()
    {
        return impl.begin();
    }
    iterator end()
    {
        return impl.end();
    }
    const_iterator begin() const
    {
        return impl.begin();
    }
    const_iterator end() const
    {
        return impl.end();
    }

    // modifiers (insert)
    std::pair<iterator, bool> insert(const value_type& obj)
    {
        return impl.insert(obj);
    }
    iterator insert_oblivious(const value_type& obj)
    {
        return impl.insert_oblivious(obj);
    }
    template <class InputIterator>
    void insert(InputIterator first, InputIterator last, internal_size_type mem)
    {
        impl.insert(first, last, mem);
    }

    // modifiers (erase)
    void erase(const_iterator position)
    {
        impl.erase(position);
    }
    internal_size_type erase(const key_type& key)
    {
        return impl.erase(key);
    }
    void erase_oblivious(const key_type& key)
    {
        impl.erase_oblivious(key);
    }
    void clear()
    {
        impl.clear();
    }

    // lookup (check)
    iterator find(const key_type& key)
    {
        return impl.find(key);
    }
    const_iterator find(const key_type& key) const
    {
        return impl.find(key);
    }
    internal_size_type count(const key_type& key) const
    {
        return impl.count(key);
    }
    std::pair<iterator, iterator> equal_range(const key_type& key)
    {
        return impl.equal_range(key);
    }
    std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const
    {
        return impl.equal_range(key);
    }
    mapped_type& operator [] (const key_type& key)
    {
        return impl[key];
    }

    // observers
    hasher hash_function() const
    {
        return impl.hash_function();
    }
    key_compare key_comp() const
    {
        return impl.key_cmp();
    }

    // bucket interface
    internal_size_type bucket_count() const
    {
        return impl.bucket_count();
    }
    internal_size_type max_bucket_count() const
    {
        return impl.max_bucket_count();
    }
    internal_size_type bucket(const key_type& k) const
    {
        return impl.bucket_index(k);
    }

    // hash policy
    float load_factor() const
    {
        return impl.load_factor();
    }
    float opt_load_factor() const
    {
        return impl.opt_load_factor();
    }
    void opt_load_factor(float z)
    {
        impl.opt_load_factor(z);
    }
    void rehash(internal_size_type buckets)
    {
        impl.rehash(buckets);
    }

    // buffer policy
    internal_size_type buffer_size() const
    {
        return impl.buffer_size();
    }
    internal_size_type max_buffer_size() const
    {
        return impl.max_buffer_size();
    }
    void max_buffer_size(internal_size_type bs)
    {
        impl.max_buffer_size(bs);
    }

    // statistics
    void reset_statistics()
    {
        impl.reset_statistics();
    }
    void print_statistics(std::ostream& o = std::cout) const
    {
        impl.print_statistics(o);
    }
    void print_load_statistics(std::ostream& o = std::cout) const
    {
        impl.print_load_statistics(o);
    }
};

//! \}

STXXL_END_NAMESPACE

namespace std {

template <
    class KeyType,
    class MappedType,
    class HashType,
    class CompareType,
    unsigned SubBlockSize,
    unsigned BlockSize,
    class Alloc
    >
void swap(stxxl::unordered_map<KeyType, MappedType, HashType, CompareType, SubBlockSize, BlockSize, Alloc>& a,
          stxxl::unordered_map<KeyType, MappedType, HashType, CompareType, SubBlockSize, BlockSize, Alloc>& b
          )
{
    a.swap(b);
}

} // namespace std

#endif // !STXXL_CONTAINERS_UNORDERED_MAP_HEADER
