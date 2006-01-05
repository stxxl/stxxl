#ifndef SORT_STREAM_HEADER
#define SORT_STREAM_HEADER

/***************************************************************************
 *            sort_stream.h
 *
 *  Thu Oct  2 12:09:50 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#include "../common/utils.h"
#include "../algo/sort.h"
#include "../mng/buf_istream.h"
#include "../mng/buf_ostream.h"

__STXXL_BEGIN_NAMESPACE

/* depricated
template < unsigned blk_sz,class Y,class X > 
  struct bid_iterator_traits< 
    __gnu_cxx::__normal_iterator<  sort_local::trigger_entry<BID<blk_sz>,Y> * ,  X> >
	{
		enum
		{
			block_size = blk_sz
		};	
	};
*/

namespace stream
{
  //! \addtogroup streampack Stream package
  //! \{
 
  template <class ValueType,class TriggerEntryType>
  struct sorted_runs
  {
    typedef TriggerEntryType trigger_entry_type;
    typedef ValueType value_type;
    typedef typename trigger_entry_type::bid_type bid_type;
    typedef stxxl::int64 size_type;
    typedef std::vector<trigger_entry_type> run_type;
    typedef typed_block<bid_type::size,value_type> block_type;
    size_type elements;
    std::vector<run_type> runs;
	std::vector<unsigned> runs_sizes;

	 // Optimization:
    // if the input is small such that its total size is	  
	// less than B (block_type::size)
	// then input is sorted internally
	// and kept in the array "small"
	std::vector<ValueType> small_;
	  
    sorted_runs():elements(0) {}
		
	//! \brief Deallocates the blocks which the runs occupy
	//!
	//! \remark Usually there is no need in calling this method,
	//! since the \c runs_merger calls it when it is being destructed
	void deallocate_blocks()
	{
		block_manager * bm = block_manager::get_instance();
		 for(unsigned i=0; i < runs.size();++i)
          bm->delete_blocks(
          trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(runs[i].begin()),
          trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(runs[i].end()) );
		 
		 runs.clear();
	}
  };
  
  //! \brief Forms sorted runs of data from a stream
  //!
  //! Template parameters:
  //! - \c Input_ type of the input stream
  //! - \c Cmp_ type of omparison object used for sorting the runs
  //! - \c BlockSize_ size of blocks used to store the runs
  //! - \c AllocStr_ functor that defines allocation strategy for the runs
  template <  
  			  class Input_,
              class Cmp_,
              unsigned BlockSize_ = STXXL_DEFAULT_BLOCK_SIZE(typename Input_::value_type),
              class AllocStr_ = STXXL_DEFAULT_ALLOC_STRATEGY>
  class runs_creator
  {
    Input_ & input;
    Cmp_ cmp;
  public:
	typedef Cmp_ cmp_type;
	typedef typename Input_::value_type value_type;
    typedef BID<BlockSize_> bid_type;
    typedef typed_block<BlockSize_,value_type> block_type;
    typedef sort_local::trigger_entry<bid_type,value_type> trigger_entry_type;
    typedef sorted_runs<value_type,trigger_entry_type> sorted_runs_type;
  private:
    typedef typename sorted_runs_type::run_type run_type;
    sorted_runs_type result_; // stores the result (sorted runs)
    unsigned m_; // memory for internal use in blocks
    bool result_computed; // true result is already computed (used in 'result' method)
    
    runs_creator();// default construction is forbidden
    runs_creator(const runs_creator & );// copy construction is forbidden
  	runs_creator & operator = (const runs_creator &); // copying is forbidden
    
    
    void compute_result();
    void sort_run(block_type * run,unsigned elements)
    {
      if(block_type::has_filler)
        std::sort(
                  TwoToOneDimArrayRowAdaptor< 
                    block_type,
                    value_type,
                    block_type::size > (run,0 ),
                  TwoToOneDimArrayRowAdaptor< 
                    block_type,
                    value_type,
                    block_type::size > (run, 
                      elements )
                  ,cmp);
      else 
        std::sort(run[0].elem, run[0].elem + elements, cmp);
    }
  public:
    //! \brief Creates the object
    //! \param i input stream
    //! \param c comparator object
    //! \param memory_to_use memory amount that is allowed to used by the sorter in bytes
    runs_creator(Input_ & i,Cmp_ c,unsigned memory_to_use):
      input(i),cmp(c),m_(memory_to_use/BlockSize_),result_computed(false)
    {
		assert (2*BlockSize_ <= memory_to_use);
    }
    
