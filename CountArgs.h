#include <type_traits>
#include <mruby.h>

#include "DefaultWrap.h"

namespace MrbWrap {

    template <class T, class ... TArgs> struct CountArgs {

        static constexpr std::size_t req = CountArgs<TArgs...>::req + CountArgs<T>::req;
        static constexpr std::size_t opt = CountArgs<TArgs...>::opt + CountArgs<T>::opt;

    };

    template <class T> struct CountArgs<T> {

        static constexpr std::size_t req = (std::is_base_of_v<BaseDefaultWrap, T> ? 0 : 1);
        static constexpr std::size_t opt = (std::is_base_of_v<BaseDefaultWrap, T> ? 1 : 0);

    };

    struct ZeroArgs {

        static constexpr std::size_t req = 0;
        static constexpr std::size_t opt = 0;

    };

    template <class ... TArgs> struct ArgTypeNonZero {

        using count = CountArgs<TArgs...>;
        static constexpr auto value = MRB_ARGS_ARG(count::req, count::opt);

    };

    template <class ... TArgs> struct ArgType {

        static constexpr auto value = ArgTypeNonZero<TArgs...>::value;

    };

    template <> struct ArgType<> {

        static constexpr auto value = MRB_ARGS_NONE();

    };

}