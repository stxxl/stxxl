#include <stxxl/bits/verbose.h>

#include "ppq_index_winner_tree.h"

STXXL_BEGIN_NAMESPACE

template <class Parent>
class minima_tree {

    typedef minima_tree<Parent> Self;
    typedef typename Parent::inv_compare_type compare_type;
    typedef typename Parent::value_type value_type;
    typedef typename Parent::heaps_type heaps_type;
    typedef typename Parent::internal_arrays_type ias_type;
    typedef typename Parent::external_arrays_type eas_type;

    static const unsigned initial_ia_size = 2;
    static const unsigned initial_ea_size = 2;

public:

    enum Types {
        HEAP = 0,
        EB = 1,
        IA = 2,
        EA = 3,
        ERROR = 4
    };

    minima_tree(Parent& parent) :
            m_parent(parent),
            m_compare(),

            m_cache_line_factor(m_parent.c_cache_line_factor),
            
            m_head_comp(*this, parent.insertion_heaps, parent.internal_arrays, parent.external_arrays, m_compare, m_cache_line_factor),
            m_heaps_comp(parent.insertion_heaps,m_compare,m_cache_line_factor),
            m_ia_comp(parent.internal_arrays,m_compare),
            m_ea_comp(parent.external_arrays,m_compare),
            
            m_head(4,m_head_comp),
            m_heaps(m_parent.num_insertion_heaps,m_heaps_comp),
            m_ia(initial_ia_size,m_ia_comp),
            m_ea(initial_ea_size,m_ea_comp) {
    }


    std::pair<unsigned,unsigned> top() {
        unsigned type = m_head.top();
        switch (type)
        {
            case HEAP: return std::make_pair(HEAP,m_heaps.top()); break;
            case EB: return std::make_pair(EB,0); break;
            case IA: return std::make_pair(IA,m_ia.top()); break;
            case EA: return std::make_pair(EA,m_ea.top()); break;
            default: return std::make_pair(ERROR,0);
        }
    }

    void update_heap(unsigned index)
    {
        m_heaps.notify_change(index);
        m_head.notify_change(HEAP);
    }

    void update_extract_buffer()
    {
        m_head.notify_change(EB);
    }

    void update_internal_array(unsigned index)
    {
        m_ia.notify_change(index);
        m_head.notify_change(IA);
    }

    void update_external_array(unsigned index)
    {
        m_ea.notify_change(index);
        m_head.notify_change(EA);
    }

    void add_internal_array(unsigned index)
    {
        m_ia.activate_player(index);
        m_head.notify_change(IA);
    }

    void add_external_array(unsigned index)
    {
        m_ea.activate_player(index);
        m_head.notify_change(EA);
    }

    void deactivate_heap(unsigned index)
    {
        m_heaps.deactivate_player(index);
        if (!m_heaps.empty()) {
             m_head.notify_change(HEAP);
        } else {
             m_head.deactivate_player(HEAP);
        }
    }

    void deactivate_extract_buffer()
    {
        m_head.deactivate_player(EB);
    }

    void deactivate_internal_array(unsigned index)
    {
        m_ia.deactivate_player(index);
        if (!m_ia.empty()) {
             m_head.notify_change(IA);
        } else {
             m_head.deactivate_player(IA);
        }
    }

    void deactivate_external_array(unsigned index)
    {
        m_ea.deactivate_player(index);
        if (!m_ea.empty()) {
             m_head.notify_change(EA);
        } else {
             m_head.deactivate_player(EA);
        }
    }

    void clear_heaps()
    {
        m_heaps.clear();
        m_head.deactivate_player(HEAP);
    }
    
    void clear_internal_arrays()
    {
        m_ia.resize_and_clear(initial_ia_size);
        m_head.deactivate_player(IA);
    }

    void clear_external_arrays()
    {
        m_ea.resize_and_clear(initial_ea_size);
        m_head.deactivate_player(EA);
    }

    //! Returns a readable representation of the winner tree as string.
    std::string to_string() const
    {
        std::stringstream ss;
        ss << "Head:" << std::endl << m_head.to_string() << std::endl;
        ss << "Heaps:" << std::endl << m_heaps.to_string() << std::endl;
        ss << "IA:" << std::endl << m_ia.to_string() << std::endl;
        ss << "EA:" << std::endl << m_ea.to_string() << std::endl;
        return ss.str();
    }

