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

    // --word-count
    cout << "Word count: "    << boolalpha     << IS_WORD_COUNT << endl;
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
    cout <<    "COUNT: "
         << " | SF: " << FLAG_COUNT[Mode::Short]
         << " | LF: " << FLAG_COUNT[Mode::Long]
         << " | MF: " << FLAG_COUNT[Mode::Mixed]
         << " | TT: " << FLAG_COUNT[Mode::Short] + FLAG_COUNT[Mode::Long] + FLAG_COUNT[Mode::Mixed]
         << " |" << endl;
}

string print_title(const string man_page) {
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
        auto title = print_title(man_page); 
        cout << "NAME: " << title << endl;
    }
    if (IS_WORD_COUNT) print_flag_count();
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

void add_flags(vector<string> & flags, string man_page, string regx) {
    const boost::regex expr(regx, boost::regex::no_mod_s | boost::regex::mod_x);
    boost::sregex_iterator sr_iterator(man_page.begin(), man_page.end(), expr);

    for (auto mr = sr_iterator; mr != boost::sregex_iterator(); mr++) {
        string command {};
        auto sm = *mr;

        if (IS_DEBUG) debug_print_sub_matches(sm); // DEBUG

        if (MODE == Mode::Short) { 
            if (sm[2].matched) {
                if (IS_DEBUG) cout << "LS" << endl;
                command = sm[2];
            }
            else if (sm[3].matched) {
                if (IS_DEBUG) cout << "SHORT OR SL" << endl;
                command = sm[3];
            }
            // else if (sm[2].matched) {
                // if (IS_DEBUG) cout << "LLS" << endl;
                // command = sm[2];
            // }
            else { 
                if (IS_DEBUG) cout << "NO MATCH" << endl;
                continue;
            }
            FLAG_COUNT[Mode::Short]++;
        }

        else if (MODE == Mode::Long) {
            if (sm[2].matched) {
                if (IS_DEBUG) cout << "SL" << endl;
                command = sm[2];
            }
            else if (sm[3].matched) {
                if (IS_DEBUG) cout << "LONG OR LS" << endl;
                command = sm[3];
            }
            else {
                if (IS_DEBUG) cout << "NO MATCH" << endl;
                continue;
            }
            FLAG_COUNT[Mode::Long]++;
        } 

        else if (MODE == Mode::Mixed) {
            if (sm[2].matched) {
                if (IS_DEBUG) cout << "LS" << endl;
                command = sm[3] + "," + sm[4];
                FLAG_COUNT[Mode::Mixed]++;
            }
            else if (sm[5].matched) {
                if (IS_DEBUG) cout << "LLS" << endl;
                command = sm[7] + "," + sm[6];
                FLAG_COUNT[Mode::Mixed]++;
            }
            else if (!IS_MIXED_ONLY) {
                if (sm[8].matched) {
                    if (IS_DEBUG) cout << "SHORT" << endl;
                    command = sm[8];
                    FLAG_COUNT[Mode::Short]++;
                }
                else if (sm[9].matched) {
                    if (IS_DEBUG) cout << "LONG" << endl;
                    command = sm[9];
                    FLAG_COUNT[Mode::Long]++;
                }
                else {
                    if (IS_DEBUG) cout << "NO MATCH" << endl;
                    continue;
                }
            }
            else continue;
        } else throw string{"invalid flag notation (expected short, long or mixed)"};
        if (IS_DEBUG) cout << endl;

        // Remove duplicates
        bool found = find(flags.begin(), flags.end(), command) != flags.end();
        if (!found) flags.push_back(command);
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

    string reg_start {R"(
        (?:^[ ]{3,6})\K             # netstat has 3 spaces
        (?:\+\w+,[ ](?=-\w))?\K     # +command, -c command (nvim)
    )"};

    string reg_short_helper {R"(
        (?:-[#!?@$\w]+)                                         # Start of the short for flag (e.g. -a)
        (?:-\w+)*                                               # The flag name can contain a dash in the middle, e.g. -fcheck-new (gcc)
        (?:
            (?:
                (?:[\[\]_=<>?{}]|[ ](?![ ]|--|and[ ]--))|       # The short flag can have arguments, e.g. -E ENCODING (rg) or -I[FMT] (date)
                ,(?![ \-])|                                     # E.g. -w max[,here,near] (traceroute)
                -(?![\-])|                                      # E.g. -O output-file (zip)
                :(?!\n)|                                        # E.g. -D [bind_address:]port (ssh)
                \w
            )+
        )?
    )"};

    string reg_short =
        R"((?<!-[#?\w]\n[ ]{3}))" // This is to preven -? from matchin here (to enforce the no alias rule)
                                  // e.g. 2020 2020 202d 680a 2020 2020 202d 3f0a 2020 2020 202d 2d68 656c 700a (zip)
        R"((?<!-[#?\w]\n[ ]{4}))"
        R"((?<!-[#?\w]\n[ ]{5}))"
        R"((?<!-[#?\w]\n[ ]{6}))" + reg_short_helper;

    string reg_long {R"(
        (?:--\w+)                           # Start of the long flag (e.g. --all)
        (?:-\w+)*                           # The flag name can contain a dash in the middle, e.g. --max-line-length (wc)
        (?:
            (?:
                [()\-\w\[\]{}|+=<>."~]|     # The long flag can have arguments, e.g. --follow[={name|descriptor}] (tail)
                [,:](?![ ]|\n)|             # E.g. --host=[unix:///var/run/docker.sock] (docker)
                /(?![ ]-\w)|
                [ ](?=\w|[\[\{\<\(])        # E.g. --temp-path path (zip)
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
           :([ ]|\n)|
           (?:[ ]|,[ ])-+|
           [ ]and[ ]|
           [ ]/[ ]|
           [ ]{2,}|
           ,[ ]-\w{2,}|
           [ ]\(\w
        )
    )"};

    // LONG-SHORT (want to capture SHORT)
    string reg_sls =
        "(?:" +
            reg_long + reg_sep +
        ")" +
        "(" + reg_short + ")" // Actual capture
    ;

    // SHORT-LONG (want to capture LONG)
    string reg_lsl =
        "(?:" +
            reg_short + reg_sep +
        ")" +
        "(" + reg_long + ")" // Actual capture
    ;

    // SHORT-LONG (want to capture BOTH)
    string reg_sl = 
        "((" + reg_short + ")" + // Capture
            reg_sep +
            "(" + reg_long + ")" +  // Capture
        ")"
    ;

    // LONG-SHORT (want to capture BOTH)
    string reg_ls =
        "((" + reg_long + ")" +  // Capture
            reg_sep +
            "(" + reg_short + ")" + // Capture
        ")"
    ;

    if (MODE == Mode::Short) {
        reg_final = 
            reg_start +
            "(" +
                reg_sls +
                "|" +
                "(" + reg_short + ")" +
            ")" +
            reg_end
        ;
    } else if (MODE == Mode::Long) {
        reg_final = 
            reg_start +
            "(" +
                reg_lsl +
                "|" +
                "(" + reg_long + ")" +
            ")" +
            reg_end
        ;
    } else if (MODE == Mode::Mixed) {
        reg_final = 
            reg_start +
            "(" +
                reg_sl +
                "|" +
                reg_ls +
                "|" +
                "(" + reg_short + ")" +
                "|" +
                "(" + reg_long +  ")" +
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


int set_program_options(int end, char ** av) {
    boost::program_options::options_description desc ("Usage: flag [flags] [argument]\nFlags", kCOLUMN_WIDTH);
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

        if (VM["version"].as<bool>() == true) {
            cout << "flag " << VERSION << endl;
            return ExitCodes::Version;
        }
        else if (VM["help"].as<bool>() == true or !VM.count("input")) {
            cout << desc << endl;
            return ExitCodes::Help;
        }

        if (!VM["reverse"].defaulted()) IS_REVERSE = true;
        if (!VM["sort"].defaulted()) IS_SORT = true;
        if (!VM["word-count"].defaulted()) IS_WORD_COUNT = true;
        if (!VM["mixed-only"].defaulted()) IS_MIXED_ONLY = true;
        if (!VM["title"].defaulted()) IS_TITLE = true;
        if (!VM["debug"].defaulted()) IS_DEBUG = true;

        DELIMITER = VM["delimiter"].as<string>();
        if (DELIMITER == "\\n") DELIMITER = "\n";
        else if (DELIMITER == "\\t") DELIMITER = "\t";

        if (VM["enclose"].as<string>().length() == 0) ENCLOSE_COUNT = 0;
        else if (VM["enclose"].as<string>().length() == 1) ENCLOSE_COUNT = 1;
        else if (VM["enclose"].as<string>().length() == 2) ENCLOSE_COUNT = 2;
        else throw string{"'--enclose' excpects 0, 1 or 2 characters in the argument"};
        ENCLOSE = VM["enclose"].as<string>();

        COLUMN_COUNT = VM["column"].as<int>();

        MODE = Mode::Mixed;
        for (const auto& opt : parsed.options) {
            auto it = kMODES.find(opt.string_key);
            if (it != kMODES.end())
                MODE = it->second;
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

