CXXFLAGS   += -std=c++17 -Wall -Wextra -Wunused -O2 -Isrc
LDFLAGS    += -lboost_program_options

PREFIX     ?= /usr
BINDIR     ?= $(PREFIX)/bin
DATADIR    ?= $(PREFIX)/share
MANDIR     ?= $(DATADIR)/man
LICENSEDIR ?= $(DATADIR)/licenses/flag

.PHONY: all clean install uninstall

all: flag flag_test

flag: src/flag.o src/main.o
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

flag_test: test/flag_test.o src/flag.o
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

install: flag
	install -Dm755 flag "$(DESTDIR)$(BINDIR)/flag"
	install -Dm644 LICENSE -t "$(DESTDIR)$(LICENSEDIR)/"
	install -Dm644 man/flag.1 -t "$(DESTDIR)$(MANDIR)/man1/"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/flag"
	rm -f "$(DESTDIR)$(LICENSEDIR)/LICENSE"
	rm -f "$(DESTDIR)$(MANDIR)/man1/flag.1"
	-rmdir "$(DESTDIR)$(LICENSEDIR)"

clean:
	rm -f src/*.o test/*.o flag flag_test

# $@ - The file name of the target of the rule.
# $< - The name of the first prerequisite.
# $^ - The names of all the prerequisites, with spaces between them.
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

