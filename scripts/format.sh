#!/usr/bin/env bash
# Formats (or, with --check, verifies) every hand-written C source and header.
#
# Generated sources are excluded by path: they live under the build directory and are never
# committed, so reformatting them would be pure churn.
#
# Usage: scripts/format.sh [--check]

set -euo pipefail

cd "$(dirname "$0")/.."

command -v clang-format >/dev/null || {
	echo "clang-format not found." >&2
	echo "  macOS/Linux, no root:  python3 -m venv .venv && .venv/bin/pip install clang-format==22.1.8" >&2
	exit 1
}

formatter_version="$(clang-format --version)"
if [[ ! ${formatter_version} =~ version[[:space:]]([0-9]+)\. ]]; then
	echo "Could not determine clang-format's major version from: ${formatter_version}" >&2
	exit 1
fi
formatter_major="${BASH_REMATCH[1]}"
if (( formatter_major < 22 )); then
	echo "clang-format 22 or newer is required; found ${formatter_version}" >&2
	exit 1
fi

# A plain read loop rather than mapfile: macOS still ships bash 3.2, where mapfile does not exist
# and the script would silently lint nothing.
files=()
while IFS= read -r file; do
	files+=("${file}")
done < <(git ls-files '*.c' '*.h' | grep -Ev '^lib/|src/shaders/')
(( ${#files[@]} > 0 )) || { echo "no tracked C sources found" >&2; exit 1; }

if [[ ${1:-} == "--check" ]]; then
	# --dry-run --Werror is the only form that exits non-zero; -n alone just prints.
	clang-format --dry-run --Werror "${files[@]}"
	echo "clang-format ${formatter_major}: ${#files[@]} file(s) correctly formatted"
else
	clang-format -i "${files[@]}"
	echo "clang-format ${formatter_major}: formatted ${#files[@]} file(s)"
fi
