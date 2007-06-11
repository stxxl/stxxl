include Makefile.common
include ../make.settings

all: create 

create: $(UTIL_BINARIES)

tests: $(TESTS)

lib: $(LIB_SRC)


DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES) $(CONTAINER_FILES) $(ALGO_FILES)


createdisks: createdisks.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c createdisks.cpp -DNDEBUG
	$(LINKER) createdisks.$(OBJEXT) $(OUT)createdisks.$(EXEEXT) $(STXXL_LINKER_OPTIONS)


clean:
	$(RM) *.$(OBJEXT) createdisks.$(EXEEXT)
