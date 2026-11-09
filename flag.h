#pragma once
#include <tuple>
#include <string>
#include <unistd.h>
#include <iostream>
#include <boost/regex.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>

using namespace std;

enum class Mode { Short, Long, Mixed};
inline const unordered_map<string, Mode> kMODES {
    {"short"      , Mode::Short},
    {"long"       , Mode::Long },
    {"mixed"      , Mode::Mixed},
    {"mixed-only" , Mode::Mixed}
};

const inline int kCOLUMN_WIDTH                  = 100;
const inline string VERSION                     = "1.0.0";
inline boost::program_options::variables_map VM = {};
inline Mode MODE                                = Mode::Mixed;
inline bool IS_MIXED_ONLY                       = false;
inline bool IS_REVERSE                          = false;
inline int  ENCLOSE_COUNT                       = 2;
inline int  COLUMN_COUNT                        = 1;
inline bool IS_WORD_COUNT                       = false;
inline bool IS_TITLE                            = false;
inline bool IS_SORT                             = false;
inline bool IS_DEBUG                            = false;
inline string DELIMITER                         = "\n";
inline unordered_map<Mode, int> FLAG_COUNT      = {};
inline boost::program_options::options_description DESC ("Usage: flag [flags] [argument]\nFlags", kCOLUMN_WIDTH);

// DEBUGGING
int debug_print();

void debug_print_sub_matches(const boost::smatch sm);
// DEBUGGING

// PRINTING
int print_help();

int print_program_version();

void print_flag_count();

string print_title(const string man_page);

void print_flags_helper(const vector<string> commands);

void print_flags(const vector<string> commands, const string man_page);
// PRINTING

// GETTING
void get_flags(vector<string> & commands, string man_page);

vector<string> get_n_longest_flags(const vector<string> commands);
// GETTING

int sort_flags(vector<string> & flags);

int reverse_flags(vector<string> & flags);

// SETTING / ADDING
int set_program_options(int end,char ** av);

string set_man_page();

void add_colors(string & output,int pipe_pos);

void add_enclosing(string & output);

void add_flags(vector<string> & commands, string man_page, string regx);

string set_delimiter();
// SETTING / ADDING

inline void reset_globals() {
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

