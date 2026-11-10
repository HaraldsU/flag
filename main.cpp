#include "flag.h"
#include <iostream>

using namespace std;

int main(int ac, char ** av) {
    try {
        auto spo = set_program_options(ac, av);
        if (spo == ExitCodes::Help or spo == ExitCodes::Version) return 0;
        if (IS_DEBUG) debug_print();

        string man_page = set_man_page();
        vector<string> commands {};
        get_flags(commands, man_page);
        print_flags(commands, man_page);

        return 0;
    } catch (const string & em) {
        cout << "Flag: " << em << endl;

        return 1;
    }
}

