#include <ios>
#include "flag.h"
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <boost/regex.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>

using namespace std;
namespace po = boost::program_options;

const static int kColumn_width  = 100;
const static string version     = "1.0.0";

// DEBUGGING
string escape(const string& s) {
    string out;
    for (char c : s) {
        switch (c) {
            case '\n': out += "\\n";  break;
            case '\t': out += "\\t";  break;
            case '\\': out += "\\\\"; break;
            default:   out += c;
        }
    }
    return out;
}

int print_debug() {
    cout << "DEBUG:" << endl;
    auto m = int(MODE) == 0 ? "short"
        : int(MODE) == 1 ? "long"
        : "mixed";

    cout << "Mode: "          << int(MODE)     << " (" << m << ")" << endl;
    cout << "Mixed-only: "    << boolalpha     << IS_MIXED_ONLY << endl;
    if (VM.count("DELIMITER")) {
        cout << "Delimiter: "     << "'"           << escape(VM["DELIMITER"].as<string>()) << "'" << endl;
    } else cout << "Delimiter: N/A" << endl;
    cout << "Sort: "          << boolalpha     << IS_SORT << endl;
    cout << "Reverse: "       << boolalpha     << IS_REVERSE << endl;
    cout << "Word count: "    << boolalpha     << IS_WORD_COUNT << endl;
    cout << "Title: "         << boolalpha     << IS_TITLE      << endl;
    cout << "Enclose count: " << ENCLOSE_COUNT << endl;
    cout << "Debug: "         << boolalpha     << IS_DEBUG   << endl;
    cout << "Version: "       << version       << endl;
    cout << endl;

    return 0;
}

void print_sub_matches_debug(boost::smatch sm) {
    for (size_t i = 0; i < sm.size(); i++) {
        if (sm[i].matched) {
            cout << "s[" << i << "]: " << sm[i] << endl;
        }
    }
}
// DEBUGGING

// PRINTING
int print_help(po::options_description desc) {
    cout << desc << endl;
    return 1;
}

int print_program_version() {
    cout << "flag " << version << endl;
    return 1;
}

void print_flag_count() {
    cout <<    "COUNT: "
         <<    "SF: " << FLAG_COUNT[Mode::Short]
         << " | LF: " << FLAG_COUNT[Mode::Long]
         << " | MF: " << FLAG_COUNT[Mode::Mixed]
         << " | TT: " << FLAG_COUNT[Mode::Short] + FLAG_COUNT[Mode::Long] + FLAG_COUNT[Mode::Mixed] << endl;
}

string print_title(const string man_page) {
    string regx = R"(
        (?:^NAME\n\s{2,})\K
        .*
        (?=\n)
    )";

    const boost::regex expr(regx, boost::regex::no_mod_s | boost::regex::mod_x);
    boost::sregex_iterator sr_iterator(man_page.begin(), man_page.end(), expr);

    auto sm = *sr_iterator;
    return sm[0];
}
// PRINTING

