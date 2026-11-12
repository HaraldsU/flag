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

struct ilexic {
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

    // --enclose
    cout << "Enclose count: " << ENCLOSE_COUNT << endl;
    cout << "Enclose: "       << "'" << VM["enclose"].as<string>() << "'" << endl;

    // --debug
    cout << "Debug: "         << boolalpha     << IS_DEBUG   << endl;

    // Modes (--long, --mixed, --mixed-only, --short)
    auto m = int(MODE) == 0 ? "short"
        : int(MODE) == 1 ? "long"
        : "mixed";
    cout << "Mode: "          << int(MODE)     << " (" << m << ")" << endl;
    cout << "Mixed-only: "    << boolalpha     << IS_MIXED_ONLY << endl;

    // --reverse
    cout << "Reverse: "       << boolalpha     << IS_REVERSE << endl;

    // --sort
    cout << "Sort: "          << boolalpha     << IS_SORT << endl;

    // --title
    cout << "Title: "         << boolalpha     << IS_TITLE      << endl;

    // --version
    cout << "Version: "       << VERSION       << endl;

    // --flag-count
    cout << "Flag count: "    << boolalpha     << IS_FLAG_COUNT << endl;
    cout << endl;

    return 0;
}

void debug_print_sub_matches(boost::smatch sm) {
    for (size_t i = 0; i < sm.size(); i++) {
        if (sm[i].matched) cout << "s[" << i << "]: " << sm[i] << endl;
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


int sort_flags(vector<string> & flags) {
    try {
        sort(flags.begin(), flags.end(), ilexic());
    } catch (const exception & e) {
        throw e.what();
    }

    return 0;
}

int reverse_flags(vector<string> & flags) {
    try {
        reverse(flags.begin(), flags.end());
    } catch (const exception & e) {
        throw e.what();
    }
    
    return 0;
}

int add_flags_mixed(vector<string> & flags, boost::smatch sm) {
    string command {};
    vector<string> captured_flags {};
    Mode mode {};

    if (sm[2].matched) {
        if (IS_DEBUG) cout << "SL" << endl;
        captured_flags.push_back(sm[3]);
        captured_flags.push_back(sm[10]);
        FLAG_COUNT[Mode::Mixed]++;
        mode = Mode::Mixed;

        if (!IS_NO_ARGUMENT) command = sm[3] + "," + sm[10];
        else command = sm[4] + "," + sm[11];
    }
    else if (sm[13].matched) {
        if (IS_DEBUG) cout << "LS" << endl;
        captured_flags.push_back(sm[14]);
        captured_flags.push_back(sm[21]);
        FLAG_COUNT[Mode::Mixed]++;
        mode = Mode::Mixed;

        if (!IS_NO_ARGUMENT) command = sm[21] + "," + sm[14];
        else command = sm[22] + "," + sm[15];
    }
    else if (!IS_MIXED_ONLY) {
        if (sm[24].matched) {
            if (IS_DEBUG) cout << "SHORT" << endl;
            FLAG_COUNT[Mode::Short]++;
            mode = Mode::Short;

            if (!IS_NO_ARGUMENT) { 
                command = sm[24];
                captured_flags.push_back(sm[24]);
            }
            else { 
                command = sm[25];
                captured_flags.push_back(sm[25]);
            }
        }
        else if (sm[27].matched) {
            if (IS_DEBUG) cout << "LONG" << endl;
            FLAG_COUNT[Mode::Long]++;
            mode = Mode::Long;
            captured_flags.push_back(sm[28]);

            if (!IS_NO_ARGUMENT) command = sm[27];
            else command = sm[28];
        }
        else {
            if (IS_DEBUG) cout << "NO MATCH" << endl;
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

int add_flags_short_long(vector<string> & flags, boost::smatch sm) {
    string command {};
    string flag {};
    Mode mode {};

    if (MODE == Mode::Short) { 
        if (sm[8].matched) {
            if (IS_DEBUG) cout << "LS" << endl;
            flag = sm[8];

            if (!IS_NO_ARGUMENT) command = sm[8];
            else command = sm[9];
        }
        else if (sm[11].matched) {
            if (IS_DEBUG) cout << "SHORT OR SL" << endl;
            flag = sm[11];

            if (!IS_NO_ARGUMENT) command = sm[11];
            else command = sm[12];
        }
        else { 
            if (IS_DEBUG) cout << "NO MATCH" << endl;
            return 1;
        }
        FLAG_COUNT[Mode::Short]++;
        mode = Mode::Short;
    }
    else if (MODE == Mode::Long) {
        if (sm[8].matched) {
            if (IS_DEBUG) cout << "SL" << endl;
            flag = sm[8];   // Make --indicator-style=WORD and -p, --indicator-style=slash work (ls)

            if (!IS_NO_ARGUMENT) command = sm[8];
            else command = sm[9];
        }
        else if (sm[11].matched) {
            if (IS_DEBUG) cout << "LONG OR LS" << endl;
            flag = sm[12];

            if (!IS_NO_ARGUMENT) command = sm[11];
            else command = sm[12];
        }
        else {
            if (IS_DEBUG) cout << "NO MATCH" << endl;
            return 1;
        }
        FLAG_COUNT[Mode::Long]++;
        mode = Mode::Long;
    } 

    bool found = find(FLAGS_ONLY.begin(), FLAGS_ONLY.end(), flag) != FLAGS_ONLY.end();
    if (!found) flags.push_back(command);
    else FLAG_COUNT[mode]--;
    FLAGS_ONLY.insert(flag);

    return 0;
}

void add_flags(vector<string> & flags, string man_page, string regx) {
    const boost::regex expr(regx, boost::regex::no_mod_s | boost::regex::mod_x);
    boost::sregex_iterator sr_iterator(man_page.begin(), man_page.end(), expr);

    for (auto mr = sr_iterator; mr != boost::sregex_iterator(); mr++) {
        auto sm = *mr;

        if (IS_DEBUG) debug_print_sub_matches(sm);

        if (MODE == Mode::Short or MODE == Mode::Long) {
            auto r = add_flags_short_long(flags, sm);
            if (r == 1) continue;
        }
        else if (MODE == Mode::Mixed) {
            auto r = add_flags_mixed(flags, sm);
            if (r == 1) continue;
        } 
        else throw string{"invalid flag notation (expected short, long or mixed)"};

        if (IS_DEBUG) cout << endl;
    }

    if (IS_SORT) {
        try {
            sort_flags(flags);
        } catch (const string & em) {
            throw em;
        }
    }

    if (IS_REVERSE) {
        try {
            reverse_flags(flags);
        } catch (const string & em) {
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
    // (1, 2?)
    string reg_short_helper {R"(
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
    )"};

    string reg_short =
        R"((?<!-[#?\w]\n[ ]{3}))" // This is to prevent -? from matching here (to enforce the no alias rule)
                                  // e.g. 2020 2020 202d 680a 2020 2020 202d 3f0a 2020 2020 202d 2d68 656c 700a (zip)
        R"((?<!-[#?\w]\n[ ]{4}))"
        R"((?<!-[#?\w]\n[ ]{5}))"
        R"((?<!-[#?\w]\n[ ]{6}))" + reg_short_helper;

    // Start of the long flag (e.g. --all)
    // The flag name can contain a dash in the middle, e.g. --max-line-length (wc)
    // The long flag can have arguments, e.g. --follow[={name|descriptor}] (tail)
    // E.g. --host=[unix:///var/run/docker.sock] (docker)
    // E.g. --temp-path path (zip)
    // (1, 2?)
    string reg_long {R"(
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
    )"};

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

    // (1) + (rl) + (sep) + (rs) LONG-SHORT (want to capture SHORT)
    string reg_sls =
        "(?:" +
            reg_long + reg_sep +
        ")" +
        "(" + reg_short + ")" // Actual capture
    ;

    // (1) + (rs) + (sep) + (rl) SHORT-LONG (want to capture LONG)
    string reg_lsl =
        "(?:" +
            reg_short + reg_sep +
        ")" +
        "(" + reg_long + ")" // Actual capture
    ;

    // (1, 2, 3) + (rs) + (sep) + (rl) SHORT-LONG (want to capture BOTH)
    string reg_sl = 
        "((" + reg_short + ")" + // Capture
            reg_sep +
            "(" + reg_long + ")" +  // Capture
        ")"
    ;

    // (1, 2, 3) + (rl) + (sep) + (rs) LONG-SHORT (want to capture BOTH)
    string reg_ls =
        "((" + reg_long + ")" +  // Capture
            reg_sep +
            "(" + reg_short + ")" + // Capture
        ")"
    ;

    if (MODE == Mode::Short) {
        reg_final =                     // 0
            reg_start +
            "(" +                       // 1
                reg_sls +               // 2, 3, 4, 5, 6, 7, 8 (full), 9 (flag), 10 (arg)
                "|" +
                "(" + reg_short + ")" + // 11 (full), 12 (flag), 13 (arg)
            ")" +
            reg_end
        ;
    } else if (MODE == Mode::Long) {
        reg_final =                    // 0
            reg_start +
            "(" +                      // 1
                reg_lsl +              // 2, 3, 4, 5, 6, 7, 8 (full), 9 (flag), 10 (arg)
                "|" +
                "(" + reg_long + ")" + // 11 (full), 12 (flag), 13 (arg)
            ")" +
            reg_end
        ;
    } else if (MODE == Mode::Mixed) {
        reg_final =                     // 0
            reg_start +
            "(" +                       // 1
                reg_sl +                // 2, 3 (full_s), 4 (flag_s), 5 (arg_s), 6, 7, 8, 9, 10 (full_l), 11 (flag_l), 12 (arg_l)
                "|" +
                reg_ls +                // 13, 14 (full_l), 15 (flag_l), 16 (arg_l), 17, 18, 19, 20, 21 (full_s), 22 (flag_s), 23 (arg_s)
                "|" +
                "(" + reg_short + ")" + // 24 (full), 25 (flag), 26 (arg)
                "|" +
                "(" + reg_long +  ")" + // 27 (full), 28 (flag), 29 (arg)
            ")" +
            reg_end
        ;
    } else throw string{"invalid flag notation (expected short, long or mixed)"};
    // cout << reg_final << endl;

    try {
        add_flags(commands, man_page, reg_final);
        if (commands.size() == 0) throw string{"no flags found (try flag -m)"};
    } catch (const string & e) { throw e; }
}


/*
    @retval ExitCode::Version if -v was set.
    @retval ExitCode::Help if -h was set.
    @retval 0 otherwise.

    @throws If enclose count is not 0, 1 or 2, or there is a po:: parsing error
*/
int set_program_options(int end, char ** av) {
    boost::program_options::options_description desc (
            "flag is a command-line tool for fast discovery of command flags from man pages.\n"
            "See the man page for more information.\n\n"
            "Usage: flag [flags] [arguments]\n"
            "Flags", kCOLUMN_WIDTH);
    po::options_description hidden("Hidden options");
    po::options_description all;

    desc.add_options()
        ("color", po::value<string>()->value_name("WHEN")->implicit_value("never")->default_value("never", ""), "Use markers to highlight the output; WHEN is 'always', 'never' or 'auto'")

        ("column,c", po::value<int>()->value_name("COL")->implicit_value(1)->default_value(1, ""), "List entries by columns; COL specifies the column count")

        ("delimiter,d", po::value<string>()->value_name("DEM")->implicit_value("'\\n'")->default_value("\n", ""), "Specify the delimiter (DEL) used for separating flags; the characters '\\n' and '\\t' retain special meaning")

        ("debug,D", po::bool_switch()->default_value(false), "Show debug messages")

        ("enclose,e", po::value<string>()->value_name("ENC")->implicit_value("''")->default_value("", ""), "Specify characters (ENC) used for 'enclosing' the output (prepending and appending); 0, 1 or 2 arguments are expected")

        // ("full,f", po::bool_switch()->default_value(false), "For each flag show one explanatory line from the man page") // TO DO

        ("help,h", po::bool_switch()->default_value(false), "Show this help message")

        ("long,l", po::bool_switch()->default_value(false), "Show long flag notation, e.g. --help")

        ("mixed,m", po::bool_switch()->default_value(true), "Show mixed flag notation, e.g. -h|--help")

        ("mixed-only,M", po::bool_switch()->default_value(false), "Show only flags that have both short and long notations")

        ("no-argument,n", po::bool_switch()->default_value(false), "Don't include the flag arguments")

        ("pretty,p", po::bool_switch()->default_value(false), "Alias for '-Mn -c 2 -e [] --color=auto'")

        ("reverse,r", po::bool_switch()->default_value(false), "Reverse the order of the output")

        ("short,s", po::bool_switch()->default_value(false), "Show short flag notation, e.g. -h")

        ("sort,S", po::bool_switch()->default_value(false), "Sort the output lexicographically (by default the flags are sorted by the order of appearance)")

        ("title,t", po::bool_switch()->default_value(false), "Show a sentence from the man page NAME section")

        ("version,v", po::bool_switch()->default_value(false), "Display version information and exit")

        ("flag-count,w", po::bool_switch()->default_value(false), "Show the flag count")
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

        if (VM["version"].as<bool>() == true) {
            cout << "flag " << VERSION << endl;
            return ExitCode::Version;
        }
        else if (VM["help"].as<bool>() == true or !VM.count("input")) {
            cout << desc << endl;
            return ExitCode::Help;
        }

        if (!VM["reverse"].defaulted()) IS_REVERSE = true;
        if (!VM["sort"].defaulted()) IS_SORT = true;
        if (!VM["flag-count"].defaulted()) IS_FLAG_COUNT = true;
        if (!VM["mixed-only"].defaulted()) IS_MIXED_ONLY = true;
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

        MODE = Mode::Mixed;
        for (const auto& opt : parsed.options) {
            auto it = kMODES.find(opt.string_key);
            if (it != kMODES.end())
                MODE = it->second;
        }
        // Alias for '-Mn -c 2 -e [] --color=auto'
        if (!VM["pretty"].defaulted()) {
            IS_PRETTY = true;
            IS_MIXED_ONLY = true;
            IS_NO_ARGUMENT = true;
            COLUMN_COUNT = 2;
            ENCLOSE_COUNT = 2;
            ENCLOSE = "[]";
            VM.at("enclose").value() = string("[]");
            VM.at("color").value() = string("auto");
        }
    } catch (const po::error & e) {
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
    char buf[1024];
    while (fgets(buf, 1024, po)) man_page.append(buf);
    pclose(po);

    if (man_page.size() == 0) throw string{"man page for command \"" + input + "\" not found"};

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

