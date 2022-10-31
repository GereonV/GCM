#include <fstream>
#include <iostream>
#include <iterator>
#include "GAT/args.hpp"

#ifdef _WIN32
#define DEFAULT_VAULT_LOCATION "%USERPROFILE%\\.gpwvault"
#else
#define DEFAULT_VAULT_LOCATION "~/.gpwvault"
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

int main(int argc, char ** argv) {
	struct result {
		bool help{};
		bool version{};
	};
	auto [r, a] = gat::args::parse<result, gat::args::all, gat::args::options<
			gat::args::option<result, 'h', "help", &result::help>,
			gat::args::option<result, 'v', "version", &result::version>
		>{}, gat::args::options<
			gat::args::argoption<result, 0, "", nullptr>
		>{}>({argv + 1, static_cast<std::size_t>(argc - 1)});
	if(r.help) {
		std::cout << R"(Usage: gpw [<options>] [<item>[:<vault>]]

Options:
	-h, --help      Display this
	-v, --version   Display the version number
	-               Use stdin as vault-content
)";
		return 0;
	} else if(r.version) {
		std::cout << "GPW version 0.1.0\n";
		return 0;
	}
	auto [item, stdin] = source(a);
	if(!item.data()) {
		std::cerr << "gpw: fatal: item not specified correctly (use -h for help)\n";
		return 1;
	}
	std::string_view vault_path{DEFAULT_VAULT_LOCATION};
	if(auto sep = item.find(':'); sep != item.npos) {
		vault_path = item.substr(sep + 1);
		item = item.substr(0, sep);
	}
	std::vector<unsigned char> vault;
	if(stdin) {
		std::istream_iterator<unsigned char> begin{std::cin}, end;
		vault.assign(begin, end);
	} else {
		std::ifstream file{vault_path.data(), std::ios_base::binary};
		std::istream_iterator<unsigned char> begin{file}, end;
		vault.assign(begin, end);
	}
}
