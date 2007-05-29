#ifndef STXXL_RANDOM_SHUFFLE_HEADER
#define STXXL_RANDOM_SHUFFLE_HEADER

/***************************************************************************
 *            random_shuffle.h
 *
 *  Tue May  29 14:48:04 2007
 *  Copyright  2007 Manuel Krings, Markus Westphal
 *  
 * 
 ****************************************************************************/

// TODO: improve main memory consumption in recursion
//        (free stacks buffers)


#include "../stream/stream.h"
#include "../algo/scan.h"
#include "../containers/stack.h"

__STXXL_BEGIN_NAMESPACE

namespace random_shuffle_local
{
  // efficiently writes data into an stxxl::vector with overlapping of I/O and
  // computation
  template <class ExtIterator>
  class write_vector
  {
    write_vector(); // forbidden
    typedef typename ExtIterator::size_type size_type;
    typedef typename ExtIterator::value_type value_type;
    typedef typename ExtIterator::block_type block_type;
    typedef typename ExtIterator::const_iterator ConstExtIterator;
    typedef stxxl::buf_ostream<block_type,typename ExtIterator::bids_container_iterator> buf_ostream_type;
    
    ExtIterator it;
    unsigned_type nbuffers;
    buf_ostream_type * outstream;
  public:
    write_vector( ExtIterator begin,
        unsigned nbuffers_=2 // buffers to use for overlapping (>=2 recommended)
      ): it(begin),nbuffers(nbuffers_)
    {
      outstream = new buf_ostream_type(it.bid(),nbuffers);
    }
    
    value_type & operator * ()
    {
      if(it.block_offset() ==0 ) it.touch(); // tells the vector that the block was modified
      return **outstream;
      
    }
    
    write_vector & operator ++()
    {
      ++it;
      ++(*outstream);
      return *this;
    }
    
    void flush()
    {
      ConstExtIterator const_out = it;
    
      while(const_out.block_offset())
      {
        **outstream =  * const_out; // might cause I/Os for loading the page that
        ++const_out;                     // contains data beyond out
        ++(*outstream);
      }
      
      it.flush();
      delete outstream;
      outstream = NULL;
    }
    
    virtual ~write_vector()
    {
      if(outstream) flush();
    }
  };
  
};


//! \addtogroup stlalgo
//! \{


template < typename Tp_, typename AllocStrategy_, typename SzTp_,typename DiffTp_,
    unsigned BlockSize_, typename PgTp_, unsigned PageSize_, typename RandomNumberGenerator_>  
void random_shuffle(    stxxl::vector_iterator<Tp_,AllocStrategy_,SzTp_,DiffTp_,BlockSize_,PgTp_,PageSize_> first, 
                        stxxl::vector_iterator<Tp_,AllocStrategy_,SzTp_,DiffTp_,BlockSize_,PgTp_,PageSize_> beyond,
                        RandomNumberGenerator_ & rand, 
                        unsigned_type M);


//! \brief External equivalent of std::random_shuffle
//! \param first begin of the range to shuffle
//! \param beyond end of the range to shuffle
//! \param rand random number generator object (functor) 
//! \param M number of bytes for internal use
//! \param RC paralel disk allocation strategy 
//!
//! - BlockSize_ size of the block to use for external memory data structures
//! - PageSize_ page size in blocks to use for external memory data structures
template <  typename ExtIterator_,
            typename RandomNumberGenerator_,
            unsigned BlockSize_,
            unsigned PageSize_,
            typename AllocStrategy_>
