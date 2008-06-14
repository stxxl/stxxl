SUBDIRS		+= .
SUBDIRS		+= include include/stxxl include/stxxl/bits
SUBDIRS		+= $(foreach d, algo common containers containers/btree io mng stream utils, include/stxxl/bits/$d)
SUBDIRS		+= algo common containers containers/btree io mng stream utils

FILES_IGNORE	:= ./doxymain.h
FILES		:= $(filter-out $(FILES_IGNORE),$(wildcard $(foreach d, $(SUBDIRS), $d/*.h $d/*.cpp)))
FILES		+= $(foreach h, algorithm all deque io ksort mallocstats map mng priority_queue queue random scan sort stable_ksort stack stream timer vector, include/stxxl/$h)


all: uncrustify-test
all: uncrustify-diff

uncrustify-test: $(FILES:=.uncrustify)

uncrustify-diff: $(FILES:=.diff)

uncrustify-apply: $(FILES:=.unc-apply)

viewdiff:
	view $(wildcard $(foreach d, $(SUBDIRS), $d/*.diff))

clean:
	$(RM) $(FILES:=.uncrustify) $(FILES:=.uncrustifyT) $(FILES:=.diff)
	$(RM) $(UNCRUSTIFY_CFG)-file-header

.SECONDARY:

############################################################################

#UNCRUSTIFY	?= ./uncrustify
UNCRUSTIFY	?= uncrustify
UNCRUSTIFY_CFG	?= misc/uncrustify.cfg
UNCRUSTIFY_FLAGS+= -c $(UNCRUSTIFY_CFG) -l CPP

%.diff: % %.uncrustify
	@test -f $*
	@test -f $*.uncrustify
	diff -u $^ > $@ || test $$? == 1
	test -s $@ || $(RM) $@

%.uncrustify: % $(UNCRUSTIFY_CFG) #$(UNCRUSTIFY)
	$(RM) $<.diff
	$(UNCRUSTIFY) $(UNCRUSTIFY_FLAGS) -o $@T < $<
	sed -i -e 's/template</template </g' $@T
	mv $@T $@

%.unc-apply: %.uncrustify
	cmp -s $* $*.uncrustify || cp $*.uncrustify $*

update-uncrustify-cfg:
	$(UNCRUSTIFY) -c $(UNCRUSTIFY_CFG) --update-config-with-doc | \
		sed -e '/indent_access_spec/s/-[0-9][0-9]*/-indent_columns/' > $(UNCRUSTIFY_CFG).tmp
	cmp -s $(UNCRUSTIFY_CFG).tmp $(UNCRUSTIFY_CFG) || cp -p $(UNCRUSTIFY_CFG) $(UNCRUSTIFY_CFG).old
	cmp -s $(UNCRUSTIFY_CFG).tmp $(UNCRUSTIFY_CFG) || cp $(UNCRUSTIFY_CFG).tmp $(UNCRUSTIFY_CFG)
	$(RM) $(UNCRUSTIFY_CFG).tmp

uncrustify-file-header: $(UNCRUSTIFY_CFG)-file-header
	$(MAKE) -f $(lastword $(MAKEFILE_LIST)) uncrustify-test uncrustify-diff UNCRUSTIFY_CFG=$(UNCRUSTIFY_CFG)-file-header

$(UNCRUSTIFY_CFG)-file-header: $(UNCRUSTIFY_CFG)
	sed -e '/cmt_insert_file_header/s/""/misc\/fileheader.txt/' $< > $@

.PHONY: all clean viewdiff uncrustify-test uncrustify-diff uncrustify-apply update-uncrustify-cfg
