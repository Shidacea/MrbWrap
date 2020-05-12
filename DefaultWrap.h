#pragma once

#include <type_traits>
#include <string>

//! Basic value wrappers

namespace MrbWrap {

	//! Base wrapper class, only used as a base class to identify other wrappers
	struct BaseDefaultWrap { using type = void; };

	//! Default wrapper class for basic data types which can be used as template arguments
	template <class T, T DefaultValue = 0>
	struct DefaultWrap : public BaseDefaultWrap {

		using type = T;
		constexpr static auto value = DefaultValue;

	};

	//! Special wrapper class for float and double values, utilizing a rational number
	template <class T, int Nom = 0, int Denom = 1>
	struct RationalDefaultWrap : public BaseDefaultWrap {

		using type = T;
		constexpr static auto value = static_cast<T>(Nom) / static_cast<T>(Denom);

	};

	//! If a class is a wrapper, return the real type
	template <class T, class Trait = void> struct GetRealType { using type = T; };

	template <class T>
	struct GetRealType<T, typename std::enable_if<std::is_base_of_v<BaseDefaultWrap, T>>::type> {

		using real_type = typename T::type;
		using type = typename GetRealType<real_type>::type;

	};

	template <> struct GetRealType<std::string> { using type = std::string; };

	//! TODO: Do not generate a value each time calling this
	//! Get default value of wrapper class, else just return C++ default
	template <class T> constexpr auto get_default(T arg) {

		if constexpr (std::is_base_of<BaseDefaultWrap, T>::value) {

			return T::value;

		}
		else {

			auto ret_value = T();
			return ret_value;

		}

	};

}