// SETTING / ADDING
int set_program_options(int end, char ** av) {
    po::options_description desc("Usage: flag [flags] [argument]\nFlags", kColumn_width);
    po::options_description hidden("Hidden options");
    po::options_description all;

    desc.add_options()
        ("color", po::value<string>()->value_name("WHEN")->implicit_value("never")->default_value("never", ""), "Use markers to highlight the first letter of the flag; WHEN is 'always', 'never', or 'auto'")

        ("column,c", po::value<int>()->value_name("COL")->implicit_value(1)->default_value(1, ""), "List entries by columns; COL specifies the column count")

        ("delimiter,d", po::value<string>()->value_name("DEM")->implicit_value("'\\n'")->default_value("\n", ""), "Specify DEM used for separating flags; the characters '\\n' and '\\t' retain special meaning")

        ("debug,D", po::bool_switch()->default_value(false), "Show debug messages")

        ("enclose,e", po::value<string>()->value_name("ENC")->implicit_value("''")->default_value("", ""), "Specify ENC used for 'enclosing' the flag output (prepending and appending); 0, 1 or 2 arguments are expected, e.g. -e '|' or --enclose '{}'")

        // ("full,f", po::bool_switch()->default_value(false), "For each flag show one explanatory line from the man page") // TO DO

        ("help,h", po::bool_switch()->default_value(false), "Show this help message")

        ("long,l", po::bool_switch()->default_value(false), "Show long flag notation, e.g. --help")

        ("mixed,m", po::bool_switch()->default_value(true), "Show mixed flag notation, e.g. -h|--help")

        ("mixed-only,M", po::bool_switch()->default_value(false), "Show only flags that have both short and long notations")

        ("reverse,r", po::bool_switch()->default_value(false), "Reverse the sorting of the output")

        ("short,s", po::bool_switch()->default_value(false), "Show short flag notation, e.g. -h")

        ("sort,S", po::bool_switch()->default_value(false), "Sort the output lexicographically (by default flag are sorted by the order of appearance)")

        ("title,t", po::bool_switch()->default_value(false), "Show the man page NAME section at the bottom of the output")

        ("version,v", po::bool_switch()->default_value(false), "Display version information and exit")

        ("word-count,w", po::bool_switch()->default_value(false), "Show the flag count at the bottom of the output")
    ;

    try {
        hidden.add_options()
            ("input", po::value<vector<string>>(), "input")
            ;
        po::positional_options_description pos;
        pos.add("input", -1);
        all.add(desc).add(hidden);

        po::parsed_options parsed = po::command_line_parser(end, av)
            .options(all)
            .positional(pos)
            .run();
        po::store(parsed, VM);
        po::notify(VM); 

        if (VM["version"].as<bool>()) {
            return print_program_version();
        }
        else if (VM["help"].as<bool>() or !VM.count("input")) {
            return print_help(desc);
        }

        if (VM["reverse"].as<bool>()) IS_REVERSE = true;
        if (VM["sort"].as<bool>()) IS_SORT = true;
        if (VM["word-count"].as<bool>()) IS_WORD_COUNT = true;
        if (VM["mixed-only"].as<bool>()) IS_MIXED_ONLY = true;
        if (VM["title"].as<bool>()) IS_TITLE = true;
        if (VM["debug"].as<bool>()) IS_DEBUG = true;

        if (VM.count("delimiter")) {
            DELIMITER = VM["delimiter"].as<string>();
        }

        if (VM.count("enclose")) {
            if (VM["enclose"].as<string>().length() == 0) ENCLOSE_COUNT = 0;
            else if (VM["enclose"].as<string>().length() == 1) ENCLOSE_COUNT = 1;
            else if (VM["enclose"].as<string>().length() == 2) ENCLOSE_COUNT = 2;
            else throw string{"Flag '--enclose' excpects 0, 1 or 2 characters in the argument"};
        }

        MODE = Mode::Mixed;
        for (const auto& opt : parsed.options) {
            auto it = kMODES.find(opt.string_key);
            if (it != kMODES.end())
                MODE = it->second;
        }
    } catch (const po::invalid_command_line_syntax& e) {
        if (e.kind() == po::invalid_command_line_syntax::empty_adjacent_parameter) {
            string opt = e.get_option_name();
            if (opt == "--enclose") {
                throw string{"Empty value for " + e.get_option_name() + "=, use --enclose '' instead"};
            } else if (opt == "--DELIMITER") {
                throw string{"Empty value for " + e.get_option_name() + "=, use --DELIMITER '' instead"};
            }
        }

        throw string{"Flag: unknown error occured"};
    }

    return 0;
}

string set_man_page() {
	setenv("MANWIDTH", "1000", 1); // Needed for not wrapping long lines
    auto input = boost::algorithm::join(VM["input"].as<vector<string>>(), " ");
    string command {"man " + input + " 2>/dev/null | " + "cat"};
    string man_page {};

    auto po = popen(command.c_str(), "r");
    char buf[1024];
    while (fgets(buf, 1024, po)) man_page.append(buf);
    pclose(po);

    if (man_page.size() == 0) throw string{"Man page for command \"" + input + "\" not found"};

    return man_page;
}

void add_colors(string & output, int pipe_pos) {
    // 31: red; 32: green; 33: yellow
    bool is_solo_long = false;
    if (ENCLOSE_COUNT == 0) {
        (output[1] == '-') ? is_solo_long = true : is_solo_long = false;
    } else {
        (output[2] == '-') ? is_solo_long = true : is_solo_long = false;
    }

    auto s = "\033[31m";
    auto l = "\033[32m";
    auto sep = "\033[33m";
    auto reset = "\033[0m";

    if (pipe_pos != 0) {
        output.insert(pipe_pos + 1, l);            // [-s|{--l}]
        output.insert(pipe_pos, sep);              // [-s{|}--l]
    }

    if (ENCLOSE_COUNT != 0) { 
        output.insert(output.length() - 1, sep);   // [?|?{]}
        if (!is_solo_long) output.insert(1, s);    // [{-s}|?]
        else output.insert(1, l);                  // [{--l}]
    }
    else { 
        if (!is_solo_long) output.insert(0, s);    // {-s}|?
        else output.insert(0, l);                  // {--l}
    }

    if (ENCLOSE_COUNT != 0) output.insert(0, sep); // {[}-s|--l]

    output.insert(0, reset);                       // {}[-s|--l]
    output += reset;                               // [-s|--l]{}
}

void add_enclosing(string & output) {
    auto enclose = VM["enclose"].as<string>();
    if (ENCLOSE_COUNT == 1) output = enclose[0] + output + enclose[0];
    else if (ENCLOSE_COUNT == 2) output = enclose[0] + output + enclose[1];
}
// SETTING / ADDING

