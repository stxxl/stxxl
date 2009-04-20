/***************************************************************************
 *  containers/test_iterators.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2007 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <vector>
#include <stxxl.h>


template <typename T>
struct modify
{
    void operator () (T & obj) const
    {
        ++obj;
    }
};

template <typename Iterator>
bool test_inc_dec(Iterator it)
{
    Iterator i = it;

    ++i;
    i++;
    --i;
    i--;

    return it == i;
}

template <typename Iterator>
bool test_inc_dec_random(Iterator it)
{
    Iterator i = it;

    ++i;
    i = i + 2;
    i++;
    i += 3;
    --i;
    i = i - 3;
    i--;
    i -= 2;

    return it == i;
}

template <typename svt>
void test(svt & sv)
{
    typedef const svt csvt;
    typedef typename svt::value_type value_type;

    sv[0] = 108;

    typename svt::iterator svi = sv.begin();
    modify<value_type>() (*svi);

    typename svt::const_iterator svci = sv.begin();
    //modify<value_type>()(*svci);      // read-only

    typename csvt::iterator xsvi = sv.begin();
    modify<value_type>() (*xsvi);

    // test assignment
    svci = xsvi;
    //xsvi = svci; // not allowed

    typename csvt::const_iterator xsvci = sv.begin();
    //modify<value_type>()(*xsvci);     // read-only

    // test comparison between const and non-const iterators
    svci == xsvi;
    xsvi == svci;
    svci != xsvi;
    xsvi != svci;

    // test increment/decrement
    test_inc_dec(svi);
    test_inc_dec(svci);
    test_inc_dec(xsvi);
    test_inc_dec(xsvci);

///////////////////////////////////////////////////////////////////////////

    csvt & csv = sv;
    //csv[0] = 108; // read-only

    //typename csvt::iterator csvi = csv.begin();    // read-only
    //modify<value_type>()(*csvi);      // read-only

    typename csvt::const_iterator csvci = csv.begin();
    //modify<value_type>()(*csvci);     // read-only

    //typename svt::iterator xcsvi = csv.begin();    // read-only
    //modify<value_type>()(*xcsvi);     // read-only

    typename svt::const_iterator xcsvci = csv.begin();
    //modify<value_type>()(*csvci);     // read-only

    // test increment/decrement
    test_inc_dec(csvci);
    test_inc_dec(xsvci);
}

template <typename svt>
void test_random_access(svt & sv)
{
    typename svt::const_iterator svci = sv.begin();
    typename svt::iterator xsvi = sv.begin();

    // test subtraction of const and non-const iterators
    svci - xsvi;
    xsvi - svci;

    // bracket operators
    svci[0];
    xsvi[0];
    //svci[0] = 1; // read-only
    xsvi[0] = 1;

    // test +, -, +=, -=
    test_inc_dec_random(svci);
    test_inc_dec_random(xsvi);
}


typedef unsigned int key_type;
typedef unsigned int data_type;

struct cmp : public std::less<key_type>
{
    static key_type min_value()
    {
        return (std::numeric_limits<key_type>::min)();
    }
    static key_type max_value()
    {
        return (std::numeric_limits<key_type>::max)();
    }
};


template <>
struct modify<std::pair<const key_type, data_type> >
{
    void operator () (std::pair<const key_type, data_type> & obj) const
    {
        ++(obj.second);
    }
};

int main()
{
    std::vector<int> V(8);
    test(V);
    test_random_access(V);

    stxxl::vector<int> Vector(8);
    test(Vector);
    test_random_access(Vector);

#if ! defined(__GNUG__) || ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 30400)
    typedef stxxl::map<key_type, data_type, cmp, 4096, 4096> map_type;
    map_type Map(4096 * 10, 4096 * 10);
    Map[4] = 8;
    Map[15] = 16;
    Map[23] = 42;
    test(Map);
#endif

    stxxl::deque<int> Deque(1);
    test(Deque);
    test_random_access(Deque);

    return 0;
}

// vim: et:ts=4:sw=4
