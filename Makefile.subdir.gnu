
include ../make.settings

TEST_BINARIES	 = $(foreach t, $(TESTS), $t.$(bin))

tests: $(TEST_BINARIES)

lib: $(LIB_SRC:.cpp=.$o)

clean::
	$(RM) *.$o
	$(RM) *.$d *.dT
	$(RM) $(TEST_BINARIES)

-include *.d

.SECONDARY:

.PHONY: tests lib clean
