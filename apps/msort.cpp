/***************************************************************************
 *            msort.cpp
 *
 *  Sat Aug 24 23:51:47 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "msort.h"


#define OffsetStriping OffsetStriping
//#define OffsetStriping OffsetSR
//#define OffsetStriping OffsetFR
//#define OffsetStriping OffsetRC


#define InterleavedRuns InterleavedStriping
//#define InterleavedRuns InterleavedSR
//#define InterleavedRuns InterleavedFR
//#define InterleavedRuns InterleavedRC

void
gen_input (std::list < BID < B > >&input, int ndisks)
{
	block_manager *bm = block_manager::get_instance ();
	BIDArray < B > array (_m);
	Block *Blocks = new Block[_m];
	request **reqs = new request *[_m];
	unsigned nruns = _n / _m;

	for (unsigned i = 0; i < nruns; i++)
	{
		bm->new_blocks (OffsetStriping (0, ndisks, (i * _m) % ndisks),
				array.begin (), array.end ());
		unsigned j = 0;
		for (; j < _m; j++)
		{
			for (int k = 0; k < Block::size; k++)
				Blocks[j][k].key = rand32 ();

			Blocks[j].write (array[j], reqs[j]);
		}

		for (j = 0; j < _m; j++)
			input.push_front (array[j]);

		mc::waitdel_all (reqs, _m);
	}

	if (_n % _m)
	{
		int rest = _n - nruns * _m;
		bm->new_blocks (OffsetStriping
				(0, ndisks, (nruns * _m) % ndisks),
				array.begin (), array.begin () + rest);
		int j = 0;
		for (; j < rest; j++)
		{
			for (int k = 0; k < Block::size; k++)
				Blocks[j][k].key = rand32 ();

			Blocks[j].write (array[j], reqs[j]);
		}

		for (j = 0; j < rest; j++)
			input.push_front (array[j]);

		mc::waitdel_all (reqs, rest);
	}


	delete[]Blocks;
	delete[]reqs;
}


template < unsigned _blk_sz, class __pos_type =
	int >struct RunsToBIDArrayAdaptor:public TwoToOneDimArrayAdaptorBase <
	Run *, BID < _blk_sz >, __pos_type >
{
	typedef RunsToBIDArrayAdaptor < _blk_sz, __pos_type > _Self;
	typedef BID < _blk_sz > data_type;

	enum
	{ block_size = _blk_sz };

	unsigned dim_size;

	  RunsToBIDArrayAdaptor (Run * *a, __pos_type p,
				 unsigned d):TwoToOneDimArrayAdaptorBase <
		Run *, BID < _blk_sz >, __pos_type > (a, p), dim_size (d)
	{
	};
	RunsToBIDArrayAdaptor (const _Self & a):TwoToOneDimArrayAdaptorBase <
		Run *, BID < _blk_sz >, __pos_type > (a),
		dim_size (a.dim_size)
	{
	};

	const _Self & operator = (const _Self & a)
	{
		array = a.array;
		pos = a.pos;
		dim_size = a.dim_size;
		return *this;
	}

	data_type & operator * ()
	{
		CHECK_RUN_BOUNDS (pos)
			return (BID < _blk_sz >
				&)((*(array[(pos) % dim_size]))[(pos) /
								dim_size].
				   bid);
	};

	const data_type *operator -> () const
	{
		CHECK_RUN_BOUNDS (pos)
			return
			&((*(array[(pos) % dim_size])[(pos) / dim_size].bid));
	};


	data_type & operator [](__pos_type n) const
	{
		n += pos;
		CHECK_RUN_BOUNDS (n)
			return (BID < _blk_sz >
				&) ((*(array[(n) % dim_size]))[(n) /
							       dim_size].bid);
	};

};

     BLOCK_ADAPTOR_OPERATORS (RunsToBIDArrayAdaptor) template < unsigned
     _blk_sz, class __pos_type =
	     int >struct RunsToBIDArrayAdaptor2:public
	     TwoToOneDimArrayAdaptorBase < Run *, BID < _blk_sz >,
	     __pos_type >
     {
	     typedef RunsToBIDArrayAdaptor2 < _blk_sz, __pos_type > _Self;
	     typedef BID < _blk_sz > data_type;

	     enum
	     { block_size = _blk_sz };

	     __pos_type w, h, K;

	       RunsToBIDArrayAdaptor2 (Run * *a, __pos_type p, int _w,
				       int _h):TwoToOneDimArrayAdaptorBase <
		     Run *, BID < _blk_sz >, __pos_type > (a, p), w (_w),
		     h (_h), K (_w * _h)
	     {
	     };

	     RunsToBIDArrayAdaptor2 (const _Self &
				     a):TwoToOneDimArrayAdaptorBase < Run *,
		     BID < _blk_sz >, __pos_type > (a), w (a.w), h (a.h),
		     K (a.K)
	     {
	     };

	     const _Self & operator = (const _Self & a)
	     {
		     array = a.array;
		     pos = a.pos;
		     w = a.w;
		     h = a.h;
		     K = a.K;
		     return *this;
	     }

	     data_type & operator * ()
	     {
		     register __pos_type i = pos - K;
		     if (i < 0)
			     return (BID < _blk_sz >
				     &)((*(array[(pos) % w]))[(pos) / w].bid);

		     register __pos_type _w = w;
		     _w--;
		     return (BID < _blk_sz >
			     &)((*(array[(i) % _w]))[h + (i / _w)].bid);

	     };

	     const data_type *operator -> () const
	     {
		     register __pos_type i = pos - K;
		     if (i < 0)
			       return &((*(array[(pos) % w])[(pos) / w].bid));

		     register __pos_type _w = w;
		       _w--;
		       return &((*(array[(i) % _w])[h + (i / _w)].bid));
	     };


	     data_type & operator [](__pos_type n) const
	     {
		     n += pos;
		     register __pos_type i = n - K;
		     if (i < 0)
			       return (BID < _blk_sz >
				       &) ((*(array[(n) % w]))[(n) / w].bid);

		     register __pos_type _w = w;
		       _w--;
		       return (BID < _blk_sz >
			       &) ((*(array[(i) % _w]))[h + (i / _w)].bid);
	     };

     };

BLOCK_ADAPTOR_OPERATORS (RunsToBIDArrayAdaptor2)

void create_runs (std::list < BID < B > >&input, Run ** runs, int nruns)
{
	int m2 = _m / 2;
	block_manager *bm = block_manager::get_instance ();
	Block *Blocks1 = new Block[m2];
	Block *Blocks2 = new Block[m2];
	request **read_reqs = new request *[m2];
	request **write_reqs = new request *[m2];
	BIDArray < B > bids (m2);
	Run *run;

	//  STXXL_MSG("Creating runs")
	disk_queues::get_instance ()->set_priority_op (disk_queue::READ);

	int i;
	std::list < BID < B > >::iterator it = input.begin ();
	int k = 0;
	int run_size = 0;
	for (; LIKELY (k < nruns); k++)
	{
		run = runs[k];
		run_size = run->size ();

		for (i = 0; i < run_size; i++)
		{
			bids[i] = *(it++);
			Blocks1[i].read (bids[i], read_reqs[i]);
		}

		for (i = 0; i < run_size; i++)
			bm->delete_block (bids[i]);

		mc::waitdel_all (read_reqs, run_size);

		//      std::sort(
		//              TwoToOneDimArrayRowAdaptor< Block,Block::type,Block::size > (Blocks1,0),
		//              TwoToOneDimArrayRowAdaptor< Block,Block::type,Block::size > (Blocks1, run_size*Block::size )
		//              );


		// here goes a hack
		std::sort (Blocks1[0].elem, Blocks1[m2].elem);

		if (UNLIKELY (k))
			mc::waitdel_all (write_reqs, m2);

		for (i = 0; i < run_size; i++)
		{
			(*run)[i].value = Blocks1[i][0];
			Blocks1[i].write ((*run)[i].bid, write_reqs[i]);
		}
		std::swap (Blocks1, Blocks2);
	}

	mc::waitdel_all (write_reqs, run_size);


	delete[]Blocks1;
	delete[]Blocks2;
	delete[]read_reqs;
	delete[]write_reqs;
}



struct RunCursor
{
	int pos;
	Block *buffer;

	inline const Type & current () const
	{
		return (*buffer)[pos];
	}
	inline void operator ++ (int)
	{
		pos++;
	}
};


class PrefetcherWriter;

struct RunCursor2:public RunCursor
{
	static PrefetcherWriter *prefetcher;
	  RunCursor2 ()
	{
	};
	inline bool empty () const
	{
		return (pos >= Block::size);
	};
	inline void operator ++ (int);
	inline void make_inf ()
	{
		pos = Block::size;
	};
};


PrefetcherWriter *
	RunCursor2::prefetcher =
	NULL;

#include "prefetcher.h"


void
RunCursor2::operator ++ (int)
{
	pos++;
	if (UNLIKELY (pos >= Block::size))
	{
		prefetcher->block_consumed (*this);
	}
};

struct RunCursorCmp
{
	inline bool operator  () (const RunCursor & a, const RunCursor & b)	// greater or equal
	{
		return !((*a.buffer)[a.pos] < (*b.buffer)[b.pos]);
	};
};

struct RunCursor2Cmp
{
	inline bool operator  () (const RunCursor2 & a, const RunCursor2 & b)
	{
		if (UNLIKELY (b.empty ()))
			return true;	// sentinel emulation
		if (UNLIKELY (a.empty ()))
			return false;	//sentinel emulation

		return (a.current () < b.current ());
	};
};

#include "loosertree.h"


void merge_2runs (Run ** in_runs, PrefetcherWriter & prefetcher, int nblocks);
void merge_2runs (Run ** in_runs, PrefetcherWriter & prefetcher);


void
merge_runs_lt (Run ** in_runs, int nruns, Run * out_run)
{
	//  STXXL_MSG(__PRETTY_FUNCTION__)
	int i;
	Run prefetch_seq (out_run->size ());

	Run::iterator copy_start = prefetch_seq.begin ();
	for (i = 0; i < nruns; i++)
	{
		// TODO: try to avoid copiing
		copy_start =
			std::copy (in_runs[i]->begin (), in_runs[i]->end (),
				   copy_start);
	}
	std::sort (prefetch_seq.begin (), prefetch_seq.end ());

	// start prefetching
	int disks_number = config::get_instance ()->disks_number ();
	PrefetcherWriter prefetcher (&prefetch_seq,
				     out_run, nruns,
				     3 * disks_number + 2 * (_m - nruns) / 3,
				     3 * disks_number + (_m - nruns) / 3,
				     3 * disks_number / 2 + (_m - nruns) / 6);

	int out_run_size = out_run->size ();

	switch (nruns)
	{
	case 2:
		merge_2runs (in_runs, prefetcher, out_run_size);
		return;
	}

	looser_tree loosers (&prefetcher, nruns);

	int i_out_buffer = prefetcher.get_free_write_block ();
	//  STXXL_MSG("Block "<<i_out_buffer<<" taken")
	Type *out_buffer = prefetcher.w_block (i_out_buffer)->elem;

	for (i = 0; i < out_run_size; i++)
	{
		loosers.multi_merge (out_buffer);

		i_out_buffer = prefetcher.block_filled (i_out_buffer);
		//    STXXL_MSG("Block "<<i_out_buffer<<" taken")
		out_buffer = prefetcher.w_block (i_out_buffer)->elem;
	}

	block_manager *bm = block_manager::get_instance ();
	for (i = 0; i < nruns; i++)
	{
		unsigned sz = in_runs[i]->size ();
		for (unsigned j = 0; j < sz; j++)
			bm->delete_block ((*in_runs[i])[j].bid);
	}
}

void
merge_runs (Run ** in_runs, int nruns, Run * out_run)
{
	int i;
	Run prefetch_seq (out_run->size ());
	Run::iterator copy_start = prefetch_seq.begin ();
	for (i = 0; i < nruns; i++)
	{
		// TODO: try to avoid copiing
		copy_start =
			std::copy (in_runs[i]->begin (), in_runs[i]->end (),
				   copy_start);
	}
	std::sort (prefetch_seq.begin (), prefetch_seq.end ());

	// start prefetching
	int disks_number = config::get_instance ()->disks_number ();
	PrefetcherWriter prefetcher (&prefetch_seq,
				     out_run, nruns,
				     3 * disks_number + 2 * (_m - nruns) / 3,
				     3 * disks_number + (_m - nruns) / 3,
				     3 * disks_number / 2 + (_m - nruns) / 6);


	switch (nruns)
	{
	case 2:
		merge_2runs (in_runs, prefetcher, out_run->size ());
		return;
		/*
		 * case 3:
		 * merge_3runs(in_runs,prefetcher,out_run->size());
		 * return;
		 * case 4:
		 * merge_4runs(in_runs,prefetcher,out_run->size());
		 * return; */

	}


	std::priority_queue < RunCursor, std::vector < RunCursor >,
		RunCursorCmp > sel_heap;

	RunCursor cursor;
	for (i = 0; i < nruns; i++)
	{
		cursor.buffer = prefetcher.r_block (i);
		cursor.pos = 0;
		//      prefetcher.wait_read_completion(i);
		sel_heap.push (cursor);
	};

	int i_out_buffer = prefetcher.get_free_write_block ();
	Block *out_buffer = prefetcher.w_block (i_out_buffer);
	int out_buffer_pos = 0;

	for (;;)
	{

		RunCursor smallest = sel_heap.top ();
		sel_heap.pop ();

		(*out_buffer)[out_buffer_pos++] = smallest.current ();
		smallest++;

		if (UNLIKELY (smallest.pos >= Block::size))
		{
			if (LIKELY (prefetcher.block_consumed (smallest)))
			{
				sel_heap.push (smallest);
			}
			else
			{

				if (sel_heap.empty ())
				{
					prefetcher.
						block_filled (i_out_buffer);

					block_manager *bm =
						block_manager::
						get_instance ();
					for (i = 0; i < nruns; i++)
					{
						unsigned sz =
							in_runs[i]->size ();
						for (unsigned j = 0; j < sz;
						     j++)
							bm->delete_block ((*in_runs[i])[j].bid);
					}

					return;
				}

			}
		}
		else
		{
			sel_heap.push (smallest);
		}

		if (UNLIKELY (out_buffer_pos >= Block::size))
		{
			i_out_buffer = prefetcher.block_filled (i_out_buffer);
			out_buffer = prefetcher.w_block (i_out_buffer);
			out_buffer_pos = 0;
		}

	};

}