    //! \brief Returns the sorted runs object
    //! \return Sorted runs object. The result is computed lazily, i.e. on the first call
    //! \remark Returned object is intended to be used by \c runs_merger object as input
    const sorted_runs_type & result()
    {
      if(!result_computed)
      {
        compute_result();
        result_computed = true;
		#ifdef STXXL_PRINT_STAT_AFTER_RF
		 STXXL_MSG(*stats::get_instance())
		#endif
      }
      return result_;
    }
  };
  
 
  template <class Input_,class Cmp_,unsigned BlockSize_,class AllocStr_>
  void runs_creator<Input_,Cmp_,BlockSize_,AllocStr_>::compute_result()
  {
    unsigned i = 0;
    unsigned m2 = m_ / 2;
    const unsigned el_in_run = m2* block_type::size; // # el in a run
	STXXL_VERBOSE1("runs_creator::compute_result m2="<<m2)
	unsigned pos = 0;	
	  
#ifndef STXXL_SMALL_INPUT_PSORT_OPT
    block_type * Blocks1 = new block_type[m2*2];
#else
	/*
	block_type * Blocks1 = new block_type[1];       // allocate only one block first
	                                                                        // if needed reallocate
	while(!input.empty() && pos != block_type::size)
    {
      Blocks1[pos/block_type::size][pos%block_type::size] = *input;
      ++input;
      ++pos;
    }*/
	
	
	while(!input.empty() && pos != block_type::size)
	{
		result_.small_.push_back(*input);
		++input;
      	++pos;
	}		
	
	block_type * Blocks1;	
	
	if( pos == block_type::size ) 
	{      // ennlarge/reallocate Blocks1 array
			block_type * NewBlocks = new block_type[m2*2];
			std::copy(result_.small_.begin(), result_.small_.end(), NewBlocks[0].begin());
			result_.small_.clear();
			//delete [] Blocks1;
			Blocks1 = NewBlocks;
	}
	else
	{
		STXXL_VERBOSE1("runs_creator: Small input optimization, input length: "<<pos);
		result_.elements = pos;
		std::sort(result_.small_.begin(), result_.small_.end(), cmp);
		return;		
	}
#endif
	
    while(!input.empty() && pos != el_in_run)
    {
      Blocks1[pos/block_type::size][pos%block_type::size] = *input;
      ++input;
      ++pos;
    }
    
    // sort first run
    sort_run(Blocks1,pos);
    result_.elements = pos;
	if(pos <  block_type::size && input.empty() ) // small input, do not flush it on the disk(s)
	{
		STXXL_VERBOSE1("runs_creator: Small input optimization, input length: "<<pos);
		result_.small_.resize(pos);
		std::copy(Blocks1[0].begin(), Blocks1[0].begin()+pos, result_.small_.begin());
      	delete [] Blocks1;
		return;
	}

	
	block_type * Blocks2 = Blocks1 + m2;
	block_manager * bm = block_manager::get_instance();
	request_ptr * write_reqs = new request_ptr[m2];
    run_type run;
    
	
    unsigned cur_run_size = div_and_round_up(pos,block_type::size); // in blocks
    run.resize(cur_run_size);
    bm->new_blocks(AllocStr_(),
      trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(run.begin()),
      trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(run.end())
    );
    
    disk_queues::get_instance ()->set_priority_op(disk_queue::WRITE);
    
	result_.runs_sizes.push_back(pos);
    
	// fill the rest of the last block with max values
    for(;pos!=el_in_run;++pos)
        Blocks1[pos/block_type::size][pos%block_type::size] = cmp.max_value();
    
    for (i = 0; i < cur_run_size; ++i)
    {
        run[i].value = Blocks1[i][0];
        write_reqs[i] = Blocks1[i].write(run[i].bid);
        //STXXL_MSG("BID: "<<run[i].bid<<" val: "<<run[i].value)
    }
    result_.runs.push_back(run); // #
    
    if(input.empty())
    {
      // return
      wait_all(write_reqs,write_reqs + cur_run_size);
      delete [] write_reqs;
      delete [] Blocks1;
      return;
    }
      
    pos = 0;
    while(!input.empty() && pos != el_in_run)
    {
      Blocks2[pos/block_type::size][pos%block_type::size] = *input;
      ++input;
      ++pos;
    }
    result_.elements += pos;
    
    if(input.empty())
    {
      // (re)sort internally and return
      pos += el_in_run;
      sort_run(Blocks1,pos); // sort first an second run together
      wait_all(write_reqs,write_reqs + cur_run_size);
      bm->delete_blocks(trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(run.begin()),
        trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(run.end()) );
      
      cur_run_size = div_and_round_up(pos,block_type::size);
      run.resize(cur_run_size);
      bm->new_blocks(AllocStr_(),
        trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(run.begin()),
        trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(run.end())
      );
      
	  result_.runs_sizes[0] = pos;
      // fill the rest of the last block with max values
      for(;pos!=2*el_in_run;++pos)
        Blocks1[pos/block_type::size][pos%block_type::size] = cmp.max_value();
      
	  assert(cur_run_size > m2);
	  
	  for (i = 0; i < m2; ++i)
      {
        run[i].value = Blocks1[i][0];
        write_reqs[i]->wait();
        write_reqs[i] = Blocks1[i].write(run[i].bid);
      }
	  
	  request_ptr * write_reqs1 = new request_ptr[cur_run_size - m2];
	  
      for (;i < cur_run_size; ++i)
      {
        run[i].value = Blocks1[i][0];
        write_reqs1[i-m2] = Blocks1[i].write(run[i].bid);
      }
	  
      result_.runs[0] = run;
      
      wait_all(write_reqs,write_reqs+m2);
      delete [] write_reqs;
	  wait_all(write_reqs1,write_reqs1+cur_run_size - m2);
      delete [] write_reqs1;	  
	  
      delete [] Blocks1;
      
      return;
    }
    
    sort_run(Blocks2,pos);
    
    cur_run_size = div_and_round_up(pos,block_type::size); // in blocks
    run.resize(cur_run_size);
    bm->new_blocks(AllocStr_(),
      trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(run.begin()),
      trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(run.end())
    );
    
    for(i = 0; i < cur_run_size; ++i)
    {
        run[i].value = Blocks2[i][0];
        write_reqs[i]->wait();
        write_reqs[i] = Blocks2[i].write(run[i].bid);
    }
	assert((pos % el_in_run) == 0);
	
    result_.runs.push_back(run);
	result_.runs_sizes.push_back(pos);
    
    while(!input.empty())
    {
      pos = 0;
      while(!input.empty() && pos != el_in_run)
      {
        Blocks1[pos/block_type::size][pos%block_type::size] = *input;
        ++input;
        ++pos;
      }
      result_.elements += pos;
      sort_run(Blocks1,pos);
      cur_run_size = div_and_round_up(pos,block_type::size); // in blocks
      run.resize(cur_run_size);
      bm->new_blocks(AllocStr_(),
        trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(run.begin()),
        trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(run.end())
      );
      
      result_.runs_sizes.push_back(pos); 
	  
	  // fill the rest of the last block with max values (occurs only on the last run)
      for(;pos!=el_in_run;++pos)
        Blocks1[pos/block_type::size][pos%block_type::size] = cmp.max_value();
      
      for(i = 0; i < cur_run_size; ++i)
      {
        run[i].value = Blocks1[i][0];
        write_reqs[i]->wait();
        write_reqs[i] = Blocks1[i].write(run[i].bid);
      }
      result_.runs.push_back(run); // #
	  
      std::swap(Blocks1,Blocks2);
    }
    
    wait_all(write_reqs,write_reqs+m2);
    delete [] write_reqs;
    delete [] ((Blocks1<Blocks2)?Blocks1:Blocks2);
    
  }
  
  
  //! \brief Input strategy for \c runs_creator class
  //!
  //! This strategy together with \c runs_creator class 
  //! allows to create sorted runs 
  //! data structure usable for \c runs_merger 
  //! pushing elements into the sorter 
  //! (using runs_creator::push())
  template <class ValueType_>
  struct use_push
  {
		typedef ValueType_ value_type;
  };
  
