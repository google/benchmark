"""Python benchmarking utilities."""

from absl import app
from benchmark import _benchmark

__all__ = [
    "register",
    "main",
]

__version__ = "0.1.0"


def register(f=None, *, name=None):
  if f is None:
    return lambda f: register(f, name=name)
  if name is None:
    name = f.__name__
  _benchmark.RegisterBenchmark(name, f)
  return f


def _flags_parser(argv):
  argv = _benchmark.Initialize(argv)
  return app.parse_flags_with_usage(argv)


def _run_benchmarks(argv):
  if len(argv) > 1:
    raise app.UsageError('Too many command-line arguments.')
  return _benchmark.RunSpecifiedBenchmarks()


def main(argv=None):
  return app.run(_run_benchmarks, argv=argv, flags_parser=_flags_parser)
