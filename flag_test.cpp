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
        // std::cout << command << ": mode=" << static_cast<int>(mode)
          // << " mixed_only=" << is_mixed_only
          // << " sort=" << is_sort << '\n';

        Input input(command);
        string man_page = load_man_page(input.ac, input.av);
        vector<string> actual;
        get_commands(actual, man_page);
        return actual;
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

// ls
BOOST_AUTO_TEST_CASE(get_flags_ls) {
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
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s ls", expected_short},
        {"flag -l ls", expected_long},
        {"flag -m ls", expected_mixed},
        {"flag -M ls", expected_mixed_only},
    };

    for (const auto& [command, expected] : cases) {
        BOOST_TEST_CONTEXT("command: " << command) {
            BOOST_TEST(commands_for(command) == expected, boost::test_tools::per_element());
        }
    }
}

// id
BOOST_AUTO_TEST_CASE(get_flags_id) {
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
    BOOST_TEST(expected_short.size() == expected_long.size());

    vector<string> expected_mixed = {};
    vector<string> expected_mixed_only = {};
    make_mixed_from_short_and_long(expected_short, expected_long, expected_mixed, expected_mixed_only);
    BOOST_TEST(expected_long.size() == expected_mixed.size());

    remove_spaces(expected_short);
    remove_spaces(expected_long);

    const vector<pair<const char*, vector<string>>> cases = {
        {"flag -s id", expected_short},
        {"flag -l id", expected_long},
        {"flag -m id", expected_mixed},
        {"flag -M id", expected_mixed_only},
    };

    for (const auto& [command, expected] : cases) {
        BOOST_TEST_CONTEXT("command: " << command) {
            BOOST_TEST(commands_for(command) == expected, boost::test_tools::per_element());
        }
    }
}

// wc
BOOST_AUTO_TEST_CASE(get_flags_wc) {
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

    for (const auto& [command, expected] : cases) {
        BOOST_TEST_CONTEXT("command: " << command) {
            BOOST_TEST(commands_for(command) == expected, boost::test_tools::per_element());
        }
    }
}

// head
BOOST_AUTO_TEST_CASE(get_flags_head) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-c", "-n", "-q", "-v",
        "-z", " ", " "
    };
    vector<string> expected_long = {
        "--bytes=[-]NUM", "--lines=[-]NUM", "--quiet", "--verbose",
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

    for (const auto& [command, expected] : cases) {
        BOOST_TEST_CONTEXT("command: " << command) {
            BOOST_TEST(commands_for(command) == expected, boost::test_tools::per_element());
        }
    }
}

// tail
BOOST_AUTO_TEST_CASE(get_flags_tail) {
    vector<string> expected_short = {
        // " ", " ", " ", " ",
        "-c", " ", "-f", "-F",
        "-n", " ", " ", "-q",
        " ", "-s", "-v", "-z",
        " ", " "
    };
    vector<string> expected_long = {
        "--bytes=[+]NUM", "--debug", "--follow[={name|descriptor}]", " ",
        "--lines=[+]NUM", "--max-unchanged-stats=N", "--pid=PID", "--quiet",
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

    for (const auto& [command, expected] : cases) {
        BOOST_TEST_CONTEXT("command: " << command) {
            BOOST_TEST(commands_for(command) == expected, boost::test_tools::per_element());
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
