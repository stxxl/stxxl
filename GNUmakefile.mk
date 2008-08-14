#
# HowTo use GNUmakefile.mk: create GNUMAKEfile containing:
#

## # override MODE, NICE if you want
##
## # select your favorite subset of targets
## all: lib tests header-compile-test doxy
##
## include GNUmakefile.mk


MODE	?= g++
MODE	?= icpc

NICE	?= nice

PMODE	?= parallel_mode # undefine to disable
MCSTL	?= mcstl # undefine to disable


default-all: lib tests header-compile-test

lib:
	$(MAKE) -f Makefile library_$(MODE) $(if $(PMODE),library_$(MODE)_pmode) $(if $(MCSTL),library_$(MODE)_mcstl)

tests: lib
	$(NICE) $(MAKE) -f Makefile tests_$(MODE) $(if $(PMODE),tests_$(MODE)_pmode) $(if $(MCSTL),tests_$(MODE)_mcstl)

header-compile-test: lib
	$(NICE) $(MAKE) -C test/compile-stxxl-headers
	$(if $(PMODE),$(NICE) $(MAKE) -C test/compile-stxxl-headers INSTANCE=pmstxxl)
	$(if $(MCSTL),$(NICE) $(MAKE) -C test/compile-stxxl-headers INSTANCE=mcstxxl)

clean:
	$(MAKE) -f Makefile clean_$(MODE) clean_$(MODE)_pmode clean_$(MODE)_mcstl clean_doxy
	$(MAKE) -C test/compile-stxxl-headers clean

GNUmakefile:
	echo "" > $@
	echo "#MODE=g++" >> $@
	echo "#MODE=icpc" >> $@
	echo "#NICE=" >> $@
	echo "#PMODE=#empty" >> $@
	echo "#MCSTL=#empty" >> $@
	echo "" >> $@
	echo "all: lib header-compile-test tests" >> $@
	echo "" >> $@
	echo "-include make.settings.local" >> $@
	echo "include GNUmakefile.mk" >> $@

%::
	$(MAKE) -f Makefile $@

.PHONY: all default-all lib tests header-compile-test clean
