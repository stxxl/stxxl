/***************************************************************************
 *            test_stream.cpp
 *
 *  Fri Oct 5  13:40:22 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "stxxl/stream"
#include "stxxl/vector"
#include <vector>

//! \example stream/test_stream.cpp
//! This is an example of how to use some basic algorithms from
//! stream package. The example sorts characters of a string producing
//! array of sorted tuples (character,index position).
//!

#define USE_FORMRUNS_N_MERGE // comment if you want to use one 'sort' algorithm
                             // without producing intermediate sorted runs.

#define USE_EXTERNAL_ARRAY // comment if you want to use internal vectors as
                           // input/output of the algorithm

#define block_size (8 * 1024)

typedef stxxl::tuple<char, int> tuple_type;

#ifdef USE_EXTERNAL_ARRAY
typedef stxxl::VECTOR_GENERATOR<char>::result input_array_type;
typedef stxxl::VECTOR_GENERATOR<tuple_type>::result output_array_type;
#else
typedef std::vector<char> input_array_type;
typedef std::vector<tuple_type> output_array_type;
#endif

using namespace stxxl;
using stxxl::stream::streamify;
using stxxl::stream::streamify_traits;
using stxxl::stream::make_tuple;
using stxxl::tuple;


const char * phrase = "Hasta la vista, baby";


template <class Container_, class It_>
void fill_input_array(Container_ & container, It_ p)
{
    while (*p)
    {
        container.push_back(*p);
        ++p;
    }
}

template <class ValTp>
struct counter
{
    typedef ValTp value_type;

    value_type cnt;
    counter() : cnt(0) { }

    value_type operator()  ()
    {
        value_type ret = cnt;
        ++cnt;
        return ret;
    }
};

typedef counter<int> counter_type;

struct cmp_type : std::binary_function<tuple_type, tuple_type, bool>
{
    typedef tuple_type value_type;
    bool operator ()  (const value_type & a, const value_type & b) const
    {
        return a.first < b.first;
    }

    value_type min_value() const
    {
        return value_type((std::numeric_limits < char > ::min)(), 0);
    }
    value_type max_value() const
    {
        return value_type((std::numeric_limits < char > ::max)(), 0);
    }
};

struct cmp_int : std::binary_function<int, int, bool>
{
    typedef int value_type;
    bool operator ()  (const value_type & a, const value_type & b) const
    {
        return a > b;
    }

    value_type max_value() const
    {
        return ((std::numeric_limits < value_type > ::min)());
    }
    value_type min_value() const
    {
        return ((std::numeric_limits < value_type > ::max)());
    }
};

int main()
{
    input_array_type input;
    output_array_type output;

    stxxl::stats * s = stxxl::stats::get_instance();

    std::cout << *s;

    fill_input_array(input, phrase);

    output.resize(input.size());

    // HERE streaming part begins (streamifying)
    // create input stream
#ifdef BOOST_MSVC
    typedef streamify_traits<input_array_type::iterator>::stream_type input_stream_type;
#else
    typedef typeof(streamify(input.begin(), input.end())) input_stream_type;
#endif

    input_stream_type input_stream = streamify(input.begin(), input.end());


    // create counter stream
#ifdef BOOST_MSVC
    typedef stxxl::stream::generator2stream<counter_type> counter_stream_type;
#else
    typedef __typeof (streamify(counter_type())) counter_stream_type;
#endif
    counter_stream_type counter_stream = streamify(counter_type());

    // create tuple stream
    typedef make_tuple<input_stream_type, counter_stream_type> tuple1_stream_type;
    tuple1_stream_type tuples1_stream(input_stream, counter_stream);

#ifdef USE_FORMRUNS_N_MERGE
    // sort tuples by character
    // 1. form runs
    typedef stream::runs_creator<tuple1_stream_type, cmp_type, block_size> run_creator_type;
    run_creator_type runscreator(tuples1_stream, cmp_type(), 128 * 1024);
    // 2. merge runs
    typedef stream::runs_merger<run_creator_type::sorted_runs_type, cmp_type> runs_merger_type;
    runs_merger_type sorted_stream(runscreator.result(), cmp_type(), 128 * 1024);
#else
    // sort tuples by character
    // (combination of the previous two steps in one algorithm: form runs and merge)
    typedef stream::sort<tuple1_stream_type, cmp_type, block_size> sorted_stream_type;
    sorted_stream_type sorted_stream(tuples1_stream, cmp_type(), 128 * 1024);
#endif

    // HERE streaming part ends (materializing)
    output_array_type::iterator o = materialize(sorted_stream, output.begin(), output.end());
    // or materialize(sorted_stream,output.begin());
    assert(o == output.end() );


    STXXL_MSG("input string (character,position) :");
    for (unsigned i = 0; i < input.size(); ++i)
    {
        STXXL_MSG("('" << input[i] << "'," << i << ")");
    }
    STXXL_MSG("sorted tuples (character,position):");
    for (unsigned i = 0; i < input.size(); ++i)
    {
        STXXL_MSG("('" << output[i].first << "'," << output[i].second << ")");
    }

    std::cout << *s;

    std::vector<int> InternalArray(1024 * 1024);
    std::sort(InternalArray.begin(), InternalArray.end(), cmp_int());
    stxxl::sort < 1024 * 1024 > (                                       InternalArray.begin(),
                                                                        InternalArray.end(),
                                                                        cmp_int(),
                                                                        1024 * 1024 * 10,
                                                                        stxxl::RC());
}
