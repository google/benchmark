#!/bin/bash
# Verification script for test argument integration

set -e

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$TEST_DIR"

echo "Verifying test argument integration..."
echo ""

# Check that default_arguments.h exists
echo "1. Checking default_arguments.h exists..."
if [ -f "default_arguments.h" ]; then
    echo "   ✓ default_arguments.h found"
else
    echo "   ✗ default_arguments.h not found"
    exit 1
fi

# Find all tests with custom main() functions
echo ""
echo "2. Finding tests with custom main() functions..."
TESTS_WITH_MAIN=$(grep -l "^int main" *_test.cc 2>/dev/null | wc -l)
echo "   Found $TESTS_WITH_MAIN tests with custom main()"

# Check that all tests with main() include default_arguments.h
echo ""
echo "3. Verifying all tests include default_arguments.h..."
MISSING_INCLUDE=0
for f in *_test.cc; do
    if grep -q "^int main" "$f" 2>/dev/null; then
        if ! grep -q "#include \"default_arguments.h\"" "$f" 2>/dev/null; then
            echo "   ✗ $f is missing #include \"default_arguments.h\""
            MISSING_INCLUDE=$((MISSING_INCLUDE + 1))
        fi
    fi
done

if [ $MISSING_INCLUDE -eq 0 ]; then
    echo "   ✓ All tests include default_arguments.h"
else
    echo "   ✗ $MISSING_INCLUDE tests are missing the include"
    exit 1
fi

# Check that all tests with main() call AddTestArguments
echo ""
echo "4. Verifying all tests call AddTestArguments()..."
MISSING_CALL=0
for f in *_test.cc; do
    if grep -q "^int main" "$f" 2>/dev/null; then
        if ! grep -q "AddTestArguments" "$f" 2>/dev/null; then
            echo "   ✗ $f does not call AddTestArguments()"
            MISSING_CALL=$((MISSING_CALL + 1))
        fi
    fi
done

if [ $MISSING_CALL -eq 0 ]; then
    echo "   ✓ All tests call AddTestArguments()"
else
    echo "   ✗ $MISSING_CALL tests are missing AddTestArguments() call"
    exit 1
fi

# Check that CMakeLists.txt has been updated
echo ""
echo "5. Checking CMakeLists.txt for removed explicit arguments..."
EXPLICIT_MIN_TIME=$(grep -c "benchmark_min_time=0.01s" CMakeLists.txt 2>/dev/null || true)
if [ $EXPLICIT_MIN_TIME -eq 0 ]; then
    echo "   ✓ No explicit --benchmark_min_time=0.01s in most test commands"
else
    echo "   ⚠ Warning: Found $EXPLICIT_MIN_TIME instances of --benchmark_min_time=0.01s"
    echo "     (Some may be intentional, e.g., in filter_test macro)"
fi

# Check that BUILD file has been updated
echo ""
echo "6. Checking BUILD file for empty TEST_ARGS..."
if [ -f "BUILD" ]; then
    if grep -q "TEST_ARGS = \[\]" BUILD 2>/dev/null; then
        echo "   ✓ TEST_ARGS is empty in BUILD file"
    else
        echo "   ⚠ Warning: TEST_ARGS might not be empty"
    fi

    if grep -q "PER_SRC_TEST_ARGS = {}" BUILD 2>/dev/null; then
        echo "   ✓ PER_SRC_TEST_ARGS is empty in BUILD file"
    else
        echo "   ⚠ Warning: PER_SRC_TEST_ARGS might not be empty"
    fi
else
    echo "   ⚠ BUILD file not found (this is OK if not using Bazel)"
fi

# Summary
echo ""
echo "=========================================="
echo "Verification Summary"
echo "=========================================="
echo "Tests with custom main():     $TESTS_WITH_MAIN"
echo "Tests with AddTestArguments(): $(grep -l "AddTestArguments" *_test.cc 2>/dev/null | wc -l)"
echo ""
echo "✓ All checks passed!"
echo ""
echo "Integration complete. Tests are now self-contained."
