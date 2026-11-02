#include <iostream>
#include <ios>
#include <set>
#include <string>
#include <variant>
#include <tuple>
#include <unistd.h>
#include <stdio.h>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/regex.hpp>

using namespace std;
namespace po = boost::program_options;

static po::options_description desc("Usage: flag [flags] arguments\nAllowed flags", 80);
static po::options_description hidden("Hidden options");
static po::options_description all;
static po::variables_map vm;

enum class Mode { Short, Long, Mixed};
static const std::unordered_map<std::string, Mode> kModes {
	{"short", Mode::Short},
	{"long",  Mode::Long},
	{"mixed",  Mode::Mixed}
};
static Mode mode;

static bool is_sort = false;
static bool is_reverse = false;
static bool is_debug = false;
static string version = "1.0.0";

int set_program_options(int end, char ** av) {
	desc.add_options()
		("help,h", po::bool_switch()->default_value(false), "Show this help message")

		// ("short,s", po::bool_switch(), "Show short flag notation, e.g. -h")
		("long,l", po::bool_switch()->default_value(false), "Show long flag notation, e.g. [--help] (default notation is short form, e.g. [-h])")

		("mixed,m", po::bool_switch()->default_value(true), "Show mixed flag notation, e.g. [-h|--help]")

		("full,f", po::bool_switch()->default_value(false), "For each flag show one explanatory line from the man page") // TO DO

		("column,c", po::bool_switch()->default_value(false), "List entries by columns") // TO DO

		("delimiter,d", po::value<string>()->value_name("DELIM")->implicit_value("'\\n'")->default_value("\n", ""), "Specify DELIM used for separating flags; the characters '\\n' and '\\t' retain special meaning, i.e. new line and tab character respectively")

		("sort,s", po::bool_switch()->default_value(false), "Sort the output lexicographically (by default it is sorted by the order of appearance in the man page)")
		("title,t", po::bool_switch()->default_value(false), "Show the man page NAME section at the start of the output") // TO DO
		("word-count,w", po::bool_switch()->default_value(false), "Show the printed flag count at the bottom of the output") // TO DO
		("reverse,r", po::bool_switch()->default_value(false), "Reverse the sorting of the output")

		("color", po::value<string>()->value_name("WHEN")->implicit_value("never")->default_value("never", ""), "Use markers to highlight the first letter of the flag; WHEN is 'always', 'never', or 'auto'")

		("debug,D", po::bool_switch()->default_value(false), "Show debug messages")
		("version,v", po::bool_switch()->default_value(false), "Display version information and exit")
	;

	hidden.add_options()
		("input", po::value<vector<string>>(), "input")
		;

	po::positional_options_description pos;
	pos.add("input", -1);
	all.add(desc).add(hidden);

	po::parsed_options parsed = po::command_line_parser(end, av)
		.options(all)
		.positional(pos)
		.run();

	po::store(parsed, vm);
	po::notify(vm); 

	if (vm["help"].as<bool>() or !vm.count("input")) {
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
	if (vm["debug"].as<bool>()) {
		is_debug = true;
	}

	mode = Mode::Mixed;
	for (const auto& opt : parsed.options) {
		auto it = kModes.find(opt.string_key);
		if (it != kModes.end())
			mode = it->second;
	}

	return 0;
}

struct ilexic {
	// Strip the dashes at the start to start comparing with alnum
	static string strip_start(const string& s) {
		auto it = find_if(s.begin(), s.end(), [](unsigned char c) { return isalnum(c); });
		return string(it, s.end());
	}

	static string lower(string s) {
		for (auto & c : s) c = tolower(static_cast<unsigned char>(c));
		return s;
	}

	bool operator()(const string& a, const string& b) const {
		string x = strip_start(a);
		string y = strip_start(b);
		if (is_reverse == true) return make_tuple(lower(y), y, b) < make_tuple(lower(x), x, a);
		else return make_tuple(lower(x), x, a) < make_tuple(lower(y), y, b);
	}
};

string load_man_page(int ac, char ** av) {
	auto input = boost::algorithm::join(vm["input"].as<vector<string>>(), " ");
	string command{ "man "s + input + " 2>/dev/null | "s + "cat"s};
	
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
	string delim {vm["delimiter"].as<string>()};
	delim = vm["delimiter"].as<string>();
	if (delim == "\\n") delim = "\n";
	else if (delim == "\\t") delim = "\t";

	return delim;
}

// DEBUGGING
void print_debug() {
	cout << "DEBUG:" << endl;
	auto m = int(mode) == 0 ? "short"
		: int(mode) == 1 ? "long"
		: "mixed";
	cout << "Mode: " << int(mode) << " (" << m << ")" << endl;
	cout << "Delimiter: " << "'" << vm["delimiter"].as<string>() << "'" << endl;
	cout << "Sort: " << boolalpha << is_sort << endl;
	cout << "Reverse: " << is_reverse << endl;
	cout << "Debug: " << is_debug << endl;
	cout << "Version: " << version << endl;
	cout << endl;
}

void print_sub_matches_debug(boost::smatch sm) {
	for (std::size_t i = 0; i < sm.size(); i++) {
		if (sm[i].matched) {
			cout << "s[" << i << "]: " << sm[i] << endl;
		}
	}
	cout << endl;
}

template <typename A>
void add_commands(A & commands, string man_page, string regx) {
	const boost::regex expr(regx, boost::regex::no_mod_s | boost::regex::mod_x);
	boost::sregex_iterator sr_iterator(man_page.begin(), man_page.end(), expr);

	for (auto mr = sr_iterator; mr != boost::sregex_iterator(); mr++) {
		string command {};
		auto sm = *mr;

		if (is_debug) print_sub_matches_debug(sm); // DEBUG

		if (mode == Mode::Short) { 
			if (sm[2].matched) {
				command = sm[2] + sm[3];
			}
			else if (sm[4].matched) {
				command = sm[4] + sm[5];
			}
			else continue;
		}
		else if (mode == Mode::Long) {
			if (sm[2].matched) {
				command = sm[2] + sm[3] + sm[4];
			}
			else if (sm[5].matched) {
				command = sm[5] + sm[6] + sm[7];
			}
			else continue;
		} else {
			if (sm[2].matched) {
				command = sm[3] + "," + sm[6];
			}
			else if (sm[10].matched) {
				command = sm[15] + "," + sm[11];
			}
			else if (sm[18].matched) {
				command = sm[18];
			}
			else if (sm[21].matched) {
				command = sm[21];
			}
			else continue;
		}

		if (command.size() == 0) cout << "EMPTY: " << sm[0] << endl;
		command = "[" + command + "]";
		// cout << "command: " << command << endl;

		bool found = find(commands.begin(), commands.end(), command) != commands.end();
		if (!found) {
			if (is_reverse == false) commands.insert(commands.end(), command);
			else commands.insert(commands.begin(), command);
		}
	}
}

template <typename A>
void get_commands(A & commands, string man_page) {
	string regx{};
	if (mode == Mode::Short) {
		regx = R"(
			(?:^[\s]{3,6})\K				# Has to start with 3-6 \s

											# CASE 1: [--[l-f], -[s-f]]
			((?:(?:--\w+)					# (1) Long form starts with -- followed by alnum, e.g. --all
				(?:-\w+)*					# The flag name can contain a dash in the middle, e.g. 
											# --cert-type
				(?:=\w+)?					# Long form can have an equal sign (=) followed by an option argument, e.g.
											# --protocol=family
				(?:[^-\w\n]{1,2}))			# Separation
			(-\w+)(-\w+)*|					# (2, 3)
											# --|--
											# CASE 2: [-[s-f], --[l-f]] and CASE 3: -[s-f]
			(-\w+)(-\w+)*)					# (4, 5) Short form starts with - followed my alnum, e.g. -a
											# ---
			(?:$|[^-\w\n]{3,})				# END
		)";
	}
	else if (mode == Mode::Long) {
		regx = R"(
			(?:^[\s]{3,6})\K

											# CASE 1: [-[s-f], --[l-f]]
			((?:(?:-\w+)					# (1)
				(?:-\w+)*
				(?:[^-\w\n]{1,2}))
			(--\w+)(-\w+)*(=\w+)?|			# (2, 3, 4)
											# --|--
											# CASE 2: [--[l-f], -[s-f]] and CASE 3: --[l-f]
			(--\w+)(-\w+)*(=\w+)?)			# (5, 6, 7)

			(?:$|[^-\w\n]{3,})				# END
		)";
	}
	else if (mode == Mode::Mixed) {
		regx = R"(
			(?:^[\s]{3,6})\K					# START

			(									# (1)
				# CASE 1: [-[s-f], --[l-f]]
				(((-\w+)						# (2, 3, 4)
					(-\w+)*)					# (5)
					(?:[^-\w\n]{1,2})
				((--\w+)						# (6, 7)
					(-\w+)*						# (8)
					(=\w+)?))|					# (9)

				# CASE 2: [--[l-f], -[s-f]]
				(((--\w+)						# (10, 11, 12)
					(-\w+)*						# (13)
					(=\w+)?)					# (14)
					(?:[^-\w\n]{1,2})
				((-\w+)							# (15, 16)
					(-\w+)*))|					# (17)

				# CASE 3: only -[s-f]
				((-\w+)(-\w+)*)|				# (18, 19, 20)

				# CASE 4: only --[l-f]
				((--\w+)(-\w+)*(=\w+)?)			# (21, 22, 23, 24)
			)									

			(?:$|[^-\w\n]{3,})					# END
		)";
	}

	add_commands(commands, man_page, regx);
}

