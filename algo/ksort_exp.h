#ifndef KSORT_HEADER
#define KSORT_HEADER

/***************************************************************************
 *            ksort.h
 *
 *  Fri Oct  4 19:18:04 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "../mng/mng.h"
#include "../common/rand.h"
#include "../mng/adaptor.h"
#include "../common/simple_vector.h"
#include "../common/switch.h"
#include "interleaved_alloc.h"
#include "intksort.h"
#include "adaptor.h"
#include <list>
#include "async_schedule.h"

//#define SORT_OPT_PREFETCHING
//#define INTERLEAVED_ALLOC


__STXXL_BEGIN_NAMESPACE


template <typename _BIDTp,typename _KeyTp>
struct trigger_entry
{
	typedef _BIDTp bid_type;
	typedef _KeyTp key_type;

	bid_type bid;
	key_type key;
	
	operator bid_type ()
	{
		return bid;
	};
};

template <typename _BIDTp,typename _KeyTp>
inline bool operator < (const trigger_entry<_BIDTp,_KeyTp> & a, 
												const trigger_entry<_BIDTp,_KeyTp> & b)
{
	return (a.key < b.key);
};

template <typename _BIDTp,typename _KeyTp>
inline bool operator > (const trigger_entry<_BIDTp,_KeyTp> & a,
												const trigger_entry<_BIDTp,_KeyTp> & b)
{
	return (a.key > b.key);
};

template <typename type, typename key_type>
struct type_key
{
	key_type key;
	type * ptr;
	
	type_key() {};
	type_key(typename key_type k, type * p): key(k), ptr (p) { };
};

template <typename type>
bool operator  < (const type_key<type> & a, const type_key<type> & b)
{
		return a.key < b.key;
}

template <typename type>
bool operator  > (const type_key<type> & a, const type_key<type> & b)
{
		return a.key > b.key;
}



template <typename block_type,typename bid_type>
struct write_completion_handler: public generic_completion_handler
{
	block_type * block;
	bid_type bid;
	request ** req;
	void operator () (request * completed_req)
	{
		block->read(bid,*req); 
	}
};


template <typename type,typename type_key, typename key_extractor>
void classify_block(type * begin,type * end,type_key * & out,int * bucket,
	unsigned offset, unsigned shift,key_extractor keyobj)
{
	for (type * p = begin;p<end; p++,out++)	// count & create references
	{
		out->ptr = p;
		typename key_extractor::key_type key = keyobj(*p);
		int ibucket = (key - offset) >> shift;
		out->key = key;
		bucket[ibucket]++;
	}
}


template <typename type,
					typename type_key,
					typename block_type,
					typename bid_type,
					typename run_type,
					typename input_bid_iterator,
					typename key_extractor>
inline void write_out(
					type_key *begin,
					type_key * end,
					block_type *& cur_blk,
					const block_type * end_blk,
					int & out_block,
					int & out_pos,
					run_type & run,
					write_completion_handler<block_type,bid_type> *& next_read,
					request ** write_reqs,
					request ** read_reqs,
					input_bid_iterator & it,
					key_extractor keyobj)
{			
	
	block_manager *bm = block_manager::get_instance ();
	type * elem = cur_blk->elem;
	for (type_key * p = begin; p < end; p++)
	{
		elem[out_pos++] = *(p->ptr);
		
		if (out_pos >= block_type::size)
		{
			run[out_block].key = keyobj(*(cur_blk->elem));
			
			if (cur_blk < end_blk)
			{
					next_read->block = cur_blk;
					next_read->req = read_reqs + out_block;
					read_reqs[out_block] = NULL;
					bm->delete_block( next_read->bid = *(it++) );
																																
					cur_blk->write (	
							run[out_block].bid, 
							write_reqs[out_block],
							                  // queue read of block from next run
							*(next_read++));  // after write of block from this run
					
			}
			else
			{
				cur_blk->write (run[out_block].bid, write_reqs[out_block]);
			}
										
			cur_blk++;
			elem = cur_blk->elem;
			out_block++;
			out_pos = 0;
		}
	}
}

template <typename type,
					typename type_key,
					typename block_type,
					typename bid_type,
					typename run_type,
					typename input_bid_iterator>
void
create_runs(
		input_bid_iterator it,
		run_type ** runs,
		int nruns,
		int _m )
{
	const int m2 = _m / 2;
	block_manager *bm = block_manager::get_instance ();
	block_type *Blocks1 = new block_type[m2];
	block_type *Blocks2 = new block_type[m2];
	type_key *refs1 = new type_key[m2 * Blocks1->size];
	type_key *refs2 = new type_key[m2 * Blocks1->size];
	request **read_reqs = new request *[m2];
	request **write_reqs = new request *[m2];
	write_completion_handler<block_type,bid_type> * next_run_reads = 
		new write_completion_handler<block_type,bid_type>[m2];
	
	const int offset = 0;
	const int log_k1 = 10;
	const int log_k2 = int(log2 (m2 * Blocks1->size)) - log_k1 - 1;
	const int k1 = 1 << log_k1;
	const int k2 = 1 << log_k2;
	int *bucket1 = new int[k1];
	int *bucket2 = new int[k2];
	int i;
	
	run_type *run;

	disk_queues::get_instance ()->set_priority_op (disk_queue::WRITE);

	run = *runs;
	int run_size = (*runs)->size ();
	
	for (i = 0; i < run_size; i++)
	{
		bid_type bid = *(it++);
		Blocks1[i].read(bid, read_reqs[i]);
		bm->delete_block(bid);
	}
	
	int k = 0;
	int shift1 = sizeof(typename type::key_type)*8 - log_k1;
	int shift2 = shift1 - log_k2;

	for (; k < nruns; k++)
	{
		run = runs[k];
		run_size = run->size ();
		
		for (i = 0; i < k1; i++)
			bucket1[i] = 0;

		type_key * ref_ptr = refs1;
		for (i = 0; i < run_size; i++)
		{
			if (k)
			{
				write_reqs[i]->wait();
				delete write_reqs[i];
			}
			
	/*	if(! read_reqs[i])
			{
				STXXL_ERRMSG("NULL");
				abort();
			}*/
			read_reqs[i]->wait();
			delete read_reqs[i];

			classify_block(Blocks1[i].begin(),Blocks1[i].end(),ref_ptr,bucket1,offset,shift1);
		}
				
		exclusive_prefix_sum(bucket1, k1);
		classify(refs1, refs1 + run_size * Blocks1->size, refs2, bucket1,
			  offset, shift1);
		
		int out_block = 0;
		int out_pos = 0;
		int next_run_size = (k < nruns - 1)?(runs[k + 1]->size ()):0;
		
		// recurse on each bucket
		type_key *c = refs2;
		type_key *d = refs1;
		block_type *cur_blk = Blocks2;
		block_type *end_blk = Blocks2 + next_run_size;
		write_completion_handler<block_type,bid_type> * next_read = next_run_reads;
		
		for (i = 0; i < k1; i++)
		{
			type_key *cEnd = refs2 + bucket1[i];
			type_key *dEnd = refs1 + bucket1[i];
			
			l1sort (c, cEnd, d, bucket2, k2,
				offset + (1 << shift1) * i , shift2);
			
			write_out<type,type_key,block_type,bid_type,run_type>(
							d,dEnd,cur_blk,end_blk,
							out_block,out_pos,*run,next_read,
							write_reqs,read_reqs,it,keyobj);
			
			c = cEnd;
			d = dEnd;
		}

		std::swap (Blocks1, Blocks2);
	}
	
	mc::waitdel_all (write_reqs, m2);

	delete[]bucket1;
	delete[]bucket2;
	delete[]refs1;
	delete[]refs2;
	delete[]Blocks1;
	delete[]Blocks2;
	delete[]next_run_reads;
	delete[]read_reqs;
	delete[]write_reqs;
}



