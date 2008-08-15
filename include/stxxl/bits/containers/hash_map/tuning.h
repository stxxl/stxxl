#ifndef _STXXL_TUNING_H_
#define _STXXL_TUNING_H_

#include <stxxl/mng>
#include <stxxl/bits/singleton.h>


__STXXL_BEGIN_NAMESPACE

namespace hash_map
{
	class tuning : public singleton<tuning>
	{
		friend class singleton<tuning>;
		public:
			size_t prefetch_page_size;	/* see buffered_reader */
			size_t prefetch_pages;		/* see buffered_reader */
			size_t blockcache_size;		/* see block_cache and hash_map */

		private:
			/** set reasonable default values for tuning params */
			tuning() :
				prefetch_page_size(config::get_instance()->disks_number()*2),
				prefetch_pages(2),
				blockcache_size(config::get_instance()->disks_number()*12)
			{ }
	};

} /* namespace hash_map */

__STXXL_END_NAMESPACE

#endif
