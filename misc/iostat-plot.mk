############################################################################
#  misc/iostat-plot.mk
#
#  Part of the STXXL. See http://stxxl.sourceforge.net
#
#  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################

#
# to record the data, include this file from your Makefile
# and use a target like
#
# my_results.out:
#	$(IOSTAT_PLOT_RECORD_DATA) -p $(@:.out=) \
#	my_program arg1 ... argn > $@
#
# and then run
#
# make my_results.out
# make my_results.io.plot
# make my_results.cpu.plot
#

empty	?=#
space	?= $(empty) $(empty)
comma	?= ,

IOSTAT_PLOT_BINDIR		?= .
IOSTAT_PLOT_RECORD_DATA		?= $(IOSTAT_PLOT_BINDIR)/record-load-iostat
IOSTAT_PLOT_CONCAT_LINES	?= $(IOSTAT_PLOT_BINDIR)/concat-lines
IOSTAT_PLOT_FLOATING_AVERAGE	?= $(IOSTAT_PLOT_BINDIR)/floating-average

IOSTAT_PLOT_LINE_IDENTIFIER	?= ^sd
IOSTAT_PLOT_CPU_LINE_IDENTIFIER	?= ^$(space)$(space)$(space)
IOSTAT_PLOT_DISKS		?= 8
IOSTAT_PLOT_CPUS		?= 8
IOSTAT_PLOT_STRIDE		?= 12
IOSTAT_PLOT_AVERAGE		?= 1
IOSTAT_PLOT_AVERAGE.io		?= $(IOSTAT_PLOT_AVERAGE)
IOSTAT_PLOT_AVERAGE.cpu		?= $(IOSTAT_PLOT_AVERAGE)
IOSTAT_PLOT_LOAD_REDUCE		?= 0
IOSTAT_PLOT_OFFSET_read_12	?= 6
IOSTAT_PLOT_OFFSET_write_12	?= 7
IOSTAT_PLOT_OFFSET_read_14	?= 8
IOSTAT_PLOT_OFFSET_write_14	?= 9
IOSTAT_PLOT_DISK_LIST		?= sda sdb sdc sdd sde sdf sdg sdh sdi sdj

IOSTAT_PLOT_Y_LABEL.io		?= Bandwidth [MiB/s]
IOSTAT_PLOT_Y_LABEL.cpu		?= CPU Usage [%]

# $1 = first, $2 = increment, $3 = last
define gnuplot-column-sequence-sum
$(subst $(space),+,$(patsubst %,$$%,$(shell seq $1 $2 $3)))
endef

# $1 = io | cpu, $2 = numdisks (io) | numcpus (cpu), $3 = stride
define template-iostat-gnuplot
	$(RM) $@
	echo 'set title "$(subst _, ,$*) (avg=$(IOSTAT_PLOT_AVERAGE.$(strip $1)))"' >> $@
	echo 'set xlabel "Time [s]"' >> $@
	echo 'set ylabel "$(IOSTAT_PLOT_Y_LABEL.$(strip $1))"' >> $@
#	echo 'set data style linespoints' >> $@
	echo 'set data style lines' >> $@
	echo 'set macros' >> $@
	echo 'set pointsize 0.4' >> $@
#	echo 'set samples 1000' >> $@
	echo 'set key top left' >> $@
	echo 'set yrange [0:]' >> $@
	echo '' >> $@
$(if $(filter io,$1),
	echo 'read = "$(call gnuplot-column-sequence-sum, $(IOSTAT_PLOT_OFFSET_read_$3), $3, $(shell expr $2 '*' $3))"' >> $@
	echo 'write = "$(call gnuplot-column-sequence-sum, $(IOSTAT_PLOT_OFFSET_write_$3), $3, $(shell expr $2 '*' $3))"' >> $@
	echo '' >> $@
	echo 'plot \' >> $@
	echo '	"$<" using 0:(@read + @write) title "Read + Write" ls 3$(comma) \' >> $@
	echo '	"$<" using 0:(@read) title "Read" ls 2$(comma) \' >> $@
	echo '	"$<" using 0:(@write) title "Write" ls 1$(comma) \' >> $@
	echo '	"not.existing.dummy" using 08:15 notitle' >> $@
)
$(if $(filter cpu,$1),
	echo 'plot \' >> $@
	echo '	"$(word 2,$^)" using 0:(100*($$1-$(IOSTAT_PLOT_LOAD_REDUCE))/$(strip $2)) title "Load (100% = $2)" ls 5$(comma) \' >> $@
	echo '	"$<" using 0:($$1+$$3+$$4) title "Total" ls 4$(comma) \' >> $@
	echo '	"$<" using 0:4 title "Wait" ls 2$(comma) \' >> $@
	echo '	"$<" using 0:1 title "User" ls 1$(comma) \' >> $@
	echo '	"$<" using 0:3 title "System" ls 3$(comma) \' >> $@
	echo '	"not.existing.dummy" using 08:15 notitle' >> $@ 
)
	echo '' >> $@
	echo 'pause -1' >> $@
	echo '' >> $@
	echo 'set terminal postscript enhanced $(GPLT_COLOR_PS) 10' >> $@
	echo 'set output "$*.$(strip $1).eps"' >> $@
	echo 'replot' >> $@
	echo '' >> $@
