SUBDIRS		+= .
SUBDIRS		+= include/stxxl include/stxxl/bits
SUBDIRS		+= $(foreach d, algo common containers containers/btree io mng stream utils, include/stxxl/bits/$d)
SUBDIRS		+= algo common containers containers/btree io mng stream utils

FILES		:= $(wildcard $(foreach d, $(SUBDIRS), $d/*.h $d/*.cpp))


all: uncrustify-test
all: uncrustify-diff

uncrustify-test: $(FILES:=.uncrustify)

uncrustify-diff: $(FILES:=.diff)

uncrustify-apply: $(FILES:=.unc-apply)

clean:
	$(RM) $(FILES:=.uncrustify) $(FILES:=.uncrustifyT) $(FILES:=.diff)

.SECONDARY:

############################################################################

UNCRUSTIFY	?= ./uncrustify
UNCRUSTIFY_CFG	?= misc/uncrustify.cfg
UNCRUSTIFY_FLAGS+= -c $(UNCRUSTIFY_CFG) -l CPP

%.diff: % %.uncrustify
	@test -f $*
	@test -f $*.uncrustify
	diff -u $^ > $@ || test $$? == 1
	test -s $@ || $(RM) $@

%.uncrustify: % $(UNCRUSTIFY_CFG) $(UNCRUSTIFY)
	$(RM) $<.diff
	$(UNCRUSTIFY) $(UNCRUSTIFY_FLAGS) -o $@T < $<
	mv $@T $@

%.unc-apply: %.uncrustify
	cmp -s $* $*.uncrustify || cp $*.uncrustify $*

update-uncrustify-cfg:
	$(UNCRUSTIFY) -c $(UNCRUSTIFY_CFG) --update-config-with-doc | \
		sed -e '/indent_access_spec/s/-[0-9][0-9]*/-indent_columns/' > $(UNCRUSTIFY_CFG).tmp
	cmp -s $(UNCRUSTIFY_CFG).tmp $(UNCRUSTIFY_CFG) || cp -p $(UNCRUSTIFY_CFG) $(UNCRUSTIFY_CFG).old
	cmp -s $(UNCRUSTIFY_CFG).tmp $(UNCRUSTIFY_CFG) || cp $(UNCRUSTIFY_CFG).tmp $(UNCRUSTIFY_CFG)
	$(RM) $(UNCRUSTIFY_CFG).tmp
