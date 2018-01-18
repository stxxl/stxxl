/***************************************************************************
 *  tests/containers/test_iterators.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2007 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <cstring>
#include <deque>
#include <map>
#include <vector>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <foxxll/common/error_handling.hpp>

#include <stxxl.h>

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)

template <typename T>
const char * _()
{
    const char* start = strchr(FOXXLL_PRETTY_FUNCTION_NAME, '[');
    if (start == nullptr)
        return "unknown";
    else
        return start;
}

template <typename I>
void dump_iterator_info(I&)
{
    LOG1 << FOXXLL_PRETTY_FUNCTION_NAME;
    LOG1 << "  category:        " << _<typename std::iterator_traits<I>::iterator_category>();
    LOG1 << "  value_type:      " << _<typename std::iterator_traits<I>::value_type>();
    LOG1 << "  difference_type: " << _<typename std::iterator_traits<I>::difference_type>();
    LOG1 << "  pointer:         " << _<typename std::iterator_traits<I>::pointer>();
    LOG1 << "  reference:       " << _<typename std::iterator_traits<I>::reference>();
}

template <typename C>
void dump_container_info(C&)
{
    LOG1 << FOXXLL_PRETTY_FUNCTION_NAME;
    LOG1 << "  value_type:      " << _<typename C::value_type>();
    LOG1 << "  size_type:       " << _<typename C::size_type>();
    LOG1 << "  difference_type: " << _<typename C::difference_type>();
    LOG1 << "  pointer:         " << _<typename C::pointer>();
    LOG1 << "  const_pointer:   " << _<typename C::const_pointer>();
    LOG1 << "  reference:       " << _<typename C::reference>();
    LOG1 << "  const_reference: " << _<typename C::const_reference>();
    LOG1 << "  iterator:        " << _<typename C::iterator>();
    LOG1 << "  const_iterator:  " << _<typename C::const_iterator>();
}

template <typename T>
struct modify
{
    void operator () (T& obj) const
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

    die_unless(it == i);
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

    die_unless(it == i);
    return it == i;
}

template <typename IteratorA, typename IteratorB, typename Category>
struct test_comparison_lt_gt
{
    void operator () (IteratorA, IteratorB)
    {
        // operators <, <=, >=, > are not available in all iterator categories
    }
};

template <typename IteratorA, typename IteratorB>
struct test_comparison_lt_gt<IteratorA, IteratorB, std::random_access_iterator_tag>
{
    void operator () (IteratorA a, IteratorB b)
    {
        die_unless(! (a < b));
        die_unless((a <= b));
        die_unless(! (a > b));
        die_unless((a >= b));

        die_unless(! (b < a));
        die_unless((b <= a));
        die_unless(! (b > a));
        die_unless((b >= a));
    }
};

template <typename IteratorA, typename IteratorB>
void test_comparison(IteratorA a, IteratorB b)
{
    die_unless((a == b));
    die_unless(! (a != b));

    die_unless((b == a));
    die_unless(! (b != a));

    test_comparison_lt_gt<IteratorA, IteratorB, typename std::iterator_traits<IteratorA>::iterator_category>()(a, b);
}

template <typename Iterator>
void test_operators(Iterator it)
{
    *it;
    it.operator -> ();
    test_comparison(it, it);
}

template <typename svt>
void test(svt& sv)
{
    dump_container_info(sv);

    using csvt = const svt;
    using value_type = typename svt::value_type;

    sv[0] = 108;

    typename svt::iterator svi = sv.begin();
    dump_iterator_info(svi);
    modify<value_type>()(*svi);

    typename svt::const_iterator svci = sv.begin();
    dump_iterator_info(svci);
    //modify<value_type>()(*svci);      // read-only

    typename csvt::iterator xsvi = sv.begin();
    modify<value_type>()(*xsvi);

    // test assignment
    svci = xsvi;
    //xsvi = svci; // not allowed

    typename csvt::const_iterator xsvci = sv.begin();
    //modify<value_type>()(*xsvci);     // read-only

    // test comparison between const and non-const iterators
    test_comparison(svci, xsvi);

    // test increment/decrement
    test_inc_dec(svi);
    test_inc_dec(svci);
    test_inc_dec(xsvi);
    test_inc_dec(xsvci);

    // test operators
    test_operators(svi);
    test_operators(svci);
    test_operators(xsvi);
    test_operators(xsvci);

    // test forward iteration
    for (typename svt::iterator i = sv.begin(); i != sv.end(); ++i) ;

///////////////////////////////////////////////////////////////////////////

    csvt& csv = sv;
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
    test_inc_dec(xcsvci);

    // test operators
    test_operators(csvci);
    test_operators(xcsvci);

    // test forward iteration
    for (typename svt::const_iterator ci = sv.begin(); ci != sv.end(); ++ci) ;
}

template <typename svt>
void test_reverse(svt& sv)
{
    dump_container_info(sv);

    using csvt = const svt;
    using value_type = typename svt::value_type;

    sv[0] = 108;

    typename svt::reverse_iterator svi = sv.rbegin();
    dump_iterator_info(svi);
    modify<value_type>()(*svi);

    typename svt::const_reverse_iterator svci = sv.rbegin();
    dump_iterator_info(svci);
    //modify<value_type>()(*svci);      // read-only

    typename csvt::reverse_iterator xsvi = sv.rbegin();
    modify<value_type>()(*xsvi);

    // test assignment
    svci = xsvi;
    //xsvi = svci; // not allowed

    typename csvt::const_reverse_iterator xsvci = sv.rbegin();
    //modify<value_type>()(*xsvci);     // read-only

#if !defined(__GNUG__) || (GCC_VERSION >= 40100)
    // test comparison between const and non-const iterators
    test_comparison(svci, xsvi);
#endif

    // test increment/decrement
    test_inc_dec(svi);
    test_inc_dec(svci);
    test_inc_dec(xsvi);
    test_inc_dec(xsvci);

    // test operators
    test_operators(svi);
    test_operators(svci);
    test_operators(xsvi);
    test_operators(xsvci);

    // test forward iteration
    for (typename svt::reverse_iterator i = sv.rbegin(); i != sv.rend(); ++i) ;

///////////////////////////////////////////////////////////////////////////

    csvt& csv = sv;
    //csv[0] = 108; // read-only

    //typename csvt::reverse_iterator csvi = csv.rbegin();    // read-only
    //modify<value_type>()(*csvi);      // read-only

    typename csvt::const_reverse_iterator csvci = csv.rbegin();
    //modify<value_type>()(*csvci);     // read-only

    //typename svt::reverse_iterator xcsvi = csv.rbegin();    // read-only
    //modify<value_type>()(*xcsvi);     // read-only

    typename svt::const_reverse_iterator xcsvci = csv.rbegin();
    //modify<value_type>()(*csvci);     // read-only

    // test increment/decrement
    test_inc_dec(csvci);
    test_inc_dec(xcsvci);

    // test operators
    test_operators(csvci);
    test_operators(xcsvci);

    // test forward iteration
#if !defined(__GNUG__) || (GCC_VERSION >= 40100)
    for (typename svt::const_reverse_iterator ci = sv.rbegin(); ci != sv.rend(); ++ci) ;
#else
    for (typename svt::const_reverse_iterator ci = sv.rbegin(); ci != typename svt::const_reverse_iterator(sv.rend()); ++ci) ;
#endif
}

template <typename svt>
void test_random_access(svt& sv)
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

template <typename svt>
void test_random_access_reverse(svt& sv)
{
    typename svt::const_reverse_iterator svcri = sv.rbegin();
    typename svt::reverse_iterator xsvri = sv.rbegin();

#if !defined(__GNUG__) || (GCC_VERSION >= 40100)
    // test subtraction of const and non-const iterators
    svcri - xsvri;
    xsvri - svcri;
#endif

    // bracket operators
    svcri[0];
    //svcri[0] = 1; // read-only
#ifndef _LIBCPP_VERSION
    // see note about problem with std::reverse_iterator::operator[] in
    // vector_iterator::operator [], in effect:
    // vector_reverse_iterator::operator[] does not work with libc++.
    xsvri[0];
    xsvri[0] = 1;
#endif

    // test +, -, +=, -=
    test_inc_dec_random(svcri);
    test_inc_dec_random(xsvri);
}

using key_type = float;
using data_type = double;

struct cmp : public std::less<key_type>
{
    static key_type min_value()
    {
        return std::numeric_limits<key_type>::min();
    }
    static key_type max_value()
    {
        return std::numeric_limits<key_type>::max();
    }
};

template <>
struct modify<std::pair<const key_type, data_type> >
{
    void operator () (std::pair<const key_type, data_type>& obj) const
    {
        ++(obj.second);
    }
};

int main()
{
    std::vector<double> V(8);
    test(V);
    test_reverse(V);
    test_random_access(V);
    test_random_access_reverse(V);

    stxxl::vector<double> Vector(8);
    test(Vector);
    test_reverse(Vector);
    test_random_access(Vector);
    test_random_access_reverse(Vector);

    std::map<key_type, data_type, cmp> M;
    M[4] = 8;
    M[15] = 16;
    M[23] = 42;
    test(M);
    test_reverse(M);

#if !defined(__GNUG__) || (GCC_VERSION >= 30400)
    using map_type = stxxl::map<key_type, data_type, cmp, 4096, 4096>;
    map_type Map(4096 * 10, 4096 * 10);
    Map[4] = 8;
    Map[15] = 16;
    Map[23] = 42;
    test(Map);
    test_reverse(Map);
#endif

    std::deque<double> D(8);
    test(D);
    test_reverse(D);
    test_random_access(D);
    test_random_access_reverse(D);

    stxxl::deque<double> Deque(8);
    test(Deque);
    test_reverse(Deque);
    test_random_access(Deque);
    test_random_access_reverse(Deque);

    return 0;
}

// vim: et:ts=4:sw=4