  //! \brief Forms sorted runs of elements passed in push() method
  //!
  //! A specialization of \c runs_creator that
  //! allows to create sorted runs 
  //! data structure usable for \c runs_merger from
  //! elements passed in sorted push() method. <BR>
  //! Template parameters:
  //! - \c ValueType_ type of values (parameter for \c use_push strategy)
  //! - \c Cmp_ type of comparison object used for sorting the runs
  //! - \c BlockSize_ size of blocks used to store the runs
  //! - \c AllocStr_ functor that defines allocation strategy for the runs
  template <  
  			  class ValueType_,
              class Cmp_,
              unsigned BlockSize_,
              class AllocStr_>
  class runs_creator<
	  	use_push<ValueType_>,
	  	Cmp_ ,
  		BlockSize_,
  		AllocStr_>
  {
    Cmp_ cmp;
  public:
	typedef Cmp_ cmp_type;
	typedef ValueType_ value_type;
    typedef BID<BlockSize_> bid_type;
    typedef typed_block<BlockSize_,value_type> block_type;
    typedef sort_local::trigger_entry<bid_type,value_type> trigger_entry_type;
    typedef sorted_runs<value_type,trigger_entry_type> sorted_runs_type;
  private:
    typedef typename sorted_runs_type::run_type run_type;
    sorted_runs_type result_; // stores the result (sorted runs)
    unsigned m_; // memory for internal use in blocks
    
    bool output_requested; // true after the result() method was called for the first time
  
    const unsigned m2;
    const unsigned el_in_run;
	unsigned cur_el;  
    block_type *Blocks1;
    block_type *Blocks2;
    request_ptr * write_reqs;
    run_type run;
  
  
    runs_creator();// default construction is forbidden
    runs_creator(const runs_creator & );// copy construction is forbidden
  	runs_creator & operator = (const runs_creator &); // copying is forbidden
    
    
    void sort_run(block_type * run,unsigned elements)
    {
      if(block_type::has_filler)
        std::sort(
                  TwoToOneDimArrayRowAdaptor< 
                    block_type,
                    value_type,
                    block_type::size > (run,0 ),
                  TwoToOneDimArrayRowAdaptor< 
                    block_type,
                    value_type,
                    block_type::size > (run, 
                      elements )
                  ,cmp);
      else 
        std::sort(run[0].elem, run[0].elem + elements, cmp);
    }
	void finish_result()
	{
		if(cur_el == 0) return;
			
		unsigned cur_el_reg = cur_el;
		sort_run(Blocks1, cur_el_reg);
		result_.elements += cur_el_reg;
		if(cur_el_reg <  block_type::size &&  
			result_.elements == cur_el_reg) // small input, do not flush it on the disk(s)
		{
			STXXL_VERBOSE1("runs_creator(use_push): Small input optimization, input length: "<<cur_el_reg);
			result_.small_.resize(cur_el_reg);
			std::copy(Blocks1[0].begin(), Blocks1[0].begin()+cur_el_reg, result_.small_.begin());
			return;
		}
		
		const unsigned cur_run_size = div_and_round_up(cur_el_reg, block_type::size); // in blocks
		run.resize(cur_run_size);
		block_manager * bm = block_manager::get_instance();
		bm->new_blocks(AllocStr_(),
		  trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(run.begin()),
		  trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(run.end())
		);
		
		disk_queues::get_instance ()->set_priority_op(disk_queue::WRITE);
		
		result_.runs_sizes.push_back(cur_el_reg);
		
		// fill the rest of the last block with max values
		for(;cur_el_reg!=el_in_run;++cur_el_reg)
			Blocks1[cur_el_reg/block_type::size][cur_el_reg%block_type::size] = cmp.max_value();
		
		unsigned i=0;
		for (; i < cur_run_size; ++i)
		{
			run[i].value = Blocks1[i][0];
			if(write_reqs[i].get()) write_reqs[i]->wait();
			write_reqs[i] = Blocks1[i].write(run[i].bid);
		}
		result_.runs.push_back(run);
		
		for (i=0; i < m2; ++i)
			if(write_reqs[i].get()) write_reqs[i]->wait();
		
	}
	void cleanup()
	{
			delete [] write_reqs;
    		delete [] ((Blocks1<Blocks2)?Blocks1:Blocks2);
			write_reqs = NULL;
			Blocks1 = Blocks2 = NULL;
	}
  public:
    //! \brief Creates the object
    //! \param c comparator object
    //! \param memory_to_use memory amount that is allowed to used by the sorter in bytes
    runs_creator(Cmp_ c,unsigned memory_to_use):
      	cmp(c),m_(memory_to_use/BlockSize_),output_requested(false),
      	m2(m_ / 2),
    	el_in_run(m2* block_type::size),
  		cur_el(0),
  		Blocks1(new block_type[m2*2]),
  		Blocks2(Blocks1 + m2),
  		write_reqs(new request_ptr[m2])
    {
		assert (2*BlockSize_ <= memory_to_use);
    }
	
	~runs_creator()
	{
			if(!output_requested)
				cleanup();
	}
	
	//! \brief Adds new element to the sorter
	//! \param val value to be added
	void push(const value_type & val)
	{
		assert(output_requested == false);
		unsigned cur_el_reg = cur_el;
		if(cur_el_reg < el_in_run)
		{
			Blocks1[cur_el_reg/block_type::size][cur_el_reg%block_type::size] = val;
			++cur_el;
			return;
		}
		
		assert(el_in_run == cur_el);
		cur_el = 0;
		
		//sort and store Blocks1
		sort_run(Blocks1,el_in_run);
		result_.elements += el_in_run;
		
		const unsigned cur_run_size = div_and_round_up(el_in_run,block_type::size); // in blocks
		run.resize(cur_run_size);
		block_manager * bm = block_manager::get_instance();
		bm->new_blocks(AllocStr_(),
		  trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(run.begin()),
		  trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(run.end())
		);
		
		disk_queues::get_instance ()->set_priority_op(disk_queue::WRITE);
		
		result_.runs_sizes.push_back(el_in_run);
		
		for (unsigned i = 0; i < cur_run_size; ++i)
		{
			run[i].value = Blocks1[i][0];
			if( write_reqs[i].get() ) write_reqs[i]->wait();
			write_reqs[i] = Blocks1[i].write(run[i].bid);
		}
		
		result_.runs.push_back(run); 
	
		std::swap(Blocks1,Blocks2);
		
		push(val);
	}
    
    //! \brief Returns the sorted runs object
    //! \return Sorted runs object.
    //! \remark Returned object is intended to be used by \c runs_merger object as input
    const sorted_runs_type & result()
    {
      
	  if(!output_requested)
      {
        finish_result();
        output_requested = true;
		cleanup();
		 #ifdef STXXL_PRINT_STAT_AFTER_RF
		 STXXL_MSG(*stats::get_instance())
		#endif
      }
      return result_;
    }
  };
  
  
 
  
  //! \brief Input strategy for \c runs_creator class
  //!
  //! This strategy together with \c runs_creator class 
  //! allows to create sorted runs 
  //! data structure usable for \c runs_merger from
  //! sequences of elements in sorted order
  template <class ValueType_>
  struct from_sorted_sequences
  {
		typedef ValueType_ value_type;
  };
  
