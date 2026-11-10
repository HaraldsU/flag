#define BOOST_TEST_MODULE Flag 
#include "flag.h"
#include <boost/test/included/unit_test.hpp>
#include <boost/algorithm/string/split.hpp>
#include <wordexp.h>

using namespace std;

void make_mixed_from_short_and_long (
    const vector<string> exp_s, 
    const vector<string> exp_l,
    vector<string> & exp_m,
    vector<string> & exp_mo
) {
    for (size_t i = 0; i < exp_s.size(); i++) {
        auto es_ = exp_s[i];
        auto el_ = exp_l[i];
        if (es_ != " " and el_ != " ") {
            exp_m.push_back(es_ + "," + el_);
            exp_mo.push_back(es_ + "," + el_);
        } 
        else if (es_ != " ") exp_m.push_back(es_);
        else if (el_ != " ") exp_m.push_back(el_);
    }
}

void remove_spaces(vector<string> & v) {
    v.erase(std::remove(v.begin(), v.end(), " "), v.end());
}

void reset_globals() {
    VM.clear();
    MODE          = Mode::Mixed;
    IS_MIXED_ONLY = false;
    IS_REVERSE    = false;
    ENCLOSE_COUNT = 2;
    IS_WORD_COUNT = false;
    IS_TITLE      = false;
    IS_SORT       = false;
    IS_DEBUG      = false;
    FLAG_COUNT.clear();
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

    int reset_globals_and_set_options(string command) {
        reset_globals();
        Input input(("flag " + command).c_str());
        return set_program_options(input.ac, input.av);
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

    // Test the --[s]hort, --[l]ong, --[m]ixed and --[M]ixed-only flags
    void test_modes(
        vector<string> exp, 
        vector<string> act
    ) {
        BOOST_CHECK_EQUAL_COLLECTIONS(act.begin(), act.end(), exp.begin(), exp.end());
        if (act.size() < exp.size()) BOOST_TEST_MESSAGE("missing at end: " << exp[act.size()]);
        if (act.size() > exp.size()) BOOST_TEST_MESSAGE("extra at end: " << act[exp.size()]);
    }

    // Test the --[t]itle flag
    void test_title(
        const string title, 
        const string man_page
    ) {
        BOOST_TEST(print_title(man_page) == title);
    }

    // Test the --[w]ord-count flag
    void test_flag_count(
        string command,
        vector<string> exp_s, 
        vector<string> exp_l,
        vector<string> exp_m,
        vector<string> exp_mo
    ) {
        if (command.find("flag -s ") != std::string::npos) BOOST_TEST(exp_s.size() == FLAG_COUNT[Mode::Short]);
        else if (command.find("flag -l ") != std::string::npos) BOOST_TEST(exp_l.size() == FLAG_COUNT[Mode::Long]);
        else if (command.find("flag -m ") != std::string::npos) BOOST_TEST( exp_m.size() == FLAG_COUNT[Mode::Short] + FLAG_COUNT[Mode::Long] + FLAG_COUNT[Mode::Mixed]);
        else if (command.find("flag -M ") != std::string::npos) BOOST_TEST(exp_mo.size() == FLAG_COUNT[Mode::Mixed]);
        else cout << "FLAG COUNT TEST ERROR" << endl;
    }

    void test_flags(
        string command, 
        string man_page, 
        vector<string> exp,
        vector<string> exp_s, 
        vector<string> exp_l,
        vector<string> exp_m,
        vector<string> exp_mo 
    ) {
        BOOST_TEST_CONTEXT("command: " << command) {
            if (exp.size() == 0) BOOST_CHECK_THROW(get_command_flags(command, man_page), string);
            else {
                auto actual = get_command_flags(command, man_page);

                test_modes(exp, actual);
                test_flag_count(command, exp_s, exp_l, exp_m, exp_mo);
            }
        }
    }

    void for_all_modes(
        const string command, 
        const string man_page,
        vector<string> exp_s, 
        vector<string> exp_l,
        vector<string> exp_m,
        vector<string> exp_mo 
    ) {
        BOOST_TEST(exp_s.size() == exp_l.size());
        BOOST_TEST(exp_l.size() == exp_m.size());

        remove_spaces(exp_s);
        remove_spaces(exp_l);

        const vector<pair<string, vector<string>>> cases = {
            {string{"flag -s " + command}, exp_s},
            {string{"flag -l " + command}, exp_l},
            {string{"flag -m " + command}, exp_m},
            {string{"flag -M " + command}, exp_mo}
        };

        for (const auto& [com, exp] : cases) 
            test_flags(com, man_page, exp, exp_s, exp_l, exp_m, exp_mo);
    }

    void test_command (
        string command, 
        const string title,
        vector<string> exp_s, 
        vector<string> exp_l
    ) {
		try {
            reset_globals_and_set_options(command);
			vector<string> expected_mixed = {};
			vector<string> expected_mixed_only = {};
			make_mixed_from_short_and_long(exp_s, exp_l, expected_mixed, expected_mixed_only);

			auto man_page = set_man_page();
			for_all_modes(command, man_page, exp_s, exp_l, expected_mixed, expected_mixed_only);
			test_title(title, man_page);
		} catch (const string & em) {
			BOOST_CHECK_THROW(set_man_page(), string);
		}
    }
};

BOOST_FIXTURE_TEST_SUITE(flag_tests, Args)

// Test the --color flag
BOOST_AUTO_TEST_CASE(color) {
    ENCLOSE_COUNT = 0;
    string flag_s = "-a";
    add_colors(flag_s, 0);
    BOOST_TEST(flag_s == "\033[0m\033[31m-a\033[0m");

    string flag_l = "--all";
    add_colors(flag_l, 0);
    BOOST_TEST(flag_l == "\033[0m\033[32m--all\033[0m");

    string flag_m = "-a|--all";
    add_colors(flag_m, 2);
    BOOST_TEST(flag_m == "\033[0m\033[31m-a\033[33m|\033[32m--all\033[0m");

    ENCLOSE_COUNT = 1;
    string flag_me1 = "@-a|--all@";
    add_colors(flag_me1, 3);
    BOOST_TEST(flag_me1 == "\033[0m\033[33m@\033[31m-a\033[33m|\033[32m--all\033[33m@\033[0m");

    ENCLOSE_COUNT = 2;
    string flag_me2 = "[-a|--all]";
    add_colors(flag_me2, 3);
    BOOST_TEST(flag_me2 == "\033[0m\033[33m[\033[31m-a\033[33m|\033[32m--all\033[33m]\033[0m");
}

// Test the --[c]olumn flag
BOOST_AUTO_TEST_CASE(column) {
    string command1 {"flag --column 1 --delimiter '" + DELIMITER + "' ls"};
    reset_globals_and_set_options(command1);
    auto man_page = set_man_page();
    auto actual1 = get_command_flags(command1, man_page);
    string out1;
    {
        CaptureCout capture;
        print_flags_helper(actual1);
        out1 = capture.str();
    }
    BOOST_TEST(!VM["column"].defaulted());

    string command2 {"flag --column 3 -delimiter '" + DELIMITER + "' ls"};
    reset_globals_and_set_options(command2);
    auto actual2 = get_command_flags(command2, man_page);
    string out2;
    {
        CaptureCout capture;
        print_flags_helper(actual2);
        out2 = capture.str();
    }
    BOOST_TEST(!VM["column"].defaulted());

    BOOST_TEST(out1.size() != out2.size());
}

// Test the --[d]elimiter flag
BOOST_AUTO_TEST_CASE(delimiter) {
    vector<pair<string, string>> cases {
        {"\n", {}},
        {"\t", {}},
        {"aboba", {}}
    };

    for (auto & [del, out] : cases) {
        DELIMITER = del;
        string command {"flag --delimiter '" + DELIMITER + "' pwd"};
        reset_globals_and_set_options(command);
        auto man_page = set_man_page();
        auto actual1 = get_command_flags(command, man_page);
        {
            CaptureCout capture;
            print_flags(actual1, man_page);
            out = capture.str();
        }
        BOOST_TEST(!VM["delimiter"].defaulted());
        BOOST_TEST(out.find(del));
        vector<boost::iterator_range<string::iterator>> hits;
        boost::algorithm::find_all(hits, out, del);
        if (del == "\n") BOOST_TEST(actual1.size() == hits.size());
        else BOOST_TEST(actual1.size() - 1 == hits.size()); // If we have a delimiter like ',' it is removed from the last occurence
    }
}

// Test the -D|--debug flag
BOOST_AUTO_TEST_CASE(debug) {
    string command = "flag --debug ls";
    int rc;
    string out;
    {
        CaptureCout capture;
        rc = reset_globals_and_set_options(command);
        out = capture.str();
    }
    BOOST_TEST(!VM["debug"].defaulted());
    BOOST_TEST(rc == 0);
}

// Test the --[e]nclose flag
BOOST_AUTO_TEST_CASE(enclose) {
    ENCLOSE = "";
    ENCLOSE_COUNT = 0;
    string flag_e0 = "-a|--all";
    add_enclosing(flag_e0);
    BOOST_TEST(flag_e0 == "-a|--all");

    ENCLOSE = "@";
    ENCLOSE_COUNT = 1;
    string flag_e1 = "-a|--all";
    add_enclosing(flag_e1);
    BOOST_TEST(flag_e1 == "@-a|--all@");

    ENCLOSE = "[]";
    ENCLOSE_COUNT = 2;
    string flag_e2 = "-a|--all";
    add_enclosing(flag_e2);
    BOOST_TEST(flag_e2 == "[-a|--all]");
}

// Test the --[h]elp flag
BOOST_AUTO_TEST_CASE(help) {
    string command = "flag --help";
    int rc;
    string out;
    {
        CaptureCout capture;
        rc = reset_globals_and_set_options(command);
        out = capture.str();
    }
    BOOST_TEST(!VM["help"].defaulted());
    BOOST_TEST(rc == ExitCodes::Help);
    BOOST_TEST(out.length() > 0);
}

// Test the --[r]everse flag
BOOST_AUTO_TEST_CASE(reverse) {
    string command1 = "flag wc";
    reset_globals_and_set_options(command1);
    auto man_page = set_man_page();
    auto regular = get_command_flags(command1, man_page);

    string command2 = "flag -r wc";
    reset_globals_and_set_options(command2);
    auto reversed = get_command_flags(command2, man_page);

    for (size_t i = 0; i < regular.size(); i++)
        BOOST_TEST(regular[i] == reversed[regular.size() - i - 1]);
}

// Test the -S|--sort flag
BOOST_AUTO_TEST_CASE(sort) {
    vector<string> sorted_exp_wc = {
         "-c,--bytes", "--debug", "--files0-from=F", "--help", 
         "-l,--lines", "-L,--max-line-length", "-m,--chars", "--total=WHEN", 
         "--version", "-w,--words"
    };

    string command = "flag -S wc";
    reset_globals_and_set_options(command);
    auto man_page = set_man_page();
    auto sorted_act_wc = get_command_flags(command, man_page);

    BOOST_CHECK_EQUAL_COLLECTIONS(sorted_act_wc.begin(), sorted_act_wc.end(), sorted_exp_wc.begin(), sorted_exp_wc.end());
}

// Test the --[v]ersion flag
BOOST_AUTO_TEST_CASE(version) {
    string command = "flag --version";
    int rc;
    string out;
    {
        CaptureCout capture;
        rc = reset_globals_and_set_options(command);
        out = capture.str();
    }
    BOOST_TEST(VM.count("version"));
    BOOST_TEST(rc == ExitCodes::Version);
    BOOST_TEST(out == "flag " + VERSION + "\n");
}

BOOST_AUTO_TEST_CASE(string_to_argc_argv) {
    Input input("flag --color=always --enclose [] --column 3 --mixed-only --sort --reverse --title --word-count ls");
    BOOST_TEST(input.ac == 12);
    BOOST_TEST(string(input.av[0]) == "flag");
    BOOST_TEST(string(input.av[1]) == "--color=always");
    BOOST_TEST(string(input.av[2]) == "--enclose");
    BOOST_TEST(string(input.av[3]) == "[]");
    BOOST_TEST(string(input.av[4]) == "--column");
    BOOST_TEST(string(input.av[5]) == "3");
    BOOST_TEST(string(input.av[6]) == "--mixed-only");
    BOOST_TEST(string(input.av[7]) == "--sort");
    BOOST_TEST(string(input.av[8]) == "--reverse");
    BOOST_TEST(string(input.av[9]) == "--title");
    BOOST_TEST(string(input.av[10]) == "--word-count");
    BOOST_TEST(string(input.av[11]) == "ls");
    BOOST_TEST(input.av[input.ac] == nullptr);
}

BOOST_AUTO_TEST_CASE(set_man_page_ls) {
    string command = "flag --short --delimiter '\n' ls";
    Input input(command.c_str());
    {
        CaptureCout capture;
        set_program_options(input.ac, input.av);
    }
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
        "--after-context=NUM", "--before-context=NUM", "--context=NUM", "--group-separator=SEP",
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

BOOST_AUTO_TEST_SUITE(git)
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

// git add
BOOST_AUTO_TEST_CASE(git_add) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-n", "-v", "-f", " ",
        "-i", "-p", "-U<n>", " ",
        "-e", "-u", "-A", " ",
        "-N", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " "
    };
    vector<string> expected_long = {
        "--dry-run", "--verbose", "--force", "--sparse",
        "--interactive", "--patch", "--unified=<n>", "--inter-hunk-context=<n>",
        "--edit", "--update", "--all", "--no-all",
        "--intent-to-add", "--refresh", "--ignore-errors", "--ignore-missing",
        "--no-warn-embedded-repo", "--renormalize", "--resolved", "--chmod=(+|-)x",
        "--pathspec-from-file=<file>", "--pathspec-file-nul"
    };
    string command {"git add"};
    string title {"git-add - Add file contents to the index"};
    test_command(command, title, expected_short, expected_long);
}