template <typename block_type>
struct run_cursor
{
	unsigned int pos;
	block_type *buffer;

	inline const typename block_type::type & current () const
	{
		return (*buffer)[pos];
	}
	inline void operator ++ (int)
	{
		pos++;
	}
};

template <typename run_type,
					typename block_type,
					typename run_cursor,
					typename run_cursor2,
					typename trigger_entry>
class prefetcher_writer;


struct have_prefetcher
{
	static void * untyped_prefetcher;
};

template <typename bid_type,
					typename key_type,
					typename block_type,
					unsigned block_size,
					typename run_type>
struct run_cursor2:public run_cursor<block_type>,public have_prefetcher
{
	typedef run_cursor2<bid_type,key_type,block_type,block_size,run_type> _Self;
	typedef prefetcher_writer<run_type,block_type,
		run_cursor<block_type>,_Self,trigger_entry<bid_type,key_type> > prefetcher_writer_type;
											
	static prefetcher_writer_type *& prefetcher() // sorry, hack
	{
		return (prefetcher_writer_type * &) untyped_prefetcher;
	};
	
	run_cursor2 ()
	{
	};
	
	inline bool empty () const
	{
		return (pos >= block_size);
	};
	inline void operator ++ (int);
	inline void make_inf ()
	{
		pos = block_size;
	};
};