  //! \brief Forms sorted runs of data taking elements in sorted order (element by element)
  //!
  //! A specialization of \c runs_creator that
  //! allows to create sorted runs 
  //! data structure usable for \c runs_merger from
  //! sequences of elements in sorted order. <BR>
  //! Template parameters:
  //! - \c ValueType_ type of values (parameter for \c from_sorted_sequences strategy)
  //! - \c Cmp_ type of comparison object used for sorting the runs
  //! - \c BlockSize_ size of blocks used to store the runs
  //! - \c AllocStr_ functor that defines allocation strategy for the runs
 template <  
  			  class ValueType_,
              class Cmp_,
              unsigned BlockSize_,
              class AllocStr_>
  class runs_creator<
	  	from_sorted_sequences<ValueType_>,
	  	Cmp_ ,
  		BlockSize_,
  		AllocStr_>
  {
    typedef ValueType_ value_type;
    typedef BID<BlockSize_> bid_type;
    typedef typed_block<BlockSize_,value_type> block_type;
    typedef sort_local::trigger_entry<bid_type,value_type> trigger_entry_type;
	typedef AllocStr_ alloc_strategy_type;
    Cmp_ cmp;
	 
  public:
	typedef Cmp_ cmp_type;
    typedef sorted_runs<value_type,trigger_entry_type> sorted_runs_type;
  private:
    typedef typename sorted_runs_type::run_type run_type;
    sorted_runs_type result_; // stores the result (sorted runs)
    unsigned m_; // memory for internal use in blocks
    buffered_writer<block_type>  writer;
  	block_type * cur_block;
  	unsigned offset;
  	unsigned iblock;
  	unsigned irun;
  	alloc_strategy_type alloc_strategy;
 
    runs_creator();// default construction is forbidden
    runs_creator(const runs_creator & );// copy construction is forbidden
    runs_creator & operator = (const runs_creator &); // copying is forbidden
    
  public:
    //! \brief Creates the object
    //! \param c comparator object
    //! \param memory_to_use memory amount that is allowed to used by the sorter in bytes.
    //! Recommended value: 2 * block_size * D
    runs_creator(Cmp_ c,unsigned memory_to_use):
      	cmp(c),
  		m_(memory_to_use/BlockSize_),
  		writer(m_,m_/2),
  		cur_block(writer.get_free_block()),
  		offset(0),
  		iblock(0),
  		irun(0)
    {
		assert (2*BlockSize_ <= memory_to_use);
    }
	
	//! \brief Adds new element to the current run
	//! \param val value to be added to the current run
	void push(const value_type & val)
	{
		assert(offset < block_type::size );
			
		(*cur_block)[offset] = val;
		++offset;
		
		if(offset == block_type::size)
		{
			// write current block
			
			block_manager * bm = block_manager::get_instance ();
			// allocate space for the block
			result_.runs.resize(irun + 1);
			result_.runs[irun].resize( iblock + 1 );
			bm->new_blocks(
				offset_allocator<alloc_strategy_type>(iblock,alloc_strategy),
          		trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(result_.runs[irun].begin() 
					+ iblock),
        		trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(result_.runs[irun].end())
			);
			
			result_.runs[irun][iblock].value = (*cur_block)[0]; // init trigger
			cur_block = writer.write(cur_block, result_.runs[irun][iblock].bid);
			++iblock;
			
			offset = 0 ;
		}
		
		++result_.elements;
	}
    
	//! \brief Finishes current run and begins new one
	void finish()
	{
		if(offset == 0 && iblock == 0) // current run is empty
			return;
			
		result_.runs_sizes.resize(irun + 1);
		result_.runs_sizes.back() = iblock* block_type::size + offset;
		
		if(offset) // if current block is parially filled
		{
			while(offset != block_type::size)
			{
				(*cur_block)[offset] = cmp.max_value();
				++offset;
			}
			offset = 0;
			
			block_manager *  bm = block_manager::get_instance ();
			// allocate space for the block
			result_.runs.resize(irun + 1);
			result_.runs[irun].resize( iblock + 1 );
			bm->new_blocks(
				offset_allocator<alloc_strategy_type>(iblock,alloc_strategy),
          		trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(result_.runs[irun].begin() 
					+ iblock),
        		trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(result_.runs[irun].end())
			);
			
			result_.runs[irun][iblock].value = (*cur_block)[0]; // init trigger
			cur_block = writer.write(cur_block, result_.runs[irun][iblock].bid);

		}
		else
		{
		}
		
		iblock = 0 ;
		++irun;
	}
	
    //! \brief Returns the sorted runs object
    //! \return Sorted runs object
    //! \remark Returned object is intended to be used by \c runs_merger object as input
    const sorted_runs_type & result()
    {
			finish();
			writer.flush();
		
			return result_;
    }
  };
  
  
  
