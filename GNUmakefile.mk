#
# HowTo use GNUmakefile.mk: create a sample GNUmakefile using the command
#
#     make -f GNUmakefile.mk GNUmakefile
#
# then edit it to override MODE, NICE (if you want)
# and select your favorite subset of targets


MODE	?= g++
MODE	?= icpc

NICE	?= nice

PMODE	?= parallel_mode # undefine to disable
MCSTL	?= mcstl # undefine to disable

MODESUFFIX	 = $(shell $(MAKE) -f Makefile.gnu getmodesuffix 2>/dev/null || true)


default-all: lib tests header-compile-test

lib: dep-check-sanity
	$(MAKE) -f Makefile library_$(MODE) $(if $(PMODE),library_$(MODE)_pmode) $(if $(MCSTL),library_$(MODE)_mcstl)

tests: lib
	$(NICE) $(MAKE) -f Makefile tests_$(MODE) $(if $(PMODE),tests_$(MODE)_pmode) $(if $(MCSTL),tests_$(MODE)_mcstl)
	find . -name \*.d -exec grep -H '\.\..*:' {} + ; test $$? = 1

localtests: lib
	$(NICE) $(MAKE) -f Makefile tests_$(MODE) $(if $(PMODE),tests_$(MODE)_pmode) $(if $(MCSTL),tests_$(MODE)_mcstl) SUBDIRS=local

localclean:
	$(MAKE) -C local clean

examples: lib
	$(MAKE) -C doc/tutorial/examples clean
	$(MAKE) -C doc/tutorial/examples all STXXL_MK=stxxl$(MODESUFFIX).mk

header-compile-test: lib
	$(NICE) $(MAKE) -C test/compile-stxxl-headers INSTANCE=stxxl$(MODESUFFIX)
	$(if $(PMODE),$(NICE) $(MAKE) -C test/compile-stxxl-headers INSTANCE=pmstxxl$(MODESUFFIX))
	$(if $(MCSTL),$(NICE) $(MAKE) -C test/compile-stxxl-headers INSTANCE=mcstxxl$(MODESUFFIX))

do-run-all-tests:
	@test -n "$(STXXL_TMPDIR)" || ( echo "STXXL_TMPDIR is not set"; exit 1 )
	@test -z "$(LD_PRELOAD)" || ( echo "LD_PRELOAD is set"; exit 1 )
	./misc/run-all-tests stxxl$(MODESUFFIX) $(WITH_VALGRIND) $(WITH_VALGRIND)
	$(if $(MCSTL),./misc/run-all-tests mcstxxl$(MODESUFFIX) $(WITH_VALGRIND) $(WITH_VALGRIND))
	$(if $(PMODE),./misc/run-all-tests pmstxxl$(MODESUFFIX) $(WITH_VALGRIND) $(WITH_VALGRIND))

run-all-tests: WITH_VALGRIND=no
run-all-tests: do-run-all-tests ;

run-all-tests-valgrind: WITH_VALGRIND=yes
run-all-tests-valgrind: do-run-all-tests ;

dep-check-sanity:
	$(SKIP_SANITY_CHECK)find . -name '*.o' -exec misc/remove-unless {} .o .d \; 
	$(SKIP_SANITY_CHECK)find . -name '*.bin' -exec misc/remove-unless {} .bin .o \; 

clean:
	$(MAKE) -f Makefile clean_$(MODE) clean_$(MODE)_pmode clean_$(MODE)_mcstl clean_doxy clean_examples
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
