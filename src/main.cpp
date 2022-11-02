#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>
#include "GAT/args.hpp"
#include "gpw.hpp"

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

static std::string generate_password(std::size_t size, std::string_view allowed = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~") noexcept {
	static std::mt19937 mt{std::random_device{}()};
	std::string pass;
	pass.reserve(size);
	std::uniform_int_distribution<std::size_t> dist{0, allowed.size() - 1};
	while(size--)
		pass += allowed[dist(mt)];
	return pass;
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
	-g, --generate=<size>   Specify size of randomly generated passwords
	-h, --help              Display this
	-v, --version           Display the version number
)";
		return 0;
	} else if(r.version) {
		std::cout << "GPW version 0.1.0\n";
		return 0;
	}
	auto size = r.size.data() ? to_size(r.size) : 64;
	char const * vault_path;
	switch(a.size()) {
	case 0:
		vault_path = DEFAULT_VAULT_LOCATION;
		std::cout << "Using vault \"" DEFAULT_VAULT_LOCATION "\"\n";
		break;
	case 1:
		vault_path = a[0].data();
		break;
	default:
		std::cerr << "gpw: fatal: vault not specified correctly (use -h for help)\n";
		return 1;
	}
	std::vector<std::pair<std::string, std::string>> vault;
	if(std::filesystem::exists(vault_path)) {
		std::ifstream file{vault_path, std::ios_base::binary};
		if(!file.is_open()) {
			std::cerr << "gpw: fatal: couldn't open \"" << vault_path << "\"\n";
			return 1;
		}
		vault.resize(gpw::read_size(file));
		for(auto & p : vault)
			p = gpw::read(file);
	}
	std::cout << "Password: " << std::flush;
	std::string password;
	std::getline(std::cin, password);
	for(std::string buffer, buffer2;;) {
		std::cout << ">>> " << std::flush;
		std::cin >> buffer;
		if(!std::cin.good()) {
			std::cout << '\n';
			break;
		}
		if(buffer == "help") {
			std::cout << R"(list            List all slots
size [<size>]   Request or set size of password generator
save            Save vault to file (for retrying after failure)
new <name>      Create a new slot with a random password
get <id>        Decrypt and display password
edit <id>       Change password for a slot
rename <id>     Rename a slot
delete <id>     Remove a slot

Identifiers:
	either name or index of a slot
)";
		} else if(buffer == "list") {
			for(std::size_t i{}; auto && [name, _] : vault)
				std::cout << ++i << ": " << name << '\n';
		} else if(buffer == "size") {
			std::getline(std::cin, buffer);
			std::string_view sv{buffer};
			if(auto pos = sv.find_first_not_of(' '); pos != sv.npos) {
				try {
					size = std::stoull(sv.data() + pos, nullptr, 10);
				} catch(std::exception const &) {
					std::cout << "Invalid size\n";
					continue;
				}
			}
			std::cout << "Size: " << size << '\n';
		} else {
			if(buffer != "save") {
				std::getline(std::cin, buffer2);
				auto pos = buffer2.find_first_not_of(' ');
				if(pos == buffer2.npos) {
					std::cout << "No identifier provided\n";
					continue;
				}
				std::string_view id{buffer2.data() + pos, buffer2.data() + buffer2.find_last_not_of(' ') + 1};
				if(buffer == "new") {
					auto pass = generate_password(size);
					std::cout << "New Password: " << pass << "\n";
					gpw::use_password({pass.data(), size}, password);
					vault.emplace_back(id, std::move(pass));
				} else {
					auto it = vault.begin(), end = vault.end();
					for(; it != end; ++it)
						if(it->first == id)
							goto found;
					try {
						auto idx = std::stoull(buffer2.data() + pos, nullptr, 10);
						if(idx > vault.size()) {
							std::cout << "Nothing at index " << idx << '\n';
							continue;
						}
						it = vault.begin() + (idx - 1);
					} catch(std::exception const &) {
						std::cout << "Unknown identifier\n";
						continue;
					}
				found:
					if(buffer == "get") {
						gpw::use_password({it->second.data(), it->second.size()}, password);
						std::cout << it->second << '\n';
						gpw::use_password({it->second.data(), it->second.size()}, password);
						continue;
					} else if(buffer == "edit") {
						std::cout << "New password: " << std::flush;
						std::getline(std::cin, buffer2);
						gpw::use_password({buffer2.data(), buffer2.size()}, password);
						it->second = std::move(buffer2);
					} else if(buffer == "rename") {
						std::cout << "New name: " << std::flush;
						std::getline(std::cin, buffer);
						pos = buffer.find_first_not_of(' ');
						it->first = std::string{buffer, pos, buffer.find_last_not_of(' ') - pos + 1};
					} else if(buffer == "delete") {
						std::cout << "Deleting " << it->first << "\nSure? [y/N] " << std::flush;
						std::getline(std::cin, buffer);
						if(buffer == "y")
							vault.erase(it);
						else
							std::cout << "Aborted\n";
					} else {
						std::cout << "Unknown command\n";
						continue;
					}
				}
			}
			std::ofstream file{vault_path, std::ios_base::binary};
			if(!file.is_open()) {
				std::cout << "Saving to file \"" << vault_path << "\" failed :/\n";
				continue;
			}
			gpw::write_size(file, vault.size());
			for(auto && [name, value] : vault)
				gpw::write(file, name, value);
		}
	}
} catch(std::exception const & e) {
	std::cerr << "gpw: fatal: " << e.what() << '\n';
	return 1;
}
