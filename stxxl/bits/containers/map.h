#ifndef STXXL_MAP_INCLUDE
#define STXXL_MAP_INCLUDE

/***************************************************************************
 *            map.h
 *
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#include "stxxl/bits/containers/btree/btree.h"


__STXXL_BEGIN_NAMESPACE

namespace btree
{
    template <      class KeyType,
              class DataType,
              class CompareType,
              unsigned LogNodeSize,
              unsigned LogLeafSize,
              class PDAllocStrategy
    >
    class btree;
}

//! \addtogroup stlcont
//! \{

//! \brief Priority queue type generator

//! Template parameters:
//! - KeyType key type
//! - DataType data type
//! - CompareType comparison type used to determine
//! whether a key is smaller than another one.
//! If CompareType()(x,y) is true, then x is smaller than y.
//! CompareType must also provide a static \c max_value method, that returns
//! a value of type KeyType that is
//! larger than any key stored in map : i.e. for all \b x in map holds
//! CompareType()(x,CompareType::max_value()) <BR>
//! <BR>
//! Example: :
//! \verbatim
//! struct CmpIntGreater
//! {
//!   bool operator () (const int & a, const int & b) const { return a>b; }
//!   static int max_value() { return std::numeric_limits<int>::max(); }
//! };
//! \endverbatim
//! Anpother example:
//! \verbatim
//! struct CmpIntLess
//! {
//!   bool operator () (const int & a, const int & b) const { return a<b; }
//!   static int max_value() const  { return std::numeric_limits<int>::min(); }
//! };
//! \endverbatim
//! Note that CompareType must define a strict weak ordering.
//! (<A HREF="http://www.sgi.com/tech/stl/StrictWeakOrdering.html">see what it is</A>)
//! - \c RawNodeSize size of internal nodes of map in bytes (btree implementation).
//! - \c RawLeafSize size of leaves of map in bytes (btree implementation).
//! - \c PDAllocStrategy parallel disk allocation strategy (\c stxxl::SR is recommended and default)
//!
template <      class KeyType,
          class DataType,
          class CompareType,
          unsigned RawNodeSize = 16 * 1024,                             // 16 KBytes default
          unsigned RawLeafSize = 128 * 1024,                             // 128 KBytes default
          class PDAllocStrategy = stxxl::SR
>
class map
{
    typedef btree::btree<KeyType, DataType, CompareType, RawNodeSize, RawLeafSize, PDAllocStrategy> impl_type;

    impl_type Impl;

    map();     // forbidden
    map(const map &);    // forbidden
    map & operator=(const map &);   // forbidden

public:
    typedef typename impl_type::node_block_type node_block_type;
    typedef typename impl_type::leaf_block_type leaf_block_type;

    typedef typename impl_type::key_type key_type;
    typedef typename impl_type::data_type data_type;
    typedef typename impl_type::data_type mapped_type;
    typedef typename impl_type::value_type value_type;
    typedef typename impl_type::key_compare key_compare;
    typedef typename impl_type::value_compare value_compare;
    typedef typename impl_type::pointer pointer;
    typedef typename impl_type::reference reference;
    typedef typename impl_type::const_reference const_reference;
    typedef typename impl_type::size_type size_type;
    typedef typename impl_type::difference_type difference_type;
    typedef typename impl_type::iterator iterator;
    typedef typename impl_type::const_iterator const_iterator;

    iterator begin() { return Impl.begin(); }
    iterator end() { return Impl.end(); }
    const_iterator begin() const { return Impl.begin(); }
    const_iterator end() const { return Impl.end(); }
    size_type size() const { return Impl.size(); }
    size_type max_size() const { return Impl.max_size(); }
    bool empty() const { return Impl.empty(); }
    key_compare key_comp() const { return Impl.key_comp(); }
    value_compare value_comp() const { return Impl.value_comp(); }

    //! \brief A constructor
    //! \param node_cache_size_in_bytes size of node cache in bytes (btree implementation)
    //! \param leaf_cache_size_in_bytes size of leaf cache in bytes (btree implementation)
    map(    unsigned_type node_cache_size_in_bytes,
            unsigned_type leaf_cache_size_in_bytes
    ) : Impl(node_cache_size_in_bytes, leaf_cache_size_in_bytes)
    { }

    //! \brief A constructor
    //! \param c_ comparator object
    //! \param node_cache_size_in_bytes size of node cache in bytes (btree implementation)
    //! \param leaf_cache_size_in_bytes size of leaf cache in bytes (btree implementation)
    map(    const key_compare & c_,
            unsigned_type node_cache_size_in_bytes,
            unsigned_type leaf_cache_size_in_bytes
    ) : Impl(c_, node_cache_size_in_bytes, leaf_cache_size_in_bytes)
    { }

    //! \brief Constructs a map from a given input range
    //! \param b beginning of the range
    //! \param e end of the range
    //! \param node_cache_size_in_bytes size of node cache in bytes (btree implementation)
    //! \param leaf_cache_size_in_bytes size of leaf cache in bytes (btree implementation)
    //! \param range_sorted if \c true than the constructor assumes that the range is sorted
    //! and performs a fast bottom-up bulk construction of the map (btree implementation)
    //! \param node_fill_factor node fill factor in [0,1] for bulk construction
    //! \param leaf_fill_factor leaf fill factor in [0,1] for bulk construction
    template <class InputIterator>
    map(    InputIterator b,
            InputIterator e,
            unsigned_type node_cache_size_in_bytes,
            unsigned_type leaf_cache_size_in_bytes,
            bool range_sorted = false,
            double node_fill_factor = 0.75,
            double leaf_fill_factor = 0.6
    ) : Impl(b, e, node_cache_size_in_bytes, leaf_cache_size_in_bytes,
             range_sorted, node_fill_factor, leaf_fill_factor)
    { }

    //! \brief Constructs a map from a given input range
    //! \param b beginning of the range
    //! \param e end of the range
    //! \param c_ comparator object
    //! \param node_cache_size_in_bytes size of node cache in bytes (btree implementation)
    //! \param leaf_cache_size_in_bytes size of leaf cache in bytes (btree implementation)
    //! \param range_sorted if \c true than the constructor assumes that the range is sorted
    //! and performs a fast bottom-up bulk construction of the map (btree implementation)
    //! \param node_fill_factor node fill factor in [0,1] for bulk construction
    //! \param leaf_fill_factor leaf fill factor in [0,1] for bulk construction
    template <class InputIterator>
    map(    InputIterator b,
            InputIterator e,
            const key_compare & c_,
            unsigned_type node_cache_size_in_bytes,
            unsigned_type leaf_cache_size_in_bytes,
            bool range_sorted = false,
            double node_fill_factor = 0.75,
            double leaf_fill_factor = 0.6
    ) : Impl(b, e, c_, node_cache_size_in_bytes, leaf_cache_size_in_bytes,
             range_sorted, node_fill_factor, leaf_fill_factor)
    { }

    void swap(map  & obj) { std::swap(Impl, obj.Impl); }
    std::pair<iterator, bool> insert(const value_type & x)
    {
        return Impl.insert(x);
    }
    iterator insert(iterator pos, const value_type & x)
    {
        return Impl.insert(pos, x);
    }
    template <class InputIterator>
    void insert(InputIterator b, InputIterator e)
    {
        Impl.insert(b, e);
    }
    void erase(iterator pos)
    {
        Impl.erase(pos);
    }
    size_type erase(const key_type & k)
    {
        return Impl.erase(k);
    }
    void erase(iterator first, iterator last)
    {
        Impl.erase(first, last);
    }
    void clear()
    {
        Impl.clear();
    }
    iterator find(const key_type & k)
    {
        return Impl.find(k);
    }
    const_iterator find(const key_type & k) const
    {
        return Impl.find(k);
    }
    size_type count(const key_type & k)
    {
        return Impl.count(k);
    }
    iterator lower_bound(const key_type & k)
    {
        return Impl.lower_bound(k);
    }
    const_iterator lower_bound(const key_type & k) const
    {
        return Impl.lower_bound(k);
    }
    iterator upper_bound(const key_type & k)
    {
        return Impl.upper_bound(k);
    }
    const_iterator upper_bound(const key_type & k) const
    {
        return Impl.upper_bound(k);
    }
    std::pair<iterator, iterator> equal_range(const key_type & k)
    {
        return Impl.equal_range(k);
    }
    std::pair<const_iterator, const_iterator> equal_range(const key_type & k) const
    {
        return Impl.equal_range(k);
    }
    data_type & operator[](const key_type & k)
    {
        return Impl[k];
    }

    //! \brief Enables leaf prefetching during scanning
    void enable_prefetching()
    {
        Impl.enable_prefetching();
    }

    //! \brief Disables leaf prefetching during scanning
    void disable_prefetching()
    {
        Impl.disable_prefetching();
    }

    //! \brief Returns the status of leaf prefetching during scanning
    bool prefetching_enabled()
    {
        return Impl.prefetching_enabled();
    }

    //! \brief Prints cache statistics
    void print_statistics(std::ostream & o) const
    {
        Impl.print_statistics(o);
    }

    //! \brief Resets cache statistics
    void reset_statistics()
    {
        Impl.reset_statistics();
    }

    //////////////////////////////////////////////////
    template <      class KeyType_,
              class DataType_,
              class CompareType_,
              unsigned RawNodeSize_,
              unsigned RawLeafSize_,
              class PDAllocStrategy_ >
    friend bool operator==( const map<KeyType_, DataType_, CompareType_, RawNodeSize_, RawLeafSize_, PDAllocStrategy_> & a,
                            const map<KeyType_, DataType_, CompareType_, RawNodeSize_, RawLeafSize_, PDAllocStrategy_> & b);
    //////////////////////////////////////////////////
    template <      class KeyType_,
              class DataType_,
              class CompareType_,
              unsigned RawNodeSize_,
              unsigned RawLeafSize_,
              class PDAllocStrategy_ >
    friend bool operator <( const map<KeyType_, DataType_, CompareType_, RawNodeSize_, RawLeafSize_, PDAllocStrategy_> & a,
                            const map<KeyType_, DataType_, CompareType_, RawNodeSize_, RawLeafSize_, PDAllocStrategy_> & b);
    //////////////////////////////////////////////////
    template <      class KeyType_,
              class DataType_,
              class CompareType_,
              unsigned RawNodeSize_,
              unsigned RawLeafSize_,
              class PDAllocStrategy_ >
    friend bool operator >( const map<KeyType_, DataType_, CompareType_, RawNodeSize_, RawLeafSize_, PDAllocStrategy_> & a,
                            const map<KeyType_, DataType_, CompareType_, RawNodeSize_, RawLeafSize_, PDAllocStrategy_> & b);
    //////////////////////////////////////////////////
    template <      class KeyType_,
              class DataType_,
              class CompareType_,
              unsigned RawNodeSize_,
              unsigned RawLeafSize_,
              class PDAllocStrategy_ >
    friend bool operator !=(        const map<KeyType_, DataType_, CompareType_, RawNodeSize_, RawLeafSize_, PDAllocStrategy_> & a,
                                    const map<KeyType_, DataType_, CompareType_, RawNodeSize_, RawLeafSize_, PDAllocStrategy_> & b);
    //////////////////////////////////////////////////
    template <      class KeyType_,
              class DataType_,
              class CompareType_,
              unsigned RawNodeSize_,
              unsigned RawLeafSize_,
              class PDAllocStrategy_ >
    friend bool operator <=(        const map<KeyType_, DataType_, CompareType_, RawNodeSize_, RawLeafSize_, PDAllocStrategy_> & a,
                                    const map<KeyType_, DataType_, CompareType_, RawNodeSize_, RawLeafSize_, PDAllocStrategy_> & b);
    //////////////////////////////////////////////////
    template <      class KeyType_,
              class DataType_,
              class CompareType_,
              unsigned RawNodeSize_,
              unsigned RawLeafSize_,
              class PDAllocStrategy_ >
    friend bool operator >=(        const map<KeyType_, DataType_, CompareType_, RawNodeSize_, RawLeafSize_, PDAllocStrategy_> & a,
                                    const map<KeyType_, DataType_, CompareType_, RawNodeSize_, RawLeafSize_, PDAllocStrategy_> & b);
    //////////////////////////////////////////////////
};

template <      class KeyType,
          class DataType,
          class CompareType,
          unsigned RawNodeSize,
          unsigned RawLeafSize,
          class PDAllocStrategy
>
inline bool operator==( const map<KeyType, DataType, CompareType, RawNodeSize, RawLeafSize, PDAllocStrategy> & a,
                        const map<KeyType, DataType, CompareType, RawNodeSize, RawLeafSize, PDAllocStrategy> & b)
{
    return a.Impl == b.Impl;
}

template <      class KeyType,
          class DataType,
          class CompareType,
          unsigned RawNodeSize,
          unsigned RawLeafSize,
          class PDAllocStrategy
>
inline bool operator < (const map<KeyType, DataType, CompareType, RawNodeSize, RawLeafSize, PDAllocStrategy> & a,
                        const map<KeyType, DataType, CompareType, RawNodeSize, RawLeafSize, PDAllocStrategy> & b)
{
    return a.Impl < b.Impl;
}

template <      class KeyType,
          class DataType,
          class CompareType,
          unsigned RawNodeSize,
          unsigned RawLeafSize,
          class PDAllocStrategy
>
inline bool operator > (const map<KeyType, DataType, CompareType, RawNodeSize, RawLeafSize, PDAllocStrategy> & a,
                        const map<KeyType, DataType, CompareType, RawNodeSize, RawLeafSize, PDAllocStrategy> & b)
{
    return a.Impl > b.Impl;
}

template <      class KeyType,
          class DataType,
          class CompareType,
          unsigned RawNodeSize,
          unsigned RawLeafSize,
          class PDAllocStrategy
>
inline bool operator != (const map<KeyType, DataType, CompareType, RawNodeSize, RawLeafSize, PDAllocStrategy> & a,
                         const map<KeyType, DataType, CompareType, RawNodeSize, RawLeafSize, PDAllocStrategy> & b)
{
    return a.Impl != b.Impl;
}

template <      class KeyType,
          class DataType,
          class CompareType,
          unsigned RawNodeSize,
          unsigned RawLeafSize,
          class PDAllocStrategy
>
inline bool operator <= (const map<KeyType, DataType, CompareType, RawNodeSize, RawLeafSize, PDAllocStrategy> & a,
                         const map<KeyType, DataType, CompareType, RawNodeSize, RawLeafSize, PDAllocStrategy> & b)
{
    return a.Impl <= b.Impl;
}

template <      class KeyType,
          class DataType,
          class CompareType,
          unsigned RawNodeSize,
          unsigned RawLeafSize,
          class PDAllocStrategy
>
inline bool operator >= (const map<KeyType, DataType, CompareType, RawNodeSize, RawLeafSize, PDAllocStrategy> & a,
                         const map<KeyType, DataType, CompareType, RawNodeSize, RawLeafSize, PDAllocStrategy> & b)
{
    return a.Impl >= b.Impl;
}

//! \}

__STXXL_END_NAMESPACE

namespace std
{
    template <      class KeyType,
              class DataType,
              class CompareType,
              unsigned RawNodeSize,
              unsigned RawLeafSize,
              class PDAllocStrategy
    >
    void swap(      stxxl::map < KeyType, DataType, CompareType, RawNodeSize, RawLeafSize, PDAllocStrategy > & a,
                    stxxl::map<KeyType, DataType, CompareType, RawNodeSize, RawLeafSize, PDAllocStrategy> & b
    )
    {
        a.swap(b);
    }
}

#endif
