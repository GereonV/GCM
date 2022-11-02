#ifndef GPW_HPP
#define GPW_HPP

#include <iosfwd>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace gpw {

	template<std::size_t Extent = std::dynamic_extent>
	constexpr void use_password(std::span<char, Extent> data, std::string_view pass) noexcept {
		for(auto i = pass.size(); auto & c : data) {
			c ^= pass[--i];
			if(!i)
				i = pass.size();
		}
	}

	inline std::size_t read_size(std::istream & is) {
		std::size_t size;
		is >> size;
		is.ignore();
		return size;
	}

	inline void write_size(std::ostream & os, std::size_t size) {
		os << size << '\0';
	}

	inline std::pair<std::string, std::string> read(std::istream & is) {
		std::pair<std::string, std::string> res;
		std::getline(is, res.first, '\0');
		auto size = read_size(is);
		res.second.resize(size);
		is.read(res.second.data(), size);
		return res;
	}

	inline void write(std::ostream & os, std::string_view name, std::string_view value) {
		os << name << '\0';
		write_size(os, value.size());
		os.write(value.data(), value.size());
	}

}

#endif // GPW_HPP