void * have_prefetcher::untyped_prefetcher = NULL;

#ifdef SORT_OPT_PREFETCHING
#include "koptprefetcher.h"
#else
#include "kprefetcher.h"
#endif

template <typename bid_type,
					typename key_type,
					typename block_type,
					unsigned block_size,
					typename run_type>
void
run_cursor2<bid_type,key_type,block_type,block_size,run_type>::operator ++ (int)
{
	pos++;
	if (UNLIKELY (pos >= block_size))
	{
		prefetcher()->block_consumed (*this);
	}
};



template <typename block_type>
struct run_cursor_cmp
{
	typedef run_cursor<block_type> cursor_type;
	
	inline bool operator  () (const cursor_type & a, const cursor_type & b)	// greater or equal
	{
		return !((*a.buffer)[a.pos] < (*b.buffer)[b.pos]);
	};
};

template <typename bid_type,
					typename key_type,
					typename block_type,
					unsigned block_size,
					typename run_type>
struct run_cursor2_cmp
{
	typedef run_cursor2<bid_type,key_type,block_type,block_size,run_type> cursor_type;
	inline bool operator  () (const cursor_type & a, const cursor_type & b)
	{
		if (UNLIKELY (b.empty ()))
			return true;	// sentinel emulation
		if (UNLIKELY (a.empty ()))
			return false;	//sentinel emulation

		return (a.current ().key() < b.current ().key());
	};
};


#include "loosertree.h"

template <typename run_type,
					typename prefetcher_writer_type,
					typename block_type,
					unsigned block_size>
void
merge_2runs (run_type ** in_runs, prefetcher_writer_type & prefetcher, unsigned nblocks)
{
	unsigned pos1 = 0, pos2 = 0, out_buf_pos = 0, i_out_buf =
		prefetcher.get_free_write_block();
	unsigned blocks_written = 0;
	block_type *buf1 = prefetcher.r_block (0), *buf2 =
		prefetcher.r_block (1), *out_buf =
		prefetcher.w_block (i_out_buf);
	

	while (UNLIKELY (blocks_written < nblocks))
	{
		if ((*buf1)[pos1].key() < (*buf2)[pos2].key())
		{
			(*out_buf)[out_buf_pos++] = (*buf1)[pos1++];

			if (UNLIKELY (pos1 >= block_size))
			{
				if (LIKELY (prefetcher.block_consumed (buf1)))
				{
					pos1 = 0;
				}
				else
				{
					// first run is empty
					for (;;)
					{
						if (UNLIKELY(out_buf_pos >= block_size))
						{
							i_out_buf = prefetcher.block_filled(i_out_buf);
							
							if (++blocks_written >= nblocks)
								break;

							out_buf = prefetcher.w_block(i_out_buf);
							out_buf_pos = 0;
						}
						else
						{
							(*out_buf)[out_buf_pos++] = (*buf2)[pos2++];
							if (UNLIKELY(pos2 >= block_size))
							{
								pos2 = 0;
								prefetcher.block_consumed(buf2);
							}
						}

					};

					break;
				}
			}
		}
		else
		{
			(*out_buf)[out_buf_pos++] = (*buf2)[pos2++];

			if (UNLIKELY (pos2 >= block_size))
			{
				if (LIKELY (prefetcher.block_consumed (buf2)))
				{
					pos2 = 0;
				}
				else
				{
					// second run is empty
					for (;;)
					{
						if (UNLIKELY(out_buf_pos >= block_size))
						{
							i_out_buf =
								prefetcher.
								block_filled
								(i_out_buf);
							if (++blocks_written
							    >= nblocks)
								break;

							out_buf =
								prefetcher.
								w_block
								(i_out_buf);
							out_buf_pos = 0;
						}
						else
						{
							(*out_buf)
								[out_buf_pos++]
								=
								(*buf1)
								[pos1++];
							if (UNLIKELY
							    (pos1 >=
							     block_size))
							{
								pos1 = 0;
								prefetcher.
									block_consumed
									(buf1);
							}
						}

					};

					break;
				}
			}
		}

		if (UNLIKELY (out_buf_pos >= block_size))
		{
			blocks_written++;
			i_out_buf = prefetcher.block_filled (i_out_buf);
			out_buf = prefetcher.w_block (i_out_buf);
			out_buf_pos = 0;
		}

	}


	block_manager *bm = block_manager::get_instance ();
	for (unsigned i = 0; i < 2; i++)
	{
		unsigned sz = in_runs[i]->size ();
		for (unsigned j = 0; j < sz; j++)
			bm->delete_block ((*in_runs[i])[j].bid);
	}
}