void
merge_2runs (Run ** in_runs, PrefetcherWriter & prefetcher)
{
	RunCursor2Cmp cmp;
	RunCursor2 a, b;
	RunCursor2::prefetcher = &prefetcher;
	a.pos = 0;
	b.pos = 0;
	a.buffer = prefetcher.r_block (0);
	b.buffer = prefetcher.r_block (1);
	int i_out_buffer = prefetcher.get_free_write_block ();
	int out_buffer_pos = 0;
	Block *out_buffer = prefetcher.w_block (i_out_buffer);

	for (;;)
	{
		if (UNLIKELY (a.empty () && b.empty ()))
		{
			block_manager *bm = block_manager::get_instance ();
			for (unsigned i = 0; i < 2; i++)
			{
				unsigned sz = in_runs[i]->size ();
				for (unsigned j = 0; j < sz; j++)
					bm->delete_block ((*in_runs[i])[j].
							  bid);
			}
			return;
		}
		else
		{
			if (cmp (a, b))
			{
				(*out_buffer)[out_buffer_pos++] =
					a.current ();
				a++;
			}
			else
			{
				(*out_buffer)[out_buffer_pos++] =
					b.current ();
				b++;
			}

			if (UNLIKELY (out_buffer_pos >= Block::size))
			{
				i_out_buffer =
					prefetcher.
					block_filled (i_out_buffer);
				out_buffer =
					prefetcher.w_block (i_out_buffer);
				out_buffer_pos = 0;
			}

		}
	}

}

