#ifndef BENCHMARK_VALUE_BENCHMARK_H
#define BENCHMARK_VALUE_BENCHMARK_H

#if __cplusplus < 201103L
#error Variable benchmarks can only be used in C++ >= 11
#endif

#include "macros.h"
#include "benchmark_api.h"

#include <functional>
#include <tuple>
#include <utility>


namespace benchmark {
namespace internal {

template <class T, T ...Indexes>
struct IntegerSequence {
    typedef IntegerSequence type;
};

template <std::size_t ...Indexes>
using IndexSequence = IntegerSequence<std::size_t, Indexes...>;

////////////////////////////////////////////////////////////////////////
template <class LHS, class RHS>
struct integer_seq_join;

template <class T, T ...Vs0, T ...Vs1>
struct integer_seq_join<
      IntegerSequence<T, Vs0...>
    , IntegerSequence<T, Vs1...>
    >
{
    using type = IntegerSequence<T, Vs0..., (Vs1 + sizeof...(Vs0))...>;
};

////////////////////////////////////////////////////////////////////////
template <class T, std::size_t N>
struct integer_seq_impl
    : integer_seq_join<
        typename integer_seq_impl<T, (N / 2)>::type
      , typename integer_seq_impl<T, (N - (N/2))>::type
      >
{};

template <class T>
struct integer_seq_impl<T, 0>
{
    using type = IntegerSequence<T>;
};

template <class T>
struct integer_seq_impl<T, 1>
{
    using type = IntegerSequence<T, 0>;
};

template <std::size_t N>
using MakeIndexSequence = typename integer_seq_impl<std::size_t, N>::type;

#define BENCHMARK_AUTO_RETURN(...) \
  -> decltype(__VA_ARGS__) { return __VA_ARGS__; }

template <class Fn, class Tuple, std::size_t ...Id>
inline
auto ApplyTupleImp(Fn && f, Tuple && t, IndexSequence<Id...>)
BENCHMARK_AUTO_RETURN(
    std::forward<Fn>(f)(std::get<Id>(std::forward<Tuple>(t))...)
)

template <class Fn, class Tuple>
inline
auto ApplyTuple(Fn && f, Tuple && t)
BENCHMARK_AUTO_RETURN(
    ::benchmark::internal::ApplyTupleImp(
        std::forward<Fn>(f), std::forward<Tuple>(t),
        MakeIndexSequence<
            std::tuple_size<typename std::decay<Tuple>::type>::value
        >())
)

#undef BENCHMARK_AUTO_RETURN

} // end namespace internal


template <class Fn>
class ValueFunctionBenchmark;

template <class ...Args>
class ValueFunctionBenchmark<void(State&, Args...)>
    : public internal::Benchmark
{
public:
    template <class Fn, class ...Args2>
    ValueFunctionBenchmark(const char* name, Fn fn, Args2&&... args2)
        : internal::Benchmark(name), func_(fn),
          args_(std::forward<Args2&&>(args2)...)
    {}

    virtual void Run(State& st) {
        ::benchmark::internal::ApplyTuple(
            func_, std::tuple_cat(std::forward_as_tuple(st), args_)
        );
    }

private:
    using Func = void(State&, Args...);
    using TupleT = std::tuple<typename std::decay<Args>::type...>;
    std::function<Func> func_;
    TupleT args_;
};

template <class Fn, class ...Args>
inline
internal::Benchmark* CreateValueBenchmark(const char* name,
    Fn && func, Args&&... args)
{
    using Func = typename std::remove_pointer<
                    typename std::remove_reference<Fn>::type
                >::type;
    auto bench = new ValueFunctionBenchmark<Func>(
                        name, std::forward<Func>(func),
                        std::forward<Args>(args)...);
    return internal::RegisterBenchmarkInternal(bench);
}

} // namespace benchmark

#define BENCHMARK_V(fn, ...)                   \
    BENCHMARK_PRIVATE_DECLARE(fn) =            \
        (::benchmark::CreateValueBenchmark( \
            #fn "<" #__VA_ARGS__ ">", fn, __VA_ARGS__))


#endif // BENCHMARK_VALUE_BENCHMARK_H
