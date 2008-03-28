#include "stxxl/mng"
#include "stxxl/sort"
#include "stxxl/vector"

//! \example algo/test_sort1.cpp
//! This is an example of how to use \c stxxl::sort() algorithm

using namespace stxxl;

#define RECORD_SIZE 8

struct my_type
{
    typedef unsigned key_type;

    key_type _key;
    char _data[RECORD_SIZE - sizeof(key_type)];
    key_type key() const
    {
        return _key;
    }

    my_type() { };
    my_type(key_type __key) : _key(__key) { };

    static my_type min_value()
    {
        return my_type(0);
    }
    static my_type max_value()
    {
        return my_type(0xffffffff);
    }

    ~my_type() { }
};

std::ostream & operator << (std::ostream & o, const my_type & obj)
{
    o << obj._key;
    return o;
}

bool operator < (const my_type & a, const my_type & b)
{
    return a.key() < b.key();
}

bool operator == (const my_type & a, const my_type & b)
{
    return a.key() == b.key();
}

bool operator != (const my_type & a, const my_type & b)
{
    return a.key() != b.key();
}

struct cmp : public std::less<my_type>
{
    my_type min_value() const
    {
        return my_type(0);
    }
    my_type max_value() const
    {
        return my_type(0xffffffff);
    }
};

#define BLOCK_COUNT 3
typedef u_int64_t word_type;

enum node_state {
	ns_normal,
	ns_min,
	ns_max
};

struct trie {
	trie() {
		state = ns_normal;
	};
	trie(node_state state_):
		state(state_), Id(0), ParentId(0), i(0), branch_pos(0) {};
	node_state state;	
	unsigned Id;
	unsigned ParentId;
	unsigned i;
	unsigned branch_pos;
	word_type branch[BLOCK_COUNT];
	word_type cp1[BLOCK_COUNT];
	word_type cp2[BLOCK_COUNT];
};

/* Sort based on (i, branch_pos) */
struct trie_compare_first {
	bool operator() (const trie & l, const trie & r) const
	{
		if ((l.state == ns_min) || (r.state == ns_max)) return true;
		if ((r.state == ns_min) || (l.state == ns_max)) return false;
		if (l.i < r.i) return true;
		if (l.i == r.i)
			return (l.branch_pos < r.branch_pos);
		return false;
	};
	trie min_value() const {
		return trie(ns_min);
	};
	trie max_value() const {
		return trie(ns_max);
	};
};


int main()
{
    unsigned memory_to_use = 16 * 1024 * 1024;
    typedef stxxl::vector<trie> vector_type;
    const stxxl::int64 n_records =
        stxxl::int64(64) * stxxl::int64(1024 * 1024) / sizeof(trie);
    vector_type v(n_records);

    random_number32 rnd;
    STXXL_MSG("Filling vector..., input size =" << v.size());
    for (vector_type::size_type i = 0; i < v.size(); i++)
        v[i].i = 1 + (rnd() % 0xffff);


    STXXL_MSG("Checking order...");
    STXXL_MSG( ((stxxl::is_sorted(v.begin(), v.end(), trie_compare_first())) ? "OK" : "WRONG" ));

    STXXL_MSG("Sorting...");
    stxxl::sort(v.begin(), v.end(), trie_compare_first(), memory_to_use);

    STXXL_MSG("Checking order...");
    STXXL_MSG( ((stxxl::is_sorted(v.begin(), v.end(), trie_compare_first())) ? "OK" : "WRONG" ));


    STXXL_MSG("Done, output size=" << v.size());

    return 0;
}
