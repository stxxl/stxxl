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

lib: dep-check-sanity
	$(MAKE) -f Makefile library_$(MODE) $(if $(PMODE),library_$(MODE)_pmode) $(if $(MCSTL),library_$(MODE)_mcstl)

tests: lib
	$(NICE) $(MAKE) -f Makefile tests_$(MODE) $(if $(PMODE),tests_$(MODE)_pmode) $(if $(MCSTL),tests_$(MODE)_mcstl)
	find . -name \*.d -exec grep -H '\.\..*:' {} + ; test $$? = 1

header-compile-test: lib
	$(NICE) $(MAKE) -C test/compile-stxxl-headers
	$(if $(PMODE),$(NICE) $(MAKE) -C test/compile-stxxl-headers INSTANCE=pmstxxl)
	$(if $(MCSTL),$(NICE) $(MAKE) -C test/compile-stxxl-headers INSTANCE=mcstxxl)

do-run-all-tests:
	@test -n "$(STXXL_TMPDIR)" || ( echo "STXXL_TMPDIR is not set"; exit 1 )
	@test -z "$(LD_PRELOAD)" || ( echo "LD_PRELOAD is set"; exit 1 )
	./misc/run-all-tests stxxl $(WITH_VALGRIND) $(WITH_VALGRIND)
	$(if $(MCSTL),./misc/run-all-tests mcstxxl $(WITH_VALGRIND) $(WITH_VALGRIND))
	$(if $(PMODE),./misc/run-all-tests pmstxxl $(WITH_VALGRIND) $(WITH_VALGRIND))

run-all-tests: WITH_VALGRIND=no
run-all-tests: do-run-all-tests ;

run-all-tests-valgrind: WITH_VALGRIND=yes
run-all-tests-valgrind: do-run-all-tests ;

dep-check-sanity:
	find . -name '*.o' -exec misc/remove-unless {} .o .d \; 
	find . -name '*.bin' -exec misc/remove-unless {} .bin .o \; 

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
