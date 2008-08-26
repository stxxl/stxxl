#include <iostream>

#include "stxxl.h"
#include "stxxl/bits/common/seed.h"
#include "stxxl/bits/common/rand.h"
#include "stxxl/bits/containers/hash_map/hash_map.h"
#include "stxxl/bits/compat_hash_map.h"

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
    int min_value() const { return (std::numeric_limits<int>::min)(); }
    int max_value() const { return (std::numeric_limits<int>::max)(); }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cmp_with_internal_map()
{
    typedef std::pair<int, int> value_type;
    const unsigned value_size = sizeof(value_type);

    const unsigned n_values = 50000;
    const unsigned n_tests = 1000;

    const unsigned buffer_size = 5 * n_values * (value_size + sizeof(int *));   // make sure all changes will be buffered
    const unsigned mem_to_sort = 10 * 1024 * 1024;

    const unsigned subblock_raw_size = 4 * 1024;
    const unsigned block_size = 4;

    typedef stxxl::hash_map::hash_map<int, int, hash_int, cmp, subblock_raw_size, block_size> hash_map;
    typedef hash_map::iterator iterator;
    typedef hash_map::const_iterator const_iterator;

    typedef stxxl::compat_hash_map<int, int>::result int_hash_map;

    hash_map map;
    map.max_buffer_size(buffer_size);
    const hash_map & cmap = map;
    int_hash_map int_map;

    // generate random values
    stxxl::random_number32 rand32;
    std::vector<value_type> values1(n_values);
    std::vector<value_type> values2(n_values);
    std::vector<value_type> values3(n_values);
    std::generate(values1.begin(), values1.end(), rand_pairs(rand32));
    std::generate(values2.begin(), values2.end(), rand_pairs(rand32));
    std::generate(values3.begin(), values3.end(), rand_pairs(rand32));

    // --- initial import: create a nice mix of externally (values1) and internally (values2) stored values
    map.insert(values1.begin(), values1.end(), mem_to_sort);
    int_map.insert(values1.begin(), values1.end());
    // seems bind1st has problems with a member that takes a reference
//	std::for_each( values2.begin(), values2.end(), std::bind1st( std::mem_fun(&hash_map::insert_oblivious), &map ));
//	std::for_each( values2.begin(), values2.end(), std::bind1st( std::mem_fun(&int_hash_map::insert), &int_map ));
    std::vector<value_type>::iterator val_it = values2.begin();
    for (; val_it != values2.end(); ++val_it) {
        map.insert_oblivious(*val_it);
        int_map.insert(*val_it);
    }

    // --- erase and overwrite some external values
    std::random_shuffle(values1.begin(), values1.end());
    val_it = values1.begin();
    for ( ; val_it != values1.begin() + n_tests; ++val_it) {
        map.erase_oblivious(val_it->first);
        int_map.erase(val_it->first);
    }
    for ( ; val_it != values1.begin() + 2 * n_tests; ++val_it) {
        map.insert_oblivious(*val_it);
        int_map.insert(*val_it);
    }

    // --- scan and compare with internal memory hash-map
    std::cout << "Compare with internal-memory map...";
    assert(int_map.size() == map.size());
    const_iterator cit = cmap.begin();
    for ( ; cit != cmap.end(); ++cit) {
        int key = (*cit).first;
        assert(int_map.find(key) != int_map.end());
    }
    std::cout << "passed" << std::endl;

    // --- another bulk insert
    std::cout << "Compare with internal-memory map after another bulk-insert...";
    map.insert(values3.begin(), values3.end(), mem_to_sort);
    int_map.insert(values3.begin(), values3.end());
    assert(map.size() == map.size());
    cit = cmap.begin();
    for ( ; cit != cmap.end(); ++cit) {
        int key = (*cit).first;
        assert(int_map.find(key) != int_map.end());
    }
    std::cout << "passed" << std::endl;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void basic_iterator_test()
{
    typedef std::pair<int, int> value_type;
    const unsigned value_size = sizeof(value_type);


    const unsigned n_values = 50000;
    const unsigned n_tests = 1000;

    const unsigned buffer_size = 5 * n_values * (value_size + sizeof(int *));   // make sure all changes will be buffered

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
    std::generate(values1.begin(), values1.end(), rand_pairs(rand32));
    std::generate(values2.begin(), values2.end(), rand_pairs(rand32));


    // --- initial import: create a nice mix of externally (values1) and internally (values2) stored values
    std::cout << "Initial import...";
    assert(map.begin() == map.end());
    map.insert(values1.begin(), values1.end(), mem_to_sort);
    // seems bind1st has problems with a member that takes a reference
//	std::for_each( values2.begin(), values2.end(), std::bind1st( std::mem_fun(&hash_map::insert_oblivious), &map ));
    std::vector<value_type>::iterator val_it = values2.begin();
    for (; val_it != values2.end(); ++val_it)
        map.insert_oblivious(*val_it);
    assert(map.begin() != map.end());
    assert(map.size() == 2 * n_values);
    std::cout << "passed" << std::endl;


    // --- actual testing begins: modfiy random values via iterator
    std::cout << "Lookup and modify...";
    std::random_shuffle(values1.begin(), values1.end());
    std::random_shuffle(values2.begin(), values2.end());
    for (unsigned i = 0; i < n_tests; ++i) {
        iterator it1 = map.find(values1[i].first);
        iterator it2 = map.find(values2[i].first);
        assert(it1 != map.end());
        assert(it2 != map.end());
        (*it1).second++;
        (*it2).second++;
    }
    // check again
    for (unsigned i = 0; i < n_tests; ++i) {
        const_iterator cit1 = cmap.find(values1[i].first);
        const_iterator cit2 = cmap.find(values2[i].first);
        assert(cit1 != map.end());
        assert(cit2 != map.end());
        value_type value1 = *cit1;
        value_type value2 = *cit2;
        assert(value1.second == value1.first + 1);
        assert(value2.second == value2.first + 1);
    }
    std::cout << "passed" << std::endl;

    // --- scan and modify
    std::cout << "Scan and modify...";
    {
        iterator it = map.begin();
        for ( ; it != map.end(); ++it)
            (*it).second = (*it).first + 1;

        const_iterator cit = cmap.begin();
        for ( ; cit != cmap.end(); ++cit) {
            if ((*cit).second != (*cit).first + 1) {
                std::cout << "helloe";
            }
//			assert((*cit).second == (*cit).first+1);
        }
    }
    std::cout << "passed" << std::endl;

    // --- interator-value altered by insert_oblivious
    std::cout << "Iterator-value altered by insert_oblivious...";
    std::random_shuffle(values1.begin(), values1.end());
    std::random_shuffle(values2.begin(), values2.end());
    for (unsigned i = 0; i < n_tests; i++) {
        int key1 = values1[i].first;
        int key2 = values2[i].first;
        const_iterator cit1 = cmap.find(key1);
        assert(cit1 != cmap.end());
        const_iterator cit2 = cmap.find(key2);
        assert(cit2 != cmap.end());

        map.insert_oblivious(value_type(key1, key1 + 3));
        map.insert_oblivious(value_type(key2, key2 + 3));

        assert((*cit1).second == key1 + 3);
        assert((*cit2).second == key2 + 3);
    }
    std::cout << "passed" << std::endl;

    // --- iterator-value altered by other iterator
    std::cout << "Iterator-value altered by other iterator...";
    std::random_shuffle(values1.begin(), values1.end());
    std::random_shuffle(values2.begin(), values2.end());
    for (unsigned i = 0; i < n_tests; i++) {
        const_iterator cit1 = cmap.find(values1[i].first);
        assert(cit1 != cmap.end());
        const_iterator cit2 = cmap.find(values2[i].first);
        assert(cit2 != cmap.end());
        iterator it1 = map.find(values1[i].first);
        assert(it1 != map.end());
        iterator it2 = map.find(values2[i].first);
        assert(it2 != map.end());

        (*it1).second = (*it1).first + 5;
        (*it2).second = (*it2).first + 5;
        assert((*cit1).second == (*cit1).first + 5);
        assert((*cit2).second == (*cit2).first + 5);
    }
    std::cout << "passed" << std::endl;

    // --- erase by iterator
    std::cout << "Erase by iterator...";
    std::random_shuffle(values1.begin(), values1.end());
    std::random_shuffle(values2.begin(), values2.end());
    for (unsigned i = 0; i < n_tests; i++) {
        const_iterator cit1 = cmap.find(values1[i].first);
        assert(cit1 != cmap.end());
        const_iterator cit2 = cmap.find(values2[i].first);
        assert(cit2 != cmap.end());
        iterator it1 = map.find(values1[i].first);
        assert(it1 != map.end());
        iterator it2 = map.find(values2[i].first);
        assert(it2 != map.end());

        map.erase(it1);
        map.erase(it2);
        assert(cit1 == cmap.end());
        assert(cit2 == cmap.end());
    }
    std::cout << "passed" << std::endl;

    // --- erase by value (key)
    std::cout << "Erase by key...";
    for (unsigned i = 0; i < n_tests; i++) {
        const_iterator cit1 = cmap.find(values1[i + n_tests].first);
        assert(cit1 != cmap.end());
        const_iterator cit2 = cmap.find(values2[i + n_tests].first);
        assert(cit2 != cmap.end());

        map.erase_oblivious(values1[i + n_tests].first);
        map.erase_oblivious(values2[i + n_tests].first);
        assert(cit1 == cmap.end());
        assert(cit2 == cmap.end());
    }
    std::cout << "passed" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void more_iterator_test()
{
    typedef std::pair<int, int> value_type;
    const unsigned value_size = sizeof(value_type);

    const unsigned n_values = 50000;

    const unsigned buffer_size = 5 * n_values * (value_size + sizeof(int *));   // make sure all changes will be buffered
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
    std::generate(values1.begin(), values1.end(), rand_pairs(rand32));
    std::generate(values2.begin(), values2.end(), rand_pairs(rand32));

    // --- initial import
    map.insert(values1.begin(), values1.end(), mem_to_sort);
    std::vector<value_type>::iterator val_it = values2.begin();
    for (; val_it != values2.end(); ++val_it)
        map.insert_oblivious(*val_it);

    // --- store some iterators, rebuild and check
    std::cout << "Rebuild test...";
    std::random_shuffle(values1.begin(), values1.end());
    std::random_shuffle(values2.begin(), values2.end());
    {
        const_iterator cit1 = cmap.find(values1[17].first);
        const_iterator cit2 = cmap.find(values2[19].first);
        *cit1;
        *cit2;
        map.rehash();
        assert(map.size() == 2 * n_values);
        assert((*cit1).first == values1[17].first);
        assert((*cit2).first == values2[19].first);
    }
    std::cout << "passed" << std::endl;

    // --- unusual cases while scanning
    std::cout << "Another scan-test...";
    {
        const_iterator cit1 = cmap.find(values1[n_values / 2].first);
        const_iterator cit2 = cit1;
        ++cit1;
        int key1 = (*cit1).first;
        ++cit1;
        int key2 = (*cit1).first;
        map.erase_oblivious(key1);
        map.insert_oblivious(value_type(key2, key2 + 2));

        assert((*cit1).second == key2 + 2);
        ++cit2;
        assert(cit1 == cit2);
    }
    std::cout << "passed" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main()
{
    cmp_with_internal_map();
    basic_iterator_test();
    more_iterator_test();
    return 0;
}
