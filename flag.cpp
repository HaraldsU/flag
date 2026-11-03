#include <ios>
#include <set>
#include <tuple>
#include <string>
#include <variant>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <boost/regex.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>

using namespace std;
namespace po = boost::program_options;

const static int kColumn_width = 80;
static po::options_description desc("Usage: flag [flags] arguments\nAllowed flags", kColumn_width);
static po::options_description hidden("Hidden options");
static po::options_description all;
static po::variables_map vm;

enum class Mode { Short, Long, Mixed};
static const std::unordered_map<std::string, Mode> kModes {
    {"short", Mode::Short},
    {"long",  Mode::Long},
    {"mixed",  Mode::Mixed}
};
static Mode mode;

static bool is_sort       = false;
static bool is_reverse    = false;
static bool is_debug      = false;
static bool is_word_count = false;
static bool is_title      = false;
static string version     = "1.0.0";


// DEBUGGING
void print_debug() {
    cout << "DEBUG:" << endl;
    auto m = int(mode) == 0 ? "short"
        : int(mode) == 1 ? "long"
        : "mixed";

    cout << "Mode: "      << int(mode)  << " (" << m << ")" << endl;
    cout << "Delimiter: " << "'"        << vm["delimiter"].as<string>() << "'" << endl;
    cout << "Sort: "      << boolalpha  << is_sort << endl;
    cout << "Reverse: "   << is_reverse << endl;
    cout << "Debug: "     << is_debug   << endl;
    cout << "Version: "   << version    << endl;
    cout << endl;
}

void print_sub_matches_debug(boost::smatch sm) {
    for (std::size_t i = 0; i < sm.size(); i++) {
        if (sm[i].matched) {
            cout << "s[" << i << "]: " << sm[i] << endl;
        }
    }
    cout << endl;
}
// DEBUGGING


int set_program_options(int end, char ** av) {
    desc.add_options()
        ("help,h", po::bool_switch()->default_value(false), "Show this help message")

        ("short,s", po::bool_switch()->default_value(false), "Show short flag notation, e.g. [-h]")

        ("long,l", po::bool_switch()->default_value(false), "Show long flag notation, e.g. [--help] (default notation is short form, e.g. [-h])")

        ("mixed,m", po::bool_switch()->default_value(true), "Show mixed flag notation, e.g. [-h|--help]")

        ("full,f", po::bool_switch()->default_value(false), "For each flag show one explanatory line from the man page") // TO DO

        ("column,c", po::value<int>()->implicit_value(1)->default_value(1, ""), "List entries by columns") // TO DO

        ("delimiter,d", po::value<string>()->value_name("DELIM")->implicit_value("'\\n'")->default_value("\n", ""), "Specify DELIM used for separating flags; the characters '\\n' and '\\t' retain special meaning, i.e. new line and tab character respectively")

        ("sort,S", po::bool_switch()->default_value(false), "Sort the output lexicographically (by default it is sorted by the order of appearance in the man page)")

        ("title,t", po::bool_switch()->default_value(false), "Show the man page NAME section at the start of the output")

        ("word-count,w", po::bool_switch()->default_value(false), "Show the printed flag count at the bottom of the output")

        ("reverse,r", po::bool_switch()->default_value(false), "Reverse the sorting of the output")

        ("color", po::value<string>()->value_name("WHEN")->implicit_value("never")->default_value("never", ""), "Use markers to highlight the first letter of the flag; WHEN is 'always', 'never', or 'auto'")

        ("debug,D", po::bool_switch()->default_value(false), "Show debug messages")

        ("version,v", po::bool_switch()->default_value(false), "Display version information and exit")
    ;

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

    po::store(parsed, vm);
    po::notify(vm); 

    if (vm["help"].as<bool>() or !vm.count("input")) {
        cout << desc << endl;
        return 1;
    }
    else if (vm["version"].as<bool>()) {
        cout << "flag " << version << endl;
        return 1;
    }

    if (vm["reverse"].as<bool>()) {
        is_reverse = true;
    }
    if (vm["sort"].as<bool>()) {
        is_sort = true;
    }
    if (vm["word-count"].as<bool>()) {
        is_word_count = true;
    }
    if (vm["title"].as<bool>()) {
        is_title = true;
    }
    if (vm["debug"].as<bool>()) {
        is_debug = true;
    }

    mode = Mode::Mixed;
    for (const auto& opt : parsed.options) {
        auto it = kModes.find(opt.string_key);
        if (it != kModes.end())
            mode = it->second;
    }

    return 0;
}

struct ilexic {
    // Strip the dashes at the start to start comparing with alnum
    static string strip_start(const string& s) {
        auto it = find_if(s.begin(), s.end(), [](unsigned char c) { return isalnum(c); });
        return string(it, s.end());
    }

    static string lower(string s) {
        for (auto & c : s) c = tolower(static_cast<unsigned char>(c));
        return s;
    }

    bool operator()(const string& a, const string& b) const {
        string x = strip_start(a);
        string y = strip_start(b);
        if (is_reverse == true) return make_tuple(lower(y), y, b) < make_tuple(lower(x), x, a);
        else return make_tuple(lower(x), x, a) < make_tuple(lower(y), y, b);
    }
};

