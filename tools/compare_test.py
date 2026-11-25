"""Unit tests for compare.py."""

import unittest

import compare


def load_tests(
    loader: unittest.TestLoader,
    standard_tests: unittest.TestSuite,
    pattern: str | None,
) -> unittest.TestSuite:
    del standard_tests, pattern
    return loader.loadTestsFromModule(compare)


if __name__ == "__main__":
    unittest.main()
