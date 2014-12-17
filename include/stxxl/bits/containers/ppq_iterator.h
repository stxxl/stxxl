//#define PPQ_VERBOSE_ITERATOR(x) STXXL_MSG(x);
#define PPQ_VERBOSE_ITERATOR(x) 

//! A random-access iterator class for block oriented data.
//! The iterator is intended to be provided by the internal_array and
//! external_array classes and to be used by the multiway_merge algorithm.
//! 
//! \tparam ValueType the value type
template <class ValueType>
class ppq_iterator {
    
public:

    typedef ValueType value_type;
    typedef value_type& reference;
    typedef value_type* pointer;
    typedef ptrdiff_t difference_type;
    typedef std::random_access_iterator_tag iterator_category;
    
    typedef std::vector< std::pair<pointer,pointer> > block_pointers_type;

protected:

    typedef ppq_iterator self_type;
    
    //! reference to a vector of begin/end pointer pairs
    //! They allow access to the data blocks.
    //! Unfortunately this cannot be const because we need copy-construction
    block_pointers_type& m_block_pointers;
    
    //! empty dummy vector for the default constructor
    block_pointers_type m_dummy_blocks;
    
    //! pointer to the current element
    pointer m_current;
    
    //! index of the current element
    size_t m_index;
    
    //! index of the current element's block
    size_t m_block_index;
    
    //! size of each data block
    size_t m_block_size;
    
public:

    //! default constructor
    //! should not be used directly
    ppq_iterator()
        : m_block_pointers(m_dummy_blocks)
    { }

    //! constructor
    //! 
    //! \param block_pointers   A reference to the properly initialized vector of begin and end pointers.
    //!                         One pair for each block. The pointers should be valid for all blocks that
    //!                         are expected to be accessed with this iterator.
    //! \param block_size       The size of a single block. If there is only one block (e.g. if the iterator
    //!                         belongs to an internal_array), use the total size here.
    //! \param index            The index of the current element (global - index 0 belongs to the first element
    //!                         in the first block, no matter if the values are still valid)
    ppq_iterator(block_pointers_type& block_pointers, size_t block_size, size_t index)
        : m_block_pointers(block_pointers),
        m_index(index),
        m_block_size(block_size)
    {
        m_block_index = index / block_size;
        const size_t local_index = index % block_size;
        
        if ( m_block_index < m_block_pointers.size() ) {
            m_current = m_block_pointers[m_block_index].first + local_index;
            assert(m_current < m_block_pointers[m_block_index].second);
        } else {
            // global end
            assert(m_block_index==m_block_pointers.size());
            assert(local_index==0);
            m_current = m_block_pointers[m_block_index-1].second;
        }
    }
    
    //! copy constructor
    ppq_iterator(const self_type& o)
        : m_block_pointers(o.m_block_pointers),
        m_current(o.m_current),
        m_index(o.m_index),
        m_block_index(o.m_block_index),
        m_block_size(o.m_block_size)
    { }
        
    //! copy assignment
    self_type& operator = (const self_type& o)
    {
        m_block_pointers = o.m_block_pointers;
        m_current = o.m_current;
        m_index = o.m_index;
        m_block_index = o.m_block_index;
        m_block_size = o.m_block_size;
        return *this;
    }
    
    //! returns the value's index in the external array
    size_t get_index() const
    {
        return m_index;
    }
    
