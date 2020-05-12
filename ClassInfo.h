#pragma once

#include <mruby.h>
#include <memory>

namespace MrbWrap {

	struct ClassInfoStruct {

		RClass* class_ptr = nullptr;
		mrb_sym symbol = 0;
		const char* name = "";

	};

	template <class T> ClassInfoStruct ClassInfo;

	template <class T> auto get_class_info_symbol() {

		return ClassInfo<T>.symbol;

	}

	template <class T> auto get_class_info_ptr() {

		return ClassInfo<T>.class_ptr;

	}

	template <class T> auto get_class_info_name() {

		return ClassInfo<T>.name;

	}

}