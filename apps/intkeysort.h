#ifndef HEADER_INTKEYSORT
#define HEADER_INTKEYSORT

#include <vector>
#include <algorithm>

struct type_key
{
	Type::key_type key;
	int index;
	  type_key (Type::key_type k, int i):key (k), index (i)
	{
	};
};

struct type_key_cmp
{
	bool operator  () (const type_key & a, const type_key & b) const
	{
		return a.key < b.key;
	}
};


void
distribute (Type * array,
	    const unsigned array_size,
	    std::vector < type_key > *buckets, const int hbits)
{
	unsigned shift_amount = 8 * sizeof (Type::key_type) - hbits;
	for (unsigned i = 0; i < array_size; i++)	// distribute
	{
		register Type::key_type key = array[i].key ();
		buckets[key >> shift_amount].push_back (type_key (key, i));
	};
}

void
build_inv_index (std::vector < type_key > *buckets,
		 const unsigned n_buckets, unsigned *out_pos, int *inv_index)
{
	for (unsigned i = 0; i < n_buckets; i++)	// build inverse index
	{
		std::vector < type_key > &cur_bucket = buckets[i];

		for (unsigned j = 0; j < cur_bucket.size (); j++)
		{
			inv_index[cur_bucket[j].index] = out_pos[i] + j;
		}
	}
}

void
do_permutation (Type * array, const unsigned array_size, int *inv_index)
{
	unsigned next_cycle = 0;
	unsigned next_index = 0;
	Type cur = array[0];
	for (;;)		// do permutation
	{
		int next_inv_index = inv_index[next_index];
		if (next_inv_index >= 0)
		{
			std::swap (array[next_inv_index], cur);
			inv_index[next_index] = -1;
			next_index = next_inv_index;
		}
		else
		{
			while (inv_index[next_cycle] < 0)
			{
				next_cycle++;

				if (next_cycle > array_size)
					return;	// all cycles are done
			}

			next_index = next_cycle;
			cur = array[next_cycle];
		}
	}
}

 // array      - input
 // array_size - input size in elements
 // hbits      - how many bits are used as bucket address
 // R          - defines initial bucket size = R * (array_size<<hbits)
void
int_keysort (Type * array,
	     const unsigned array_size, const int hbits, const double R)
{
	unsigned n_buckets = 1 << hbits;	// number of buckets;
	std::vector < type_key > *buckets =
		new std::vector < type_key >[n_buckets];
	unsigned i = 0;
	for (i = 0; i < n_buckets; i++)
		buckets[i].
			reserve (size_t
				 (R * double (array_size) /
				  double (n_buckets)));

	distribute (array, array_size, buckets, hbits);

	unsigned *out_pos = new unsigned[n_buckets];	// starting out positions of buckets, required for permutation
	out_pos[0] = 0;

	//STXXL_MSG("out_pos[0]=0, bucket size = " << buckets[0].size())
	for (i = 1; i < n_buckets; i++)
	{
		out_pos[i] = out_pos[i - 1] + buckets[i - 1].size ();
		//STXXL_MSG("out_pos["<< i<<"]="<< out_pos[i] << ", bucket size = " << buckets[i].size())
	}

	for (i = 0; i < n_buckets; i++)
	{
		std::sort (buckets[i].begin (), buckets[i].end (),
			   type_key_cmp ());
	}

	int *inv_index = new int[array_size];

	build_inv_index (buckets, n_buckets, out_pos, inv_index);

	delete[]out_pos;
	delete[]buckets;

	do_permutation (array, array_size, inv_index);

	delete[]inv_index;

}


#endif