    reference operator* () const
    {
        //PPQ_VERBOSE_ITERATOR("*");
        return *m_current;
    }
    pointer operator-> () const
    {
        //PPQ_VERBOSE_ITERATOR("->");
        return m_current;
    }
    reference operator[] (difference_type index) const
    {
        PPQ_VERBOSE_ITERATOR("[] myindex="<<m_index<<" index="<<index);
        
        const size_t block_index = index / m_block_size;
        const size_t local_index = index % m_block_size;
        
        if ( block_index >= m_block_pointers.size() ) {
            // STXXL_MSG("Workaround for multiway_merge bug");
            assert(block_index == m_block_pointers.size());
            return *(m_block_pointers[block_index-1].second);
        }
        
        assert(block_index<m_block_pointers.size());
        assert(m_block_pointers[block_index].first + local_index
                < m_block_pointers[block_index].second);
        
        PPQ_VERBOSE_ITERATOR("accessing block_index="<<block_index
            <<" local_index="<<local_index<<" value="
            <<(m_block_pointers[block_index].first + local_index)->first);
        
        return *(m_block_pointers[block_index].first + local_index);
    }
    
    //! pre-increment operator
    self_type& operator ++ ()
    {
        PPQ_VERBOSE_ITERATOR("++ old_index = "<<m_index<<" old current="<<m_current->first);
        ++m_index;
        ++m_current;
        
        if (m_current == m_block_pointers[m_block_index].second) {
            if ( m_block_index+1 < m_block_pointers.size() ) {
                m_current = m_block_pointers[++m_block_index].first;
            } else {
                // global end
                assert(m_block_index+1==m_block_pointers.size());
                m_current = m_block_pointers[m_block_index++].second;
            }
        }
        
        PPQ_VERBOSE_ITERATOR("++ new_index = "<<m_index<<" new current="<<m_current->first);
        
        return *this;
    }
    //! post-increment operator
    self_type operator ++ (int)
    {
        PPQ_VERBOSE_ITERATOR("++(int)");
        self_type former(*this);
        operator ++ ();
        return former;
    }
    //! pre-increment operator
    self_type& operator -- ()
    {
        PPQ_VERBOSE_ITERATOR("--");
        --m_index;
        
        if ( m_block_index >= m_block_pointers.size()
            || m_current == m_block_pointers[m_block_index].first ) {
            // begin of current block or global end
            assert(m_block_index>0);
            assert(m_block_index<=m_block_pointers.size());
            m_current = m_block_pointers[--m_block_index].second-1;
        } else {
            --m_current;
        }
        
        return *this;
    }
    //! post-increment operator
    self_type operator -- (int)
    {
        PPQ_VERBOSE_ITERATOR("--(int)");
        self_type former(*this);
        operator -- ();
        return former;
    }
    self_type operator + (difference_type addend) const
    {
        //PPQ_VERBOSE_ITERATOR("+int");
        return self_type(m_block_pointers, m_block_size, m_index+addend);
    }
    self_type& operator += (difference_type addend)
    {
        PPQ_VERBOSE_ITERATOR("+=int myindex="<<m_index<<" addend="<<addend);
        m_index += addend;
        m_current = &(operator[](m_index));
        return *this;
    }
    self_type operator - (difference_type subtrahend) const
    {
        PPQ_VERBOSE_ITERATOR("-int");
        return self_type(m_block_pointers, m_block_size, m_index-subtrahend);
    }
    difference_type operator - (const self_type& o) const
    {
        PPQ_VERBOSE_ITERATOR("-iterator myindex="<<m_index<<" otherindex="<<o.m_index);
        return (m_index-o.m_index);
    }
    self_type& operator -= (difference_type subtrahend)
    {
        PPQ_VERBOSE_ITERATOR("-=int");
        m_index -= subtrahend;
        m_current = &(operator[](m_index));
        return *this;
    }
    bool operator == (const self_type& o) const
    {
        return m_index == o.m_index;
    }
    bool operator != (const self_type& o) const
    {
        return m_index != o.m_index;
    }
    bool operator < (const self_type& o) const
    {
        return m_index < o.m_index;
    }
    bool operator <= (const self_type& o) const
    {
        return m_index <= o.m_index;
    }
    bool operator > (const self_type& o) const
    {
        return m_index > o.m_index;
    }
    bool operator >= (const self_type& o) const
    {
        return m_index >= o.m_index;
    }
};
