#include "flag.h"
#include <ios>
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

struct Lexic {
    // Strip the dashes at the start to start comparing with alnum
    string strip_start(const string & s) {
        auto it = find_if(s.begin(), s.end(), [](unsigned char c) { return isalnum(c); });

        return string(it, s.end());
    }

    string flag_name(const string & s) {
        string x = strip_start(s);
        return x.substr(0, x.find_first_of(",= ["));
    }

    // Prioritize small lower case before upper case
    string swap_case(string s) {
        for (auto & c : s) {
            unsigned char u = static_cast<unsigned char>(c);
            c = isupper(u) ? tolower(u) : toupper(u);
        }

        return s;
    }

    string lower(string s) {
        for (auto & c : s) c = tolower(static_cast<unsigned char>(c));

        return s;
    }

    bool operator()(const string & a, const string & b) {
        string x = flag_name(a);
        string y = flag_name(b);

        return make_tuple(lower(x), swap_case(x), a) < make_tuple(lower(y), swap_case(y), b);
    }
};


string escape(const string & s) {
    string out;
    for (auto c : s) {
        switch (c) {
            case '\n': out += "\\n";  break;
            case '\t': out += "\\t";  break;
            case '\\': out += "\\\\"; break;
            default:   out += c;
        }
    }
    return out;
}

int debug_print() {
    cout << "DEBUG:" << endl;
    
    // --color
    cout << "Color: "         << VM["color"].as<string>() << endl;

    // --column
    cout << "Column: "        << VM["column"].as<int>() << endl;


    // --delimiter
    cout << "Delimiter: "     << "'"           << escape(VM["delimiter"].as<string>()) << "'" << endl;

    // --debug
    cout << "Debug: "         << boolalpha     << IS_DEBUG   << endl;

    // --enclose
    cout << "Enclose count: " << ENCLOSE_COUNT << endl;
    cout << "Enclose: "       << "'" << VM["enclose"].as<string>() << "'" << endl;

    // Modes (--long, --mixed, --mixed-only, --short)
    auto m = int(MODE) == 0 ? "short"
        : int(MODE) == 1 ? "long"
        : "mixed";
    cout << "Mode: "          << int(MODE)     << " (" << m << ")" << endl;
    cout << "Mixed-only: "    << boolalpha     << IS_MIXED_ONLY << endl;

    // --no-argument
    cout << "No argument: "   << boolalpha     << IS_NO_ARGUMENT << endl;

    // --pretty
    cout << "Pretty: "        << boolalpha     << IS_PRETTY << endl;

    // --reverse
    cout << "Reverse: "       << boolalpha     << IS_REVERSE << endl;

    // --sort
    cout << "Sort: "          << boolalpha     << IS_SORT << endl;

    // --title
    cout << "Title: "         << boolalpha     << IS_TITLE      << endl;

    // --flag-count
    cout << "Flag count: "    << boolalpha     << IS_FLAG_COUNT << endl;

    // --version
    cout << "Version: "       << VERSION       << endl;

    cout << endl;

    return 0;
}

void debug_print_sub_matches(boost::smatch sm) {
    for (size_t i = 0; i < sm.size(); i++) {
        if (sm[i].matched) cout << "s[" << i << "]: " << "\"" << sm[i] << "\"" << endl;
    }
}


vector<string> get_n_longest_flags(const vector<string> commands) {
    int n = VM["column"].as<int>();
    vector<string> sorted(commands.begin(), commands.end());
    sort(sorted.begin(), sorted.end(),
        [](const string& a, const string& b) { return a.size() > b.size(); }
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
    cout <<    "SF=" << FLAG_COUNT[Mode::Short]
         << " | LF=" << FLAG_COUNT[Mode::Long]
         << " | MF=" << FLAG_COUNT[Mode::Mixed]
         << " | TT=" << FLAG_COUNT[Mode::Short] + FLAG_COUNT[Mode::Long] + FLAG_COUNT[Mode::Mixed] << endl;
}

string get_title(const string man_page) {
    string regx = R"(
        (?:^(?:NAME|Name)\n\s{2,})\K
        .*
        (?=\n)
    )";

    const boost::regex expr(regx, boost::regex::no_mod_s | boost::regex::mod_x);
    boost::sregex_iterator sr_iterator(man_page.begin(), man_page.end(), expr);

    auto sm = *sr_iterator;
    return sm[0];
}

