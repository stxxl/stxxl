#ifndef STXXL_VECTOR_HEADER
#define STXXL_VECTOR_HEADER

/***************************************************************************
 *            vector.h
 *
 *  Sat Aug 24 23:54:35 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#include "../mng/mng.h"
#include "../common/tmeta.h"
#include "pager.h"
#include <vector>



namespace stxxl
{
//! \weakgroup stlcont Containers
//! \ingroup stllayer
//! Containers with STL-compatible interface

	
//! \weakgroup stlcontinternals Internals
//! \ingroup stlcont
//! \{
  
	template < unsigned BlkSize_ >
		class bid_vector:public std::vector < BID < BlkSize_ > >
	{
	public:
		enum
		{ block_size = BlkSize_ };
		typedef bid_vector < block_size > _Self;
		typedef std::vector < BID < BlkSize_ > >_Derived;
		typedef unsigned size_type;

		bid_vector (size_type _sz):_Derived (_sz)
		{
		}
	};
  

	template < 
    typename Tp_,
    unsigned PgSz_,
    typename PgTp_,
    unsigned BlkSize_ ,
		typename AllocStr_ ,
    typename SzTp_ >
	class vector;
	
	
	template < typename Tp_, typename AllocStr_, typename SzTp_, typename DiffTp_,
			unsigned BlkSize_,typename PgTp_,unsigned PgSz_ > class const_vector_iterator;

	
	//! \brief External vector iterator, model of \c ext_random_access_iterator concept
	template < typename Tp_, typename AllocStr_, typename SzTp_,typename DiffTp_,
		unsigned BlkSize_, typename PgTp_, unsigned PgSz_ > 
  class vector_iterator
	{
		typedef vector_iterator < Tp_, AllocStr_, SzTp_,DiffTp_,
			BlkSize_,PgTp_,PgSz_ > _Self;
		typedef const_vector_iterator < Tp_, AllocStr_, SzTp_,DiffTp_,
			BlkSize_,PgTp_,PgSz_ > _CIterator;
		
		friend class const_vector_iterator < Tp_, AllocStr_, SzTp_,DiffTp_, BlkSize_,PgTp_,PgSz_>;
public:
		typedef _CIterator const_iterator;
		typedef _Self iterator;

		typedef SzTp_ size_type;
		typedef DiffTp_ difference_type;
		typedef unsigned block_offset_type;
		typedef vector < Tp_, PgSz_, PgTp_, BlkSize_, AllocStr_,SzTp_> vector_type;
		friend class vector < Tp_, PgSz_, PgTp_, BlkSize_, AllocStr_,SzTp_>;
		typedef bid_vector < BlkSize_ > bids_container_type;
		typedef typename bids_container_type::iterator bids_container_iterator;
		typedef typed_block<BlkSize_, Tp_> block_type;
		typedef BID< BlkSize_ > bid_type;
	
		typedef std::random_access_iterator_tag iterator_category;
		typedef typename vector_type::value_type value_type;
		typedef typename vector_type::reference reference;
		typedef typename vector_type::const_reference const_reference;
		typedef typename vector_type::pointer pointer;
		typedef typename vector_type::const_pointer const_pointer;
		
		enum{ block_size = BlkSize_ };

	protected:
		size_type offset;
		vector_type *p_vector;
	private:
		  vector_iterator (vector_type * v, size_type o):offset (o),
			p_vector (v)
		{
		}
public:
	    vector_iterator ():offset (0), p_vector (NULL){}
		vector_iterator (const _Self & a):
			offset (a.offset),
			p_vector (a.p_vector) {}
				
		vector_iterator (const const_iterator & a):
			offset (a.offset),
			p_vector (a.p_vector) {}
				
		block_offset_type block_offset () const
		{
			return static_cast < block_offset_type >
				(offset % block_type::size);
		}
		bids_container_iterator bid () const
		{
			return p_vector->bid (offset);
		}
		
		difference_type operator - (const _Self & a) const
		{
			return offset - a.offset;
		}
		
		_Self operator - (size_type op) const
		{
			return _Self(p_vector,offset - op);
		}
		
		_Self operator + (size_type op) const
		{
			return _Self(p_vector,offset + op);
		}
		
		reference operator *()
		{
			return p_vector->element(offset);
		}
    
    	const_reference operator *() const
		{
			return p_vector->const_element(offset);
		}
		
		void touch()
		{
			p_vector->touch(offset);
		}
		
		_Self & operator ++()
		{
			offset++;
			return *this;
		}
		_Self operator ++(int)
		{
			_Self __tmp = *this;
			offset++;
			return __tmp;
		}
		_Self & operator --()
		{
			offset--;
			return *this;
		}
		_Self operator --(int)
		{
			_Self __tmp = *this;
			offset--;
			return __tmp;
		}
		bool operator == (const _Self &a) const
		{
			assert(p_vector == a.p_vector);
			return offset == a.offset;
		}
		bool operator != (const _Self &a) const
		{
			assert(p_vector == a.p_vector);
			return offset != a.offset;
		}	
		bool operator < (const _Self &a) const
		{
			assert(p_vector == a.p_vector);
			return offset < a.offset;
		}
		
		void flush()
		{
			p_vector->flush();
		}
		/*
		std::ostream & operator<< (std::ostream & o) const
		{
			o << "vectorpointer: "  << ((void*)p_vector) <<" offset: "<<offset;
			return o;
		}*/
		
	};

	//! \brief Const external vector iterator, model of \c ext_random_access_iterator concept
	template < typename Tp_, typename AllocStr_, typename SzTp_,typename DiffTp_,
		unsigned BlkSize_, typename PgTp_, unsigned PgSz_ >
	class const_vector_iterator
	{
		typedef const_vector_iterator < Tp_, AllocStr_, SzTp_,DiffTp_,
			BlkSize_,PgTp_,PgSz_ > _Self;
		typedef vector_iterator < Tp_, AllocStr_, SzTp_,DiffTp_,
			BlkSize_,PgTp_,PgSz_ > _NonConstIterator;

		friend class vector_iterator < Tp_, AllocStr_, SzTp_,DiffTp_, BlkSize_,PgTp_,PgSz_ >;
public:
		typedef _Self const_iterator;
		typedef _NonConstIterator iterator;

		typedef SzTp_ size_type;
		typedef DiffTp_ difference_type;
		typedef unsigned block_offset_type;
		typedef vector < Tp_, PgSz_, PgTp_, BlkSize_, AllocStr_,SzTp_> vector_type;
		friend class vector < Tp_, PgSz_, PgTp_, BlkSize_, AllocStr_,SzTp_>;
		typedef bid_vector < BlkSize_ > bids_container_type;
		typedef typename bids_container_type::iterator bids_container_iterator;
		typedef typed_block<BlkSize_, Tp_> block_type;
		typedef BID< BlkSize_ > bid_type;

    typedef std::random_access_iterator_tag iterator_category;
		typedef typename vector_type::value_type value_type;
		typedef typename vector_type::reference reference;
		typedef typename vector_type::const_reference const_reference;
		typedef typename vector_type::pointer pointer;
		typedef typename vector_type::const_pointer const_pointer;
		
		enum { block_size = BlkSize_ };

protected:
		  size_type offset;
		const vector_type *p_vector;
	      private:
		  const_vector_iterator (const vector_type * v, size_type o):offset (o),
			p_vector (v)
		{
		}
public:
	  const_vector_iterator ():offset (0), p_vector (NULL)
		{
		}
		const_vector_iterator (const _Self & a):
			offset (a.offset),
			p_vector (a.p_vector) {}
		
		const_vector_iterator (const iterator & a):
			offset (a.offset),
			p_vector (a.p_vector) {}
				
		block_offset_type block_offset () const
		{
			return static_cast < block_offset_type >
				(offset % block_type::size);
		}
		bids_container_iterator bid () const
		{
			return p_vector->bid (offset);
		}
		
		difference_type operator - (const _Self & a) const
		{
			return offset - a.offset;
		}
		
		_Self operator - (size_type op) const
		{
			return _Self(p_vector,offset - op);
		}
		
		_Self operator + (size_type op) const
		{
			return _Self(p_vector,offset + op);
		}
		
		const_reference operator *() const
		{
			return p_vector->const_element(offset);
		}
		
		void touch()
		{
			p_vector->touch(offset);
		}
		
		_Self & operator ++()
		{
			offset++;
			return *this;
		}
		_Self operator ++(int)
		{
			_Self tmp_ = *this;
			offset++;
			return tmp_;
		}
		_Self & operator --()
		{
			offset--;
			return *this;
		}
		_Self operator --(int)
		{
			_Self __tmp = *this;
			offset--;
			return __tmp;
		}
		bool operator == (const _Self &a) const
		{
			assert(p_vector == a.p_vector);
			return offset == a.offset; // or (offset + stxxl::int64(p_vector))
		}
		bool operator != (const _Self &a) const
		{
			assert(p_vector == a.p_vector);
			return offset != a.offset;
		}	
		bool operator < (const _Self &a) const
		{
			assert(p_vector == a.p_vector);
			return offset < a.offset;
		}	
		void flush()
		{
			p_vector->flush();
		}
		
		std::ostream & operator<< (std::ostream & o) const
		{
			o << "vectorpointer: "  << ((void*)p_vector) <<" offset: "<<offset;
			return o;
		}
	};	
	
	
	//! \brief External vector container
  
  //! For semantics of the methods see documentation of the STL std::vector
  //! Template parameters:
  //!  - \c Tp_ type of contained objects
  //!  - \c PgSz_ number of blocks in a page 
  //!  - \c PgTp_ pager type, \c random_pager<x> or \c lru_pager<x>, where x is number of pages, 
  //!  default is \c lru_pager<8>
  //!  - \c BlkSize_ external block size in bytes, default is 2 Mbytes
  //!  - \c AllocStr_ one of allocation strategies: \c striping , \c RC , \c SR , or \c FR
  //!  default is RC <BR>
  //! Memory consumption: BlkSize_*x*PgSz_ bytes
  //! \warning Do not store references to the elements of an external vector. Such references 
  //! might be invalidated during any following access to elements of the vector
	template < 
    typename Tp_,
    unsigned PgSz_ = 4,
    typename PgTp_ = lru_pager<8>,
		unsigned BlkSize_ = STXXL_DEFAULT_BLOCK_SIZE (Tp_),
    typename AllocStr_ = STXXL_DEFAULT_ALLOC_STRATEGY,
    typename SzTp_ = stxxl::int64
		>
	class vector 
  {
  public:
		typedef Tp_ value_type;
		typedef value_type & reference;
		typedef const value_type & const_reference;
		typedef value_type * pointer;
		typedef SzTp_ size_type;
		typedef SzTp_ difference_type;
		typedef const value_type *const_pointer;
		
		typedef PgTp_ pager_type;
		typedef AllocStr_ alloc_strategy;

		enum{ 
			block_size = BlkSize_, 
			page_size = PgSz_,
			n_pages = pager_type::n_pages,
			on_disk = -1 };

		typedef vector_iterator < value_type, alloc_strategy, size_type,
			difference_type,block_size,pager_type,page_size > iterator;
		friend class vector_iterator < value_type, alloc_strategy, size_type, difference_type,block_size,pager_type,page_size >;
		typedef const_vector_iterator < value_type, alloc_strategy,
			size_type,difference_type, block_size,pager_type,page_size > const_iterator;
		friend class const_vector_iterator < value_type, alloc_strategy, size_type,difference_type, block_size,pager_type,page_size >;

		typedef bid_vector < block_size > bids_container_type;
		typedef typename bids_container_type::
			iterator bids_container_iterator;
		typedef typename bids_container_type::
			const_iterator const_bids_container_iterator;
		
		typedef typed_block<BlkSize_, Tp_> block_type;
		

private:
		alloc_strategy _alloc_strategy;
		size_type _size;
		bids_container_type _bids;
		//bids_container_iterator _bids_finish;
		mutable pager_type pager;

		enum { uninitialized = 1, dirty= 2 };
		mutable std::vector<unsigned char> _page_status;
		mutable std::vector<int> _last_page;
		mutable simple_vector<int> _page_no;
		mutable std::queue<int> _free_pages;
		mutable simple_vector<block_type> _cache;
		file * _from;
		block_manager *bm;
		config *cfg;
		
		size_type size_from_file_length(stxxl::int64 file_length)
		{
			stxxl::int64 blocks_fit = file_length/stxxl::int64(block_type::raw_size);
			size_type cur_size = blocks_fit*stxxl::int64(block_type::size);
			stxxl::int64 rest = file_length - blocks_fit*stxxl::int64(block_type::raw_size);
			return (cur_size + rest/stxxl::int64(sizeof(value_type)));
		}
		stxxl::int64 file_length()
		{
			size_type cur_size = size();
			if(cur_size % size_type(block_type::size))
			{
				stxxl::int64 full_blocks_length = size_type(_bids.size()-1)*size_type(block_type::raw_size);
				size_type rest = cur_size - size_type(_bids.size()-1)*size_type(block_type::size);
				return full_blocks_length + rest*size_type(sizeof(value_type));
			}
			return size_type(_bids.size())*size_type(block_type::raw_size);
		}
public:
		vector (size_type n = 0):
			_size (n),
			_bids (div_and_round_up (n, block_type::size)),
			_page_status(div_and_round_up (_bids.size(), page_size)),
			_last_page(div_and_round_up (_bids.size(), page_size)),
			_page_no(n_pages),
			_cache(n_pages * page_size),
			_from(NULL)
		{
			bm = block_manager::get_instance ();
			cfg = config::get_instance ();
			
			int all_pages = div_and_round_up (_bids.size(), page_size);
			int i=0;
			for(;i<all_pages;++i)
			{
				_page_status[i] = uninitialized;
				_last_page[i] = on_disk;
			}
			
			for(i=0;i<n_pages;++i)
				_free_pages.push(i);
		

			bm->new_blocks (_alloc_strategy, _bids.begin (),
					_bids.end ());
			
		}
		
		void swap(vector & obj)
		{
			std::swap(_alloc_strategy,obj._alloc_strategy); 
			std::swap(_size,obj._size); 
			std::swap(_bids,obj._bids); 
			std::swap(pager,obj.pager); 
			std::swap(_page_status,obj._page_status); 
			std::swap(_last_page,obj._last_page); 
			std::swap(_page_no,obj._page_no); 
			std::swap(_free_pages,obj._free_pages); 
			std::swap(_cache,obj._cache);
		}
    size_type capacity() const
    {
      return size_type(_bids.size())*block_type::size;
    }
    void reserve(size_type n)
    {
      if(n<=capacity())
        return;
      
      unsigned old_bids_size = _bids.size();
      unsigned new_bids_size = div_and_round_up (n, block_type::size);
      unsigned new_pages = div_and_round_up(new_bids_size, page_size); 
      _page_status.resize(new_pages,uninitialized);
      _last_page.resize(new_pages,on_disk);
      
      _bids.resize(new_bids_size);
	  if(_from == NULL)
      		bm->new_blocks(offset_allocator<alloc_strategy>(old_bids_size,_alloc_strategy),
          		_bids.begin() + old_bids_size,_bids.end());
	  else
	  {
		  size_type offset = size_type(old_bids_size)*size_type(block_type::raw_size); 
		  bids_container_iterator it = _bids.begin() + old_bids_size;
		  for(;it!=_bids.end();++it,offset+=size_type(block_type::raw_size) )
		  {
			  (*it).storage = _from;
			  (*it).offset = offset;
		  }
		  STXXL_VERBOSE1("vector::reserve(): Changing size of file "<<((void *)_from)<<" to "
					<<offset);
		  _from->set_size(offset);
	  }
    }
    void resize(size_type n)
    {
	  reserve(n);		
	  /*
      unsigned old_bids_size = _bids.size();
      unsigned new_bids_size = div_and_round_up (n, block_type::size);
      
      
	  if(new_bids_size > old_bids_size)
      {
        reserve(n);
      }
      else if(new_bids_size < old_bids_size)
      {
        bm->delete_blocks(_bids.begin() + new_bids_size,_bids.end());
        _bids.resize(new_bids_size);
        
        unsigned first_page_to_evict = div_and_round_up(new_bids_size, page_size);
        std::fill(_page_status.begin() + first_page_to_evict,
                  _page_status.end(), 0); // clear dirty flag, so this pages 
                                          // will be never written
      }*/
      _size = n;
    }
	void clear()
	{	
		_size = 0;
		bm->delete_blocks(_bids.begin(),_bids.end());
		
        _bids.clear();
		_page_status.clear();
		_last_page.clear();
		while(!_free_pages.empty())
			_free_pages.pop();
		
		for(int i=0;i<n_pages;++i)
			_free_pages.push(i);
		
	}
    void push_back(const_reference obj)
    {
      size_type old_size = _size;
      resize(old_size + 1);
      element(old_size) = obj;
    }
    void pop_back()
    {
      resize(_size - 1);
    }
    reference back()
    {
      return element(_size - 1);
    }
    reference front()
    {
      return element(0);
    }
	const_reference back() const
    {
      return const_element(_size - 1);
    }
    const_reference front() const
    {
      return const_element(0);
    }
	//! \brief Construct vector from a file
	//! \param from file to be constructed from
	//! \warning Only one \c vector can be assigned to a file 
	//! (does not matter whether the files are different \c file objects).
    vector (file * from):
			_size(size_from_file_length(from->size())),
			_bids(div_and_round_up(_size,size_type(block_type::size))),
			_page_status(div_and_round_up (_bids.size(), page_size)),
			_last_page(div_and_round_up (_bids.size(), page_size)),
			_page_no(n_pages),
			_cache(n_pages * page_size),
			_from(from)
		{
			// initialize from file
			assert(from->get_disk_number() == -1);
			
			bm = block_manager::get_instance ();
			cfg = config::get_instance ();
			
			int all_pages = div_and_round_up (_bids.size(), page_size);
			int i=0;
			for(;i<all_pages;++i)
			{
				_page_status[i] = 0;
				_last_page[i] = on_disk;
			}
			
			for(i=0;i<n_pages;++i)
				_free_pages.push(i);
		
			
			size_type offset = 0; 
			bids_container_iterator it = _bids.begin();
			for(;it!=_bids.end(); ++it, offset+=size_type(block_type::raw_size) )
			{
				(*it).storage = from;
				(*it).offset = offset;
			}
			from->set_size(offset);
		}
  private:
    vector(const vector & obj) // Copying external vectors is discouraged
                               // (and not implemented)
    {
      STXXL_ERRMSG("stxxl::vector copy constructor is not implemented yet");
      abort();
    }
	
	vector & operator = (const vector & obj); // Copying external vectors is discouraged
                                              // (and not implemented)
	
  public:
		size_type size () const
		{
			return _size;
		}
		bool empty() const
		{
			return (!_size);
		}
		iterator begin ()
		{
			return iterator (this, 0);
		}
		const_iterator begin () const
		{
			return const_iterator (this, 0);
		}
		iterator end ()
		{
			return iterator (this, _size);
		}
		const_iterator end () const
		{
			return const_iterator (this, _size);
		}
		reference operator [] (size_type offset)
		{
			return element(offset);
		}
		const_reference operator [] (size_type offset) const
		{
			return const_element(offset);
		}
		
		void flush() const
		{
			simple_vector<bool> non_free_pages(n_pages);
			int i=0;
			for(;i<n_pages;i++)
				non_free_pages[i] = true;
			
			while(!_free_pages.empty())
			{
				non_free_pages[_free_pages.front()] = false;
				_free_pages.pop();
			}
				
			for(i=0;i<n_pages;i++)
			{
				_free_pages.push(i);
				int page_no = _page_no[i];
				if(non_free_pages[i])
				{
					STXXL_VERBOSE1("vector: flushing page "<<i<< " address: "<<(int64(page_no)*
						int64(block_type::size)*int64(page_size)));
					if(_page_status[page_no] & dirty)
						write_page(page_no,i);
				
					_last_page[page_no] = on_disk;
					_page_status[page_no] = 0;
				}
			};
		}
		~vector()
		{
			flush();
			bm->delete_blocks(_bids.begin(),_bids.end());
			if(_from) // file must be truncated
			{
				STXXL_VERBOSE1("~vector(): Changing size of file "<<((void *)_from)<<" to "
					<<file_length());
				STXXL_VERBOSE1("~vector(): size of the vector is "<<size())
				_from->set_size(file_length());
			}
		}
private:
		bids_container_iterator bid (const size_type & offset)
		{
			return (_bids.begin () +
				static_cast < typename bids_container_type::size_type >
				(offset / block_type::size));
		}
		const_bids_container_iterator bid (const size_type & offset) const
		{
			return (_bids.begin () +
				static_cast < typename bids_container_type::size_type >
				(offset / block_type::size));
		}
		void read_page(int page_no, int cache_page) const
		{
			STXXL_VERBOSE1("vector: reading page_no="<<page_no<<" cache_page="<<cache_page)
			request_ptr * reqs = new request_ptr [page_size];
			int block_no = page_no*page_size;
			int last_block = STXXL_MIN(block_no + page_size,int(_bids.size()));
			int i=cache_page*page_size,j=0;
			for(;block_no < last_block; ++block_no,++i,++j)
				reqs[j] = _cache[i].read(_bids[block_no]);
			wait_all(reqs,last_block - page_no*page_size);
			delete [] reqs;
		}
		void write_page(int page_no, int cache_page) const
		{
			STXXL_VERBOSE1("vector: writing page_no="<<page_no<<" cache_page="<<cache_page)
			request_ptr * reqs = new request_ptr [page_size];
			int block_no = page_no*page_size;
			int last_block = STXXL_MIN(block_no + page_size,int(_bids.size()));
			int i=cache_page*page_size,j=0;
			for(;block_no < last_block; ++block_no,++i,++j)
			{
				reqs[j] = _cache[i].write(_bids[block_no]);
			}
			wait_all(reqs,last_block - page_no*page_size);
			delete [] reqs;
		}
		reference element(size_type offset)
		{
			int page_no = offset/(block_type::size*page_size);
			int page_offset = offset % (block_type::size*page_size);
			assert(page_no < int(_last_page.size()) ); // fails if offset is too large, out of bound access
			int last_page = _last_page[page_no];
			if(last_page < 0)  // == on_disk
			{ 
				if(_free_pages.empty()) // has to kick
				{
					int kicked_page = pager.kick();
					pager.hit(kicked_page);
					int old_page_no = _page_no[kicked_page];
					_last_page[page_no] = kicked_page;
					_last_page[old_page_no] = on_disk;
					_page_no[kicked_page] = page_no;
								
					// what to do with the old page ?
					if(_page_status[old_page_no] & dirty)
					{
						// has to store changes
						write_page(old_page_no,kicked_page);
					}
					
					if(_page_status[page_no] != uninitialized)
					{
						read_page(page_no,kicked_page);
					}
					
					_page_status[page_no] = dirty;
					
					return _cache[kicked_page*page_size + page_offset/block_type::size][page_offset % block_type::size];
				}
				else
				{
					int free_page = _free_pages.front();
					_free_pages.pop();
					pager.hit(free_page);
					_last_page[page_no] = free_page;
					_page_no[free_page] = page_no;
					
					if(_page_status[page_no] != uninitialized)
					{
						read_page(page_no,free_page);
					}
					
					_page_status[page_no] = dirty;
					
					return _cache[free_page*page_size + page_offset/block_type::size][page_offset % block_type::size];
				}
			} 
			else
			{
				_page_status[page_no] = dirty;
				pager.hit(last_page);
				return _cache[last_page*page_size + page_offset/block_type::size][page_offset % block_type::size];
			}
		};
		void touch(size_type offset) const
		{
			// fails if offset is too large, out of bound access
			assert(offset/(block_type::size*page_size) < _page_status.size() );
			_page_status[offset/(block_type::size*page_size)] = 0;
		}
		const_reference const_element(size_type offset) const
		{
			int page_no = offset/(block_type::size*page_size);
			int page_offset = offset % (block_type::size*page_size);
			assert(page_no < int(_last_page.size()) ); // fails if offset is too large, out of bound access
			int last_page = _last_page[page_no];
			if(last_page < 0)  // == on_disk
			{ 
				if(_free_pages.empty()) // has to kick
				{
					int kicked_page = pager.kick();
					pager.hit(kicked_page);
					int old_page_no = _page_no[kicked_page];
					_last_page[page_no] = kicked_page;
					_last_page[old_page_no] = on_disk;
					_page_no[kicked_page] = page_no;
								
					// what to do with the old page ?
					if(_page_status[old_page_no] & dirty)
					{
						// has to store changes
						write_page(old_page_no,kicked_page);
					}
					
					if(_page_status[page_no] != uninitialized)
					{
						read_page(page_no,kicked_page);
					}
					
					_page_status[page_no] = 0;
					
					return _cache[kicked_page*page_size + page_offset/block_type::size][page_offset % block_type::size];
				}
				else
				{
					int free_page = _free_pages.front();
					_free_pages.pop();
					pager.hit(free_page);
					_last_page[page_no] = free_page;
					_page_no[free_page] = page_no;
					
					if(_page_status[page_no] != uninitialized)
					{
						read_page(page_no,free_page);
					}
					
					_page_status[page_no] = 0;
					
					return _cache[free_page*page_size + page_offset/block_type::size][page_offset % block_type::size];
				}
			} 
			else
			{
				pager.hit(last_page);
				return _cache[last_page*page_size + page_offset/block_type::size][page_offset % block_type::size];
			}
		};
	};
  
  //! \}
  
  //! \addtogroup stlcont
  //! \{
  
	//! \brief External vector type generator
  
  //! Parameters:
  //!  - \c Tp_ type of contained objects
  //!  - \c PgSz_ number of blocks in a page
  //!  - \c Pages_ number of pages
  //!  - \c BlkSize_ external block size in bytes, default is 2 Mbytes
  //!  - \c AllocStr_ one of allocation strategies: \c striping , \c RC , \c SR , or \c FR
  //!  default is RC 
  //!  - \c Pager_ pager type:
  //!    - \c random ,
  //!    - \c lru , is default
  //!
  //! Memory consumption of constructed vector is BlkSize_*Pages_*PgSz_ bytes
  //! Configured vector type is available as \c STACK_GENERATOR<>::result. <BR> <BR>
  //! Examples:
  //!    - \c VECTOR_GENERATOR<double>::result external vector of \c double's ,
  //!    - \c VECTOR_GENERATOR<double,8>::result external vector of \c double's ,
  //!      with 8 blocks per page,
  //!    - \c VECTOR_GENERATOR<double,8,2,512*1024,RC,lru>::result external vector of \c double's ,
  //!      with 8 blocks per page, 2 pages, 512 KB blocks, Random Cyclic allocation
  //!      and lru cache replacement strategy
  //! \warning Do not store references to the elements of an external vector. Such references 
  //! might be invalidated during any following access to elements of the vector
	template
  < 
    typename Tp_,
    unsigned PgSz_ = 4,
    unsigned Pages_ = 8,
    unsigned BlkSize_ = STXXL_DEFAULT_BLOCK_SIZE (Tp_),
		typename AllocStr_ = STXXL_DEFAULT_ALLOC_STRATEGY,
    pager_type Pager_ = lru
	>
  struct VECTOR_GENERATOR
  {
      typedef typename IF<Pager_==lru,
        lru_pager<Pages_>,random_pager<Pages_> >::result PagerType;
    
      typedef vector<Tp_,PgSz_,PagerType,BlkSize_,AllocStr_> result;
  };
    
    
  //! \}
}



namespace std
{
	template < 
    	typename Tp_,
    	unsigned PgSz_,
    	typename PgTp_,
    	unsigned BlkSize_ ,
		typename AllocStr_ ,
    	typename SzTp_ >
	void swap(	stxxl::vector<Tp_,PgSz_,PgTp_,BlkSize_,AllocStr_,SzTp_> & a,
				stxxl::vector<Tp_,PgSz_,PgTp_,BlkSize_,AllocStr_,SzTp_> & b )
	{
		a.swap(b);
	}
}


#endif
