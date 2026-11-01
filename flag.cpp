#include <iostream>
#include <set>
#include <string>
#include <variant>
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

static bool is_sort = false;
static bool is_reverse = false;
static string version = "1.0.0";

int set_program_options(int ac, char ** av) {
	desc.add_options()
		("help,h", po::bool_switch(), "Show this help message")
		// ("short,s", po::bool_switch(), "Show short flag notation, e.g. -h")
		("long,l", po::bool_switch(), "Show long flag notation, e.g. --help (default is short form, e.g. -h)")
		("mixed,m", po::bool_switch(), "Show mixed flag notation, e.g. [-h|--help]")
		("delimiter,d", po::value<string>()->value_name("DELIM")->implicit_value("; ", "\"; \""), "Specify the delimiter used for separating flags, e.g. -d \"\\n\" or --delimiter=\"\\n\". The default delimiter is \"; \". The characters \"\\n\" and \"\\t\" retain special meaning, i.e. the new line and tab characters appropriately")
		("sort,s", po::bool_switch(), "Sort the output lexicographically (by default it is sorted by the order of appearance in the man page)")
		("reverse,r", po::bool_switch(), "Reverse the sorting of the output")
		("version,v", po::bool_switch(), "Display version information and exit")
	;

	po::parsed_options parsed = po::parse_command_line(ac, av, desc);
	po::store(parsed, vm);
	po::notify(vm); 

	if (vm["help"].as<bool>() or ac < 2) {
		cout << desc << endl;
		return 1;
	}
	else if (vm["version"].as<bool>()) {
		cout << "flag " << version << endl;
		return 1;
	}

	if (vm["reverse"].as<bool>()) {
		is_reverse = true;
	}
	if (vm["sort"].as<bool>()) {
		is_sort = true;
	}

	for (const auto& opt : parsed.options) {
		auto it = kModes.find(opt.string_key);
		if (it != kModes.end())
			mode = it->second;
	}

	return 0;
}

struct ilexic {
	// Strip the fluff to compare only words and numbers
	static string alnum_only(const string& s) {
		string out;
		for (unsigned char c : s)
			if (std::isalnum(c)) out += c;

		return out;
	}

	bool operator()(const string& a, const string& b) const {
		if (is_reverse == true) return boost::algorithm::ilexicographical_compare(alnum_only(b), alnum_only(a));
		else return boost::algorithm::ilexicographical_compare(alnum_only(a), alnum_only(b));
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

template <typename A>
void add_commands(A & commands, string man_page, string regx) {
	const boost::regex expr(regx, boost::regex::no_mod_s | boost::regex::mod_x);
	boost::sregex_iterator sr_iterator(man_page.begin(), man_page.end(), expr);
	string command {};

	for (auto mr = sr_iterator; mr != boost::sregex_iterator(); mr++) {
		auto smatch = *mr;
		if (smatch[3].matched and smatch[4].matched) { 		// [short|long] format
			command = smatch[3]+ ", " + smatch[4];		
		}
		else if (smatch[6].matched and smatch[7].matched) {	// [long|short] -> [short|long] format
			command = smatch[7]+ ", " + smatch[6];
		}
		else {
			command = mr->str();
		}

		if (is_reverse == false) commands.insert(commands.end(), command);
		else commands.insert(commands.begin(), command);
	}
}

template <typename A>
void get_commands(A & commands, string man_page) {
	string regx{};
	if (mode == Mode::Short) {
		regx = R"(
			(?:[[:space:]]{3,})\K		# Has to start with 3 or more spaces
			-[[:alnum:]]+				# Short form starts with - followed my alnum, e.g. -a
			(?:-\w+)*					# Commands can have - in the middle followed by alnum, e.g. -names-only
			(?=,|[[:space:]]{3,})		# Has to end with a , or 3 or more spaces
		)";
	}
	else if (mode == Mode::Long) {
		regx = R"(
			(?:[[:space:]]{3,})\K
			--[[:alnum:]]+(=\w+)?		# Long form starts with -- followed by alnum, e.g. --all
										# Long form can have an equal sign (=) followed by an option argument, e.g.
										# --protocol=family
			(?:-\w+)*
			(?=,|[[:space:]]{3,})
		)";
	}
	else if (mode == Mode::Mixed) {
		regx = R"(
			(?:[[:space:]]{3,})\K
			(((-[[:alnum:]]+),[[:space:]](--[[:alnum:]]+(?:=\w+)?))| 	# (1, 2, 3, 4) Short form followed by long form, e.g. -a, --all
			((--[[:alnum:]]+(?:=\w+)?),[[:space:]](-[[:alnum:]]+))|		# (5, 6, 7) Long form followed by short form, e.g. --all, -a
			(-[[:alnum:]]+)|											# (8) Short form
			(--[[:alnum:]]+(=\w+)?))									# (9) Long form
			(?:-\w+)*
			(?=,|[[:space:]]{3,})
		)";
	}

	add_commands(commands, man_page, regx);
}

template <typename A>
void print_commands(A commands, string delim) {
	for (auto i = commands.begin(); i != commands.end(); i++) {
		if (std::next(i) == commands.end()) delim = "";
		if (mode == Mode::Mixed) {
			string output {};
			for (auto c : *i) {
				if (c != ',' and c != ' ') { 
					output.append(std::string{c});
				}
				else if (c != ' ') { 
					output.append("|"); 
				}
			}
			output = "[" + output;
			output.append("]"); 
			cout << output << delim;
		}
		else cout << *i << delim; // Short and long mode
	}
	cout << endl;
}

int main(int ac, char ** av) {
	auto spo = set_program_options(ac, av);
	if (spo == 1) return 1;

	string man_page {};
	try {
		man_page = load_man_page(ac, av);
		variant<set<string, ilexic>, vector<string>> commands;
		if (is_sort == true) commands = set<string, ilexic>{};
		else commands = vector<string>{};
		visit([&](auto & c) { get_commands(c, man_page); }, commands);

		string delim = set_delimiter();
		visit([&](auto & c) { print_commands(c, delim); }, commands);

		return 0;
	} catch (const string& e) {
		cout << e << endl;
		return 1;
	}
}

