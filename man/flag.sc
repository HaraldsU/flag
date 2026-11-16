flag(1) ["flag 1.0.0" ["User Commands"]]

; Conventional section names include NAME, SYNOPSIS,  CONFIGURATION,  
; DESCRIPTION, OPTIONS,  EXIT STATUS, RETURN VALUE, ERRORS, ENVIRONMENT, 
; FILES, VERSIONS, STANDARDS, NOTES, BUGS, EXAMPLE, AUTHORS, and SEE ALSO.

# NAME

flag - fast discovery of command flags from man pages

# SYNOPSIS

*flag* [_flags_] [_arguments_]

*flag* [_arguments_] [_flags_]

# DESCRIPTION

flag is a command-line tool for fast discovery of command flags from man pages. ++
flag relies on the man(1) and cat(1) commands for parts of it's functionality, ++
effectively executing "*man [*_arguments_*] 2>/dev/null | cat*" to get the man ++
page for the specified command and then matching it against a regular expression ++
(regex). ++
Because flag uses regex for it's functionality, there are some known ++
limitations; see *LIMITATIONS* below.

# OPTIONS

*\-\-color=*_WHEN_
	Use markers to highlight the output; _WHEN_ is *never*, *auto* or *always*.

	- The default. If *never* is used, the colors will never be used.

	- If *auto* is used, the colors will be used only if the output is sent to a ++
	tty.

	- If *always* is used, the colors will always be used regardless of where ++
	the output is sent.
	

*\-c, --column=*_N_
	List entries by columns; _N_ specifies the column count. ++
	If the *--column=*_N_ flag is set, the *--delimiter=*_DEM_ value is ignored. ++
	If too many columns are specified or if the flag output is too long to fit ++
	into the terminal width, the output will overflow (adding the *--no-argument* ++
	flag can be useful here).

*\-d, --delimiter=*_DEM_
	Specify delimiter _DEM_ used for separating flags; the characters '\\n' ++
	(the default) and '\\t' retain special meaning, i.e. the new line and tab ++
	characters respectively. ++
	If the *--column=*_N_ flag is set this flag is ignored.

*\-D, --debug*
	Show debug messages (this outputs a lot of information about the state and ++
	behavior of the program).

*\-e, --enclose=*_ENC_
	Specify _ENC_ used for enclosing (prefixing and suffixing) the ++
	output; 0, 1 or 2 characters are expected.

	- If 0 characters are given, e.g. *-e ""*, then the output is not enclosed ++
	(this is the default).

	- If 1 character is given, e.g. *-e "|"*, then it is used for both ++
	prefixing and suffixing the output.

	- If 2 characters are given, e.g. *-e "[]"*, then the first character is used ++
	for prefixing and the second character is used for suffixing the output.

	- If >=3 characters are given then the program prints an error message and ++
	exits with exit value 1.

*\-h, --help*
	Show help message. The help message is also displayed if the flag ++
	command is used with no flags or arguments.

*\-l, --long*
	Show long flag notation. This notation shows only flags starting ++
	with *--* and immediately followed by the flag name, e.g. *--help*.

*\-m, --mixed*
	Show mixed flag notation. This notation effectively shows the output of ++
	the *-s*, *-l* and *-M* flags combined, i.e. flags that have only the short ++
	or long notation are shown separately and flags that have both the short and ++
	long notations are shown together, hence the mixed notation gives the ++
	largest output, e.g. *-h|--help*. This is the default notation used.

*\-M, --mixed-only*
	Show only flags that have both the short and long notations. Unlike the *-m* ++
	flag, this flag doesn't show flags that have only the short or long ++
	notations, hence this flag shows the least amount of output.

*\-n, --no-argument*
	This flag specifies to not include the flag arguments in the output ++
	(this relates to both short and long flags), e.g. *--enclose* _ENC_ would be ++
	shown as *--enclose*. This can greatly reduce the length of the output and ++
	can be paired nicely with the *--column* _COL_ flag.

*\-p, --pretty*
	Alias for *'--color=*_always_ *--enclose=*_[]_'.

*\-r, --reverse*
	Reverse the order of the output.

*\-s, --short*
	Show short flag notation. This notation shows only flags starting ++
	with *-* and immediately followed by the flag name, e.g. *-h*.