endef

%.io-$(IOSTAT_PLOT_AVERAGE.io).dat: %.iostat $(wildcard $(TOPDIR_RESULTS)/*.mk Makefile*)
	$(pipefail) \
	$(IOSTAT_PLOT_CONCAT_LINES) $< $(IOSTAT_PLOT_LINE_IDENTIFIER) | \
	grep "$(IOSTAT_PLOT_LINE_IDENTIFIER)" | \
	$(IOSTAT_PLOT_FLOATING_AVERAGE) $(IOSTAT_PLOT_AVERAGE.io) > $@

define per-disk-plot-template
%.$(disk).io-$$(IOSTAT_PLOT_AVERAGE.io).dat: IOSTAT_PLOT_LINE_IDENTIFIER=^$(disk)
%.$(disk).io-$$(IOSTAT_PLOT_AVERAGE.io).plot: IOSTAT_PLOT_DISKS=1

%.$(disk).io-$$(IOSTAT_PLOT_AVERAGE.io).dat: %.iostat $$(wildcard $$(TOPDIR_RESULTS)/*.mk Makefile*)
	$$(pipefail) \
	$$(IOSTAT_PLOT_CONCAT_LINES) $$< $$(IOSTAT_PLOT_LINE_IDENTIFIER) | \
	grep "$$(IOSTAT_PLOT_LINE_IDENTIFIER)" | \
	$$(IOSTAT_PLOT_FLOATING_AVERAGE) $$(IOSTAT_PLOT_AVERAGE.io) > $$@
endef
$(foreach disk, $(IOSTAT_PLOT_DISK_LIST),$(eval $(per-disk-plot-template)))

%.cpu-$(IOSTAT_PLOT_AVERAGE.cpu).dat: %.iostat $(wildcard $(TOPDIR_RESULTS)/*.mk Makefile*)
	$(pipefail) \
	grep "$(IOSTAT_PLOT_CPU_LINE_IDENTIFIER)" $< | \
	$(IOSTAT_PLOT_FLOATING_AVERAGE) $(IOSTAT_PLOT_AVERAGE.cpu) > $@

%.io-$(IOSTAT_PLOT_AVERAGE.io).plot: %.io-$(IOSTAT_PLOT_AVERAGE.io).dat $(wildcard $(TOPDIR_RESULTS)/*.mk Makefile*)
	$(call template-iostat-gnuplot,io,$(IOSTAT_PLOT_DISKS),$(IOSTAT_PLOT_STRIDE))

%.io.plot: %.io-$(IOSTAT_PLOT_AVERAGE.io).plot
	@echo Your plot file is: $<

%.cpu-$(IOSTAT_PLOT_AVERAGE.cpu).plot: %.cpu-$(IOSTAT_PLOT_AVERAGE.cpu).dat %.loadavg $(wildcard $(TOPDIR_RESULTS)/*.mk Makefile*)
	$(call template-iostat-gnuplot,cpu,$(IOSTAT_PLOT_CPUS),$(IOSTAT_PLOT_STRIDE))

%.cpu.plot: %.cpu-$(IOSTAT_PLOT_AVERAGE.cpu).plot
	@echo Your plot file is: $<

%.io.xplot: %.io-$(IOSTAT_PLOT_AVERAGE.io).plot
	gnuplot $<

%.cpu.xplot: %.cpu-$(IOSTAT_PLOT_AVERAGE.cpu).plot
	gnuplot $<

.SECONDARY:
.DELETE_ON_ERROR:
