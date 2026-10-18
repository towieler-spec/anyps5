
import subprocess
import sys
import unittest
from pathlib import Path


class GitErrorHandlingTests(unittest.TestCase):
    def test_invalid_base_branch(self):
        script = Path(__file__).with_name("check_conventions.py")
        result = subprocess.run(
            [
                sys.executable,
                str(script),
                "--base",
                "this-branch-does-not-exist",
            ],
            capture_output=True,
            text=True,
        )

        self.assertEqual(result.returncode, 2)
        self.assertIn("Git command failed:", result.stderr)
        self.assertNotIn("Traceback", result.stderr)

    def test_invalid_head_branch(self):
        script = Path(__file__).with_name("check_conventions.py")
        result = subprocess.run(
            [
                sys.executable,
                str(script),
                "--head",
                "this-branch-does-not-exist",
            ],
            capture_output=True,
            text=True,
        )

        self.assertEqual(result.returncode, 2)
        self.assertIn("Git command failed:", result.stderr)
        self.assertNotIn("Traceback", result.stderr)


if __name__ == "__main__":
    unittest.main()
