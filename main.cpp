#include "flag.h"
#include <variant>

int main(int ac, char ** av) {
    try {
        auto spo = set_program_options(ac, av);

        if (spo == 1) return 0;
        if (IS_DEBUG) print_debug();

        std::string man_page = set_man_page();

        std::variant<std::set<std::string, ilexic>, std::vector<std::string>> commands;
        if (IS_SORT == true) commands = std::set<std::string, ilexic>{};
        else commands = std::vector<std::string>{};
        visit([&](auto & c) { get_flags(c, man_page); }, commands);

        visit([&](auto & c) { print_flags(c, man_page); }, commands);

        return 0;
    } catch (const std::string & e) {
        std::cout << e << std::endl;
        return 1;
    }
}

