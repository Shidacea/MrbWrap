#pragma once

//! Serves as a compiletime string class

namespace MrbWrap {

	//! Base class for an array of chars, serving as a string
	template <char ... Args> struct StaticString {

		constexpr static const char value[] = { Args..., 0 };

	};

	//! Class for joining two string arrays
	template <class Arg1, class Args2> struct JoinStaticStrings;

	template <char ... Args1, char ... Args2>
	struct JoinStaticStrings<StaticString<Args1...>, StaticString<Args2...>> {

		using type = StaticString<Args1..., Args2...>;

	};

}