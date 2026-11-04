#include "flag.h"

int main(int ac, char ** av) {
    try {
        auto end = get_command_end_idx(ac, av);
        auto spo = set_program_options(end, av);

        if (spo == 1) return 0;
        if (is_debug) print_debug();

        std::string man_page {};
        man_page = load_man_page(ac, av);

        std::variant<std::set<std::string, ilexic>, std::vector<std::string>> commands;
        if (is_sort == true) commands = std::set<std::string, ilexic>{};
        else commands = std::vector<std::string>{};
        visit([&](auto & c) { get_commands(c, man_page); }, commands);

        std::string delim = set_delimiter();
        visit([&](auto & c) { print_commands(c, delim, man_page); }, commands);

        return 0;
    } catch (const std::string & e) {
        std::cout << e << std::endl;
        return 1;
    }
}

