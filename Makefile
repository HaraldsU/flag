CXX      := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wunused -g1 -O0 -Isrc
LDFLAGS  := -lboost_program_options

.PHONY: flag clean

all: flag flag_test

flag: src/flag.o src/main.o
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

flag_test: test/flag_test.o src/flag.o
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f src/*.o test/*.o flag flag_test

# $@ - The file name of the target of the rule.
# $< - The name of the first prerequisite.
# $^ - The names of all the prerequisites, with spaces between them.
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

