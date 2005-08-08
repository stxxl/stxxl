#ifndef STXXL_MAIN_HEADER
#define STXXL_MAIN_HEADER

#include "common/utils.h"

#include "algo/sort.h"
#include "algo/ksort.h"
#include "algo/scan.h"

#include "containers/vector.h"
#include "containers/stack.h"
#include "containers/priority_queue.h"

#if (__GNUC__==3 && __GNUC_MINOR__ == 4)
#else
#include "containers/map.h" // not compatible with g++ 3.4.x
#endif

#include "containers/queue.h"

#include "stream/stream.h"
#include "stream/sort_stream.h"

#include "common/debug.h"

#include "common/timer.h"

#endif // STXXL_MAIN_HEADER
