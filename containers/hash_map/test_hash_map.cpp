#include <iostream>

#include "stxxl.h"
#include "stxxl/bits/common/seed.h"
#include "stxxl/bits/common/rand.h"
#include "stxxl/bits/containers/hash_map/hash_map.h"

struct rand_pairs {
    stxxl::random_number32 & rand_;

    rand_pairs(stxxl::random_number32 & rand) : rand_(rand)
    { }

    std::pair<int, int> operator () ()
    {
        int v = rand_();
        return std::pair<int, int>(v, v);
    }
};

struct hash_int {
    stxxl::uint64 operator () (stxxl::uint64 key) const
    {
        key = (~key) + (key << 21);                    // key = (key << 21) - key - 1;
        key = key ^ (key >> 24);
        key = (key + (key << 3)) + (key << 8);         // key * 265
        key = key ^ (key >> 14);
        key = (key + (key << 2)) + (key << 4);         // key * 21
        key = key ^ (key >> 28);
        key = key + (key << 31);
        return key;
    }

    stxxl::uint64 hash(stxxl::uint64 key) const { return (*this)(key); }
};

struct cmp : public std::less<int>{
    int min_value() const { return std::numeric_limits<int>::min(); }
    int max_value() const { return std::numeric_limits<int>::max(); }
};


void basic_test()
{
    typedef std::pair<int, int> value_type;
    const unsigned value_size = sizeof(value_type);


    const unsigned n_values = 50000;
    const unsigned n_tests = 1000;

    const unsigned buffer_size = 5 * n_values * (value_size + sizeof(int *));   // make sure all changes will be buffered (*)

    const unsigned mem_to_sort = 10 * 1024 * 1024;

    const unsigned subblock_raw_size = 4 * 1024;
    const unsigned block_size = 4;

    typedef stxxl::hash_map::hash_map<int, int, hash_int, cmp, subblock_raw_size, block_size> hash_map;
    typedef hash_map::iterator iterator;
    typedef hash_map::const_iterator const_iterator;


    hash_map map;
    map.max_buffer_size(buffer_size);
    const hash_map & cmap = map;


    // generate random values
    stxxl::random_number32 rand32;

    std::vector<value_type> values1(n_values);
    std::vector<value_type> values2(n_values);
    std::vector<value_type> values3(n_values / 2);
    std::generate(values1.begin(), values1.end(), rand_pairs(rand32));
    std::generate(values2.begin(), values2.end(), rand_pairs(rand32));
    std::generate(values3.begin(), values3.end(), rand_pairs(rand32));


    // --- initial import
    std::cout << "Initial import...";
    assert(map.begin() == map.end());
    map.insert(values1.begin(), values1.end(), mem_to_sort);
    assert(map.begin() != map.end());
    assert(map.size() == n_values);
    std::cout << "passed" << std::endl;
    // (*) all these values are stored in external memory; the remaining changes will be buffered in internal memory


    // --- insert: new (from values2) and existing (from values1) values, with and without checking
    std::cout << "Insert...";
    for (unsigned i = 0; i < n_values / 2; i++) {
        // new without checking
        map.insert_oblivious(values2[2 * i]);
        // new with checking
        std::pair<iterator, bool> res = map.insert(values2[2 * i + 1]);
        assert(res.second && (*(res.first)).first == values2[2 * i + 1].first);
        // existing without checking
        map.insert_oblivious(values1[2 * i]);
        // exiting with checking
        res = map.insert(values1[2 * i + 1]);
        assert(!res.second && (*(res.first)).first == values1[2 * i + 1].first);
    }
    assert(map.size() == 2 * n_values);
    std::cout << "passed" << std::endl;
    // "old" values are stored in external memory, "new" values are stored in internal memory


    // --- find: existing (from external and internal memory) and non-existing values
    std::cout << "Find...";
    std::random_shuffle(values1.begin(), values1.end());
    std::random_shuffle(values2.begin(), values2.end());
    for (unsigned i = 0; i < n_tests; i++) {
        assert(cmap.find(values1[i].first) != cmap.end());
        assert(cmap.find(values2[i].first) != cmap.end());
        assert(cmap.find(values3[i].first) == cmap.end());
    }
    std::cout << "passed" << std::endl;

    // --- insert with overwriting
    std::cout << "Insert with overwriting...";
    std::random_shuffle(values1.begin(), values1.end());
    std::random_shuffle(values2.begin(), values2.end());
    for (unsigned i = 0; i < n_tests; i++) {
        value_type value1 = values1[i];         // in external memory
        value1.second++;
        map.insert_oblivious(value1);

        value_type value2 = values2[i];         // in internal memory
        value2.second++;
        map.insert_oblivious(value2);
    }
    // now check
    assert(map.size() == 2 * n_values);         // nothing added, nothing removed
    for (unsigned i = 0; i < n_tests; i++) {
        const_iterator it1 = cmap.find(values1[i].first);
        const_iterator it2 = cmap.find(values2[i].first);

        assert((*it1).second == values1[i].second + 1);
        assert((*it2).second == values2[i].second + 1);
    }
    std::cout << "passed" << std::endl;


    // --- erase: existing and non-existing values, with and without checking
    std::cout << "Erase...";
    std::random_shuffle(values1.begin(), values1.end());
    std::random_shuffle(values2.begin(), values2.end());
    std::random_shuffle(values3.begin(), values3.end());
    for (unsigned i = 0; i < n_tests / 2; i++) {        // external
        // existing without checking
        map.erase_oblivious(values1[2 * i].first);
        // existing with checking
        assert(map.erase(values1[2 * i + 1].first) == 1);
    }
    for (unsigned i = 0; i < n_tests / 2; i++) {        // internal
        // existing without checking
        map.erase_oblivious(values2[2 * i].first);
        // existing with checking
        assert(map.erase(values2[2 * i + 1].first) == 1);
        // non-existing without chekcing
        map.erase_oblivious(values3[i].first);
        // non-existing with checking
    }
    assert(map.size() == 2 * n_values - 2 * n_tests);
    std::cout << "passed" << std::endl;

    map.clear();
    assert(map.size() == 0);

    std::cout << "\nAll tests passed" << std::endl;


    map.buffer_size();
}


int main()
{
    basic_test();

    return 0;
}
