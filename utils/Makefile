include Makefile.common
include ../make.settings

create: $(UTIL_BINARIES)

tests: $(TESTS)
#tests: $(TESTS_NON_MSVC)

lib: $(LIB_SRC)


DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES) $(CONTAINER_FILES) $(ALGO_FILES)


createdisks: createdisks.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c createdisks.cpp -DNDEBUG
	$(LINKER) createdisks.$(OBJEXT) $(OUT)createdisks.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

malloc: malloc.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c malloc.cpp -DNDEBUG
	$(LINKER) malloc.$(OBJEXT) $(OUT)malloc.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

off_t_size: off_t_size.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c off_t_size.cpp -DNDEBUG
	$(LINKER) off_t_size.$(OBJEXT) $(OUT)off_t_size.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

pq_param: pq_param.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c pq_param.cpp -DNDEBUG
	$(LINKER) pq_param.$(OBJEXT) $(OUT)pq_param.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

log2: log2.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c log2.cpp -DNDEBUG
	$(LINKER) log2.$(OBJEXT) $(OUT)log2.$(EXEEXT) $(STXXL_LINKER_OPTIONS)


clean:
	$(RM) *.$(OBJEXT) *.$(EXEEXT)
