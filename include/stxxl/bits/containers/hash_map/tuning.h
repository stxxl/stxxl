#ifndef _STXXL_TUNING_H_
#define _STXXL_TUNING_H_

#include <stxxl/mng>

__STXXL_BEGIN_NAMESPACE

namespace hash_map
{


	class tuning
	{
		public:
			size_t prefetch_page_size;	/* see buffered_reader */
			size_t prefetch_pages;		/* see buffered_reader */
			size_t blockcache_size;		/* see block_cache and hash_map */


			//! \brief Returns instance of config
			//! \return pointer to the instance of config
			static tuning *get_instance ();

		private:
			static tuning *instance;
			
			/** set reasonable default values for tuning params */
			tuning() :
				prefetch_page_size(config::get_instance()->disks_number()*2),
				prefetch_pages(2),
				blockcache_size(config::get_instance()->disks_number()*12)
			{ }
	};


	tuning *tuning::instance = NULL;
	tuning *tuning::get_instance ()
	{
		if (!instance)
			instance = new tuning ();

		return instance;
	}



} /* namespace hash_map */

__STXXL_END_NAMESPACE

#endif