void
merge_2runs (Run ** in_runs, PrefetcherWriter & prefetcher, int nblocks)
{
	int pos1 = 0, pos2 = 0, out_buf_pos = 0, i_out_buf =
		prefetcher.get_free_write_block ();
	int blocks_written = 0;
	Block *buf1 = prefetcher.r_block (0), *buf2 =
		prefetcher.r_block (1), *out_buf =
		prefetcher.w_block (i_out_buf);
	////Type v1= (*buf1)[pos1], v2 = (*buf2)[pos2];

	while (UNLIKELY (blocks_written < nblocks))
	{
		if ((*buf1)[pos1] < (*buf2)[pos2])
		{
			(*out_buf)[out_buf_pos++] = (*buf1)[pos1++];

			if (UNLIKELY (pos1 >= Block::size))
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
						if (UNLIKELY
						    (out_buf_pos >=
						     Block::size))
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
								(*buf2)
								[pos2++];
							if (UNLIKELY
							    (pos2 >=
							     Block::size))
							{
								pos2 = 0;
								prefetcher.
									block_consumed
									(buf2);
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

			if (UNLIKELY (pos2 >= Block::size))
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
						if (UNLIKELY
						    (out_buf_pos >=
						     Block::size))
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
							     Block::size))
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

		if (UNLIKELY (out_buf_pos >= Block::size))
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


int
main (int argc, char *argv[])
{
	STXXL_MSG ("Seed: " << ran32State)
		//_n = (1024*1024*1024/(B));
	_n = (off_t (16*1024) * off_t (1024 * 1024) / (B));
	M = ((512) * 1024 * 1024);
	D = config::get_instance ()->disks_number ();

	switch (argc)
	{
	case 3:
		M = atol (argv[2]);
	case 2:
		_n = atoll (argv[1]) / B;
	};

	m = M / B;

	std::cout << "Input:  " << _n * B << " bytes" << std::endl;
	std::cout << "Memory: " << M << " bytes" << std::endl;
	std::cout << "Disks:  " << D << std::endl;
	std::cout << "Memory/BlkSize: " << m << std::endl;
	std::cout << "Record size: " << sizeof (Type) << " bytes" << std::
		endl;



	unsigned i, j;
	config *cfg = config::get_instance ();
	block_manager *mng = block_manager::get_instance ();
	int ndisks = cfg->disks_number ();
	//  int half_disks=cfg->disks_number()/2;
	std::list < BID < B > >input;

	gen_input (input, ndisks);

#ifdef STXXL_IO_STATS
	stats *iostats = stats::get_instance ();
	iostats->reset ();
#endif



	unsigned int m2 = _m / 2;
	unsigned int full_runs = _n / m2;
	unsigned int partial_runs = ((_n % m2) ? 1 : 0);
	unsigned int nruns = full_runs + partial_runs;
	STXXL_MSG ("n=" << _n << " nruns=" << nruns << "=" << full_runs << "+"
		   << partial_runs) double begin =
		stxxl_timestamp (), after_runs_creation, end;

	Run **runs = new Run *[nruns];

	for (i = 0; i < full_runs; i++)
		runs[i] = new Run (m2);


	if (partial_runs)
	{
		unsigned int last_run_size = _n - full_runs * m2;
		runs[i] = new Run (last_run_size);

		mng->new_blocks (InterleavedRuns (nruns, 0, ndisks),
				 RunsToBIDArrayAdaptor2 < Block::raw_size >
				 (runs, 0, nruns, last_run_size),
				 RunsToBIDArrayAdaptor2 < Block::raw_size >
				 (runs, _n, nruns, last_run_size));
	}
	else
		mng->new_blocks (InterleavedRuns (nruns, 0, ndisks),
				 RunsToBIDArrayAdaptor < Block::raw_size >
				 (runs, 0, nruns),
				 RunsToBIDArrayAdaptor < Block::raw_size >
				 (runs, _n, nruns));


	//#define CHECK_ALLOC

#ifdef CHECK_ALLOC
	InterleavedRuns functor (nruns, 0, ndisks);
	for (i = 0; i < nruns; i++)
	{


		for (j = 0; j < m / 2; j++)
		{
			std::cout << "(" << (i * m / 2 +
					     j) << "," << functor (i * m / 2 +
								   j) << ")";
			std::cout << "(" << (*runs[i])[j].bid.storage->
				get_disk_number () << "," << (*runs[i])[j].
				bid.offset << " ";
		}

		std::cout << std::endl;
	}

	return 0;
#endif

	create_runs (input, runs, nruns);

	after_runs_creation = stxxl_timestamp ();

	//#define CHECK_SORTED_RUNS

#ifdef CHECK_SORTED_RUNS
	Block blk;
	request *req;
	for (i = 0; i < nruns; i++)
	{
		for (j = 1; j < runs[i]->size (); j++)
			if ((*runs[i])[j] < (*runs[i])[j - 1])
			{
				std::cerr << "Sort error" << std::endl;
				return -1;
			}

		for (j = 0; j < runs[i]->size (); j++)
		{
			blk.read ((*runs[i])[j].bid, req);
			req->wait ();
			delete req;
			for (int k = 1; k < Block::size; k++)
			{
				if (blk[k].key < blk[k - 1].key)
				{
					for (k = 0; k < Block::size; k++)
						STXXL_MSG (blk[k].key)
							STXXL_MSG
							("Blocks are not sorted ");
					STXXL_MSG ("offset:" << (*runs[i])[j].
						   bid.
						   offset << " disk:" <<
						   (*runs[i])[j].bid.storage->
						   get_disk_number ());
					abort ();
				}
			}
		}
	}
#endif

#define merge_runs merge_runs_lt


	disk_queues::get_instance ()->set_priority_op (disk_queue::WRITE);

	unsigned int full_runsize = m2;
	Run **new_runs;

	while (nruns > 1)
	{
		full_runsize = full_runsize * m;
		unsigned int new_full_runs = _n / full_runsize;
		unsigned int new_partial_runs = ((_n % full_runsize) ? 1 : 0);
		unsigned int new_nruns = new_full_runs + new_partial_runs;

		new_runs = new Run *[new_nruns];

		for (i = 0; i < new_full_runs; i++)
			new_runs[i] = new Run (full_runsize);

		if (nruns - new_full_runs * _m == 1)
		{
			// case where one partial run is left to be sorted
			//      STXXL_MSG("case where one partial run is left to be sorted")
			new_runs[i] =
				new Run (_n - full_runsize * new_full_runs);
			Run *tmp = runs[new_full_runs * _m];
			std::copy (tmp->begin (), tmp->end (),
				   new_runs[i]->begin ());

			mng->new_blocks (InterleavedRuns
					 (new_nruns - 1, 0, ndisks),
					 RunsToBIDArrayAdaptor <
					 Block::raw_size > (new_runs, 0,
							    new_nruns - 1),
					 RunsToBIDArrayAdaptor <
					 Block::raw_size > (new_runs,
							    new_full_runs *
							    full_runsize,
							    new_nruns - 1));


			for (i = 0; i < new_full_runs; i++)
			{
				//      STXXL_MSG("Merge of m ("<< _m <<") runs")
				merge_runs (runs + i * _m, _m,
					    *(new_runs + i));
			}


		}
		else
		{

			//allocate output blocks
			if (new_partial_runs)
			{
				unsigned int last_run_size =
					_n - full_runsize * new_full_runs;
				new_runs[i] = new Run (last_run_size);

				mng->new_blocks (InterleavedRuns
						 (new_nruns, 0, ndisks),
						 RunsToBIDArrayAdaptor2 <
						 Block::raw_size > (new_runs,
								    0,
								    new_nruns,
								    last_run_size),
						 RunsToBIDArrayAdaptor2 <
						 Block::raw_size > (new_runs,
								    _n,
								    new_nruns,
								    last_run_size));
			}
			else
				mng->new_blocks (InterleavedRuns
						 (new_nruns, 0, ndisks),
						 RunsToBIDArrayAdaptor <
						 Block::raw_size > (new_runs,
								    0,
								    new_nruns),
						 RunsToBIDArrayAdaptor <
						 Block::raw_size > (new_runs,
								    _n,
								    new_nruns));


			//      STXXL_MSG("Output runs:" << new_nruns << "=" << new_full_runs << "+" << new_partial_runs)
			for (i = 0; i < new_full_runs; i++)
			{
				//        STXXL_MSG("Merge of m ("<< _m <<") runs")
				merge_runs (runs + i * _m, _m,
					    *(new_runs + i));
			}

			if (new_partial_runs)
			{
				//      STXXL_MSG("Partial merge of "<< (nruns - i*_m) <<" runs")
				merge_runs (runs + i * _m, nruns - i * _m,
					    *(new_runs + i));
			}

		}

		nruns = new_nruns;
		delete[]runs;
		runs = new_runs;

	}

	end = stxxl_timestamp ();

	STXXL_MSG ("Elapsed time:" << end - begin << " sec. Run creation time: " << after_runs_creation - begin)
#ifdef STXXL_IO_STATS
	STXXL_MSG ("reads              : " << iostats->get_reads() )
	STXXL_MSG ("writes             : " << iostats->get_writes() )
	STXXL_MSG ("read time          : "  << iostats->get_read_time() << " s") 
	STXXL_MSG ("write time         : " << iostats->get_write_time() << " s")
	STXXL_MSG ("parallel read time : " << iostats->get_pread_time() << " s")
	STXXL_MSG ("parallel write time: " << iostats->get_pwrite_time() << " s")
	STXXL_MSG ("parallel io time   : " << iostats->get_pio_time() << " s")
#endif
#ifdef COUNT_WAIT_TIME
		STXXL_MSG ("Time in I/O wait   : " << stxxl::wait_time_counter << " s")
#endif
#define FINAL_CHECK
#ifdef FINAL_CHECK
	STXXL_MSG ("Checking order...")
	if (1)
	{
		Block *pblk = new Block;
		Block & blk = *pblk;

		request *req;
		Type lastrecord;
		for (i = 0; i < _n; i++)
		{
			blk.read ((*runs[0])[i].bid, req);
			req->wait ();
			delete req;
			if (i)
			{
				if (blk[0] < lastrecord)
					STXXL_MSG ("bug between blocks " << i - 1 << " and " << i << ", keys " << 
						lastrecord.key << ":" << blk[0].key) 
			}	
						
			for (j = 1; j < Block::size; j++)
			{
				if (blk[j] < blk[j - 1])
				{
					STXXL_MSG("bug: block " << i << " pos " << j << " keys " << blk[j-1].key << ":"<< blk[j].key)
				}
			}
				lastrecord = blk[Block::size - 1];
		}
		delete pblk;
	}
#endif

	delete runs[0];
	delete [] runs;

	return 0;
}
