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

template < unsigned blk_sz,class Y,class X > 
  struct bid_iterator_traits< 
    __gnu_cxx::__normal_iterator<  sort_local::trigger_entry<BID<blk_sz>,Y> * ,  X> >
	{
		enum
		{
			block_size = blk_sz
		};	
	};


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
    typedef off_t size_type;
    typedef std::vector<trigger_entry_type> run_type;
    typedef typed_block<bid_type::size,value_type> block_type;
    size_type elements;
    std::vector<run_type> runs;
    
    sorted_runs():elements(0) {}
  };
  
  //! \brief Forms sorted runs of data from a stream
  //!
  //! Template parameters:
  //! - \c Input_ type of the input stream
  //! - \c Cmp_ type of omparison object used for sorting the runs
  //! - \c BlockSize_ size of blocks used to store the runs
  //! - \c AllocStr_ functor that defines allocation strategy for the runs
  template <  class Input_,
              class Cmp_,
              unsigned BlockSize_ = STXXL_DEFAULT_BLOCK_SIZE(typename Input_::value_type),
              class AllocStr_ = STXXL_DEFAULT_ALLOC_STRATEGY>
  class runs_creator
  {
    typedef typename Input_::value_type value_type;
    typedef BID<BlockSize_> bid_type;
    typedef typed_block<BlockSize_,value_type> block_type;
    typedef sort_local::trigger_entry<bid_type,value_type> trigger_entry_type;
    Input_ & input;
    Cmp_ cmp;
  public:
    typedef sorted_runs<value_type,trigger_entry_type> sorted_runs_type;
  private:
    typedef typename sorted_runs_type::run_type run_type;
    sorted_runs_type result_; // stores the result (sorted runs)
    unsigned m_; // memory for internal use in blocks
    bool result_computed; // true result is already computed (used in 'result' method)
    
    runs_creator();// default construction is forbidden
    runs_creator(const runs_creator & );// copy construction is forbidden
    
    
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
    block_manager * bm = block_manager::get_instance();
    block_type *Blocks1 = new block_type[m2*2];
    block_type *Blocks2 = Blocks1 + m2;
    request_ptr * write_reqs = new request_ptr[m2];
    run_type run;
    
    unsigned pos = 0;
    while(!input.empty() && pos != el_in_run)
    {
      Blocks1[pos/block_type::size][pos%block_type::size] = *input;
      ++input;
      ++pos;
    }
    
    // sort first run
    sort_run(Blocks1,pos);
    result_.elements = pos;
    unsigned cur_run_size = div_and_round_up(pos,block_type::size); // in blocks
    run.resize(cur_run_size);
    bm->new_blocks(AllocStr_(),
      trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(run.begin()),
      trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(run.end())
    );
    
    disk_queues::get_instance ()->set_priority_op(disk_queue::WRITE);
    
    // fill the rest of the last block with max values
    for(;pos!=el_in_run;++pos)
        Blocks1[pos/block_type::size][pos%block_type::size] = cmp.max_value();
    
    for (i = 0; i < cur_run_size; ++i)
    {
        run[i].value = Blocks1[i][0];
        write_reqs[i] = Blocks1[i].write(run[i].bid);
        //STXXL_MSG("BID: "<<run[i].bid<<" val: "<<run[i].value)
    }
    result_.runs.push_back(run);
    
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
      
      // fill the rest of the last block with max values
      for(;pos!=2*el_in_run;++pos)
        Blocks1[pos/block_type::size][pos%block_type::size] = cmp.max_value();
      
      for (i = 0; i < cur_run_size; ++i)
      {
        run[i].value = Blocks1[i][0];
        write_reqs[i]->wait();
        write_reqs[i] = Blocks1[i].write(run[i].bid);
      }
      result_.runs[0] = run;
      
      wait_all(write_reqs,write_reqs+cur_run_size);
      delete [] write_reqs;
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
    result_.runs.push_back(run);
    
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
      
      // fill the rest of the last block with max values (occurs only on the last run)
      for(;pos!=el_in_run;++pos)
        Blocks1[pos/block_type::size][pos%block_type::size] = cmp.max_value();
      
      for(i = 0; i < cur_run_size; ++i)
      {
        run[i].value = Blocks1[i][0];
        write_reqs[i]->wait();
        write_reqs[i] = Blocks1[i].write(run[i].bid);
      }
      result_.runs.push_back(run);
      std::swap(Blocks1,Blocks2);
    }
    
    wait_all(write_reqs,write_reqs+m2);
    delete [] write_reqs;
    delete [] ((Blocks1<Blocks2)?Blocks1:Blocks2);
    
  }
  
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
           return false;
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
                      (irun<nruns-1)?(nblocks*block_type::size): (sruns.elements%(nblocks*block_type::size))
                  ),cmp) )
           return false;
       
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
    block_type current_block;
    run_type consume_seq;
    prefetcher_type * prefetcher;
    looser_tree_type * loosers;
    int * prefetch_seq;
    
    void merge_recursively();
    
    runs_merger(); // forbidden
    runs_merger(const runs_merger &); // forbidden
  public:
    //! \brief Standard stream typedef
    typedef typename sorted_runs_type::value_type value_type;
    
    //! \brief Creates a runs merger object
    //! \param r input sorted runs object
    //! \param c comparison object
    //! \param memory_to_use amount of memory available for the merger in bytes
    runs_merger(const sorted_runs_type & r,value_cmp c,unsigned memory_to_use):
      sruns(r),m_(memory_to_use/block_type::raw_size /* - 1 */),cmp(c),
      elements_remaining(r.elements)
    {
      disk_queues::get_instance ()->set_priority_op (disk_queue::WRITE);
      
      unsigned nruns = sruns.runs.size();
      
      if(m_ < nruns)
      {
        // can not merge runs in one pass
        // merge recursively:
        STXXL_ERRMSG("An implementation requires more than one merge pass, therefore for")
        STXXL_ERRMSG("efficiency decrease block size of run storage (a parameter of the run_creator)")
        STXXL_ERRMSG("or increase the ammount memory dedicated to the merger.")
        STXXL_ERRMSG("m = "<< m_<<" nruns="<<nruns)
        
        merge_recursively();
        
        nruns = sruns.runs.size();
      }
      
      assert(nruns <= m_);
      
      unsigned i;
      const unsigned out_run_size = div_and_round_up(elements_remaining,size_type(block_type::size));
      consume_seq.resize(out_run_size);
    
      prefetch_seq = new int[out_run_size];
      
      typename run_type::iterator copy_start = consume_seq.begin ();
      for (i = 0; i < nruns; i++)
      {
        copy_start = std::copy(
                sruns.runs[i].begin (),
                sruns.runs[i].end (),
                copy_start	);
      }
      
      std::sort(consume_seq.begin (), consume_seq.end (),
              sort_local::trigger_entry_cmp<bid_type,value_type,value_cmp>(cmp));
    
      int disks_number = config::get_instance ()->disks_number ();
      
      const int n_prefetch_buffers = std::max( 2 * disks_number , (int(m_) - int(nruns)) );
    
      #ifdef SORT_OPT_PREFETCHING
      // heuristic
      const int n_opt_prefetch_buffers = 2 * disks_number + (3*(n_prefetch_buffers - 2 * disks_number))/10;
      
      compute_prefetch_schedule(
          consume_seq,
          prefetch_seq,
          n_opt_prefetch_buffers,
          disks_number );
      #else
      for(i=0;i<out_run_size;++i)
        prefetch_seq[i] = i;
      #endif
      
     
      prefetcher = new prefetcher_type(	consume_seq.begin(),
                                        consume_seq.end(),
                                        prefetch_seq,
                                        nruns + n_prefetch_buffers);
      
    
      loosers  =  new looser_tree_type(prefetcher, nruns,run_cursor2_cmp_type(cmp));
      
      loosers->multi_merge(current_block.elem);
      current_value = current_block.elem[0];
      buffer_pos = 1;
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
        current_value = current_block.elem[buffer_pos];
        ++buffer_pos;
      }
      else
      {
        if(!empty())
        {
          loosers->multi_merge(current_block.elem);
          current_value = current_block.elem[0];  
          buffer_pos = 1;
        }
      }
      
      
      return *this;
    }
    //! \brief Standard stream method
    const value_type & operator * () const
    {
      assert(!empty());
      return current_value;
    }
    //! \brief Destructor
    //! \remark Deallocates blocks of the input sorted runs object
    virtual ~runs_merger()
    {
      delete loosers;
      delete prefetcher;
      delete [] prefetch_seq;
      
      block_manager * bm = block_manager::get_instance();
      // free blocks in runs
      for(unsigned i=0;i<sruns.runs.size();++i)
        bm->delete_blocks(
          trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(sruns.runs[i].begin()),
          trigger_entry_iterator<typename run_type::iterator,block_type::raw_size>(sruns.runs[i].end()) );
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
    const unsigned merge_factor = static_cast<unsigned>(ceil(pow(nruns,1./ceil(log(nruns)/log(m_)))));
    assert(merge_factor <= m_);
    while(nruns > m_)
    {
      unsigned new_nruns = div_and_round_up(nruns,merge_factor);
      STXXL_VERBOSE("Starting new merge phase: nruns: "<<nruns<<
        " opt_merge_factor: "<<merge_factor<<" m:"<<m_<<" new_nruns: "<<new_nruns)
      
      sorted_runs_type new_runs;
      new_runs.runs.resize(new_nruns);
      new_runs.elements = sruns.elements;
      
      unsigned runs_left = nruns;
      unsigned cur_out_run = 0;
      unsigned blocks_in_new_run = 0;
      
      
      while(runs_left > 0)
      {
        int runs2merge = STXXL_MIN(runs_left,merge_factor);
        blocks_in_new_run = 0;
        for(unsigned i = nruns - runs_left; i < (nruns - runs_left + runs2merge);++i)
          blocks_in_new_run += sruns.runs[i].size();
        // allocate run
        new_runs.runs[cur_out_run++].resize(blocks_in_new_run);
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
        
          std::copy(sruns.runs.begin() + nruns - runs_left,
                    sruns.runs.begin() + nruns - runs_left + runs2merge,
                    cur_runs.runs.begin() );
        
          runs_left -= runs2merge;
          cur_runs.elements = (runs_left)? 
            (new_runs.runs[cur_out_run].size()*block_type::size):
            (elements_left);
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
      merger(creator.result(),c,memory_to_use) {}
        
    //! \brief Standard stream method
    const value_type & operator * () const
    {
      return *merger;
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
    
  
//! \}
  
};



__STXXL_END_NAMESPACE

#endif