// git bisect
BOOST_AUTO_TEST_CASE(git_bisect) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        " ", " ", " "
    };
    vector<string> expected_long = {
        "--no-checkout", "--first-parent", "--reset-when-found[=<where>]"
    };
    string command {"git bisect"};
    string title {"git-bisect - Use binary search to find the commit that introduced a bug"};
    test_command(command, title, expected_short, expected_long);
}

// git branch 
BOOST_AUTO_TEST_CASE(git_branch) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-d", "-D", " ", "-f",
        "-m", "-M", "-c", "-C",
        " ", " ", "-i", " ",
        " ", " ", "-r", "-a",
        "-l", " ", " ", " ",
        "-v", "-q", " ", " ",
        "-t", " ", " ", " ",
        "-u <upstream>", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " "
    };
    vector<string> expected_long = {
        "--delete", " ", "--create-reflog", "--force",
        "--move", " ", "--copy", " ",
        "--color[=<when>]", "--no-color", "--ignore-case", "--omit-empty",
        "--column[=<options>]", "--sort=<key>", "--remotes", "--all", // --column[=<options>], --no-column ???
        "--list", "--show-current", "--delete-merged <pattern>", "--dry-run",
        "--verbose", "--quiet", "--abbrev=<n>", "--no-abbrev",
        "--track[=(direct|inherit)]", "--no-track", "--recurse-submodules", "--set-upstream",
        "--set-upstream-to=<upstream>", "--unset-upstream", "--edit-description", "--contains [<commit>]",
        "--no-contains [<commit>]", "--merged [<commit>]", "--no-merged [<commit>]", "--forked <branch>",
        "--points-at <object>", "--format <format>"
    };
    string command {"git branch"};
    string title {"git-branch - List, create, or delete branches"};
    test_command(command, title, expected_short, expected_long);
}

