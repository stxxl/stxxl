include Makefile.common
include ../make.settings

tools: $(UTIL_BINARIES)

tests: $(TESTS)
#tests: $(TESTS_NON_MSVC)

NDEBUG		 = #-DNDEBUG
DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES) $(CONTAINER_FILES) $(ALGO_FILES)

createdisks: createdisks.$(EXEEXT)
createdisks.$(EXEEXT): createdisks.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c createdisks.cpp $(NDEBUG)
	$(LINKER) createdisks.$(OBJEXT) $(OUT)createdisks.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

malloc: malloc.$(EXEEXT)
malloc.$(EXEEXT): malloc.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c malloc.cpp $(NDEBUG)
	$(LINKER) malloc.$(OBJEXT) $(OUT)malloc.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

off_t_size: off_t_size.$(EXEEXT)
off_t_size.$(EXEEXT): off_t_size.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c off_t_size.cpp $(NDEBUG)
	$(LINKER) off_t_size.$(OBJEXT) $(OUT)off_t_size.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

pq_param: pq_param.$(EXEEXT)
pq_param.$(EXEEXT): pq_param.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c pq_param.cpp $(NDEBUG)
	$(LINKER) pq_param.$(OBJEXT) $(OUT)pq_param.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

log2: log2.$(EXEEXT)
log2.$(EXEEXT): log2.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c log2.cpp $(NDEBUG)
	$(LINKER) log2.$(OBJEXT) $(OUT)log2.$(EXEEXT) $(STXXL_LINKER_OPTIONS)


clean:
	$(RM) *.$(OBJEXT) *.$(EXEEXT)
