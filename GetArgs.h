#pragma once

#include "CastToRuby.h"

namespace MrbWrap {

	//! Initialize and return a tuple of the ruby arguments
	template <class ... TArgs> constexpr auto generate_arg_tuple() {
		//! Generate a tuple of the final mruby types
		std::tuple<typename CastToRuby<TArgs>::type...> args;

		//! Set the tuple values to their default values if given
		std::apply([](auto&& ...arg) {

			//! Also ensure correct casting to the mruby type
			((arg = arg_cast<typename CastToRuby<TArgs>::type>(get_default(TArgs()))), ...);

		}, args);

		return args;
	}

	//! Obtain values from ruby function call
	template <class ... TArgs, class ... CastedTypes> inline void get_args_from_tuple(mrb_state* mrb, std::tuple<CastedTypes...>& args) {
		//! If any arguments, get them and store them into the tuple
		if constexpr (sizeof...(TArgs) > 0) {
			std::apply([&mrb](auto& ...arg) {

				//! The format string gets determined automatically from the template arguments
				mrb_get_args(mrb, FormatString<TArgs...>::value, &arg...);

			}, args);
		}

	}

	//! Get a tuple of the mruby values of the args
	template <class ... TArgs> constexpr auto get_raw_args(mrb_state* mrb) {

		auto args = generate_arg_tuple<TArgs...>();
		get_args_from_tuple<TArgs...>(mrb, args);

		return args;

	}

	//! Get a tuple of the C++ values of the args
	template <class ... TArgs> constexpr auto get_converted_args(mrb_state* mrb) {

		auto args = get_raw_args<TArgs...>(mrb);

		auto converted_args = std::apply([&mrb](auto&& ...arg) {

			return std::make_tuple(arg_cast<typename GetRealType<TArgs>::type>(arg, mrb)...);

		}, args);

		return converted_args;

	}

}