// git checkout 
BOOST_AUTO_TEST_CASE(git_checkout) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-q", " ", "-f", " ",
        "-b <new-branch>", "-B <new-branch>", "-t", " ",
        " ", "-l", "-d", " ",
        " ", "-m", " ", "-p",
        "-U<n>", " ", " ", " ",
        " ", " ", " ", " "
    };
    vector<string> expected_long = {
        "--quiet", "--progress", "--force", "--ours", // --ours, --theirs etc...
        " ", " ", "--track[=(direct|inherit)]", "--no-track",
        "--guess", " ", "--detach", "--orphan <new-branch>",
        "--ignore-skip-worktree-bits", "--merge", "--conflict=<style>", "--patch",
        "--unified=<n>", "--inter-hunk-context=<n>", "--ignore-other-worktrees", "--overwrite-ignore",
        "--recurse-submodules", "--overlay", "--pathspec-from-file=<file>", "--pathspec-file-nul"
    };
    string command {"git checkout"};
    string title {"git-checkout - Switch branches or restore working tree files"};
    test_command(command, title, expected_short, expected_long);
}

// git clean 
BOOST_AUTO_TEST_CASE(git_clean) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-d", "-f", "-i", "-n",
        "-q", "-e <pattern>", "-x", "-X"
    };
    vector<string> expected_long = {
        " ", "--force", "--interactive", "--dry-run",
        "--quiet", "--exclude=<pattern>", " ", " ",
    };
    string command {"git clean"};
    string title {"git-clean - Remove untracked files from the working tree"};
    test_command(command, title, expected_short, expected_long);
}

