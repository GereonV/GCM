#include <fstream>
#include <iostream>
#include <iterator>
#include "GAT/args.hpp"

#ifdef _WIN32
#define DEFAULT_VAULT_LOCATION "%USERPROFILE%\\.gpwvault"
#else
#define DEFAULT_VAULT_LOCATION "$HOME/.gpwvault"
#endif

int main(int argc, char ** argv) try {
	struct result {
		bool help{};
		bool quiet{};
		bool version{};
	};
	auto [r, a] = gat::args::parse<result, gat::args::all, gat::args::options<
			gat::args::option<result, 'h', "help", &result::help>,
			gat::args::option<result, 'v', "version", &result::version>
		>{}, gat::args::options<
			gat::args::argoption<result, 0, "", nullptr>
		>{}>({argv + 1, static_cast<std::size_t>(argc - 1)});
	if(r.help) {
		std::cout << R"(Usage: gpw [<options>] [<vault>]

Options:
	-h, --help              Display this
	-v, --version           Display the version number
)";
		return 0;
	} else if(r.version) {
		std::cout << "GPW version 0.1.0\n";
		return 0;
	}
	char const * vault_path;
	switch(a.size()) {
	case 0:
		vault_path = DEFAULT_VAULT_LOCATION;
		std::cout << "Using vault \"" << vault_path << "\"\n";
		break;
	case 1:
		vault_path = a[0].data();
		break;
	default:
		std::cerr << "gpw: fatal: vault not specified correctly (use -h for help)\n";
		return 1;
	}
	std::vector<unsigned char> vault;
	if(std::ifstream file{vault_path, std::ios_base::binary}; file.is_open()) {
		std::istream_iterator<unsigned char> begin{file}, end;
		vault.assign(begin, end);
	} else {
		std::cerr << "gpw: fatal: couldn't open \"" << vault_path << "\"\n";
		return 1;
	}
	// TODO saved authentification
	std::cout << "Password: " << std::flush;
	std::string password;
	std::getline(std::cin, password);
	// TODO authentificate and decrypt
} catch(std::exception const & e) {
	std::cerr << "gpw: fatal: " << e.what() << '\n';
	return 1;
}