template <typename type,
					typename bid_type,
					typename key_type,
					typename block_type,
					unsigned block_size,
					typename run_type,
					typename run_cursor_type,
					typename run_cursor2_type,
					typename trigger_entry_type>
void
merge_runs_lt (run_type ** in_runs, int nruns, run_type * out_run,unsigned  _m)
{
	typedef prefetcher_writer<run_type,block_type,run_cursor_type,
		run_cursor2_type,trigger_entry_type> prefetcher_writer_type;

	int i;
	run_type consume_seq(out_run->size());

	#ifdef SORT_OPT_PREFETCHING
	int * prefetch_seq = new int[out_run->size()];
	#endif

	typename run_type::iterator copy_start = consume_seq.begin ();
	for (i = 0; i < nruns; i++)
	{
		// TODO: try to avoid copy
		copy_start = std::copy(
						in_runs[i]->begin (),
						in_runs[i]->end (),
						copy_start	);
	}
	std::stable_sort (consume_seq.begin (), consume_seq.end ());

	int disks_number = config::get_instance ()->disks_number ();
	
	
	//const int n_write_buffers = std::max(2 * disks_number ,  ((int(_m) - nruns) / 3) );
	//const int n_prefetch_buffers = std::max( 2 * disks_number , (2 * (int(_m) - nruns) / 3));
	//const int n_write_buffers = 3 * disks_number;
	//const int n_prefetch_buffers = std::max( 2 * disks_number , int(_m) - nruns - n_write_buffers );
	
	const int n_write_buffers = std::max( 2 * disks_number , int(_m) - nruns - n_prefetch_buffers );
	
	#ifdef SORT_OPT_PREFETCHING
	compute_prefetch_schedule(
			consume_seq,
			prefetch_seq,
			n_opt_prefetch_buffers,
			disks_number );
	#endif
	
	#ifdef SORT_OPT_PREFETCHING
	prefetcher_writer_type prefetcher(
						&consume_seq,
						prefetch_seq,
				    out_run,
						nruns,
				    n_prefetch_buffers,
				    n_write_buffers,
				    n_write_buffers/2 );
	#else
	prefetcher_writer_type prefetcher(
						&consume_seq,
				    out_run,
						nruns,
				    n_prefetch_buffers,
				    n_write_buffers,
				    n_write_buffers/2 );
	#endif

	int out_run_size = out_run->size ();

	
	switch (nruns)
	{
	case 2:
		merge_2runs<run_type,prefetcher_writer_type,block_type,block_size>(in_runs, prefetcher, out_run_size);
		return;
	}

	looser_tree<type,
							run_cursor2_type,
							run_cursor2_cmp<bid_type,key_type,block_type,block_size,run_type>,
							prefetcher_writer_type,
							block_size> 
														loosers (&prefetcher, nruns);

	int i_out_buffer = prefetcher.get_free_write_block ();
	//  STXXL_MSG("Block "<<i_out_buffer<<" taken")
	type *out_buffer = prefetcher.w_block (i_out_buffer)->elem;

	for (i = 0; i < out_run_size; i++)
	{
		loosers.multi_merge (out_buffer);

		i_out_buffer = prefetcher.block_filled (i_out_buffer);
		//    STXXL_MSG("Block "<<i_out_buffer<<" taken")
		out_buffer = prefetcher.w_block (i_out_buffer)->elem;
	}
	
	#ifdef SORT_OPT_PREFETCHING
	delete [] prefetch_seq;
	#endif

	block_manager *bm = block_manager::get_instance ();
	for (i = 0; i < nruns; i++)
	{
		unsigned sz = in_runs[i]->size ();
		for (unsigned j = 0; j < sz; j++)
			bm->delete_block ((*in_runs[i])[j].bid);
	}
}


template <typename type,
					typename bid_type,
					typename block_type,
					typename alloc_strategy,
					typename input_bid_iterator>