// git commit
BOOST_AUTO_TEST_CASE(git_commit) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-a", "-p", "-U<n>", " ",
        "-C <commit>", "-c <commit>", " ", " ",
        " ", " ", " ", " ",
        " ", "-z", "-F <file>", " ",
        " ", "-m <msg>", "-t <file>", "-s",
        " ", "-n", " ", " ",
        " ", "-e", " ", " ",
        " ", "-i", "-o", " ",
        " ", "-u[<mode>]", "-v", "-q",
        " ", " ", " ", "-S[<key-id>]"
    };
    vector<string> expected_long = {
        "--all", "--patch", "--unified=<n>", "--inter-hunk-context=<n>",
        "--reuse-message=<commit>", "--reedit-message=<commit>", "--fixup=[(amend|reword):]<commit>", "--squash=<commit>",
        "--reset-author", "--short", "--branch", "--porcelain",
        "--long", "--null", "--file=<file>", "--author=<author>",
        "--date=<date>", "--message=<msg>", "--template=<file>", "--signoff",
        "--trailer <token>[(=|:)<value>]", "--verify", "--allow-empty", "--allow-empty-message",
        "--cleanup=<mode>", "--edit", "--no-edit", "--amend",
        "--no-post-rewrite", "--include", "--only", "--pathspec-from-file=<file>",
        "--pathspec-file-nul", "--untracked-files[=<mode>]", "--verbose", "--quiet",
        "--dry-run", "--status", "--no-status", "--gpg-sign[=<key-id>]"
    };
    string command {"git commit"};
    string title {"git-commit - Record changes to the repository"};
    test_command(command, title, expected_short, expected_long);
}

