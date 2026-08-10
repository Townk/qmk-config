#!/usr/bin/env python3
"""Run the host test suite for this userspace.

    python3 tests/run_tests.py

Compiles the code under test into a shared library and drives it directly --
no firmware build, no flashing, no keyboard attached. Exits non-zero on
failure so it can gate a commit or a CI job.
"""

import os
import sys
import unittest

if __name__ == "__main__":
    tests_dir = os.path.dirname(os.path.abspath(__file__))
    sys.path.insert(0, tests_dir)

    suite = unittest.TestLoader().discover(tests_dir, pattern="test_*.py")
    result = unittest.TextTestRunner(verbosity=2).run(suite)

    sys.exit(0 if result.wasSuccessful() else 1)
