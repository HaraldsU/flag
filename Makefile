CXX      := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wunused -g1 -O0 -Isrc
LDFLAGS  := -lboost_program_options
# Virtual path
VPATH	  = src:test

all: flag flag_test
flag: flag.o main.o
flag_test: flag_test.o flag.o
clean:
	rm -f *.o flag flag_test

# $@ - The file name of the target of the rule.
# $< - The name of the first prerequisite.
# $^ - The names of all the prerequisites, with spaces between them.
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%: %.o
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

