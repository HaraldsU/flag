#include <iostream>
#include <set>
#include <string>
#include <stdio.h>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/regex.hpp>

using namespace std;
namespace po = boost::program_options;

static po::options_description desc("Expected usage:\n  flag [flags] arguments\nAllowed flags");
static po::variables_map vm;

int set_program_options(int ac, char ** av) {
	desc.add_options()
		("help,h", "show help message")
		("long,l", "show long flag notation, e.g. --help")
		("short,s", "show short flag notation, e.g. -h")
	;

	po::store(po::parse_command_line(ac, av, desc), vm);
	po::notify(vm); 

	if (vm.count("help")) {
		cout << desc << endl;

		return 1;
	}

	return 0;
}

struct iless {
	bool operator()(const string& a, const string& b) const {
		return boost::algorithm::ilexicographical_compare(a, b);
	}
};

int main(int ac, char ** av) {
	set_program_options(ac, av);

	if (ac < 2) {
		cout << desc << endl;
		return 1;
	}

	string command{ "man 1 "s + av[ac - 1] + " 2>/dev/null | "s + "cat"s};
	cout << "DEBUG: command = " << command << endl;
	
	string man_page {};
	auto po = popen(command.c_str(), "r");
	char buf[1024];
	while (fgets(buf, 1024, po)) {
		man_page.append(buf);
	}
	pclose(po);

	string r{};
	if (vm.count("long")) {
		r = R"(
			(?:^|\s+|\W)\K		# Start
			--[[:alnum:]]+		# Longa form starts with --
			(?:-\w+)*			# Commands can have - in the middle, e.g. --names-only
			(?![\w-])			# Has to end with a word character, e.g. --no- is not valid
		)";
	}
	else if (vm.count("short")) {
		r = R"(
			(?:^|\s+|\W)\K		# Start
			-[[:alnum:]]+		# Short form starts with -
			(?:-\w+)*			# Commands can have - in the middle, e.g. -names-only
			(?![\w-])			# Has to end with a word character, e.g. -no- is not valid
		)";
	}

	const boost::regex expr(r, boost::regex::no_mod_s | boost::regex::mod_x);
	boost::sregex_iterator sr_iterator(man_page.begin(), man_page.end(), expr);

	set<string, iless> long_commands {};
	for (auto mr = sr_iterator; mr != boost::sregex_iterator(); mr++) {
		long_commands.insert(mr->str());
		// cout << *mr << endl;
	}

	// Print set
	for (auto i : long_commands) {
		cout << i << endl;
	}

	return 0;
}