template <typename A>
void print_commands(A commands, string delim) {
	// cout << "delim = " << delim;
	for (auto iter = commands.begin(); iter != commands.end(); iter++) {
		if (std::next(iter) == commands.end()) delim = "";

		string output {};
		if (mode == Mode::Mixed) {
			int pipe_pos = 0;
			int i = 0;
			for (auto c : *iter) {
				if (c != ',') {
					output.append(std::string{c});
					i++;
				}
				else {
					output.append("|");
					pipe_pos = i;
				}
			}

			auto when = vm["color"].as<string>();
			if ( when == "always" or (when == "auto" and isatty(STDOUT_FILENO))) {
				if (pipe_pos != 0) {
					output.insert(pipe_pos + 1, "\033[32m");
					output.insert(pipe_pos, "\033[33m");
				}
				output.insert(output.length() - 1, "\033[33m");
				output.insert(1, "\033[31m");
				output.insert(0, "\033[33m");
				output.insert(0, "\033[0m");
				output += "\033[0m";
			}
		}
		else {
			output = *iter;
		}
		cout << output << delim;
	}
	cout << endl;
}

int get_command_end_idx(int ac, char ** av) {
	int n = ac - 1;
	while (n > 0 && av[n][0] == '-') n--;
	return n > 0 ? n + 1 : ac;
}

int main(int ac, char ** av) {
	auto end = get_command_end_idx(ac, av);
	auto spo = set_program_options(end, av);
	if (spo == 1) return 1;

	string man_page {};
	try {
		if (is_debug) print_debug();

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