void random_shuffle(   ExtIterator_ first, 
                        ExtIterator_ beyond,
                        RandomNumberGenerator_ & rand, 
                        unsigned_type M,
                        AllocStrategy_ AS = stxxl::RC())
{
  typedef typename ExtIterator_::value_type value_type;
  typedef typename stxxl::STACK_GENERATOR<value_type, stxxl::external, 
    stxxl::grow_shrink2, PageSize_, 
    BlockSize_,void,0,AllocStrategy_>::result stack_type;
  typedef typename stack_type::block_type block_type;
  
  STXXL_VERBOSE1("random_shuffle: Plain Version")
  
  stxxl::int64 n = beyond - first; // the number of input elements
  
  // make sure we have at least 6 blocks + 1 page
  if(M < 6*BlockSize_+PageSize_*BlockSize_)
    M = 6*BlockSize_+PageSize_*BlockSize_;
  
  int_type k = M/(3*BlockSize_); // number of buckets
 
  
  stxxl::int64 i,j,size = 0;
  
  value_type * temp_array;
  typedef typename stxxl::VECTOR_GENERATOR<value_type, 
    PageSize_, 4, BlockSize_, AllocStrategy_>::result temp_vector_type;
  temp_vector_type * temp_vector;

  stxxl::prefetch_pool<block_type> p_pool(0); // no read buffers
  STXXL_VERBOSE1("random_shuffle: "<<M/BlockSize_-k<<" write buffers for "<<k<<" buckets")
  stxxl::write_pool<block_type> w_pool(M/BlockSize_-k); // M/B-k write buffers

  stack_type **buckets; 
    
  // create and put buckets into container
  buckets = new stack_type * [k];
  for (j = 0; j < k; j++)
    buckets[j] = new stack_type(p_pool, w_pool, 0);
  
  ///// Reading input /////////////////////
  typedef typename stream::streamify_traits<ExtIterator_>::stream_type input_stream;
  input_stream in = stxxl::stream::streamify(first,beyond); 
  
  // distribute input into random buckets
  int_type random_bucket = 0;
  for(i=0; i<n; ++i){     
    random_bucket = rand(k);
    buckets[random_bucket]->push(*in); // reading the current input element
    ++in; // go to the next input element
  }
  
  ///// Processing //////////////////////
  // resize buffers
  w_pool.resize(0);
  p_pool.resize(PageSize_);
  
  // Set prefetch aggr to PageSize_
  for (int_type j=0; j<k; j++){
    buckets[j]->set_prefetch_aggr(PageSize_);
  }
  
  unsigned_type space_left = M - k*BlockSize_ - 
    PageSize_*BlockSize_;// remaining int space  
  ExtIterator_ Writer = first;
  ExtIterator_ it = first;
  
  for (i=0;i<k;i++){
    STXXL_VERBOSE1("random_shuffle: bucket no " << i << " contains " << buckets[i]->size() << " elements");
  }

  // shuffle each bucket
  for (i=0;i<k;i++){
    size = buckets[i]->size();
  
    // does the bucket fit into memory?   
    if (size * sizeof(value_type) < space_left){ 
      STXXL_VERBOSE1("random_shuffle: no recursion");
  
      // copy bucket into temp. array   
      temp_array = new value_type[size];
      for (j=0; j<size; j++){
        temp_array[j] = buckets[i]->top();
        buckets[i]->pop();
      }
      
      // shuffle
      std::random_shuffle(temp_array, temp_array+size,rand);
      
      // write back
      for (j=0; j<size; j++){
        *Writer = temp_array[j];
        ++Writer;
      }
      
      // free memory
      delete[] temp_array;
    }
    else {
      STXXL_VERBOSE1("random_shuffle: recursion");
      
      // copy bucket into temp. stxxl::vector 
      temp_vector  = new temp_vector_type(size);
      
      for (j = 0; j < size; j++){
        (*temp_vector)[j]=buckets[i]->top();
        buckets[i]->pop();
      }
      
      p_pool.resize(0);
      space_left += PageSize_*BlockSize_;
      STXXL_VERBOSE1("random_shuffle: Space left: " << space_left);
      
      // recursive shuffle
      stxxl::random_shuffle(temp_vector->begin(), 
           temp_vector->end(), rand, space_left);
      
      p_pool.resize(PageSize_);
      
      // write back
      for (j=0; j<size; j++){
        *Writer = (*temp_vector)[j];
        ++Writer;
      }
      
      // free memory
      delete temp_vector;
    }
  }

  // free buckets
  for (int j = 0; j < k; j++)
    delete buckets[j];
  delete [] buckets;

}

//! \brief External equivalent of std::random_shuffle (specialization for stxxl::vector)
//! \param first begin of the range to shuffle
//! \param beyond end of the range to shuffle
//! \param rand random number generator object (functor) 
//! \param M number of bytes for internal use
template < typename Tp_, typename AllocStrategy_, typename SzTp_,typename DiffTp_,
    unsigned BlockSize_, typename PgTp_, unsigned PageSize_, typename RandomNumberGenerator_>  
