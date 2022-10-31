#ifndef _GAT_GAT_HPP_
#define _GAT_GAT_HPP_

#include <string_view>

namespace gat {

	template<std::size_t N>
	struct literal {
		consteval literal(char const (&str)[N]) noexcept {
			for(auto i = N; i--;)
				value[i] = str[i];
		}

		constexpr auto size() const noexcept { return N; }
		constexpr operator std::string_view() const noexcept { return {value, N - 1}; }
		constexpr std::string_view view() const noexcept { return *this; }

		char value[N];
	};

	template<typename T, typename... Tail>
	struct tuple {
		T value;
		tuple<Tail...> tail;
	};

	template<typename T>
	struct tuple<T> {
		T value;
	};

}

#endif // _GAT_GAT_HPP_