void print_flags_helper(const vector<string> flags) {
    auto lf = get_n_longest_flags(flags);
    int longest_flag_n = lf[0].length();
    auto delim = DELIMITER;

    int count = 0;
    int extra_padding = 2 + ENCLOSE_COUNT;
    int col_n = COLUMN_COUNT;

    for (auto iter = flags.begin(); iter != flags.end(); iter++) {
        if (next(iter) == flags.end()) delim = "\n";

        string command = *iter;
        string output  = "";
        int pipe_pos   = 0;
        if (ENCLOSE_COUNT != 0) add_enclosing(command);

        if (MODE == Mode::Mixed) {
            int i = 0;
            for (auto c : command) {
                if (c != ',') {
                    output.append(string{c});
                    i++;
                }
                else {
                    output.append("|");
                    pipe_pos = i;
                }
            }
        }
        else output = command;

        auto tmp_output = output;
        auto when = VM["color"].as<string>();
        if (when == "always" or (when == "auto" and isatty(STDOUT_FILENO))) add_colors(output, pipe_pos);

        if (col_n > 1) {
            string offset(longest_flag_n + extra_padding - tmp_output.length(), ' ');
            cout << output << offset;

            count++;
            if (count == col_n or next(iter) == flags.end()) {
                cout << endl;
                count = 0;
            }
        }
        else cout << output << delim;
    }
}

void print_flags(vector<string> commands, string man_page) {
    print_flags_helper(commands);
    if (IS_TITLE) { 
        auto title = get_title(man_page); 
        cout << title << endl;
    }
    if (IS_FLAG_COUNT) print_flag_count();
}

void print_help(const po::options_description desc) {
    string flag_sep = ", ";
    string desc_sep = "  ";
    char start = ' ';
    int start_repeat = 4;
    size_t desc_offset = 25;

    cout << "flag is a command-line tool for fast discovery of command flags from man pages." << endl
         << "See flag(1) for more information." << endl << endl
         << "Usage: flag [flags] [arguments]" << endl
         << "Flags:" << endl;

    for (auto opt : desc.options()) {
        auto flag_s = opt->canonical_display_name(po::command_line_style::allow_dash_for_short);
        auto flag_l = opt->canonical_display_name(po::command_line_style::allow_long);
        auto flag_arg = opt->format_parameter();
        bool is_short = false;
        bool is_long = false;
        string output = string(start_repeat, start);

        if (flag_s[0] == '-') is_short = true;
        if (flag_l[0] == '-' and flag_l[1] == '-') is_long = true;

        if (is_short) output += flag_s;
        if (is_short and is_long) output += flag_sep;
        if (is_long) {
            output += flag_l;
            for (size_t i = 1; i < flag_arg.length(); i++) {
                if (flag_arg[i] != '(') output += flag_arg[i];
                else break;
            }
        }

        if (output.length() + 1 < desc_offset) output += string(desc_offset - output.length(), start);
        else output += " ";

        output += desc_sep;
        output += opt->description();

        cout << output << endl;
    }
    cout << endl;
}


int sort_flags(vector<string> & flags) {
    try {
        sort(flags.begin(), flags.end(), Lexic());
    } 
    catch (const exception & e) {
        throw e.what();
    }

    return 0;
}

int reverse_flags(vector<string> & flags) {
    try {
        reverse(flags.begin(), flags.end());
    } 
    catch (const exception & e) {
        throw e.what();
    }
    
    return 0;
}

struct CaptureGroupCount {
    int reg_short      = 0;
    int reg_long       = 0;
    int reg_sep        = 0;
    int reg_short_long = 0;
    int reg_long_short = 0;
};

