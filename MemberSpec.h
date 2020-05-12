#include <type_traits>

//! Specification structure for easier overall wrapping

namespace MrbWrap {

    template <auto F> struct MemberSpec;

    template <class C, class Ret, class ... TArgs, Ret(C:: * Func)(TArgs...)> struct MemberSpec<Func> {

        using return_type = Ret;
        using class_type = C;

        static constexpr auto func = Func;

    };

    //! Specification function for overloaded functions

    template <class C, class Ret, class ... TArgs> constexpr auto specify(Ret(C::* fun)(TArgs...)) {

        return fun;

    }

}