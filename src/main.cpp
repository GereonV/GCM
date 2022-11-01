#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include "GAT/args.hpp"

#ifdef _WIN32
#define DEFAULT_VAULT_LOCATION "%USERPROFILE%\\.gpwvault"
#else
#define DEFAULT_VAULT_LOCATION "$HOME/.gpwvault"
#endif

constexpr std::size_t to_size(std::string_view sv) noexcept {
	std::size_t res{};
	for(auto c : sv)
		res = res * 10 + c - '0';
	return res;
}

static std::string generate_password(std::size_t size, std::string_view allowed = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~") noexcept {
	static std::mt19937 mt{std::random_device{}()};
	std::string pass;
	pass.reserve(size);
	std::uniform_int_distribution<std::size_t> dist{0, allowed.size() - 1};
	while(size--)
		pass += allowed[dist(mt)];
	return pass;
}

constexpr void use_password(std::span<char> data, std::string_view pass) noexcept {
	for(auto i = pass.size(); auto & c : data) {
		c ^= pass[--i];
		if(!i)
			i = pass.size();
	}
}

int main(int argc, char ** argv) try {
	struct result {
		std::string_view size;
		bool help{};
		bool version{};
	};
	auto [r, a] = gat::args::parse<result, gat::args::all, gat::args::options<
			gat::args::option<result, 'h', "help", &result::help>,
			gat::args::option<result, 'v', "version", &result::version>
		>{}, gat::args::options<
			gat::args::argoption<result, 'g', "generate", &result::size>
		>{}>({argv + 1, static_cast<std::size_t>(argc - 1)});
	if(r.help) {
		std::cout << R"(Usage: gpw [<options>] [<vault>]

Options:
	-g, --generate=<size>   Generate a random password (vault is ignored if specified)
	-h, --help              Display this
	-v, --version           Display the version number
)";
		return 0;
	} else if(r.version) {
		std::cout << "GPW version 0.1.0\n";
		return 0;
	} else if(r.size.data()) {
		std::cout << generate_password(to_size(r.size)) << '\n';
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
	std::string vault;
	if(std::filesystem::exists(vault_path)) {
		std::ifstream file{vault_path, std::ios_base::binary};
		if(!file.is_open()) {
			std::cerr << "gpw: fatal: couldn't open \"" << vault_path << "\"\n";
			return 1;
		}
		vault.assign<std::istreambuf_iterator<char>>(file, {});
		std::cout << "Password: " << std::flush;
		std::string password;
		std::getline(std::cin, password);
		use_password(vault, password);
	}
} catch(std::exception const & e) {
	std::cerr << "gpw: fatal: " << e.what() << '\n';
	return 1;
}