// git fetch
BOOST_AUTO_TEST_CASE(git_fetch) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        " ", "-a", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        "-f", "-k", " ", " ",
        " ", " ", "-p", "-P",
        "-n", " ", " ", "-t",
        " ", "-j <n>", " ", " ", 
        " ", " ", "-u", " ", 
        "-q", "-v", " ", "-o <option>", 
        " ", " ", "-4", "-6", 
        " "
    };
    vector<string> expected_long = {
        "--all", "--append", "--atomic", "--depth=<depth>",
        "--deepen=<depth>", "--shallow-since=<date>", "--shallow-exclude=<ref>", "--unshallow",
        "--update-shallow", "--negotiation-restrict=(<commit>|<glob>)", "--negotiation-include=(<commit>|<glob>)", "--negotiate-only",
        "--dry-run", "--porcelain", "--filter=<filter-spec>", "--write-fetch-head",
        "--force", "--keep", "--multiple", "--auto-maintenance",
        "--write-commit-graph", "--prefetch", "--prune", "--prune-tags",
        "--no-tags", "--refetch", "--refmap=<refspec>", "--tags",
        "--recurse-submodules[=(yes|on-demand|no)]", "--jobs=<n>", "--no-recurse-submodules", "--set-upstream",
        "--submodule-prefix=<path>", "--recurse-submodules-default=(yes|on-demand)", "--update-head-ok", "--upload-pack <upload-pack>",
        "--quiet", "--verbose", "--progress", "--server-option=<option>",
        "--show-forced-updates", "--no-show-forced-updates", "--ipv4", "--ipv6",
        "--stdin"
    };
    string command {"git fetch"};
    string title {"git-fetch - Download objects and refs from another repository"};
    test_command(command, title, expected_short, expected_long);
}

