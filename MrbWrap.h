#pragma once

#include <mruby.h>
#include <mruby/dump.h>
#include <mruby/variable.h>
#include <mruby/data.h>
#include <mruby/compile.h>
#include <mruby/class.h>
#include <mruby/array.h>
#include <mruby/string.h>
#include <cstring>
#include <string>
#include <functional>
#include <type_traits>
#include <tuple>
#include <utility>

#include "StaticString.h"
#include "DefaultWrap.h"
#include "CountArgs.h"
#include "FormatString.h"
#include "ClassInfo.h"
#include "MemberSpec.h"
#include "FileString.h"
#include "CastToRuby.h"
#include "GetArgs.h"
#include "Converter.h"

//! Alias for the lamdba type used for mruby functions

#define MRUBY_FUNC [](mrb_state* mrb, mrb_value self) noexcept -> mrb_value

//! Short alias for optional value wrappers

#define MRBW_OPT MrbWrap::DefaultWrap
#define MRBW_RAT_OPT MrbWrap::RationalDefaultWrap
#define MRBW_FILE MrbWrap::FileString

namespace MrbWrap {

	//! Will be defined below
	template <class T> mrb_value ruby_class_default_init_copy(mrb_state* mrb, mrb_value self);

	//! Universal destructor wrapped into a C representation
	//! Will be used as callback for ruby object deallocation
	template <class T> static void free_data(mrb_state* mrb, void* object_ptr) {

		delete static_cast<T*>(object_ptr);

	}

	//! Create and returns a ruby class which can be used as a C++ class wrapper
	//! Use this when setting up a ruby class
	inline RClass* define_data_class(mrb_state* mrb, const char* name, RClass* super_class = nullptr) {

		if (!super_class) super_class = mrb->object_class;

		auto ruby_class = mrb_define_class(mrb, name, super_class);
		MRB_SET_INSTANCE_TT(ruby_class, MRB_TT_DATA);

		return ruby_class;

	}

	inline RClass* define_data_class_under(mrb_state* mrb, const char* name, RClass* ruby_module, RClass* super_class = nullptr) {

		if (!super_class) super_class = mrb->object_class;

		auto ruby_class = mrb_define_class_under(mrb, ruby_module, name, super_class);
		MRB_SET_INSTANCE_TT(ruby_class, MRB_TT_DATA);

		return ruby_class;

	}

	template <class T> void wrap_class(mrb_state* mrb, const char* name, RClass* super_class = nullptr) {

		ClassInfo<T>.class_ptr = define_data_class(mrb, name, super_class);
		ClassInfo<T>.symbol = mrb_intern_static(mrb, name, strlen(name));
		ClassInfo<T>.name = name;

	}

	template <class T> void wrap_class_under(mrb_state* mrb, const char* name, RClass* ruby_module, RClass* super_class = nullptr) {

		ClassInfo<T>.class_ptr = define_data_class_under(mrb, name, ruby_module, super_class);
		ClassInfo<T>.symbol = mrb_intern_static(mrb, name, strlen(name));	//! TODO: Test this
		ClassInfo<T>.name = name;

	}

	//! Define a new function for ruby
	inline void define_member_function(mrb_state* mrb, RClass* ruby_class, const char* name, mrb_value(*func)(mrb_state* mrb, mrb_value self) noexcept, mrb_aspec aspec = MRB_ARGS_NONE()) {

		mrb_define_method(mrb, ruby_class, name, func, aspec);

	}

	inline void define_module_function(mrb_state* mrb, RClass* ruby_module, const char* name, mrb_value(*func)(mrb_state* mrb, mrb_value self) noexcept, mrb_aspec aspec = MRB_ARGS_NONE()) {

		mrb_define_module_function(mrb, ruby_module, name, func, aspec);

	}

	//! Define a copy method automatically for any wrapped C++ object
	//! Use this when setting up a ruby class
	template <class T> void define_default_copy_init(mrb_state* mrb) {

		auto ruby_class = get_class_info_ptr<T>();

		mrb_define_method(mrb, ruby_class, "initialize_copy", ruby_class_default_init_copy<T>, MRB_ARGS_REQ(1));

	}

	//! Now following are function wrappers which should cover about 75%-100% of all needed cases.
	//! Definitely supported are the following argument and return types:
	//!
	//! - int (with default value 0)
	//! - float (with default values 0, except inf and nan, as long as you use the rational wrapper)
	//! - char (with default value 0)
	//! - char* (no default value)
	//! - std::string (no default value)
	//! - bool (with default value false)
	//! - Many classes (no default value)
	//! - Pointers to classes (WARNING: NOT TESTED YET)
	//!
	//! If you want to use any other type (or reference), this will probably not work yet.
	//! Please contact me (Hadeweka) or make a pull request if you find something which doesn't work.
	//! The list may expand in the future if needed.
	//! Remember that this is also still in a somewhat experimental stage.
	//! If in doubt, just write your own wrapper using define_mruby_function to avoid any problems.

