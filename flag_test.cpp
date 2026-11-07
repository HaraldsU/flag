#define BOOST_TEST_MODULE Flag 
#include <boost/test/included/unit_test.hpp>
#include <wordexp.h>
#include "flag.h"

using namespace std;

const static char * in = "flag -s --delimiter '\n' ls";

struct Input {
    Input (const char * command) {
        BOOST_REQUIRE(wordexp(command, &w, WRDE_NOCMD) == 0);

        ac = static_cast<int>(w.we_wordc);
        av = w.we_wordv;
    }
    ~Input () {
        wordfree(&w);
    }

    int       ac;
    char**    av;
    wordexp_t w;
};

struct Args {
    Args() { load(in); }

    void load(const char* command) {
        reset_globals();
        Input input(command);
        set_program_options(input.ac, input.av);
    }

    vector<string> commands_for(const char* command) {
        load(command);

        Input input(command);
        string man_page = load_man_page(input.ac, input.av);
        vector<string> actual;
        get_commands(actual, man_page);
        return actual;
    }

    void check_cases(const vector<pair<const char*, vector<string>>> cases) {
        for (const auto& [command, expected] : cases) {
            BOOST_TEST_CONTEXT("command: " << command) {
                if (expected.size() == 0) BOOST_CHECK_THROW(commands_for("flag -M test"), std::string);
                else {
                    auto actual = commands_for(command);
                    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
                    if (actual.size() < expected.size()) BOOST_TEST_MESSAGE("missing at end: " << expected[actual.size()]);
                    if (actual.size() > expected.size()) BOOST_TEST_MESSAGE("extra at end: " << actual[expected.size()]);
                }
            }
        }
    }
};

void remove_spaces(vector<string> & v) {
    v.erase(std::remove(v.begin(), v.end(), " "), v.end());
}

void make_mixed_from_short_and_long(
        const vector<string> es, 
        const vector<string> el,
        vector<string> & em,
        vector<string> & emo) {
    for (size_t i = 0; i < es.size(); i++) {
        auto es_ = es[i];
        auto el_ = el[i];
        if (es_ != " " and el_ != " ") {
            em.push_back(es_ + "," + el_);
            emo.push_back(es_ + "," + el_);
        } else if (es_ != " ") {
            em.push_back(es_);
        } else if (el_ != " ") {
            em.push_back(el_);
        }
    }
}

BOOST_FIXTURE_TEST_SUITE(flag_tests, Args)

BOOST_AUTO_TEST_CASE(string_to_argc_argv) {
    Input input("flag --color=always --enclose [] -c 3 -MSrtw ls");
    BOOST_TEST(input.ac == 8);
    BOOST_TEST(string(input.av[0]) == "flag");
    BOOST_TEST(string(input.av[1]) == "--color=always");
    BOOST_TEST(string(input.av[2]) == "--enclose");
    BOOST_TEST(string(input.av[3]) == "[]");
    BOOST_TEST(string(input.av[4]) == "-c");
    BOOST_TEST(string(input.av[5]) == "3");
    BOOST_TEST(string(input.av[6]) == "-MSrtw");
    BOOST_TEST(string(input.av[7]) == "ls");
    BOOST_TEST(input.av[input.ac] == nullptr);
}

BOOST_AUTO_TEST_CASE(load_man_page_ls) {
    Input input(in);
    string man_page = load_man_page(input.ac, input.av);
    BOOST_TEST(man_page.length() > 0);
}

