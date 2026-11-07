#pragma once
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

inline boost::program_options::variables_map vm;

enum class Mode { Short, Long, Mixed};
inline const std::unordered_map<std::string, Mode> kModes {
    {"short", Mode::Short},
    {"long",  Mode::Long},
    {"mixed",  Mode::Mixed},
    {"mixed-only",  Mode::Mixed}
};

inline Mode mode                                = Mode::Mixed;
inline bool is_mixed_only                       = false;
inline bool is_reverse                          = false;
inline int  enclose_count                       = 2;
inline bool is_word_count                       = false;
inline bool is_title                            = false;
inline bool is_sort                             = false;
inline bool is_debug                            = false;
inline std::unordered_map<Mode, int> flag_count = {};

int set_program_options(int end,char ** av);
void print_debug();
std::string load_man_page(int ac,char ** av);
std::string set_delimiter();

struct ilexic {
    // Strip the dashes at the start to start comparing with alnum
    static std::string strip_start(const std::string & s) {
        auto it = find_if(s.begin(), s.end(), [](unsigned char c) { return isalnum(c); });
        return std::string(it, s.end());
    }

    static std::string lower(std::string s) {
        for (auto & c : s) c = tolower(static_cast<unsigned char>(c));
        return s;
    }

    bool operator()(const std::string & a, const std::string & b) const {
        std::string x = strip_start(a);
        std::string y = strip_start(b);
        if (is_reverse == true) return make_tuple(lower(y), y, b) < make_tuple(lower(x), x, a);
        else return make_tuple(lower(x), x, a) < make_tuple(lower(y), y, b);
    }
};

void print_sub_matches_debug(boost::smatch sm);
template <typename A>
void add_commands(A & commands, std::string man_page, std::string regx) {
    const boost::regex expr(regx, boost::regex::no_mod_s | boost::regex::mod_x);
    boost::sregex_iterator sr_iterator(man_page.begin(), man_page.end(), expr);

    for (auto mr = sr_iterator; mr != boost::sregex_iterator(); mr++) {
        std::string command {};
        auto sm = *mr;

        if (is_debug) print_sub_matches_debug(sm); // DEBUG

        if (mode == Mode::Short) { 
            if (sm[4].matched) {
                if (is_debug) std::cout << "LS" << std::endl;
                command = sm[4];
            }
            else if (sm[3].matched) {
                if (is_debug) std::cout << "SHORT OR SL" << std::endl;
                command = sm[3];
            }
            else { 
                if (is_debug) std::cout << "NO MATCH" << std::endl;
                continue;
            }
            flag_count[Mode::Short]++;
        }

        else if (mode == Mode::Long) {
            if (sm[3].matched) {
                if (is_debug) std::cout << "SL" << std::endl;
                command = sm[3];
            }
            else if (sm[4].matched) {
                if (is_debug) std::cout << "LONG OR LS" << std::endl;
                command = sm[4];
            }
            else {
                if (is_debug) std::cout << "NO MATCH" << std::endl;
                continue;
            }
            flag_count[Mode::Long]++;
        } 

        else if (mode == Mode::Mixed) {
            if (sm[5].matched) {
                if (is_debug) std::cout << "LS" << std::endl;
                command = sm[7] + "," + sm[6];
                flag_count[Mode::Mixed]++;
            }
            else if (sm[2].matched) {
                if (is_debug) std::cout << "SL" << std::endl;
                command = sm[3] + "," + sm[4];
                flag_count[Mode::Mixed]++;
            }
            else if (!is_mixed_only) {
                if (sm[8].matched) {
                    if (is_debug) std::cout << "SHORT" << std::endl;
                    command = sm[8];
                    flag_count[Mode::Short]++;
                }
                else if (sm[10].matched) {
                    if (is_debug) std::cout << "LONG" << std::endl;
                    command = sm[10];
                    flag_count[Mode::Long]++;
                }
                else {
                    if (is_debug) std::cout << "NO MATCH" << std::endl;
                    continue;
                }
            }
            else continue;
        } else throw std::string{"Invalid flag notation (expected short, long or mixed)"};

        if (is_debug) std::cout << std::endl;
        bool found = find(commands.begin(), commands.end(), command) != commands.end();
        if (!found) {
            if (is_reverse == false) commands.insert(commands.end(), command);
            else commands.insert(commands.begin(), command);
        }
    }
}