simple_vector< trigger_entry<bid_type,typename type::key_type> > * 
	ksort_blocks(input_bid_iterator input_bids,unsigned _n,unsigned _m)
{
	typedef trigger_entry<bid_type,typename type::key_type> trigger_entry_type;
	typedef simple_vector< trigger_entry_type > run_type;
	typedef typename interleaved_alloc_traits<alloc_strategy>::strategy interleaved_alloc_strategy;
	unsigned int m2 = _m / 2;
	unsigned int full_runs = _n / m2;
	unsigned int partial_runs = ((_n % m2) ? 1 : 0);
	unsigned int nruns = full_runs + partial_runs;
	unsigned int i;
	
	config *cfg = config::get_instance ();
	block_manager *mng = block_manager::get_instance ();
	int ndisks = cfg->disks_number ();
	
	//STXXL_MSG ("n=" << _n << " nruns=" << nruns << "=" << full_runs << "+"
	//	   << partial_runs) 
	
#ifdef STXXL_IO_STATS
	stats *iostats = stats::get_instance ();
	iostats->reset ();
#endif

	
	double begin =
		stxxl_timestamp (), after_runs_creation, end;

	run_type **runs = new run_type *[nruns];

	for (i = 0; i < full_runs; i++)
		runs[i] = new run_type (m2);

#ifdef INTERLEAVED_ALLOC
	if (partial_runs)
	{
		unsigned int last_run_size = _n - full_runs * m2;
		runs[i] = new run_type (last_run_size);

		mng->new_blocks (interleaved_alloc_strategy (nruns, 0, ndisks),
				 RunsToBIDArrayAdaptor2 < block_type::raw_size,run_type >
				 (runs, 0, nruns, last_run_size),
				 RunsToBIDArrayAdaptor2 < block_type::raw_size,run_type >
				 (runs, _n, nruns, last_run_size));

	}
	else
		mng->new_blocks (interleaved_alloc_strategy (nruns, 0, ndisks),
				 RunsToBIDArrayAdaptor < block_type::raw_size,run_type >
				 (runs, 0, nruns),
				 RunsToBIDArrayAdaptor < block_type::raw_size,run_type >
				 (runs, _n, nruns));
#else
	
		if (partial_runs)
			runs[i] = new run_type (_n - full_runs * m2);
		
		for(i=0;i<nruns;i++)
		{
			mng->new_blocks(	alloc_strategy(0,ndisks),
												trigger_entry_iterator<trigger_entry_type,block_type::raw_size>(runs[i]->begin()),
												trigger_entry_iterator<trigger_entry_type,block_type::raw_size>(runs[i]->end())	);
		}
#endif
	
	
	create_runs<type,
							type_key<type>,
							block_type,
							bid_type,
							run_type,
							input_bid_iterator > (input_bids, runs, nruns,_m);

	after_runs_creation = stxxl_timestamp ();
#ifdef COUNT_WAIT_TIME
	double io_wait_after_rf = stxxl::wait_time_counter;
#endif

#define merge_runs merge_runs_lt


	disk_queues::get_instance ()->set_priority_op (disk_queue::WRITE);

	unsigned int full_runsize = m2;
	run_type **new_runs;

	while (nruns > 1)
	{
		full_runsize = full_runsize * _m;
		unsigned int new_full_runs = _n / full_runsize;
		unsigned int new_partial_runs = ((_n % full_runsize) ? 1 : 0);
		unsigned int new_nruns = new_full_runs + new_partial_runs;

		new_runs = new run_type *[new_nruns];

		for (i = 0; i < new_full_runs; i++)
			new_runs[i] = new run_type (full_runsize);

		if (nruns - new_full_runs * _m == 1)
		{
			// case where one partial run is left to be sorted
			//      STXXL_MSG("case where one partial run is left to be sorted")
			new_runs[i] =
				new run_type (_n - full_runsize * new_full_runs);
			
			run_type *tmp = runs[new_full_runs * _m];
			std::copy (tmp->begin (), tmp->end (),
				   new_runs[i]->begin ());

			mng->new_blocks (interleaved_alloc_strategy
					 (new_nruns - 1, 0, ndisks),
					 RunsToBIDArrayAdaptor <
					 block_type::raw_size,run_type > (new_runs, 0,
							    new_nruns - 1),
					 RunsToBIDArrayAdaptor <
					 block_type::raw_size,run_type > (new_runs,
							    new_full_runs *
							    full_runsize,
							    new_nruns - 1));

			
			for (i = 0; i < new_full_runs; i++)
			{
				merge_runs <type,
										bid_type,
										typename type::key_type,
										block_type,
										block_type::size,
										run_type,
										run_cursor<block_type>,
										run_cursor2<bid_type,typename type::key_type,block_type,block_type::size,run_type>,
										trigger_entry<bid_type,typename type::key_type>
									>
							(runs + i * _m, _m,*(new_runs + i),_m);
			}


		}
		else
		{

			//allocate output blocks
			if (new_partial_runs)
			{
				unsigned int last_run_size =
					_n - full_runsize * new_full_runs;
				new_runs[i] = new run_type (last_run_size);

				mng->new_blocks (interleaved_alloc_strategy
						 (new_nruns, 0, ndisks),
						 RunsToBIDArrayAdaptor2 <
						 block_type::raw_size,run_type > (new_runs,
								    0,
								    new_nruns,
								    last_run_size),
						 RunsToBIDArrayAdaptor2 <
						 block_type::raw_size,run_type > (new_runs,
								    _n,
								    new_nruns,
								    last_run_size));
			}
			else
				mng->new_blocks (interleaved_alloc_strategy
						 (new_nruns, 0, ndisks),
						 RunsToBIDArrayAdaptor <
						 block_type::raw_size,run_type > (new_runs,
								    0,
								    new_nruns),
						 RunsToBIDArrayAdaptor <
						 block_type::raw_size,run_type > (new_runs,
								    _n,
								    new_nruns));


			//      STXXL_MSG("Output runs:" << new_nruns << "=" << new_full_runs << "+" << new_partial_runs)
			for (i = 0; i < new_full_runs; i++)
			{
				//        STXXL_MSG("Merge of m ("<< _m <<") runs")
				merge_runs <type,
										bid_type,
										typename type::key_type,
										block_type,
										block_type::size,
										run_type,
										run_cursor<block_type>,
										run_cursor2<bid_type,typename type::key_type,block_type,block_type::size,run_type >,
										trigger_entry<bid_type,typename type::key_type>
									>
						(runs + i * _m, _m, *(new_runs + i),_m);
			}

			if (new_partial_runs)
			{
				//      STXXL_MSG("Partial merge of "<< (nruns - i*_m) <<" runs")
				merge_runs <type,
										bid_type,
										typename type::key_type,
										block_type,
										block_type::size,
										run_type,
										run_cursor<block_type>,
										run_cursor2<bid_type,typename type::key_type,block_type,block_type::size,run_type >,
										trigger_entry<bid_type,typename type::key_type>
									>
											(runs + i * _m, nruns - i * _m,*(new_runs + i),_m);
			}

		}

		nruns = new_nruns;
		delete[]runs;
		runs = new_runs;

	}
	run_type * result = *runs;
	delete [] runs;
	
	
	end = stxxl_timestamp ();

	STXXL_MSG ("Elapsed time        : " << end - begin << " s. Run creation time: " << 
	after_runs_creation - begin << " s")
#ifdef STXXL_IO_STATS
	STXXL_MSG ("reads               : " << iostats->get_reads ()) 
	STXXL_MSG ("writes              : " << iostats->get_writes ())
	STXXL_MSG ("read time           : " << iostats->get_read_time () << " s") 
	STXXL_MSG ("write time          : " << iostats->get_write_time () <<" s")
	STXXL_MSG ("parallel read time  : " << iostats->get_pread_time () << " s")
	STXXL_MSG ("parallel write time : " << iostats->get_pwrite_time () << " s")
	STXXL_MSG ("parallel io time    : " << iostats->get_pio_time () << " s")
#endif
#ifdef COUNT_WAIT_TIME
	STXXL_MSG ("Time in I/O wait(rf): " << io_wait_after_rf << " s")
	STXXL_MSG ("Time in I/O wait    : " << stxxl::wait_time_counter << " s")
#endif
	
	return result;
}

