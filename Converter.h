#pragma once

#include <mruby.h>

namespace MrbWrap {

	//! Utility method for more descriptive code
	template <class T> inline static T* cast_ptr(void* object_ptr) {

		return static_cast<T*>(object_ptr);

	}

	//! Utility method for more descriptive code
	template <class T> inline static void call_destructor(void* object_ptr) {

		delete cast_ptr<T>(object_ptr);

	}

	//! Universal destructor wrapped into a C representation
	//! Will be used as callback for ruby object deallocation
	template <class T> static void free_data(mrb_state* mrb, void* object_ptr) {

		call_destructor<T>(object_ptr);

	}

	//! Creates a direct ruby wrapper for any C++ object and stores it in 'self'
	//! Constructor arguments can be given as 'TArgs', if needed
	//! The name of the class T will be used as the argument for the ruby-internal name
	//! This may cause some weird names from certain compilers, but this shouldn't pose a problem
	//! DO NOT destroy the created object manually, the mruby garbage collector will do this for you!
	//! IMPORTANT: This works only with ruby classes specifically declared as data object
	//! Do this by using the function 'define_data_class' defined here
	//! Use this in callbacks
	template <class T, class ... TArgs> T* convert_to_object(mrb_state* mrb, mrb_value self, TArgs ... args) {

		auto new_object = new T(args...);

		static const mrb_data_type data_type = {

			typeid(T).name(), free_data<T>

		};

		DATA_PTR(self) = new_object;
		DATA_TYPE(self) = &data_type;

		return new_object;

	}

	template <class C> bool check_for_type(mrb_state* mrb, mrb_value obj) {

		auto expected_class = get_class_info_ptr<C>();

		return mrb_obj_is_kind_of(mrb, obj, expected_class);

	}

	template <class C> void verify_type(mrb_state* mrb, mrb_value obj) {

		if (!check_for_type<C>(mrb, obj)) {

			std::string message = std::string("Object of class ")
				+ std::string(mrb_class_name(mrb, mrb_obj_class(mrb, obj)))
				+ std::string(" is not of class ")
				+ std::string(mrb_class_name(mrb, get_class_info_ptr<C>()));

			mrb_raise(mrb, mrb->eException_class, message.c_str());

		}

	}

	//! Return the wrapped C++ object of class 'T' from 'self'
	//! This can be used to modify the internal properties of a wrapped C++ object
	//! Use this in callbacks
	template <class T> T* convert_from_object(mrb_state* mrb, mrb_value self) {

		verify_type<T>(mrb, self);

		auto type = DATA_TYPE(self);

		return static_cast<T*>(mrb_data_get_ptr(mrb, self, type));

	}

	//! Copy the content of the wrapped C++ structure to the new wrapper in 'self'
	//! Use this in callbacks
	template <class T> void copy_object(mrb_state* mrb, mrb_value self) {

		mrb_value original;

		mrb_get_args(mrb, "o", &original);

		auto old_value = convert_from_object<T>(mrb, original);
		auto new_value = convert_to_object<T>(mrb, self);

		*new_value = *old_value;

	}

	//! Default copy routine which can be used for trivial copy constructors
	//! Use this as a callback routine
	template <class T> mrb_value ruby_class_default_init_copy(mrb_state* mrb, mrb_value self) {

		copy_object<T>(mrb, self);

		return self;
	}

	//! Creates a C++ instance of the class 'T' and wraps it directly into the ruby instance variable of the ruby object 'self'
	//! Use this in callbacks
	template <class T, class ... TArgs> T* convert_to_instance_variable(mrb_state* mrb, mrb_value self, const char* var_c_str, TArgs ... args) {

		auto new_object = new T(args...);

		static const struct mrb_data_type data_type = {

			typeid(T).name(), free_data<T>

		};

		//! mrb_intern_lit does not work here, because string lengths are pretty weird in mruby
		//! Also, the symbols do not need to be computed more than once, so this variable is made static (also see below)
		static auto symbol = mrb_intern_static(mrb, var_c_str, strlen(var_c_str));
		auto wrapper = Data_Wrap_Struct(mrb, mrb->object_class, &data_type, new_object);

		mrb_iv_set(mrb, self, symbol, mrb_obj_value(wrapper));

		return new_object;

	}

	//! Obtains a pointer to the C++ object of class 'T' back from the instance variable with name 'var_c_str' from the ruby object 'self'
	//! Use this in callbacks
	template <class T> T* convert_from_instance_variable(mrb_state* mrb, mrb_value self, const char* var_c_str) {

		static auto symbol = mrb_intern_static(mrb, var_c_str, strlen(var_c_str));
		auto instance_var = mrb_iv_get(mrb, self, symbol);
		auto type = DATA_TYPE(instance_var);

		return static_cast<T*>(mrb_data_get_ptr(mrb, instance_var, type));

	}

}