// git init 
BOOST_AUTO_TEST_CASE(git_init) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-q", " ", " ", " ",
        " ", " ", "-b <branch-name>", " "
    };
    vector<string> expected_long = {
        "--quiet", "--bare", "--object-format=<format>", "--ref-format=<format>",
        "--template=<template-directory>", "--separate-git-dir=<git-dir>", "--initial-branch=<branch-name>", "--shared[=(false|true|umask|group|all|world|everybody|<perm>)]",
    };
    string command {"git init"};
    string title {"git-init - Create an empty Git repository or reinitialize an existing one"};
    test_command(command, title, expected_short, expected_long);
}

// git merge 
BOOST_AUTO_TEST_CASE(git_merge) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        " ", "-e", " ", " ",
        "-S[<key-id>]", " ", " ", "-n",
        " ", " ", " ", "-s <strategy>",
        "-X <option>", " ", " ", "-q",
        "-v", " ", " ", " ",
        "-m <msg>", " ", "-F <file>", " ",
        " ", " ", " ", " "
    };
    vector<string> expected_long = {
        "--commit", "--edit", "--cleanup=<mode>", "--ff",
        "--gpg-sign[=<key-id>]", "--log[=<n>]", "--signoff", "--stat",
        "--compact-summary", "--squash", "--verify", "--strategy=<strategy>",
        "--strategy-option=<option>", "--verify-signatures", "--summary", "--quiet",
        "--verbose", "--progress", "--autostash", "--allow-unrelated-histories",
        " ", "--into-name <branch>", "--file=<file>", "--rerere-autoupdate",
        "--overwrite-ignore", "--abort", "--quit", "--continue"
    };
    string command {"git merge"};
    string title {"git-merge - Join two or more development histories together"};
    test_command(command, title, expected_short, expected_long);
}

// git pull
BOOST_AUTO_TEST_CASE(git_pull) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-q", "-v", " ", " ",
        "-e", " ", " ", " ",
        "-S[<key-id>]", " ", " ", "-n",
        " ", " ", " ", "-s <strategy>",
        "-X <option>", " ", " ", " ",
        " ", "-r", " ", " ",
        "-a", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", "-f", "-k",
        " ", "-p", " ", " ",
        "-t", "-j <n>", " ", " ",
        " ", "-o <option>", " ", " ",
        "-4", "-6"
    };
    vector<string> expected_long = {
        "--quiet", "--verbose", "--recurse-submodules[=(yes|on-demand|no)]", "--commit",
        "--edit", "--cleanup=<mode>", "--ff-only", "--ff",
        "--gpg-sign[=<key-id>]", "--log[=<n>]", "--signoff", "--stat",
        "--compact-summary", "--squash", "--verify", "--strategy=<strategy>",
        "--strategy-option=<option>", "--verify-signatures", "--summary", "--autostash",
        "--allow-unrelated-histories", "--rebase[=(true|merges|false|interactive)]", "--no-rebase", "--all",
        "--append", "--atomic", "--depth=<depth>", "--deepen=<depth>",
        "--shallow-since=<date>", "--shallow-exclude=<ref>", "--unshallow", "--update-shallow",
        "--negotiation-restrict=(<commit>|<glob>)", "--negotiation-include=(<commit>|<glob>)", "--negotiate-only", "--dry-run",
        "--porcelain", "--filter=<filter-spec>", "--force", "--keep",
        "--prefetch", "--prune", "--no-tags", "--refmap=<refspec>",
        "--tags", "--jobs=<n>", "--set-upstream", "--upload-pack <upload-pack>",
        "--progress", "--server-option=<option>", "--show-forced-updates", "--no-show-forced-updates",
        "--ipv4", "--ipv6"
    };
    string command {"git pull"};
    string title {"git-pull - Fetch from and integrate with another repository or a local branch"};
    test_command(command, title, expected_short, expected_long);
}

// git push
BOOST_AUTO_TEST_CASE(git_push) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        " ", " ", " ", "-n",
        " ", "-d", " ", " ",
        " ", " ", "-o <option>", " ",
        " ", "-f", " ", " ",
        "-u", " ", "-q", "-v",
        " ", " ", " ", "-4",
        "-6"
    };
    vector<string> expected_long = {
        "--all", "--prune", "--mirror", "--dry-run",
        "--porcelain", "--delete", "--tags", "--follow-tags",
        "--signed", "--atomic", "--push-option=<option>", "--receive-pack=<git-receive-pack>",
        "--force-with-lease", "--force", "--force-if-includes", "--repo=<repository>",
        "--set-upstream", "--thin", "--quiet", "--verbose",
        "--progress", "--no-recurse-submodules", "--verify", "--ipv4",
        "--ipv6"
    };
    string command {"git push"};
    string title {"git-push - Update remote refs along with associated objects"};
    test_command(command, title, expected_short, expected_long);
}