string load_man_page(int ac, char ** av) {
    auto input = boost::algorithm::join(vm["input"].as<vector<string>>(), " ");
    string command{ "man "s + input + " 2>/dev/null | "s + "cat"s};
    
    string man_page {};
    auto po = popen(command.c_str(), "r");
    char buf[1024];
    while (fgets(buf, 1024, po)) {
        man_page.append(buf);
    }
    pclose(po);

    if (man_page.size() == 0) throw std::string{"Man page for command \"" + std::string{av[ac -1]} + "\" not found"};
    else return man_page;
}

string set_delimiter() {
    if (vm["column"].as<int>() > 1) return "";

    string delim {vm["delimiter"].as<string>()};
    delim = vm["delimiter"].as<string>();
    if (delim == "\\n") delim = "\n";
    else if (delim == "\\t") delim = "\t";

    return delim;
}

unordered_map<Mode, int> flag_count {};
template <typename A>
void add_commands(A & commands, string man_page, string regx) {
    const boost::regex expr(regx, boost::regex::no_mod_s | boost::regex::mod_x);
    boost::sregex_iterator sr_iterator(man_page.begin(), man_page.end(), expr);

    for (auto mr = sr_iterator; mr != boost::sregex_iterator(); mr++) {
        string command {};
        auto sm = *mr;

        if (is_debug) print_sub_matches_debug(sm); // DEBUG

        if (mode == Mode::Short) { 
            if (sm[2].matched) {
                command = sm[2];
                flag_count[Mode::Short]++;
            }
            else if (sm[5].matched) {
                command = sm[5];
                flag_count[Mode::Short]++;
            }
            else continue;
        }
        else if (mode == Mode::Long) {
            if (sm[2].matched) {
                command = sm[2];
                flag_count[Mode::Long]++;
            }
            else if (sm[6].matched) {
                command = sm[6];
                flag_count[Mode::Long]++;
            }
            else continue;
        } else {
            if (sm[2].matched) {
                command = sm[3] + "," + sm[6];
                flag_count[Mode::Mixed]++;
            }
            else if (sm[10].matched) {
                command = sm[15] + "," + sm[11];
                flag_count[Mode::Mixed]++;
            }
            else if (sm[18].matched) {
                command = sm[18];
                flag_count[Mode::Short]++;
            }
            else if (sm[21].matched) {
                command = sm[21];
                flag_count[Mode::Long]++;
            }
            else continue;
        }
        command = "[" + command + "]";

        bool found = find(commands.begin(), commands.end(), command) != commands.end();
        if (!found) {
            if (is_reverse == false) commands.insert(commands.end(), command);
            else commands.insert(commands.begin(), command);
        }
    }
}

template <typename A>
void get_commands(A & commands, string man_page) {
    string reg_final {};

    string reg_start {R"(
        # START
        (?:^[\s]{3,6})\K
    )"};

    string reg_short {R"(
        # (1, 2, 3) SHORT
        ((-\w+)(-\w+)*)
    )"};

    string reg_long {R"(
        # (1, 2, 3, 4) LONG
        ((--\w+)(-\w+)*(=\w+)?)
    )"};

    string reg_sls {R"(
        # SHORT-LONG-SHORT
        # Long form starts with -- followed by alnum, e.g. --all
        (?:(?:--\w+)
            # The flag name can contain a dash in the middle, e.g. --cert-type
            (?:-\w+)*
            # Long form can have an equal sign (=), e.g. --protocol=family
            (?:=\w+)?
            # Separation
            (?:[^-\w\n]{1,2})
            # (1, 2, 3)
            ((-\w+)(-\w+)*))|
    )"};

    string reg_lsl {R"(
        # LONG-SHORT-LONG
        (?:(?:-\w+)
            (?:-\w+)*
            (?:[^-\w\n]{1,2})
        # (1, 2, 3, 4)
        ((--\w+)(-\w+)*(=\w+)?))|
    )"};

    string reg_sl {R"(
        # SHORT-LONG
        # (1, 2, 3)
        (((-\w+)
            # (4)
            (-\w+)*)
            (?:[^-\w\n]{1,2})
        # (5, 6)
        ((--\w+)
            # (7)
            (-\w+)*
            # (8)
            (=\w+)?))|
    )"};

    string reg_ls {R"(
        # LONG_SHORT
        # (1, 2, 3)
        (((--\w+)
            # (4)
            (-\w+)*
            # (5)
            (=\w+)?)
            (?:[^-\w\n]{1,2})
        # (6, 7)
        ((-\w+)
            # (8)
            (-\w+)*))|
    )"};

    string reg_end {R"(
        # END
        (?:$|[^-\w\n]{2,})
    )"};

    if (mode == Mode::Short) {
        reg_final = 
            reg_start +
            "(" +       // (1)
            reg_sls +   // (2, ..., 4)
            reg_short + // (5, ..., 7)
            ")" + 
            reg_end
        ;
    } else if (mode == Mode::Long) {
        reg_final = 
            reg_start +
            "(" +       // (1)
            reg_lsl +   // (2, ..., 5)
            reg_long +  // (6, ..., 9)
            ")" + 
            reg_end
        ;
    } else if (mode == Mode::Mixed) {
        reg_final = 
            reg_start +
            "(" +       // (1)
            reg_sl +    // (2, ..., 9)
            reg_ls +    // (10, ..., 17)
            reg_short + // (18, ..., 20)
            "|" +
            reg_long +  // (21, ..., 24)
            ")" +
            reg_end
        ;
    } else {
        throw "Invalid flag notation (expected short, long or mixed)";
    }

    // cout << "reg_final: " << reg_final << endl;
    add_commands(commands, man_page, reg_final);
}