  //! \brief Checker for the sorted runs object created by the \c runs_creator .
  //! \param sruns sorted runs object
  //! \param cmp comparson object used for checking the order of elements in runs
  //! \return \c true if runs are sorted, \c false otherwise
  template <class RunsType_, class Cmp_>
  bool check_sorted_runs(RunsType_ & sruns, Cmp_ cmp)
  {
    typedef typename RunsType_::block_type block_type;
    typedef typename block_type::value_type value_type;
    STXXL_VERBOSE2("Elements: "<<sruns.elements)
    unsigned nruns = sruns.runs.size();
    STXXL_VERBOSE2("Runs: "<<nruns)
    unsigned irun=0;
    for(irun = 0;irun<nruns;++irun)
    {
       const unsigned nblocks = sruns.runs[irun].size();
       block_type * blocks = new block_type[nblocks];
       request_ptr * reqs = new request_ptr[nblocks];
       for(unsigned j=0;j<nblocks;++j)
       {
         reqs[j] = blocks[j].read(sruns.runs[irun][j].bid);
       }
       wait_all(reqs,reqs + nblocks);
       for(unsigned j=0;j<nblocks;++j)
       {
         if(blocks[j][0] != sruns.runs[irun][j].value)
		 {
		   STXXL_ERRMSG("check_sorted_runs  wrong trigger in the run")
           return false;
		 }
       }
       if(!is_sorted(
                  TwoToOneDimArrayRowAdaptor< 
                    block_type,
                    value_type,
                    block_type::size > (blocks,0 ),
                  TwoToOneDimArrayRowAdaptor< 
                    block_type,
                    value_type,
                    block_type::size > (blocks, 
                       //nblocks*block_type::size
                      //(irun<nruns-1)?(nblocks*block_type::size): (sruns.elements%(nblocks*block_type::size))
	   				sruns.runs_sizes[irun]
                  ),cmp) )
	   {
		   STXXL_ERRMSG("check_sorted_runs  wrong order in the run")
           return false;
	   }
       
       delete [] reqs;
       delete [] blocks;
    }

    return true;
  }
  
  
  
  //! \brief Merges sorted runs
  //!
  //! Template parameters:
  //! - \c RunsType_ type of the sorted runs, available as \c runs_creator::sorted_runs_type ,
  //! - \c Cmp_ type of comparison object used for merging
  //! - \c AllocStr_ allocation strategy used to allocate the blocks for 
  //! storing intermediate results if several merge passes are required
  template <  class RunsType_,
              class Cmp_,
              class AllocStr_ = STXXL_DEFAULT_ALLOC_STRATEGY >
  class runs_merger
  {
    typedef RunsType_ sorted_runs_type;
    typedef AllocStr_ alloc_strategy;
    typedef typename sorted_runs_type::size_type size_type;
    typedef Cmp_ value_cmp;
    typedef typename sorted_runs_type::run_type run_type;
    typedef typename sorted_runs_type::block_type block_type;
    typedef typename block_type::bid_type bid_type;
    typedef block_prefetcher<block_type,typename run_type::iterator> prefetcher_type;
    typedef run_cursor2<block_type,prefetcher_type> run_cursor_type;
    typedef sort_local::run_cursor2_cmp<block_type,prefetcher_type,value_cmp> run_cursor2_cmp_type;
    typedef looser_tree<run_cursor_type,run_cursor2_cmp_type,block_type::size> looser_tree_type;
    
