#ifndef PREFETCHER_HEADER
#define PREFETCHER_HEADER

/***************************************************************************
 *            prefetcher.h
 *
 *  Sat Aug 24 23:53:05 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


class PrefetcherWriter
{
	PrefetcherWriter ()
	{
	};
      protected:
	Run * seq;
	Run *out;
	unsigned nextread;
	unsigned nextwrite;
	const int nreadblocks;
	const int nwriteblocks;
	Block *read_buffers;
	Block *write_buffers;
	int *write_bids;	// bids (pos in out) of blocks in write_buffers
	request **read_reqs;
	request **write_reqs;
	const unsigned writebatchsize;

	std::vector < int >free_write_blocks;	// contains free write blocks
	std::vector < int >busy_write_blocks;	// blocks that are in writing, notice that if block is not in free_
	// an not in busy then block is not yet filled

	struct batch_entry
	{
		off_t offset;
		int ibuffer;
		  batch_entry (off_t o, int b):offset (o), ibuffer (b)
		{
		};
	};
	struct batch_entry_cmp
	{
		bool operator () (const batch_entry & a,
				  const batch_entry & b) const
		{
			return (a.offset > b.offset);
		};
	};

	//  typedef std::multimap<off_t   , int > batch_type; //  TODO: replace to priority queue
	//                    disk pos, block number in write_buffers
	typedef std::priority_queue < batch_entry,
		std::vector < batch_entry >, batch_entry_cmp > batch_type;

	batch_type batch_write_blocks;	// sorted sequence of blocks to write


	std::queue < int >in_prefetching;	// buffers in prefetching

      public:
	PrefetcherWriter (Run * s, Run * o, int nruns, int prefetch_buf_size,
			  int write_buf_size, int write_batch_size):seq (s),
		out (o), nextread (nruns + prefetch_buf_size), nextwrite (0),
		nreadblocks (nextread),
		nwriteblocks ((write_buf_size > 2) ? write_buf_size : 2),
		writebatchsize (write_batch_size ? write_batch_size : 1)
	{
		read_buffers = new Block[nreadblocks];
		read_reqs = new request *[nreadblocks];
		int i;
		for (i = 0; i < nreadblocks; i++)
		{
			read_buffers[i].read ((*seq)[i].bid, read_reqs[i]);
			//      STXXL_MSG(" scheduled for read: "<< (*seq)[i].value.key )
		}

		for (i = nruns; i < nreadblocks; i++)
			in_prefetching.push (i);

		write_buffers = new Block[nwriteblocks];
		write_reqs = new request *[nwriteblocks];
		write_bids = new int[nwriteblocks];

		for (i = 0; i < nwriteblocks; i++)
			free_write_blocks.push_back (i);

		mc::waitdel_all (read_reqs, nruns);

	};
	Block *r_block (int i)
	{
		return (read_buffers + i);
	};
	Block *w_block (int i)
	{
		return (write_buffers + i);
	};

	void wait_read_completion (int ibuffer)
	{
		read_reqs[ibuffer]->wait ();
		delete read_reqs[ibuffer];
	};

	bool block_consumed (RunCursor & cursor)	// return false if seq emptied
	{
		if (nextread < seq->size ())
		{
			int ibuffer = cursor.buffer - read_buffers;
			read_buffers[ibuffer].read ((*seq)[nextread++].bid,
						    read_reqs[ibuffer]);
			in_prefetching.push (ibuffer);
		}

		if (in_prefetching.empty ())
			return false;

		int next_block = in_prefetching.front ();
		in_prefetching.pop ();

		cursor.buffer = read_buffers + next_block;
		cursor.pos = 0;

		wait_read_completion (next_block);

		return true;
	}

	void block_consumed (RunCursor2 & cursor)	// return false if seq emptied
	{
		//    STXXL_MSG("consumed "<< cursor.buffer - read_buffers )
		if (nextread < seq->size ())
		{
			int ibuffer = cursor.buffer - read_buffers;
			//      STXXL_MSG("consumed "<< cursor.buffer - read_buffers << " scheduled for read: "<< (*seq)[nextread].value.key )
			read_buffers[ibuffer].read ((*seq)[nextread++].bid,
						    read_reqs[ibuffer]);
			in_prefetching.push (ibuffer);
		}

		if (!in_prefetching.empty ())
		{
			int next_block = in_prefetching.front ();
			in_prefetching.pop ();

			cursor.buffer = read_buffers + next_block;
			cursor.pos = 0;

			wait_read_completion (next_block);
			//       STXXL_MSG( next_block << " retrieved: "<< (*cursor.buffer)[0].key )
		}
	}


	bool block_consumed (Block * &buffer)	// return false if seq emptied
	{
		if (nextread < seq->size ())
		{
			int ibuffer = buffer - read_buffers;
			read_buffers[ibuffer].read ((*seq)[nextread++].bid,
						    read_reqs[ibuffer]);
			in_prefetching.push (ibuffer);
		}

		if (in_prefetching.empty ())
			return false;

		int next_block = in_prefetching.front ();
		in_prefetching.pop ();
		buffer = read_buffers + next_block;
		wait_read_completion (next_block);

		return true;
	}

	int get_free_write_block ()
	{
		int ibuffer;
		for (std::vector < int >::iterator it =
		     busy_write_blocks.begin ();
		     it != busy_write_blocks.end (); it++)
		{
			if (write_reqs[ibuffer = (*it)]->poll ())
			{
				delete write_reqs[ibuffer];
				busy_write_blocks.erase (it);
				free_write_blocks.push_back (ibuffer);

				break;
			}
		}
		if (UNLIKELY (free_write_blocks.empty ()))
		{
			//      STXXL_MSG( "Out of output buffers, waiting (busy:"<<busy_write_blocks.size()<<" all:"<< nwriteblocks <<")" )

			int size = busy_write_blocks.size ();
			request **reqs = new request *[size];
			int i = 0;
			for (; i < size; i++)
			{
				reqs[i] = write_reqs[busy_write_blocks[i]];
			}
			int completed = mc::wait_any (reqs, size);
			//      STXXL_MSG( "Out of output buffers, block finished" )
			//      assert(completed >= 0 && completed < size );
			int completed_global = busy_write_blocks[completed];
			delete[]reqs;
			busy_write_blocks.erase (busy_write_blocks.begin () +
						 completed);
			delete write_reqs[completed_global];

			write_bids[completed_global] = nextwrite++;

			return completed_global;
		}
		ibuffer = free_write_blocks.back ();
		free_write_blocks.pop_back ();

		write_bids[ibuffer] = nextwrite++;

		//    STXXL_MSG("Block "<<ibuffer << " given")

		return ibuffer;
	}
	/*
	 * int block_filled(int filled_block) // writes filled_block and returns new block 
	 * {
	 * int new_block = get_free_write_block();
	 * 
	 * #ifdef CHECK_ORDER_IN_BLOCKS
	 * Block * buffer = w_block(filled_block);
	 * for(int i=1;i<Block::size;i++)
	 * if( (*buffer)[i].key < (*buffer)[i-1].key )
	 * {
	 * STXXL_MSG("!!! " << i)
	 * int error_pos = i;
	 * for(i= error_pos -2; i < error_pos + 2;i++)
	 * {
	 * STXXL_MSG((*buffer)[i].key )
	 * }
	 * STXXL_MSG("Order in write block is violated")
	 * STXXL_MSG( filled_block <<" "<< buffer)
	 * //         abort();
	 * }
	 * #endif
	 * 
	 * TriggerEntry & next_trigger = (*out)[ nextwrite++ ];
	 * 
	 * next_trigger.value = write_buffers[filled_block][0];
	 * write_buffers[filled_block].write( next_trigger.bid, write_reqs[filled_block] );
	 * 
	 * busy_write_blocks.push_back(filled_block);
	 * 
	 * // probably check if nextwrite is not greater than size of output run
	 * 
	 * return new_block;
	 * } */

	int block_filled (int filled_block)	// writes filled_block and returns new block 
	{
		//    int new_block = get_free_write_block();
		//    STXXL_MSG("Block filled, write batch size: "<< batch_write_blocks.size())

		if (batch_write_blocks.size () >= writebatchsize)
		{
			// flush batch
			//      STXXL_MSG("Flushing write blocks");
			while (!batch_write_blocks.empty ())
			{
				int ibuffer =
					batch_write_blocks.top ().ibuffer;
				batch_write_blocks.pop ();

				TriggerEntry & next_trigger =
					(*out)[write_bids[ibuffer]];
				next_trigger.key =
					write_buffers[ibuffer].elem->key();
				write_buffers[ibuffer].write (next_trigger.
							      bid,
							      write_reqs
							      [ibuffer]);

				busy_write_blocks.push_back (ibuffer);
			}

		}

		//    STXXL_MSG("Adding write request to batch")
		batch_write_blocks.
			push (batch_entry
			      ((*out)[write_bids[filled_block]].bid.offset,
			       filled_block));


		return get_free_write_block ();
	}


	~PrefetcherWriter ()
	{
		int ibuffer;
		while (!batch_write_blocks.empty ())
		{
			ibuffer = batch_write_blocks.top ().ibuffer;
			batch_write_blocks.pop ();

			TriggerEntry & next_trigger =
				(*out)[write_bids[ibuffer]];
			next_trigger.key = write_buffers[ibuffer].elem->key();
			write_buffers[ibuffer].write (next_trigger.bid,
						      write_reqs[ibuffer]);

			busy_write_blocks.push_back (ibuffer);
		}
		for (std::vector < int >::const_iterator it =
		     busy_write_blocks.begin ();
		     it != busy_write_blocks.end (); it++)
		{
			ibuffer = *it;
			write_reqs[ibuffer]->wait ();
			delete write_reqs[ibuffer];
		}

		delete[]read_buffers;
		delete[]write_buffers;
		delete[]read_reqs;
		delete[]write_reqs;
		delete[]write_bids;

	}
};

#endif