	//! TODO: Wrap static functions

	//! Wrap a constructor with arbitrary arguments
	template <class C, class ... TArgs> void wrap_constructor(mrb_state* mrb) {

		auto ruby_class = get_class_info_ptr<C>();

		define_member_function(mrb, ruby_class, "initialize", MRUBY_FUNC {

			auto args = get_converted_args<TArgs...>(mrb);

			//! Generate the desired object with the called arguments
			std::apply([&mrb, self](auto& ...arg) {

				convert_to_object<C>(mrb, self, arg...);

			}, args);

			return self;

		//! TODO: Maybe change this at some point if it should become relevant
		}, ArgType<TArgs...>::value);

	}

	template <class C, auto Member, class ... TArgs> void wrap_member_function(mrb_state* mrb, const char* name) {

		using Spec = MemberSpec<Member>;

		auto ruby_class = get_class_info_ptr<C>();

		define_member_function(mrb, ruby_class, name, MRUBY_FUNC {

			auto args = get_converted_args<TArgs...>(mrb);

			auto content = convert_from_object<C>(mrb, self);

			if constexpr(std::is_void_v<typename GetReturnType<decltype(Member)>::type>) {

				std::apply([&content](auto& ...arg) {

					((*content).*Member)(arg...);

				}, args);

				return mrb_nil_value();

			} else {

				typename GetReturnType<decltype(Member)>::type return_value = std::apply([&content](auto& ...arg) {

					return ((*content).*Member)(arg...);

				}, args);

				using ret_type = decltype(return_value);
				using casted_ret_type = typename CastToRuby<ret_type>::type;

				auto casted_return_value = return_cast<casted_ret_type>(return_value, mrb);

				if constexpr (std::is_same_v<casted_ret_type, mrb_value>) {

					auto new_value = convert_from_object<ret_type>(mrb, casted_return_value);
					*new_value = return_value;

				}

				return cast_value_to_ruby(mrb, casted_return_value);

			}

		}, ArgType<TArgs...>::value);

	}

	//! Wrap a getter using a member function or object
	template <class C, auto Member> void wrap_getter(mrb_state* mrb, const char* name) {

		auto ruby_class = get_class_info_ptr<C>();

		define_member_function(mrb, ruby_class, name, MRUBY_FUNC {

			auto content = convert_from_object<C>(mrb, self);

			if constexpr (std::is_member_object_pointer_v<decltype(Member)>) {

				auto return_value = ((*content).*Member);

				using ret_type = decltype(return_value);
				using casted_ret_type = typename CastToRuby<ret_type>::type;
				
				auto casted_return_value = return_cast<casted_ret_type>(return_value, mrb);

				if constexpr (std::is_same_v<casted_ret_type, mrb_value>) {

					auto new_value = convert_from_object<ret_type>(mrb, casted_return_value);
					*new_value = return_value;

				}

				return cast_value_to_ruby(mrb, casted_return_value);

			} else if constexpr (std::is_member_function_pointer_v<decltype(Member)>) {

				auto return_value = ((*content).*Member)();

				using ret_type = decltype(return_value);
				using casted_ret_type = typename CastToRuby<ret_type>::type;
				
				auto casted_return_value = return_cast<casted_ret_type>(return_value, mrb);

				if constexpr (std::is_same_v<casted_ret_type, mrb_value>) {

					auto new_value = convert_from_object<ret_type>(mrb, casted_return_value);
					*new_value = return_value;

				}

				return cast_value_to_ruby(mrb, casted_return_value);

			} else {

				//! TODO: Maybe an error here?
				return mrb_nil_value();

			}

		}, MRB_ARGS_NONE());

	}

	//! Wrap a setter using a member function or object
	template <class C, auto Member, class ArgType> void wrap_setter(mrb_state* mrb, const char* name) {

		auto ruby_class = get_class_info_ptr<C>();

		define_member_function(mrb, ruby_class, name, MRUBY_FUNC {

			typename CastToRuby<ArgType>::type new_value;
			mrb_get_args(mrb, FormatString<ArgType>::value, &new_value);

			auto content = convert_from_object<C>(mrb, self);

			if constexpr (std::is_member_object_pointer_v<decltype(Member)>) {

				((*content).*Member) = arg_cast<ArgType>(new_value);

				return cast_value_to_ruby(mrb, new_value);

			} else if constexpr (std::is_member_function_pointer_v<decltype(Member)>) {

				((*content).*Member)(arg_cast<ArgType>(new_value));

				return cast_value_to_ruby(mrb, new_value);

			} else {

				//! TODO: Maybe an error here?
				return mrb_nil_value();

			}

		}, MRB_ARGS_REQ(1));

	}

}