// git rebase 
BOOST_AUTO_TEST_CASE(git_rebase) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", "-m", "-s <strategy>", "-X <strategy-option>",
        " ", "-S[<keyid>]", "-q", "-v",
        " ", "-n", " ", " ",
        "-C<n>", "-f", " ", " ",
        " ", " ", " ", " ",
        " ", "-i", "-r", "-x <cmd>",
        " ", " ", " ", " ",
        " "
    };
    vector<string> expected_long = {
        "--continue", "--skip", "--abort", "--quit",
        "--edit-todo", "--show-current-patch", "--onto <newbase>", "--keep-base",
        "--apply", "--empty=(drop|keep|stop)", "--no-keep-empty", "--reapply-cherry-picks",
        "--allow-empty-message", "--merge", "--strategy=<strategy>", "--strategy-option=<strategy-option>",
        "--rerere-autoupdate", "--gpg-sign[=<keyid>]", "--quiet", "--verbose",
        "--stat", "--no-stat", "--no-verify", "--verify",
        " ", "--no-ff", "--fork-point", "--ignore-whitespace",
        "--whitespace=<option>", "--committer-date-is-author-date", "--ignore-date", "--signoff",
        "--trailer=<trailer>", "--interactive", "--rebase-merges[=(rebase-cousins|no-rebase-cousins)]", "--exec <cmd>",
        "--root", "--autosquash", "--autostash", "--reschedule-failed-exec",
        "--update-refs"
    };
    string command {"git rebase"};
    string title {"git-rebase - Reapply commits on top of another base tip"};
    test_command(command, title, expected_short, expected_long);
}

// git tag
BOOST_AUTO_TEST_CASE(git_tag) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-a", "-s", " ", "-u <key-id>",
        "-f", "-d", "-v", "-n<num>",
        "-l", " ", " ", "-i",
        " ", " ", " ", " ",
        " ", " ", " ", "-m <msg>",
        "-F <file>", " ", "-e", " ",
        " ", " "
    };
    vector<string> expected_long = {
        "--annotate", "--sign", "--no-sign", "--local-user=<key-id>",
        "--force", "--delete", "--verify", " ",
        "--list", "--sort=<key>", "--color[=<when>]", "--ignore-case",
        "--omit-empty", "--column[=<options>]", "--contains [<commit>]", "--no-contains [<commit>]",
        "--merged [<commit>]", "--no-merged [<commit>]", "--points-at [<object>]", "--message=<msg>",
        "--file=<file>", "--trailer <token>[(=|:)<value>]", "--edit", "--cleanup=<mode>",
        "--create-reflog", "--format=<format>"
    };
    string command {"git tag"};
    string title {"git-tag - Create, list, delete or verify tags"};
    test_command(command, title, expected_short, expected_long);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(docker)
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

// docker commit
BOOST_AUTO_TEST_CASE(docker_commit) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-a", "-c", "-m", " "
    };
    vector<string> expected_long = {
        "--author=\"\"", "--change=", "--message=\"\"", "--no-pause[=false]",
    };
    string command {"docker commit"};
    string title {"docker-commit - Create a new image from a container's changes"};
    test_command(command, title, expected_short, expected_long);
}

