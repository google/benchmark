#ifndef BENCHMARK_EXPORT_H
#define BENCHMARK_EXPORT_H

#ifdef BENCHMARK_STATIC_DEFINE
#  define BENCHMARK_EXPORT
#  define BENCHMARK_NO_EXPORT
#else
#  ifndef BENCHMARK_EXPORT
#    ifdef benchmark_EXPORTS
        /* We are building this library */
#      define BENCHMARK_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define BENCHMARK_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef BENCHMARK_NO_EXPORT
#    define BENCHMARK_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef BENCHMARK_DEPRECATED
#  define BENCHMARK_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef BENCHMARK_DEPRECATED_EXPORT
#  define BENCHMARK_DEPRECATED_EXPORT BENCHMARK_EXPORT BENCHMARK_DEPRECATED
#endif

#ifndef BENCHMARK_DEPRECATED_NO_EXPORT
#  define BENCHMARK_DEPRECATED_NO_EXPORT BENCHMARK_NO_EXPORT BENCHMARK_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef BENCHMARK_NO_DEPRECATED
#    define BENCHMARK_NO_DEPRECATED
#  endif
#endif

#endif /* BENCHMARK_EXPORT_H */