int add_flags_mixed(vector<string> & flags, boost::smatch sm, CaptureGroupCount cgc) {
    string command {};
    vector<string> captured_flags {};
    Mode mode {};

    // reg_final = reg_start + "(?:" + reg_sl + "|" + reg_ls + "|" + reg_long + "|" + reg_short + ")" + reg_end ;
    int short_long = 1;
    int short_long_short = short_long + 1;
    int short_long_long = short_long_short + cgc.reg_short + cgc.reg_sep;

    int long_short = short_long + cgc.reg_short_long;
    int long_short_long = long_short + 1;
    int long_short_short = long_short_long + cgc.reg_long + cgc.reg_sep;

    int lng = long_short + cgc.reg_long_short;
    int shrt = lng + cgc.reg_long;

    if (sm[short_long].matched) {
        if (IS_DEBUG) cout << "SL" << endl << endl;

        captured_flags.push_back(sm[short_long_short]);
        captured_flags.push_back(sm[short_long_long]);
        FLAG_COUNT[Mode::Mixed]++;
        mode = Mode::Mixed;

        if (!IS_NO_ARGUMENT) command = sm[short_long_short] + "," + sm[short_long_long];
        else command = sm[short_long_short + 1] + "," + sm[short_long_long + 1];
    }
    else if (sm[long_short].matched) {
        if (IS_DEBUG) cout << "LS" << endl << endl;

        captured_flags.push_back(sm[long_short_short]);
        captured_flags.push_back(sm[long_short_long]);
        FLAG_COUNT[Mode::Mixed]++;
        mode = Mode::Mixed;

        if (!IS_NO_ARGUMENT) command = sm[long_short_short] + "," + sm[long_short_long];
        else command = sm[long_short_short + 1] + "," + sm[long_short_long + 1];
    }
    else if (!IS_MIXED_ONLY) {
        if (sm[lng].matched) {
            if (IS_DEBUG) cout << "LONG" << endl << endl;

            FLAG_COUNT[Mode::Long]++;
            mode = Mode::Long;
            captured_flags.push_back(sm[lng + 1]);

            if (!IS_NO_ARGUMENT) command = sm[lng];
            else command = sm[lng + 1];
        }
        else if (sm[shrt].matched) {
            if (IS_DEBUG) cout << "SHORT" << endl << endl;

            FLAG_COUNT[Mode::Short]++;
            mode = Mode::Short;

            if (!IS_NO_ARGUMENT) command = sm[shrt];
            else command = sm[shrt + 1];
            captured_flags.push_back(sm[shrt]);
        }
        else {
            if (IS_DEBUG) cout << "NO MATCH" << endl << endl;
            return 1;
        }
    }
    else return 1;

    if (captured_flags.size() == 1) {
        bool found = find(FLAGS_ONLY.begin(), FLAGS_ONLY.end(), captured_flags[0]) != FLAGS_ONLY.end();
        if (!found) flags.push_back(command);
        else FLAG_COUNT[mode]--;
        FLAGS_ONLY.insert(captured_flags[0]);
    }
    else {
        bool found1 = find(FLAGS_ONLY.begin(), FLAGS_ONLY.end(), captured_flags[0]) != FLAGS_ONLY.end();
        bool found2 = find(FLAGS_ONLY.begin(), FLAGS_ONLY.end(), captured_flags[1]) != FLAGS_ONLY.end();

        if (!found1 and !found2) flags.push_back(command);
        else FLAG_COUNT[mode]--;

        FLAGS_ONLY.insert(captured_flags[0]);
        FLAGS_ONLY.insert(captured_flags[1]);
    }

    return 0;
}

