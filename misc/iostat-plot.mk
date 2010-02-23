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
IOSTAT_PLOT_OFFSET_utilize_12	?= 12
IOSTAT_PLOT_OFFSET_read_14	?= 8
IOSTAT_PLOT_OFFSET_write_14	?= 9
IOSTAT_PLOT_OFFSET_utilize_14	?= ???
IOSTAT_PLOT_DISK_LIST		?= sda sdb sdc sdd sde sdf sdg sdh sdi sdj
IOSTAT_PLOT_IO_WITH_UTILIZATION	?= no

IOSTAT_PLOT_Y_LABEL.io		?= Bandwidth [MiB/s]
IOSTAT_PLOT_Y_LABEL.cpu		?= CPU Usage [%]

ECHO				?= /bin/echo

# $1 = first, $2 = increment, $3 = last
define gnuplot-column-sequence-sum
$(subst $(space),+,$(patsubst %,$$%,$(shell seq $1 $2 $3)))
endef

# $1 = io | cpu, $2 = numdisks (io) | numcpus (cpu), $3 = stride
define template-iostat-gnuplot
	$(RM) $@
	$(ECHO) 'set title "$(subst _, ,$*) (avg=$(IOSTAT_PLOT_AVERAGE.$(strip $1)))"' >> $@
	$(ECHO) 'set xlabel "Time [s]"' >> $@
	$(ECHO) 'set ylabel "$(IOSTAT_PLOT_Y_LABEL.$(strip $1))"' >> $@
$(if $(filter yes,$(IOSTAT_PLOT_IO_WITH_UTILIZATION)),
	$(ECHO) 'set y2label "Utilization [%]"' >> $@
	$(ECHO) 'set ytics nomirror' >> $@
	$(ECHO) 'set y2tics' >> $@
,$(if $(wildcard $*.waitlog),
	$(ECHO) 'set y2label "Wait Time [s]"' >> $@
	$(ECHO) 'set ytics nomirror' >> $@
	$(ECHO) 'set y2tics' >> $@
))
#	$(ECHO) 'set data style linespoints' >> $@
	$(ECHO) 'set data style lines' >> $@
	$(ECHO) 'set macros' >> $@
	$(ECHO) 'set pointsize 0.4' >> $@
#	$(ECHO) 'set samples 1000' >> $@
	$(ECHO) 'set key top left' >> $@
	$(ECHO) 'set yrange [0:]' >> $@
	$(ECHO) '' >> $@
$(if $(filter io,$1),
	$(ECHO) 'read = "$(call gnuplot-column-sequence-sum, $(IOSTAT_PLOT_OFFSET_read_$3), $3, $(shell expr $2 '*' $3))"' >> $@
	$(ECHO) 'write = "$(call gnuplot-column-sequence-sum, $(IOSTAT_PLOT_OFFSET_write_$3), $3, $(shell expr $2 '*' $3))"' >> $@
	$(ECHO) 'utilize = "($(call gnuplot-column-sequence-sum, $(IOSTAT_PLOT_OFFSET_utilize_$3), $3, $(shell expr $2 '*' $3))) / $2"' >> $@
	$(ECHO) '' >> $@
	$(ECHO) 'plot \' >> $@
	$(ECHO) '	"$<" using 0:(@read + @write) title "Read + Write" ls 3$(comma) \' >> $@
	$(ECHO) '	"$<" using 0:(@read) title "Read" ls 2$(comma) \' >> $@
	$(ECHO) '	"$<" using 0:(@write) title "Write" ls 1$(comma) \' >> $@
$(if $(filter yes,$(IOSTAT_PLOT_IO_WITH_UTILIZATION)),
	$(ECHO) '	"$<" using 0:(@utilize) title "Utilization" ls 5 axes x1y2$(comma) \' >> $@
,$(if $(wildcard $*.waitlog),
	$(ECHO) '	"$*.waitlog" using 1:4 title "Wait Read" ls 5 axes x1y2$(comma) \' >> $@
	$(ECHO) '	"$*.waitlog" using 1:5 title "Wait Write" ls 4 axes x1y2$(comma) \' >> $@
))
	$(ECHO) '	"not.existing.dummy" using 08:15 notitle' >> $@
)
$(if $(filter cpu,$1),
	$(ECHO) 'plot \' >> $@
	$(ECHO) '	"$(word 2,$^)" using 0:(100*($$1-$(IOSTAT_PLOT_LOAD_REDUCE))/$(strip $2)) title "Load (100% = $2)" ls 5$(comma) \' >> $@
	$(ECHO) '	"$<" using 0:($$1+$$3+$$4) title "Total" ls 4$(comma) \' >> $@
	$(ECHO) '	"$<" using 0:4 title "Wait" ls 2$(comma) \' >> $@
	$(ECHO) '	"$<" using 0:1 title "User" ls 1$(comma) \' >> $@
	$(ECHO) '	"$<" using 0:3 title "System" ls 3$(comma) \' >> $@
	$(ECHO) '	"not.existing.dummy" using 08:15 notitle' >> $@ 
)
	$(ECHO) '' >> $@
	$(ECHO) 'pause -1' >> $@
	$(ECHO) '' >> $@
	$(ECHO) 'set terminal postscript enhanced $(GPLT_COLOR_PS) 10' >> $@
	$(ECHO) 'set output "$*.$(strip $1).eps"' >> $@
	$(ECHO) 'replot' >> $@
	$(ECHO) '' >> $@
