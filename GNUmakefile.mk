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

MCSTL	?= mcstl # undefine to disable


default-all: lib tests header-compile-test

doxy release:
	$(MAKE) -f Makefile $@

lib:
	$(MAKE) -f Makefile library_$(MODE) $(if $(MCSTL),library_$(MODE)_mcstl)

tests: lib
	$(NICE) $(MAKE) -f Makefile tests_$(MODE) $(if $(MCSTL),tests_$(MODE)_mcstl)

header-compile-test: lib
	$(NICE) $(MAKE) -C test/compile-stxxl-headers
	$(if $(MCSTL),$(NICE) $(MAKE) -C test/compile-stxxl-headers INSTANCE=mcstxxl)

clean:
	$(MAKE) -f Makefile clean_$(MODE) clean_$(MODE)_mcstl clean_doxy
	$(MAKE) -C test/compile-stxxl-headers clean

.PHONY: all default-all doxy lib tests header-compile-test clean
