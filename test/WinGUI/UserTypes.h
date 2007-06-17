#ifndef INCLUDE_USER_TYPES
#define INCLUDE_USER_TYPES

#include <stxxl/all>
#include <limits>

struct type1
{
    int key;
    int data[3];
    bool operator == (const type1 & a) const
    {
        return key == a.key;
    }
    type1() { }
    type1(int key_) : key(key_) { }
};
struct type2
{
    int key;
    int data[7];
    bool operator == (const type2 & a) const
    {
        return key == a.key;
    }
    type2() { }
    type2(int key_) : key(key_) { }
};

struct my_rnd
{
    stxxl::random_number32 rnd;
    int operator()  () const
    {
        return rnd() % 0xfffff;
    }
};

template <class T>
struct LessCmp
{
    bool operator ()  (const T & a, const T & b) const
    {
        return a.key < b.key;
    }
    static int min_value()
    {
        return (std::numeric_limits < int > ::min)();
    }
    static int max_value()
    {
        return (std::numeric_limits < int > ::max)();
    }
};

template <class T>
struct GreaterCmp
{
    bool operator ()  (const T & a, const T & b) const
    {
        return a.key > b.key;
    }
    static int min_value()
    {
        return (std::numeric_limits < int > ::max)();
    }
    static int max_value()
    {
        return (std::numeric_limits < int > ::min)();
    }
};

#endif // INCLUDE_USER_TYPES