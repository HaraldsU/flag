#define BOOST_TEST_MODULE Flag 
#include "flag.h"
#include <boost/test/included/unit_test.hpp>
#include <wordexp.h>

using namespace std;

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

void remove_spaces(vector<string> & v) {
    v.erase(std::remove(v.begin(), v.end(), " "), v.end());
}

struct CaptureCout {
    ostringstream sink;
    streambuf* old = cout.rdbuf(sink.rdbuf());
    ~CaptureCout() { cout.rdbuf(old); }
    string str() const { return sink.str(); }
};

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
    Args() { }

    void reset_globals_and_set_options(string command) {
        reset_globals();
        Input input(("flag " + command).c_str());
        set_program_options(input.ac, input.av);
    }

    vector<string> get_command_flags(
        const string command, 
        const string man_page
    ) {
        reset_globals_and_set_options(command.c_str());
        vector<string> actual {};
        get_flags(actual, man_page);
        return actual;
    }

    void test_flags(
        string com, 
        string man_page, 
        vector<string> exp,
        vector<string> expected_short, 
        vector<string> expected_long,
        vector<string> expected_mixed,
        vector<string> expected_mixed_only 
    ) {
        BOOST_TEST_CONTEXT("command: " << com) {
            if (exp.size() == 0) BOOST_CHECK_THROW(get_command_flags(com, man_page), string);
            else {
                auto actual = get_command_flags(com, man_page);
                // Test --[s]hort, --[l]ong, --[m]ixed and --[M]ixed-only flags
                test_modes(exp, actual);

                // Test -S|--sort flag
                test_sort(actual);

                // Test --[r]everse flag
                test_reverse(actual);

                // Test --[w]ord-count flag
                test_flag_count(com, expected_short, expected_long, expected_mixed, expected_mixed_only);
            }
        }
    }

    void test_modes(vector<string> exp, vector<string> actual) {
        BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), exp.begin(), exp.end());
        if (actual.size() < exp.size()) BOOST_TEST_MESSAGE("missing at end: " << exp[actual.size()]);
        if (actual.size() > exp.size()) BOOST_TEST_MESSAGE("extra at end: " << actual[exp.size()]);
    }

    // Not really testing if the sort happens but I would need ground truth values to do that ...
    void test_sort(vector<string> actual) {
        set<string, ilexic> sorted;
        for (auto el : actual) {
            sorted.insert(el);
        }
        BOOST_CHECK_EQUAL(actual.size(), sorted.size());
    }

    void test_reverse(vector<string> actual) {
        IS_REVERSE = true;

        vector<string> reversed;
        for (auto el : actual) {
            reversed.insert(reversed.begin(), el);
        }
        BOOST_CHECK_EQUAL(actual.size(), reversed.size());
        BOOST_CHECK_EQUAL(actual[0], reversed[reversed.size() - 1]);

        IS_REVERSE = false;
    }

    void test_delimiter() {

    }

    void for_all_modes(
        const string command, 
        const string man_page,
        vector<string> expected_short, 
        vector<string> expected_long,
        vector<string> expected_mixed,
        vector<string> expected_mixed_only 
    ) {
        BOOST_TEST(expected_short.size() == expected_long.size());
        BOOST_TEST(expected_long.size() == expected_mixed.size());

        remove_spaces(expected_short);
        remove_spaces(expected_long);

        const vector<pair<string, vector<string>>> cases = {
            {string{"flag -s " + command}, expected_short},
            {string{"flag -l " + command}, expected_long},
            {string{"flag -m " + command}, expected_mixed},
            {string{"flag -M " + command}, expected_mixed_only}
        };

        for (const auto& [com, exp] : cases) {
            test_flags(com, man_page, exp, expected_short, expected_long, expected_mixed, expected_mixed_only);
        }
    }

    void test_title(const string title, const string man_page) {
        BOOST_CHECK_EQUAL(print_title(man_page), title);
    }

    void test_flag_count(
        string command,
        vector<string> expected_short, 
        vector<string> expected_long,
        vector<string> expected_mixed,
        vector<string> expected_mixed_only
    ) {
        if (command.find("flag -s ") != std::string::npos) {
            BOOST_CHECK_EQUAL(expected_short.size(), FLAG_COUNT[Mode::Short]);
        }
        else if (command.find("flag -l ") != std::string::npos) {
            BOOST_CHECK_EQUAL(expected_long.size(), FLAG_COUNT[Mode::Long]);
        }
        else if (command.find("flag -m ") != std::string::npos) {
            BOOST_CHECK_EQUAL(
                expected_mixed.size(), 
                FLAG_COUNT[Mode::Short] + FLAG_COUNT[Mode::Long] + FLAG_COUNT[Mode::Mixed]
            );
        }
        else if (command.find("flag -M ") != std::string::npos) {
            BOOST_CHECK_EQUAL(expected_mixed_only.size(), FLAG_COUNT[Mode::Mixed]);
        }
        else cout << "FLAG COUNT TEST ERROR" << endl;
    }

    void test_command (
        string command, 
        const string title,
        vector<string> expected_short, 
        vector<string> expected_long
    ) {
        reset_globals_and_set_options(command);
        auto man_page = set_man_page();

        vector<string> expected_mixed = {};
        vector<string> expected_mixed_only = {};
        make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);

        for_all_modes(command, man_page, expected_short, expected_long, expected_mixed, expected_mixed_only);

        // Test -t flag
        test_title(title, man_page);
    }
};

