#pragma once

#include <type_traits>
#include <string>

#include "DefaultWrap.h"
#include "StaticString.h"
#include "FileString.h"

//! Templates to generate mruby argument format strings

namespace MrbWrap {

	//! Structures to generate the necessary format characters
	template <class T, class Trait = void> struct FormatChar;

	template <class T>
	struct FormatChar<T, typename std::enable_if<std::is_base_of_v<BaseDefaultWrap, T>>::type> {

		using real_type = typename T::type;
		constexpr static const char value = FormatChar<real_type>::value;

	};

	//! Format string specifications

	template <> struct FormatChar<bool> { constexpr static const char value = 'b'; };
	template <> struct FormatChar<const char*> { constexpr static const char value = 'z'; };
	template <> struct FormatChar<std::string> { constexpr static const char value = 'z'; };
	template <> struct FormatChar<FileString> { constexpr static const char value = 'z'; };
	template <class T> struct FormatChar<T, typename std::enable_if<std::is_floating_point_v<T>>::type> { constexpr static const char value = 'f'; };
	template <class T> struct FormatChar<T, typename std::enable_if<std::is_integral_v<T>>::type> { constexpr static const char value = 'i'; };
	template <class T> struct FormatChar<T, typename std::enable_if<std::conjunction_v<std::is_class<T>, std::negation<std::is_base_of<BaseDefaultWrap, T>>>>::type> { constexpr static const char value = 'o'; };

	//! Helper structure to generate format strings recursively
	template <bool IsDefault, class ... TArgs>
	struct FormatStringHelper {

		using type = StaticString<>;

	};

	template <bool IsDefault, class T, class ... TArgs>
	struct FormatStringHelper<IsDefault, T, TArgs...> {

		constexpr static auto separator = '|';

		using default_condition = std::is_base_of<BaseDefaultWrap, T>;
		constexpr static auto condition = std::conditional<IsDefault, std::false_type, default_condition>::type::value;

		using arg_1 = typename std::conditional<condition, StaticString<separator, FormatChar<T>::value>, StaticString<FormatChar<T>::value>>::type;
		using decider = typename std::conditional<condition, FormatStringHelper<true, TArgs...>, FormatStringHelper<IsDefault, TArgs...>>::type;

		using arg_2 = typename decider::type;
		using joined = JoinStaticStrings<arg_1, arg_2>;
		using type = typename joined::type;

	};

	//! The actual class to use for the format strings
	template <class ... TArgs> struct FormatString {

		constexpr static auto value = FormatStringHelper<false, TArgs...>::type::value;

	};

}