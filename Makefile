CXX       = clang++
CXXFLAGS += -std=c++17 -Wall -Wextra -Wunused -O2 -Isrc
LDFLAGS  += -lboost_program_options
PREFIX   ?= /usr/local
BINDIR   ?= $(PREFIX)/bin

.PHONY: all clean install

all: flag flag_test

flag: src/flag.o src/main.o
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

flag_test: test/flag_test.o src/flag.o
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

install: flag
	install -Dm755 flag "$(DESTDIR)$(BINDIR)/flag"

clean:
	rm -f src/*.o test/*.o flag flag_test

# $@ - The file name of the target of the rule.
# $< - The name of the first prerequisite.
# $^ - The names of all the prerequisites, with spaces between them.
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