int add_flags_long(vector<string> & flags, boost::smatch sm, CaptureGroupCount cgc) {
    string command {};
    string flag {};
    Mode mode {};
    int idx = 0;

    // reg_final = reg_start + "(?:" + reg_sl + "|" + reg_ls + "|" + reg_long + "|" + reg_short + ")" + reg_end ;
    int short_long_long = 1 + 1 + cgc.reg_short + cgc.reg_sep;
    int long_short_short = 1 + short_long_long + cgc.reg_long;
    int lng = long_short_short + cgc.reg_long_short - 1;

    if (sm[short_long_long].matched) {
        if (IS_DEBUG) cout << "SLL" << endl << endl;
        idx = short_long_long;
    }
    else if (sm[long_short_short].matched) {
        if (IS_DEBUG) cout << "LSL" << endl << endl;
        idx = long_short_short;
    }
    else if (sm[lng].matched) {
        if (IS_DEBUG) cout << "LONG" << endl << endl;
        idx = lng;
    }
    else {
        if (IS_DEBUG) cout << "NO MATCH" << endl << endl;
        return 1;
    }

    if (sm[short_long_long].matched) flag = sm[idx];
    else flag = sm[idx + 1];

    if (!IS_NO_ARGUMENT) command = sm[idx];
    else command = sm[idx + 1];

    FLAG_COUNT[Mode::Long]++;
    mode = Mode::Long;

    bool found = find(FLAGS_ONLY.begin(), FLAGS_ONLY.end(), flag) != FLAGS_ONLY.end();
    if (!found) flags.push_back(command);
    else FLAG_COUNT[mode]--;
    FLAGS_ONLY.insert(flag);

    return 0;
}

int add_flags_short(vector<string> & flags, boost::smatch sm, CaptureGroupCount cgc) {
    string command {};
    string flag {};
    Mode mode {};
    int idx = 0;

    // reg_final = reg_start + "(?:" + reg_sl + "|" + reg_ls + "|" + reg_long + "|" + reg_short + ")" + reg_end ;
    int short_long_short = 1 + 1;
    int long_short_short = short_long_short + cgc.reg_short_long + cgc.reg_long + cgc.reg_sep;
    int shrt = long_short_short + cgc.reg_short + cgc.reg_long;

    if (sm[short_long_short].matched) {
         if (IS_DEBUG) cout << "SLS" << endl << endl;
         idx = short_long_short;
    }
    else if (sm[long_short_short].matched) {
        if (IS_DEBUG) cout << "LSS" << endl << endl;
        idx = long_short_short;
    }
    else if (sm[shrt].matched) {
         if (IS_DEBUG) cout << "SHORT" << endl << endl;
         idx = shrt;
    }
    else { 
        if (IS_DEBUG) cout << "NO MATCH" << endl << endl;
        return 1;
    }

    flag = sm[idx];
    if (!IS_NO_ARGUMENT) command = sm[idx];
    else command = sm[idx + 1];

    FLAG_COUNT[Mode::Short]++;
    mode = Mode::Short;

    bool found = find(FLAGS_ONLY.begin(), FLAGS_ONLY.end(), flag) != FLAGS_ONLY.end();
    if (!found) flags.push_back(command);
    else FLAG_COUNT[mode]--;
    FLAGS_ONLY.insert(flag);

    return 0;
}

void add_flags(vector<string> & flags, string man_page, string regx, CaptureGroupCount cgc) {
    const boost::regex expr(regx, boost::regex::mod_x);
    boost::sregex_iterator sr_iterator(man_page.begin(), man_page.end(), expr);

    for (auto mr = sr_iterator; mr != boost::sregex_iterator(); mr++) {
        auto sm = *mr;
        if (IS_DEBUG) debug_print_sub_matches(sm);

        if (MODE == Mode::Short) {
            auto r = add_flags_short(flags, sm, cgc);
            if (r == 1) continue;
        }
        else if (MODE == Mode::Long) {
            auto r = add_flags_long(flags, sm, cgc);
            if (r == 1) continue;
        }
        else if (MODE == Mode::Mixed) {
            auto r = add_flags_mixed(flags, sm, cgc);
            if (r == 1) continue;
        } 
        else throw string{"invalid flag notation (expected short, long or mixed)"};
    }

    if (IS_SORT) {
        try {
            sort_flags(flags);
        } 
        catch (const string & em) {
            throw em;
        }
    }

    if (IS_REVERSE) {
        try {
            reverse_flags(flags);
        } 
        catch (const string & em) {
            throw em;
        }
    }
}