template <typename _RAIter>
void ksort(_RAIter __first, _RAIter __last,unsigned __M)
{
	typedef simple_vector< trigger_entry<typename _RAIter::bid_type,
			typename _RAIter::vector_type::value_type::key_type> > run_type;
	typedef typename _RAIter::vector_type::value_type value_type;
	typedef typename _RAIter::block_type block_type;
	
	unsigned n=0;
	block_manager *mng = block_manager::get_instance ();
	
	__first.flush();
	
	if((__last - __first)*sizeof(value_type) < __M)
	{
		STXXL_ERRMSG("In-memory sort, not implemented")
		abort();
	}
	else
	{
		if(__first.block_offset()) 
		{
			if(__last.block_offset()) // first and last element reside 
																// not in the beginning of the block
			{
				typename _RAIter::block_type * first_block = new typename _RAIter::block_type;
				typename _RAIter::block_type * last_block = new typename _RAIter::block_type;
				typename _RAIter::bid_type first_bid,last_bid;
				request *req;
				
				first_block->read(*__first.bid(),req);
				mng->new_blocks( FR(), &first_bid,(&first_bid) + 1); // try to overlap
				mng->new_blocks( FR(), &last_bid,(&last_bid) + 1);
				req->wait();
				delete req;
			
				last_block->read(*__last.bid(),req);
				
				unsigned i=0;
				for(;i<__first.block_offset();i++)
				{
					first_block->elem[i] = value_type::min_value();
				}
				
				req->wait();
				delete req;
				
				first_block->write(first_bid,req);
				for(i=__last.block_offset(); i < block_type::size;i++)
				{
					last_block->elem[i] = value_type::max_value();
				}
				
				req->wait();
				delete req;
				
				last_block->write(last_bid,req);
				
				n=__last.bid() - __first.bid() + 1;
				
				std::swap(first_bid,*__first.bid());
				std::swap(last_bid,*__last.bid());
				
				req->wait();
				delete req;
				
				delete first_block;
				delete last_block;

				run_type * out =
						ksort_blocks<	typename _RAIter::vector_type::value_type,
													typename _RAIter::bid_type,
													typename _RAIter::block_type,
													typename _RAIter::vector_type::alloc_strategy,
													typename _RAIter::bids_container_iterator >
														 (__first.bid(),n,__M/block_type::raw_size);
					
				
				first_block = new typename _RAIter::block_type;
				last_block = new typename _RAIter::block_type;
				typename _RAIter::block_type * sorted_first_block = new typename _RAIter::block_type;
				typename _RAIter::block_type * sorted_last_block = new typename _RAIter::block_type;
				request ** reqs = new request * [2];
				
				first_block->read(first_bid,reqs[0]);
				sorted_first_block->read((*(out->begin())).bid,reqs[1]);
				mc::waitdel_all(reqs,2);
				
				last_block->read(last_bid,reqs[0]);
				sorted_last_block->read( ((*out)[out->size() - 1]).bid,reqs[1]);
				
				for(i=__first.block_offset();i<block_type::size;i++)
				{
					first_block->elem[i] = sorted_first_block->elem[i];
				}
				mc::waitdel_all(reqs,2);
				
				first_block->write(first_bid,req);
				
				for(i=0;i<__last.block_offset();i++)
				{
					last_block->elem[i] = sorted_last_block->elem[i];
				}
				
				req->wait();
				delete req;
				
				last_block->write(last_bid,req);
				
				mng->delete_block(out->begin()->bid);
				mng->delete_block((*out)[out->size() - 1].bid);
				
				*__first.bid() = first_bid;
				*__last.bid() = last_bid; 
				
				run_type::iterator it = out->begin(); it++;
				typename _RAIter::bids_container_iterator cur_bid = __first.bid(); cur_bid ++;
				
				for(;cur_bid != __last.bid(); cur_bid++,it++)
				{
					*cur_bid = (*it).bid;
				}
				
				delete first_block;
				delete sorted_first_block;
				delete sorted_last_block;
				delete [] reqs;
				delete out;
				
				req->wait();
				delete req;
				
				delete last_block;
			}
			else
			{
				// first element resides
				// not in the beginning of the block
				
				typename _RAIter::block_type * first_block = new typename _RAIter::block_type;
				typename _RAIter::bid_type first_bid;
				request *req;
				
				first_block->read(*__first.bid(),req);
				mng->new_blocks( FR(), &first_bid,(&first_bid) + 1); // try to overlap
				req->wait();
				delete req;
				
				unsigned i=0;
				for(;i<__first.block_offset();i++)
				{
					first_block->elem[i] = value_type::min_value();
				}
				
				first_block->write(first_bid,req);
				
				n=__last.bid() - __first.bid();
				
				std::swap(first_bid,*__first.bid());
				
				req->wait();
				delete req;
				
				delete first_block;

				run_type * out =
						ksort_blocks<	typename _RAIter::vector_type::value_type,
													typename _RAIter::bid_type,
													typename _RAIter::block_type,
													typename _RAIter::vector_type::alloc_strategy,
													typename _RAIter::bids_container_iterator >
														 (__first.bid(),n,__M/block_type::raw_size);
					
				
				first_block = new typename _RAIter::block_type;
				
				typename _RAIter::block_type * sorted_first_block = new typename _RAIter::block_type;
	
				request ** reqs = new request * [2];
				
				first_block->read(first_bid,reqs[0]);
				sorted_first_block->read((*(out->begin())).bid,reqs[1]);
				mc::waitdel_all(reqs,2);
				
				for(i=__first.block_offset();i<block_type::size;i++)
				{
					first_block->elem[i] = sorted_first_block->elem[i];
				}
				
				first_block->write(first_bid,req);
				
				mng->delete_block(out->begin()->bid);
				
				*__first.bid() = first_bid;
				
				run_type::iterator it = out->begin(); it++;
				typename _RAIter::bids_container_iterator cur_bid = __first.bid(); cur_bid ++;
				
				for(;cur_bid != __last.bid(); cur_bid++,it++)
				{
					*cur_bid = (*it).bid;
				}
				
				*cur_bid = (*it).bid;
				
				delete sorted_first_block;
				delete [] reqs;
				delete out;
				
				req->wait();
				delete req;
				
				delete first_block;
				
			}
			
		}
		else
		{
			if(__last.block_offset()) // last element resides
																// not in the beginning of the block
			{
				typename _RAIter::block_type * last_block = new typename _RAIter::block_type;
				typename _RAIter::bid_type last_bid;
				request *req;
				unsigned i;
				
				last_block->read(*__last.bid(),req);
				mng->new_blocks( FR(), &last_bid,(&last_bid) + 1);
				req->wait();
				delete req;
			
				for(unsigned i=__last.block_offset(); i < block_type::size;i++)
				{
					last_block->elem[i] = value_type::max_value();
				}
				
				last_block->write(last_bid,req);
				
				n=__last.bid() - __first.bid() + 1;
				
				std::swap(last_bid,*__last.bid());
				
				req->wait();
				delete req;
				
				delete last_block;

				run_type * out =
						ksort_blocks<	typename _RAIter::vector_type::value_type,
													typename _RAIter::bid_type,
													typename _RAIter::block_type,
													typename _RAIter::vector_type::alloc_strategy,
													typename _RAIter::bids_container_iterator >
														 (__first.bid(),n,__M/block_type::raw_size);
					
				
				
				last_block = new typename _RAIter::block_type;
				typename _RAIter::block_type * sorted_last_block = new typename _RAIter::block_type;
				request ** reqs = new request * [2];
				
				last_block->read(last_bid,reqs[0]);
				sorted_last_block->read( ((*out)[out->size() - 1]).bid,reqs[1]);
				mc::waitdel_all(reqs,2);
				
				for(i=0;i<__last.block_offset();i++)
				{
					last_block->elem[i] = sorted_last_block->elem[i];
				}
				
				last_block->write(last_bid,req);
				
				mng->delete_block((*out)[out->size() - 1].bid);
				
				*__last.bid() = last_bid; 
				
				run_type::iterator it = out->begin();
				typename _RAIter::bids_container_iterator cur_bid = __first.bid();
				
				for(;cur_bid != __last.bid(); cur_bid++,it++)
				{
					*cur_bid = (*it).bid;
				}
				
				delete sorted_last_block;
				delete [] reqs;
				delete out;
				
				req->wait();
				delete req;
				
				delete last_block;
			}
			else
			{
				// first and last element resine in the beginning of blocks 
				n = __last.bid() - __first.bid();
				
				run_type * out =
						ksort_blocks<	typename _RAIter::vector_type::value_type,
													typename _RAIter::bid_type,
													typename _RAIter::block_type,
													typename _RAIter::vector_type::alloc_strategy,
													typename _RAIter::bids_container_iterator >
														 (__first.bid(),n,__M/block_type::raw_size);
				
				run_type::iterator it = out->begin();
				typename _RAIter::bids_container_iterator cur_bid = __first.bid();
				
				for(;cur_bid != __last.bid(); cur_bid++,it++)
				{
					*cur_bid = (*it).bid;
				}
				
			}
		}
		
	}
};

__STXXL_END_NAMESPACE

#endif