template <typename A>
vector<string> get_n_longest_flags(const A commands) {
    int n = vm["column"].as<int>();
    vector<string> sorted(commands.begin(), commands.end());
    std::sort(sorted.begin(), sorted.end(),
        [](const std::string& a, const std::string& b) {
            return a.size() > b.size();
        }
    );

    vector<string> res = {};
    auto it = sorted.begin();
    for (int i = 0; i < n && it != sorted.end(); i++) {
        res.push_back(*it);
        it++;
    }

    return res;
}

void print_flag_count() {
    cout <<    "SF: " << flag_count[Mode::Short]
         << " | LF: " << flag_count[Mode::Long]
         << " | MF: " << flag_count[Mode::Mixed]
         << " | TT: " << flag_count[Mode::Short] + flag_count[Mode::Long] + flag_count[Mode::Mixed] << endl;
}

void print_title(string man_page) {
    string regx = R"(
        (?:^NAME\n\s{2,})\K
        .*
        (?=\n)
    )";

    const boost::regex expr(regx, boost::regex::no_mod_s | boost::regex::mod_x);
    boost::sregex_iterator sr_iterator(man_page.begin(), man_page.end(), expr);

    auto sm = *sr_iterator;
    cout << "NAME: " << sm[0] << endl;
}

void add_colors(string & output, int pipe_pos) {
    if (pipe_pos != 0) {
        output.insert(pipe_pos + 1, "\033[32m");
        output.insert(pipe_pos, "\033[33m");
    }
    output.insert(output.length() - 1, "\033[33m");
    output.insert(1, "\033[31m");
    output.insert(0, "\033[33m");
    output.insert(0, "\033[0m");
    output += "\033[0m";
}

template <typename A>
void print_commands_helper(A commands, string delim, int longest_flag_n) {
    int count = 0;
    int extra_padding = 2;
    int col_n = vm["column"].as<int>();
	for (auto iter = commands.begin(); iter != commands.end(); iter++) {
		if (std::next(iter) == commands.end()) delim = "";

		string output {};
		int pipe_pos = 0;
		if (mode == Mode::Mixed) {
			int i = 0;
			for (auto c : *iter) {
				if (c != ',') {
					output.append(std::string{c});
					i++;
				}
				else {
					output.append("|");
					pipe_pos = i;
				}
			}
		}
		else output = *iter;

        auto tmp_output = output;
		auto when = vm["color"].as<string>();
		if ( when == "always" or (when == "auto" and isatty(STDOUT_FILENO))) add_colors(output, pipe_pos);

        if (col_n > 1) {
            cout << output << string(longest_flag_n + extra_padding - tmp_output.length(), ' ') << delim;

            count++;
            if (count == col_n) {
                cout << endl;
                count = 0;
            }
        }
        else cout << output << delim;
	}
	cout << endl;
}

template <typename A>
void print_commands(A commands, string delim, string man_page) {
    int longest_flag_n = 1;

    auto lf = get_n_longest_flags(commands);
    longest_flag_n = lf[0].length();
    int char_count = 0;
    for (auto i : lf) char_count += i.length();

    // (kColumn_width < char_count) 
        // ? cout << "Can NOT fit " << col_n << " columns" << endl
        // : cout << "CAN fit " << col_n << " columns" << endl;
    // cout << endl;
    // delim = "";

    if (is_title) print_title(man_page);
    if (is_word_count) print_flag_count();

    print_commands_helper(commands, delim, longest_flag_n);

    if (is_title) print_title(man_page);
    if (is_word_count) print_flag_count();
}

int get_command_end_idx(int ac, char ** av) {
    int n = ac - 1;
    while (n > 0 && av[n][0] == '-') n--;
    return n > 0 ? n + 1 : ac;
}

int main(int ac, char ** av) {
    auto end = get_command_end_idx(ac, av);
    auto spo = set_program_options(end, av);
    if (spo == 1) return 1;

    string man_page {};
    try {
        if (is_debug) print_debug();

        man_page = load_man_page(ac, av);
        variant<set<string, ilexic>, vector<string>> commands;
        if (is_sort == true) commands = set<string, ilexic>{};
        else commands = vector<string>{};
        visit([&](auto & c) { get_commands(c, man_page); }, commands);

        string delim = set_delimiter();
        visit([&](auto & c) { print_commands(c, delim, man_page); }, commands);

        return 0;
    } catch (const string& e) {
        cout << e << endl;
        return 1;
    }
}

