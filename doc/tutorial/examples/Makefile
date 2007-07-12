all: phonebills phonebills_stl genlog

include ../../../make.settings.g++

phonebills: phonebills.cpp
	g++ phonebills.cpp -o phonebills $(STXXL_COMPILER_OPTIONS) $(STXXL_LINKER_OPTIONS) -DUSE_STXXL -O3 


phonebills_stl: phonebills.cpp
	g++ phonebills.cpp -o phonebills_stl $(STXXL_COMPILER_OPTIONS) $(STXXL_LINKER_OPTIONS) -O3 -DNDEBUG


genlog: genlog.cpp
	g++ genlog.cpp -o genlog $(STXXL_COMPILER_OPTIONS) $(STXXL_LINKER_OPTIONS) -O3 -DNDEBUG 


clean:
	rm -f genlog phonebills_stl phonebills *~

