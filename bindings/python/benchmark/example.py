"""Example of Python using C++ benchmark framework."""

import benchmark


@benchmark.register
def empty(state):
  while state:
    pass


@benchmark.register
def sum_million(state):
  while state:
    sum(range(1_000_000))


if __name__ == '__main__':
  benchmark.main()
