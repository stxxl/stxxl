
HOST		?= unknown
SIZE		?= 100	# GB

disk2file	?= /stxxl/sd$1/stxxl

DISKS_1by1	?= a b c d
DISKS_a		?= a
DISKS_ab	?= a b
DISKS_abcd	?= a b c d

DISKS		?= abcd $(DISKS_1by1) ab

pipefail		?= set -o pipefail;

define do-some-disks
	-$(pipefail) \
	$(RECORD_LOAD_IOSTAT) \
	./benchmark_disks.stxxl.bin 0 $(SIZE) $(foreach d,$(DISKS_$*),$(call disk2file,$d)) | tee $@
endef

$(HOST)-%.cr.log:
	$(RM) $(foreach d,$(DISKS_$*),$(call disk2file,$d))
	$(do-some-disks)

$(HOST)-%.wr.log:
	$(do-some-disks)

all: cr wr

cr: $(foreach d,$(DISKS_1by1),$(HOST)-$d.cr.log)
wr: $(foreach d,$(DISKS),$(HOST)-$d.wr.log)

plot: $(HOST).gnuplot
	gnuplot $<

# $1 = logfile, $2 = column
extract_average	= $(if $(wildcard $1),$(shell tail -n 1 $1 | awk '{ print $$($2+1) }'),......)

# $1 = logfile, $2 = disk, $3 = column, $4 = label
define plotline
	echo '        "$1" using ($$3/1024):($$$3) w l title "$2 $4 ($(call extract_average,$1,$3))", \' >> $@ ;
endef

# $1 = logfile, $2 = disk
define plotline-cr1
	$(if $(wildcard $1),$(call plotline,$1,$2,7,cr1))
endef
define plotline-cr
	$(call plotline,$1,$2,7,cr)
endef
define plotline-wr
	$(call plotline,$1,$2,7,wr)
endef
define plotline-rd
	$(call plotline,$1,$2,14,rd)
endef

# $1 = disk letter
disk2label	?= sd$1
disks2label	?= sd[$1]

DISKNAME	?= unknown disk
PLOTXMAX	?= 475
PLOTYMAX	?= 120

$(HOST).gnuplot: Makefile $(wildcard *.log)
	$(RM) $@
	echo 'set title "STXXL Disk Benchmark $(DISKNAME) @ $(HOST)"' >> $@
	echo 'set xlabel "Disk offset [GB]"' >> $@
	echo 'set ylabel "Bandwidth per disk [MB/s]"' >> $@
	echo '' >> $@

	echo 'plot [0:$(PLOTXMAX)] [0:$(PLOTYMAX)] \' >> $@
	$(foreach d,$(DISKS_1by1),\
		$(call plotline-cr1,$(HOST)-$d.cr1.log,$(call disk2label,$d)) \
		$(call plotline-cr,$(HOST)-$d.cr.log,$(call disk2label,$d)) \
		$(call plotline-wr,$(HOST)-$d.wr.log,$(call disk2label,$d)) \
		$(call plotline-rd,$(HOST)-$d.wr.log,$(call disk2label,$d)))
	$(foreach d,$(filter-out $(DISKS_1by1),$(DISKS)),\
		$(call plotline-wr,$(HOST)-$d.wr.log,$(call disks2label,$d)) \
		$(call plotline-rd,$(HOST)-$d.wr.log,$(call disks2label,$d)))
	echo '        "nothing" notitle' >> $@

	echo '' >> $@
	echo 'pause mouse' >> $@
	echo '' >> $@
	echo 'set title "STXXL Disk Benchmark $(DISKNAME) \\@ $(HOST)"' >> $@
	echo 'set term postscript enhanced color' >> $@
	echo 'set output "$(HOST).ps"' >> $@
	echo 'replot' >> $@

-include iostat-plot.mk