    sorted_runs_type sruns;
    unsigned m_; //  blocks to use - 1
    value_cmp cmp;
    size_type elements_remaining;
    unsigned buffer_pos;
    block_type * current_block;
    run_type consume_seq;
    prefetcher_type * prefetcher;
    looser_tree_type * loosers;
    int * prefetch_seq;
	#ifdef STXXL_CHECK_ORDER_IN_SORTS
	typename block_type::value_type last_element;
	#endif
    
    void merge_recursively();
    
    runs_merger(); // forbidden
    runs_merger(const runs_merger &); // forbidden
	runs_merger & operator = (const runs_merger &); // copying is forbidden
	void deallocate_prefetcher()
	{
		if(prefetcher)
		{
			delete loosers;
      		delete prefetcher;
      		delete [] prefetch_seq;
			prefetcher = NULL;
		}
		// free blocks in runs , (or the user should do it?)
	  	sruns.deallocate_blocks();
	}
  public:
    //! \brief Standard stream typedef
    typedef typename sorted_runs_type::value_type value_type;
    
    //! \brief Creates a runs merger object
    //! \param r input sorted runs object
    //! \param c comparison object
    //! \param memory_to_use amount of memory available for the merger in bytes
    runs_merger(const sorted_runs_type & r,value_cmp c,unsigned memory_to_use):
      sruns(r),m_(memory_to_use/block_type::raw_size /* - 1 */),cmp(c),
      elements_remaining(r.elements),
      current_block(NULL),
  #ifdef STXXL_CHECK_ORDER_IN_SORTS
  		last_element(cmp.min_value()),
  #endif
      prefetcher(NULL)
    {
		
	  if(empty()) return;
		  
	  
	  if(!sruns.small_.empty()) // we have a small input < B, 
		  								 // that is kept in the main memory
	  {
		  STXXL_VERBOSE1("runs_merger: small input optimization, input length: "<<elements_remaining)
		  assert(elements_remaining == sruns.small_.size());
		  current_block = new block_type;
		  std::copy(sruns.small_.begin(), sruns.small_.end(), current_block->begin());
		  current_value = current_block->elem[0];
      	  buffer_pos = 1;
		  
		  return;
	  }
		  
	 #ifdef STXXL_CHECK_ORDER_IN_SORTS
	 assert(check_sorted_runs(r,c));
	 #endif
		
	  current_block = new block_type;
	  
      disk_queues::get_instance ()->set_priority_op (disk_queue::WRITE);
      
      unsigned nruns = sruns.runs.size();
      
      if(m_ < nruns)
      {
        // can not merge runs in one pass
        // merge recursively:
        STXXL_ERRMSG("The implementation of sort requires more than one merge pass, therefore for a better")
        STXXL_ERRMSG("efficiency decrease block size of run storage (a parameter of the run_creator)")
        STXXL_ERRMSG("or increase the amount memory dedicated to the merger.")
        STXXL_ERRMSG("m = "<< m_<<" nruns="<<nruns)
        
        merge_recursively();
        
        nruns = sruns.runs.size();
      }
      
      assert(nruns <= m_);
      
      unsigned i;
	  /*
      const unsigned out_run_size =  
	  	div_and_round_up(elements_remaining,size_type(block_type::size));
	  */
	  unsigned prefetch_seq_size = 0;
	  for (i = 0; i < nruns; i++)
      {
		  prefetch_seq_size += sruns.runs[i].size() ;
      }
	  
      consume_seq.resize(prefetch_seq_size);
    
      prefetch_seq = new int[prefetch_seq_size];
      
      typename run_type::iterator copy_start = consume_seq.begin ();
      for (i = 0; i < nruns; i++)
      {
        copy_start = std::copy(
                sruns.runs[i].begin (),
                sruns.runs[i].end (),
                copy_start	);
      }
      
      std::stable_sort(consume_seq.begin (), consume_seq.end (),
              sort_local::trigger_entry_cmp<bid_type,value_type,value_cmp>(cmp));
    
      int disks_number = config::get_instance ()->disks_number ();
      
      const int n_prefetch_buffers = STXXL_MAX( 2 * disks_number , (int(m_) - int(nruns)) );
    
	  
      #ifdef SORT_OPT_PREFETCHING
      // heuristic
      const int n_opt_prefetch_buffers = 2 * disks_number + (3*(n_prefetch_buffers - 2 * disks_number))/10;
      
      compute_prefetch_schedule(
          consume_seq,
          prefetch_seq,
          n_opt_prefetch_buffers,
          disks_number );
      #else
      for(i=0;i<prefetch_seq_size;++i)
        prefetch_seq[i] = i;
      #endif
      
     
      prefetcher = new prefetcher_type(	
	  									consume_seq.begin(),
                                        consume_seq.end(),
                                        prefetch_seq,
                                        nruns + n_prefetch_buffers);
      
    
      loosers  =  new looser_tree_type(prefetcher, nruns,run_cursor2_cmp_type(cmp));
      
      loosers->multi_merge(current_block->elem);
      current_value = current_block->elem[0];
      buffer_pos = 1;
	  
	  if(elements_remaining <= block_type::size) deallocate_prefetcher();
		  
    }
    
