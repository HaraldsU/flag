#include <iostream>
#include <set>
#include <string>
#include <stdexcept>
#include <stdio.h>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/regex.hpp>

using namespace std;
namespace po = boost::program_options;

static po::options_description desc("Usage: flag [flags] arguments\nAllowed flags", 120);
static po::variables_map vm;

enum class Mode { Short, Long, Mixed};
static const std::unordered_map<std::string, Mode> kModes {
	{"short", Mode::Short},
	{"long",  Mode::Long},
	{"mixed",  Mode::Mixed}
};
static Mode mode = Mode::Short;

int set_program_options(int ac, char ** av) {
	desc.add_options()
		("help,h", "Show this help message")
		("short,s", po::bool_switch(), "Show short flag notation, e.g. -h")
		("long,l", po::bool_switch(), "Show long flag notation, e.g. --help")
		("mixed,m", po::bool_switch(), "Show mixed flag notation, e.g. -h [ --help ]")
		("delimiter,d", po::value<string>()->value_name("DELIM")->implicit_value("; ", "\"; \""), "Specify the delimiter used for separating flags, e.g. -d \"\\n\" or --delimiter=\"\\n\". The default delimiter is \"; \". The characters \"\\n\" and \"\\t\" retain special meaning, i.e. the new line and tab characters appropriately.")
	;

	po::parsed_options parsed = po::parse_command_line(ac, av, desc);
	po::store(parsed, vm);
	po::notify(vm); 

	if (vm.count("help")) {
		cout << desc << endl;
		return 1;
	}

	for (const auto& opt : parsed.options) {
		auto it = kModes.find(opt.string_key);
		if (it != kModes.end())
			mode = it->second;
	}

	return 0;
}

struct iless {
	static string alnum_only(const string& s) {
		string out;
		for (unsigned char c : s)
			if (std::isalnum(c)) out += c;

		return out;
	}

	bool operator()(const string& a, const string& b) const {
		return boost::algorithm::ilexicographical_compare(alnum_only(a), alnum_only(b));
	}
};

string load_man_page(int ac, char ** av) {
	string command{ "man "s + av[ac - 1] + " 2>/dev/null | "s + "cat"s};
	// cout << "DEBUG: command = " << command << endl;
	
	string man_page {};
	auto po = popen(command.c_str(), "r");
	char buf[1024];
	while (fgets(buf, 1024, po)) {
		man_page.append(buf);
	}
	pclose(po);

	if (man_page.size() == 0) throw std::string{"Man page for command \"" + std::string{av[ac -1]} + "\" not found"};
	else return man_page;
}

string set_delimiter() {
	string delim {"; "};
	if (vm.count("delimiter")) {
		delim = vm["delimiter"].as<string>();
		if (delim == "\\n") delim = "\n";
		else if (delim == "\\t") delim = "\t";
	}

	return delim;
}

void add_commands(set<string, iless> & commands, string man_page, string r) {
	const boost::regex expr(r, boost::regex::no_mod_s | boost::regex::mod_x);
	boost::sregex_iterator sr_iterator(man_page.begin(), man_page.end(), expr);

	for (auto mr = sr_iterator; mr != boost::sregex_iterator(); mr++) {
		commands.insert(mr->str());
	}
}

set<string, iless> get_commands(string man_page) {
	set<string, iless> commands {};
	// cout << "mode: " << int(mode) << endl;

	string r{};
	if (mode == Mode::Short) {
		r = R"(
			(?:[[:space:]]{3,})\K		# Has to start with 3 or more spaces
			-[[:alnum:]]+				# Short form starts with - followed my alnum, e.g. -a
			(?:-\w+)*					# Commands can have - in the middle followed by alnum, e.g. -names-only
			(?=,|[[:space:]]{3,})		# Has to end with a , or 3 or more spaces
		)";
	}
	else if (mode == Mode::Long) {
		r = R"(
			(?:[[:space:]]{3,})\K
			--[[:alnum:]]+(=\w+)?		# Long form starts with -- followed by alnum, e.g. --all
										# Long form can have an equal sign (=) followed by an option argument, e.g.
										# --protocol=family
			(?:-\w+)*
			(?=,|[[:space:]]{3,})
		)";
	}
	else if (mode == Mode::Mixed) {
		r = R"(
			(?:[[:space:]]{3,})\K
			(((-[[:alnum:]]+),[[:space:]](--[[:alnum:]]+(=\w+)?))| 	# Short form followed by long form, e.g. -a, --all
			((--[[:alnum:]]+(=\w+)?),[[:space:]](-[[:alnum:]]+))|	# Long form followed by short form, e.g. --all, -a
			(-[[:alnum:]]+)|										# Short form
			(--[[:alnum:]]+(=\w+)?))								# Long form
			(?:-\w+)*
			(?=,|[[:space:]]{3,})
		)";
	}
	add_commands(commands, man_page, r);

	return commands;
}

void print_commands(set<string, iless> commands, string delim) {
	for (auto i = commands.begin(); i != commands.end(); i++) {
		if (std::next(i) == commands.end()) delim = "";
		if (mode == Mode::Mixed) {
			string output {};
			bool is_mixed = false;
			for (auto c : *i) {
				if (c != ',') { 
					output.append(std::string{c});
				}
				else { 
					output.append(" ["); 
					is_mixed = true;
				}
			}
			if (is_mixed) output.append(" ]"); 
			cout << output << delim;
		}
		else cout << *i << delim;
	}
	cout << endl;
}

int main(int ac, char ** av) {
	auto spo = set_program_options(ac, av);
	if (spo == 1) return 1;

	if (ac < 2) {
		cout << desc << endl;
		return 1;
	}

	string man_page {};
	try {
		man_page = load_man_page(ac, av);
		set<string, iless> commands {get_commands(man_page)};
		string delim = set_delimiter();
		print_commands(commands, delim);

		return 0;
	} catch (const string& e) {
		cout << e << endl;
		return 1;
	}
}

