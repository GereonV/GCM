#ifndef GAT_ARGS_HPP
#define GAT_ARGS_HPP

// See https://www.gnu.org/software/libc/manual/html_node/Argument-Syntax.html

#include <span>
#include <stdexcept>
#include <vector>
#include "gat.hpp"

namespace gat::args {

	template<typename ReturnT, typename MemberT, char c, literal str, MemberT ReturnT::* ptr>
	struct basic_option {
		static constexpr auto as_short = c;
		static constexpr std::string_view as_long{str};
		static constexpr auto member = ptr;
	};

	template<typename ReturnT, char c, literal str, bool ReturnT::* ptr>
	using option = basic_option<ReturnT, bool, c, str, ptr>;

	template<typename ReturnT, char c, literal str, std::string_view ReturnT::* ptr>
	using argoption = basic_option<ReturnT, std::string_view, c, str, ptr>;

	template<typename... Options>
	class options {
	public:
		constexpr auto operator()(char) const noexcept { return nullptr; }
		constexpr auto operator()(std::string_view) const noexcept { return nullptr; }
	};

	template<typename Option, typename... Options>
	class options<Option, Options...> {
	public:
		constexpr decltype(Option::member) operator()(char c) const noexcept {
			if(Option::as_short == c)
				return Option::member;
			return options<Options...>{}(c);
		}

		constexpr auto operator()(std::string_view sv) const {
			auto ptr = Option::member;
			if(!Option::as_long.starts_with(sv))
				ptr = nullptr;
			if constexpr(!sizeof...(Options))
				return ptr;
			if(((	!Options::as_long.starts_with(sv) ||
				(!ptr && (ptr = Options::member, true))
			 ) && ...))
				return ptr;
			throw std::invalid_argument{std::string{"Ambiguos abbreviation: \""}.append(sv) + '"'};
		}
	};

	template<typename ReturnT>
	class all {
	public:
	using type = std::pair<ReturnT, std::vector<std::string_view>>;
		constexpr void add_arg(std::string_view sv) noexcept { args.emplace_back(sv); }
		constexpr void set_opt(bool ReturnT::* ptr) noexcept { options.*ptr = true; }
		constexpr void set_opt(std::string_view ReturnT::* ptr, std::string_view sv) noexcept { options.*ptr = sv; }
		constexpr type result() noexcept { return {options, std::move(args)}; }
		std::vector<std::string_view> args;
		ReturnT options;
	};

	template<typename ReturnT>
	class opts {
	public:
	using type = ReturnT;
		constexpr void add_arg(std::string_view) noexcept {}
		constexpr void set_opt(bool ReturnT::* ptr) noexcept { options.*ptr = true; }
		constexpr void set_opt(std::string_view ReturnT::* ptr, std::string_view sv) noexcept { options.*ptr = sv; }
		constexpr type result() noexcept { return options; }
		ReturnT options;
	};

	template<typename ReturnT>
	class strict {
	public:
	using type = std::pair<ReturnT, std::vector<std::string_view>>;
		constexpr void add_arg(std::string_view sv) noexcept { args.emplace_back(sv); }
		constexpr void set_opt(bool ReturnT::* ptr) {
			if(_args)
				throw std::invalid_argument{"Option after non-option argument"};
			options.*ptr = true;
		}

		constexpr void set_opt(std::string_view ReturnT::* ptr, std::string_view sv) {
			if(options.*ptr.data())
				throw std::invalid_argument{std::string{"Already specified \""} + options.*ptr + "\" for this option"};
			options.*ptr = sv;
			_args = true;
		}

		constexpr type result() noexcept { return {options, std::move(args)}; }
	public:
		std::vector<std::string_view> args;
		ReturnT options;
	private:
		bool _args{};
	};

	template<typename ReturnT, template<typename> typename ReturnPolicy, auto options, auto argoptions, std::size_t Extent = std::dynamic_extent>
	[[nodiscard]] constexpr typename ReturnPolicy<ReturnT>::type parse(std::span<char const * const, Extent> input) {
		ReturnPolicy<ReturnT> res;
		for(auto it = input.begin(), end = input.end(); it != end; ++it) {
			std::string_view sv{*it};
			if(sv.size() == 1 || sv[0] != '-') {
				res.add_arg(sv);
				continue;
			}
			if(sv[1] == '-') {
				sv.remove_prefix(2);
				if(sv.empty()) {
					while(++it != end)
						res.add_arg(*it);
					break;
				}
				auto eq = sv.find('=');
				if(eq == sv.npos) {
					if(auto opt = options(sv); opt) {
						res.set_opt(opt);
						continue;
					}
				} else {
					auto value = sv.substr(eq + 1);
					sv = sv.substr(0, eq);
					if(auto opt = argoptions(sv); opt) {
						res.set_opt(opt, value);
						continue;
					}
				}
				throw std::invalid_argument{std::string{"Unknown long option: \""}.append(sv) + '"'};
			}
			for(std::size_t i{1}; i < sv.size(); ++i) {
				auto c = sv[i];
				if(auto opt = options(c); opt) {
					res.set_opt(opt);
					continue;
				}
				auto opt = argoptions(c);
				if(!opt)
					throw std::invalid_argument{std::string{"Unknown option: '"} + c + '\''};
				auto arg = [&]() {
					if(++i != sv.size())
						return sv.substr(i);
					if(++it != end)
						*it;
					throw std::invalid_argument{std::string{"Missing argument for option '"} + c + '\''};
				};
				res.set_opt(opt, arg());
				break;
			}
		}
		return res.result();
	}

}

#endif // GAT_ARGS_HPP