BOOST_FIXTURE_TEST_SUITE(flag_tests, Args)

// Test --[v]ersion flag
BOOST_AUTO_TEST_CASE(version) {
    BOOST_CHECK_EQUAL(print_program_version(), 1);
}

// Test --[h]elp flag
BOOST_AUTO_TEST_CASE(help) {
    BOOST_CHECK_EQUAL(print_help(boost::program_options::options_description{}), 1);
}

// Test -D|--debug flag
BOOST_AUTO_TEST_CASE(debug) {
    BOOST_CHECK_EQUAL(print_debug(), 0);
}

// Test --[d]elimiter flag
BOOST_AUTO_TEST_CASE(delimiter) {
    vector<pair<string, string>> cases {
        {"\n", {}},
        {"\t", {}},
        {"; ", {}},
        {"aboba", {}}
    };

    for (auto & [del, out] : cases) {
        DELIMITER = del;
        string command {"flag -d '" + DELIMITER + "' pwd"};
        reset_globals_and_set_options(command);
        auto man_page = set_man_page();
        auto actual1 = get_command_flags(command, man_page);
        {
            CaptureCout capture;
            print_flags(actual1, man_page);
            out = capture.str();
        }
        BOOST_TEST(out.find(del));
    }
}

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

BOOST_AUTO_TEST_CASE(set_man_page_ls) {
    string command = "flag -s --delimiter '\n' ls";
    Input input(command.c_str());
    set_program_options(input.ac, input.av);
    auto man_page = set_man_page();
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
    string command {"ls"};
    string title {"ls - list directory contents"};
    test_command(command, title, expected_short, expected_long);
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
    string command {"id"};
    string title {"id - print real and effective user and group IDs"};
    test_command(command, title, expected_short, expected_long);
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
    string command {"wc"};
    string title {"wc - word, line, and byte or character count"};
    test_command(command, title, expected_short, expected_long);
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
    string command {"head"};
    string title {"head - output the first part of files"};
    test_command(command, title, expected_short, expected_long);
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
    string command {"tail"};
    string title {"tail - output the last part of files"};
    test_command(command, title, expected_short, expected_long);
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
    string command {"date"};
    string title {"date - print or set the system date and time"};
    test_command(command, title, expected_short, expected_long);
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
    string command {"od"};
    string title {"od - dump files in octal and other formats"};
    test_command(command, title, expected_short, expected_long);
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
    string command {"test"};
    string title {"test - check file types and compare values"};
    test_command(command, title, expected_short, expected_long);
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
    string command {"echo"};
    string title {"echo - display a line of text"};
    test_command(command, title, expected_short, expected_long);
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
    string command {"bzip2"};
    string title {"bzip2, bunzip2 - a block-sorting file compressor, v1.0.8"};
    test_command(command, title, expected_short, expected_long);
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
    string command {"grep"};
    string title {"grep - print lines that match patterns"};
    test_command(command, title, expected_short, expected_long);
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
    // test_command(cases);
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
    string command {"ifconfig"};
    string title {"ifconfig - configure a network interface"};
    test_command(command, title, expected_short, expected_long);
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
    string command {"arp"};
    string title {"arp - manipulate the system ARP cache"};
    test_command(command, title, expected_short, expected_long);
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
    string command {"tcpdump"};
    string title {"tcpdump - dump traffic on a network"};
    test_command(command, title, expected_short, expected_long);
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
    string command {"ping"};
    string title {"ping - send ICMP ECHO_REQUEST to network hosts"};
    test_command(command, title, expected_short, expected_long);
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
    string command {"traceroute"};
    string title {"traceroute - print the route packets trace to network host"};
    test_command(command, title, expected_short, expected_long);
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
    string command {"netstat"};
    string title {"netstat - Print network connections, routing tables, interface statistics, masquerade connections, and multicast memberships"};
    test_command(command, title, expected_short, expected_long);
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
    string command {"nc"};
    string title {"nc —— arbitrary TCP and UDP connections and listens"};
    test_command(command, title, expected_short, expected_long);
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
    string command {"git"};
    string title {"git - the stupid content tracker"};
    test_command(command, title, expected_short, expected_long);
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
    string command {"docker"};
    string title {"docker - Docker image and container command line interface"};
    test_command(command, title, expected_short, expected_long);
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
