#!/usr/bin/env python3
"""Enforce graduated line limits for C and C++ source files."""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Sequence


DEFAULT_LIMIT = 100
LIMIT_STEP = 100
HARD_LIMIT = 500
SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"})
EXCLUDED_DIRECTORIES = frozenset({".git", "build", "third_party", "vendor"})


class ConfigurationError(ValueError):
    """Raised when the line-limit configuration is invalid."""


@dataclass(frozen=True)
class Override:
    max_lines: int
    reason: str


def parse_arguments(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Check C/C++ files against a 100-line default limit. "
            "Documented exceptions may raise the limit in 100-line steps, "
            "up to 500 lines."
        )
    )
    parser.add_argument(
        "paths",
        nargs="*",
        type=Path,
        default=[Path(".")],
        help="files or directories to inspect (default: current directory)",
    )
    parser.add_argument(
        "--config",
        type=Path,
        default=Path(".line-limits.json"),
        help="exception configuration (default: .line-limits.json)",
    )
    return parser.parse_args(argv)


def load_overrides(config_path: Path, project_root: Path) -> dict[str, Override]:
    try:
        raw_config = json.loads(config_path.read_text(encoding="utf-8"))
    except FileNotFoundError as error:
        raise ConfigurationError(f"configuration file not found: {config_path}") from error
    except json.JSONDecodeError as error:
        raise ConfigurationError(
            f"invalid JSON in {config_path}:{error.lineno}:{error.colno}: {error.msg}"
        ) from error
    except OSError as error:
        raise ConfigurationError(f"cannot read configuration file {config_path}: {error}") from error

    if not isinstance(raw_config, dict):
        raise ConfigurationError("configuration root must be a JSON object")

    unknown_keys = set(raw_config) - {"overrides"}
    if unknown_keys:
        keys = ", ".join(sorted(unknown_keys))
        raise ConfigurationError(f"unknown configuration key(s): {keys}")

    raw_overrides = raw_config.get("overrides", {})
    if not isinstance(raw_overrides, dict):
        raise ConfigurationError("'overrides' must be a JSON object")

    overrides: dict[str, Override] = {}
    for raw_path, raw_override in raw_overrides.items():
        path = _validate_override_path(raw_path, project_root)
        override = _validate_override(raw_path, raw_override)
        overrides[path] = override

    return overrides


def _validate_override_path(raw_path: object, project_root: Path) -> str:
    if not isinstance(raw_path, str) or not raw_path.strip():
        raise ConfigurationError("override paths must be non-empty strings")

    candidate = Path(raw_path)
    if candidate.is_absolute() or ".." in candidate.parts:
        raise ConfigurationError(f"override path must be relative to the project: {raw_path!r}")

    normalized = candidate.as_posix()
    target = project_root / candidate
    if not target.is_file():
        raise ConfigurationError(f"override target is not a file: {normalized}")

    return normalized


def _validate_override(raw_path: object, raw_override: object) -> Override:
    if not isinstance(raw_override, dict):
        raise ConfigurationError(f"override for {raw_path!r} must be a JSON object")

    unknown_keys = set(raw_override) - {"max_lines", "reason"}
    if unknown_keys:
        keys = ", ".join(sorted(unknown_keys))
        raise ConfigurationError(f"unknown key(s) for {raw_path!r}: {keys}")

    max_lines = raw_override.get("max_lines")
    if isinstance(max_lines, bool) or not isinstance(max_lines, int):
        raise ConfigurationError(f"max_lines for {raw_path!r} must be an integer")
    if max_lines <= DEFAULT_LIMIT or max_lines > HARD_LIMIT or max_lines % LIMIT_STEP != 0:
        raise ConfigurationError(
            f"max_lines for {raw_path!r} must be one of 200, 300, 400, or 500"
        )

    reason = raw_override.get("reason")
    if not isinstance(reason, str) or not reason.strip():
        raise ConfigurationError(f"reason for {raw_path!r} must be a non-empty string")

    return Override(max_lines=max_lines, reason=reason.strip())


def discover_source_files(paths: Sequence[Path], project_root: Path) -> list[Path]:
    discovered: set[Path] = set()

    for raw_path in paths:
        path = raw_path if raw_path.is_absolute() else project_root / raw_path
        if not path.exists():
            raise ConfigurationError(f"path does not exist: {raw_path}")

        if path.is_file():
            _add_source_file(path, project_root, discovered)
            continue

        if not path.is_dir():
            raise ConfigurationError(f"path is neither a file nor a directory: {raw_path}")

        for candidate in path.rglob("*"):
            if candidate.is_file():
                _add_source_file(candidate, project_root, discovered)

    return sorted(discovered, key=lambda path: path.relative_to(project_root).as_posix())


def _add_source_file(path: Path, project_root: Path, discovered: set[Path]) -> None:
    try:
        relative_path = path.resolve().relative_to(project_root)
    except ValueError as error:
        raise ConfigurationError(f"source path is outside the project: {path}") from error

    if path.suffix.lower() not in SOURCE_SUFFIXES:
        return
    if any(part in EXCLUDED_DIRECTORIES for part in relative_path.parts[:-1]):
        return

    discovered.add(path.resolve())


def count_lines(path: Path) -> int:
    try:
        return len(path.read_text(encoding="utf-8").splitlines())
    except UnicodeDecodeError as error:
        raise ConfigurationError(f"source file is not valid UTF-8: {path}") from error
    except OSError as error:
        raise ConfigurationError(f"cannot read source file {path}: {error}") from error


def check_files(
    source_files: Sequence[Path], project_root: Path, overrides: dict[str, Override]
) -> list[str]:
    violations: list[str] = []

    for path in source_files:
        relative_path = path.relative_to(project_root).as_posix()
        line_count = count_lines(path)
        limit = overrides.get(relative_path, Override(DEFAULT_LIMIT, "")).max_lines
        if line_count > limit:
            violations.append(f"{relative_path}: {line_count} lines (limit: {limit})")

    return violations


def main(argv: Sequence[str] | None = None) -> int:
    arguments = parse_arguments(argv)
    config_path = arguments.config.resolve()
    project_root = config_path.parent

    try:
        overrides = load_overrides(config_path, project_root)
        source_files = discover_source_files(arguments.paths, project_root)
        violations = check_files(source_files, project_root, overrides)
    except ConfigurationError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    if violations:
        print("Line limit violations:", file=sys.stderr)
        for violation in violations:
            print(f"  - {violation}", file=sys.stderr)
        print(
            "Add a justified 100-line-step override to .line-limits.json, "
            "or split the file. The absolute maximum is 500 lines.",
            file=sys.stderr,
        )
        return 1

    print(f"Line limit check passed ({len(source_files)} files checked).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