void get_flags(vector<string> & commands, string man_page) {
    /* 
   "When I wrote this code, only God and I knew how it worked. Now, only God knows it!"

    This RegEx was acquired by empirically testing against the CLI man pages included in the unit-tests.
    For the most part I tried to make this RegEx as generic as possible to avoid overfitting to some one
    niche man page format but in some cases it was not possible (e.g. man zip or man jq).

    Some known limitations exist and are mentioned in https://github.com/HaraldsU/flag/blob/main/README.md
    */

    string reg_final {};
    CaptureGroupCount cgc {};

    // netstat has 3 spaces
    // +command, -c command (nvim)
    string reg_start {R"(
        (?:
            (?:^[ ]{3,6})\K
            (?:\+\w+,[ ](?=-\w))?\K
        )
    )"};

    // Start of the short for flag (e.g. -a)
    // The flag name can contain a dash in the middle, e.g. -fcheck-new (gcc)
    // The short flag can have arguments, e.g. -E ENCODING (rg) or -I[FMT] (date)
    // E.g. -w max[,here,near] (traceroute)
    // E.g. -O output-file (zip)
    // E.g. -D [bind_address:]port (ssh)
    // (1, 2, 3>)
    string reg_short_helper {R"(
        (
            (
                (?:-[#!?@$.\w]+)
                (?:-\w+)*
            )
            (
                (?:
                    (?:[\[\]_=<>?{}]|[ ](?![ ]|--|and[ ]--))|
                    ,(?![ \-])|
                    -(?![\-])|
                    :(?!\n)|
                    \w
                )+
            )?
        )
    )"};

    string reg_short =
        R"((?<!-[#?\w]\n[ ]{3}))" // This is to prevent -? from matching here (to enforce the no alias rule)
                                  // e.g. 2020 2020 202d 680a 2020 2020 202d 3f0a 2020 2020 202d 2d68 656c 700a (zip)
        R"((?<!-[#?\w]\n[ ]{4}))"
        R"((?<!-[#?\w]\n[ ]{5}))"
        R"((?<!-[#?\w]\n[ ]{6}))" + reg_short_helper;
    int reg_short_cgc = boost::regex{reg_short, boost::regex::no_mod_s | boost::regex::mod_x}.mark_count();
    cgc.reg_short = reg_short_cgc;

    // Start of the long flag (e.g. --all)
    // The flag name can contain a dash in the middle, e.g. --max-line-length (wc)
    // The long flag can have arguments, e.g. --follow[={name|descriptor}] (tail)
    // E.g. --host=[unix:///var/run/docker.sock] (docker)
    // E.g. --temp-path path (zip)
    // (1, 2?)
    string reg_long {R"(
        (
            (
                (?:--\w+)
                (?:-\w+)*
            ) 
            (
                (?:
                    [()\-\w\[\]{}|%+=<>?."~]|
                    [,:](?![ ]|\n)|
                    /(?![ ]-\w)|
                    [ ](?=\w|[\[\{\<\(])
                )+
            )?
        )
    )"};
    int reg_long_cgc = boost::regex{reg_long, boost::regex::no_mod_s | boost::regex::mod_x}.mark_count();
    cgc.reg_long = reg_long_cgc;

    string reg_sep {
        R"(
            (?:
                (?:[ ]|,[ ])(?=-)|
                \n[ ]{3,}|
                [ ]/[ ](?=-)|
        )"
            R"((?:,[ ]|\n[ ]{3,6})(?:)" + reg_short_helper + R"()(?:,[ ]|\n[ ]{3,6})(?=--\w))"
            R"(|)"
            R"(,[ ](?:)" + reg_long + R"(),[ ](?=-\w))"
        R"())"};
    int reg_sep_cgc = boost::regex{reg_sep, boost::regex::no_mod_s | boost::regex::mod_x}.mark_count();
    cgc.reg_sep = reg_sep_cgc;

    string reg_end {R"(
        (?=
           $|
           :(?:[ ]|\n)|
           (?:[ ]|,[ ])-+|
           [ ]and[ ]|
           [ ]/[ ]|
           [ ]{2,}|
           ,[ ]-\w{2,}|
           [ ]\(\w
        )
    )"};

    // e.g. -h, --help
    string reg_sl =
        "(" +
            reg_short +
            reg_sep +
            reg_long +
        ")"
    ;
    int reg_sl_cgc = boost::regex{reg_sl, boost::regex::no_mod_s | boost::regex::mod_x}.mark_count();
    cgc.reg_short_long = reg_sl_cgc;

    // e.g. --help, -h
    string reg_ls =
        "(" +
            reg_long +
            reg_sep +
            reg_short +
        ")"
    ;
    int reg_ls_cgc = boost::regex{reg_ls, boost::regex::no_mod_s | boost::regex::mod_x}.mark_count();
    cgc.reg_long_short = reg_ls_cgc;

    reg_final =
        reg_start +
        "(?:" +
            reg_sl +
                "|" +
            reg_ls +
                "|" +
            reg_long +
                "|" +
            reg_short +
        ")" +
        reg_end
    ;

    try {
        add_flags(commands, man_page, reg_final, cgc);
        if (commands.size() == 0) throw string{"no flags found (try -m)"};
    } 
    catch (const string & e) {
        throw e;
    }
}


/*
    @retval ExitCode::Version if -v was set.
    @retval ExitCode::Help if -h was set.
    @retval 0 otherwise.

    @throws If enclose count is not 0, 1 or 2, or there is a po:: parsing error
*/
int set_program_options(int end, char ** av) {
    boost::program_options::options_description desc {};
    po::options_description hidden("Hidden options");
    po::options_description all;

    desc.add_options()
        ("color", po::value<string>()->value_name("WHEN")->implicit_value("never")->default_value("never", ""), 
         "Use markers to highlight the output")

        ("column,c", po::value<int>()->value_name("N")->implicit_value(1)->default_value(1, ""), 
         "List entries by columns")

        ("delimiter,d", po::value<string>()->value_name("DEM")->implicit_value("'\\n'")->default_value("\n", ""), 
         "Specify delimiter used for separating flags")

        ("debug,D", po::bool_switch()->default_value(false), 
         "Show debug messages")

        ("enclose,e", po::value<string>()->value_name("ENC")->implicit_value("''")->default_value("", ""), 
         "Specify characters used for enclosing the output")

        // ("full,f", po::bool_switch()->default_value(false), "For each flag show one explanatory line from the man page") // TO DO

        ("help,h", po::bool_switch()->default_value(false), 
         "Show this help message")

        ("long,l", po::bool_switch()->default_value(false), 
         "Show long flag notation, e.g. --help")

        ("mixed,m", po::bool_switch()->default_value(false), 
         "Show mixed flag notation, e.g. -h|--help")

        ("mixed-only,M", po::bool_switch()->default_value(false), 
         "Show only flags that have both short and long notations")

        ("no-argument,n", po::bool_switch()->default_value(false), 
         "Don't include flag arguments")

        ("pretty,p", po::bool_switch()->default_value(false), 
         "Alias for '--color=always --enclose=[]'")

        ("reverse,r", po::bool_switch()->default_value(false), 
         "Reverse order of the output")

        ("short,s", po::bool_switch()->default_value(false), 
         "Show short flag notation, e.g. -h")

        ("sort,S", po::bool_switch()->default_value(false), 
         "Sort the output lexicographically (default by appearance)")

        ("title,t", po::bool_switch()->default_value(false), 
         "Show a line from the man page NAME section")

        ("version,v", po::bool_switch()->default_value(false), 
         "Display version information and exit")

        ("flag-count,w", po::bool_switch()->default_value(false), 
         "Show flag count")
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

		for (auto it = parsed.options.rbegin(); it != parsed.options.rend(); it++) {
			if (it->string_key == "version") {
				cout << "flag " << VERSION << endl;
				return ExitCode::Version;
			}
			else if (it->string_key == "help") {
                print_help(desc);
				return ExitCode::Help;
			}
		}

		if (!VM.count("input")) {
            print_help(desc);
			return ExitCode::Help;
		}

        if (!VM["reverse"].defaulted()) IS_REVERSE = true;
        if (!VM["sort"].defaulted()) IS_SORT = true;
        if (!VM["flag-count"].defaulted()) IS_FLAG_COUNT = true;
        if (!VM["title"].defaulted()) IS_TITLE = true;
        if (!VM["debug"].defaulted()) IS_DEBUG = true;
        if (!VM["no-argument"].defaulted()) IS_NO_ARGUMENT = true;

        DELIMITER = VM["delimiter"].as<string>();
        if (DELIMITER == "\\n") DELIMITER = "\n";
        else if (DELIMITER == "\\t") DELIMITER = "\t";

        if (VM["enclose"].as<string>().length() == 0) ENCLOSE_COUNT = 0;
        else if (VM["enclose"].as<string>().length() == 1) ENCLOSE_COUNT = 1;
        else if (VM["enclose"].as<string>().length() == 2) ENCLOSE_COUNT = 2;
        else throw string{"'--[e]nclose' excpects 0, 1 or 2 characters in the argument"};
        ENCLOSE = VM["enclose"].as<string>();

        COLUMN_COUNT = VM["column"].as<int>();

		MODE = Mode::Mixed;	// Default is -m
		for (auto it = parsed.options.rbegin(); it != parsed.options.rend(); it++) {
			if (it->string_key == "long") {
				MODE = Mode::Long;
				break;
			}
			else if (it->string_key == "short") {
				MODE = Mode::Short;
				break;
			}
			else if (it->string_key == "mixed") {
				MODE = Mode::Mixed;
				break;
			}
			else if (it->string_key == "mixed-only") {
				MODE = Mode::Mixed;
				IS_MIXED_ONLY = true;
				break;
			}
		}

        // Alias for '--color=always --enclose=[]
        if (!VM["pretty"].defaulted()) {
            IS_PRETTY = true;
            ENCLOSE_COUNT = 2;
            ENCLOSE = "[]";
            VM.at("enclose").value() = string("[]");
            VM.at("color").value() = string("always");
        }
    } 
    catch (const po::error & e) {
        throw string{e.what()};
    }

    return 0;
}

string set_man_page() {
	setenv("MANWIDTH", "1000", 1); // Needed for not wrapping long lines
    auto input = boost::algorithm::join(VM["input"].as<vector<string>>(), " ");
    string command {"man " + input + " 2>/dev/null | " + "cat"};
    string man_page {};

    auto po = popen(command.c_str(), "r");
    if (!po) throw string{"failed to run man"};

    char buf[1024];
    while (fgets(buf, 1024, po)) man_page.append(buf);

    int status = pclose(po);
    if (status == -1 or !WIFEXITED(status) or WEXITSTATUS(status) != 0 or man_page.empty()) {
        throw string{"man page for command '" + input + "' not found"};
    }

    return man_page;
}

void add_colors(string & output, int pipe_pos) {
    // 31: red; 32: green; 33: yellow
    bool is_solo_long = false;
    if (ENCLOSE_COUNT == 0) (output[1] == '-') ? is_solo_long = true : is_solo_long = false;
    else (output[2] == '-') ? is_solo_long = true : is_solo_long = false;


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
    if (ENCLOSE_COUNT == 1) output = ENCLOSE[0] + output + ENCLOSE[0];
    else if (ENCLOSE_COUNT == 2) output = ENCLOSE[0] + output + ENCLOSE[1];
}

