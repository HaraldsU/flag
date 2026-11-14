#pragma once
#include <set>
#include <string>
#include <unistd.h>
#include <boost/regex.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>

using namespace std;

namespace ExitCode {
    enum : int {
        Success = 0,
        Error = 1,
        Help = 2,
        Version = 3
    };
}

enum class Mode {
    Short, 
    Long, 
    Mixed
};

inline const unordered_map<string, Mode> kMODES {
    {"short"      , Mode::Short},
    {"long"       , Mode::Long },
    {"mixed"      , Mode::Mixed},
    {"mixed-only" , Mode::Mixed}
};

inline unordered_map<Mode, int> FLAG_COUNT      = {};
inline boost::program_options::variables_map VM = {};
inline set<string> FLAGS_ONLY                   = {};

const inline int kCOLUMN_WIDTH                  = 100;
const inline string VERSION                     = "1.0.0";

inline Mode MODE                                = Mode::Mixed;
inline bool IS_MIXED_ONLY                       = false;

inline int  COLUMN_COUNT                        = 1;
inline bool IS_NO_ARGUMENT                      = false;
inline bool IS_PRETTY                           = false;
inline bool IS_DEBUG                            = false;

inline bool IS_REVERSE                          = false;
inline bool IS_SORT                             = false;

inline bool IS_FLAG_COUNT                       = false;
inline bool IS_TITLE                            = false;

inline string DELIMITER                         = "\n";
inline int  ENCLOSE_COUNT                       = 0;
inline string ENCLOSE                           = "";

int debug_print();
void print_flags_helper(const vector<string> flags);
void print_flags(const vector<string> flags, const string man_page);

string get_title(const string man_page);
void get_flags(vector<string> & flags, const string man_page);

int set_program_options(int end,char ** av);
string set_man_page();

void add_colors(string & output, int pipe_pos);
void add_enclosing(string & output);

