#include <fstream>
#include <iostream>
#include <iterator>
#include "GAT/args.hpp"

#ifdef _WIN32
#define DEFAULT_VAULT_LOCATION "%USERPROFILE%\\.gpwvault"
#else
#define DEFAULT_VAULT_LOCATION "$HOME/.gpwvault"
#endif

constexpr std::pair<std::string_view, bool> source(std::span<std::string_view const> args) noexcept {
	switch(args.size()) {
	case 1:
		return {args[0], {}};
	case 2:
		if(args[0][0] == '-')
			return {args[1], true};
		else if(args[1][0] == '-')
			return {args[0], true};
		[[fallthrough]];
	case 0:
	default:
		return {{}, {}};
	}
}

int main(int argc, char ** argv) try {
	struct result {
		std::string_view auth;
		std::string_view create_path;
		bool create{};
		bool help{};
		bool list{};
		bool quiet{};
		bool version{};
	};
	auto [r, a] = gat::args::parse<result, gat::args::all, gat::args::options<
			gat::args::option<result, 'c', "create", &result::create>,
			gat::args::option<result, 'h', "help", &result::help>,
			gat::args::option<result, 'l', "list", &result::list>,
			gat::args::option<result, 'q', "quiet", &result::quiet>,
			gat::args::option<result, 'v', "version", &result::version>
		>{}, gat::args::options<
			gat::args::argoption<result, 'a', "auth", &result::auth>,
			gat::args::argoption<result, 0, "create", &result::create_path>
		>{}>({argv + 1, static_cast<std::size_t>(argc - 1)});
	if(r.help) {
		std::cout << R"(Usage: gpw [<options>] [<item>[:<vault>]]
           -l [<vault>]

Options:
	-a, --auth=<file>	Authentificate access via file and skip password prompt
	-c, --create[=<file>]   Create file for use with -a (defaults to auth.gpw)
	-h, --help              Display this
	-l, --list              List all items in a vault
	-q, --quiet             Don't be verbose
	-v, --version           Display the version number
	-                       Use stdin as vault-content (only with -a)
)";
		return 0;
	} else if(r.version) {
		std::cout << "GPW version 0.1.0\n";
		return 0;
	}
	auto [item, stdin] = source(a);
	std::string_view vault_path{DEFAULT_VAULT_LOCATION};
	bool vault_specified;
	if(r.list) {
		if((vault_specified = item.data()))
			vault_path = item;
	} else if(!item.data()) {
		std::cerr << "gpw: fatal: item not specified correctly (use -h for help)\n";
		return 1;
	} else if(auto sep = item.find(':'); (vault_specified = sep != item.npos)) {
		vault_path = item.substr(sep + 1);
		item = item.substr(0, sep);
	}
	std::vector<unsigned char> vault;
	if(stdin) {
		if(!r.auth.data()) {
			std::cerr << "gpw: fatal: can't use - without -a\n";
			return 1;
		}
		if(vault_specified)
			std::cerr << "gpw: warning: won't use specified vault \"" << vault_path << "\"\n";
		std::istream_iterator<unsigned char> begin{std::cin}, end;
		vault.assign(begin, end);
	} else {
		if(!r.quiet) {
			std::cout << "Using vault \"" << vault_path << '"';
			if(!vault_specified)
				std::cout << " (default)";
			std::cout << '\n';
		}
		std::ifstream file{vault_path.data(), std::ios_base::binary};
		if(!file.is_open()) {
			std::cerr << "gpw: fatal: couldn't open \"" << vault_path << "\"\n";
			return 1;
		}
		std::istream_iterator<unsigned char> begin{file}, end;
		vault.assign(begin, end);
	}
} catch(std::exception const & e) {
	std::cerr << "gpw: fatal: " << e.what() << '\n';
	return 1;
}
