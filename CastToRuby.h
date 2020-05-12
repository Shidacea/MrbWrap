#pragma once

#include <string>
#include <mruby.h>

#include "DefaultWrap.h"
#include "FileString.h"

namespace MrbWrap {

	//! Function to cast any value to an internal ruby value
	template <class T> inline mrb_value cast_value_to_ruby(mrb_state* mrb, T value) {

		if constexpr (std::is_same_v<T, mrb_int>) {

			return mrb_fixnum_value(value);

		}
		else if constexpr (std::is_same_v<T, mrb_float>) {

			return mrb_float_value(mrb, value);

		}
		else if constexpr (std::is_same_v<T, mrb_bool>) {

			return mrb_bool_value(value);

		}
		else if constexpr (std::is_same_v<T, char*>) {

			return mrb_str_new(mrb, value, strlen(value));

		}
		else if constexpr (std::is_same_v<T, mrb_value>) {

			return value;

		}
		else {

			//! TODO: Make conditional here
			//static_assert(std::false_type::value, "Invalid mruby type given");

		}

	}

	//! TODO: Return mrb_value

	//! Get matching ruby class, e.g. mrb_int for int
	template <class T, class Trait = void> struct CastToRuby { using type = void; };

	template <class T>
	struct CastToRuby<T, typename std::enable_if<std::is_base_of_v<BaseDefaultWrap, T>>::type> {

		using real_type = typename T::type;
		using type = typename CastToRuby<real_type>::type;

	};

	template <> struct CastToRuby<bool> { using type = mrb_bool; };
	template <> struct CastToRuby<std::string> { using type = char*; };
	template <> struct CastToRuby<const char*> { using type = char*; };
	template <> struct CastToRuby<FileString> { using type = char*; };
	template <class T> struct CastToRuby<T, typename std::enable_if<std::is_floating_point_v<T>>::type> { using type = mrb_float; };
	template <class T> struct CastToRuby<T, typename std::enable_if<std::is_integral_v<T>>::type> { using type = mrb_int; };
	template <class T> struct CastToRuby<T, typename std::enable_if<std::conjunction_v<std::is_class<T>, std::negation<std::is_base_of<BaseDefaultWrap, T>>>>::type> { using type = mrb_value; };

	//! Get the return type of a function
	template <class F> struct GetReturnType { using type = void; };

	template <class C, class T, class ... TArgs> struct GetReturnType<T (C::*)(TArgs...)> {

		using type = T;

	};

	template <class C, class ... TArgs> struct GetReturnType<void (C::*)(TArgs...)> {

		using type = void;

	};

	//! Utility function to cast arguments into usable values
	template <class Dest, class C> auto arg_cast(C arg, mrb_state* mrb = nullptr) {

		if constexpr (std::is_same_v<Dest, C>) {

			return arg;

		} else if constexpr (std::conjunction_v<std::is_same<C, FileString>, std::is_same<Dest, char*>>) {

			return const_cast<char*>(arg.content.c_str());

		} else if constexpr (std::is_same_v<Dest, std::string>) {

			return std::string(arg);

		} else if constexpr (std::is_same_v<Dest, const char*>) {

			return const_cast<const char*>(arg);

		} else if constexpr(std::conjunction_v<std::is_same<Dest, char*>, std::is_same<C, const char*>>) {

			return const_cast<char*>(arg);

		} else if constexpr (std::is_same_v<Dest, char*>) {

			return const_cast<char*>(arg.c_str());

		} else if constexpr (std::is_same_v<Dest, FileString>) {

			//! TODO: Cache these maybe

			//! This is a really vile solution, but it pays out massively.

			auto script_module = mrb_module_get_under(mrb, mrb_module_get(mrb, "SDC"), "Script");
			auto path = mrb_mod_cv_get(mrb, script_module, mrb_intern_static(mrb, "@@_path", strlen("@@_path")));

			return std::string(mrb_string_cstr(mrb, path)) + std::string("/") + std::string(arg);

		} else if constexpr (std::is_same_v<Dest, mrb_value>) {

			return mrb_value();

		} else if constexpr (std::conjunction_v<std::is_same<C, mrb_value>, std::is_pointer<Dest>>) {

			return convert_from_object<std::remove_pointer_t<Dest>>(mrb, arg);

		} else if constexpr (std::is_same_v<C, mrb_value>) {

			return *convert_from_object<Dest>(mrb, arg);

		} else {

			return static_cast<Dest>(arg);

		}

	}

	//! Utility function to cast return values into mruby compatible values
	template <class Dest, class C> auto return_cast(C arg, mrb_state* mrb) {

		if constexpr (std::is_same_v<Dest, C>) {

			return arg;

		} else if constexpr (std::is_same_v<Dest, std::string>) {

			return std::string(arg);

		} else if constexpr (std::is_same_v<Dest, const char*>) {

			return const_cast<const char*>(arg);

		} else if constexpr (std::is_same_v<Dest, char*>) {

			return const_cast<char*>(arg.c_str());

		} else if constexpr (std::is_same_v<Dest, FileString>) {

			return std::string(arg);

		} else if constexpr (std::is_same_v<Dest, mrb_value>) {

			auto ruby_class = get_class_info_ptr<C>();
			auto new_mruby_obj = mrb_obj_new(mrb, ruby_class, 0, NULL);

			return new_mruby_obj;

		} else if constexpr (std::conjunction_v<std::is_same<C, mrb_value>, std::is_pointer<Dest>>) {

			return convert_from_object<std::remove_pointer_t<Dest>>(mrb, arg);

		} else if constexpr (std::is_same_v<C, mrb_value>) {

			return *convert_from_object<Dest>(mrb, arg);

		} else {

			return static_cast<Dest>(arg);

		}

	}

}