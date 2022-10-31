#include <iostream>
#include "GAT/args.hpp"

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
)";
		return 0;
	} else if(r.version) {
		std::cout << "GPW version 0.1.0\n";
		return 0;
	}
	std::cerr << "gpw: fatal: nothing to do (use -h for help)\n";
	return 1;
}