    //! \brief Standard stream method
    bool empty() const
    {
      return elements_remaining==0;
    }
    //! \brief Standard stream method
    runs_merger & operator ++() // preincrement operator
    {
      assert(!empty());
      
      --elements_remaining;
      
      if(buffer_pos != block_type::size)
      {
        current_value = current_block->elem[buffer_pos];
        ++buffer_pos;
      }
      else
      {
        if(!empty())
        {
          loosers->multi_merge(current_block->elem);
		  #ifdef STXXL_CHECK_ORDER_IN_SORTS
		  assert(is_sorted(current_block->elem,current_block->elem +current_block->size,cmp));
		  assert(!cmp(current_block->elem[0],current_value));
		  #endif 
          current_value = current_block->elem[0];  
          buffer_pos = 1;
        }
		if(elements_remaining <= block_type::size)
			deallocate_prefetcher();
      }
	 
      
	  #ifdef STXXL_CHECK_ORDER_IN_SORTS
	  if(!empty())
	  {
		 assert(!cmp(current_value,last_element));
	  	 last_element = current_value;
	  }
	  #endif
      
      return *this;
    }
    //! \brief Standard stream method
    const value_type & operator * () const
    {
      assert(!empty());
      return current_value;
    }
	
	//! \brief Standard stream method
    const value_type * operator -> () const
    {
      assert(!empty());
      return &current_value;
    }
	
	
    //! \brief Destructor
    //! \remark Deallocates blocks of the input sorted runs object
    virtual ~runs_merger()
    {
	  deallocate_prefetcher();
	
      if(current_block) delete current_block;
      // free blocks in runs , (or the user should do it?)
	  sruns.deallocate_blocks();
    }
  private:
    // cache for the current value
    value_type current_value;
  };
  
  
  template <class RunsType_,class Cmp_,class AllocStr_>
  void runs_merger<RunsType_,Cmp_,AllocStr_>::merge_recursively()
  {
    block_manager * bm = block_manager::get_instance();
    unsigned ndisks = config::get_instance ()->disks_number ();
    unsigned nwrite_buffers = 2*ndisks;
    
    unsigned nruns = sruns.runs.size();
    const unsigned merge_factor = static_cast<unsigned>(ceil(pow(nruns,1./ceil(
		log(double(nruns))/log(double(m_))))));
    assert(merge_factor <= m_);
    while(nruns > m_)
    {
      unsigned new_nruns = div_and_round_up(nruns,merge_factor);
      STXXL_VERBOSE("Starting new merge phase: nruns: "<<nruns<<
        " opt_merge_factor: "<<merge_factor<<" m:"<<m_<<" new_nruns: "<<new_nruns)
      
      sorted_runs_type new_runs;
      new_runs.runs.resize(new_nruns);
	  new_runs.runs_sizes.resize(new_nruns);
      new_runs.elements = sruns.elements;
      
      unsigned runs_left = nruns;
      unsigned cur_out_run = 0;
	  unsigned elements_in_new_run = 0 ;
      //unsigned blocks_in_new_run = 0;
      
      
      while(runs_left > 0)
      {
        int runs2merge = STXXL_MIN(runs_left,merge_factor);
        //blocks_in_new_run = 0 ;
		elements_in_new_run = 0 ;
        for(unsigned i = nruns - runs_left; i < (nruns - runs_left + runs2merge);++i)
		{
		  elements_in_new_run += sruns.runs_sizes[i];
          //blocks_in_new_run += sruns.runs[i].size();
		}
		const unsigned blocks_in_new_run1 = div_and_round_up(elements_in_new_run,block_type::size);
		//assert(blocks_in_new_run1 == blocks_in_new_run);
        
		new_runs.runs_sizes[cur_out_run] = elements_in_new_run;
		// allocate run
        new_runs.runs[cur_out_run++].resize(blocks_in_new_run1);
        runs_left -= runs2merge;
        
      }
      
      // allocate blocks for the new runs
      for(unsigned i=0;i<new_runs.runs.size();++i)
        bm->new_blocks( alloc_strategy(),
          trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(new_runs.runs[i].begin()),
          trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(new_runs.runs[i].end()) );
      
      // merge all
      runs_left = nruns;
      cur_out_run = 0;
      size_type elements_left = sruns.elements;
      
      while(runs_left > 0)
      {
          unsigned runs2merge = STXXL_MIN(runs_left,merge_factor);
          STXXL_VERBOSE("Merging "<<runs2merge<<" runs")
          
          sorted_runs_type cur_runs;
          cur_runs.runs.resize(runs2merge);
		  cur_runs.runs_sizes.resize(runs2merge);
        
          std::copy(sruns.runs.begin() + nruns - runs_left,
                    sruns.runs.begin() + nruns - runs_left + runs2merge,
                    cur_runs.runs.begin() );
		  std::copy(sruns.runs_sizes.begin() + nruns - runs_left,
                    sruns.runs_sizes.begin() + nruns - runs_left + runs2merge,
                    cur_runs.runs_sizes.begin() );
        
          runs_left -= runs2merge;
		  /*
          cur_runs.elements = (runs_left)? 
            (new_runs.runs[cur_out_run].size()*block_type::size):
            (elements_left);
		  */
		  cur_runs.elements = new_runs.runs_sizes[cur_out_run];
          elements_left -= cur_runs.elements;
          
          if(runs2merge > 1)
          { 
            
            runs_merger<RunsType_,Cmp_,AllocStr_> merger(cur_runs,cmp,m_*block_type::raw_size);  
            
            { // make sure everything is being destroyed in right time
              
              buf_ostream<block_type,typename run_type::iterator> out(
                  new_runs.runs[cur_out_run].begin(),
                  nwrite_buffers );
              
              size_type cnt = 0;
              const size_type cnt_max = cur_runs.elements;
              
              while(cnt != cnt_max)
              {
                *out = *merger;
                if( (cnt % block_type::size) == 0) // have to write the trigger value
                  new_runs.runs[cur_out_run][cnt/size_type(block_type::size)].value = *merger;
                
                ++cnt;
                ++out;
                ++merger;
              }
              assert(merger.empty());
              
              while(cnt % block_type::size)
              {
                *out = cmp.max_value();
                ++out;
                ++cnt;
              }
            } 
          }
          else
          {
            bm->delete_blocks( 
              trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(
                  new_runs.runs.back().begin()),
              trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(
                  new_runs.runs.back().end()) );
      
            assert(cur_runs.runs.size() == 1);
            
            std::copy(cur_runs.runs.front().begin(),
                      cur_runs.runs.front().end(),
                      new_runs.runs.back().begin());
			new_runs.runs_sizes.back() = cur_runs.runs_sizes.back();
          }
          
          ++cur_out_run;
      }
      assert(elements_left == 0);
      
      nruns = new_nruns;
      sruns = new_runs;
      
    }
  }
  
  
  //! \brief Produces sorted stream from input stream
  //!
  //! Template parameters:
  //! - \c Input_ type of the input stream
  //! - \c Cmp_ type of omparison object used for sorting the runs
  //! - \c BlockSize_ size of blocks used to store the runs
  //! - \c AllocStr_ functor that defines allocation strategy for the runs
  //! \remark Implemented as the composition of \c runs_creator and \c runs_merger .
  template <  class Input_,
              class Cmp_,
              unsigned BlockSize_ = STXXL_DEFAULT_BLOCK_SIZE(typename Input_::value_type),
              class AllocStr_ = STXXL_DEFAULT_ALLOC_STRATEGY>
  class sort
  {
    typedef runs_creator<Input_,Cmp_,BlockSize_,AllocStr_> runs_creator_type;
    typedef typename runs_creator_type::sorted_runs_type sorted_runs_type;
    typedef runs_merger<sorted_runs_type,Cmp_,AllocStr_> runs_merger_type;
    
