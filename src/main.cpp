#include "flag.h"
#include <iostream>

using namespace std;

int main(int ac, char ** av) {
    try {
        auto spo = set_program_options(ac, av);
        if (spo == ExitCode::Help or spo == ExitCode::Version) return 0;
        if (IS_DEBUG) debug_print();

        auto man_page = set_man_page();
        vector<string> commands {};
        get_flags(commands, man_page);
        print_flags(commands, man_page);

        // cout << "**********************" << endl;
        // cout << "***** FLAGS ONLY *****" << endl;
        // cout << "**********************" << endl;
        // for (auto el : FLAGS_ONLY) {
            // cout << el << endl;
        // }

        return 0;
    } catch (const string & em) {
        cout << "Flag: " << em << endl;

        return 1;
    }
}

