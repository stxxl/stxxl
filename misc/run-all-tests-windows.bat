:: ###########################################################################
::  misc\run-all-tests-windows.bat
::
::  Part of the STXXL. See http://stxxl.sourceforge.net
::
::  Copyright (C) 2010 Johannes Singler <singler@kit.edu>
::
::  Distributed under the Boost Software License, Version 1.0.
::  (See accompanying file LICENSE_1_0.txt or copy at
::  http://www.boost.org\LICENSE_1_0.txt)
:: ###########################################################################
@echo on
set STXXL_TMPDIR=.

common\stxxl_info
common\test_random
common\test_manyunits
common\test_globals

io\gen_file "%STXXL_TMPDIR%\in"
utils\createdisks 1024 "%STXXL_TMPDIR%\testdiskx" "%STXXL_TMPDIR%\testdisky"
utils\createdisks 1000 "%STXXL_TMPDIR%\testdiskx" "%STXXL_TMPDIR%\testdisky"
utils\log2
utils\pq_param

io\benchmark_disks 0 2 32 "%STXXL_TMPDIR%\testdiskx" "%STXXL_TMPDIR%\testdisky"
io\flushbuffers 2 "%STXXL_TMPDIR%\testdiskx" "%STXXL_TMPDIR%\testdisky"
io\test_io "%STXXL_TMPDIR%"
io\test_cancel syscall "%STXXL_TMPDIR%\testdiskx"
io\test_cancel fileperblock_syscall "%STXXL_TMPDIR%\testdiskx"
io\test_cancel boostfd "%STXXL_TMPDIR%\testdiskx"
io\test_cancel fileperblock_boostfd "%STXXL_TMPDIR%\testdiskx"
io\test_cancel memory "%STXXL_TMPDIR%\testdiskx"
io\test_cancel wbtl "%STXXL_TMPDIR%\testdiskx"
io\test_io_sizes wincall "%STXXL_TMPDIR%\out" 5368709120
io\test_io_sizes syscall "%STXXL_TMPDIR%\out" 5368709120
io\test_io_sizes boostfd "%STXXL_TMPDIR%\out" 5368709120
::io\benchmark_disk_and_flash 0 2 "%STXXL_TMPDIR%\testdiskx" "%STXXL_TMPDIR%\testdisky"
io\benchmark_configured_disks 2 128
io\benchmark_random_block_access 2048 1024 1000000 i
io\benchmark_random_block_access 2048 128 1000000 r
io\benchmark_random_block_access 2048 128 1000000 w
io\iobench_scatter_in_place 100 10 4096 "%STXXL_TMPDIR%\out"
io\verify_disks 123456 0 2 w "%STXXL_TMPDIR%\out"
io\verify_disks 123456 0 2 r "%STXXL_TMPDIR%\out"

mng\test_block_alloc_strategy
mng\test_buf_streams
mng\test_mng
mng\test_mng1
mng\test_prefetch_pool
mng\test_write_pool
mng\test_pool_pair
mng\test_read_write_pool
mng\test_mng_recursive_alloc
mng\test_aligned

containers\bench_pqueue
containers\copy_file "%STXXL_TMPDIR%\in" "%STXXL_TMPDIR%\out"
containers\monotonic_pq 800 1
containers\pq_benchmark 1 1000000
containers\pq_benchmark 2 1000000
containers\stack_benchmark 1 1073741824
containers\stack_benchmark 2 1073741824
containers\stack_benchmark 3 1073741824
containers\stack_benchmark 4 1073741824
containers\test_deque 33333333
containers\test_ext_merger
containers\test_ext_merger2
containers\test_iterators
containers\test_many_stacks 42
containers\test_migr_stack
containers\test_pqueue
containers\test_queue
containers\test_stack 1024
containers\test_vector
containers\test_vector_export
containers\write_vector "%STXXL_TMPDIR%\in" "%STXXL_TMPDIR%\out"
containers\write_vector2 "%STXXL_TMPDIR%\in" "%STXXL_TMPDIR%\out"
containers\benchmark_naive_matrix
containers\test_map 32
containers\test_map_random 2000
containers\btree\test_btree 10000
containers\btree\test_btree 100000
containers\btree\test_btree 1000000
containers\btree\test_const_scan 10000
containers\btree\test_const_scan 100000
containers\btree\test_const_scan 1000000
containers\btree\test_corr_insert_erase 14
containers\btree\test_corr_insert_find 14
containers\btree\test_corr_insert_scan 14

algo\sort_file generate "%STXXL_TMPDIR%\sort_file.dat"
algo\sort_file sort "%STXXL_TMPDIR%\sort_file.dat"
algo\sort_file generate "%STXXL_TMPDIR%\sort_file.dat"
algo\sort_file ksort "%STXXL_TMPDIR%\sort_file.dat"
algo\sort_file generate "%STXXL_TMPDIR%\sort_file.dat"
algo\sort_file stable_sort "%STXXL_TMPDIR%\sort_file.dat"
algo\test_asch 3 100 1000 42
algo\test_ksort
algo\test_random_shuffle
algo\test_scan
algo\test_sort

stream\test_push_sort
stream\test_sorted_runs
stream\test_stream
stream\test_stream1
stream\test_transpose
stream\test_loop 100 -v
