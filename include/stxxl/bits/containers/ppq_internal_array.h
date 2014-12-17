/*!
 * Internal arrays store a sorted sequence of values in RAM, which will be
 * merged together into the deletion buffer when it needs to be
 * refilled. Internal arrays are constructed from the insertions heaps when
 * they overflow.
 */
template <class ValueType>
class internal_array : private noncopyable
{
public:

    typedef ValueType value_type;
    typedef ppq_iterator<value_type> iterator;

protected:

    typedef typename iterator::block_pointers_type block_pointers_type;

    //! Contains the items of the sorted sequence.
    std::vector<value_type> m_values;

    //! Index of the current head
    size_t m_min_index;

    //! Begin and end pointers of the array
    //! This is used by the iterator
    block_pointers_type m_block_pointers;

public:

    //! Default constructor. Don't use this directy. Needed for regrowing in
    //! surrounding vector.
    internal_array() { }

    //! Constructor which takes a value vector. The value vector is empty
    //! afterwards.
    internal_array(std::vector<ValueType>& values)
        : m_values(), m_min_index(0), m_block_pointers(1)
    {
        std::swap(m_values, values);
        m_block_pointers[0] = std::make_pair(&(*m_values.begin()), &(*m_values.end()));
    }

    //! Swap internal_array with another one.
    void swap(internal_array& o)
    {
        std::swap(m_values, o.m_values);
        std::swap(m_min_index, o.m_min_index);
        std::swap(m_block_pointers, o.m_block_pointers);
    }

    //! Random access operator
    inline ValueType& operator [] (size_t i)
    {
        return m_values[i];
    }

    //! Use inc_min if a value has been extracted.
    inline void inc_min()
    {
        m_min_index++;
    }

    //! Use inc_min(diff) if multiple values have been extracted.
    inline void inc_min(size_t diff)
    {
        m_min_index += diff;
    }

    //! The currently smallest element in the array.
    inline const ValueType & get_min() const
    {
        return m_values[m_min_index];
    }

    //! The index of the currently smallest element in the array.
    inline size_t get_min_index() const
    {
        return m_min_index;
    }

    //! The index of the largest element in the array.
    inline size_t get_max_index() const
    {
        return (m_values.size() - 1);
    }

    //! Returns if the array has run empty.
    inline bool empty() const
    {
        return (m_min_index >= m_values.size());
    }

    //! Returns the current size of the array.
    inline size_t size() const
    {
        return (m_values.size() - m_min_index);
    }

    //! Returns the initial size of the array.
    inline size_t capacity() const
    {
        return m_values.size();
    }

    //! Return the amount of internal memory used by the array
    inline size_t int_memory() const
    {
        return m_values.capacity() * sizeof(ValueType);
    }

    //! Begin iterator
    inline iterator begin()
    {
        // not const, unfortunately.
        return iterator(m_block_pointers, capacity(), m_min_index);
    }

    //! End iterator
    inline iterator end()
    {
        // not const, unfortunately.
        return iterator(m_block_pointers, capacity(), capacity());
    }
};