    runs_creator_type creator;
    runs_merger_type merger;
    
    sort(); // forbidden
    sort(const sort &); // forbidden
  public:
    //! \brief Standard stream typedef
    typedef typename Input_::value_type value_type;
   
    //! \brief Creates the object
    //! \param in input stream
    //! \param c comparator object
    //! \param memory_to_use memory amount that is allowed to used by the sorter in bytes
    sort(Input_ & in,Cmp_ c,unsigned memory_to_use):
      creator(in,c,memory_to_use),
      merger(creator.result(),c,memory_to_use)
  {
  }

    //! \brief Creates the object
    //! \param in input stream
    //! \param c comparator object
    //! \param memory_to_use_rc memory amount that is allowed to used by the runs creator in bytes
    //! \param memory_to_use_m memory amount that is allowed to used by the merger in bytes
    sort(Input_ & in,Cmp_ c,unsigned memory_to_use_rc, unsigned memory_to_use_m):
      creator(in,c,memory_to_use_rc),
      merger(creator.result(),c,memory_to_use_m)
  {
  }

        
    //! \brief Standard stream method
    const value_type & operator * () const
    {
		assert(!empty());
      	return *merger;
    }
	
	const value_type * operator -> () const
    {	
      	assert(!empty());
		return merger.operator->();
	}
    
    //! \brief Standard stream method
    sort & operator ++()
    {
      ++merger;
      return *this;
    }
    
    //! \brief Standard stream method
    bool empty() const
    {
      return merger.empty();
    }
    
  };
  
  //! \brief Computes sorted runs type from value type and block size
  //!
  //! Template parameters
  //! - \c ValueType_ type of values ins sorted runs
  //! - \c BlockSize_ size of blocks where sorted runs stored
  template <  
  			  class ValueType_,
              unsigned BlockSize_>
  class compute_sorted_runs_type
  {
    typedef ValueType_ value_type;
    typedef BID<BlockSize_> bid_type;
    typedef sort_local::trigger_entry<bid_type,value_type> trigger_entry_type;
  public:
    typedef sorted_runs<value_type,trigger_entry_type> result;
  };
  
//! \}
  
};

//! \addtogroup stlalgo
//! \{

//! \brief Sorts range of any random access iterators externally

//! \param begin iterator pointing to the first element of the range
//! \param end iterator pointing to the last+1 element of the range
//! \param cmp comparison object
//! \param MemSize memory to use for sorting (in bytes)
//! \param AS allocation strategy
//!
//! The \c BlockSize template parameter defines the block size to use (in bytes)
//! \warning Slower than External Iterator Sort
template <	unsigned BlockSize, 
					class RandomAccessIterator, 
					class CmpType, 
				   	class  AllocStr>
void sort(RandomAccessIterator begin, 
				RandomAccessIterator end, 
				CmpType cmp, 
				unsigned MemSize,
				AllocStr  AS)
{
	#ifdef BOOST_MSVC
	typedef typename streamify_traits<RandomAccessIterator>::stream_type InputType;
	#else
	typedef typeof(stream::streamify(begin,end)) InputType;
	#endif
	InputType Input(begin,end);
	typedef stream::sort<InputType,CmpType,BlockSize,AllocStr> sorter_type;
	sorter_type	Sort(Input, cmp, MemSize);
	stream::materialize(Sort, begin);
}  

//! \}

__STXXL_END_NAMESPACE

#endif
