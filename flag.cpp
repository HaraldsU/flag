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
static bool show_long;
static bool show_short;

int set_program_options(int ac, char ** av) {
	desc.add_options()
		("help,h", "show help message")
		("short,s", po::bool_switch(), "show short flag notation, e.g. -h")
		("long,l", po::bool_switch(), "show long flag notation, e.g. --help")
	;

	po::parsed_options parsed = po::parse_command_line(ac, av, desc);
	po::store(parsed, vm);
	po::notify(vm); 

	if (vm.count("help")) {
		cout << desc << endl;

		return 1;
	}

	show_long = false;     
	for (const auto& opt : parsed.options) {
		if (opt.string_key == "short") show_long = false;
		else if (opt.string_key == "long") show_long = true;
	}
	show_short = !show_long;

	return 0;
}

struct iless {
	bool operator()(const string& a, const string& b) const {
		return boost::algorithm::ilexicographical_compare(a, b);
	}
};

int main(int ac, char ** av) {
	set_program_options(ac, av);

	// cout << ac << endl;
	if (ac < 2) {
		cout << desc << endl;
		return 1;
	}

	string command{ "man 1 "s + av[ac - 1] + " 2>/dev/null | "s + "cat"s};
	// cout << "DEBUG: command = " << command << endl;
	
	string man_page {};
	auto po = popen(command.c_str(), "r");
	char buf[1024];
	while (fgets(buf, 1024, po)) {
		man_page.append(buf);
	}
	pclose(po);

	string r{};
	if (show_short) {
		r = R"(
			(?:\s+)\K			# Start
			-[[:alnum:]]+		# Short form starts with -
			(?:-\w+)*			# Commands can have - in the middle, e.g. -names-only
			(?![\w-])			# Has to end with a word character, e.g. -no- is not valid
		)";
	}
	else if (show_long) {
		r = R"(
			(?:\s+)\K			# Start
			--[[:alnum:]]+		# Long form starts with --
			(?:-\w+)*			# Commands can have - in the middle, e.g. --names-only
			(?![\w-])			# Has to end with a word character, e.g. --no- is not valid
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