// docker create
BOOST_AUTO_TEST_CASE(docker_create) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        " ", " ", "-a", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        "-c", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", "-e",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", "-h",
        " ", "-i", " ", " ",
        " ", " ", " ", " ",
        "-l", " ", " ", " ",
        " ", " ", " ", "-m",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", " ", "-p", "-P",
        " ", "-q", " ", " ",
        " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", "-t", " ", " ",
        " ", "-u", " ", " ",
        "-v", " ", " ", "-w"
    };
    vector<string> expected_long = {
        "--add-host=", "--annotation=map[]", "--attach=", "--blkio-weight=0",
        "--blkio-weight-device=[]", "--cap-add=", "--cap-drop=", "--cgroup-parent=\"\"",
        "--cgroupns=\"\"", "--cidfile=\"\"", "--cpu-count=0", "--cpu-percent=0",
        "--cpu-period=0", "--cpu-quota=0", "--cpu-rt-period=0", "--cpu-rt-runtime=0",
        "--cpu-shares=0", "--cpus=", "--cpuset-cpus=\"\"", "--cpuset-mems=\"\"",
        "--device=", "--device-cgroup-rule=", "--device-read-bps=[]", "--device-read-iops=[]",
        "--device-write-bps=[]", "--device-write-iops=[]", "--dns=", "--dns-option=",
        "--dns-search=", "--domainname=\"\"", "--entrypoint=\"\"", "--env=",
        "--env-file=", "--expose=", "--gpus=", "--group-add=",
        "--health-cmd=\"\"", "--health-interval=0s", "--health-retries=0", "--health-start-interval=0s",
        "--health-start-period=0s", "--health-timeout=0s", "--help[=false]", "--hostname=\"\"",
        "--init[=false]", "--interactive[=false]", "--io-maxbandwidth=0", "--io-maxiops=0",
        "--ip=", "--ip6=", "--ipc=\"\"", "--isolation=\"\"",
        "--label=", "--label-file=", "--link=", "--link-local-ip=",
        "--log-driver=\"\"", "--log-opt=", "--mac-address=\"\"", "--memory=0",
        "--memory-reservation=0", "--memory-swap=0", "--memory-swappiness=-1", "--mount=",
        "--name=\"\"", "--network=", "--network-alias=", "--no-healthcheck[=false]",
        "--oom-kill-disable[=false]", "--oom-score-adj=0", "--pid=\"\"", "--pids-limit=0",
        "--platform=\"\"", "--privileged[=false]", "--publish=", "--publish-all[=false]",
        "--pull=\"missing\"", "--quiet[=false]", "--read-only[=false]", "--restart=\"no\"",
        "--rm[=false]", "--runtime=\"\"", "--security-opt=", "--shm-size=0",
        "--stop-signal=\"\"", "--stop-timeout=0", "--storage-opt=", "--sysctl=map[]",
        "--tmpfs=", "--tty[=false]", "--ulimit=[]", "--umask=",
        "--use-api-socket[=false]", "--user=\"\"", "--userns=\"\"", "--uts=\"\"",
        "--volume=", "--volume-driver=\"\"", "--volumes-from=", "--workdir=\"\"",
    };
    string command {"docker create"};
    string title {"docker-create - Create a new container"};
    test_command(command, title, expected_short, expected_long);
}

// docker logs
BOOST_AUTO_TEST_CASE(docker_logs) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        " ", "-f", " ", "-n",
        "-t", " " 
    };
    vector<string> expected_long = {
        "--details[=false]", "--follow[=false]", "--since=\"\"", "--tail=\"all\"",
        "--timestamps[=false]", "--until=\"\""
    };
    string command {"docker logs"};
    string title {"docker-logs - Fetch the logs of a container"};
    test_command(command, title, expected_short, expected_long);
}

// docker stats
BOOST_AUTO_TEST_CASE(docker_stats) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-a", " ", " ", " "
    };
    vector<string> expected_long = {
        "--all[=false]", "--format=\"\"", "--no-stream[=false]", "--no-trunc[=false]"
    };
    string command {"docker stats"};
    string title {"docker-stats - Display a live stream of container(s) resource usage statistics"};
    test_command(command, title, expected_short, expected_long);
}

// docker update
BOOST_AUTO_TEST_CASE(docker_update) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        " ", " ", " ", " ",
        " ", "-c", " ", " ",
        " ", "-m", " ", " ",
        " ", " "
    };
    vector<string> expected_long = {
        "--blkio-weight=0", "--cpu-period=0", "--cpu-quota=0", "--cpu-rt-period=0",
        "--cpu-rt-runtime=0", "--cpu-shares=0", "--cpus=", "--cpuset-cpus=\"\"",
        "--cpuset-mems=\"\"", "--memory=0", "--memory-reservation=0", "--memory-swap=0",
        "--pids-limit=0", "--restart=\"\""
    };
    string command {"docker update"};
    string title {"docker-update - Update configuration of one or more containers"};
    test_command(command, title, expected_short, expected_long);
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