template <typename A>
void get_commands(A & commands, std::string man_page) {
    std::string reg_final {};

    // netstat has 3 spaces
    std::string reg_start {R"(
        # START
        (?:^[ ]{3,6})\K
    )"};

    // Short form starts with - followed by \w, e.g. -a (ls)
    // The flag name can contain a dash in the middle, e.g. -fcheck-new (gcc)
    // (needs example ???)
    // This is a valid flag -E ENCODING (rg)
    // -I[FMT] (date)
    std::string reg_short {R"(
        # SHORT
        (
            (?:-[#\w]+)
            (?:-\w+)*
            (?:
                [ ]?
                (?:
                    (?:[\w\[\]_:=<>]|
                        [ ](?![ ]))|
                        ,(?![ \-])|
                        \-(?![\-])
                )+
            )?
        )
    )"};

    // Long form starts with -- followed by \w, e.g. --all (ls)
    // The flag name can contain a dash in the middle, e.g. --max-line-length (wc)
    // This is a valid flag --follow[={name|descriptor}] (tail)
    // This is the usual notation: --date=STRING (date)
    // [--bytes=[-]NUM] (head) and [-L --license -V --version] (bzip2)
    std::string reg_long {R"(
        # LONG
        (
            (?:--\w+)
            (?:-\w+)*
            (?:\[)?
            (?:
                (?:=|[ ](?!\-))
                (?:
                    [\-\w\[\]{}|+=<>."/~]|
                    [,:](?![ ])
                )+
            )?
        )
    )"};

    std::string reg_sep {R"(
        # SEP
        (?:
            \n[ ]{3,}|
            [ |, ](?=--)|
            [ ]-\w|
            [^-\w\n]{2,}|
            [ ]\(
        )
    )"};

    std::string reg_end {R"(
        # END
        (?=
           $|
           :[ ]|
           (?:[ ]|,[ ])-+|
           [ ]{2,}\w|
           ,[ ]-\w{2,}|
           [ ]\(\w
        )
    )"};

    // LONG-SHORT (want to capture SHORT)
    std::string reg_sls =
        "(?:" +
            reg_long + reg_sep +
        ")" +
        reg_short // Actual capture
    ;

    // SHORT-LONG (want to capture LONG)
    std::string reg_lsl =
        "(?:" +
            reg_short + reg_sep +
        ")" +
        reg_long // Actual capture
    ;

    // SHORT-LONG (want to capture BOTH)
    std::string reg_sl = 
        "(" +
            reg_short + // Capture
            reg_sep +
            reg_long +  // Capture
        ")"
    ;

    // LONG-SHORT (want to capture BOTH)
    std::string reg_ls =
        "(" +
            reg_long +  // Capture
            reg_sep +
            reg_short + // Capture
        ")"
    ;

    if (mode == Mode::Short) {
        reg_final = 
            reg_start +
            "(" +           // (1)
                reg_sls +   // (2...6)
                "|" +
                reg_short + // (7...10...)
            ")" +
            reg_end
        ;
    } else if (mode == Mode::Long) {
        reg_final = 
            reg_start +
            "(" +           // (1)
                reg_lsl +   // (2...6)
                "|" +
                reg_long +  // (6...10...)
            ")" +
            reg_end
        ;
    } else if (mode == Mode::Mixed) {
        reg_final = 
            reg_start +
            "(" +                       // (1)
                reg_sl +                // (2...9)
                "|" +
                reg_ls +                // (10...17)
                "|" +
                "(" + reg_short + ")" + // (18...21)
                "|" +
                "(" + reg_long +  ")" +  // (22...25)
            ")" +
            reg_end
        ;
    } else throw std::string{"Invalid flag notation (expected short, long or mixed)"};

    // std::cout << "reg_final: " << reg_final << std::endl;

    try {
        add_commands(commands, man_page, reg_final);
        if (commands.size() == 0) throw std::string{"No flags found (try flag -m)"};
    } catch (const std::string & e) { throw e; }
}

template <typename A>
std::vector<std::string> get_n_longest_flags(const A commands) {
    int n = vm["column"].as<int>();
    std::vector<std::string> sorted(commands.begin(), commands.end());
    sort(sorted.begin(), sorted.end(),
        [](const std::string& a, const std::string& b) { return a.size() > b.size(); }
    );

    std::vector<std::string> res = {};
    auto it = sorted.begin();
    for (int i = 0; i < n && it != sorted.end(); i++) {
        res.push_back(*it);
        it++;
    }

    return res;
}

void print_flag_count();
void print_title(std::string man_page);
void add_colors(std::string & output,int pipe_pos);
void add_enclosing(std::string & output);

template <typename A>
void print_commands_helper(A commands, std::string delim) {
auto lf = get_n_longest_flags(commands);
int longest_flag_n = lf[0].length();

// int char_count = 0;
// for (auto i : lf) char_count += i.length();
// (kColumn_width < char_count) 
    // ? std::cout << "Can NOT fit " << col_n << " columns" << std::endl
    // : std::cout << "CAN fit " << col_n << " columns" << std::endl;
// std::cout << std::endl;
// delim = "";

int count = 0;
int extra_padding = 2 + enclose_count;
int col_n = vm["column"].as<int>();

for (auto iter = commands.begin(); iter != commands.end(); iter++) {
    if (std::next(iter) == commands.end()) delim = "";

    std::string command = *iter;
    std::string output  = "";
    int pipe_pos   = 0;
    if (enclose_count != 0) add_enclosing(command);

    if (mode == Mode::Mixed) {
        int i = 0;
        for (auto c : command) {
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
    else output = command;

    auto tmp_output = output;
    auto when = vm["color"].as<std::string>();
    if (when == "always" or (when == "auto" and isatty(STDOUT_FILENO))) add_colors(output, pipe_pos);

    if (col_n > 1) {
        std::string offset(longest_flag_n + extra_padding - tmp_output.length(), ' ');
        std::cout << output << offset << delim;

        count++;
        if (count == col_n) {
            std::cout << std::endl;
            count = 0;
        }
    }
    else std::cout << output << delim;
}
std::cout << std::endl;
}

template <typename A>
void print_commands(A commands, std::string delim, std::string man_page) {
    print_commands_helper(commands, delim);
    if (is_title) print_title(man_page);
    if (is_word_count) print_flag_count();
}

inline void reset_globals() {
    vm.clear();
    mode          = Mode::Mixed;
    is_mixed_only = false;
    is_reverse    = false;
    enclose_count = 2;
    is_word_count = false;
    is_title      = false;
    is_sort       = false;
    is_debug      = false;
    flag_count.clear();
}