BOOST_AUTO_TEST_SUITE(coreutils) // (coreutils 9.12)
// ls
BOOST_AUTO_TEST_CASE(ls) {
    vector<string> expected_short = {
        "-a", "-A", " ", "-b",
        " ", "-B", "-c", "-C",
        " ", "-d", "-D", "-f",
        "-F", " ", " ", " ",
        "-g", " ", "-G", "-h",
        " ", "-H", " ", " ",
        " ", " ", "-i", "-I",
        "-k", "-l", "-L", "-m",
        "-n", "-N", "-o", "-p",
        "-q", " ", "-Q", " ",
        "-r", "-R", "-s", "-S",
        " ", " ", " ", "-t",
        "-T", "-u", "-U", "-v",
        "-w", "-x", "-X", "-Z",
        " ", "-1", " ", " "
    };
    vector<string> expected_long = {
        "--all", "--almost-all", "--author", "--escape", 
        "--block-size=SIZE", "--ignore-backups", " ", " ", 
        "--color[=WHEN]", "--directory", "--dired", " ", 
        "--classify[=WHEN]", "--file-type", "--format=WORD", "--full-time", 
        " ", "--group-directories-first", "--no-group", "--human-readable", 
        "--si", "--dereference-command-line", "--dereference-command-line-symlink-to-dir", "--hide=PATTERN", 
        "--hyperlink[=WHEN]", "--indicator-style=WORD", "--inode", "--ignore=PATTERN", 
        "--kibibytes", " ", "--dereference", " ", 
        "--numeric-uid-gid", "--literal", " ", "--indicator-style=slash", 
        "--hide-control-chars", "--show-control-chars", "--quote-name", "--quoting-style=WORD", 
        "--reverse", "--recursive", "--size", " ",
        "--sort=WORD", "--time=WORD", "--time-style=TIME_STYLE", " ", 
        "--tabsize=COLS", " ", " ", " ",
        "--width=COLS", " ", " ", "--context", 
        "--zero", " ", "--help", "--version"
    };

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());
    BOOST_TEST(expected_short.size() == expected_long.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s ls", expected_short},
        {"flag -l ls", expected_long},
        {"flag -m ls", expected_mixed},
        {"flag -M ls", expected_mixed_only},
    };
    check_cases(cases);
}

// id
BOOST_AUTO_TEST_CASE(id) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-a", "-Z", "-g", "-G",
        "-n", "-r", "-u", "-z",
        " ", " "
    };
    vector<string> expected_long = {
        " ", "--context", "--group", "--groups", 
        "--name", "--real", "--user", "--zero", 
        "--help", "--version"
    };

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_short.size() == expected_long.size());
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s id", expected_short},
        {"flag -l id", expected_long},
        {"flag -m id", expected_mixed},
        {"flag -M id", expected_mixed_only},
    };
    check_cases(cases);
}

// wc
BOOST_AUTO_TEST_CASE(wc) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-c", "-m", "-l", " ",
        " ", "-L", "-w", " ",
        " ", " "
    };
    vector<string> expected_long = {
        "--bytes", "--chars", "--lines", "--debug",
        "--files0-from=F", "--max-line-length", "--words", "--total=WHEN",
        "--help", "--version"
    };
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s wc", expected_short},
        {"flag -l wc", expected_long},
        {"flag -m wc", expected_mixed},
        {"flag -M wc", expected_mixed_only},
    };
    check_cases(cases);
}

// head
BOOST_AUTO_TEST_CASE(head) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-c", "-n", "-q", "-v",
        "-z", " ", " "
    };
    vector<string> expected_long = {
        "--bytes=[-]NUM", "--lines=[-]NUM", "--quiet", "--verbose", // -q, --quiet, --silent ???
        "--zero-terminated", "--help", "--version"
    };
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s head", expected_short},
        {"flag -l head", expected_long},
        {"flag -m head", expected_mixed},
        {"flag -M head", expected_mixed_only},
    };
    check_cases(cases);
}

// tail
BOOST_AUTO_TEST_CASE(tail) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-c", " ", "-f", "-F",
        "-n", " ", " ", "-q",
        " ", "-s", "-v", "-z",
        " ", " "
    };
    vector<string> expected_long = {
        "--bytes=[+]NUM", "--debug", "--follow[={name|descriptor}]", " ",
        "--lines=[+]NUM", "--max-unchanged-stats=N", "--pid=PID", "--quiet", // --quiet, --silent ???
        "--retry", "--sleep-interval=N", "--verbose", "--zero-terminated",
        "--help", "--version"
    };
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s tail", expected_short},
        {"flag -l tail", expected_long},
        {"flag -m tail", expected_mixed},
        {"flag -M tail", expected_mixed_only},
    };
    check_cases(cases);
}

// date 
BOOST_AUTO_TEST_CASE(date) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-d", " ", "-f", "-I[FMT]",
        " ", "-R", " ", "-r",
        "-s", "-u", " ", " "
    };
    vector<string> expected_long = {
        "--date=STRING", "--debug", "--file=DATEFILE", "--iso-8601[=FMT]",
        "--resolution", "--rfc-email", "--rfc-3339=FMT", "--reference=FILE",
        "--set=STRING", "--utc", "--help", "--version" // -u, --utc, --universal ???
    };
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s date", expected_short},
        {"flag -l date", expected_long},
        {"flag -m date", expected_mixed},
        {"flag -M date", expected_mixed_only},
    };
    check_cases(cases);
}

// od 
BOOST_AUTO_TEST_CASE(od) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-A", " ", "-j", "-N",
        "-S BYTES", "-t", "-v", "-w[BYTES]",
        " ", " ", " ", "-a",
        "-b", "-c", "-d", "-f",
        "-i", "-l", "-o", "-s",
        "-x"
    };
    vector<string> expected_long = {
        "--address-radix=RADIX", "--endian={big|little}", "--skip-bytes=BYTES", "--read-bytes=BYTES",
        "--strings[=BYTES]", "--format=TYPE", "--output-duplicates", "--width[=BYTES]",
        "--traditional", "--help", "--version", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " "
    };
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s od", expected_short},
        {"flag -l od", expected_long},
        {"flag -m od", expected_mixed},
        {"flag -M od", expected_mixed_only},
    };
    check_cases(cases);
}

// test
BOOST_AUTO_TEST_CASE(test) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        " ", " ", "-n STRING", "-z STRING",
        "-b FILE", "-c FILE", "-d FILE", "-e FILE",
        "-f FILE", "-g FILE", "-G FILE", "-h FILE",
        "-k FILE", "-L FILE", "-N FILE", "-O FILE",
        "-p FILE", "-r FILE", "-s FILE", "-S FILE",
        "-t FD", "-u FILE", "-w FILE", "-x FILE"
    };
    vector<string> expected_long = {
        "--help", "--version", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " "
    };
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s test", expected_short},
        {"flag -l test", expected_long},
        {"flag -m test", expected_mixed},
        {"flag -M test", expected_mixed_only}
    };
    check_cases(cases);
}

// echo
BOOST_AUTO_TEST_CASE(echo) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-n", "-e", "-E", " ",
        " "
    };
    vector<string> expected_long = {
        " ", " ", " ", "--help",
        "--version"
    };
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s echo", expected_short},
        {"flag -l echo", expected_long},
        {"flag -m echo", expected_mixed}
    };
    check_cases(cases);
    BOOST_CHECK_THROW(commands_for("flag -M echo"), std::string);
}
BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(base)
// bzip2 (1.0.8)
BOOST_AUTO_TEST_CASE(bzip2) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-c", "-d", "-z", "-t",
        "-f", "-k", "-s", "-q",
        "-v", "-L", "-1", " "
    };
    vector<string> expected_long = {
        "--stdout", "--decompress", "--compress", "--test",
        "--force", "--keep", "--small", "--quiet",
        "--verbose", "--license", " ", "--repetitive-fast" 	// -L --license -V --version ???
															// -1 (or --fast) to -9 (or --best) ???
															// --repetitive-fast --repetitive-best ???
    };
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s bzip2", expected_short},
        {"flag -l bzip2", expected_long},
        {"flag -m bzip2", expected_mixed},
        {"flag -M bzip2", expected_mixed_only},
    };
    check_cases(cases);
}

// grep (3.12-modified)
BOOST_AUTO_TEST_CASE(grep) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        " ", "-V", "-E", "-F",
        "-G", "-P", "-e PATTERNS", "-f FILE",
        "-i", " ", "-v", "-w",
        "-x", "-c", " ", "-L",
        "-l", "-m NUM", "-o", "-q",
        "-s", "-b", "-H", "-h",
        " ", "-n", "-T", "-Z",
        "-A NUM", "-B NUM", "-C NUM", " ",
        " ", "-a", " ", "-D ACTION",
        "-d ACTION", " ", " ", " ",
        "-I", " ", "-r", "-R",
        " ", "-U", "-z"
    };
    vector<string> expected_long = {
        "--help", "--version", "--extended-regexp", "--fixed-strings",
        "--basic-regexp", "--perl-regexp", "--regexp=PATTERNS", "--file=FILE",
        "--ignore-case", "--no-ignore-case", "--invert-match", "--word-regexp",
        "--line-regexp", "--count", "--color[=WHEN]", "--files-without-match", // --color[=WHEN], --colour[=WHEN] ???
        "--files-with-matches", "--max-count=NUM", "--only-matching", "--quiet", // -q, --quiet, --silent ???
        "--no-messages", "--byte-offset", "--with-filename", "--no-filename",
        "--label=LABEL", "--line-number", "--initial-tab", "--null",
        "--after-context=NUM", "--before-context=NUM", " ", "--group-separator=SEP", // -C NUM, -NUM, --context=NUM ???
        "--no-group-separator", "--text", "--binary-files=TYPE", "--devices=ACTION",
        "--directories=ACTION", "--exclude=GLOB", "--exclude-from=FILE", "--exclude-dir=GLOB",
        " ", "--include=GLOB", "--recursive", "--dereference-recursive",
        "--line-buffered", "--binary", "--null-data"
    };
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s grep", expected_short},
        {"flag -l grep", expected_long},
        {"flag -m grep", expected_mixed},
        {"flag -M grep", expected_mixed_only},
    };
    check_cases(cases);
}

// file (5.48)
BOOST_AUTO_TEST_CASE(file) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        " ", "-b", "-C", "-c",
        "-d", "-E", "-e", " ",
        " ", "-F", "-f", "-h",
        "-i", " ", "-k", "-l",
        "-L", "-m", "-N", "-n",
        "-p", "-P", "-r", "-s",
        "-S", "-v", "-z", "-Z",
        "-0", " "
    };
    vector<string> expected_long = {
        "--apple", "--brief", "--compile", "--checking-printout",
        " ", " ", "--exclude testname", "--exclude-quiet",
        "--extension", "--separator separator", "--files-from namefile", "--no-dereference",
        "--mime", "--mime-type", "--keep-going", "--list", // --mime-type, --mime-encoding ???
        "--dereference", "--magic-file magicfiles", "--no-pad", "--no-buffer",
        "--preserve-date", "--parameter name=value", "--raw", "--special-files",
        "--no-sandbox", "--version", "--uncompress", "--uncompress-noreport",
        "--print0", "--help"
    };
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s file", expected_short},
        {"flag -l file", expected_long},
        {"flag -m file", expected_mixed},
        {"flag -M file", expected_mixed_only},
    };
    check_cases(cases);
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(network)
// ifconfig (net-tools 2.10)
BOOST_AUTO_TEST_CASE(ifconfig) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-a", "-s", "-v"
    };
    vector<string> expected_long = {
        " ", " ", " "
    };
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s ifconfig", expected_short},
        {"flag -l ifconfig", expected_long},
        {"flag -m ifconfig", expected_mixed},
        {"flag -M ifconfig", expected_mixed_only},
    };
    check_cases(cases);
}

// arp (net-tools 2.10)
BOOST_AUTO_TEST_CASE(arp) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-v", "-n", "-H type", "-a", // -H type, --hw-type type, -t type ???
        "-e", "-D", "-i If", "-f filename"
    };
    vector<string> expected_long = {
        "--verbose", "--numeric", "--hw-type type", " ",
        " ", "--use-device", "--device If", "--file filename",
    };
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s arp", expected_short},
        {"flag -l arp", expected_long},
        {"flag -m arp", expected_mixed},
        {"flag -M arp", expected_mixed_only},
    };
    check_cases(cases);
}

// tcpdump (4.99.7)
BOOST_AUTO_TEST_CASE(tcpdump) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-A", "-b", "-B buffer_size", "-c count",
        " ", "-C file_size", "-d", "-dd",
        "-ddd", "-D", "-e", "-E", 
        "-f", "-F file", "-g", "-G rotate_seconds", 
        "-h", " ", "-H", "-i interface",
        "-I", " ", "-j tstamp_type", "-J", 
        " ", " ", " ", "-K", 
        "-l", "-L", "-m module", "-M secret", 
        "-n", "-N", "-#", "-O", 
        "-p", " ", "-Q direction", "-q", 
        "-r file", "-S", "-s snaplen", "-T type", 
        "-t", "-tt", "-ttt", "-tttt", 
        "-ttttt", "-u", "-U", "-v", 
        "-vv", "-vvv", "-V file", "-w file", 
        "-W filecount", "-x", "-xx", "-X", 
        "-XX", "-y datalinktype", "-z postrotate-command", "-Z user"
    };
    vector<string> expected_long = {
        " ", " ", "--buffer-size=buffer_size", " ",
        "--count", " ", " ", " ",
        " ", "--list-interfaces", " ", " ",
        " ", " ", "--ip-oneline", " ",
        "--help", "--version", " ", "--interface=interface",
        "--monitor-mode", "--immediate-mode", "--time-stamp-type=tstamp_type", "--list-time-stamp-types",
        "--time-stamp-precision=tstamp_precision", "--micro", "--nano", "--dont-verify-checksums", 
        " ", "--list-data-link-types", " ", " ", 
        " ", " ", "--number", "--no-optimize", 
        "--no-promiscuous-mode", "--print", "--direction=direction", " ", 
        " ", "--absolute-tcp-sequence-numbers", "--snapshot-length=snaplen", " ", 
        " ", " ", " ", " ", 
        " ", " ", "--packet-buffered", " ", 
        " ", " ", " ", " ", 
        " ", " ", " ", " ", 
        " ", "--linktype=datalinktype", " ", "--relinquish-privileges=user"
    };
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s tcpdump", expected_short},
        {"flag -l tcpdump", expected_long},
        {"flag -m tcpdump", expected_mixed},
        {"flag -M tcpdump", expected_mixed_only},
    };
    check_cases(cases);
}

// ping (iputils 20250605)
BOOST_AUTO_TEST_CASE(ping) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-3", "-4", "-6", "-a",
        "-A", "-b", "-B", "-c count",
        "-C", "-d", "-D", "-e identifier",
        "-f", "-F flow label", "-h", "-H",
        "-i interval", "-I interface", "-l preload", "-L",
        "-m mark", "-M pmtudisc_opt", "-N nodeinfo_option", "-n",
        "-O", "-p pattern", "-q", "-Q tos",
        "-r", "-R", "-s packetsize", "-S sndbuf",
        "-t ttl", "-T timestamp option", "-U", "-v",
        "-V", "-w deadline", "-W timeout" 
    };
    vector<string> expected_long = {
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " "
    };
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s ping", expected_short},
        {"flag -l ping", expected_long},
        {"flag -m ping", expected_mixed},
        {"flag -M ping", expected_mixed_only},
    };
    check_cases(cases);
}

// traceroute (2.1.6)
BOOST_AUTO_TEST_CASE(traceroute) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        " ", "-4", "-I", "-T", // -4, -6 ???
        "-d", "-F", "-f first_ttl", "-g gateway",
        "-i interface", "-m max_ttl", "-N squeries", "-n",
        "-p port", "-t tos", "-l flow_label", "-w max[,here,near]",
        "-q nqueries", "-r", "-s source_addr", "-z sendwait",
        "-e", "-A", "-V", " ",
        " ", "-M method", "-O option", "-U",
        "-UL", "-D", "-P protocol", " ",
        " "
    };
    vector<string> expected_long = {
        "--help", " ", "--icmp", "--tcp",
        "--debug", "--dont-fragment", "--first=first_ttl", "--gateway=gateway",
        "--interface=interface", "--max-hops=max_ttl", "--sim-queries=squeries", " ",
        "--port=port", "--tos=tos", "--flowlabel=flow_label", "--wait=max[,here,near]", 
        "--queries=nqueries", " ", "--source=source_addr", "--sendwait=sendwait", 
        "--extensions", "--as-path-lookups", "--version", "--sport=port", 
        "--fwmark=mark", "--module=name", "--options=options", "--udp", 
        " ", "--dccp", "--protocol=protocol", "--mtu", 
        "--back"
    };
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s traceroute", expected_short},
        {"flag -l traceroute", expected_long},
        {"flag -m traceroute", expected_mixed},
        {"flag -M traceroute", expected_mixed_only},
    };
    check_cases(cases);
}

// netstat (net-tools 2.10)
BOOST_AUTO_TEST_CASE(netstat) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-r", "-g", "-i", "-M",
        "-s", "-v", "-W", "-n",
        " ", " ", " ", "-A",
        "-c", "-e", "-o", "-p",
        "-l", "-a", "-F", "-C"
    };
    vector<string> expected_long = {
        "--route", "--groups", "--interfaces", "--masquerade",
        "--statistics", "--verbose", "--wide", "--numeric",
        "--numeric-hosts", "--numeric-ports", "--numeric-users", "--protocol=family",
        "--continuous", "--extend", "--timers", "--program",
        "--listening", "--all", " ", " "
    };
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s netstat", expected_short},
        {"flag -l netstat", expected_long},
        {"flag -m netstat", expected_mixed},
        {"flag -M netstat", expected_mixed_only},
    };
    check_cases(cases);
}

// nc (1.101)
BOOST_AUTO_TEST_CASE(nc) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-4", "-6", "-b", "-C",
        "-D", "-d", "-F", "-h",
        "-I length", "-i interval", "-k", "-l",
        "-M ttl", "-m minttl", "-N", "-n",
        "-O length", "-P proxy_username", "-p source_port", "-q seconds",
        "-r", "-S", "-s sourceaddr", "-T keyword",
        "-t", "-U", "-u", "-V rtable",
        "-v", "-W recvlimit", "-w timeout", "-X proxy_protocol",
        "-x proxy_address[:port]", "-Z", "-z" 
    };
    vector<string> expected_long = {
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " "
    };
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s nc", expected_short},
        {"flag -l nc", expected_long},
        {"flag -m nc", expected_mixed},
        {"flag -M nc", expected_mixed_only},
    };
    check_cases(cases);
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(other)
// git (2.56.0)
BOOST_AUTO_TEST_CASE(git) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-v", "-h", "-C <path>", "-c <name>=<value>",
        " ", " ", " ", " ",
        " ", "-p", "-P", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " "
    };
    vector<string> expected_long = {
        "--version", "--help", " ", " ",
        "--config-env=<name>=<envvar>", "--exec-path[=<path>]", "--html-path", "--man-path",
        "--info-path", "--paginate", "--no-pager", "--git-dir=<path>",
        "--work-tree=<path>", "--namespace=<path>", "--bare", "--no-replace-objects",
        "--no-lazy-fetch", "--no-optional-locks", "--no-advice", "--literal-pathspecs",
        "--glob-pathspecs", "--noglob-pathspecs", "--icase-pathspecs", "--list-cmds=<group>[,<group>...]",
        "--attr-source=<tree-ish>"
    };
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s git", expected_short},
        {"flag -l git", expected_long},
        {"flag -m git", expected_mixed},
        {"flag -M git", expected_mixed_only},
    };
    check_cases(cases);
}

// docker (29.9.2)
BOOST_AUTO_TEST_CASE(docker) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        " ", " ", "-D", "-H",
        "-l", " ", " ", " ",
        " ", " ", "-v"
    };
    vector<string> expected_long = {
        "--help", "--config=\"\"", "--debug=true|false", "--host=[unix:///var/run/docker.sock]",
        "--log-level=\"debug|info|warn|error|fatal\"", "--tls=true|false", "--tlscacert=~/.docker/ca.pem", "--tlscert=~/.docker/cert.pem", 
        "--tlskey=~/.docker/key.pem", "--tlsverify=true|false", "--version=true|false"
    };
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s docker", expected_short},
        {"flag -l docker", expected_long},
        {"flag -m docker", expected_mixed},
        {"flag -M docker", expected_mixed_only},
    };
    check_cases(cases);
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
