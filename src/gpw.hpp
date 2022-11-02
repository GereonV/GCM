#ifndef GPW_HPP
#define GPW_HPP

#include <cstring>
#include <iosfwd>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gpw {

	template<std::size_t Extent = std::dynamic_extent>
	constexpr void use_password(std::span<char, Extent> data, std::string_view pass) noexcept {
		for(auto i = pass.size(); auto & c : data) {
			c ^= pass[--i];
			if(!i)
				i = pass.size();
		}
	}

	// TODO iterator constructed with password

	class vault {
	public:
		constexpr ~vault() { clear(); }
		constexpr bool empty() const noexcept { return _keys.empty(); }
		constexpr void clear() noexcept {
			_keys.clear();
			delete[] std::exchange(_data, nullptr);
		}

		template<typename S>
		constexpr void emplace_back(S && name, std::string_view data) noexcept {
			_keys.emplace_back(static_cast<S &&>(name));
			auto new_size = _size + data.size();
			if(new_size > _capacity) {
				auto new_data = new char[_capacity = new_size * 2];
				std::memcpy(new_data, _data, _size);
				delete[] std::exchange(_data, new_data);
			}
			std::memcpy(_data + _size, data.data(), data.size());
			_size = new_size;
		}

		friend std::istream & operator>>(std::istream & is, vault & v) {
			v.clear();
			is >> v._size >> v._capacity;
			for(v._keys.resize(v._size), is.ignore(); auto & key : v._keys)
				std::getline(is, key);
			is.read(v._data = new char[v._size = v._capacity], v._capacity);
			return is;
		}

		friend std::ostream & operator<<(std::ostream & os, vault const & v) {
			os << v._keys.size() << ' ' << v._size << '\n';
			for(auto && key : v._keys)
				os << key << '\n';
			os.write(v._data, v._size);
			return os;
		}

	private:
		std::vector<std::string> _keys;
		char * _data{};
		std::size_t _size{}, _capacity{};
	};

}

#endif // GPW_HPP
