############################################################################
#  tests/parallel/bench_multiway_merge.plot
#
#  SqlPlotTools script for output of bench_multiway_merge.cpp
#  Run the benchmark and save the output to stats.txt
#
#  Part of the STXXL. See http://stxxl.sourceforge.net
#
#  Copyright (C) 2014-2015 Timo Bingmann <tb@panthema.net>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################

# IMPORT-DATA stats stats.txt

set terminal pdf size 28cm,18cm linewidth 2.0
set output "bench_multiway_merge-results.pdf"

set pointsize 0.7
set style line 6 lc rgb "#f0b000"
set style line 15 lc rgb "#f0b000"
set style line 24 lc rgb "#f0b000"
set style line 33 lc rgb "#f0b000"
set style line 42 lc rgb "#f0b000"
set style line 51 lc rgb "#f0b000"
set style line 60 lc rgb "#f0b000"
set style increment user

set grid xtics ytics

set key top left

set title 'Speed of Sequential Multiway Merging'
set xlabel 'Number of 2 MiB Sequences'
set ylabel 'Run Time per Item [Nanoseconds / Item]'

## MULTIPLOT(method)
## SELECT seqnum AS x, AVG(time / total_size * 1e9) AS y, MULTIPLOT
## FROM stats WHERE method LIKE 'seq_%' GROUP BY MULTIPLOT,x ORDER BY MULTIPLOT,x
plot \
    'bench_multiway_merge-data.txt' index 0 title "method=seq_gnu_mwm" with linespoints, \
    'bench_multiway_merge-data.txt' index 1 title "method=seq_mwm" with linespoints

set title 'Speed of Parallel Multiway Merging'

## MULTIPLOT(method)
## SELECT seqnum AS x, AVG(time / total_size * 1e9) AS y, MULTIPLOT
## FROM stats WHERE method LIKE 'para_%' GROUP BY MULTIPLOT,x ORDER BY MULTIPLOT,x
plot \
    'bench_multiway_merge-data.txt' index 2 title "method=para_gnu_mwm" with linespoints, \
    'bench_multiway_merge-data.txt' index 3 title "method=para_mwm" with linespoints