void random_shuffle(    stxxl::vector_iterator<Tp_,AllocStrategy_,SzTp_,DiffTp_,BlockSize_,PgTp_,PageSize_> first, 
                        stxxl::vector_iterator<Tp_,AllocStrategy_,SzTp_,DiffTp_,BlockSize_,PgTp_,PageSize_> beyond,
                        RandomNumberGenerator_ & rand, 
                        unsigned_type M)
{
  typedef stxxl::vector_iterator<Tp_,AllocStrategy_,SzTp_,DiffTp_,BlockSize_,PgTp_,PageSize_> ExtIterator_; 
  typedef typename ExtIterator_::value_type value_type;
  typedef typename stxxl::STACK_GENERATOR<value_type, stxxl::external, 
    stxxl::grow_shrink2, PageSize_, BlockSize_>::result stack_type;
  typedef typename stack_type::block_type block_type;
  
  STXXL_VERBOSE1("random_shuffle: Vector Version")
  
  // make sure we have at least 6 blocks + 1 page
  if(M < 6*BlockSize_+PageSize_*BlockSize_)
    M = 6*BlockSize_+PageSize_*BlockSize_;
  
  stxxl::int64 n = beyond - first; // the number of input elements
  int_type k = M/(3*BlockSize_); // number of buckets
  
  stxxl::int64 i,j,size = 0;
  
  value_type * temp_array;
  typedef typename stxxl::VECTOR_GENERATOR<value_type, 
    PageSize_, 4, BlockSize_, AllocStrategy_>::result temp_vector_type;
  temp_vector_type * temp_vector;

  stxxl::prefetch_pool<block_type> p_pool(0); // no read buffers
  stxxl::write_pool<block_type> w_pool(M/BlockSize_-k); // M/B-k write buffers

  stack_type **buckets;
    
  // create and put buckets into container
  buckets = new stack_type * [k];
  for (j = 0; j < k; j++)
    buckets[j] = new stack_type(p_pool, w_pool, 0);
  
  ///// Reading input /////////////////////
  typedef typename stream::streamify_traits<ExtIterator_>::stream_type input_stream;
  input_stream in = stxxl::stream::streamify(first,beyond); 
  
  // distribute input into random buckets
  int_type random_bucket = 0;
  for(i=0; i<n; ++i){     
    random_bucket = rand(k);
    buckets[random_bucket]->push(*in); // reading the current input element
    ++in; // go to the next input element
  }
  
  ///// Processing //////////////////////
  // resize buffers
  w_pool.resize(0);
  p_pool.resize(PageSize_);
  
  // Set prefetch aggr to PageSize_
  for (int_type j=0; j<k; j++){
    buckets[j]->set_prefetch_aggr(PageSize_);
  }
  
  unsigned_type space_left = M - k*BlockSize_ - 
    PageSize_*BlockSize_;// remaining int space  
  random_shuffle_local::write_vector<ExtIterator_> 
    Writer(first,2*PageSize_);
  ExtIterator_ it = first;
  
  for (i=0;i<k;i++){
    STXXL_VERBOSE1("random_shuffle: bucket no " << i << " contains " << buckets[i]->size() << " elements");
  }

  // shuffle each bucket
  for (i=0;i<k;i++){
    size = buckets[i]->size();
  
    // does the bucket fit into memory?   
    if (size * sizeof(value_type) < space_left){ 
      STXXL_VERBOSE1("random_shuffle: no recursion");
  
      // copy bucket into temp. array   
      temp_array = new value_type[size];
      for (j=0; j<size; j++){
        temp_array[j] = buckets[i]->top();
        buckets[i]->pop();
      }
      
      // shuffle
      std::random_shuffle(temp_array, temp_array+size,rand);
      
      // write back
      for (j=0; j<size; j++){
        *Writer = temp_array[j];
        ++Writer;
      }
      
      // free memory
      delete[] temp_array;
    }
    else {
      STXXL_VERBOSE1("random_shuffle: recursion");
      // copy bucket into temp. stxxl::vector 
      temp_vector  = new temp_vector_type(size);
      
      for (j = 0; j < size; j++){
        (*temp_vector)[j]=buckets[i]->top();
        buckets[i]->pop();
      }
      
      p_pool.resize(0);
      space_left += PageSize_*BlockSize_;
      
      STXXL_VERBOSE1("random_shuffle: Space left: " << space_left);
      
      // recursive shuffle
      stxxl::random_shuffle(temp_vector->begin(), 
           temp_vector->end(), rand, space_left);
      
      p_pool.resize(PageSize_);
      
      // write back
      for (j=0; j<size; j++){
        *Writer = (*temp_vector)[j];
        ++Writer;
      }
      
      // free memory
      delete temp_vector;
    }
  }

  // free buckets
  for (int j = 0; j < k; j++)
    delete buckets[j];
  delete [] buckets;

  Writer.flush(); // flush buffers
}

//! \brief External equivalent of std::random_shuffle (specialization for stxxl::vector)
//! \param first begin of the range to shuffle
//! \param beyond end of the range to shuffle 
//! \param M number of bytes for internal use
template < typename Tp_, typename AllocStrategy_, typename SzTp_,typename DiffTp_,
    unsigned BlockSize_, typename PgTp_, unsigned PageSize_>  
void random_shuffle(    stxxl::vector_iterator<Tp_,AllocStrategy_,SzTp_,DiffTp_,BlockSize_,PgTp_,PageSize_> first, 
                        stxxl::vector_iterator<Tp_,AllocStrategy_,SzTp_,DiffTp_,BlockSize_,PgTp_,PageSize_> beyond, 
                        unsigned_type M)
{
  stxxl::random_number<> rand;
  stxxl::random_shuffle(first,beyond,rand,M);
}                        


//! \}

__STXXL_END_NAMESPACE
 
#endif
