#ifndef LAZY_TEST_ALGORITHM
#define LAZY_TEST_ALGORITHM

#include <iterator>
#include <assert.h>

/*

 LAZY ALGORITHM CONCEPT

struct lazy_algorithm // stream algorithm
{
  typedef some_type result_type;
  
  convertible_to_result_type current();  // return current element of the stream
  bool next();            // go to next element, returns false if end of stream reached
  bool empty();           // return true if end of stream is reached
  
  // optional: do we need them?
  typedef some_type size_type;
  size_type left();
};

*/


//! \brief "Input iterator range to lazy algorithm" convertor
template <class InputIterator_>
class iterator_range_to_la 
{
  InputIterator_ current_,end_;
  
  //! \brief Default construction is forbidden
  iterator_range_to_la() {};
public:
  typedef typename std::iterator_traits<InputIterator_>::value_type result_type;
  typedef result_type return_type;
    
  iterator_range_to_la(InputIterator_ begin,InputIterator_ end): 
    current_(begin),end_(end) {}
  
  iterator_range_to_la(const iterator_range_to_la & a): current_(a.current_),end_(a.end_) {}
     
  return_type current()
  {
    assert(end_ != current_);
    return *current_;
  }
  bool next()
  {
    assert(end_ != current_);
    return ((++current_)!=end_);
  }
  bool empty()
  {
    return (current_==end_);
  }
};

template <class InputIterator_> 
iterator_range_to_la<InputIterator_> create_lazy_stream(InputIterator_ begin,InputIterator_ end)
{
  return iterator_range_to_la<InputIterator_>(begin,end);
}

/*

//! \brief "Lazy algorithm to output iterator range" convertor
template <class OutputIterator_, class LazyAlgorithm_>
class la_to_iterator_range
{
  OutputIterator_ begin_, end_;
  LazyAlgorithm_ & input_;
  
  //! \brief Default construction is forbidden
public:
  typedef void result_type;
  
  la_to_iterator_range(OutputIterator_ begin, OutputIterator_ end, LazyAlgorithm_ & input):
    begin_(begin),end_(end),input_(input) { }
  
  void current() { }
  void next()
  {
    assert(begin_ != end_);
    for(;begin_ != end_; begin_++)
    {
      assert(!input_.empty());
      *begin_ = input_.current();
      input_.next();
    }
  }
  void empty()
  {
    return (begin_ == end_);
  }
    
};


*/

// An easier convertor (flushes an lazy algorithm instance to)

template <class OutputIterator_,class LazyAlgorithm_>
OutputIterator_ flush_to_iterator(OutputIterator_ out,LazyAlgorithm_ & in)
{
  while(!in.empty())
  {
    *out++ = in.current();
    in.next();
  }
  return out;
}


template <class InputStream1_, class InputStream2_, class T>
T inner_product(InputStream1_ & stream1,InputStream2_ & stream2,T init)
{
  while(!stream1.empty())
  {
    assert(!stream2.empty());
    init+= stream1.current()*stream2.current();
    stream1.next();
    stream2.next();
  }
  return init;
}





























#endif