    //! Prints statistical data.
    void print_stats() const
    {
        STXXL_MSG("Head winner tree stats:");
        m_head.print_stats();
        STXXL_MSG("Heaps winner tree stats:");
        m_heaps.print_stats();
        STXXL_MSG("IA winner tree stats:");
        m_ia.print_stats();
        STXXL_MSG("EA winner tree stats:");
        m_ea.print_stats();
    }
    
private:

    //! Comparator for the head winner tree.
    //! It accesses all relevant data structures from the priority queue.
    struct head_comp {
    
        Self& m_parent;
        heaps_type& m_heaps;
        ias_type& m_ias;
        eas_type& m_eas;
        compare_type& m_compare;
        unsigned m_cache_line_factor;
        
        head_comp(Self& parent, heaps_type& heaps, ias_type& ias, eas_type& eas, compare_type& compare, unsigned cache_line_factor)
            : m_parent(parent),
                m_heaps(heaps),
                m_ias(ias),
                m_eas(eas),
                m_compare(compare),
                m_cache_line_factor(cache_line_factor) { }
            
        bool operator () (const int a, const int b) const
        {
            value_type val_a;
            switch(a) {
                case HEAP: val_a = m_heaps[m_parent.m_heaps.top()*m_cache_line_factor][0]; break;
                case EB: val_a = m_parent.m_parent.extract_buffer[m_parent.m_parent.extract_index]; break;
                case IA: val_a = m_ias[m_parent.m_ia.top()].get_min(); break;
                case EA: val_a = m_eas[m_parent.m_ea.top()].get_min_element(); break;
            }
            value_type val_b;
            switch(b) {
                case HEAP: val_b = m_heaps[m_parent.m_heaps.top()*m_cache_line_factor][0]; break;
                case EB: val_b = m_parent.m_parent.extract_buffer[m_parent.m_parent.extract_index]; break;
                case IA: val_b = m_ias[m_parent.m_ia.top()].get_min(); break;
                case EA: val_b = m_eas[m_parent.m_ea.top()].get_min_element(); break;
            }
            return m_compare(val_a,val_b);
        }
    };
    
    //! Comparator for the insertion heaps winner tree.
    struct heaps_comp {
        heaps_type& m_heaps;
        compare_type& m_compare;
        unsigned m_cache_line_factor;
        heaps_comp(heaps_type& heaps, compare_type& compare, unsigned cache_line_factor)
            : m_heaps(heaps), m_compare(compare), m_cache_line_factor(cache_line_factor) { }
        bool operator () (const int a, const int b) const
        {
            return m_compare(m_heaps[a*m_cache_line_factor][0],m_heaps[b*m_cache_line_factor][0]);
        }
    };
    
    //! Comparator for the internal arrays winner tree.
    struct ia_comp {
        ias_type& m_ias;
        compare_type& m_compare;
        ia_comp(ias_type& ias, compare_type& compare)
            : m_ias(ias), m_compare(compare) { }
        bool operator () (const int a, const int b) const
        {
            return m_compare(m_ias[a].get_min(),m_ias[b].get_min());
        }
    };
    
    //! Comparator for the external arrays winner tree.
    struct ea_comp {
        eas_type& m_eas;
        compare_type& m_compare;
        ea_comp(eas_type& eas, compare_type& compare)
            : m_eas(eas), m_compare(compare) { }
        bool operator () (const int a, const int b) const
        {
            return m_compare(m_eas[a].get_min_element(),m_eas[b].get_min_element());
        }
    };

    //! The priority queue
    Parent& m_parent;
    
    //! value_type comparator
    compare_type m_compare;

    unsigned m_cache_line_factor;

    //! Comperator instances
    head_comp m_head_comp;
    heaps_comp m_heaps_comp;
    ia_comp m_ia_comp;
    ea_comp m_ea_comp;
    
    //! The winner trees
    index_winner_tree< head_comp > m_head;
    index_winner_tree< heaps_comp > m_heaps;
    index_winner_tree< ia_comp > m_ia;
    index_winner_tree< ea_comp > m_ea;
    
};


STXXL_END_NAMESPACE
