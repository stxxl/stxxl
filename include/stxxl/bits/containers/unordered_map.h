#ifndef _STXXL_UNORDERED_MAP_H_
#define _STXXL_UNORDERED_MAP_H_

/***************************************************************************
 *            unordered_map.h
 *
 *  Copyright  2008  Markus Westphal
 ****************************************************************************/

#include "stxxl/bits/noncopyable.h"
#include "stxxl/bits/containers/hash_map/hash_map.h"


__STXXL_BEGIN_NAMESPACE

namespace hash_map
{
    template <
        class KeyType,
        class DataType,
        class HashType,
        class CompareType,
        unsigned SubBlkSize,
        unsigned BlkSize,
        class Alloc
        >
    class hash_map;
}


template <
    class KeyType,
    class DataType,
    class HashType,
    class CompareType,
    unsigned SubBlkSize = 8 *1024,                                              // subblock raw size; 8k default
    unsigned BlkSize = 256,                                                     // block size as number of subblocks; default 256*8KB = 2MB
    class Alloc = std::allocator<std::pair<const KeyType, DataType> >           // allocator for internal-memory buffer
    >
class unordered_map : private noncopyable
{
    typedef hash_map::hash_map<KeyType, DataType, HashType, CompareType, SubBlkSize, BlkSize, Alloc> impl_type;

    impl_type Impl;

public:
    typedef typename impl_type::key_type key_type;
    typedef typename impl_type::mapped_type mapped_type;
    typedef typename impl_type::value_type value_type;

    typedef typename impl_type::reference reference;
    typedef typename impl_type::const_reference const_reference;
    typedef typename impl_type::pointer pointer;
    typedef typename impl_type::const_pointer const_pointer;

    typedef typename impl_type::size_type size_type;
    typedef typename impl_type::difference_type difference_type;

    typedef typename impl_type::iterator iterator;
    typedef typename impl_type::const_iterator const_iterator;

    typedef typename impl_type::hasher hasher;
    typedef typename impl_type::keycmp key_compare;

    typedef Alloc allocator_type;


    // construct

    //! \brief A constructor
    //! \param n initial number of buckets
    //! \param hf hash-function
    //! \param cmp comparator-object
    //! \param buffer_size size of internal-memory buffer in bytes
    //! \param a allocation-strategory for internal-memory buffer
    unordered_map(size_type n = 10000,
                  const hasher & hf = hasher(),
                  const key_compare & cmp = key_compare(),
                  size_type buffer_size = 100 *1024 *1024,
                  const Alloc & a = Alloc()
                  ) : Impl(n, hf, cmp, buffer_size, a)
    { }


    //! \brief Constructs a unordered-map from a given input range
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
                  size_type mem = 256 *1024 *1024,
                  const hasher & hf = hasher(),
                  const key_compare & cmp = key_compare(),
                  size_type buffer_size = 100 *1024 *1024,
                  const Alloc & a = Alloc()
                  ) : Impl(f, l, mem, hf, cmp, buffer_size, a)
    { }

    void swap(unordered_map & obj)
    {
        std::swap(Impl, obj.Impl);
    }


    // size and capacity
    size_type size() const
    {
        return Impl.size();
    }
    size_type max_size() const
    {
        return Impl.max_size();
    }
    bool empty() const
    {
        return Impl.empty();
    }

    // iterators
    iterator begin()
    {
        return Impl.begin();
    }
    iterator end()
    {
        return Impl.end();
    }
    const_iterator begin() const
    {
        return Impl.begin();
    }
    const_iterator end() const
    {
        return Impl.end();
    }

    // modifiers (insert)
    std::pair<iterator, bool> insert(const value_type & obj)
    {
        return Impl.insert(obj);
    }
    iterator insert_oblivious(const value_type & obj)
    {
        return Impl.insert_oblivious(obj);
    }
    template <class InputIterator>
    void insert(InputIterator first, InputIterator last, size_type mem)
    {
        Impl.insert(first, last, mem);
    }

    // modifiers (erase)
    void erase(const_iterator position)
    {
        Impl.erase(position);
    }
    size_type erase(const key_type & key)
    {
        Impl.erase(key);
    }
    void erase_oblivious(const key_type & key)
    {
        Impl.erase_oblivious(key);
    }
    void clear()
    {
        Impl.clear();
    }

    // lookup (check)
    iterator find(const key_type & key)
    {
        return Impl.find(key);
    }
    const_iterator find(const key_type & key) const
    {
        return Impl.find(key);
    }
    size_type count(const key_type & key) const
    {
        return Impl.count(key);
    }
    std::pair<iterator, iterator> equal_range(const key_type & key)
    {
        return Impl.equal_range(key);
    }
    std::pair<const_iterator, const_iterator> equal_range(const key_type & key) const
    {
        return Impl.equal_range(key);
    }
    mapped_type & operator [] (const key_type & key)
    {
        return Impl[key];
    }

    // observers
    hasher hash_function() const
    {
        return Impl.hash_function();
    }
    key_compare key_comp() const
    {
        return Impl.keycmp();
    }

    // bucket interface
    size_type bucket_count() const
    {
        return Impl.bucket_count();
    }
    size_type max_bucket_count() const
    {
        return Impl.max_bucket_count();
    }
    size_type bucket(const key_type & k) const
    {
        return Impl.bucket(k);
    }

    // hash policy
    float load_factor() const
    {
        return Impl.load_factor();
    }
    float opt_load_factor() const
    {
        return Impl.opt_load_factor();
    }
    void opt_load_factor(float z)
    {
        Impl.opt_load_factor(z);
    }
    void rehash(size_type n)
    {
        Impl.rehash(n);
    }

    // buffer policy
    size_type buffer_size() const
    {
        return Impl.buffer_size();
    }
    size_type max_buffer_size() const
    {
        return Impl.max_buffer_size();
    }
    void max_buffer_size(size_type bs)
    {
        Impl.max_buffer_size(bs);
    }

    // statistics
    void reset_statistics()
    {
        Impl.reset_statistics();
    }
    void print_statistics(std::ostream & o = std::cout) const
    {
        Impl.print_statistics(o);
    }
    void print_load_statistics(std::ostream & o = std::cout) const
    {
        Impl.print_load_statistics(o);
    }
};


__STXXL_END_NAMESPACE

namespace std
{
    template <
        class KeyType,
        class DataType,
        class HashType,
        class CompareType,
        unsigned SubBlkSize,
        unsigned BlkSize,
        class Alloc
        >
    void swap(stxxl::unordered_map<KeyType, DataType, HashType, CompareType, SubBlkSize, BlkSize, Alloc> & a,
              stxxl::unordered_map<KeyType, DataType, HashType, CompareType, SubBlkSize, BlkSize, Alloc> & b
              )
    {
        a.swap(b);
    }
}

#endif