*\-S, --sort*
	Sort the output lexicographically. By default the flags are sorted by the ++
	order of appearance in the man page. The sort is case agnostic, i.e. a ++
	lowercase 'z' will not come before an uppercase 'A', however with equal ++
	letters the lowercase version takes precedence, e.g. and 'a' will come ++
	before an 'A'.

*\-t, --title*
	Show the first line from the man page *NAME* section at the bottom of the ++
	output.

*\-v, --version*
	Display version information and exit.

*\-w, --flag-count*
	Show the flag count at the bottom of the output. This displays 4 different ++
	values where:

	- *SF* stands for Short Flags. If the *-m* flag is used this counts the ++
	flags that have only the short notation. If the *-M* flag is used this value ++
	is always 0.

	- *LF* stands for Long Flags. If the *-m* flag is used this counts the flags ++
	that have only the long notation. If the *-M* flag is used this value is ++
	always 0.

	- *MF* stands for Mixed Flags. If the *-m* or *-M* flags are used, this ++
	shows the count of flags that have both the short and long notations. ++
	If the *-s* or *-l* flags are used this values is always 0.

	- *TT* stands for total. It displays the total count of flags found ++
	(flags that have only the short or long notations and flags that have both ++
	short and long notations).

*--*
	Treats all subsequent arguments as file names, even if they start with a ++
	dash.

# EXIT STATUS

	The following exit values should be returned:

	- 0  if the program runs successfully.

	- >0 if an error occurs or an exceptions is thrown.

# LIMITATIONS

	Considering the fact that regex is used for getting the flags from the man ++
	pages and there isn't a rigorous standard that most of the man pages follow, ++
	there are certain limitation to the expected output.

	- Firstly, some man pages list a flag and it's aliases on the same line, ++
	e.g. *--quiet*, *--silent* in head(1), whilst others list different, ++
	similar flags on the same line, e.g. *-4*, *-6* in traceroute(8). ++
	This induces an ambiguity where it isn't clear whether the flag is an alias ++
	or a different, similar flag. In this case the flag command expects there to ++
	be at maximum one short and one long flag on the same line, hence any ++
	additional flags are not matched and don't show up in the output. ++
	This can also cause opposite versions of flags not to be matched if they are ++
	both defined on the same line, e.g. *--column[=*_<options>_*]*, *--no-column* ++
	in git-branch(1).

	- Secondly, the flag command follows the "POSIX.1-2017" utility ++
	conventions, which state that: "All options should be preceded by ++
	the '-' delimiter character". This results in the fact that some flags that ++
	don't start with the '-' delimiter are not added to the output, e.g. ++
	*+FIRST_PAGE[:LAST_PAGE]* from pr(1) is ignored.

	- Thirdly, there are some commands that implicitly assume that a flag has to ++
	be prefixed with some sort of a character, e.g. *-Si* in pacman(8). ++
	In these cases if the prefix is not stated explicitly for each of the flags ++
	the context is lost and the flags are added to the output without the ++
	prefix.

	- Fourthly, some man pages might contain text in other sections such as ++
	the *EXAMPLE* section which can't be explicitly classified as not being a ++
	flag, e.g. *-2* in the fzf(1) *FIELD INDEX EXPRESSION* section. ++
	These cases lead to false positives where text is matched as being a flag ++
	even though it isn't a flag.

	- Fifthly, the empty long flag *--*, which is often used as a delimiter ++
	indicating the end of options, is deliberately not matched.

# EXAMPLES
	*flag ls*
		Show the flags from ls(1).
	*flag git log*
		Show the flags from git-log(1).
	*flag -n ls*
		Show the flags from ls(1) with no arguments.
	*flag --color=always --column=2 --enclose=[] -M -n -S -r -t -w ls*
		Show the flags from ls(1) with colored output, printed in 2 ++
		columns, prefixed with '[' and suffixed with ']', showing only flags ++
		that have both the short and long notations, with no arguments, sorted ++
		in the reverse order with the title and flag count added to the bottom ++
		of the output.

# SEE ALSO

*Project homepage:*
	_https://github.com/HaraldsU/flag_

# VERSION
	flag 1.0.0

# AUTHOR

	Haralds Upitis (_upitisharalds@gmail.com_)

# LICENSE

MIT

