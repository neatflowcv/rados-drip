from __future__ import annotations

import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPT_PATH = Path(__file__).parents[1] / "scripts" / "check_file_lines.py"
SPEC = importlib.util.spec_from_file_location("check_file_lines", SCRIPT_PATH)
assert SPEC is not None
assert SPEC.loader is not None
check_file_lines = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = check_file_lines
SPEC.loader.exec_module(check_file_lines)


class CheckFileLinesTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)
        self.config = self.root / ".line-limits.json"

    def tearDown(self) -> None:
        self.temporary_directory.cleanup()

    def write_source(self, relative_path: str, line_count: int) -> Path:
        path = self.root / relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("int value;\n" * line_count, encoding="utf-8")
        return path

    def write_config(self, overrides: dict[str, object]) -> None:
        self.config.write_text(json.dumps({"overrides": overrides}), encoding="utf-8")

    def test_default_limit_accepts_exactly_100_lines(self) -> None:
        source = self.write_source("app/drip.cc", 100)
        self.write_config({})

        overrides = check_file_lines.load_overrides(self.config, self.root)
        violations = check_file_lines.check_files([source], self.root, overrides)

        self.assertEqual(violations, [])

    def test_default_limit_rejects_101_lines(self) -> None:
        source = self.write_source("app/drip.cc", 101)
        self.write_config({})

        overrides = check_file_lines.load_overrides(self.config, self.root)
        violations = check_file_lines.check_files([source], self.root, overrides)

        self.assertEqual(violations, ["app/drip.cc: 101 lines (limit: 100)"])

    def test_justified_override_raises_limit(self) -> None:
        source = self.write_source("client/client.cc", 200)
        self.write_config(
            {
                "client/client.cc": {
                    "max_lines": 200,
                    "reason": "Keeps one cohesive protocol implementation together.",
                }
            }
        )

        overrides = check_file_lines.load_overrides(self.config, self.root)
        violations = check_file_lines.check_files([source], self.root, overrides)

        self.assertEqual(violations, [])

    def test_override_must_use_a_100_line_step(self) -> None:
        self.write_source("client/client.cc", 101)
        self.write_config(
            {
                "client/client.cc": {
                    "max_lines": 250,
                    "reason": "Invalid increment.",
                }
            }
        )

        with self.assertRaisesRegex(check_file_lines.ConfigurationError, "must be one of"):
            check_file_lines.load_overrides(self.config, self.root)

    def test_override_requires_a_reason(self) -> None:
        self.write_source("client/client.cc", 101)
        self.write_config(
            {"client/client.cc": {"max_lines": 200, "reason": "  "}}
        )

        with self.assertRaisesRegex(check_file_lines.ConfigurationError, "non-empty"):
            check_file_lines.load_overrides(self.config, self.root)

    def test_override_cannot_exceed_500_lines(self) -> None:
        self.write_source("client/client.cc", 101)
        self.write_config(
            {
                "client/client.cc": {
                    "max_lines": 600,
                    "reason": "Too large.",
                }
            }
        )

        with self.assertRaisesRegex(check_file_lines.ConfigurationError, "must be one of"):
            check_file_lines.load_overrides(self.config, self.root)

    def test_500_line_override_rejects_501_lines(self) -> None:
        source = self.write_source("app/delete.cc", 501)
        self.write_config(
            {
                "app/delete.cc": {
                    "max_lines": 500,
                    "reason": "Largest permitted cohesive implementation.",
                }
            }
        )

        overrides = check_file_lines.load_overrides(self.config, self.root)
        violations = check_file_lines.check_files([source], self.root, overrides)

        self.assertEqual(violations, ["app/delete.cc: 501 lines (limit: 500)"])


if __name__ == "__main__":
    unittest.main()
