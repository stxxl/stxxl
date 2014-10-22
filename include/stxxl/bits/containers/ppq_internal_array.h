    class internal_array
    {
    private:
        std::vector<value_type> m_values;
        size_type m_min_index;
        bool m_deleted;

    public:
        //! The value iterator type used by begin() and end()
        //! We use pointers as iterator so internal arrays
        //! are compatible to external arrays and can be
        //! merged together.
        typedef value_type* iterator;

        //! Default constructor. Don't use this directy. Needed for regrowing in surrounding vector.
        internal_array() = default;

        //! Constructor which takes a value vector.
        //! The vector should not be used outside this
        //! class anymore!
        internal_array(std::vector<value_type>& values)
            : m_values(), m_min_index(0), m_deleted(false)
        {
            std::swap(m_values, values);
        }
        
        //! Move constructor. Needed for regrowing in surrounding vector.
        internal_array(internal_array&& o)
            : m_values(std::move(o.m_values)),
                m_min_index(o.m_min_index),
                m_deleted(o.m_deleted) { }
        
    
        //! Delete copy assignment for emplace_back to use the move semantics.
        internal_array& operator=(internal_array& other) = delete;
        
        //! Delete copy constructor for emplace_back to use the move semantics.
        internal_array(const internal_array& other) = delete;
        
        //! Move assignment.
        internal_array& operator=(internal_array&&){
            return *this;
        }

        //! Random access operator
        inline value_type& operator [] (size_t i) const
        {
            return m_values[i];
        }

        //! Use inc_min if a value has been extracted.
        inline void inc_min()
        {
            m_min_index++;
        }

        //! Use inc_min(diff) if multiple values have been extracted.
        inline void inc_min(size_type diff)
        {
            m_min_index += diff;
        }

        //! The currently smallest element in the array.
        inline const value_type & get_min() const
        {
            return m_values[m_min_index];
        }

        //! The index of the currently smallest element in the array.
        inline size_type get_min_index() const
        {
            return m_min_index;
        }

        //! The index of the largest element in the array.
        inline size_type get_max_index() const
        {
            return (m_values.size() - 1);
        }

        //! Returns if the array has run empty.
        inline bool empty() const
        {
            return (m_min_index >= m_values.size());
        }

        //! Returns the current size of the array.
        inline size_type size() const
        {
            return (m_values.size() - m_min_index);
        }

        //! Begin iterator
        inline iterator begin()
        {
            // We use &(*()) in order to get a pointer iterator.
            // This is allowed because values are guaranteed to be
            // consecutive in std::vecotor.
            return &(*(m_values.begin() + m_min_index));
        }

        //! End iterator
        inline iterator end()
        {
            // We use &(*()) in order to get a pointer iterator.
            // This is allowed because values are guaranteed to be
            // consecutive in std::vecotor.
            return &(*(m_values.end()));
        }
    };
