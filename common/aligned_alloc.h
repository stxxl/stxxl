#ifndef ALIGNED_ALLOC
#define ALIGNED_ALLOC

/***************************************************************************
 *            aligned_alloc.h
 *
 *  Sat Aug 24 23:53:12 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#include "../common/utils.h"

#ifdef STXXL_USE_POSIX_MEMALIGN_ALLOC
#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <malloc.h>
#endif


__STXXL_BEGIN_NAMESPACE 

#ifdef STXXL_USE_POSIX_MEMALIGN_ALLOC

template < size_t ALIGNMENT > 
inline void * aligned_alloc (size_t size)
{
	void * result;
	STXXL_VERBOSE1("stxxl::aligned_alloc<"<<ALIGNMENT <<
		">() POSIX VERSION asking for "<<size<<" bytes");
	int status = posix_memalign(&result,ALIGNMENT,size);
	
	if(status != 0)
	{
		// error occured
		switch(status)
		{
			case EINVAL:
				STXXL_ERRMSG("posix_memalign call failed with status code EINVAL")
				break;
			case ENOMEM:
				STXXL_ERRMSG("posix_memalign call failed with status code ENOMEM")
				break;
			default:
				STXXL_ERRMSG("posix_memalign call failed with status code "<<status)
		}
		abort();
	}
	STXXL_VERBOSE1("stxxl::aligned_alloc<"<<ALIGNMENT <<
		">() POSIX VERSION asking for "<<size<<
	    " bytes,  returning block starting at "<<result);
	
	return result;
};

template < size_t ALIGNMENT > inline void
aligned_dealloc (void *ptr)
{
	STXXL_VERBOSE1("stxxl::aligned_dealloc<"<<ALIGNMENT <<">() POSIX VERSION, ptr = "<<ptr) 
	free(ptr);
};

#else

template < size_t ALIGNMENT > 
inline void * aligned_alloc (size_t size)
{
	STXXL_VERBOSE1("stxxl::aligned_alloc<"<<ALIGNMENT <<">(), size = "<<size) 
    // TODO: use posix_memalign() and free() on Linux
	char *buffer = new char[size + ALIGNMENT];
	char *result = buffer + ALIGNMENT - (((unsigned long) buffer) % (ALIGNMENT));
	STXXL_VERBOSE1("stxxl::aligned_alloc<"<<ALIGNMENT <<">() address "<<long(result)
		<< " lost "<<unsigned(result-buffer)<<" bytes")
	assert( unsigned(result-buffer) >= sizeof(char*) );
	*(((char **) result) - 1) = buffer;
	STXXL_VERBOSE1("stxxl::aligned_alloc<"<<ALIGNMENT <<
		">(), allocated at "<<std::hex <<((unsigned long)buffer)<<" returning "<< ((unsigned long)result)
		<<std::dec) 
	//abort();
	
	return result;
};

template < size_t ALIGNMENT > inline void
aligned_dealloc (void *ptr)
{
	STXXL_VERBOSE2("stxxl::aligned_dealloc(<"<<ALIGNMENT <<">), ptr = "<<long(ptr)) 
	delete[] * (((char **) ptr) - 1);
};

#endif

__STXXL_END_NAMESPACE
#endif
