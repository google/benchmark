"""Tests for benchmark execution and output-file ownership."""

import io
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from gbench import util

OUTPUT_PATH = """
import sys
output = next(arg.split('=', 1)[1] for arg in sys.argv[1:]
              if arg.startswith('--benchmark_out='))
"""
VALID_OUTPUT = (
    OUTPUT_PATH
    + """
import json
with open(output, 'w') as stream:
    json.dump({'benchmarks': []}, stream)
"""
)


class TestRunBenchmark(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.output_file = None
        mkstemp = tempfile.mkstemp

        def create_output():
            descriptor, filename = mkstemp(dir=self.directory.name)
            self.output_file = filename
            return descriptor, filename

        output_patch = patch.object(
            util.tempfile, "mkstemp", side_effect=create_output
        )
        output_patch.start()
        self.addCleanup(output_patch.stop)
        stdout_patch = patch("sys.stdout", new_callable=io.StringIO)
        stdout_patch.start()
        self.addCleanup(stdout_patch.stop)

    def assert_temporary_output_removed(self):
        self.assertIsNotNone(self.output_file)
        self.assertFalse(os.path.exists(self.output_file))

    def test_removes_temporary_output_after_success(self):
        flags = ["-c", VALID_OUTPUT]
        original_flags = list(flags)
        result = util.run_benchmark(sys.executable, flags)

        self.assertEqual(result, {"benchmarks": []})
        self.assertEqual(flags, original_flags)
        self.assert_temporary_output_removed()

    def test_removes_temporary_output_after_nonzero_exit(self):
        with self.assertRaises(SystemExit) as error:
            util.run_benchmark(sys.executable, ["-c", "raise SystemExit(7)"])

        self.assertEqual(error.exception.code, 7)
        self.assert_temporary_output_removed()

    def test_removes_temporary_output_after_launch_failure(self):
        missing_executable = str(
            Path(self.directory.name) / "missing-benchmark"
        )
        with self.assertRaises(FileNotFoundError):
            util.run_benchmark(missing_executable, [])

        self.assert_temporary_output_removed()

    def test_removes_temporary_output_after_invalid_json(self):
        script = (
            OUTPUT_PATH
            + """
with open(output, 'w') as stream:
    stream.write('invalid JSON')
"""
        )
        with self.assertRaises(json.JSONDecodeError):
            util.run_benchmark(sys.executable, ["-c", script])

        self.assert_temporary_output_removed()

    def test_removes_temporary_output_after_unsupported_schema(self):
        script = (
            OUTPUT_PATH
            + """
import json
with open(output, 'w') as stream:
    json.dump({'context': {'json_schema_version': 2}, 'benchmarks': []}, stream)
"""
        )
        with self.assertRaises(SystemExit) as error:
            util.run_benchmark(sys.executable, ["-c", script])

        self.assertEqual(error.exception.code, 1)
        self.assert_temporary_output_removed()

    def test_preserves_explicit_output_after_failure(self):
        output = Path(self.directory.name) / "user-output.json"
        output.write_text("diagnostic output", encoding="utf-8")
        with self.assertRaises(SystemExit) as error:
            util.run_benchmark(
                sys.executable,
                ["-c", "raise SystemExit(7)", f"--benchmark_out={output}"],
            )

        self.assertEqual(error.exception.code, 7)
        self.assertEqual(
            output.read_text(encoding="utf-8"), "diagnostic output"
        )
        self.assertIsNone(self.output_file)

    def test_preserves_explicit_output_after_success(self):
        output = Path(self.directory.name) / "user-output.json"
        result = util.run_benchmark(
            sys.executable, ["-c", VALID_OUTPUT, f"--benchmark_out={output}"]
        )

        self.assertEqual(result, {"benchmarks": []})
        self.assertEqual(json.loads(output.read_text(encoding="utf-8")), result)
        self.assertIsNone(self.output_file)

    def test_preserves_exit_code_if_benchmark_removes_temporary_output(self):
        script = (
            OUTPUT_PATH
            + """
import os
os.unlink(output)
raise SystemExit(7)
"""
        )
        with self.assertRaises(SystemExit) as error:
            util.run_benchmark(sys.executable, ["-c", script])

        self.assertEqual(error.exception.code, 7)
        self.assert_temporary_output_removed()