endef

%.io-$(IOSTAT_PLOT_AVERAGE.io).dat: %.iostat $(MAKEFILE_LIST)
	$(pipefail) \
	$(IOSTAT_PLOT_CONCAT_LINES) $< $(IOSTAT_PLOT_LINE_IDENTIFIER) | \
	grep "$(IOSTAT_PLOT_LINE_IDENTIFIER)" | \
	$(IOSTAT_PLOT_FLOATING_AVERAGE) $(IOSTAT_PLOT_AVERAGE.io) > $@

define per-disk-plot-template
%.$(disk).io-$$(IOSTAT_PLOT_AVERAGE.io).dat: IOSTAT_PLOT_LINE_IDENTIFIER=^$(disk)
%.$(disk).io-$$(IOSTAT_PLOT_AVERAGE.io).plot: IOSTAT_PLOT_DISKS=1
%.$(disk).io-$$(IOSTAT_PLOT_AVERAGE.io).plot: IOSTAT_PLOT_IO_WITH_UTILIZATION=yes

%.$(disk).io-$$(IOSTAT_PLOT_AVERAGE.io).dat: %.iostat $$(MAKEFILE_LIST)
	$$(pipefail) \
	$$(IOSTAT_PLOT_CONCAT_LINES) $$< $$(IOSTAT_PLOT_LINE_IDENTIFIER) | \
	grep "$$(IOSTAT_PLOT_LINE_IDENTIFIER)" | \
	$$(IOSTAT_PLOT_FLOATING_AVERAGE) $$(IOSTAT_PLOT_AVERAGE.io) > $$@
endef
$(foreach disk, $(IOSTAT_PLOT_DISK_LIST),$(eval $(per-disk-plot-template)))

%.cpu-$(IOSTAT_PLOT_AVERAGE.cpu).dat: %.iostat $(MAKEFILE_LIST)
	$(pipefail) \
	grep "$(IOSTAT_PLOT_CPU_LINE_IDENTIFIER)" $< | \
	$(IOSTAT_PLOT_FLOATING_AVERAGE) $(IOSTAT_PLOT_AVERAGE.cpu) > $@

%.io-$(IOSTAT_PLOT_AVERAGE.io).plot: %.io-$(IOSTAT_PLOT_AVERAGE.io).dat $(MAKEFILE_LIST)
	$(call template-iostat-gnuplot,io,$(IOSTAT_PLOT_DISKS),$(IOSTAT_PLOT_STRIDE))

%.io.plot: %.io-$(IOSTAT_PLOT_AVERAGE.io).plot
	@$(ECHO) Your plot file is: $<

%.cpu-$(IOSTAT_PLOT_AVERAGE.cpu).plot: %.cpu-$(IOSTAT_PLOT_AVERAGE.cpu).dat %.loadavg $(MAKEFILE_LIST)
	$(call template-iostat-gnuplot,cpu,$(IOSTAT_PLOT_CPUS),$(IOSTAT_PLOT_STRIDE))

%.cpu.plot: %.cpu-$(IOSTAT_PLOT_AVERAGE.cpu).plot
	@$(ECHO) Your plot file is: $<

%.io.xplot: %.io-$(IOSTAT_PLOT_AVERAGE.io).plot
	gnuplot $<

%.cpu.xplot: %.cpu-$(IOSTAT_PLOT_AVERAGE.cpu).plot
	gnuplot $<

.SECONDARY:
.DELETE_ON_ERROR:
