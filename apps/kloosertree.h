#ifndef KLOOSERTREE_HEADER
#define KLOOSERTREE_HEADER

/***************************************************************************
 *            kloosertree.h
 *
 *  Thu Oct  3 13:14:27 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


class looser_tree
{
	looser_tree ()
	{
	};
	int logK;
	int k;

	int *entry;
	RunCursor2 *current;
	RunCursor2Cmp cmp;
	int init_winner (int root)
	{
		if (root >= k)
		{
			return root - k;
		}
		else
		{
			int left = init_winner (2 * root);
			int right = init_winner (2 * root + 1);
			//      STXXL_MSG("left: " << left << " right: "<<right);
			if (cmp (current[left], current[right]))
			{
				entry[root] = right;
				return left;
			}
			else
			{
				entry[root] = left;
				return right;
			}
		}
	};
      public:
	looser_tree (PrefetcherWriter * p, int nruns)
	{
		RunCursor2::prefetcher = p;
		int i;
		logK = static_cast < int >(ceil (log (nruns) / log (2.)));	// replace with something smart
		int kReg = k = (1 << logK);
		//    STXXL_MSG("k ="<< kReg << " nruns =" << nruns<< " m="<<_m)
		current = new RunCursor2[kReg];
		entry = new int[(kReg << 1)];
		// init cursors
		for (i = 0; i < nruns; i++)
		{
			current[i].buffer = p->r_block (i);
			current[i].pos = 0;
			//              STXXL_MSG( i << " retrieved: "<< (*current[i].buffer)[0].key )
			entry[kReg + i] = i;
		}


		for (i = nruns; i < kReg; i++)
		{
			current[i].make_inf ();
			entry[kReg + i] = i;
		}

		entry[0] = init_winner (1);

	}
	~looser_tree ()
	{
		delete[]current;
		delete[]entry;
	}

      private:
	template < unsigned LogK > void multi_merge_unrolled (Type * to)
	{
		RunCursor2 *currentE, *winnerE;
		int *regEntry = entry;
		Type *done = to + Block::size;
		int winnerIndex = entry[0];

		while (LIKELY (to < done))
		{
			winnerE = current + winnerIndex;
			*(to++) = winnerE->current ();

			(*winnerE)++;

			//#pragma warning(pop)

#define TreeStep(L)									\
      if (LogK >= L )									\
      {											\
	currentE = current + regEntry[ (winnerIndex+(1<<LogK)) >> ((LogK-L)+1) ];	\
	if( cmp(*currentE,*winnerE) )							\
	{										\
	  std::swap(regEntry[(winnerIndex+(1<<LogK)) >> ((LogK-L)+1)],winnerIndex);	\
	  winnerE = currentE;								\
	}										\
      }


			TreeStep (10);
			TreeStep (9);
			TreeStep (8);
			TreeStep (7);
			TreeStep (6);
			TreeStep (5);
			TreeStep (4);
			TreeStep (3);
			TreeStep (2);
			TreeStep (1);

			//#pragma warning(push)

#undef TreeStep

		}

		regEntry[0] = winnerIndex;
	};

	void multi_merge_k (Type * to)
	{
		RunCursor2 *currentE, *winnerE;
		int kReg = k;
		Type *done = to + Block::size;
		int winnerIndex = entry[0];

		while (LIKELY (to < done))
		{
			winnerE = current + winnerIndex;
			*(to++) = winnerE->current ();

			(*winnerE)++;


			for (int i = (winnerIndex + kReg) >> 1; i > 0;
			     i >>= 1)
			{
				currentE = current + entry[i];

				if (cmp (*currentE, *winnerE))
				{
					std::swap (entry[i], winnerIndex);
					winnerE = currentE;
				}
			}
		}

		entry[0] = winnerIndex;
	};
      public:
	void multi_merge (Type * to)
	{
		switch (logK)
		{
		case 2:
			multi_merge_unrolled < 2 > (to);
			break;
		case 3:
			multi_merge_unrolled < 3 > (to);
			break;
		case 4:
			multi_merge_unrolled < 4 > (to);
			break;
		case 5:
			multi_merge_unrolled < 5 > (to);
			break;
		case 6:
			multi_merge_unrolled < 6 > (to);
			break;
		case 7:
			multi_merge_unrolled < 7 > (to);
			break;
		case 8:
			multi_merge_unrolled < 8 > (to);
			break;
		case 9:
			multi_merge_unrolled < 9 > (to);
			break;
		case 10:
			multi_merge_unrolled < 10 > (to);
			break;
		default:
			multi_merge_k (to);
			break;
		}
	}

};


#endif
